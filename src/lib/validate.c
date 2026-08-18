/* awesome-personal-list - input validation for form fields. */

#include <ctype.h>
#include <string.h>

#include "app.h"
#include "strutil.h"
#include "validate.h"

int
v_url(const char *s)
{
	if (s == NULL || *s == '\0' || strlen(s) >= URL_MAX_LEN)
		return (0);

	return (str_hasprefix(s, "http://") || str_hasprefix(s, "https://") ||
	    str_hasprefix(s, "git://"));
}

int
v_slug(const char *s)
{
	size_t	i, len;

	if (s == NULL || (len = strlen(s)) == 0 || len >= NAME_MAX_LEN)
		return (0);

	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)s[i];

		if (!(isalnum(c) || c == '-' || c == '_' || c == '.'))
			return (0);
	}

	return (1);
}

int
v_text(const char *s, size_t maxlen)
{
	return (s != NULL && strlen(s) < maxlen);
}

int
v_category_name(const char *s)
{
	return (v_text(s, NAME_MAX_LEN) && s[0] != '\0');
}

int
v_admin_name(const char *s)
{
	return (v_text(s, NAME_MAX_LEN) && s[0] != '\0');
}

int
v_color(const char *s)
{
	int	i;

	if (s == NULL || strlen(s) != 7 || s[0] != '#')
		return (0);

	for (i = 1; i < 7; i++) {
		if (!isxdigit((unsigned char)s[i]))
			return (0);
	}

	return (1);
}
