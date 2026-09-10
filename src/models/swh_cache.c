/*
 * awesome-personal-list - Software Heritage check-result cache.
 * data/swh_cache.tsv: original_url \t status \t checked_at \t check_count
 *
 * Shared by both binaries: the web app reads it when rendering, and
 * awesome-personal-list-swh-checker is the only writer. Keyed on
 * original_url rather than a source slug so a not-yet-imported candidate
 * can be checked too. Legacy 3-column rows (no check_count) parse as
 * check_count == 0.
 */

#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "models.h"
#include "strutil.h"
#include "wbuf.h"

struct swh_row {
	char		url[URL_MAX_LEN];
	int		status;
	time_t		checked_at;
	unsigned	check_count;
};

static int
swh_parse(char *line, struct swh_row *out)
{
	char	*fields[5];
	int	 count, err;

	count = str_split(line, "\t", fields, 5);
	if (count < 3)
		return (-1);

	memset(out, 0, sizeof(*out));

	if (str_lcpy(out->url, fields[0], sizeof(out->url)) >= sizeof(out->url))
		return (-1);

	out->status = (int)str_tonum(fields[1], 10, 0, 2, &err);
	if (err != STR_OK)
		return (-1);

	out->checked_at = (time_t)str_tonum(fields[2], 10, 0, INT64_MAX, &err);
	if (err != STR_OK)
		return (-1);

	if (count >= 4) {
		out->check_count = (unsigned)str_tonum(fields[3], 10, 0,
		    UINT32_MAX, &err);
		if (err != STR_OK)
			return (-1);
	}

	return (0);
}

int
swh_cache_get(const char *original_url, int *status, time_t *checked_at,
    unsigned *check_count)
{
	FILE		*fp;
	char		 line[URL_MAX_LEN + 96];
	struct swh_row	 row;

	*status = SWH_STATUS_UNKNOWN;
	*checked_at = 0;
	if (check_count != NULL)
		*check_count = 0;

	if ((fp = fopen(SWH_CACHE_FILE, "r")) == NULL)
		return (-1);

	while (fgets(line, sizeof(line), fp) != NULL) {
		line[strcspn(line, "\r\n")] = '\0';
		if (line[0] == '\0')
			continue;

		if (swh_parse(line, &row) == -1)
			continue;

		if (!strcmp(row.url, original_url)) {
			*status = row.status;
			*checked_at = row.checked_at;
			if (check_count != NULL)
				*check_count = row.check_count;
			fclose(fp);
			return (0);
		}
	}

	fclose(fp);
	return (-1);
}

int
swh_cache_set(const char *original_url, int status, time_t checked_at,
    unsigned check_count)
{
	FILE		*in;
	char		 line[URL_MAX_LEN + 96];
	struct wbuf	 buf;
	struct swh_row	 row;
	int		 ret, replaced;

	wbuf_init(&buf, 4096);
	replaced = 0;

	if ((in = fopen(SWH_CACHE_FILE, "r")) != NULL) {
		while (fgets(line, sizeof(line), in) != NULL) {
			line[strcspn(line, "\r\n")] = '\0';
			if (line[0] == '\0')
				continue;

			if (swh_parse(line, &row) == -1)
				continue;

			if (!strcmp(row.url, original_url)) {
				wbuf_appendf(&buf, "%s\t%d\t%lld\t%u\n",
				    original_url, status, (long long)checked_at,
				    check_count);
				replaced = 1;
				continue;
			}

			wbuf_appendf(&buf, "%s\t%d\t%lld\t%u\n", row.url,
			    row.status, (long long)row.checked_at,
			    row.check_count);
		}
		fclose(in);
	}

	if (!replaced) {
		wbuf_appendf(&buf, "%s\t%d\t%lld\t%u\n", original_url, status,
		    (long long)checked_at, check_count);
	}

	ret = store_write(SWH_CACHE_FILE, buf.data, buf.offset);
	wbuf_cleanup(&buf);

	return (ret);
}
