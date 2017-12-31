space :=
space +=
\t := $(space)$(space)

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

# Expand a single local object dependency.
# $(1): variable to save the output to.
# $(2): dependency.
# $(3): configuration, e.g.: debug, release.
# $(4): architecture, e.g. x86-64, i386.
define EXPAND_LOCALDEP =
    $(1) := $$($1) $$($(4)_$(3)_$(2)_OBJECTS)
endef

# Expand a bunch of local object dependencies.
# $(1): variable to save the output to.
# $(2): dependencies.
# $(3): configuration, e.g.: debug, release.
# $(4): architecture, e.g. x86-64, i386.
define EXPAND_LOCALDEPS =
$$(foreach _i,$(2),$$(eval $$(call \
    EXPAND_LOCALDEP,$(1),$$(_i),$(3),$(4))))
endef

# Create build rules for object targets.
# $(1): source path, e.g.: src, playground/test.
# $(2): target path, e.g.: dekoi, playground/test.
# $(3): prefix for variable names.
# $(4): configuration, e.g.: debug, release.
# $(5): architecture, e.g. x86-64, i386.
define CREATE_OBJECT_RULES =
$(5)_$(4)_$(3)_SOURCES := $$(wildcard $(1)/*.c) $$(wildcard $(1)/private/*.c)
$(5)_$(4)_$(3)_HEADERS := $$(wildcard $(1)/*.h) $$(wildcard $(1)/private/*.h)
$(5)_$(4)_$(3)_OBJECTS := \
    $$($(5)_$(4)_$(3)_SOURCES:$(1)/%.c=$$(OBJECTDIR)/$(5)/$(4)/$(2)/%.o)
$(5)_$(4)_$(3)_PREREQS := $$($(5)_$(4)_$(3)_OBJECTS:.o=.d)

$$($(5)_$(4)_$(3)_OBJECTS): CFLAGS += \
    $$($(4)_CFLAGS) $$($(5)_CFLAGS) $$($(5)_$(4)_CFLAGS)
$$($(5)_$(4)_$(3)_OBJECTS): CXXFLAGS += \
    $$($(4)_CXXFLAGS) $$($(5)_CXXFLAGS) $$($(5)_$(4)_CXXFLAGS)
$$($(5)_$(4)_$(3)_OBJECTS): CPPFLAGS += \
    $$($(4)_CPPFLAGS) $$($(5)_CPPFLAGS) $$($(5)_$(4)_CPPFLAGS)

$$($(5)_$(4)_$(3)_OBJECTS): TARGET_ARCH := -march=$(5) -m$$(ENV)

$$($(5)_$(4)_$(3)_OBJECTS): $$(OBJECTDIR)/$(5)/$(4)/$(2)/%.o: $(1)/%.c
	@ mkdir -p $$(@D)
	@ $$(COMPILE) -o $$@ $$<

-include $$($(5)_$(4)_$(3)_PREREQS)

ALL_SOURCES += $$($(5)_$(4)_$(3)_SOURCES)
ALL_HEADERS += $$($(5)_$(4)_$(3)_HEADERS)
endef

# Create build rules for binary targets.
# $(1): source path, e.g.: src, playground/test.
# $(2): target path, e.g.: dekoi, playground/test.
# $(3): prefix for variable names.
# $(4): configuration, e.g.: debug, release.
# $(5): architecture, e.g. x86-64, i386.
define CREATE_BINARY_RULES =
$(5)_$(4)_$(3)_SOURCES := $$(wildcard $(1)/*.c) $$(wildcard $(1)/private/*.c)
$(5)_$(4)_$(3)_TARGET := $$(BINARYDIR)/$(5)/$(4)/$(2)
$(5)_$(4)_$(3)_DEPS :=
$(5)_$(4)_$(3)_LDLIBS := \
    $$($(3)_LDLIBS) $$($(4)_$(3)_LDLIBS) $$($(5)_$(3)_LDLIBS)

$$(eval $$(call \
    EXPAND_LOCALDEPS,$(5)_$(4)_$(3)_DEPS,$$($(3)_LOCALDEPS),$(4),$(5)))

$$(eval $$(call \
    CREATE_OBJECT_RULES,$(1),$(2),$(3),$(4),$(5)))

$$($(5)_$(4)_$(3)_TARGET): LDFLAGS += \
    $$($(4)_LDFLAGS) $$($(5)_LDFLAGS) $$($(5)_$(4)_LDFLAGS)

$$($(5)_$(4)_$(3)_TARGET): TARGET_ARCH := -march=$(5) -m$$(ENV)

$$($(5)_$(4)_$(3)_TARGET): $$($(5)_$(4)_$(3)_DEPS) $$($(5)_$(4)_$(3)_OBJECTS)
	@ mkdir -p $$(@D)
	@ $$(LINK.o) $$^ $$($(5)_$(4)_$(3)_LDLIBS) -o $$@
endef

# Create architecture specific build rules.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
# $(5): configuration, e.g.: debug, release.
# $(6): architecture, e.g. x86-64, i386.
define CREATE_ARCH_RULES =
$$(eval $$(call \
    CREATE_$(1)_RULES,$(2),$(3),$(4),$(5),$(6)))

ifeq "$$(strip $(1))" "BINARY"
$(5)_$(4)_TARGETS += $$($(6)_$(5)_$(4)_TARGET)
endif
endef

# Create configuration specific build rules.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
# $(5): configuration, e.g.: debug, release.
define CREATE_CONFIG_RULES =
$$(foreach _i,$$(ARCH),$$(eval $$(call \
    CREATE_ARCH_RULES,$(1),$(2),$(3),$(4),$(5),$$(_i))))

ifeq "$$(strip $(1))" "BINARY"
$(4)_TARGETS += $$($(5)_$(4)_TARGETS)
endif
endef

# Create all rule variations for a build.
# $(1): build target, e.g.: BINARY.
# $(2): source path, e.g.: src, playground/test.
# $(3): target path, e.g.: dekoi, playground/test.
# $(4): prefix for variable names.
define CREATE_RULES =
$$(foreach _i,$$(CONFIG),$$(eval $$(call \
    CREATE_CONFIG_RULES,$(1),$(2),$(3),$(4),$$(_i))))
endef

# ------------------------------------------------------------------------------

$(PROJECT)_LDLIBS := $(LDLIBS)

$(eval $(call \
    CREATE_RULES,OBJECT,src,$(PROJECT),$(PROJECT)))

# ------------------------------------------------------------------------------

PLAYGROUNDS_TARGETS :=
PLAYGROUNDS_PHONY_TARGETS :=

# Create the rules to build a playground target.
# $(1): target name.
# $(2): path.
# $(3): prefix for variable names.
define CREATE_PLAYGROUND_RULES =
$(3)_$(1)_LOCALDEPS := $$(PROJECT)
$(3)_$(1)_LDLIBS := $$(LDLIBS) -lglfw -lrt -lm -ldl

$$(eval $$(call \
    CREATE_RULES,BINARY,$(2),$(2),$(3)_$(1)))

$(3)-$(1): $$($(3)_$(1)_TARGETS)

.PHONY: $(3)-$(1)

PLAYGROUNDS_TARGETS += $$($(3)_$(1)_TARGETS)
PLAYGROUNDS_PHONY_TARGETS += $(3)-$(1)
endef

# Create the rules for all the playground targets.
# $(1): path.
# $(2): prefix for variable names.
define CREATE_PLAYGROUNDS_RULES =
PLAYGROUNDS := $$(notdir $$(wildcard $(1)/*))
$$(foreach _i,$$(PLAYGROUNDS),$$(eval $$(call \
    CREATE_PLAYGROUND_RULES,$$(_i),$(1)/$$(_i),$(2))))

playgrounds: $$(PLAYGROUNDS_PHONY_TARGETS)

.PHONY: playgrounds
endef

$(eval $(call \
    CREATE_PLAYGROUNDS_RULES,playground,playground,common))

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

all: playgrounds shaders

.PHONY: all

.DEFAULT_GOAL := all

# ------------------------------------------------------------------------------

HELP := "Usage:\n \
$(\t)make [options] [target] ...\n \
\n \
Options:\n \
\n \
$(\t)outdir=<path>\n \
$(\t)$(\t)Output directory for the target and intermediary files.\n \
$(\t)$(\t)[default: build]\n \
\n \
$(\t)compiler=<type>\n \
$(\t)$(\t)Compiler to use, either 'c' or 'cc'.\n \
$(\t)$(\t)[default: c]\n \
\n \
$(\t)env=<size>\n \
$(\t)$(\t)For x86-64 processors, this option forces generating code to \n \
$(\t)$(\t)a given target size. Either '16', '32', 'x32', or '64'.\n \
$(\t)$(\t)[default: 64]\n \
\n \
$(\t)arch=<type>\n \
$(\t)$(\t)Target CPU architecture, e.g.: 'x86-64', 'i386'.\n \
$(\t)$(\t)[default: $(DEFAULT_ARCH)]\n \
\n \
$(\t)config=<type>\n \
$(\t)$(\t)Build configuration, either 'debug', 'release', or 'all'.\n \
$(\t)$(\t)[default: release]\n \
\n \
$(\t)help\n \
$(\t)$(\t)Show this screen.\n \
\n \
$(\t)Other options are provided by make, see 'make --help.'\n \
\n \
Targets:\n \
\n \
$(\t)all (default)\n \
$(\t)clean\n \
$(\t)format\n \
$(\t)playground\n \
$(\t)$(subst $(space),\n$(\t),$(sort $(strip $(PLAYGROUNDS_PHONY_TARGETS))))\n \
$(\t)shaders\n \
$(\t)tidy"

help:
	@ echo $(subst \n$(space),\n,$(HELP))

.PHONY: help
