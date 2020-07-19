CROSS_COMPILE      = arm-nickel-linux-gnueabihf-
CC                 = $(CROSS_COMPILE)gcc
CXX                = $(CROSS_COMPILE)g++
STRIP              = $(CROSS_COMPILE)strip
PKG_CONFIG         = $(CROSS_COMPILE)pkg-config
MOC                = moc

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
$(call deps)
endif

CFLAGS_KOBO  = -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb
LDFLAGS_KOBO = -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib

## Makefile
# define deps =
#  $(call pkgconf,QT5CORE,Qt5Core) \
#  ...
# endef
# include ../nickeltc.mk
#
# CFLAGS  ?= $(CFLAGS_KOBO) ...
# LDFLAGS ?= $(LDFLAGS_KOBO) ...
#
# ...
