// Copyright (c) Paul Hodge. See LICENSE file for license terms.

#include <cairo/cairo.h>

#include "types/color.h"

using namespace circa;

namespace cairo_support {

Type *g_cairoContext_t;
Type *g_cairoSurface_t;
Type *g_cairoFontFace_t;

cairo_t* as_cairo_context(TaggedValue* value)
{
    return (cairo_t*) get_pointer(value, g_cairoContext_t);
}

cairo_surface_t* as_cairo_surface(TaggedValue* value)
{
    return (cairo_surface_t*) get_pointer(value, g_cairoSurface_t);
}

cairo_font_face_t* as_cairo_font_face(TaggedValue* value)
{
    return (cairo_font_face_t*) get_pointer(value, g_cairoFontFace_t);
}

void cairoContext_copy(Type*, TaggedValue* source, TaggedValue* dest)
{
    cairo_t* context = as_cairo_context(source);
    cairo_reference(context);
    set_pointer(dest, g_cairoContext_t, context);
}
void cairoContext_release(Type*, TaggedValue* value)
{
    cairo_t* context = as_cairo_context(value);
    cairo_destroy(context);
}
void cairoSurface_copy(Type*, TaggedValue* source, TaggedValue* dest)
{
    cairo_surface_t* surface = as_cairo_surface(source);
    cairo_surface_reference(surface);
    set_pointer(dest, g_cairoSurface_t, surface);
}
void cairoSurface_release(Type*, TaggedValue* value)
{
    cairo_surface_t* surface = as_cairo_surface(value);
    cairo_surface_destroy(surface);
}
void cairoFontFace_copy(Type*, TaggedValue* source, TaggedValue* dest)
{
    cairo_font_face_t* font = as_cairo_font_face(source);
    cairo_font_face_reference(font);
    set_pointer(dest, g_cairoFontFace_t, font);
}
void cairoFontFace_release(Type*, TaggedValue* value)
{
    cairo_font_face_t* font = as_cairo_font_face(value);
    cairo_font_face_destroy(font);
}

float radians_to_degrees(float radians) { return radians * 180.0 / M_PI; }
float degrees_to_radians(float unit) { return unit * M_PI / 180.0; }

CA_FUNCTION(save)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_save(context);
}
CA_FUNCTION(restore)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_restore(context);
}
CA_FUNCTION(create_context_for_surface)
{
    cairo_surface_t* surface = as_cairo_surface(INPUT(0));
    set_pointer(OUTPUT, g_cairoContext_t, cairo_create(surface));
}

CA_FUNCTION(create_image_surface)
{
    // user can't currently specify the format
    TaggedValue* size = INPUT(0);
    int width = as_int(list_get_index(size, 0));
    int height = as_int(list_get_index(size, 1));

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    set_pointer(OUTPUT, g_cairoSurface_t, surface);
}

CA_FUNCTION(select_font_face)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    const char* family = STRING_INPUT(1);
    // currently, user can't specify font slant or weight.
    cairo_select_font_face(context, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
}
CA_FUNCTION(set_font_size)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_set_font_size(context, FLOAT_INPUT(1));
}
CA_FUNCTION(set_font_face)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_font_face_t* font = as_cairo_font_face(INPUT(1));
    cairo_set_font_face(context, font);
}
CA_FUNCTION(show_text)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    const char* str = STRING_INPUT(1);
    cairo_show_text(context, str);
}
CA_FUNCTION(text_extents)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_text_extents_t extents;
    cairo_text_extents(context, STRING_INPUT(1), &extents);

    List* out = List::cast(OUTPUT, 3);
    set_point(out->get(0), extents.x_bearing, extents.y_bearing);
    set_point(out->get(1), extents.width, extents.height);
    set_point(out->get(2), extents.x_advance, extents.y_advance);

    out->value_type = declared_type(CALLER);
}

CA_FUNCTION(stroke)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_stroke(context);
    set_null(OUTPUT);
}
CA_FUNCTION(paint)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_paint(context);
    set_null(OUTPUT);
}
CA_FUNCTION(set_source_color)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    float r(0), g(0), b(0), a(0);
    get_color(INPUT(1), &r, &g, &b, &a);
    cairo_set_source_rgba(context, r, g, b, a);
    set_null(OUTPUT);
}
CA_FUNCTION(fill_preserve)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_fill_preserve(context);
}
CA_FUNCTION(set_operator)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_set_operator(context, (cairo_operator_t) INT_INPUT(1));
}
CA_FUNCTION(move_to)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    float x(0), y(0);
    get_point(INPUT(1), &x, &y);
    cairo_move_to(context, x, y);
}
CA_FUNCTION(curve_to)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    float x1(0), y1(0), x2(0), y2(0), x3(0), y3(0);
    get_point(INPUT(1), &x1, &x2);
    get_point(INPUT(2), &x1, &x2);
    get_point(INPUT(3), &x1, &x2);
    cairo_curve_to(context, x1, y1, x2, y2, x3, y3);
    set_null(OUTPUT);
}
CA_FUNCTION(line_to)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    float x(0), y(0);
    get_point(INPUT(1), &x, &y);
    cairo_line_to(context, x, y);
    set_null(OUTPUT);
}
CA_FUNCTION(arc)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    float center_x(0), center_y(0);
    get_point(INPUT(1), &center_x, &center_y);
    cairo_arc(context, center_x, center_y,
        FLOAT_INPUT(2),
        degrees_to_radians(FLOAT_INPUT(3)),
        degrees_to_radians(FLOAT_INPUT(4)));
}
CA_FUNCTION(new_sub_path)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_new_sub_path(context);
}
CA_FUNCTION(close_path)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_close_path(context);
}
CA_FUNCTION(set_line_width)
{
    cairo_t* context = as_cairo_context(INPUT(0));
    cairo_set_line_width(context, to_float(INPUT(1)));
    set_null(OUTPUT);
}
CA_FUNCTION(upload_surface_to_opengl)
{
    cairo_surface_t* surface = as_cairo_surface(INPUT(0));
    int texture_id = as_int(INPUT(1));

    unsigned char* pixels = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    const int channels = 4;

    // Modify 'surface' so that colors are in RGBA order instead of BGRA
    for (int y=0; y < height; y++) for (int x=0; x < width; x++) {
        size_t offset = y * stride + x * channels;
        //unsigned char alpha = pixels[offset + 3];
        unsigned char b = pixels[offset + 0];
        unsigned char g = pixels[offset + 1];
        unsigned char r = pixels[offset + 2];
        unsigned char a = pixels[offset + 3];

        pixels[offset+0] = r;
        pixels[offset+1] = g;
        pixels[offset+2] = b;
        pixels[offset+3] = a;
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);

    set_null(OUTPUT);
}

void setup(Branch& kernel)
{
    Branch& ns = nested_contents(kernel["cairo"]);

    g_cairoContext_t = as_type(ns["Context"]);
    g_cairoSurface_t = as_type(ns["Surface"]);
    g_cairoFontFace_t = as_type(ns["FontFace"]);

    g_cairoContext_t->copy = cairoContext_copy;
    g_cairoContext_t->release = cairoContext_release;
    g_cairoSurface_t->copy = cairoSurface_copy;
    g_cairoSurface_t->release = cairoSurface_release;
    g_cairoFontFace_t->copy = cairoFontFace_copy;
    g_cairoFontFace_t->release = cairoFontFace_release;

    install_function(ns["create_context_for_surface"], create_context_for_surface);
    install_function(ns["Context.save"], save);
    install_function(ns["Context.restore"], restore);
    install_function(ns["Context.stroke"], stroke);
    install_function(ns["Context.paint"], paint);
    install_function(ns["Context.set_source_color"], set_source_color);
    install_function(ns["Context.fill_preserve"], fill_preserve);
    install_function(ns["Context.set_operator"], set_operator);
    install_function(ns["create_image_surface"], create_image_surface);
    install_function(ns["Context.select_font_face"], select_font_face);
    install_function(ns["Context.set_font_size"], set_font_size);
    install_function(ns["Context.set_font_face"], set_font_face);
    install_function(ns["Context.show_text"], show_text);
    install_function(ns["Context.text_extents"], text_extents);
    install_function(ns["Context.move_to"], move_to);
    install_function(ns["Context.curve_to"], curve_to);
    install_function(ns["Context.line_to"], line_to);
    install_function(ns["Context.arc"], arc);
    install_function(ns["Context.new_sub_path"], new_sub_path);
    install_function(ns["Context.close_path"], close_path);
    install_function(ns["Context.set_line_width"], set_line_width);
    install_function(ns["upload_surface_to_opengl"], upload_surface_to_opengl);
}

} // namespace cairo_support