/* awesome-personal-list - auto-classification. See classify.h. */

#include <stdio.h>
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

struct keyword_rule {
	const char	*keyword;	/* matched case-insensitively */
	const char	*slug;
	const char	*name;
};

/* Matched against the description -- extend freely. */
static const struct keyword_rule keyword_rules[] = {
	{ "SDL",	"sdl",		"SDL" },
	{ "Qt",		"qt",		"Qt" },
	{ "GTK",	"gtk",		"GTK" },
	{ "OpenGL",	"opengl",	"OpenGL" },
	{ "Vulkan",	"vulkan",	"Vulkan" },
	{ "Box2D",	"box2d",	"Box2D" },
	{ "Godot",	"godot",	"Godot" },
	{ "Unity",	"unity",	"Unity" },
	{ "Unreal",	"unreal",	"Unreal Engine" },
	{ "ncurses",	"ncurses",	"ncurses" },
	{ "Electron",	"electron",	"Electron" },
	{ "React",	"react",	"React" },
	{ "Node.js",	"nodejs",	"Node.js" },
	{ "FFmpeg",	"ffmpeg",	"FFmpeg" },
	{ "libretro",	"libretro",	"libretro" },
	{ "Django",	"django",	"Django" },
	{ "Flask",	"flask",	"Flask" },
	{ "TensorFlow",	"tensorflow",	"TensorFlow" },
	{ "PyTorch",	"pytorch",	"PyTorch" },
	{ "QEMU",	"qemu",		"QEMU" },

	/* Console-homebrew: specific enough not to false-positive on
	 * everyday text (unlike e.g. bare "Switch" or "Vita"). */
	{ "homebrew",		"homebrew-console", "Homebrew Console" },
	{ "devkitPro",		"homebrew-console", "Homebrew Console" },
	{ "libctru",		"homebrew-console", "Homebrew Console" },
	{ "libnx",		"homebrew-console", "Homebrew Console" },
	{ "Nintendo DS",	"homebrew-console", "Homebrew Console" },
	{ "Game Boy Advance",	"homebrew-console", "Homebrew Console" },
	{ "GameCube",		"homebrew-console", "Homebrew Console" },
	{ "Dreamcast",		"homebrew-console", "Homebrew Console" },
	{ "3DS",		"homebrew-console", "Homebrew Console" },
};

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
classify_source(const char *description, const char *language,
    struct classified_category *out, size_t max)
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

	for (i = 0; i < sizeof(keyword_rules) / sizeof(keyword_rules[0]); i++) {
		if (str_casestr(description, keyword_rules[i].keyword) != NULL) {
			n = add_category(out, max, n, keyword_rules[i].slug,
			    keyword_rules[i].name);
		}
	}

	return (n);
}
