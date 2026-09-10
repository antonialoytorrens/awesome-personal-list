/*
 * awesome-personal-list - shared exponential backoff for periodic checkers.
 */

#include "check_backoff.h"

time_t
check_backoff_interval(unsigned count)
{
	time_t		interval;
	unsigned	i;

	interval = CHECK_BACKOFF_BASE_SEC;
	for (i = 0; i < count; i++) {
		if (interval >= CHECK_BACKOFF_MAX_SEC / 2)
			return (CHECK_BACKOFF_MAX_SEC);
		interval *= 2;
	}
	if (interval > CHECK_BACKOFF_MAX_SEC)
		return (CHECK_BACKOFF_MAX_SEC);
	return (interval);
}

int
check_backoff_due(time_t checked_at, unsigned count, time_t now)
{
	if (checked_at == 0)
		return (1);
	return (now >= checked_at + check_backoff_interval(count));
}
