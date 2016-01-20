/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "dtext.h"

static dt_error load_char(dt_context *ctx, dt_font *fnt, char c);

dt_error
dt_init(dt_context **res, Display *dpy, Window win)
{
	dt_error err;
	dt_context *ctx;
	Visual *visual;
	Pixmap pix;
	XRenderPictureAttributes attrs;

	if (!(ctx = malloc(sizeof(*ctx))))
		return -ENOMEM;

	if ((err = FT_Init_FreeType(&ctx->ft_lib)))
		goto fail_init_ft;

	visual = XDefaultVisual(dpy, XDefaultScreen(dpy));
	ctx->win_format = XRenderFindVisualFormat(dpy, visual);
	XFree(visual);
	ctx->argb32_format = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	ctx->dpy = dpy;
	ctx->pic = XRenderCreatePicture(dpy, win, ctx->win_format, 0, &attrs);

	pix = XCreatePixmap(dpy, DefaultRootWindow(dpy), 1, 1, 32);
	attrs.repeat = 1;
	ctx->fill = XRenderCreatePicture(ctx->dpy, pix, ctx->argb32_format,
	                                 CPRepeat, &attrs);
	XFreePixmap(dpy, pix);

	*res = ctx;
	return 0;

fail_init_ft:
	free(ctx);
	return err;
}

dt_error
dt_quit(dt_context *ctx)
{
	dt_error err = 0;

	XRenderFreePicture(ctx->dpy, ctx->fill);
	XRenderFreePicture(ctx->dpy, ctx->pic);

	XFree(ctx->argb32_format);
	XFree(ctx->win_format);

	err = FT_Done_FreeType(ctx->ft_lib);

	free(ctx);

	return err;
}

dt_error
dt_load(dt_context *ctx, dt_font **res, uint8_t size, char const *name)
{
	dt_error err;
	dt_font *fnt;

	if (!(fnt = malloc(sizeof(*fnt))))
		return -ENOMEM;

	if ((err = FT_New_Face(ctx->ft_lib, name, 0, &fnt->face)))
		goto fail_new_face;
	if ((err = FT_Set_Char_Size(fnt->face, size << 6, 0, 0, 0)))
		goto fail_char_size;

	fnt->gs = XRenderCreateGlyphSet(ctx->dpy, ctx->argb32_format);
	memset(fnt->loaded, 0, 256);

	*res = fnt;
	return 0;

fail_char_size:
	FT_Done_Face(fnt->face); // if this fails... just ignore
fail_new_face:
	free(fnt);
	return err;
}

dt_error
dt_free(dt_context *ctx, dt_font *fnt)
{
	dt_error err = 0;

	XRenderFreeGlyphSet(ctx->dpy, fnt->gs);

	err = FT_Done_Face(fnt->face);

	free(fnt);

	return err;
}

dt_error
dt_box(dt_font *fnt, dt_bbox *bbox, char const *txt)
{
	dt_error err;
	size_t len;
	size_t i;
	FT_Fixed adv;

	if (!(len = strlen(txt)))
		return -EINVAL;

	memset(bbox, 0, sizeof(*bbox));

	bbox->x = 0;
	bbox->y = - (fnt->face->ascender >> 6);

	for (i = 0; i < len; ++i) {
		if ((err = FT_Get_Advance(fnt->face, txt[i], 0, &adv)))
			return err;
		bbox->w += adv >> 16;
	}

	bbox->h = fnt->face->height >> 6;

	return 0;
}

dt_error
dt_draw(dt_context *ctx, dt_font *fnt, dt_style const *style,
        uint32_t x, uint32_t y, char const *txt)
{
	dt_error err;
	XRenderColor col;
	size_t len;
	size_t i;

	col.red   = (style->red   << 8) + style->red;
	col.green = (style->green << 8) + style->green;
	col.blue  = (style->blue  << 8) + style->blue;
	col.alpha = 0xFFFF - ((style->alpha << 8) + style->alpha);
	XRenderFillRectangle(ctx->dpy, PictOpSrc, ctx->fill, &col, 0, 0, 1, 1);

	len = strlen(txt);

	for (i = 0; i < len; ++i)
		if ((err = load_char(ctx, fnt, txt[i])))
			return err;

	XRenderCompositeString8(ctx->dpy, PictOpOver, ctx->fill, ctx->pic,
	                        ctx->argb32_format, fnt->gs, 0, 0, x, y,
	                        txt, len);
	return 0;
}

static dt_error
load_char(dt_context *ctx, dt_font *fnt, char c)
{
	dt_error err;
	FT_GlyphSlot slot;
	Glyph gid;
	XGlyphInfo g;
	char *img;
	size_t x, y, i;

	if (fnt->loaded[(uint8_t) c])
		return 0;

	if ((err = FT_Load_Char(fnt->face, c, FT_LOAD_RENDER)))
		return err;
	slot = fnt->face->glyph;

	gid = c;

	g.width  = slot->bitmap.width;
	g.height = slot->bitmap.rows;
	g.x = - slot->bitmap_left;
	g.y =   slot->bitmap_top;
	g.xOff = slot->advance.x >> 6;
	g.yOff = slot->advance.y >> 6;

	if (!(img = malloc(4 * g.width * g.height)))
		return -ENOMEM;
	for (y = 0; y < g.height; ++y)
		for (x = 0; x < g.width; ++x)
			for (i = 0; i < 4; ++i)
				img[4 * (y * g.width + x) + i] =
					slot->bitmap.buffer[y * g.width + x];

	XRenderAddGlyphs(ctx->dpy, fnt->gs, &gid, &g, 1,
	                 img, 4 * g.width * g.height);

	free(img);

	fnt->loaded[(uint8_t) c] = 1;
	return 0;
}
