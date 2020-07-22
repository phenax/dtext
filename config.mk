# See LICENSE file for copyright and license details.

WARN_CFLAGS = -Wall -Wextra -pedantic -D_XOPEN_SOURCE=700

DBG_CFLAGS  = -g
DBG_LDFLAGS = -g

FT_CFLAGS  = `pkg-config --cflags freetype2`
FT_LDFLAGS = `pkg-config --libs freetype2`

XLIB_CFLAGS  = -I/usr/X11R6/include/
XLIB_LDFLAGS = -lX11 -lXrender

CFLAGS  += -std=c99 ${XLIB_CFLAGS} ${FT_CFLAGS} ${WARN_CFLAGS} ${DBG_CFLAGS}
LDFLAGS += ${XLIB_LDFLAGS} ${FT_LDFLAGS} ${DBG_LDFLAGS}
# CC = cc
