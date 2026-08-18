/*
 * awesome-personal-list - a personal, curated "awesome list" of upstream projects,
 * checked against Software Heritage.
 *
 * Application-wide configuration and shared types. Laid out the way
 * didimdwiki is: routes/controllers/models/views/middleware, flat files
 * under data/, no database.
 */

#ifndef AWESOME_PERSONAL_LIST_APP_H
#define AWESOME_PERSONAL_LIST_APP_H

#include <sys/types.h>
#include <time.h>

#define APP_NAME		"awesome-personal-list"
#define APP_VERSION		"0.1.0"

/* ---------------------------------------------------------- data layout */

#define DATA_DIR		"data"
#define SOURCES_DIR		DATA_DIR "/sources"
#define CATEGORIES_DIR		DATA_DIR "/categories"
#define ADMIN_FILE		DATA_DIR "/admin.tsv"
#define SWH_CACHE_FILE		DATA_DIR "/swh_cache.tsv"
#define BODY_TMP_DIR		DATA_DIR "/tmp"

#define VIEWS_DIR		"src/templates"
#define PUBLIC_DIR		"public"
#define LOCALE_DIR		"locale"

/*
 * Optional KEY=VALUE config, loaded into the environment at startup without
 * overwriting anything already set (so a real env var, or .env sourced by
 * `make run`, still wins). Lets both binaries run standalone -- via
 * systemd, or invoked bare -- without depending on a shell having sourced
 * anything first.
 */
#define SYSTEM_CONFIG_FILE	"/etc/awesome-personal-list/awesome-personal-list.conf"

#define UNCATEGORIZED_SLUG	"uncategorized"

#define NAME_MAX_LEN		128
#define URL_MAX_LEN		1024
#define TEXT_MAX_LEN		4096
#define PATH_MAX_LEN		1024

/* ecewo-session names the cookie "session" itself; not configurable. */
#define SESSION_TTL		(60 * 60)	/* seconds; 1 hour */
#define TOKEN_BYTES		32
#define TOKEN_HEX_LEN		((TOKEN_BYTES * 2) + 1)

/*
 * argon2id parameters. The tag is stored alongside the hash so these can be
 * raised later without a file format change.
 */
#define ARGON2_MEMORY_KIB	65536		/* 64 MiB */
#define ARGON2_ITERATIONS	3
#define ARGON2_PARALLELISM	1
#define ARGON2_HASH_LEN		32
#define ARGON2_TAG_MAX		64

/* ----------------------------------------------------------- data model */

/* unknown: never checked. not_found: checked, not (yet) in the archive. */
#define SWH_STATUS_UNKNOWN	0
#define SWH_STATUS_ARCHIVED	1
#define SWH_STATUS_NOT_FOUND	2

#define CATEGORY_DEFAULT_COLOR	"#8b949e"	/* same neutral gray .lang-dot falls back to */

struct category {
	char		slug[NAME_MAX_LEN];
	char		name[NAME_MAX_LEN];
	char		description[TEXT_MAX_LEN];
	char		color[8];		/* "#rrggbb" */
};

/* A source can be generic enough to belong to several (e.g. a C+SDL project
 * is both a language and a library match), so this is a small set, not one
 * slug. */
#define SOURCE_MAX_CATEGORIES	8

struct source {
	char		slug[NAME_MAX_LEN];		/* derived from original_url */
	char		original_url[URL_MAX_LEN];
	char		description[TEXT_MAX_LEN];
	char		language[NAME_MAX_LEN];
	char		categories[SOURCE_MAX_CATEGORIES][NAME_MAX_LEN];
	int		category_count;
	char		notes[TEXT_MAX_LEN];
	int		swh_status;
	time_t		swh_checked_at;
	time_t		created_at;
	time_t		updated_at;
};

struct admin {
	char		name[NAME_MAX_LEN];
	char		algo[ARGON2_TAG_MAX];
	char		salt[TOKEN_HEX_LEN];
	char		hash[(ARGON2_HASH_LEN * 2) + 1];
	time_t		created;
	char		language[8];
};

/* Opaque; defined in ecewo-session.h. Avoids pulling ecewo into every model. */
struct ecewo_session_s;

/* Per-request state, hung off the ecewo request context. */
struct req_ctx {
	int			 authed;
	struct admin		 user;
	struct ecewo_session_s	*sess;
	char			 lang[8];	/* resolved for this request */
};

/* Runtime configuration; see app_parse_config() in main.c. */
struct app_config {
	int		tls_enabled;
	char		bind[64];
	unsigned short	port;
	char		views_dir[PATH_MAX_LEN];
	char		public_dir[PATH_MAX_LEN];

	/* Optional; Software Heritage authenticated rate limit. */
	char		swh_token[256];
};

extern struct app_config	app_cfg;

#endif /* !AWESOME_PERSONAL_LIST_APP_H */
