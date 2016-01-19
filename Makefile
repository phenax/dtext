# See LICENSE file for copyright and license details.

include config.mk

SRC = dtext.c test.c
HDR = dtext.h

all: example

test: example
	./example

example: config.mk Makefile ${SRC} ${HDR}
	${CC} ${CFLAGS} ${LDFLAGS} ${SRC} -o example

clean:
	rm -f example

.PHONY: all test clean
