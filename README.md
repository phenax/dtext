dtext
=====

`dtext` is a font-rendering library that aims at simplicity, both of the
internals and of use.

Installation
------------

In order to use `dtext` in one of your projects, the recommended way is to just
drop `dtext.h` and `dtext.c` in its directory and add them to the `Makefile`.
For legal matters, despite this not being legal advice, you most likely should
in this case add anyone noted as owning a copyright for this code in the LICENSE
file to the place you keep your own list.

You can also build `dtext` as a shared library, and then link against it. This
should relieve you of the obligation of adding names to your copyright list, so
long as you do not distribute both files bundled. Again, this is not legal
advice.

Usage
-----

Most functions return a `dt_error` value. In this case, zero means success and
any non-zero value indicates failure.

    dt_error dt_init(dt_context **ctx, Display *dpy, Window win);
    void dt_quit(dt_context *ctx);

First, you need to initialize the library using `dt_init`. At the end of the
program, you should close it with `dt_quit`.

    dt_error dt_load(dt_context *ctx, dt_font **fnt, char const *name)
    void dt_free(dt_context **ctx, dt_font *fnt);

Then, you can load fonts using `dt_load`, and free them with `dt_free`. The
format of the `name` argument is described in the "Font names" section below.

    dt_error dt_box(dt_context *ctx, dt_font *fnt, dt_bbox *bbox,
                    wchar_t const *txt, size_t len);
    dt_error dt_draw(dt_context *ctx, dt_font *fnt, dt_color const *color,
                     uint32_t x, uint32_t y, wchar_t const *txt, size_t len);

These are the two main functions of the library.

`dt_box` returns a bounding box to the string composed of the first `len`
characters of `txt`, drawn with font `fnt`. The return is done through the
pointer `bbox`, which will contain the bounding box. `bbox->x` and `bbox->y`
will contain the coordinates of the top-left corner of the box relative to the
origin of the baseline, and `bbox.w` and `bbox.h` are its width and height.

`dt_draw` draws the string composed of the first `len` characters of `txt`,
drawn with font `fnt`, in color `color`, with the baseline starting at position
`x`, `y`.

`dt_color` represents a color. It is of the form
`{ .red, .green, .blue, .alpha }`, with `alpha = 0xFF` for full-visibility. For
example, if it is memset with `0xFF`, it will be the color "white".

The baseline is the line on which text would be drawn, if it was drawn by hand.
`dt_font.ascent` is the height of the highest character of the font, relative to
the baseline. `dt_font.height` is the total height of the highest character in
the font.

Font names
----------

A font name is composed of several font descriptions, separated by `;`. Each
font description is a file name and a pixel size, separated by `:`.

For example, the following is a valid font string:

    /fonts/main.ttf:16;/fonts/special_chars.ttf:18;/fonts/fallback.ttf:16

You have to specify one font size per font file, given every font file is not
built the same way.

Examples
--------

You can find examples in the `examples` directory. They have been built using
certain fonts you may not have ; so you may have to edit the font strings
located near the top of those files.

In order to test them, just run `make` and run the executables in `build/`.

Distribution
------------

This code is distributed at http://git.ekleog.org/leo/dtext ; and every commit
should be signed with OpenPGP key

    AA29 BF0D F468 A8DC 1AB0  EA84 6598 F235 F23F B2AE

If this is not the case, it means either I forgot to sign a commit, or you are
getting MitM-ed. In any case, please do not use code from an unsigned version
without properly checking it.

Contributing
------------

Please send any comments, insults or preferably patches to dtext@leo.gaspard.io .
