# awesome-personal-list

A personal tool for curating an "awesome list" of upstream projects, checked
against Software Heritage. No database: flat files under `data/`, made in C
and ecewo framework.

This file documents the app itself. The *generated* list of curated sources
is `AWESOME.md`, produced by `make export` or served live at `/README.md`.

## Build

```sh
make build          # ./awesome-personal-list, ./awesome-personal-list-checker,
                     # and ./awesome-personal-list-lang-checker
make DEBUG=1 build  # debug build
```

## Run

On first run only, needs `AWESOME_PERSONAL_LIST_ADMIN_USER` / `AWESOME_PERSONAL_LIST_ADMIN_PASSWORD`
to seed the one login account (there is no `/register`). `SWH_API_TOKEN` is
optional, for a higher Software Heritage rate limit. `GITHUB_TOKEN` /
`GITLAB_TOKEN` are likewise optional, for a higher language-detection rate
limit (mainly useful for `GITHUB_TOKEN` -- GitLab's unauthenticated public
API limit is already generous).

All three binaries load this config themselves at startup, no shell-sourcing
required: `./.env` (relative to the working directory) first, then
`/etc/awesome-personal-list/awesome-personal-list.conf` as a fallback for anything `.env`
doesn't set. Either is optional; a real environment variable always wins
over both. Same `KEY=VALUE` syntax in both places.

```sh
make run               # web app on 127.0.0.1:48889
make run-swh-checker   # one Software Heritage sweep
make run-lang-checker  # one language-detection sweep
make export            # render AWESOME.md from the current data
```

## Layout

```
src/
├── models/        source, category, admin, swh_cache -- flat files
├── lib/           store (atomic write), SWH/GitHub/GitLab clients, README renderer
├── middleware/     session auth (ecewo-session), CSRF
├── controllers/    route handlers
├── views/          $VAR$ template engine
└── templates/      HTML
```

`awesome-personal-list` (web app), `awesome-personal-list-checker` (periodic
Software Heritage sweep), and `awesome-personal-list-lang-checker` (periodic
language-detection sweep, fills in whatever "New source" leaves blank) — both
checkers meant for a systemd timer, see `systemd/` — share the model/lib
layer and nothing else.

## Data

- `data/sources/*.json`, `data/categories/*.json` — the curated content.
  Worth committing.
- `data/admin.tsv`, `data/sessions.tsv` — credentials/sessions. Never commit.
- `data/swh_cache.tsv` — the SWH checker's cache, keyed by original URL.
  Regenerable; not worth committing. The language checker has no separate
  cache -- it writes straight into each source's own JSON record.

### Built With

*   [ecewo](https://github.com/ecewo/ecewo) - asynchronous C web framework
*   [cJSON](https://github.com/DaveGamble/cJSON) - JSON parsing and serialization
*   [libcurl](https://curl.se/libcurl/) - Software Heritage / GitHub / GitLab API HTTP requests
*   [argon2](https://github.com/P-H-C/phc-winner-argon2) - password hashing
*   [Unity](https://github.com/ThrowTheSwitch/Unity) - unit tests
