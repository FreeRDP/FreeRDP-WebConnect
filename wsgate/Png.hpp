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
    
            void PngWrite(png_bytep data, png_size_t len);
            void PngFlush();

            static void cbPngWrite(png_structp, png_bytep, png_size_t);
            static void cbPngFlush(png_structp);
    };
}
#endif
