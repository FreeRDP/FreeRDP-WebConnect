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

    typedef enum {
        WSOP_SC_BEGINPAINT,
        WSOP_SC_ENDPAINT,
        WSOP_SC_BITMAP,
        WSOP_SC_OPAQUERECT,
        WSOP_SC_PATBLT
    } WsOPsc;

    typedef enum {
        WSOP_CS_MOUSE
    } WsOPcs;

    typedef struct {
        rdpContext _p;
        RDP *pRDP;
        Update *pUpdate;
        Primary *pPrimary;
        HCLRCONV clrconv;
    } wsgContext;
}

#endif
