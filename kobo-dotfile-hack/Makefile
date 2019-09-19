CC       = $(CROSS_PREFIX)gcc
DESTDIR ?= /
PREFIX  ?= /usr/local/geek1011
CONFIG   = -DUSE_FULL_PATH -DNICKEL_ONLY
CFLAGS  ?= -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb

override CFLAGS := -Wall -shared -ldl -std=gnu99 -fPIC $(CONFIG) $(CFLAGS)

.PHONY: all install uninstall clean

all: libhidedir_kobo.so ld.so.preload KoboRoot.tgz

install: all
	install -Dm 0644 libhidedir_kobo.so $(DESTDIR)$(PREFIX)
	install -Dm 0644 ld.so.preload $(DESTDIR)/etc

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/libhidedir_kobo.so
	rm -f $(DESTDIR)/etc/ld.so.preload

clean:
	rm -f libhidedir_kobo.so ld.so.preload KoboRoot.tgz

KoboRoot.tgz: libhidedir_kobo.so ld.so.preload
	tar czf $@ $^ --group root:0 --owner root:0 \
		--transform "s,libhidedir_kobo.so,.$(PREFIX)/libhidedir_kobo.so," \
		--transform "s,ld.so.preload,./etc/ld.so.preload,"
	tar tvf $@

libhidedir_kobo.so: hidedir.c
	$(CC) -o $@ $< $(CFLAGS)

ld.so.preload: libhidedir_kobo.so
	echo $(PREFIX)/$< > $@
