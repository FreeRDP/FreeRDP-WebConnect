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
        WSOP_SC_PATBLT
    } WsOPsc;

    /**
     * OP-Codes, sent from the (JavaScript)
     * client to the server.
     */
    typedef enum {
        WSOP_CS_MOUSE
    } WsOPcs;

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
