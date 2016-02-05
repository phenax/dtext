/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "dtext.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

static dt_error load_face(dt_context *ctx, FT_Face *face, char const *name);
static dt_error load_char(dt_context *ctx, dt_font *fnt, wchar_t c);

static dt_pair hash_unavailable = { 0 };
static dt_pair const *hash_get(dt_row map[DT_HASH_SIZE], wchar_t key);
static dt_error hash_set(dt_row map[DT_HASH_SIZE], dt_pair val);

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

	if ((err = FT_Init_FreeType(&ctx->ft_lib))) {
		free(ctx);
		return err;
	}

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
}

void
dt_quit(dt_context *ctx)
{
	XRenderFreePicture(ctx->dpy, ctx->fill);
	XRenderFreePicture(ctx->dpy, ctx->pic);

	XFree(ctx->argb32_format);
	XFree(ctx->win_format);

	FT_Done_FreeType(ctx->ft_lib);

	free(ctx);
}

dt_error
dt_load(dt_context *ctx, dt_font **res, char const *name)
{
	dt_error err;
	dt_font *fnt;
	size_t i;
	size_t len;
	char *face;
	int16_t descent;

	if (!(fnt = malloc(sizeof(*fnt))))
		return -ENOMEM;

	fnt->num_faces = 1;
	for (i = 0; name[i]; ++i)
		fnt->num_faces += (name[i] == ';');

	if (!(fnt->faces = malloc(fnt->num_faces * sizeof(fnt->faces[0])))) {
		free(fnt);
		return -ENOMEM;
	}
	for (i = 0; i < fnt->num_faces; ++i) {
		len = strchr(name, ';') - name;
		if (!(face = strndup(name, len)))
			return -ENOMEM;
		if ((err = load_face(ctx, &fnt->faces[i], face))) {
			free(face);
			while (--i != (size_t) -1)
				FT_Done_Face(fnt->faces[i]);
			free(fnt->faces);
			free(fnt);
			return err;
		}
		free(face);
		name += len + 1;
	}

	fnt->gs = XRenderCreateGlyphSet(ctx->dpy, ctx->argb32_format);
	memset(fnt->advance, 0, sizeof(fnt->advance));

	fnt->ascent = descent = 0;
	for (i = 0; i < fnt->num_faces; ++i) {
		fnt->ascent = max(fnt->ascent, fnt->faces[i]->ascender >> 6);
		descent = min(descent, fnt->faces[i]->descender >> 6);
	}
	fnt->height = fnt->ascent - descent;

	*res = fnt;
	return 0;
}

void
dt_free(dt_context *ctx, dt_font *fnt)
{
	size_t i;

	XRenderFreeGlyphSet(ctx->dpy, fnt->gs);

	for (i = 0; i < fnt->num_faces; ++i)
		FT_Done_Face(fnt->faces[i]);

	free(fnt);
}

dt_error
dt_box(dt_context *ctx, dt_font *fnt, dt_bbox *bbox,
       wchar_t const *txt, size_t len)
{
	dt_error err;
	size_t i;
	dt_pair const *p;

	memset(bbox, 0, sizeof(*bbox));

	for (i = 0; i < len; ++i) {
		if ((err = load_char(ctx, fnt, txt[i])))
			return err;
		p = hash_get(fnt->advance, txt[i]);
		bbox->w += p->adv;
		bbox->h = max(p->h, bbox->h + max(0, bbox->y - p->asc));
		bbox->y = min(p->asc, bbox->y);
	}

	return 0;
}

dt_error
dt_draw(dt_context *ctx, dt_font *fnt, dt_color const *color,
        uint32_t x, uint32_t y, wchar_t const *txt, size_t len)
{
	dt_error err;
	XRenderColor col;
	size_t i;

	col.red   = (color->red   << 8) + color->red;
	col.green = (color->green << 8) + color->green;
	col.blue  = (color->blue  << 8) + color->blue;
	col.alpha = (color->alpha << 8) + color->alpha;
	XRenderFillRectangle(ctx->dpy, PictOpSrc, ctx->fill, &col, 0, 0, 1, 1);

	for (i = 0; i < len; ++i)
		if ((err = load_char(ctx, fnt, txt[i])))
			return err;

#define DO_COMPOSITE(Size, Type) \
	XRenderCompositeString ## Size(ctx->dpy, PictOpOver, ctx->fill, \
	                               ctx->pic, ctx->argb32_format, \
	                               fnt->gs, 0, 0, x, y, \
	                               (Type const *) txt, len)
	if (sizeof(wchar_t) == 1)
		DO_COMPOSITE(8, char);
	else if (sizeof(wchar_t) == 2)
		DO_COMPOSITE(16, uint16_t);
	else
		DO_COMPOSITE(32, uint32_t);
#undef DO_COMPOSITE

	return 0;
}

static dt_error
load_face(dt_context *ctx, FT_Face *face, char const *name) {
	dt_error err;
	char *file;
	char *colon;
	size_t size;

	if (!(colon = strchr(name, ':')))
		return -EINVAL;

	if (!(file = strndup(name, colon - name)))
		return -ENOMEM;
	err = FT_New_Face(ctx->ft_lib, file, 0, face);
	free(file);
	if (err)
		return err;

	name = colon + 1;
	size = strtoul(name, 0, 10);
	if ((err = FT_Set_Char_Size(*face, size << 6, 0, 0, 0))) {
		FT_Done_Face(*face);
		return err;
	}

	return 0;
}


static dt_error
load_char(dt_context *ctx, dt_font *fnt, wchar_t c)
{
	dt_error err;
	FT_UInt code;
	FT_GlyphSlot slot;
	Glyph gid;
	XGlyphInfo g;
	char *img;
	size_t x, y, i;

	if (hash_get(fnt->advance, c) != &hash_unavailable)
		return 0;

	slot = 0;
	for (i = 0; i < fnt->num_faces; ++i) {
		code = FT_Get_Char_Index(fnt->faces[i], c);
		if (!code)
			continue;

		if ((err = FT_Load_Glyph(fnt->faces[i], code, FT_LOAD_RENDER)))
			continue;
		slot = fnt->faces[i]->glyph;
		break;
	}
	if (!slot) {
		if ((err = FT_Load_Char(fnt->faces[0], c, FT_LOAD_RENDER)))
			return err;
		slot = fnt->faces[0]->glyph;
	}

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

	return hash_set(fnt->advance, (dt_pair) {
		.c = c,
		.adv = slot->advance.x >> 6,
		.asc = - slot->metrics.horiBearingY >> 6,
		.h = slot->metrics.height >> 6
	});
}

static dt_pair const *
hash_get(dt_row map[DT_HASH_SIZE], wchar_t key)
{
	dt_row row;
	size_t i;

	row = map[key % DT_HASH_SIZE];
	for (i = 0; i < row.len; ++i)
		if (row.data[i].c == key)
			return &row.data[i];

	return &hash_unavailable;
}

static dt_error
hash_set(dt_row map[DT_HASH_SIZE], dt_pair val)
{
	dt_row row;
	dt_pair *d;
	size_t i;

	row = map[val.c % DT_HASH_SIZE];

	for (i = 0; i < row.len; ++i) {
		if (row.data[i].c == val.c) {
			row.data[i] = val;
			return 0;
		}
	}

	if (row.allocated == row.len) {
		d = row.data;
		if (!(d = realloc(d, (2 * row.len + 1) * sizeof(d[0]))))
			return -ENOMEM;
		row.data = d;
		row.allocated = 2 * row.len + 1;
	}
	++row.len;

	row.data[row.len - 1] = val;

	map[val.c % DT_HASH_SIZE] = row;
	return 0;
}
