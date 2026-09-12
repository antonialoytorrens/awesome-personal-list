.PHONY: all build run run-swh-checker run-lang-checker export clean help i18n-check test

APP             := awesome-personal-list
SWH_CHECKER     := awesome-personal-list-swh-checker
LANGCHECKER     := awesome-personal-list-lang-checker
DEBUG           ?= 0
BUILD           := .objs

CORE_SRCS        := $(wildcard src/lib/*.c src/models/*.c)
APP_SRCS         := $(CORE_SRCS) src/main.c $(wildcard src/views/*.c src/middleware/*.c src/controllers/*.c)
SWH_CHECKER_SRCS := $(CORE_SRCS) src/swh_checker_main.c
LANGCHECKER_SRCS := $(CORE_SRCS) src/lang_checker_main.c

APP_OBJS         := $(patsubst %.c,$(BUILD)/%.o,$(APP_SRCS))
SWH_CHECKER_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SWH_CHECKER_SRCS))
LANGCHECKER_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(LANGCHECKER_SRCS))
DEPS             := $(patsubst %.c,$(BUILD)/%.d,$(CORE_SRCS) src/main.c src/swh_checker_main.c \
	src/lang_checker_main.c \
	$(wildcard src/views/*.c src/middleware/*.c src/controllers/*.c))

ECEWO_PKGS := ecewo ecewo-cookie ecewo-session ecewo-helmet ecewo-static
PKGS       := $(ECEWO_PKGS) libcjson libcurl

PKG_CFLAGS := $(shell pkg-config --cflags $(PKGS) 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs $(PKGS) 2>/dev/null)
CURL_LIBS  := $(shell pkg-config --libs libcurl 2>/dev/null)
CJSON_LIBS := $(shell pkg-config --libs libcjson 2>/dev/null)

# argon2 ships no pkg-config file on Debian. -lm: log() used but unrecorded.
EXTRA_LIBS := -largon2 -lm

ifeq ($(DEBUG),1)
OPTFLAGS := -g -O0
else
OPTFLAGS := -O2 -DNDEBUG
endif

CFLAGS  += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS  += $(OPTFLAGS) -Wall -Wextra -MMD -MP
CFLAGS  += -Isrc/includes $(PKG_CFLAGS)

all: build

build: $(APP) $(SWH_CHECKER) $(LANGCHECKER)

$(APP): $(APP_OBJS)
	$(CC) -o $@ $(APP_OBJS) $(PKG_LIBS) $(EXTRA_LIBS)

$(SWH_CHECKER): $(SWH_CHECKER_OBJS)
	$(CC) -o $@ $(SWH_CHECKER_OBJS) $(CURL_LIBS) $(CJSON_LIBS) $(EXTRA_LIBS)

$(LANGCHECKER): $(LANGCHECKER_OBJS)
	$(CC) -o $@ $(LANGCHECKER_OBJS) $(CURL_LIBS) $(CJSON_LIBS) $(EXTRA_LIBS)

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

run: build
	set -a; . ./.env; set +a; ./$(APP)

run-swh-checker: $(SWH_CHECKER)
	set -a; . ./.env; set +a; ./$(SWH_CHECKER)

run-lang-checker: $(LANGCHECKER)
	set -a; . ./.env; set +a; ./$(LANGCHECKER)

export: $(APP)
	set -a; . ./.env; set +a; ./$(APP) export $${EXPORT_PATH:-public/AWESOME.md}

clean:
	rm -rf $(BUILD) $(APP) $(SWH_CHECKER) $(LANGCHECKER)

# Every non-English locale/*.properties must define exactly the same key
# set as locale/en.properties (the Weblate template). Pure grep/cut/sort/comm
# over key=value flat files -- the whole reason .properties was chosen.
i18n-check:
	@ok=1; \
	base=locale/en.properties; \
	keys() { grep -v '^[[:space:]]*\(#\|!\|$$\)' "$$1" | cut -d= -f1 | sed 's/[[:space:]]*$$//' | sort -u; }; \
	keys "$$base" > /tmp/i18n-check-en.$$$$; \
	for f in locale/*.properties; do \
		[ "$$f" = "$$base" ] && continue; \
		keys "$$f" > /tmp/i18n-check-other.$$$$; \
		missing=$$(comm -23 /tmp/i18n-check-en.$$$$ /tmp/i18n-check-other.$$$$); \
		extra=$$(comm -13 /tmp/i18n-check-en.$$$$ /tmp/i18n-check-other.$$$$); \
		if [ -n "$$missing" ] || [ -n "$$extra" ]; then \
			ok=0; echo "== $$f =="; \
			[ -n "$$missing" ] && echo "$$missing" | sed 's/^/  missing: /'; \
			[ -n "$$extra" ] && echo "$$extra" | sed 's/^/  extra:   /'; \
		fi; \
		rm -f /tmp/i18n-check-other.$$$$; \
	done; \
	rm -f /tmp/i18n-check-en.$$$$; \
	[ "$$ok" = 1 ] && echo "i18n-check: all locales match $$base"; \
	[ "$$ok" = 1 ]

help:
	@echo "build             compile ./$(APP), ./$(SWH_CHECKER), and ./$(LANGCHECKER)   (DEBUG=1 for a debug build)"
	@echo "run               build, then run the web app (sources .env)"
	@echo "run-swh-checker   build, then run the SWH checker once (sources .env)"
	@echo "run-lang-checker  build, then run the language checker once (sources .env)"
	@echo "export            render public/AWESOME.md (or \$$EXPORT_PATH) from the current data"
	@echo "i18n-check        diff each locale/*.properties' keys against en.properties"
	@echo "clean             remove objects and all three binaries"

-include $(DEPS)

# Test suite
UNITY_SRC   := vendor/unity/unity.c
# Enable UNITY_SUPPORT_64 due to long long / INT64 asserts in 32-bit builds
TEST_CFLAGS := $(CFLAGS) -Ivendor/unity -DUNITY_SUPPORT_64

# Unit tests (no filesystem I/O)
$(BUILD)/tests/test_strutil: tests/test_strutil.c $(UNITY_SRC) \
    src/lib/strutil.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_validate: tests/test_validate.c $(UNITY_SRC) \
    src/lib/validate.c src/lib/strutil.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_wbuf: tests/test_wbuf.c $(UNITY_SRC) \
    src/lib/wbuf.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_form: tests/test_form.c $(UNITY_SRC) \
    src/lib/form.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_helpers: tests/test_helpers.c $(UNITY_SRC) \
    src/lib/helpers.c src/lib/wbuf.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_classify: tests/test_classify.c $(UNITY_SRC) \
    src/lib/classify.c src/lib/strutil.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_check_backoff: tests/test_check_backoff.c $(UNITY_SRC) \
    src/lib/check_backoff.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_translit: tests/test_translit.c $(UNITY_SRC) \
    src/lib/translit.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

$(BUILD)/tests/test_store: tests/test_store.c $(UNITY_SRC) \
    src/lib/store.c src/lib/strutil.c src/lib/translit.c src/lib/wbuf.c \
    src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

# Model/integration tests (tmpdir fixture, real filesystem)
$(BUILD)/tests/test_source_model: tests/test_source_model.c $(UNITY_SRC) \
    src/models/source.c src/lib/store.c src/lib/strutil.c \
    src/lib/translit.c src/lib/wbuf.c src/lib/xmalloc.c src/lib/helpers.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(CJSON_LIBS)

$(BUILD)/tests/test_category_model: tests/test_category_model.c $(UNITY_SRC) \
    src/models/category.c src/models/source.c src/lib/store.c \
    src/lib/strutil.c src/lib/translit.c src/lib/validate.c \
    src/lib/wbuf.c src/lib/xmalloc.c src/lib/helpers.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(CJSON_LIBS)

# Unit test (no filesystem I/O) for the source_controller.c filter helpers
# that now live in source.c -- see source_matches_filter().
$(BUILD)/tests/test_source_filter: tests/test_source_filter.c $(UNITY_SRC) \
    src/models/source.c src/lib/store.c src/lib/strutil.c \
    src/lib/translit.c src/lib/wbuf.c src/lib/xmalloc.c src/lib/helpers.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(CJSON_LIBS)

$(BUILD)/tests/test_admin_model: tests/test_admin_model.c $(UNITY_SRC) \
    src/models/admin.c src/lib/store.c src/lib/strutil.c \
    src/lib/translit.c src/lib/wbuf.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^ $(EXTRA_LIBS)

$(BUILD)/tests/test_swh_cache_model: tests/test_swh_cache_model.c $(UNITY_SRC) \
    src/models/swh_cache.c src/lib/store.c src/lib/strutil.c \
    src/lib/translit.c src/lib/wbuf.c src/lib/xmalloc.c
	@mkdir -p $(@D)
	$(CC) $(TEST_CFLAGS) -o $@ $^

TEST_BINS := \
    $(BUILD)/tests/test_strutil \
    $(BUILD)/tests/test_validate \
    $(BUILD)/tests/test_wbuf \
    $(BUILD)/tests/test_form \
    $(BUILD)/tests/test_helpers \
    $(BUILD)/tests/test_classify \
    $(BUILD)/tests/test_check_backoff \
    $(BUILD)/tests/test_translit \
    $(BUILD)/tests/test_store \
    $(BUILD)/tests/test_source_model \
    $(BUILD)/tests/test_source_filter \
    $(BUILD)/tests/test_category_model \
    $(BUILD)/tests/test_admin_model \
    $(BUILD)/tests/test_swh_cache_model

test: $(TEST_BINS)
	@passed=0; failed=0; \
	for t in $(TEST_BINS); do \
		if $$t; then passed=$$((passed+1)); else failed=$$((failed+1)); fi; \
	done; \
	echo ""; echo "Results: $$passed passed, $$failed failed."; \
	[ "$$failed" = "0" ]
