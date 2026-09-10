# awesome-personal-list

A personal tool for curating an "awesome list" of upstream projects, checked
against Software Heritage. No database: flat files under `data/`, made in C
and ecewo framework.

This file documents the app itself. The *generated* list of curated sources
is `public/AWESOME.md`, produced by `make export` or served live at `/AWESOME.md`.

## Usage

```sh
make build          # ./awesome-personal-list, ./awesome-personal-list-swh-checker,
                     # and ./awesome-personal-list-lang-checker
make DEBUG=1 build  # debug build
make run               # web app on 127.0.0.1:48889
make run-swh-checker   # one Software Heritage sweep
make run-lang-checker  # one language-detection sweep
make export            # render public/AWESOME.md from the current data
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

`awesome-personal-list` (web app), `awesome-personal-list-swh-checker` (periodic
Software Heritage sweep with exponential backoff; fills blanks and rechecks
unlocked sources), and `awesome-personal-list-lang-checker` (periodic
language-detection sweep with exponential backoff; fills blanks and rechecks
unlocked sources) — both checkers meant for a systemd timer, see `systemd/` —
share the model/lib layer and nothing else.

## Data

- `data/sources/*.json`, `data/categories/*.json` - the curated content.
  Worth committing.
- `data/admin.tsv`, `data/sessions.tsv` - credentials/sessions.
- `data/swh_cache.tsv` - the SWH checker's cache.

### Built With

*   [ecewo](https://github.com/ecewo/ecewo) - asynchronous C web framework
*   [cJSON](https://github.com/DaveGamble/cJSON) - JSON parsing and serialization
*   [libcurl](https://curl.se/libcurl/) - Software Heritage / GitHub / GitLab API HTTP requests
*   [argon2](https://github.com/P-H-C/phc-winner-argon2) - password hashing
*   [Unity](https://github.com/ThrowTheSwitch/Unity) - unit tests
