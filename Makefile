PROJECT := skill-gov

ifeq '$(OS)' 'Windows_NT'
	CC := gcc
	EXE_SUFFIX := .exe
	PREFIX ?= $(subst \,/,$(USERPROFILE))/.local
else
	CC := cc
	EXE_SUFFIX :=
	PREFIX ?= $(HOME)/.local
endif
BINDIR ?= $(PREFIX)/bin

SRC := $(wildcard src/*.c)
OBJ_DEBUG := $(patsubst src/%.c,build/debug/%.o,$(SRC))
OBJ_RELEASE := $(patsubst src/%.c,build/release/%.o,$(SRC))
OBJ_SAN := $(patsubst src/%.c,build/sanitize/%.o,$(SRC))
STAMP_DEBUG := build/debug/.toolchain
STAMP_RELEASE := build/release/.toolchain
STAMP_SAN := build/sanitize/.toolchain

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

debug: build/debug/$(PROJECT)$(EXE_SUFFIX)

release: build/release/$(PROJECT)$(EXE_SUFFIX)

sanitize: build/sanitize/$(PROJECT)$(EXE_SUFFIX)

build/debug/$(PROJECT)$(EXE_SUFFIX): CFLAGS := $(CSTD) $(WARN) $(OPT_DEBUG)
build/release/$(PROJECT)$(EXE_SUFFIX): CFLAGS := $(CSTD) $(WARN) $(OPT_RELEASE)
build/sanitize/$(PROJECT)$(EXE_SUFFIX): CFLAGS := $(CSTD) $(WARN) $(OPT_SAN)
build/sanitize/$(PROJECT)$(EXE_SUFFIX): LDFLAGS += -fsanitize=address,undefined

build/debug/$(PROJECT)$(EXE_SUFFIX): $(OBJ_DEBUG)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/release/$(PROJECT)$(EXE_SUFFIX): $(OBJ_RELEASE)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

build/sanitize/$(PROJECT)$(EXE_SUFFIX): $(OBJ_SAN)
	@mkdir -p $(@D)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(STAMP_DEBUG) $(STAMP_RELEASE) $(STAMP_SAN):
	@mkdir -p $(@D)
	@$(CC) -dumpmachine 2>/dev/null > $@.tmp && mv $@.tmp $@

build/debug/%.o: src/%.c $(STAMP_DEBUG)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build/release/%.o: src/%.c $(STAMP_RELEASE)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build/sanitize/%.o: src/%.c $(STAMP_SAN)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

install: release
	@mkdir -p $(DESTDIR)$(BINDIR)
	@cp -f build/release/$(PROJECT)$(EXE_SUFFIX) $(DESTDIR)$(BINDIR)/$(PROJECT)$(EXE_SUFFIX)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PROJECT)$(EXE_SUFFIX)

clean:
	rm -rf build

-include $(OBJ_DEBUG:.o=.d) $(OBJ_RELEASE:.o=.d) $(OBJ_SAN:.o=.d)
