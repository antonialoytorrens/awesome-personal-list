/* awesome-personal-list - input validation for form fields. */

#ifndef AWESOME_PERSONAL_LIST_VALIDATE_H
#define AWESOME_PERSONAL_LIST_VALIDATE_H

#include <stddef.h>

int	v_url(const char *);
int	v_slug(const char *);
int	v_text(const char *, size_t maxlen);
int	v_category_name(const char *);
int	v_admin_name(const char *);
/* "#rrggbb", case-insensitive hex. */
int	v_color(const char *);

#endif /* !AWESOME_PERSONAL_LIST_VALIDATE_H */
