/*
 * awesome-personal-list - allocation and logging shims.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xmalloc.h"

void *
xmalloc(size_t size)
{
	void	*p;

	if (size == 0)
		size = 1;

	if ((p = malloc(size)) == NULL)
		fatal("out of memory allocating %zu bytes", size);

	return (p);
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void	*p;

	if (nmemb == 0)
		nmemb = 1;

	if ((p = calloc(nmemb, size)) == NULL)
		fatal("out of memory allocating %zu * %zu bytes", nmemb, size);

	return (p);
}

void *
xrealloc(void *ptr, size_t size)
{
	void	*p;

	if (size == 0)
		size = 1;

	if ((p = realloc(ptr, size)) == NULL)
		fatal("out of memory reallocating %zu bytes", size);

	return (p);
}

char *
xstrdup(const char *s)
{
	size_t	 len;
	char	*p;

	len = strlen(s) + 1;
	p = xmalloc(len);
	memcpy(p, s, len);

	return (p);
}

APP_NORETURN void
fatal(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fprintf(stderr, "fatal: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	exit(1);
}

void
app_log(int level, const char *fmt, ...)
{
	va_list		ap;
	time_t		now;
	char		stamp[32];
	const char	*tag;

	switch (level) {
	case LOG_ERR:
		tag = "error";
		break;
	case LOG_WARNING:
		tag = "warn";
		break;
	default:
		tag = "info";
		break;
	}

	now = time(NULL);
	strftime(stamp, sizeof(stamp), "%Y-%m-%dT%H:%M:%S", localtime(&now));

	fprintf(stderr, "%s [%s] ", stamp, tag);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}
