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
#ifndef _WSGATE_UPDATE_H_
#define _WSGATE_UPDATE_H_

#include "wshandler.hpp"

typedef struct rdp_freerdp freerdp;
typedef struct rdp_context rdpContext;
typedef struct rdp_input rdpInput;
typedef struct rdp_settings rdpSettings;

namespace wsgate {

    /**
     * Implementation of the FreeRDP Update interface.
     * This class implments callbacks for FreeRDP's Update API.
     */ 
    class Update {

        public:
            /**
             * Constructs a new instance.
             * @param h A pointer to the corresponding wshandler object.
             */
            Update(wspp::wshandler *h);

            /// Destructor.
            virtual ~Update();

            /**
             * Registers the callbacks ath FreeRDP's API.
             * @param rdp A pointer to the FreeRDP instance.
             */
            void Register(freerdp *rdp);

        private:
            wspp::wshandler *m_wshandler;

            // Non-copyable
            Update(const Update &);
            Update & operator=(const Update &);

            void BeginPaint(rdpContext* context);
            void EndPaint(rdpContext* context);
            void SetBounds(rdpContext* context, rdpBounds* bounds);
            void Synchronize(rdpContext* context);
            void DesktopResize(rdpContext* context);
            void BitmapUpdate(rdpContext* context, BITMAP_UPDATE* bitmap);
            void Palette(rdpContext* context, PALETTE_UPDATE* palette);
            void PlaySound(rdpContext* context, PLAY_SOUND_UPDATE* play_sound);
            void RefreshRect(rdpContext* context, uint8 count, RECTANGLE_16* areas);
            void SuppressOutput(rdpContext* context, uint8 allow, RECTANGLE_16* area);
            void SurfaceCommand(rdpContext* context, STREAM* s);
            void SurfaceBits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command);
            void SurfaceFrameMarker(rdpContext* context, SURFACE_FRAME_MARKER* surface_frame_marker);

            // Callbacks from C - Must be static in order t be assigned to C fnPtrs.
            static void cbBeginPaint(rdpContext* context);
            static void cbEndPaint(rdpContext* context);
            static void cbSetBounds(rdpContext* context, rdpBounds* bounds);
            static void cbSynchronize(rdpContext* context);
            static void cbDesktopResize(rdpContext* context);
            static void cbBitmapUpdate(rdpContext* context, BITMAP_UPDATE* bitmap);
            static void cbPalette(rdpContext* context, PALETTE_UPDATE* palette);
            static void cbPlaySound(rdpContext* context, PLAY_SOUND_UPDATE* play_sound);
            static void cbRefreshRect(rdpContext* context, uint8 count, RECTANGLE_16* areas);
            static void cbSuppressOutput(rdpContext* context, uint8 allow, RECTANGLE_16* area);
            static void cbSurfaceCommand(rdpContext* context, STREAM* s);
            static void cbSurfaceBits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command);
            static void cbSurfaceFrameMarker(rdpContext* context, SURFACE_FRAME_MARKER* surface_frame_marker);

    };
}

#endif
