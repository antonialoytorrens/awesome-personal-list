/*
 * awesome-personal-list - UTF-8 decoding and ASCII transliteration for slugs.
 */

#ifndef AWESOME_PERSONAL_LIST_TRANSLIT_H
#define AWESOME_PERSONAL_LIST_TRANSLIT_H

/*
 * Decodes one UTF-8 code point starting at *pp and advances *pp past it.
 * Handles 1-4 byte sequences. A malformed lead byte or a truncated/invalid
 * continuation sequence decodes as U+FFFD; *pp still advances by however
 * many bytes were safely consumed, but never past a '\0', so callers can
 * loop on a NUL-terminated string without reading out of bounds.
 */
unsigned int	utf8_decode(const char **pp);

/*
 * ASCII transliteration for a Latin-1 Supplement letter (the accented
 * Latin letters used by Catalan, Spanish, French, Portuguese, German,
 * Italian...), e.g. U+00F3 ('o' with acute) -> "o". Returns NULL if cp
 * has no mapping.
 */
const char	*translit_ascii(unsigned int cp);

#endif /* !AWESOME_PERSONAL_LIST_TRANSLIT_H */
