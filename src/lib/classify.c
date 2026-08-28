/* awesome-personal-list - auto-classification. See classify.h. */

#include <string.h>
#include <strings.h>

#include "classify.h"
#include "strutil.h"

struct lang_rule {
	const char	*name;		/* as typed in the form, case-insensitive */
	const char	*slug;
	const char	*display;
};

/* Only languages worth a category of their own -- markup/build-config
 * languages (HTML, CSS, Makefile, CMake, Dockerfile...) are skipped. */
static const struct lang_rule lang_rules[] = {
	{ "C",		"lang-c",	"C" },
	{ "C++",	"lang-cpp",	"C++" },
	{ "C#",		"lang-csharp",	"C#" },
	{ "Python",	"lang-python",	"Python" },
	{ "JavaScript",	"lang-js",	"JavaScript" },
	{ "TypeScript",	"lang-ts",	"TypeScript" },
	{ "Rust",	"lang-rust",	"Rust" },
	{ "Go",		"lang-go",	"Go" },
	{ "Java",	"lang-java",	"Java" },
	{ "Objective-C","lang-objc",	"Objective-C" },
	{ "Swift",	"lang-swift",	"Swift" },
	{ "Ruby",	"lang-ruby",	"Ruby" },
	{ "PHP",	"lang-php",	"PHP" },
	{ "Kotlin",	"lang-kotlin",	"Kotlin" },
	{ "Shell",	"lang-shell",	"Shell" },
	{ "Lua",	"lang-lua",	"Lua" },
	{ "Assembly",	"lang-asm",	"Assembly" },
	{ "Perl",	"lang-perl",	"Perl" },
	{ "Dart",	"lang-dart",	"Dart" },
	{ "Zig",	"lang-zig",	"Zig" },
	{ "Nim",	"lang-nim",	"Nim" },
	{ "Pascal",	"lang-pascal",	"Pascal" },
	{ "HolyC",	"lang-holyc",	"HolyC" },
	{ "PowerShell",	"lang-powershell", "PowerShell" },
	{ "Vala",	"lang-vala",	"Vala" },
};

/* Short-form/alias spellings that don't match a lang_rules[] name directly. */
struct lang_alias {
	const char	*alias;
	const char	*canonical;
};

static const struct lang_alias lang_aliases[] = {
	{ "cpp",	"C++" },
	{ "csharp",	"C#" },
	{ "golang",	"Go" },
	{ "js",		"JavaScript" },
	{ "ts",		"TypeScript" },
	{ "bash",	"Shell" },
	{ "sh",		"Shell" },
	{ "objc",	"Objective-C" },
	{ "asm",	"Assembly" },
	{ "ps1",	"PowerShell" },
};

const char *
classify_canonical_language(const char *raw)
{
	size_t	i;

	if (raw == NULL || raw[0] == '\0')
		return (NULL);

	for (i = 0; i < sizeof(lang_rules) / sizeof(lang_rules[0]); i++) {
		if (!strcasecmp(raw, lang_rules[i].name))
			return (lang_rules[i].display);
	}

	for (i = 0; i < sizeof(lang_aliases) / sizeof(lang_aliases[0]); i++) {
		if (!strcasecmp(raw, lang_aliases[i].alias))
			return (lang_aliases[i].canonical);
	}

	return (NULL);
}

static int
add_category(struct classified_category *out, size_t max, int n,
    const char *slug, const char *name)
{
	int	i;

	if ((size_t)n >= max)
		return (n);

	for (i = 0; i < n; i++) {
		if (!strcmp(out[i].slug, slug))
			return (n);	/* already added */
	}

	str_lcpy(out[n].slug, slug, sizeof(out[n].slug));
	str_lcpy(out[n].name, name, sizeof(out[n].name));

	return (n + 1);
}

int
classify_source(const char *language, struct classified_category *out,
    size_t max)
{
	size_t	i;
	int	n = 0;

	for (i = 0; i < sizeof(lang_rules) / sizeof(lang_rules[0]); i++) {
		if (language[0] != '\0' &&
		    !strcasecmp(language, lang_rules[i].name)) {
			n = add_category(out, max, n, lang_rules[i].slug,
			    lang_rules[i].display);
			break;
		}
	}

	return (n);
}
