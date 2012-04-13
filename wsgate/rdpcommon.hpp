#ifndef _WSGATE_RDPCOMMON_H_
#define _WSGATE_RDPCOMMON_H_

extern "C" {
#include <freerdp/input.h>
}
#include <freerdp/freerdp.h>

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

    typedef enum {
        WSOP_SC_BEGINPAINT,
        WSOP_SC_ENDPAINT,
        WSOP_SC_BITMAP
    } WsOPsc;

    typedef enum {
        WSOP_CS_MOUSE
    } WsOPcs;

    typedef struct {
        rdpContext _p;
        RDP *pRDP;
        Update *pUpdate;
        Primary *pPrimary;
    } wsgContext;
}

#endif
