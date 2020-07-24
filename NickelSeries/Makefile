include ../NickelHook/NickelHook.mk

override LIBRARY  := libns.so
override SOURCES  += nickelseries.cc metadata.cc miniz/miniz.c
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden

include ../NickelHook/NickelHook.mk
