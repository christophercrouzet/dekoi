PROJECT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ------------------------------------------------------------------------------

ifndef outdir
OUT_DIR := build
else
OUT_DIR := $(outdir)
endif

ifndef config
CONFIG := release
else ifneq "$(filter-out debug release all,$(config))" ""
$(error the 'config' option is not valid)
else ifneq "$(filter all,$(config))" ""
CONFIG := debug release
else
CONFIG := $(config)
endif

# ------------------------------------------------------------------------------

PROJECT := dekoi

# ------------------------------------------------------------------------------

BUILD_DIRS :=
MAKE_FILES :=
FORMAT_FILES :=
TIDY_FILES :=

# ------------------------------------------------------------------------------

# $(1): build directory.
# $(2): rule.
define dk_forward_rule_impl =
$(MAKE) -C $(1) -s $(2)
endef

# Forward a rule to the generated Makefiles.
# $(1): rule.
define dk_forward_rule =
$(foreach _x,$(BUILD_DIRS), $(call \
	dk_forward_rule_impl,$(_x),$(1)))
endef

# ------------------------------------------------------------------------------

# Create a Makefile rule.
# # $(1): configuration.
define dk_create_makefile =
$$(OUT_DIR)/$(1)/Makefile:
	@ mkdir -p $$(OUT_DIR)/$(1)
	@ cd $$(OUT_DIR)/$(1) && cmake \
		-DCMAKE_BUILD_TYPE=$(1) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_INSTALL_PREFIX=$(PROJECT_DIR)/_install \
		$(PROJECT_DIR)

BUILD_DIRS += $$(OUT_DIR)/$(1)
MAKE_FILES += $$(OUT_DIR)/$(1)/Makefile
endef

$(foreach _config,$(CONFIG),$(eval $(call \
	dk_create_makefile,$(_config))))

# ------------------------------------------------------------------------------

MODULES := $(notdir $(wildcard src/*))

MODULES_FILES :=
MODULES_FILES += $(foreach _x,$(MODULES),$(wildcard src/$(_x)/*.[ch]))
MODULES_FILES += $(foreach _x,$(MODULES),$(wildcard src/$(_x)/private/*.[ch]))

modules: $(MAKE_FILES)
	@ $(call dk_forward_rule,modules)

.PHONY: modules

FORMAT_FILES += $(MODULES_FILES)
TIDY_FILES += $(MODULES_FILES)

# ------------------------------------------------------------------------------

DEMOS := $(notdir $(wildcard demos/*))

DEMOS_FILES := $(foreach _x,$(DEMOS),$(wildcard demos/$(_x)/*.[ch]))

demos: $(MAKE_FILES)
	@ $(call dk_forward_rule,demos)

.PHONY: demos

FORMAT_FILES += $(DEMOS_FILES)
TIDY_FILES += $(DEMOS_FILES)

# ------------------------------------------------------------------------------

CLANG_VERSION := $(shell \
	clang --version \
	| grep version \
	| sed 's/^.*version \([0-9]*\.[0-9]*\.[0-9]*\).*$$/\1/')
CLANG_DIR := $(shell dirname $(shell which clang))
CLANG_INCLUDE_DIR := $(CLANG_DIR)/../lib/clang/$(CLANG_VERSION)/include

format:
	@ clang-format -i -style=file $(FORMAT_FILES)

tidy: $(firstword $(MAKE_FILES))
	@ clang-tidy $(TIDY_FILES) \
		-p $(firstword $(BUILD_DIRS))/compile_commands.json \
		-- -I$(CLANG_INCLUDE_DIR)

.PHONY: format tidy

# ------------------------------------------------------------------------------

install: $(MAKE_FILES)
	@ $(call dk_forward_rule,install)

.PHONY: install

# ------------------------------------------------------------------------------

clean:
	@- rm -rf $(OUT_DIR)

.PHONY: clean

# ------------------------------------------------------------------------------

all: $(MAKE_FILES)

.PHONY: all

.DEFAULT_GOAL := all
