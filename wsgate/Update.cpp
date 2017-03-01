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

#ifdef _WIN32
#include <stdint.h>
#endif

#include "rdpcommon.hpp"
#include "Update.hpp"

namespace wsgate {

    using namespace std;

    Update::Update(wspp::wshandler *h)
        : m_wshandler(h)
    { }

    Update::~Update()
    { }

    void Update::Register(freerdp *rdp) {
        log::debug << __PRETTY_FUNCTION__ << endl;
        rdp->update->BeginPaint = reinterpret_cast<pBeginPaint>(cbBeginPaint);
        rdp->update->EndPaint = reinterpret_cast<pEndPaint>(cbEndPaint);
        rdp->update->SetBounds = reinterpret_cast<pSetBounds>(cbSetBounds);
        rdp->update->Synchronize = reinterpret_cast<pSynchronize>(cbSynchronize);
        rdp->update->DesktopResize = reinterpret_cast<pDesktopResize>(cbDesktopResize);
        rdp->update->BitmapUpdate = reinterpret_cast<pBitmapUpdate>(cbBitmapUpdate);
        rdp->update->Palette = reinterpret_cast<pPalette>(cbPalette);
        rdp->update->PlaySound = reinterpret_cast<pPlaySound>(cbPlaySound);
        rdp->update->SurfaceBits = reinterpret_cast<pSurfaceBits>(cbSurfaceBits);

        rdp->update->RefreshRect = reinterpret_cast<pRefreshRect>(cbRefreshRect);
        rdp->update->SuppressOutput = reinterpret_cast<pSuppressOutput>(cbSuppressOutput);
    }

    void Update::BeginPaint(rdpContext*) {
#ifdef DBGLOG_BEGINPAINT
        log::debug << "BP" << endl;
#endif
        uint32_t op = WSOP_SC_BEGINPAINT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
    }

    void Update::EndPaint(rdpContext*) {
#ifdef DBGLOG_ENDPAINT
        log::debug << "EP" << endl;
#endif
        uint32_t op = WSOP_SC_ENDPAINT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
    }

    void Update::SetBounds(rdpContext*, rdpBounds* bounds) {
        rdpBounds lB;
        uint32_t op = WSOP_SC_SETBOUNDS;
        if (bounds) {
            memcpy(&lB, bounds, sizeof(rdpBounds));
            lB.right++;
            lB.bottom++;
        } else {
            memset(&lB, 0, sizeof(rdpBounds));
        }
#ifdef DBGLOG_SETBOUNDS
        log::debug << "CL l: " << lB.left << " t: " << lB.top
            << " r: " << lB.right << " b: " << lB.bottom << endl;
#endif
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        buf.append(reinterpret_cast<const char *>(&lB), sizeof(lB));
        m_wshandler->send_binary(buf);
    }

    void Update::Synchronize(rdpContext*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::DesktopResize(rdpContext* m_rdpContext) {
        log::debug << __PRETTY_FUNCTION__ << endl;

		string sendMsg = "R:";
		sendMsg.append(std::to_string(m_rdpContext->settings->DesktopWidth));
		sendMsg.append("x");
		sendMsg.append(std::to_string(m_rdpContext->settings->DesktopHeight));

		m_wshandler->send_text(sendMsg);
    }

    void Update::BitmapUpdate(rdpContext*, BITMAP_UPDATE* bitmap) {
        int i;
        BITMAP_DATA* bmd;
        for (i = 0; i < (int) bitmap->number; i++) {
            bmd = &bitmap->rectangles[i];
            struct {
                uint32_t op;
                uint32_t x;
                uint32_t y;
                uint32_t w;
                uint32_t h;
                uint32_t dw;
                uint32_t dh;
                uint32_t bpp;
                uint32_t cf;
                uint32_t sz;
            } wxbm = {
                WSOP_SC_BITMAP,
                bmd->destLeft, bmd->destTop,
                bmd->width, bmd->height,
                bmd->destRight - bmd->destLeft + 1,
                bmd->destBottom - bmd->destTop + 1,
                bmd->bitsPerPixel,
                static_cast<uint32_t>(bmd->compressed), bmd->bitmapLength
            };
            if (!bmd->compressed) {
                freerdp_image_flip(bmd->bitmapDataStream, bmd->bitmapDataStream,
                        bmd->width, bmd->height, bmd->bitsPerPixel);
            }
            string buf(reinterpret_cast<const char *>(&wxbm), sizeof(wxbm));
            buf.append(reinterpret_cast<const char *>(bmd->bitmapDataStream),
                    bmd->bitmapLength);
#ifdef DBGLOG_BITMAP
            log::debug << "BM" << (wxbm.cf ? " C " : " U ") << "x="
                << wxbm.x << " y=" << wxbm.y << " w=" << wxbm.w << " h=" << wxbm.h
                << " bpp=" << wxbm.bpp << " dw=" << wxbm.dw << " dh=" << wxbm.dh << endl;
#endif
            m_wshandler->send_binary(buf);
        }
    }

    void Update::Palette(rdpContext*, PALETTE_UPDATE*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::PlaySound(rdpContext*, PLAY_SOUND_UPDATE*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::RefreshRect(rdpContext*, UINT8, RECTANGLE_16*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SuppressOutput(rdpContext*, UINT8, RECTANGLE_16*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceCommand(rdpContext*, wStream*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceBits(rdpContext*, SURFACE_BITS_COMMAND*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceFrameMarker(rdpContext*, SURFACE_FRAME_MARKER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    // C callbacks
    void Update::cbBeginPaint(rdpContext* context) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->BeginPaint(context);
        }
    }

    void Update::cbEndPaint(rdpContext* context) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->EndPaint(context);
        }
    }

    void Update::cbSetBounds(rdpContext* context, rdpBounds* bounds) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SetBounds(context, bounds);
        }
    }

    void Update::cbSynchronize(rdpContext* context) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->Synchronize(context);
        }
    }

    void Update::cbDesktopResize(rdpContext* context) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->DesktopResize(context);
        }
    }

    void Update::cbBitmapUpdate(rdpContext* context, BITMAP_UPDATE* bitmap) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->BitmapUpdate(context, bitmap);
        }
    }

    void Update::cbPalette(rdpContext* context, PALETTE_UPDATE* palette) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->Palette(context, palette);
        }
    }

    void Update::cbPlaySound(rdpContext* context, PLAY_SOUND_UPDATE* play_sound) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->PlaySound(context, play_sound);
        }
    }

    void Update::cbRefreshRect(rdpContext* context, UINT8 count, RECTANGLE_16* areas) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->RefreshRect(context, count, areas);
        }
    }

    void Update::cbSuppressOutput(rdpContext* context, UINT8 allow, RECTANGLE_16* area) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SuppressOutput(context, allow, area);
        }
    }

    void Update::cbSurfaceCommand(rdpContext* context, wStream* s) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SurfaceCommand(context, s);
        }
    }

    void Update::cbSurfaceBits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SurfaceBits(context, surface_bits_command);
        }
    }

    void Update::cbSurfaceFrameMarker(rdpContext* context, SURFACE_FRAME_MARKER* surface_frame_marker) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SurfaceFrameMarker(context, surface_frame_marker);
        }
    }

}
