/*
 * awesome-personal-list-lang-checker - fills in the language for every
 * source that doesn't have one yet, via the GitHub/GitLab REST APIs.
 * Standalone (no ecewo), run by a systemd timer -- same shape as
 * checker_main.c, but writes straight into each source's own JSON record
 * via source_write() rather than a shared cache file, since language (unlike
 * Software Heritage status) belongs to the record itself. Sources that
 * already have a language -- typed by hand or detected earlier -- are never
 * touched again: this is a one-shot "fill it in if missing" pass, not a
 * resync.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app.h"
#include "classify.h"
#include "envfile.h"
#include "lang.h"
#include "models.h"
#include "strutil.h"
#include "xmalloc.h"

struct app_config app_cfg;

static void
load_config(void)
{
	const char	*env;

	load_env_file(".env");
	load_env_file(SYSTEM_CONFIG_FILE);

	if ((env = getenv("GITHUB_TOKEN")) != NULL)
		str_lcpy(app_cfg.github_token, env, sizeof(app_cfg.github_token));

	if ((env = getenv("GITLAB_TOKEN")) != NULL)
		str_lcpy(app_cfg.gitlab_token, env, sizeof(app_cfg.gitlab_token));
}

/* Same shape as source_controller.c's on-the-fly category creation --
 * classify by the language just detected. Only ever adds categories,
 * never removes. */
static void
reclassify(struct source *s)
{
	struct classified_category	suggested[CLASSIFY_MAX_CATEGORIES];
	int				nsuggested, i;

	nsuggested = classify_source(s->language, suggested,
	    CLASSIFY_MAX_CATEGORIES);

	for (i = 0; i < nsuggested; i++) {
		struct category	c;

		if (category_read(suggested[i].slug, &c) == -1) {
			memset(&c, 0, sizeof(c));
			str_lcpy(c.slug, suggested[i].slug, sizeof(c.slug));
			str_lcpy(c.name, suggested[i].name, sizeof(c.name));
			category_write(&c);
		}
		source_add_category(s, suggested[i].slug);
	}
}

int
main(void)
{
	struct source	*srcs;
	size_t		 n, i;
	char		 detected[NAME_MAX_LEN];
	const char	*canon;
	long		 rate_remaining;
	int		 filled, skipped, missed;

	load_config();

	if (store_verify_dirs() == -1)
		fatal("data directories missing");

	if (lang_init() == -1)
		fatal("curl_global_init failed");

	if (source_list(&srcs, &n) != 0)
		fatal("could not list sources");

	app_log(LOG_INFO, "checking language for up to %zu sources", n);

	filled = skipped = missed = 0;

	for (i = 0; i < n; i++) {
		struct source	*s = &srcs[i];

		if (s->language[0] != '\0') {
			skipped++;
			continue;
		}

		if (lang_detect(s->original_url, app_cfg.github_token,
		    app_cfg.gitlab_token, detected, sizeof(detected),
		    &rate_remaining) == -1) {
			missed++;
			continue;
		}

		canon = classify_canonical_language(detected);
		str_lcpy(s->language, canon != NULL ? canon : detected,
		    sizeof(s->language));
		s->updated_at = time(NULL);

		reclassify(s);
		source_write(s);
		filled++;

		if (rate_remaining >= 0 && rate_remaining <= 1) {
			app_log(LOG_INFO,
			    "rate limit nearly exhausted, stopping this run");
			break;
		}

		/* Polite fixed pacing; the rate-limit check above is the
		 * real guard. */
		nanosleep(&(struct timespec){ .tv_nsec = 200000000 }, NULL);
	}

	app_log(LOG_INFO, "filled=%d skipped=%d missed=%d (of %zu)",
	    filled, skipped, missed, n);

	source_list_free(srcs);
	lang_cleanup();

	return (0);
}
