#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sstream>
#include <iomanip>
#include <png.h>

#include "Png.hpp"
#include "btexception.hpp"

namespace wsgate {

    using namespace std;

    Png::Png()
        : png_ptr(NULL)
        , info_ptr(NULL)
        , ret()
    {
    }

    Png::~Png() {
    }

    string Png::GenerateFromARGB(int width, int height, uint8_t *data)
    {
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) {
            throw new tracing::runtime_error("Could not allocate png_struct");
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            throw new tracing::runtime_error("Could not allocate png_info_struct");
        }
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            throw new tracing::runtime_error("Could not set longjump");
        }
        png_set_write_fn(png_ptr, this, cbPngWrite, cbPngFlush);
        png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_bytep *rows = new png_bytep[height];
        for (int i = 0; i < height; ++i) {
             rows[i] = reinterpret_cast<png_bytep>(data);
             data += width * 4;
        }
        png_set_rows(png_ptr, info_ptr, rows);
        ret.clear();
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
        delete []rows;
        return ret;
    }

    // private
    void Png::PngWrite(png_bytep data, png_size_t len) {
        ret.append(reinterpret_cast<const char *>(data), len);
    }

    // private
    void Png::PngFlush() {
    }

    // private static (C callback)
    void Png::cbPngWrite(png_structp png, png_bytep data, png_size_t len) {
        Png *self = reinterpret_cast<Png *>(png_get_io_ptr(png));
        if (NULL != self) {
            self->PngWrite(data, len);
        }
    }

    // private static (C callback)
    void Png::cbPngFlush(png_structp png) {
        Png *self = reinterpret_cast<Png *>(png_get_io_ptr(png));
        if (NULL != self) {
            self->PngFlush();
        }
    }
}
