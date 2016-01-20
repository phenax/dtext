/* See LICENSE file for copyright and license details. */

typedef int32_t dt_error;

typedef struct {
	FT_Library ft_lib;

	XRenderPictFormat *win_format;
	XRenderPictFormat *argb32_format;

	Display *dpy;
	Picture pic;
	Picture fill;
} dt_context;

typedef struct {
	FT_Face face;

	GlyphSet gs;
	uint8_t loaded[256];
} dt_font;

typedef struct {
	int32_t x; // Bottom-left of box, relative to point of origin of text
	int32_t y;

	uint32_t w;
	uint32_t h;
} dt_bbox;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha; // 0 means opaque
} dt_style;

dt_error dt_init(dt_context **ctx, Display *dpy, Window win);
dt_error dt_quit(dt_context *ctx);

dt_error dt_load(dt_context *ctx, dt_font **fnt,
                 uint8_t size, char const *name);
dt_error dt_free(dt_context *ctx, dt_font *fnt);

dt_error dt_box(dt_font *fnt, dt_bbox *bbox, char const *txt);
dt_error dt_draw(dt_context *ctx, dt_font *fnt, dt_style const *style,
                 uint32_t x, uint32_t y, char const *txt);
