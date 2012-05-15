/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
        png_destroy_write_struct(&png_ptr, &info_ptr);
    }

    string Png::GenerateFromARGB(int width, int height, uint8_t *data)
    {
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, this, cbPngError, cbPngWarn);
        if (!png_ptr) {
            throw tracing::runtime_error("Could not allocate png_struct");
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            throw tracing::runtime_error("Could not allocate png_info_struct");
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

    // private
    void Png::PngFailure(bool err, const char *msg) {
        if (err) {
            log::err << "Png: " << (msg ? msg : "unknown error") << endl;
            throw tracing::runtime_error(msg);
        } else {
            log::warn << "Png: " << (msg ? msg : "unknown warning") << endl;
        }
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

    // private static (C callback)
    void Png::cbPngError(png_structp png, png_const_charp msg) {
        Png *self = reinterpret_cast<Png *>(png_get_error_ptr(png));
        if (NULL != self) {
            self->PngFailure(true, msg);
        }
    }

    // private static (C callback)
    void Png::cbPngWarn(png_structp png, png_const_charp msg) {
        Png *self = reinterpret_cast<Png *>(png_get_error_ptr(png));
        if (NULL != self) {
            self->PngFailure(false, msg);
        }
    }

}
