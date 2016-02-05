# See LICENSE file for copyright and license details.

include config.mk

EXECS := $(patsubst examples/%.c,build/%,$(wildcard examples/*.c))

all: $(EXECS)

build/%: examples/%.c dtext.c dtext.h config.mk Makefile
	[ -d build ] || mkdir build
	${CC} -I. ${CFLAGS} ${LDFLAGS} $< dtext.c -o $@

clean:
	rm -f $(EXECS)

.PHONY: all clean
