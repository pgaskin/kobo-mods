.PHONY: kobo clean run

CC = $(CROSS_COMPILE)gcc

override CFLAGS := -Wall -shared -ldl -pedantic -std=c99 -fPIC $(CFLAGS)
KOBO_CFLAGS     ?= -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb

CONF_NATIVE = -DLS_ONLY
CONF_KOBO   = -DUSE_FULL_PATH -DNICKEL_ONLY

run: libhidedir.so
	@echo "=== without ==="
	ls -lah $(HOME)

	@echo "=== with ==="
	LD_PRELOAD="$(PWD)/libhidedir.so" ls -lah $(HOME)

kobo: KoboRoot.tgz

clean:
	rm -f libhidedir.so KoboRoot.tgz
	rm -rf KoboRoot

libhidedir.so: hidedir.c
	$(CC) -o $@ $^ $(CONF_NATIVE) $(CFLAGS)

libhidedir_kobo.so: hidedir.c
	$(CC) -o $@ $^ $(CONF_KOBO) $(CFLAGS) $(KOBO_CFLAGS)

KoboRoot: libhidedir_kobo.so
	mkdir -p $@/etc $@/usr/local/geek1011
	echo /usr/local/geek1011/$^ > $@/etc/ld.so.preload
	cp $^ $@/usr/local/geek1011/$^

KoboRoot.tgz: KoboRoot
	tar cvzf KoboRoot.tgz -C KoboRoot . --group=root:0 --owner=root:0
	@echo "Warning: To uninstall this, you need SSH or telnet access (unless you want to get fancy with one-time deletion scripts). This will persist past updates."
