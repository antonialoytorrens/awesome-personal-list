/*
 * awesome-personal-list-checker - re-checks Software Heritage status for every
 * curated source. Standalone (no ecewo): run by a systemd timer, resumable
 * across runs. Stops for the run once the rate limit is nearly exhausted
 * rather than sleeping for hours in-process; the next timer tick picks up
 * where the cache left off.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app.h"
#include "envfile.h"
#include "models.h"
#include "strutil.h"
#include "swh.h"
#include "xmalloc.h"

struct app_config app_cfg;

struct url_set {
	char	**urls;
	size_t	  count;
	size_t	  cap;
};

static void
url_set_add(struct url_set *set, const char *url)
{
	size_t	i;

	for (i = 0; i < set->count; i++) {
		if (!strcmp(set->urls[i], url))
			return;
	}

	if (set->count == set->cap) {
		set->cap = set->cap ? set->cap * 2 : 256;
		set->urls = xrealloc(set->urls, set->cap * sizeof(*set->urls));
	}

	set->urls[set->count++] = xstrdup(url);
}

static void
load_config(void)
{
	const char	*env;

	load_env_file(".env");
	load_env_file(SYSTEM_CONFIG_FILE);

	if ((env = getenv("SWH_API_TOKEN")) != NULL)
		str_lcpy(app_cfg.swh_token, env, sizeof(app_cfg.swh_token));
}

/* Every original_url worth checking: every curated source. */
static void
collect_urls(struct url_set *set)
{
	struct source	*srcs;
	size_t		 n, i;

	if (source_list(&srcs, &n) == 0) {
		for (i = 0; i < n; i++)
			url_set_add(set, srcs[i].original_url);
		source_list_free(srcs);
	}
}

int
main(void)
{
	struct url_set		set;
	struct swh_result	result;
	size_t			i;
	int			status, checked, skipped, failed;
	time_t			checked_at;

	load_config();

	if (store_verify_dirs() == -1)
		fatal("data directories missing");

	if (swh_init() == -1)
		fatal("curl_global_init failed");

	memset(&set, 0, sizeof(set));
	collect_urls(&set);

	app_log(LOG_INFO, "checking %zu distinct origins", set.count);

	checked = skipped = failed = 0;

	for (i = 0; i < set.count; i++) {
		if (swh_cache_get(set.urls[i], &status, &checked_at) == 0 &&
		    status == SWH_STATUS_ARCHIVED) {
			skipped++;
			continue;
		}

		if (swh_check_origin(set.urls[i],
		    app_cfg.swh_token[0] ? app_cfg.swh_token : NULL,
		    &result) == -1) {
			failed++;
			continue;
		}

		swh_cache_set(set.urls[i], result.status, time(NULL));
		checked++;

		if (result.rate_remaining >= 0 && result.rate_remaining <= 1) {
			app_log(LOG_INFO,
			    "rate limit nearly exhausted, stopping this run");
			break;
		}

		/* Polite fixed pacing; the rate-limit check above is the real guard. */
		nanosleep(&(struct timespec){ .tv_nsec = 200000000 }, NULL);
	}

	app_log(LOG_INFO, "checked=%d skipped=%d failed=%d (of %zu)",
	    checked, skipped, failed, set.count);

	for (i = 0; i < set.count; i++)
		free(set.urls[i]);
	free(set.urls);

	swh_cleanup();

	return (0);
}
