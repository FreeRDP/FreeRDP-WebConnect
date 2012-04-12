#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "rdpcommon.hpp"
#include "Primary.hpp"

namespace wsgate {

    using namespace std;

    Primary::Primary(wspp::wshandler *h)
        : m_wshandler(h)
    { }

    Primary::~Primary()
    { }

    void Primary::Register(freerdp *rdp) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::DstBlt(rdpContext* context, DSTBLT_ORDER* dstblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PatBlt(rdpContext* context, PATBLT_ORDER* patblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::ScrBlt(rdpContext* context, SCRBLT_ORDER* scrblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::OpaqueRect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::DrawNineGrid(rdpContext* context, DRAW_NINE_GRID_ORDER* draw_nine_grid) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiDstBlt(rdpContext* context, MULTI_DSTBLT_ORDER* multi_dstblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiPatBlt(rdpContext* context, MULTI_PATBLT_ORDER* multi_patblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiScrBlt(rdpContext* context, MULTI_SCRBLT_ORDER* multi_scrblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiOpaqueRect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiDrawNineGrid(rdpContext* context, MULTI_DRAW_NINE_GRID_ORDER* multi_draw_nine_grid) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::LineTo(rdpContext* context, LINE_TO_ORDER* line_to) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::Polyline(rdpContext* context, POLYLINE_ORDER* polyline) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MemBlt(rdpContext* context, MEMBLT_ORDER* memblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::Mem3Blt(rdpContext* context, MEM3BLT_ORDER* memblt) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::SaveBitmap(rdpContext* context, SAVE_BITMAP_ORDER* save_bitmap) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::GlyphIndex(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::FastIndex(rdpContext* context, FAST_INDEX_ORDER* fast_index) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::FastGlyph(rdpContext* context, FAST_GLYPH_ORDER* fast_glyph) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PolygonSC(rdpContext* context, POLYGON_SC_ORDER* polygon_sc) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PolygonCB(rdpContext* context, POLYGON_CB_ORDER* polygon_cb) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::EllipseSC(rdpContext* context, ELLIPSE_SC_ORDER* ellipse_sc) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::EllipseCB(rdpContext* context, ELLIPSE_CB_ORDER* ellipse_cb) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    // static callbacks
    void Primary::cbPatBlt(rdpContext* context, PATBLT_ORDER* patblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->PatBlt(context, patblt);
        }
    }

    void Primary::cbScrBlt(rdpContext* context, SCRBLT_ORDER* scrblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->ScrBlt(context, scrblt);
        }
    }

    void Primary::cbOpaqueRect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->OpaqueRect(context, opaque_rect);
        }
    }

    void Primary::cbDrawNineGrid(rdpContext* context, DRAW_NINE_GRID_ORDER* draw_nine_grid) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->DrawNineGrid(context, draw_nine_grid);
        }
    }

    void Primary::cbMultiDstBlt(rdpContext* context, MULTI_DSTBLT_ORDER* multi_dstblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MultiDstBlt(context, multi_dstblt);
        }
    }

    void Primary::cbMultiPatBlt(rdpContext* context, MULTI_PATBLT_ORDER* multi_patblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MultiPatBlt(context, multi_patblt);
        }
    }

    void Primary::cbMultiScrBlt(rdpContext* context, MULTI_SCRBLT_ORDER* multi_scrblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MultiScrBlt(context, multi_scrblt);
        }
    }

    void Primary::cbMultiOpaqueRect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MultiOpaqueRect(context, multi_opaque_rect);
        }
    }

    void Primary::cbMultiDrawNineGrid(rdpContext* context, MULTI_DRAW_NINE_GRID_ORDER* multi_draw_nine_grid) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MultiDrawNineGrid(context, multi_draw_nine_grid);
        }
    }

    void Primary::cbLineTo(rdpContext* context, LINE_TO_ORDER* line_to) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->LineTo(context, line_to);
        }
    }

    void Primary::cbPolyline(rdpContext* context, POLYLINE_ORDER* polyline) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->Polyline(context, polyline);
        }
    }

    void Primary::cbMemBlt(rdpContext* context, MEMBLT_ORDER* memblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->MemBlt(context, memblt);
        }
    }

    void Primary::cbMem3Blt(rdpContext* context, MEM3BLT_ORDER* memblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->Mem3Blt(context, memblt);
        }
    }

    void Primary::cbSaveBitmap(rdpContext* context, SAVE_BITMAP_ORDER* save_bitmap) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->SaveBitmap(context, save_bitmap);
        }
    }

    void Primary::cbGlyphIndex(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->GlyphIndex(context, glyph_index);
        }
    }

    void Primary::cbFastIndex(rdpContext* context, FAST_INDEX_ORDER* fast_index) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->FastIndex(context, fast_index);
        }
    }

    void Primary::cbFastGlyph(rdpContext* context, FAST_GLYPH_ORDER* fast_glyph) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->FastGlyph(context, fast_glyph);
        }
    }

    void Primary::cbPolygonSC(rdpContext* context, POLYGON_SC_ORDER* polygon_sc) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->PolygonSC(context, polygon_sc);
        }
    }

    void Primary::cbPolygonCB(rdpContext* context, POLYGON_CB_ORDER* polygon_cb) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->PolygonCB(context, polygon_cb);
        }
    }

    void Primary::cbEllipseSC(rdpContext* context, ELLIPSE_SC_ORDER* ellipse_sc) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->EllipseSC(context, ellipse_sc);
        }
    }

    void Primary::cbEllipseCB(rdpContext* context, ELLIPSE_CB_ORDER* ellipse_cb) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->EllipseCB(context, ellipse_cb);
        }
    }

}
