define deps =
 $(call pkgconf,QT5CORE,Qt5Core) \
 $(call pkgconf,QT5WIDGETS,Qt5Widgets)
endef
include ../.lib/nickeltc.mk

# Note: this builds a executable library:
#  https://unix.stackexchange.com/questions/223385/why-and-how-are-some-shared-libraries-runnable-as-though-they-are-executables
#  https://stackoverflow.com/questions/30621632/how-is-the-gnu-libc-so-both-a-shared-object-and-a-standalone-executable
#  http://rachid.koucha.free.fr/tech_corner/executable_lib.html
#  https://polentino911.wordpress.com/2013/08/08/make-your-own-executable-shared-library-on-linux/

# Note: the order matters; it will error with "undefined reference to `main'" if
# -shared is before -pie.

CFLAGS     ?= $(CFLAGS_KOBO)
CFLAGS_BIN ?= -pie
CFLAGS_LIB ?= -fPIC -shared
override CFLAGS := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) $(CFLAGS_BIN) $(CFLAGS_LIB) -Wall -Wno-unknown-pragmas

LDFLAGS     ?= $(LDFLAGS_KOBO) -ldl -s
LDFLAGS_BIN ?= -Wl,-e,_dwt_main_shim
LDFLAGS_LIB ?= -Wl,-soname,libdictword-test.so
override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS) $(LDFLAGS_BIN) $(LDFLAGS_LIB)

all: dictword-test.so

clean:
	rm -f dictword-test.so*

.PHONY: all clean

dictword-test.so: dictword-test.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)
