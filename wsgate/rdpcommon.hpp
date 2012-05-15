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
#ifndef _WSGATE_RDPCOMMON_H_
#define _WSGATE_RDPCOMMON_H_

extern "C" {
#include <freerdp/input.h>
#include <freerdp/gdi/gdi.h>
}
#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>

#include "wsgate.hpp"
#include "wshandler.hpp"

// #define DBGLOG_POINTER_NEW
// #define DBGLOG_POINTER_FREE
// #define DBGLOG_POINTER_SET
// #define DBGLOG_POINTER_SETNULL
// #define DBGLOG_POINTER_SETDEFAULT
// #define DBGLOG_BEGINPAINT
// #define DBGLOG_ENDPAINT
// #define DBGLOG_SETBOUNDS
// #define DBGLOG_BITMAP
// #define DBGLOG_OPAQUERECT
// #define DBGLOG_MULTI_OPAQUERECT
// #define DBGLOG_PATBLT
// #define DBGLOG_SCRBLT

typedef struct rdp_freerdp freerdp;
typedef struct rdp_context rdpContext;
typedef struct rdp_input rdpInput;
typedef struct rdp_settings rdpSettings;

namespace wsgate {

    class RDP;
    class Update;
    class Primary;
    struct CLRCONV;

    /**
     * OP-Codes, sent from the server to
     * the (JavaScript) client.
     */
    typedef enum {
        WSOP_SC_BEGINPAINT,
        WSOP_SC_ENDPAINT,
        WSOP_SC_BITMAP,
        WSOP_SC_OPAQUERECT,
        WSOP_SC_SETBOUNDS,
        WSOP_SC_PATBLT,
        WSOP_SC_MULTI_OPAQUERECT,
        WSOP_SC_SCRBLT,
        WSOP_SC_PTR_NEW,
        WSOP_SC_PTR_FREE,
        WSOP_SC_PTR_SET,
        WSOP_SC_PTR_SETNULL,
        WSOP_SC_PTR_SETDEFAULT
    } WsOPsc;

    /**
     * OP-Codes, sent from the (JavaScript)
     * client to the server.
     */
    typedef enum {
        WSOP_CS_MOUSE,
        WSOP_CS_KUPDOWN,
        WSOP_CS_KPRESS
    } WsOPcs;

    /**
     * Our set of session parameters
     * (from html form).
     */
    typedef struct {
        /// The RDP port to connect to
        int port;
        /// The desktop width for the RDP session.
        int width;
        /// The desktop height for the RDP session.
        int height;
        /// The performance flags for the RDP session.
        int perf;
        /// The NTLM auth version.
        int fntlm;
        /// Flag: Disable TLS.
        int notls;
        /// Flag: Disable network level authentication.
        int nonla;
        /// Flag: Disable wallpaper.
        int nowallp;
        /// Flag: Disable full window drag.
        int nowdrag;
        /// Flag: Disable menu animations.
        int nomani;
        /// Flag: Disable theming.
        int notheme;
    } WsRdpParams;

    /**
     * Our extension of FreeRDP's context
     */
    typedef struct {
        /**
         * FreeRDP's original portion.
         */
        rdpContext _p;
        /**
         * Pointer to the main RDP handler.
         */
        RDP *pRDP;
        /**
         * Pointer to the corresponding Update API module.
         */
        Update *pUpdate;
        /**
         * Pointer to the corresponding Primary API module.
         */
        Primary *pPrimary;
        /**
         * The current color space conversion parameter.
         */
        HCLRCONV clrconv;
    } wsgContext;
}

#endif
