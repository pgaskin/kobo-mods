define deps =
 $(call pkgconf,QT5CORE,Qt5Core)
 $(call pkgconf,QT5GUI,Qt5Gui)
endef
include ../nickeltc.mk

PTHREAD_CFLAGS := -pthread
PTHREAD_LIBS   := -pthread

CFLAGS   ?= $(CFLAGS_KOBO)
CXXFLAGS ?= $(CFLAGS_KOBO)
LDFLAGS  ?= $(LDFLAGS_KOBO) -Wl,--as-needed

override CFLAGS   += -std=gnu11 -Wall -Wextra -Werror -isystemlib
override CXXFLAGS += -std=gnu++11 -Wall -Wextra -Werror -isystemlib
override LDFLAGS  += -Wl,--no-undefined

NS_VERSION := $(shell git describe --tags --always --dirty)
ifdef NS_VERSION
	override CPPFLAGS += -DNS_VERSION='"$(NS_VERSION)"'
endif

define GITIGNORE_HEAD
# make gitignore

# KDevelop
.kdev4/
NickelMenu.kdev4
.kateconfig

# VSCode
.vscode/

# clangd
compile_flags.txt

# Build artifacts
endef
export GITIGNORE_HEAD

all: src/libns.so

clean:
	rm -f $(GENERATED)

gitignore:
	echo "$${GITIGNORE_HEAD}" > .gitignore
	echo '$(GENERATED)' | \
		sed 's/ /\n/g' | \
		sed 's/^./\/&/' >> .gitignore

install:
	install -Dm644 src/libns.so $(DESTDIR)/usr/local/Kobo/imageformats/libns.so

koboroot:
	tar cvzf KoboRoot.tgz --show-transformed --owner=root --group=root --mode="u=rwX,go=rX" --transform="s,src/libns.so,./usr/local/Kobo/imageformats/libns.so," src/libns.so

.PHONY: all clean gitignore install koboroot
override GENERATED += KoboRoot.tgz

src/libns.so: override CFLAGS   += $(PTHREAD_CFLAGS) -fPIC
src/libns.so: override CXXFLAGS += $(PTHREAD_CFLAGS) $(QT5CORE_CFLAGS) $(QT5GUI_CFLAGS) -fPIC
src/libns.so: override LDFLAGS  += $(PTHREAD_LIBS) $(QT5CORE_LIBS) $(QT5GUI_LIBS) -ldl -Wl,-soname,libns.so
src/libns.so: src/qtplugin.o src/hook.o src/series.o lib/miniz/miniz.o lib/nm/dlhook.o lib/nm/failsafe.o

lib/nm/dlhook.o lib/nm/failsafe.o: override CFLAGS += -DNM_LOG_NAME='"NickelSeries"'

override LIBRARIES += src/libns.so
override MOCS      += src/qtplugin.moc

define patw =
 $(foreach dir,src lib/miniz lib/nm,$(dir)/*$(1))
endef
define rpatw =
 $(patsubst %$(1),%$(2),$(foreach w,$(call patw,$(1)),$(wildcard $(w))))
endef

$(LIBRARIES): src/%.so:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -shared -o $@ $^ $(LDFLAGS)
$(MOCS): %.moc: %.h
	$(MOC) $< -o $@
$(patsubst %.moc,%.o,$(MOCS)): %.o: %.moc
	$(CXX) -xc++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(call rpatw,.c,.o): %.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(call rpatw,.cc,.o): %.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

override GENERATED += $(LIBRARIES)  $(MOCS) $(patsubst %.moc,%.o,$(MOCS)) $(call rpatw,.c,.o) $(call rpatw,.cc,.o)
