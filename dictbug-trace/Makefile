CROSS_COMPILE      = arm-nickel-linux-gnueabihf-
CXX                = $(CROSS_COMPILE)g++
PKG_CONFIG         = $(CROSS_COMPILE)pkg-config

ifneq ($(if $(MAKECMDGOALS),$(if $(filter-out clean koboroot,$(MAKECMDGOALS)),YES,NO),YES),YES)
 $(info -- Skipping configure)
else
define pkgconf =
 $(if $(filter-out undefined,$(origin $(1)_CFLAGS) $(origin $(1)_LIBS)) \
 ,$(info -- Using provided CFLAGS and LIBS for $(2)) \
 ,$(if $(shell $(PKG_CONFIG) --exists $(2) >/dev/null 2>/dev/null && echo y) \
  ,$(info -- Found $(2) ($(shell $(PKG_CONFIG) --modversion $(2))) with pkg-config) \
   $(eval $(1)_CFLAGS := $(shell $(PKG_CONFIG) --silence-errors --cflags $(2))) \
   $(eval $(1)_LIBS   := $(shell $(PKG_CONFIG) --silence-errors --libs $(2))) \
   $(if $(3) \
   ,$(if $(shell $(PKG_CONFIG) $(3) $(2) >/dev/null 2>/dev/null && echo y) \
    ,$(info .. Satisfies constraint $(3)) \
    ,$(info .. Does not satisfy constraint $(3)) \
     $(error Dependencies do not satisfy constraints)) \
   ,) \
  ,$(info -- Could not automatically detect $(2) with pkg-config. Please specify $(1)_CFLAGS and/or $(1)_LIBS manually) \
   $(error Missing dependencies)))
endef

$(call pkgconf,QT5CORE,Qt5Core)
$(call pkgconf,QT5WIDGETS,Qt5Widgets)
$(call pkgconf,QT5NETWORK,Qt5Network)
endif

CFLAGS  ?= -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb -fPIC -shared
LDFLAGS ?= -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib -ldl -s

all: libdictbug-trace.so ld.so.preload

clean:
	rm -f libdictbug-trace.so*

koboroot:
	tar cvzf KoboRoot.tgz --show-transformed --owner=root --group=root --mode="u=rwX,go=rX" --transform="s,libdictbug-trace.so,./usr/local/geek1011/libdictbug-trace.so," --transform="s,ld.so.preload,./etc/ld.so.preload," libdictbug-trace.so ld.so.preload

.PHONY: all clean koboroot

libdictbug-trace.so: override CFLAGS  := $(CFLAGS) $(QT5CORE_CFLAGS) $(QT5WIDGETS_CFLAGS) $(QT5NETWORK_CFLAGS) -Wall -Wno-unknown-pragmas
libdictbug-trace.so: override LDFLAGS := $(LDFLAGS) $(QT5CORE_LIBS) $(QT5WIDGETS_LIBS) $(QT5NETWORK_LIBS) -Wl,-soname,libdictbug-trace.so
libdictbug-trace.so: dictbug-trace.cc
	$(CXX) $(CFLAGS) "$^" -o "$@" $(LDFLAGS)

ld.so.preload:
	echo "/usr/local/geek1011/libdictbug-trace.so" > ld.so.preload
