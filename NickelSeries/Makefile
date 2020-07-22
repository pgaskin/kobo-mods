include ../.lib/nh/NickelHook.mk

override LIBRARY  := libns.so
override SOURCES  += nickelseries.cc series.cc ../.lib/miniz/miniz.c
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystem../.lib/miniz -fvisibility=hidden -fvisibility-inlines-hidden

include ../.lib/nh/NickelHook.mk
