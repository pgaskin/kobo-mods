ifdef NILUJE
HOST               = arm-nickel-linux-gnueabihf
CXX                = $(NILUJE)/bin/$(HOST)-g++
QT5CORE_CFLAGS    ?= -I$(NILUJE)/$(HOST)/sysroot/include/QtCore
QT5CORE_LIBS      ?= -L$(NILUJE)/$(HOST)/sysroot/lib -lQt5Core
QT5WIDGETS_CFLAGS ?= -I$(NILUJE)/$(HOST)/sysroot/include/QtWidgets
QT5WIDGETS_LIBS   ?= -L$(NILUJE)/$(HOST)/sysroot/lib -lQt5Widgets

ifdef DOCKER
DOCKER_IMAGE ?= geek1011/kobo-toolchain:latest
override CXX := $(DOCKER) run --volume=.:/src --workdir=/src --rm -it --entrypoint=$(CXX) $(DOCKER_IMAGE)
endif

override CXX := $(CXX) -I$(NILUJE)/$(HOST)/sysroot/include -L$(NILUJE)/$(HOST)/sysroot/lib
endif

ifndef NILUJE
CXX                = $(CROSS_PREFIX)g++
PKGCONFIG          = $(CROSS_PREFIX)pkg-config
QT5CORE_CFLAGS    ?= $(shell $(PKGCONFIG) --cflags Qt5Core)
QT5CORE_LIBS      ?= $(shell $(PKGCONFIG) --libs   Qt5Core)
QT5WIDGETS_CFLAGS ?= $(shell $(PKGCONFIG) --cflags Qt5Widgets)
QT5WIDGETS_LIBS   ?= $(shell $(PKGCONFIG) --libs   Qt5Widgets)
endif

# Note: this builds a executable library:
#  https://unix.stackexchange.com/questions/223385/why-and-how-are-some-shared-libraries-runnable-as-though-they-are-executables
#  https://stackoverflow.com/questions/30621632/how-is-the-gnu-libc-so-both-a-shared-object-and-a-standalone-executable
#  http://rachid.koucha.free.fr/tech_corner/executable_lib.html
#  https://polentino911.wordpress.com/2013/08/08/make-your-own-executable-shared-library-on-linux/

# Note: the order matters; it will error with "undefined reference to `main'" if
# -shared is before -pie.

CFLAGS     ?= -Wall -Wno-unknown-pragmas
CFLAGS_BIN ?= -pie
CFLAGS_LIB ?= -fPIC -shared
override CFLAGS := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) $(CFLAGS_BIN) $(CFLAGS_LIB)

LDFLAGS     ?= -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib -ldl -s
LDFLAGS_BIN ?= -Wl,-e,_dwt_main_shim
LDFLAGS_LIB ?= -Wl,-soname,libdictword-test.so
override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS) $(LDFLAGS_BIN) $(LDFLAGS_LIB)

all: dictword-test.so

clean:
	rm -f dictword-test.so*

dictword-test.so: dictword-test.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)

.PHONY: all clean
