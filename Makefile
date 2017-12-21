PROJECT := dekoi
CFLAGS := -std=c99 \
          -Wpedantic -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual \
          -Wconversion -Wfloat-equal -Wpointer-arith -Wshadow \
          -Wstrict-overflow=5 -Wswitch -Wswitch-default -Wundef \
          -Wunreachable-code -Wwrite-strings
CPPFLAGS := -Iinclude
LDFLAGS :=
LDLIBS := -lvulkan

SOURCEDIR := src
SHADERDIR := shaders
OBJECTDIR := build
TARGETDIR := bin

SOURCES := $(shell find $(SOURCEDIR) -name '*.c')
HEADERS := $(shell find $(SOURCEDIR) -name '*.h')
SHADERS := $(shell find $(SHADERDIR) -name '*.vert' \
                                     -o -name '*.tesc' \
                                     -o -name '*.tese' \
                                     -o -name '*.geom' \
                                     -o -name '*.frag' \
                                     -o -name '*.comp')

CLANGVERSION := $(shell clang --version \
                        | grep version \
                        | sed 's/^.*version \([0-9]*\.[0-9]*\.[0-9]*\).*$$/\1/')
CLANGDIR := $(shell dirname $(shell which clang))
CLANGINCLUDE := $(CLANGDIR)/../lib/clang/$(CLANGVERSION)/include

DEBUGCFLAGS := -O0 -g
DEBUGCPPFLAGS := -DDEBUG
DEBUGOBJECTDIR := $(OBJECTDIR)/debug
DEBUGOBJECTS := $(SOURCES:$(SOURCEDIR)/%.c=$(DEBUGOBJECTDIR)/%.o)

RELEASECFLAGS := -O3
RELEASECPPFLAGS := -DNDEBUG
RELEASEOBJECTDIR := $(OBJECTDIR)/release
RELEASEOBJECTS := $(SOURCES:$(SOURCEDIR)/%.c=$(RELEASEOBJECTDIR)/%.o)

SHADEROBJECTS := $(SHADERS:$(SHADERDIR)/%=$(SHADERDIR)/%.spv)

VULKANLAYERPATH := /usr/share/vulkan/explicit_layer.d


$(DEBUGOBJECTS): $(DEBUGOBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $(DEBUGCFLAGS) $(CPPFLAGS) $(DEBUGCPPFLAGS) -o $@ $<

$(RELEASEOBJECTS): $(RELEASEOBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $(RELEASECFLAGS) $(CPPFLAGS) $(RELEASECPPFLAGS) -o $@ $<

$(SHADEROBJECTS): $(SHADERDIR)/%.spv: $(SHADERDIR)/%
	@glslangValidator -H -o $@ -s $< > $@.txt


.PHONY: shaders

shaders: $(SHADEROBJECTS)


.PHONY: playground test testdebug testrelease runtestdebug runtestrelease

PLAYGROUNDTESTTARGET := test
PLAYGROUNDTESTSOURCEDIR := playground/$(PLAYGROUNDTESTTARGET)
PLAYGROUNDTESTOBJECTDIR := $(OBJECTDIR)/playground
PLAYGROUNDTESTSOURCES := $(shell find $(PLAYGROUNDTESTSOURCEDIR) -name '*.c')
PLAYGROUNDTESTHEADERS := $(shell find $(PLAYGROUNDTESTSOURCEDIR) -name '*.h')
PLAYGROUNDTESTLDLIBS := -lglfw -lrt -lm -ldl


PLAYGROUNDTESTDEBUGOBJECTDIR := $(PLAYGROUNDTESTOBJECTDIR)/debug
PLAYGROUNDTESTDEBUGTARGETDIR := $(TARGETDIR)/playground/debug
PLAYGROUNDTESTDEBUGOBJECTS := $(PLAYGROUNDTESTSOURCES:$(PLAYGROUNDTESTSOURCEDIR)/%.c=$(PLAYGROUNDTESTDEBUGOBJECTDIR)/%.o)

$(PLAYGROUNDTESTDEBUGOBJECTS): $(PLAYGROUNDTESTDEBUGOBJECTDIR)/%.o: $(PLAYGROUNDTESTSOURCEDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $(DEBUGCFLAGS) $(CPPFLAGS) $(DEBUGCPPFLAGS) -o $@ $<

testdebug: $(DEBUGOBJECTS) $(PLAYGROUNDTESTDEBUGOBJECTS) shaders
	@mkdir -p $(PLAYGROUNDTESTDEBUGTARGETDIR)
	@$(CC) $(DEBUGOBJECTS) $(PLAYGROUNDTESTDEBUGOBJECTS) $(LDFLAGS) $(LDLIBS) $(PLAYGROUNDTESTLDLIBS) -o $(PLAYGROUNDTESTDEBUGTARGETDIR)/$(PLAYGROUNDTESTTARGET)

runtestdebug:
	@VK_LAYER_PATH=$(VULKANLAYERPATH) $(PLAYGROUNDTESTDEBUGTARGETDIR)/$(PLAYGROUNDTESTTARGET)


PLAYGROUNDTESTRELEASEOBJECTDIR := $(PLAYGROUNDTESTOBJECTDIR)/release
PLAYGROUNDTESTRELEASETARGETDIR := $(TARGETDIR)/playground/release
PLAYGROUNDTESTRELEASEOBJECTS := $(PLAYGROUNDTESTSOURCES:$(PLAYGROUNDTESTSOURCEDIR)/%.c=$(PLAYGROUNDTESTRELEASEOBJECTDIR)/%.o)

$(PLAYGROUNDTESTRELEASEOBJECTS): $(PLAYGROUNDTESTRELEASEOBJECTDIR)/%.o: $(PLAYGROUNDTESTSOURCEDIR)/%.c
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $(RELEASECFLAGS) $(CPPFLAGS) $(RELEASECPPFLAGS) -o $@ $<

testrelease: $(RELEASEOBJECTS) $(PLAYGROUNDTESTRELEASEOBJECTS) shaders
	@mkdir -p $(PLAYGROUNDTESTRELEASETARGETDIR)
	@$(CC) $(RELEASEOBJECTS) $(PLAYGROUNDTESTRELEASEOBJECTS) $(LDFLAGS) $(LDLIBS) $(PLAYGROUNDTESTLDLIBS) -o $(PLAYGROUNDTESTRELEASETARGETDIR)/$(PLAYGROUNDTESTTARGET)

runtestrelease:
	@VK_LAYER_PATH=$(VULKANLAYERPATH) $(PLAYGROUNDTESTRELEASETARGETDIR)/$(PLAYGROUNDTESTTARGET)


test: testdebug testrelease

playground: test


.PHONY: all format tidy clean

all: playground

format:
	@-clang-format -i -style=file \
		$(SOURCES) $(HEADERS) \
		$(PLAYGROUNDTESTSOURCES) $(PLAYGROUNDTESTHEADERS)

tidy:
	clang-tidy -fix \
		$(SOURCES) $(PLAYGROUNDTESTSOURCES) \
		-- $(CFLAGS) $(CPPFLAGS) $(RELEASECFLAGS) -I$(CLANGINCLUDE)

clean:
	@-rm -rf $(OBJECTDIR)
	@-rm -rf $(TARGETDIR)
	@-rm -f $(SHADEROBJECTS) $(SHADEROBJECTS:%=%.txt)
