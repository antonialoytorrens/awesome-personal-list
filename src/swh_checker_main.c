/*
 * awesome-personal-list-swh-checker - re-checks Software Heritage status for
 * every curated source. Standalone (no ecewo): run by a systemd timer,
 * resumable across runs. Stops for the run once the rate limit is nearly
 * exhausted rather than sleeping for hours in-process; the next timer tick
 * picks up where the cache left off.
 *
 * Each origin is rechecked on an exponential backoff (1 day → 90 days)
 * keyed off checked_at + check_count in data/swh_cache.tsv, so not-yet-
 * archived projects keep being polled without hitting the API every hour.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app.h"
#include "check_backoff.h"
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
	unsigned		check_count;
	time_t			checked_at, now;

	load_config();

	if (store_verify_dirs() == -1)
		fatal("data directories missing");

	if (swh_init() == -1)
		fatal("curl_global_init failed");

	memset(&set, 0, sizeof(set));
	collect_urls(&set);

	now = time(NULL);
	app_log(LOG_INFO, "checking %zu distinct origins", set.count);

	checked = skipped = failed = 0;

	for (i = 0; i < set.count; i++) {
		int	have_cache;

		have_cache = (swh_cache_get(set.urls[i], &status, &checked_at,
		    &check_count) == 0);
		if (have_cache &&
		    !check_backoff_due(checked_at, check_count, now)) {
			skipped++;
			continue;
		}

		if (swh_check_origin(set.urls[i],
		    app_cfg.swh_token[0] ? app_cfg.swh_token : NULL,
		    &result) == -1) {
			failed++;
			continue;
		}

		if (have_cache && status == result.status)
			check_count++;
		else
			check_count = 0;

		swh_cache_set(set.urls[i], result.status, now, check_count);
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
