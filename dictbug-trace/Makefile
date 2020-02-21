all: libdictbug-trace.so

clean:
	rm -f libdictbug-trace.so*

.PHONY: all clean

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

CFLAGS     ?= -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb -fPIC -shared
override CFLAGS := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) -Wall -Wno-unknown-pragmas

LDFLAGS     ?= -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib -ldl -s -Wl,-soname,libdictbug-trace.so
override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS)

libdictbug-trace.so: dictbug-trace.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)
