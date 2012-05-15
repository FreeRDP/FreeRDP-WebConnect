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
#ifndef _WSGATE_PRIMARY_H_
#define _WSGATE_PRIMARY_H_

namespace wsgate {

    /**
     * Implementation of the FreeRDP Primary interface.
     * This class implments callbacks for FreeRDP's Primary API.
     */ 
    class Primary {

        public:
            /**
             * Constructs a new instance.
             * @param h A pointer to the corresponding wshandler object.
             */
            Primary(wspp::wshandler *h);

            /// Destructor
            virtual ~Primary();

            /**
             * Registers the callbacks ath FreeRDP's API.
             * @param rdp A pointer to the FreeRDP instance.
             */
            void Register(freerdp *rdp);

        private:
            wspp::wshandler *m_wshandler;

            // Non-copyable
            Primary(const Primary &);
            Primary & operator=(const Primary &);

            void DstBlt(rdpContext* context, DSTBLT_ORDER* dstblt);
            void PatBlt(rdpContext* context, PATBLT_ORDER* patblt);
            void ScrBlt(rdpContext* context, SCRBLT_ORDER* scrblt);
            void OpaqueRect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect);
            void DrawNineGrid(rdpContext* context, DRAW_NINE_GRID_ORDER* draw_nine_grid);
            void MultiDstBlt(rdpContext* context, MULTI_DSTBLT_ORDER* multi_dstblt);
            void MultiPatBlt(rdpContext* context, MULTI_PATBLT_ORDER* multi_patblt);
            void MultiScrBlt(rdpContext* context, MULTI_SCRBLT_ORDER* multi_scrblt);
            void MultiOpaqueRect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect);
            void MultiDrawNineGrid(rdpContext* context, MULTI_DRAW_NINE_GRID_ORDER* multi_draw_nine_grid);
            void LineTo(rdpContext* context, LINE_TO_ORDER* line_to);
            void Polyline(rdpContext* context, POLYLINE_ORDER* polyline);
            void MemBlt(rdpContext* context, MEMBLT_ORDER* memblt);
            void Mem3Blt(rdpContext* context, MEM3BLT_ORDER* memblt);
            void SaveBitmap(rdpContext* context, SAVE_BITMAP_ORDER* save_bitmap);
            void GlyphIndex(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index);
            void FastIndex(rdpContext* context, FAST_INDEX_ORDER* fast_index);
            void FastGlyph(rdpContext* context, FAST_GLYPH_ORDER* fast_glyph);
            void PolygonSC(rdpContext* context, POLYGON_SC_ORDER* polygon_sc);
            void PolygonCB(rdpContext* context, POLYGON_CB_ORDER* polygon_cb);
            void EllipseSC(rdpContext* context, ELLIPSE_SC_ORDER* ellipse_sc);
            void EllipseCB(rdpContext* context, ELLIPSE_CB_ORDER* ellipse_cb);

            // Callbacks from C - Must be static in order t be assigned to C fnPtrs.
            static void cbDstBlt(rdpContext* context, DSTBLT_ORDER* dstblt);
            static void cbPatBlt(rdpContext* context, PATBLT_ORDER* patblt);
            static void cbScrBlt(rdpContext* context, SCRBLT_ORDER* scrblt);
            static void cbOpaqueRect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect);
            static void cbDrawNineGrid(rdpContext* context, DRAW_NINE_GRID_ORDER* draw_nine_grid);
            static void cbMultiDstBlt(rdpContext* context, MULTI_DSTBLT_ORDER* multi_dstblt);
            static void cbMultiPatBlt(rdpContext* context, MULTI_PATBLT_ORDER* multi_patblt);
            static void cbMultiScrBlt(rdpContext* context, MULTI_SCRBLT_ORDER* multi_scrblt);
            static void cbMultiOpaqueRect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect);
            static void cbMultiDrawNineGrid(rdpContext* context, MULTI_DRAW_NINE_GRID_ORDER* multi_draw_nine_grid);
            static void cbLineTo(rdpContext* context, LINE_TO_ORDER* line_to);
            static void cbPolyline(rdpContext* context, POLYLINE_ORDER* polyline);
            static void cbMemBlt(rdpContext* context, MEMBLT_ORDER* memblt);
            static void cbMem3Blt(rdpContext* context, MEM3BLT_ORDER* memblt);
            static void cbSaveBitmap(rdpContext* context, SAVE_BITMAP_ORDER* save_bitmap);
            static void cbGlyphIndex(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index);
            static void cbFastIndex(rdpContext* context, FAST_INDEX_ORDER* fast_index);
            static void cbFastGlyph(rdpContext* context, FAST_GLYPH_ORDER* fast_glyph);
            static void cbPolygonSC(rdpContext* context, POLYGON_SC_ORDER* polygon_sc);
            static void cbPolygonCB(rdpContext* context, POLYGON_CB_ORDER* polygon_cb);
            static void cbEllipseSC(rdpContext* context, ELLIPSE_SC_ORDER* ellipse_sc);
            static void cbEllipseCB(rdpContext* context, ELLIPSE_CB_ORDER* ellipse_cb);

    };
}

#endif
