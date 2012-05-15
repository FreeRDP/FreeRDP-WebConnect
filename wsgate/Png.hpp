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
#ifndef _WSGATE_PNG_H_
#define _WSGATE_PNG_H_

#include <string>
#include <png.h>

#include "rdpcommon.hpp"

namespace wsgate {

    /**
     * This class implements a simple PNG
     * generator for generating in-memory
     * cursor images.
     */
    class Png {
        public:
            // Constructor
            Png();

            // Destructor
            ~Png();

            /**
             * Generates a cursor image.
             * @param width The width of the image in pixels.
             * @param height The width of the image in pixels.
             * @param data An array of bytes, defining the image data in ARGB format.
             * @return An STL string, containing the PNG-encoded image.
             */
            std::string GenerateFromARGB(int width, int height, uint8_t *data);

        private:
            png_structp png_ptr;
            png_infop info_ptr;
            std::string ret;

            // non-copyable
            Png(const Png &);
            Png & operator=(const Png &);

            void PngWrite(png_bytep data, png_size_t len);
            void PngFlush();
            void PngFailure(bool err, const char *msg);

            // C callbacks
            static void cbPngWrite(png_structp, png_bytep, png_size_t);
            static void cbPngFlush(png_structp);
            static void cbPngError(png_structp, png_const_charp);
            static void cbPngWarn(png_structp, png_const_charp);
    };
}
#endif
