/* See LICENSE file for copyright and license details. */

#include <assert.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrender.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "dtext.h"

#define TEXT L"The quick brown fox jumps over the lazy dog. "
//#define FONT "/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-Regular.otf:16"
#define FONT "/usr/share/fonts/inconsolata/Inconsolata-Regular.ttf:16;" \
             "/usr/share/fonts/powerline-symbols/PowerlineSymbols.otf:14"
//#define FONT "/usr/share/fonts/libertine/LinLibertine_R.otf:16"

Display *dpy;
Window win;
GC gc;

dt_context *ctx;
dt_font *fnt;
dt_color color;
dt_color color_inv;

static void setup_x();
static void setup_dt();
static void draw();

int main()
{
	XEvent evt;

	_Xdebug = 1;

	setup_x();

	assert(!dt_init(&ctx, dpy, win));
	assert(!dt_load(ctx, &fnt, FONT));
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

	dt_free(ctx, fnt);
	dt_quit(ctx);
	XCloseDisplay(dpy);
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
	memset(&color, 0, sizeof(color));
	memset(&color_inv, 0, sizeof(color_inv));
	color_inv.red = 0xFF;
	color_inv.green = 0xFF;
	color_inv.blue = 0xFF;
}

static void draw()
{
	dt_bbox bbox;

	assert(!dt_draw(ctx, fnt, &color, 10, 50, TEXT));

	assert(!dt_box(ctx, fnt, &bbox, TEXT));
	XFillRectangle(dpy, win, gc, 10 + bbox.x, 100 + bbox.y, bbox.w, bbox.h);
	assert(!dt_draw(ctx, fnt, &color_inv, 10, 100, TEXT));

	XFillRectangle(dpy, win, gc, 10 + bbox.x, 150 - fnt->ascent, bbox.w, fnt->height);
	assert(!dt_draw(ctx, fnt, &color_inv, 10, 150, TEXT));

	XFlush(dpy);
}
