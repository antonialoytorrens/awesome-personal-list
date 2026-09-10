/*
 * awesome-personal-list - shared exponential backoff for the language and
 * Software Heritage checkers. Base 1 day, doubles per unchanged result,
 * capped at 90 days.
 */

#ifndef AWESOME_PERSONAL_LIST_CHECK_BACKOFF_H
#define AWESOME_PERSONAL_LIST_CHECK_BACKOFF_H

#include <time.h>

#define CHECK_BACKOFF_BASE_SEC	(24 * 60 * 60)		/* 1 day */
#define CHECK_BACKOFF_MAX_SEC	(90 * 24 * 60 * 60)	/* 90 days */

/* Interval until the next due check after `count` unchanged successes. */
time_t	check_backoff_interval(unsigned count);

/* True if never checked (checked_at == 0) or now is past the interval. */
int	check_backoff_due(time_t checked_at, unsigned count, time_t now);

#endif /* !AWESOME_PERSONAL_LIST_CHECK_BACKOFF_H */
