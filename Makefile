PROJECT := skill-gov
CC ?= cc
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

SRC := $(wildcard src/*.c)
OBJ_DEBUG := $(patsubst src/%.c,build/debug/%.o,$(SRC))
OBJ_RELEASE := $(patsubst src/%.c,build/release/%.o,$(SRC))
OBJ_SAN := $(patsubst src/%.c,build/sanitize/%.o,$(SRC))

CPPFLAGS := -Iinclude -D_POSIX_C_SOURCE=200809L
CSTD := -std=c17
WARN := -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 -Wstrict-prototypes -Wmissing-prototypes
OPT_DEBUG := -O0 -g3
OPT_RELEASE := -O2 -g0 -DNDEBUG
OPT_SAN := -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
LDFLAGS :=
LDLIBS :=

.PHONY: all debug release sanitize clean install uninstall

all: release

debug: build/debug/$(PROJECT)

release: build/release/$(PROJECT)

sanitize: build/sanitize/$(PROJECT)

build/debug/$(PROJECT): CFLAGS := $(CSTD) $(WARN) $(OPT_DEBUG)
build/release/$(PROJECT): CFLAGS := $(CSTD) $(WARN) $(OPT_RELEASE)
build/sanitize/$(PROJECT): CFLAGS := $(CSTD) $(WARN) $(OPT_SAN)
build/sanitize/$(PROJECT): LDFLAGS += -fsanitize=address,undefined

build/debug/$(PROJECT): $(OBJ_DEBUG)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/release/$(PROJECT): $(OBJ_RELEASE)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/sanitize/$(PROJECT): $(OBJ_SAN)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/debug/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build/release/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build/sanitize/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

install: release
	@mkdir -p $(DESTDIR)$(BINDIR)
	install -m 0755 build/release/$(PROJECT) $(DESTDIR)$(BINDIR)/$(PROJECT)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROJECT)

clean:
	rm -rf build

-include $(OBJ_DEBUG:.o=.d) $(OBJ_RELEASE:.o=.d) $(OBJ_SAN:.o=.d)
