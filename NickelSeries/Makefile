include lib/nh/NickelHook.mk

override LIBRARY  := src/libns.so
override SOURCES  += src/nickelseries.cc src/series.cc lib/miniz/miniz.c
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemlib -fvisibility=hidden -fvisibility-inlines-hidden

include lib/nh/NickelHook.mk
