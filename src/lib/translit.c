/* awesome-personal-list - see translit.h. */

#include <stddef.h>

#include "translit.h"

unsigned int
utf8_decode(const char **pp)
{
	const unsigned char	*p = (const unsigned char *)*pp;
	unsigned int		 cp;
	int			 extra;

	if (*p < 0x80) {
		*pp = (const char *)(p + 1);
		return (*p);
	} else if ((*p & 0xe0) == 0xc0) {
		cp = *p & 0x1f;
		extra = 1;
	} else if ((*p & 0xf0) == 0xe0) {
		cp = *p & 0x0f;
		extra = 2;
	} else if ((*p & 0xf8) == 0xf0) {
		cp = *p & 0x07;
		extra = 3;
	} else {
		/* stray continuation byte or 0xf8-0xff: not a valid lead
		 * byte -- consume just this one byte so the caller's cursor
		 * keeps moving. */
		*pp = (const char *)(p + 1);
		return (0xfffd);
	}

	p++;
	while (extra-- > 0) {
		if ((*p & 0xc0) != 0x80) {
			/* truncated/invalid sequence -- stop before consuming
			 * a byte that isn't a continuation byte (which also
			 * protects against running past a '\0'). */
			*pp = (const char *)p;
			return (0xfffd);
		}
		cp = (cp << 6) | (*p & 0x3f);
		p++;
	}

	*pp = (const char *)p;
	return (cp);
}

struct translit_ent {
	unsigned int	 cp;
	const char	*ascii;
};

/*
 * Latin-1 Supplement (U+00C0-U+00FF): the diacritics used by Catalan,
 * Spanish, French, Portuguese, German, Italian, Dutch... covers this app's
 * locales and then some. Code points with no letterlike meaning (multiply
 * sign U+00D7, divide sign U+00F7) are intentionally absent -- they fall
 * through to the caller's separator handling, same as any other unmapped
 * multi-byte input.
 */
static const struct translit_ent latin1_supplement[] = {
	{ 0xc0, "a" },	{ 0xc1, "a" },	{ 0xc2, "a" },	{ 0xc3, "a" },
	{ 0xc4, "a" },	{ 0xc5, "a" },	{ 0xc6, "ae" },	{ 0xc7, "c" },
	{ 0xc8, "e" },	{ 0xc9, "e" },	{ 0xca, "e" },	{ 0xcb, "e" },
	{ 0xcc, "i" },	{ 0xcd, "i" },	{ 0xce, "i" },	{ 0xcf, "i" },
	{ 0xd0, "d" },	{ 0xd1, "n" },	{ 0xd2, "o" },	{ 0xd3, "o" },
	{ 0xd4, "o" },	{ 0xd5, "o" },	{ 0xd6, "o" },	{ 0xd8, "o" },
	{ 0xd9, "u" },	{ 0xda, "u" },	{ 0xdb, "u" },	{ 0xdc, "u" },
	{ 0xdd, "y" },	{ 0xde, "th" },	{ 0xdf, "ss" },
	{ 0xe0, "a" },	{ 0xe1, "a" },	{ 0xe2, "a" },	{ 0xe3, "a" },
	{ 0xe4, "a" },	{ 0xe5, "a" },	{ 0xe6, "ae" },	{ 0xe7, "c" },
	{ 0xe8, "e" },	{ 0xe9, "e" },	{ 0xea, "e" },	{ 0xeb, "e" },
	{ 0xec, "i" },	{ 0xed, "i" },	{ 0xee, "i" },	{ 0xef, "i" },
	{ 0xf0, "d" },	{ 0xf1, "n" },	{ 0xf2, "o" },	{ 0xf3, "o" },
	{ 0xf4, "o" },	{ 0xf5, "o" },	{ 0xf6, "o" },	{ 0xf8, "o" },
	{ 0xf9, "u" },	{ 0xfa, "u" },	{ 0xfb, "u" },	{ 0xfc, "u" },
	{ 0xfd, "y" },	{ 0xfe, "th" },	{ 0xff, "y" },
};

const char *
translit_ascii(unsigned int cp)
{
	size_t	i;

	for (i = 0; i < sizeof(latin1_supplement) / sizeof(latin1_supplement[0]); i++) {
		if (latin1_supplement[i].cp == cp)
			return (latin1_supplement[i].ascii);
	}

	return (NULL);
}
