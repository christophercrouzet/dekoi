space :=
space +=

# ------------------------------------------------------------------------------

ifndef outdir
OUTDIR := build
else
OUTDIR := $(outdir)
endif

ifndef compiler
COMPILER := c
else ifneq "$(filter-out c cc,$(compiler))" ""
$(error the 'compiler' option is not valid)
else
COMPILER := $(compiler)
endif

ifndef env
ENV := 64
else ifneq (1,$(words [$(strip $(env))]))
$(error the 'env' option should contain a single value)
else ifneq "$(filter-out 16 32 x32 64,$(env))" ""
$(error the 'env' option is not valid)
else
ENV := $(env)
endif

DEFAULT_ARCH := $(subst _,-,$(shell arch))
ifndef arch
ARCH := $(DEFAULT_ARCH)
else
ARCH := $(strip $(arch))
endif

ifndef config
CONFIG := release
else ifneq "$(filter-out debug release all,$(config))" ""
$(error the 'config' option is not valid)
else ifeq "$(filter all,$(config))" "all"
CONFIG := debug release
else
CONFIG := $(config)
endif

# ------------------------------------------------------------------------------

PROJECT := dekoi

OBJECTDIR := $(OUTDIR)/obj
BINARYDIR := $(OUTDIR)/bin

CFLAGS := -std=c99
CXXFLAGS := -std=c++11
CPPFLAGS := -Iinclude -fPIC \
            -Wpedantic -Wall -Wextra -Waggregate-return -Wcast-align \
            -Wcast-qual -Wconversion -Wfloat-equal -Wpointer-arith -Wshadow \
            -Wstrict-overflow=5 -Wswitch -Wswitch-default -Wundef \
            -Wunreachable-code -Wwrite-strings
PREREQFLAGS := -MMD -MP
LDFLAGS :=
LDLIBS := -lvulkan

COMPILE.c = $(CC) $(PREREQFLAGS) $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(PREREQFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(TARGET_ARCH) -c
LINK.o = $(CC) $(LDFLAGS) $(TARGET_ARCH)

COMPILE = $(COMPILE.$(COMPILER))

# ------------------------------------------------------------------------------

release_CFLAGS :=
release_CXXFLAGS :=
release_CPPFLAGS := -DNDEBUG -O3
release_LDFLAGS :=

debug_CFLAGS :=
debug_CXXFLAGS :=
debug_CPPFLAGS := -DDEBUG -O0 -g
debug_LDFLAGS :=

# ------------------------------------------------------------------------------

ALL_SOURCES :=
ALL_HEADERS :=

# Create build rules for object targets.
# $(1): source path, e.g.: src, playground/test.
# $(2): target path, e.g.: dekoi, playground/test.
# $(3): prefix for variable names.
# $(4): architecture, e.g. x86-64, corei7.
# $(5): configuration, e.g.: debug, release.
define CREATE_OBJECT_RULES =
$(3)_SOURCES := $$(wildcard $(1)/*.c) $$(wildcard $(1)/private/*.c)
$(3)_HEADERS := $$(wildcard $(1)/*.h) $$(wildcard $(1)/private/*.h)
$(3)_OBJECTS := $$($(3)_SOURCES:$(1)/%.c=$$(OBJECTDIR)/$(4)/$(5)/$(2)/%.o)
$(3)_PREREQS := $$($(3)_OBJECTS:.o=.d)

$$($(3)_OBJECTS): CFLAGS += $$($(4)_CFLAGS)
$$($(3)_OBJECTS): CXXFLAGS += $$($(4)_CXXFLAGS)
$$($(3)_OBJECTS): CPPFLAGS += $$($(4)_CPPFLAGS)

$$($(3)_OBJECTS): CFLAGS += $$($(5)_CFLAGS)
$$($(3)_OBJECTS): CXXFLAGS += $$($(5)_CXXFLAGS)
$$($(3)_OBJECTS): CPPFLAGS += $$($(5)_CPPFLAGS)

$$($(3)_OBJECTS): TARGET_ARCH := -march=$(4) -m$$(ENV)

$$($(3)_OBJECTS): $$(OBJECTDIR)/$(4)/$(5)/$(2)/%.o: $(1)/%.c
	@ mkdir -p $$(@D)
	@ $$(COMPILE) -o $$@ $$<

-include $$($(3)_PREREQS)

ALL_SOURCES += $$($(3)_SOURCES)
ALL_HEADERS += $$($(3)_HEADERS)
endef

# Create build rules for binary targets.
# $(1): source path, e.g.: src, playground/test.
# $(2): target path, e.g.: dekoi, playground/test.
# $(3): prefix for variable names.
# $(4): architecture, e.g. x86-64, corei7.
# $(5): configuration, e.g.: debug, release.
# $(6): whether to add project dependencies to the target.
# $(7): libraries for the linker.
define CREATE_BINARY_RULES =
$(3)_SOURCES := $$(wildcard $(1)/*.c) $$(wildcard $(1)/private/*.c)
$(3)_OBJECTS := $$($(3)_SOURCES:$(1)/%.c=$$(OBJECTDIR)/$(4)/$(5)/$(2)/%.o)
$(3)_PREREQS := $$($(3)_OBJECTS:.o=.d)
$(3)_TARGET := $$(BINARYDIR)/$(4)/$(5)/$(2)
$(3)_EXTRADEPS := $$(if $$(filter yes,$(6)),$$($(4)_$(5)_$(PROJECT)_OBJECTS),)

$$(eval $$(call \
    CREATE_OBJECT_RULES,$(1),$(2),$(3),$(4),$(5)))

$$($(3)_TARGET): LDFLAGS += $$($(4)_LDFLAGS)

$$($(3)_TARGET): LDFLAGS += $$($(5)_LDFLAGS)

$$($(3)_TARGET): TARGET_ARCH := -march=$(4) -m$$(ENV)

$$($(3)_TARGET): $$($(3)_OBJECTS) $$($(3)_EXTRADEPS)
	@ mkdir -p $$(@D)
	@ $$(LINK.o) $$^ $(7) -o $$@
endef

# Create architecture specific build rules.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
# $(5): architecture, e.g. x86-64, corei7.
# $(6): configuration, e.g.: debug, release.
# $(7): whether to add project dependencies to the target.
# $(8): libraries for the linker.
define CREATE_ARCH_RULES =
$$(eval $$(call \
    CREATE_$(1)_RULES,$(2),$(3),$(5)_$(4),$(5),$(6),$(7),$(8)))

ifeq "$$(strip $(1))" "BINARY"
$(4)_TARGETS += $$($(5)_$(4)_TARGET)
endif
endef

# Create configuration specific build rules.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
# $(5): configuration, e.g.: debug, release.
# $(6): whether to add project dependencies to the target.
# $(7): libraries for the linker.
define CREATE_CONFIG_RULES =
$$(foreach _i,$$(ARCH),$$(eval $$(call \
    CREATE_ARCH_RULES,$(1),$(2),$(3),$(5)_$(4),$$(_i),$(5),$(6),$(7))))

ifeq "$$(strip $(1))" "BINARY"
$(4)_TARGETS += $$($(5)_$(4)_TARGETS)
endif
endef

# Create all rule variations for a build.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
# $(5): whether to add project dependencies to the target.
# $(6): libraries for the linker.
define CREATE_RULES =
$$(foreach _i,$$(CONFIG),$$(eval $$(call \
    CREATE_CONFIG_RULES,$(1),$(2),$(3),$(4),$$(_i),$(5),$(6))))
endef

# ------------------------------------------------------------------------------

$(eval $(call \
    CREATE_RULES,OBJECT,src,$(PROJECT),$(PROJECT),no,$(LDLIBS)))

# ------------------------------------------------------------------------------

# Create the rules to build a playground target.
# $(1): target name.
# $(2): path.
# $(3): prefix for variable names.
define CREATE_PLAYGROUND_RULES =
$$(eval $$(call \
    CREATE_RULES,BINARY,$(2),$(2),$(3),yes,$$(PLAYGROUND_LDLIBS)))

playground-$(1): $$($(3)_TARGETS)

.PHONY: playground-$(1)

PLAYGROUND_TARGETS += playground-$(1)
endef

PLAYGROUND_DIR := playground
PLAYGROUND_LDLIBS := $(LDLIBS) -lglfw -lrt -lm -ldl
PLAYGROUND_TARGETS :=

PLAYGROUNDS := $(notdir $(wildcard $(PLAYGROUND_DIR)/*))
$(foreach _i,$(PLAYGROUNDS),$(eval $(call \
    CREATE_PLAYGROUND_RULES,$(_i),$(PLAYGROUND_DIR)/$(_i),PLAYGROUND_$(_i))))

playground: $(PLAYGROUND_TARGETS)

.PHONY: playground

# ------------------------------------------------------------------------------

SHADER_DIR := shaders
SHADER_SOURCES := $(wildcard $(SHADER_DIR)/*.vert) \
                  $(wildcard $(SHADER_DIR)/*.tesc) \
                  $(wildcard $(SHADER_DIR)/*.tese) \
                  $(wildcard $(SHADER_DIR)/*.geom) \
                  $(wildcard $(SHADER_DIR)/*.frag) \
                  $(wildcard $(SHADER_DIR)/*.comp) \

SHADER_OBJECTS := $(SHADER_SOURCES:%=%.spv)

$(SHADER_OBJECTS): %.spv: %
	@ glslangValidator -H -o $@ -s $< > $@.txt

shaders: $(SHADER_OBJECTS)

.PHONY: shaders

# ------------------------------------------------------------------------------

CLANGVERSION := $(shell clang --version \
                        | grep version \
                        | sed 's/^.*version \([0-9]*\.[0-9]*\.[0-9]*\).*$$/\1/')
CLANGDIR := $(shell dirname $(shell which clang))
CLANGINCLUDE := $(CLANGDIR)/../lib/clang/$(CLANGVERSION)/include

format:
	@ clang-format -i -style=file $(ALL_SOURCES) $(ALL_HEADERS)

tidy:
	clang-tidy -fix \
        $(ALL_SOURCES) $(ALL_HEADERS) \
        -- $(CPPFLAGS) $(CFLAGS) -I$(CLANGINCLUDE)

.PHONY: format tidy

# ------------------------------------------------------------------------------

clean:
	@- rm -rf $(OUTDIR)
	@- rm -f $(SHADER_OBJECTS:%=%.txt)

# ------------------------------------------------------------------------------

all: playground shaders

.PHONY: all

.DEFAULT_GOAL := all

# ------------------------------------------------------------------------------

HELP := \
"Usage:\n" \
"  make [options] [target] ...\n" \
"\n" \
"Options:\n" \
"\n" \
"  outdir=<path>\n" \
"    Output directory for the target and intermediary files.\n" \
"    [default: build]\n" \
"\n" \
"  compiler=<type>\n" \
"    Compiler to use, either 'c' or 'cc'.\n" \
"    [default: c]\n" \
"\n" \
"  env=<size>\n" \
"    For x86-64 processors in 64-bit environments, this option forces \n" \
"    generating code to a given target size, either '16', '32', 'x32', \n" \
"    or '64' bits.\n" \
"    [default: 64]\n" \
"\n" \
"  arch=<type>\n" \
"    Target CPU architecture, e.g.: 'x86-64', 'corei7'.\n" \
"    [default: $(DEFAULT_ARCH)]\n" \
"\n" \
"  config=<type>\n" \
"    Build configuration, either 'debug', 'release', or 'all'.\n" \
"    [default: release]\n" \
"\n" \
"  help\n" \
"    Show this screen.\n" \
"\n" \
"  Other options are provided by make, see 'make --help.'\n" \
"\n" \
"Targets:\n" \
"\n" \
"  all (default)\n" \
"  clean\n" \
"  format\n" \
"  playground\n" \
"  $(subst $(space),\n  ,$(sort $(strip $(PLAYGROUND_TARGETS))))\n" \
"  shaders\n" \
"  tidy\n"

help:
	@ echo $(HELP)

.PHONY: help
