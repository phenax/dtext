/* See LICENSE file for copyright and license details. */

#include <assert.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xrender.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "dtext.h"

// Configuration

char const *font =
	"/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-Regular.otf:16;"
	"/usr/share/fonts/powerline-symbols/PowerlineSymbols.otf:14;"
	"/usr/share/fonts/dejavu/DejaVuSansMono.ttf:16";
char const *fontb =
	"/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-Bold.otf:16;"
	"/usr/share/fonts/powerline-symbols/PowerlineSymbols.otf:14;"
	"/usr/share/fonts/dejavu/DejaVuSansMono-Bold.ttf:16";
char const *fonti =
	"/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-RegItalic.otf:16;"
	"/usr/share/fonts/powerline-symbols/PowerlineSymbols.otf:14;"
	"/usr/share/fonts/dejavu/DejaVuSansMono-Oblique.ttf:16";
char const *fontbi =
	"/usr/share/fonts/fantasque-sans-mono/FantasqueSansMono-BoldItalic.otf:16;"
	"/usr/share/fonts/powerline-symbols/PowerlineSymbols.otf:14;"
	"/usr/share/fonts/dejavu/DejaVuSansMono-BoldOblique.ttf:16";

// Fantasque is good for testing: its reported height is far too big, so it
// allows testing the boringness of tuning the scales
float chscale = 1.f; // horizontal scale
float cvscale = .51f; // vertical scale
float cascale = .482f; // ascender scale

// \f1: toggle bold
// \f2: toggle italics
// \f3??????: change fgcolor to ?????? (RGB format)
// \f4??????: change bgcolor to ?????? (RGB format)
wchar_t const *text =
	L"Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
	L"Donec a diam lectus. \f1Sed sit amet ipsum mauris.\f1\n"
	L"\f2Maecenas\f2 congue ligula ac \f1\f2quam viverra nec\f1\f2\n"
	L"consectetur ante hendrerit. \f300FF00Donec et mollis dolor.\f3000000\n"
	L"Praesent \f4FF0000et \f1diam \f2eget\f1 libero\f40000FF egestas\f2\f4FFFFFF\n"
	L"mattis sit amet vitae augue.\n"
	L"\n"
	L"\f4000000\f3FF0000 \xe0a0 \f30000FF\x2699 \f3FFFFFFsome text \f3000000\f4FFFFFF This is a pseudo-prompt.\n";

// Implementation

Display *dpy;
Window win;
GC gc;
Colormap cmap;

dt_context *ctx;
dt_font *fnt, *fntb, *fnti, *fntbi;
dt_color color;

static void setup_x();
static void setup_dt();
static void draw(wchar_t const *txt);

int main()
{
	XEvent evt;

	_Xdebug = 1;

	setup_x();
	setup_dt();

	draw(text);

	XSelectInput(dpy, win, ExposureMask | KeyPressMask);
	while (1) {
		XNextEvent(dpy, &evt);
		switch (evt.type) {
		case Expose:
			draw(text);
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

	cmap = DefaultColormap(dpy, 0);
}

static void setup_dt()
{
	assert(!dt_init(&ctx, dpy, win));

	assert(!dt_load(ctx, &fnt, font));
	assert(!dt_load(ctx, &fntb, fontb));
	assert(!dt_load(ctx, &fnti, fonti));
	assert(!dt_load(ctx, &fntbi, fontbi));

	memset(&color, 0, sizeof(color));
	color.alpha = 0xFF;
}

static void draw(wchar_t const *txt)
{
	dt_bbox bbox;
	size_t x, y;
	int bold, italics;
	dt_font *curfnt;
	dt_color fgcol;
	XColor bgcol;
	char color[8];
	size_t i;

	x = 0;
	y = fnt->ascent * cascale;

	bold = italics = 0;
	curfnt = fnt;

	memset(&fgcol, 0, sizeof(fgcol));
	fgcol.alpha = 0xFF;

	XParseColor(dpy, cmap, "#FFFFFF", &bgcol);
	XAllocColor(dpy, cmap, &bgcol);
	XSetForeground(dpy, gc, bgcol.pixel);

	memset(color, 0, sizeof(color));
	color[0] = '#';

	while (*txt) {
		if (*txt != L'\f' && *txt != L'\n') {
			dt_box(ctx, curfnt, &bbox, txt, 1);
			XFillRectangle(dpy, win, gc, x + bbox.x, y - curfnt->ascent * cascale, bbox.w, curfnt->height * cvscale);
			dt_draw(ctx, curfnt, &fgcol, x, y, txt, 1);
			x += bbox.w * chscale;
		} else if (*txt == L'\n') {
			x = 0;
			y += curfnt->height * cvscale;
		} else if (*txt == L'\f') {
			++txt;
			switch (*txt) {
			case L'1':
				bold = !bold;
				break;
			case L'2':
				italics = !italics;
				break;
			case L'3':
			case L'4':
				for (i = 0; i < 6; ++i)
					color[i + 1] = (char) txt[i + 1];
				XParseColor(dpy, cmap, color, &bgcol);
				if (*txt == L'3') {
					fgcol.red = bgcol.red >> 8;
					fgcol.green = bgcol.green >> 8;
					fgcol.blue = bgcol.blue >> 8;
				} else {
					XAllocColor(dpy, cmap, &bgcol);
					XSetForeground(dpy, gc, bgcol.pixel);
				}
				txt += 6;
				break;
			}
			if (bold && italics)
				curfnt = fntbi;
			else if (bold)
				curfnt = fntb;
			else if (italics)
				curfnt = fnti;
			else
				curfnt = fnt;
		}
		++txt;
	}

	XFlush(dpy);
}
