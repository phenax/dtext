/* See LICENSE file for copyright and license details. */

#include <assert.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrender.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "dtext.h"

#define TEXT "The quick brown fox jumps over the lazy dog."
//#define FONT "/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-Regular.otf"
#define FONT "/usr/share/fonts/inconsolata/Inconsolata-Regular.ttf"
//#define FONT "/usr/share/fonts/libertine/LinLibertine_R.otf"

Display *dpy;
Window win;
GC gc;

dt_context *ctx;
dt_font *fnt;
dt_style style;
dt_style style_inv;

static void setup_x();
static void setup_dt();
static void draw();

int main()
{
	XEvent evt;

	_Xdebug = 1;

	setup_x();
	setup_dt();

	draw();

	XSelectInput(dpy, win, ExposureMask | KeyPressMask);
	while (1) {
		XNextEvent(dpy, &evt);
		switch (evt.type) {
		case Expose:
			draw();
			break;
		case KeyPress:
			if (XLookupKeysym(&evt.xkey, 0) == XK_Escape)
				return 0;
			break;
		}
	}
}

static void setup_x()
{
	unsigned long white, black;

	dpy = XOpenDisplay(NULL);

	white = XWhitePixel(dpy, DefaultScreen(dpy));
	black = XBlackPixel(dpy, DefaultScreen(dpy));

	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
	                          700, 500, 0, white, white);
	XMapWindow(dpy, win);

	gc = XDefaultGC(dpy, 0);
	XSetForeground(dpy, gc, black);
}

static void setup_dt()
{
	assert(!dt_init(&ctx, dpy, win));

	assert(!dt_load(ctx, &fnt, 16, FONT));

	memset(&style, 0, sizeof(style));
	memset(&style_inv, 0, sizeof(style_inv));
	style_inv.red = 0xFF;
	style_inv.green = 0xFF;
	style_inv.blue = 0xFF;
}

static void draw()
{
	assert(!dt_draw(ctx, fnt, &style, 10, 50, TEXT));

	XFillRectangle(dpy, win, gc, 5, 60, 400, 100);
	assert(!dt_draw(ctx, fnt, &style_inv, 10, 90, TEXT));

	XFlush(dpy);
}
