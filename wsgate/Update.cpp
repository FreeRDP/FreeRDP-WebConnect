#ifdef HAVE_CONFIG_H
# include "config.h"
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
        rdp->update->BeginPaint = cbBeginPaint;
        rdp->update->EndPaint = cbEndPaint;
        rdp->update->SetBounds = cbSetBounds;
        // ignored rdp->update->Synchronize = cbSynchronize;
        rdp->update->DesktopResize = cbDesktopResize;
        rdp->update->BitmapUpdate = cbBitmapUpdate;
        rdp->update->Palette = cbPalette;
        rdp->update->PlaySound = cbPlaySound;
        rdp->update->SurfaceBits = cbSurfaceBits;
    }

    void Update::BeginPaint(rdpContext* context) {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        uint32_t op = WSOP_BEGINPAINT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
    }

    void Update::EndPaint(rdpContext* context) {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        uint32_t op = WSOP_ENDPAINT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
    }

    void Update::SetBounds(rdpContext* context, rdpBounds* bounds) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::Synchronize(rdpContext* context) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::DesktopResize(rdpContext* context) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::BitmapUpdate(rdpContext* context, BITMAP_UPDATE* bitmap) {
        int i;
        BITMAP_DATA* bitmap_data;
        size_t sz = sizeof(BITMAP_DATA) - sizeof(uint8*) - sizeof(boolean);
        for (i = 0; i < (int) bitmap->number; i++) {
            bitmap_data = &bitmap->rectangles[i];
            uint32_t op = WSOP_BITMAP;
            bool comp = (bitmap_data->compressed != 0);
            string buf(reinterpret_cast<const char *>(&op), sizeof(op));
            buf.append(reinterpret_cast<const char *>(bitmap_data), sz);
            buf.append(reinterpret_cast<const char *>(bitmap_data->bitmapDataStream),
                    bitmap_data->bitmapLength);
            buf.append(reinterpret_cast<const char *>(&comp), sizeof(comp));
            m_wshandler->send_binary(buf);
        }
    }

    void Update::Palette(rdpContext* context, PALETTE_UPDATE* palette) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::PlaySound(rdpContext* context, PLAY_SOUND_UPDATE* play_sound) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::RefreshRect(rdpContext* context, uint8 count, RECTANGLE_16* areas) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SuppressOutput(rdpContext* context, uint8 allow, RECTANGLE_16* area) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceCommand(rdpContext* context, STREAM* s) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceBits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Update::SurfaceFrameMarker(rdpContext* context, SURFACE_FRAME_MARKER* surface_frame_marker) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

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

    void Update::cbRefreshRect(rdpContext* context, uint8 count, RECTANGLE_16* areas) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->RefreshRect(context, count, areas);
        }
    }

    void Update::cbSuppressOutput(rdpContext* context, uint8 allow, RECTANGLE_16* area) {
        Update *self = reinterpret_cast<wsgContext *>(context)->pUpdate;
        if (self) {
            self->SuppressOutput(context, allow, area);
        }
    }

    void Update::cbSurfaceCommand(rdpContext* context, STREAM* s) {
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
