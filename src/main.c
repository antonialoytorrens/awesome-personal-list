/* awesome-personal-list - application bootstrap. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecewo.h"
#include "ecewo-helmet.h"
#include "ecewo-session.h"
#include "ecewo-static.h"

#include "app.h"
#include "controllers.h"
#include "envfile.h"
#include "i18n.h"
#include "middleware.h"
#include "models.h"
#include "readme.h"
#include "strutil.h"
#include "swh.h"
#include "views.h"
#include "xmalloc.h"

struct app_config	app_cfg = {
	.tls_enabled = 0,
	.bind = "127.0.0.1",
	.port = 48889,
	.views_dir = VIEWS_DIR,
	.public_dir = PUBLIC_DIR,
};

static void
load_config(void)
{
	const char	*env;
	long		 port;
	int		 err;

	/* Local .env (relative to cwd) wins; the system-wide file fills in
	 * anything it doesn't set. Both are optional -- real env vars beat
	 * either. */
	load_env_file(".env");
	load_env_file(SYSTEM_CONFIG_FILE);

	if ((env = getenv("SWH_API_TOKEN")) != NULL)
		str_lcpy(app_cfg.swh_token, env, sizeof(app_cfg.swh_token));

	if ((env = getenv("AWESOME_PERSONAL_LIST_BIND")) != NULL)
		str_lcpy(app_cfg.bind, env, sizeof(app_cfg.bind));

	if ((env = getenv("AWESOME_PERSONAL_LIST_PORT")) != NULL) {
		port = (long)str_tonum(env, 10, 1, 65535, &err);
		if (err == STR_OK)
			app_cfg.port = (unsigned short)port;
	}
}

static void
seed_admin(void)
{
	const char	*user, *pass;

	if (admin_exists())
		return;

	user = getenv("AWESOME_PERSONAL_LIST_ADMIN_USER");
	pass = getenv("AWESOME_PERSONAL_LIST_ADMIN_PASSWORD");

	if (user == NULL || pass == NULL)
		fatal("no admin account yet: set AWESOME_PERSONAL_LIST_ADMIN_USER and "
		    "AWESOME_PERSONAL_LIST_ADMIN_PASSWORD for first run");

	if (admin_create(user, pass) != 0)
		fatal("could not create the admin account");

	app_log(LOG_INFO, "seeded admin account '%s'", user);
}

static void
register_routes(ecewo_app_t *app)
{
	ECEWO_GET  (app, "/",               route_home);

	ECEWO_GET  (app, "/login",          route_login_get);
	ECEWO_POST (app, "/login",          route_login_post);
	ECEWO_POST (app, "/logout",         route_logout);

	ECEWO_GET  (app, "/profile",        route_profile_get);
	ECEWO_POST (app, "/profile",        route_profile_post);
	ECEWO_POST (app, "/lang",           route_lang_set);

	ECEWO_GET  (app, "/sources",        route_sources_list);
	ECEWO_POST (app, "/sources",        route_sources_bulk);
	ECEWO_GET  (app, "/sources/new",    route_source_new_get);
	ECEWO_POST (app, "/sources/new",    route_source_new_post);
	ECEWO_GET  (app, "/sources/:slug",  route_source_get);
	ECEWO_POST (app, "/sources/:slug",  route_source_post);

	ECEWO_GET  (app, "/categories",       route_categories_list);
	ECEWO_POST (app, "/categories",       route_category_create);
	ECEWO_GET  (app, "/categories/:slug", route_category_get);
	ECEWO_POST (app, "/categories/:slug", route_category_post);
	ECEWO_GET  (app, "/categories.css",   route_categories_css);

	ECEWO_GET  (app, "/README.md",      route_readme);
}

static int
install_helmet(ecewo_app_t *app)
{
	ecewo_helmet_config_t	*h;

	if ((h = ecewo_helmet_config_new()) == NULL)
		return (-1);

	/* The app ships no JavaScript at all -- every interaction is plain
	 * HTML forms plus CSS. script-src 'none' makes that enforced rather
	 * than merely conventional. */
	ecewo_helmet_config_set_csp(h, "default-src 'self'; script-src 'none'");
	ecewo_helmet_config_set_frame_options(h, "SAMEORIGIN");
	ecewo_helmet_config_set_referrer_policy(h, "same-origin");
	ecewo_helmet_config_set_nosniff(h, true);
	ecewo_helmet_config_set_hsts(h,
	    app_cfg.tls_enabled ? "31536000" : NULL, false, false);

	return (ecewo_helmet_install(app, h));
}

static void
on_shutdown(void *user_data)
{
	(void)user_data;
	ecewo_static_cleanup();
}

/* `awesome-personal-list export <path>` writes README.md and exits -- no server. */
static int
run_export(const char *path)
{
	char	*md;
	int	 ret;

	if (store_verify_dirs() == -1)
		fatal("data directories missing");

	md = readme_render();
	ret = store_write(path, md, strlen(md));
	free(md);

	if (ret == -1) {
		app_log(LOG_ERR, "export: could not write %s", path);
		return (1);
	}

	app_log(LOG_INFO, "exported to %s", path);
	return (0);
}

int
main(int argc, char **argv)
{
	ecewo_app_t	*app;

	if (argc == 3 && !strcmp(argv[1], "export")) {
		load_config();
		return (run_export(argv[2]));
	}

	load_config();

	if (store_verify_dirs() == -1)
		fatal("data directories missing");

	category_seed_defaults();
	seed_admin();

	if (swh_init() == -1)
		fatal("curl_global_init failed");

	if ((app = ecewo_create()) == NULL)
		fatal("ecewo_create failed");

	if (ecewo_session_init(app) != 0)
		fatal("ecewo_session_init failed");

	if (ecewo_static_init() != 0)
		fatal("ecewo_static_init failed");

	if (templates_load(app, app_cfg.views_dir) == -1)
		fatal("could not load templates from %s", app_cfg.views_dir);

	if (i18n_load(LOCALE_DIR) == -1)
		fatal("could not load translations from %s", LOCALE_DIR);

	if (install_helmet(app) != 0)
		fatal("ecewo_helmet_install failed");

	if (ecewo_use(app, NULL, ctx_middleware) != 0)
		fatal("could not register the session middleware");

	register_routes(app);

	/* After the routes: a mount also registers a wildcard beneath itself,
	 * and routes match in registration order. CSS/JS only -- no auth
	 * needed, so this isn't behind require_auth(). */
	if (ecewo_serve_static(app, "/public", app_cfg.public_dir, NULL) != 0)
		fatal("could not mount %s", app_cfg.public_dir);

	if (ecewo_atexit(app, on_shutdown, NULL) != 0)
		fatal("could not register the shutdown hook");

	ecewo_set_listen_address(app, app_cfg.bind);

	app_log(LOG_INFO, "%s %s listening on %s:%u", APP_NAME, APP_VERSION,
	    app_cfg.bind, app_cfg.port);

	if (ecewo_listen(app, app_cfg.port) != 0)
		fatal("could not listen on %s:%u", app_cfg.bind, app_cfg.port);

	return (0);
}
