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
        rdp->update->primary->DstBlt = cbDstBlt;
        rdp->update->primary->PatBlt = cbPatBlt;
        rdp->update->primary->ScrBlt = cbScrBlt;
        rdp->update->primary->OpaqueRect = cbOpaqueRect;
        rdp->update->primary->DrawNineGrid = cbDrawNineGrid;
        rdp->update->primary->MultiDstBlt = cbMultiDstBlt;
        rdp->update->primary->MultiPatBlt = cbMultiPatBlt;
        rdp->update->primary->MultiScrBlt = cbMultiScrBlt;
        rdp->update->primary->MultiOpaqueRect = cbMultiOpaqueRect;
        rdp->update->primary->MultiDrawNineGrid = cbMultiDrawNineGrid;
        rdp->update->primary->LineTo = cbLineTo;
        rdp->update->primary->Polyline = cbPolyline;
        rdp->update->primary->MemBlt = cbMemBlt;
        rdp->update->primary->Mem3Blt = cbMem3Blt;
        rdp->update->primary->SaveBitmap = cbSaveBitmap;
        rdp->update->primary->GlyphIndex = cbGlyphIndex;
        rdp->update->primary->FastIndex = cbFastIndex;
        rdp->update->primary->FastGlyph = cbFastGlyph;
        rdp->update->primary->PolygonSC = cbPolygonSC;
        rdp->update->primary->PolygonCB = cbPolygonCB;
        rdp->update->primary->EllipseSC = cbEllipseSC;
        rdp->update->primary->EllipseCB = cbEllipseCB;
    }

    void Primary::DstBlt(rdpContext*, DSTBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PatBlt(rdpContext *ctx, PATBLT_ORDER* po) {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        uint32_t rop3 = gdi_rop3_code(po->bRop);
        HCLRCONV hclrconv = reinterpret_cast<wsgContext *>(ctx)->clrconv;
        if (GDI_BS_SOLID == po->brush.style) {
            log::debug << "PAT S " << hex << rop3 << dec << endl;
            struct {
                uint32_t op;
                int32_t x;
                int32_t y;
                int32_t w;
                int32_t h;
                uint32_t fg;
                uint32_t rop;
            } tmp = {
                WSOP_SC_PATBLT,
                po->nLeftRect,
                po->nTopRect,
                po->nWidth,
                po->nHeight,
                freerdp_color_convert_var(po->foreColor, 16, 32, hclrconv),
                rop3
            };
            string buf(reinterpret_cast<const char *>(&tmp), sizeof(tmp));
            m_wshandler->send_binary(buf);
        } else if (GDI_BS_PATTERN ==  po->brush.style) {
            log::debug << "PAT P " << hex << rop3 << dec << endl;
        } else {
            log::debug << "PAT style " << hex << po->brush.style << dec << endl;
        }
    }

    void Primary::ScrBlt(rdpContext*, SCRBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::OpaqueRect(rdpContext* context, OPAQUE_RECT_ORDER* oro) {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        HCLRCONV hclrconv = reinterpret_cast<wsgContext *>(context)->clrconv;
        uint32_t svcolor = oro->color;
        oro->color = freerdp_color_convert_var(oro->color, 16, 32, hclrconv);
        uint32_t op = WSOP_SC_OPAQUERECT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        buf.append(reinterpret_cast<const char *>(oro), sizeof(OPAQUE_RECT_ORDER));
        m_wshandler->send_binary(buf);
        oro->color = svcolor;
    }

    void Primary::DrawNineGrid(rdpContext*, DRAW_NINE_GRID_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiDstBlt(rdpContext*, MULTI_DSTBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiPatBlt(rdpContext*, MULTI_PATBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiScrBlt(rdpContext*, MULTI_SCRBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiOpaqueRect(rdpContext*, MULTI_OPAQUE_RECT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MultiDrawNineGrid(rdpContext*, MULTI_DRAW_NINE_GRID_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::LineTo(rdpContext*, LINE_TO_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::Polyline(rdpContext*, POLYLINE_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::MemBlt(rdpContext*, MEMBLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::Mem3Blt(rdpContext*, MEM3BLT_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::SaveBitmap(rdpContext*, SAVE_BITMAP_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::GlyphIndex(rdpContext*, GLYPH_INDEX_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::FastIndex(rdpContext*, FAST_INDEX_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::FastGlyph(rdpContext*, FAST_GLYPH_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PolygonSC(rdpContext*, POLYGON_SC_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::PolygonCB(rdpContext*, POLYGON_CB_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::EllipseSC(rdpContext*, ELLIPSE_SC_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    void Primary::EllipseCB(rdpContext*, ELLIPSE_CB_ORDER*) {
        log::debug << __PRETTY_FUNCTION__ << endl;
    }

    // static callbacks
    void Primary::cbDstBlt(rdpContext* context, DSTBLT_ORDER* dstblt) {
        Primary *self = reinterpret_cast<wsgContext *>(context)->pPrimary;
        if (self) {
            self->DstBlt(context, dstblt);
        }
    }

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
