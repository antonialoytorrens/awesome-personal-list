/*
 * awesome-personal-list - the single admin account.
 * data/admin.tsv: name \t algo \t salt \t hash \t created \t language
 *
 * No registration route exists. The one row is seeded at first run from
 * AWESOME_PERSONAL_LIST_ADMIN_USER / AWESOME_PERSONAL_LIST_ADMIN_PASSWORD. `language` is a
 * later addition: rows written before it have only 5 fields and default to
 * "en" on read, so no migration step is needed.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <argon2.h>

#include "app.h"
#include "models.h"
#include "strutil.h"
#include "wbuf.h"
#include "xmalloc.h"

struct argon2_params {
	uint32_t	m_cost;
	uint32_t	t_cost;
	uint32_t	parallelism;
};

static const struct argon2_params argon2_current = {
	.m_cost = ARGON2_MEMORY_KIB,
	.t_cost = ARGON2_ITERATIONS,
	.parallelism = ARGON2_PARALLELISM,
};

static void
random_bytes(uint8_t *buf, size_t len)
{
	int	fd;
	ssize_t	r;
	size_t	off = 0;

	if ((fd = open("/dev/urandom", O_RDONLY)) == -1)
		fatal("open(/dev/urandom): %s", strerror(errno));

	while (off < len) {
		r = read(fd, buf + off, len - off);
		if (r <= 0) {
			if (r == -1 && errno == EINTR)
				continue;
			fatal("read(/dev/urandom): %s", strerror(errno));
		}
		off += (size_t)r;
	}

	close(fd);
}

void
token_generate(char *out, size_t len)
{
	uint8_t	raw[TOKEN_BYTES];

	if (len < TOKEN_HEX_LEN)
		fatal("token_generate: buffer too small");

	random_bytes(raw, sizeof(raw));
	str_hex(raw, sizeof(raw), out, len);
}

static void
admin_algo_tag(const struct argon2_params *p, char *out, size_t len)
{
	if ((size_t)snprintf(out, len, "argon2id$v=%d$m=%u,t=%u,p=%u",
	    ARGON2_VERSION_NUMBER, p->m_cost, p->t_cost, p->parallelism) >= len)
		fatal("admin_algo_tag: buffer too small");
}

static int
admin_algo_parse(const char *tag, struct argon2_params *out)
{
	unsigned int	v, m, t, p;

	if (tag == NULL)
		return (-1);

	if (sscanf(tag, "argon2id$v=%u$m=%u,t=%u,p=%u", &v, &m, &t, &p) != 4)
		return (-1);

	if (v != ARGON2_VERSION_NUMBER)
		return (-1);

	if (m < 8 || m > (1024 * 1024) || t < 1 || t > 64 || p < 1 || p > 16)
		return (-1);

	out->m_cost = m;
	out->t_cost = t;
	out->parallelism = p;

	return (0);
}

static int
admin_hash(const struct argon2_params *p, const char *salt,
    const char *password, char *out, size_t len)
{
	uint8_t	raw[ARGON2_HASH_LEN];
	int	rc;

	if (len < (ARGON2_HASH_LEN * 2) + 1)
		fatal("admin_hash: buffer too small");

	rc = argon2id_hash_raw(p->t_cost, p->m_cost, p->parallelism,
	    password, strlen(password), salt, strlen(salt), raw, sizeof(raw));

	if (rc != ARGON2_OK) {
		app_log(LOG_ERR, "argon2id_hash_raw: %s",
		    argon2_error_message(rc));
		return (-1);
	}

	str_hex(raw, sizeof(raw), out, len);

	return (0);
}

static int
admin_parse(char *line, struct admin *out)
{
	char	*fields[6];
	int	 count, err;

	count = str_split(line, "\t", fields, 6);
	if (count < 5)
		return (-1);

	memset(out, 0, sizeof(*out));

	if (str_lcpy(out->name, fields[0], sizeof(out->name)) >=
	    sizeof(out->name))
		return (-1);
	if (str_lcpy(out->algo, fields[1], sizeof(out->algo)) >=
	    sizeof(out->algo))
		return (-1);
	if (str_lcpy(out->salt, fields[2], sizeof(out->salt)) >=
	    sizeof(out->salt))
		return (-1);
	if (str_lcpy(out->hash, fields[3], sizeof(out->hash)) >=
	    sizeof(out->hash))
		return (-1);

	out->created = (time_t)str_tonum(fields[4], 10, 0, INT64_MAX, &err);
	if (err != STR_OK)
		out->created = 0;

	if (count < 6 || str_lcpy(out->language, fields[5],
	    sizeof(out->language)) >= sizeof(out->language))
		str_lcpy(out->language, "en", sizeof(out->language));

	return (0);
}

int
admin_exists(void)
{
	struct admin	a;

	return (admin_lookup(NULL, &a) == 0);
}

int
admin_lookup(const char *name, struct admin *out)
{
	FILE		*fp;
	char		 line[1024];
	struct admin	 a;
	int		 found = -1;

	if ((fp = fopen(ADMIN_FILE, "r")) == NULL)
		return (-1);

	while (fgets(line, sizeof(line), fp) != NULL) {
		line[strcspn(line, "\r\n")] = '\0';
		if (line[0] == '\0' || line[0] == '#')
			continue;

		if (admin_parse(line, &a) == -1)
			continue;

		/* NULL name: "does any admin exist" -- return the first row. */
		if (name != NULL && strcmp(a.name, name))
			continue;

		if (out != NULL)
			*out = a;
		found = 0;
		break;
	}

	fclose(fp);
	return (found);
}

int
admin_create(const char *name, const char *password)
{
	FILE		*fp;
	struct admin	 a;

	if (name == NULL || *name == '\0')
		return (-1);
	if (password == NULL || strlen(password) < 8)
		return (-1);

	if (admin_exists())
		return (-2);	/* single-account app: refuse a second row */

	memset(&a, 0, sizeof(a));
	str_lcpy(a.name, name, sizeof(a.name));
	admin_algo_tag(&argon2_current, a.algo, sizeof(a.algo));
	token_generate(a.salt, sizeof(a.salt));

	if (admin_hash(&argon2_current, a.salt, password, a.hash,
	    sizeof(a.hash)) == -1)
		return (-1);

	a.created = time(NULL);
	str_lcpy(a.language, "en", sizeof(a.language));

	if ((fp = fopen(ADMIN_FILE, "w")) == NULL)
		return (-1);

	fprintf(fp, "%s\t%s\t%s\t%s\t%lld\t%s\n", a.name, a.algo, a.salt,
	    a.hash, (long long)a.created, a.language);

	return (fclose(fp) == 0 ? 0 : -1);
}

/* The only row-update path this model has: rewrites the single admin row
 * with a new language, same fopen(ADMIN_FILE, "w") shape as admin_create(). */
int
admin_update_language(const char *name, const char *lang)
{
	struct admin	a;
	FILE		*fp;

	if (admin_lookup(name, &a) == -1)
		return (-1);

	if (str_lcpy(a.language, lang, sizeof(a.language)) >= sizeof(a.language))
		return (-1);

	if ((fp = fopen(ADMIN_FILE, "w")) == NULL)
		return (-1);

	fprintf(fp, "%s\t%s\t%s\t%s\t%lld\t%s\n", a.name, a.algo, a.salt,
	    a.hash, (long long)a.created, a.language);

	return (fclose(fp) == 0 ? 0 : -1);
}

/* Same fopen(ADMIN_FILE, "w") shape as admin_update_language(): the single
 * admin row is looked up by its current name, mutated, and rewritten. */
int
admin_update_name(const char *old_name, const char *new_name)
{
	struct admin	a;
	FILE		*fp;

	if (admin_lookup(old_name, &a) == -1)
		return (-1);

	if (str_lcpy(a.name, new_name, sizeof(a.name)) >= sizeof(a.name))
		return (-1);

	if ((fp = fopen(ADMIN_FILE, "w")) == NULL)
		return (-1);

	fprintf(fp, "%s\t%s\t%s\t%s\t%lld\t%s\n", a.name, a.algo, a.salt,
	    a.hash, (long long)a.created, a.language);

	return (fclose(fp) == 0 ? 0 : -1);
}

int
admin_verify(const char *name, const char *password, struct admin *out)
{
	struct admin		a;
	struct argon2_params	p;
	char			hash[(ARGON2_HASH_LEN * 2) + 1];

	if (password == NULL || admin_lookup(name, &a) == -1)
		return (-1);

	if (admin_algo_parse(a.algo, &p) == -1)
		return (-1);

	if (admin_hash(&p, a.salt, password, hash, sizeof(hash)) == -1)
		return (-1);

	if (!str_timingsafe_equal(a.hash, hash))
		return (-1);

	if (out != NULL)
		*out = a;

	return (0);
}
