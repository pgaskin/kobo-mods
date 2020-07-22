define deps =
 $(call pkgconf,QT5CORE,Qt5Core) \
 $(call pkgconf,QT5WIDGETS,Qt5Widgets) \
 $(call pkgconf,QT5NETWORK,Qt5Network)
endef
include ../.lib/nickeltc.mk

CFLAGS  ?= $(CFLAGS_KOBO) -fPIC -shared
LDFLAGS ?= $(LDFLAGS_KOBO) -ldl -s

all: libdictbug-trace.so ld.so.preload

clean:
	rm -f libdictbug-trace.so*

koboroot:
	tar cvzf KoboRoot.tgz --show-transformed --owner=root --group=root --mode="u=rwX,go=rX" --transform="s,libdictbug-trace.so,./usr/local/pgaskin/libdictbug-trace.so," --transform="s,ld.so.preload,./etc/ld.so.preload," libdictbug-trace.so ld.so.preload

.PHONY: all clean koboroot

libdictbug-trace.so: override CFLAGS  := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) $(QT5NETWORK_CFLAGS) -Wall -Wno-unknown-pragmas
libdictbug-trace.so: override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS) $(QT5NETWORK_LIBS) -Wl,-soname,libdictbug-trace.so
libdictbug-trace.so: dictbug-trace.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)

ld.so.preload:
	echo "/usr/local/pgaskin/libdictbug-trace.so" > ld.so.preload
