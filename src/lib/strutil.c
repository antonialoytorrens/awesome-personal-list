/*
 * awesome-personal-list - small string helpers used by the model layer.
 */

#define _DEFAULT_SOURCE	/* strsep(3) is BSD, not POSIX */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "strutil.h"

size_t
str_lcpy(char *dst, const char *src, size_t dstsize)
{
	size_t	srclen;

	srclen = strlen(src);

	if (dstsize > 0) {
		size_t	n = (srclen < dstsize - 1) ? srclen : dstsize - 1;

		memcpy(dst, src, n);
		dst[n] = '\0';
	}

	return (srclen);
}

int
str_split(char *input, const char *sep, char **out, size_t max)
{
	char	*p, *tok;
	int	 n;

	n = 0;
	p = input;

	while (n < (int)max && (tok = strsep(&p, sep)) != NULL) {
		if (*tok == '\0')
			continue;
		out[n++] = tok;
	}

	if (n < (int)max)
		out[n] = NULL;

	return (n);
}

long long
str_tonum(const char *s, int base, long long lo, long long hi, int *err)
{
	char		*end;
	long long	 v;

	*err = STR_ERROR;

	if (s == NULL || *s == '\0')
		return (0);

	errno = 0;
	v = strtoll(s, &end, base);

	if (*end != '\0' || errno == ERANGE)
		return (0);

	if (v < lo || v > hi)
		return (0);

	*err = STR_OK;
	return (v);
}

void
str_hex(const unsigned char *in, size_t len, char *out, size_t outlen)
{
	static const char	hexdigits[] = "0123456789abcdef";
	size_t			i;

	if (outlen < (len * 2) + 1)
		return;

	for (i = 0; i < len; i++) {
		out[i * 2] = hexdigits[in[i] >> 4];
		out[i * 2 + 1] = hexdigits[in[i] & 0x0f];
	}

	out[len * 2] = '\0';
}

int
str_timingsafe_equal(const char *a, const char *b)
{
	size_t		la, lb, i;
	unsigned char	diff;

	la = strlen(a);
	lb = strlen(b);

	diff = (unsigned char)(la != lb);

	for (i = 0; i < la && i < lb; i++)
		diff |= (unsigned char)(a[i] ^ b[i]);

	return (diff == 0);
}

void
str_tolower(char *s)
{
	for (; *s != '\0'; s++)
		*s = (char)tolower((unsigned char)*s);
}

int
str_hasprefix(const char *s, const char *prefix)
{
	size_t	len;

	len = strlen(prefix);

	return (strncmp(s, prefix, len) == 0);
}

char *
str_casestr(const char *haystack, const char *needle)
{
	size_t	hlen, nlen, i;

	hlen = strlen(haystack);
	nlen = strlen(needle);

	if (nlen == 0 || nlen > hlen)
		return (NULL);

	for (i = 0; i + nlen <= hlen; i++) {
		if (!strncasecmp(haystack + i, needle, nlen))
			return ((char *)haystack + i);
	}

	return (NULL);
}
