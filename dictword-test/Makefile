all: dictword-test.so dwt-wrap

clean:
	rm -f dictword-test.so*
	rm -f dwt-wrap

.PHONY: all clean

# dictword-test.so

ifdef NILUJE
HOST               = arm-nickel-linux-gnueabihf
CXX                = $(NILUJE)/bin/$(HOST)-g++
QT5CORE_CFLAGS    ?= -I$(NILUJE)/$(HOST)/sysroot/include/QtCore
QT5CORE_LIBS      ?= -L$(NILUJE)/$(HOST)/sysroot/lib -lQt5Core
QT5WIDGETS_CFLAGS ?= -I$(NILUJE)/$(HOST)/sysroot/include/QtWidgets
QT5WIDGETS_LIBS   ?= -L$(NILUJE)/$(HOST)/sysroot/lib -lQt5Widgets

ifdef DOCKER
DOCKER_IMAGE ?= geek1011/kobo-toolchain:latest
override CXX := $(DOCKER) run --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir=/src --rm --entrypoint=$(CXX) $(DOCKER_IMAGE)
endif

override CXX := $(CXX) -I$(NILUJE)/$(HOST)/sysroot/include -L$(NILUJE)/$(HOST)/sysroot/lib
endif

ifndef NILUJE
CXX                = $(CROSS_PREFIX)g++
PKG_CONFIG         = $(CROSS_PREFIX)pkg-config
QT5CORE_CFLAGS    ?= $(shell $(PKG_CONFIG) --cflags Qt5Core)
QT5CORE_LIBS      ?= $(shell $(PKG_CONFIG) --libs   Qt5Core)
QT5WIDGETS_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags Qt5Widgets)
QT5WIDGETS_LIBS   ?= $(shell $(PKG_CONFIG) --libs   Qt5Widgets)
endif

# Note: this builds a executable library:
#  https://unix.stackexchange.com/questions/223385/why-and-how-are-some-shared-libraries-runnable-as-though-they-are-executables
#  https://stackoverflow.com/questions/30621632/how-is-the-gnu-libc-so-both-a-shared-object-and-a-standalone-executable
#  http://rachid.koucha.free.fr/tech_corner/executable_lib.html
#  https://polentino911.wordpress.com/2013/08/08/make-your-own-executable-shared-library-on-linux/

# Note: the order matters; it will error with "undefined reference to `main'" if
# -shared is before -pie.

CFLAGS     ?= -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb
CFLAGS_BIN ?= -pie
CFLAGS_LIB ?= -fPIC -shared
override CFLAGS := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) $(CFLAGS_BIN) $(CFLAGS_LIB) -Wall -Wno-unknown-pragmas

LDFLAGS     ?= -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib -ldl -s
LDFLAGS_BIN ?= -Wl,-e,_dwt_main_shim
LDFLAGS_LIB ?= -Wl,-soname,libdictword-test.so
override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS) $(LDFLAGS_BIN) $(LDFLAGS_LIB)

dictword-test.so: dictword-test.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)

# dwt-wrap

GO         = go
GOOS      ?= linux
GOARCH    ?= arm
GOARM     ?= 5
GOFLAGS   ?= -trimpath
GOLDFLAGS ?= -s -w

ifdef DOCKER
DOCKER_IMAGE_GO ?= golang:1.13-buster
override GO := $(DOCKER) run --volume="$(dir $(realpath $(firstword $(MAKEFILE_LIST)))):/src" --workdir=/src --rm --env=CGO_ENABLED --env=GOOS --env=GOARCH --env=GOARM --entrypoint=$(GO) $(DOCKER_IMAGE_GO)
endif

dwt-wrap: dwt-wrap.go
	CGO_ENABLED=0 GOOS=$(GOOS) GOARCH=$(GOARCH) GOARM=$(GOARM) $(GO) env
	CGO_ENABLED=0 GOOS=$(GOOS) GOARCH=$(GOARCH) GOARM=$(GOARM) $(GO) build -ldflags "$(GOLDFLAGS)" $(GOFLAGS) -o "$@" "$^"

