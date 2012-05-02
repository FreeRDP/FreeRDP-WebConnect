#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sstream>

#include <pthread.h>

#include "RDP.hpp"
#include "Update.hpp"
#include "Primary.hpp"

#include "btexception.hpp"
extern "C" {
#include <freerdp/locale/keyboard.h>
}

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

namespace wsgate {

    using namespace std;

    map<freerdp *, RDP *> RDP::m_instances;

    RDP::RDP(wspp::wshandler *h)
        : m_freerdp(freerdp_new())
          , m_rdpContext(0)
          , m_rdpInput(0)
          , m_rdpSettings(0)
          , m_bThreadLoop(false)
          , m_worker()
          , m_wshandler(h)
          , m_errMsg()
          , m_State(STATE_INITIAL)
          , m_pUpdate(new Update(h))
          , m_pPrimary(new Primary(h))
          , m_lastError(0)
    {
        if (!m_freerdp) {
            throw tracing::runtime_error("Could not create freerep instance");
        }
        m_instances[m_freerdp] = this;
        m_freerdp->context_size = sizeof(wsgContext);
        m_freerdp->ContextNew = cbContextNew;
        m_freerdp->ContextFree = cbContextFree;
        m_freerdp->Authenticate = cbAuthenticate;
        m_freerdp->VerifyCertificate = cbVerifyCertificate;

        freerdp_context_new(m_freerdp);
        reinterpret_cast<wsgContext *>(m_freerdp->context)->pRDP = this;
        reinterpret_cast<wsgContext *>(m_freerdp->context)->pUpdate = m_pUpdate;
        reinterpret_cast<wsgContext *>(m_freerdp->context)->pPrimary = m_pPrimary;
        // create worker thread
        m_bThreadLoop = true;
        if (0 != pthread_create(&m_worker, NULL, cbThreadFunc, reinterpret_cast<void *>(this))) {
            m_bThreadLoop = false;
            log::err << "Could not create RDP client thread" << endl;
        } else {
            log::debug << "Created RDP client thread" << endl;
        }
    }

    RDP::~RDP()
    {
        log::debug << __PRETTY_FUNCTION__ << endl;
        Disconnect();
        freerdp_context_free(m_freerdp);
        freerdp_free(m_freerdp);
        m_instances.erase(m_freerdp);
        delete m_pUpdate;
        delete m_pPrimary;
    }

    bool RDP::Connect(string host, int port, string user, string domain, string pass,
            const int width, const int height)
    {
        if (!m_rdpSettings) {
            throw tracing::runtime_error("m_rdpSettings is NULL");
        }
        if (!m_bThreadLoop) {
            throw tracing::runtime_error("worker thread has terminated");
        }
        m_rdpSettings->width = width;
        m_rdpSettings->height = height;
        m_rdpSettings->port = port;
        m_rdpSettings->ignore_certificate = 1;
        m_rdpSettings->hostname = strdup(host.c_str());
        m_rdpSettings->username = strdup(user.c_str());
        if (!domain.empty()) {
            m_rdpSettings->domain = strdup(domain.c_str());
        }
        if (!pass.empty()) {
            m_rdpSettings->password = strdup(pass.c_str());
        } else {
            m_rdpSettings->authentication = 0;
        }
        m_State = STATE_CONNECT;
        return true;
    }

    bool RDP::Disconnect()
    {
        if (m_bThreadLoop) {
            m_bThreadLoop = false;
            if (STATE_CONNECTED == m_State) {
                m_State = STATE_CLOSED;
                return (freerdp_disconnect(m_freerdp) != 0);
            }
            pthread_join(m_worker, NULL);
        }
        return true;
    }

    void RDP::OnWsMessage(const string & data)
    {
        if ((STATE_CONNECTED == m_State) && (data.length() >= 4)) {
            const uint32_t *op = reinterpret_cast<const uint32_t *>(data.data());
            switch (*op) {
                case WSOP_CS_MOUSE:
                    {
                        typedef struct {
                            uint32_t op;
                            uint32_t flags;
                            uint32_t x;
                            uint32_t y;
                        } wsmsg;
                        const wsmsg *m = reinterpret_cast<const wsmsg *>(data.data());
                        // log::debug << "MM: x=" << m->x << " y=" << m->y << endl;
                        m_freerdp->input->MouseEvent(m_freerdp->input, m->flags, m->x, m->y);
                    }
                    break;
                case WSOP_CS_KUPDOWN:
                    // used only for modifiers
                    {
                        typedef struct {
                            uint32_t op;
                            uint32_t down;
                            uint32_t code;
                        } wsmsg;
                        const wsmsg *m = reinterpret_cast<const wsmsg *>(data.data());
                        log::debug << "K" << ((m->down) ? "down" : "up") << ": c=" << m->code << endl;
                        uint32_t tcode = 0;
                        switch (m->code) {
                            case 16:
                                tcode = RDP_SCANCODE_LSHIFT;
                                break;
                            case 17:
                                tcode = RDP_SCANCODE_LCONTROL;
                                break;
                            case 18:
                                tcode = RDP_SCANCODE_LMENU; // Alt
                                break;
                            case 20:
                                // capslock
                                // tcode = RDP_SCANCODE_;
                                break;
                            case 93:
                                tcode = RDP_SCANCODE_LWIN; // Win-Key
                                break;
                            case 144:
                                // numlock
                                // tcode = RDP_SCANCODE_;
                                break;
                            case 145:
                                // scrolllock
                                // tcode = RDP_SCANCODE_;
                                break;
                        }
                        if (0 < tcode) {
                            uint32_t tflag = rdp_scancode_extended(tcode) ? KBD_FLAGS_EXTENDED : 0;
                            tcode = rdp_scancode_code(tcode);
                            SendInputKeyboardEvent((m->down ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE)|tflag, tcode);
                        }
                    }
                    break;
                case WSOP_CS_KPRESS:
                    {
                        typedef struct {
                            uint32_t op;
                            uint32_t shiftstate;
                            uint32_t code;
                        } wsmsg;
                        const wsmsg *m = reinterpret_cast<const wsmsg *>(data.data());
                        uint32_t tcode = m->code;
                        log::debug << "Kpress c=" << m->code << ", ss=" << hex << m->shiftstate << dec << endl;
                        if (0x2F < m->code) {
                            if (m->shiftstate & 6) {
                                // Control and or Alt: Must use scan-codes since unicode-event can't handle these
                                if (((64 < tcode) && (91 > tcode)) || ((96 < tcode) && (123 > tcode))) {
                                    tcode -= (m->shiftstate & 1) ? 0 : 32;
                                    tcode = freerdp_keyboard_get_rdp_scancode_from_virtual_key_code(tcode);
                                    log::debug << "Kpress oc=" << tcode << endl;
                                    if (0 < tcode) {
                                        SendInputKeyboardEvent(KBD_FLAGS_DOWN, tcode);
                                        SendInputKeyboardEvent(KBD_FLAGS_RELEASE, tcode);
                                    }
                                }
                            } else {
                                if (0 < tcode) {
                                    SendInputUnicodeKeyboardEvent(KBD_FLAGS_DOWN, tcode);
                                    SendInputUnicodeKeyboardEvent(KBD_FLAGS_RELEASE, tcode);
                                }
                            }
                        } else {
                            tcode = 0;
                            switch (m->code) {
                                case 8:
                                    tcode = RDP_SCANCODE_BACKSPACE;
                                    break;
                                case 9:
                                    tcode = RDP_SCANCODE_TAB;
                                    break;
                                case 13:
                                    tcode = RDP_SCANCODE_RETURN;
                                    break;
                                case 19:
                                    tcode = RDP_SCANCODE_PAUSE;
                                    break;
                                case 27:
                                    tcode = RDP_SCANCODE_ESCAPE;
                                    break;
                                case 32:
                                    tcode = RDP_SCANCODE_SPACE;
                                    break;
                                case 33:
                                    tcode = RDP_SCANCODE_PRIOR; // Page up
                                    break;
                                case 34:
                                    tcode = RDP_SCANCODE_NEXT; // Page down
                                    break;
                                case 35:
                                    tcode = RDP_SCANCODE_END;
                                    break;
                                case 36:
                                    tcode = RDP_SCANCODE_HOME;
                                    break;
                                case 37:
                                    tcode = RDP_SCANCODE_LEFT;
                                    break;
                                case 38:
                                    tcode = RDP_SCANCODE_UP;
                                    break;
                                case 39:
                                    tcode = RDP_SCANCODE_RIGHT;
                                    break;
                                case 40:
                                    tcode = RDP_SCANCODE_DOWN;
                                    break;
                                case 44:
                                    tcode = RDP_SCANCODE_PRINTSCREEN;
                                    break;
                                case 45:
                                    tcode = RDP_SCANCODE_INSERT;
                                    break;
                                case 46:
                                    tcode = RDP_SCANCODE_DELETE;
                                    break;
                            }
                            if (0 < tcode) {
                                uint32_t tflag = rdp_scancode_extended(tcode) ? KBD_FLAGS_EXTENDED : 0;
                                tcode = rdp_scancode_code(tcode);
                                SendInputKeyboardEvent(KBD_FLAGS_DOWN|tflag, tcode);
                                SendInputKeyboardEvent(KBD_FLAGS_RELEASE|tflag, tcode);
                            }
                        }
                    }
                    break;
            }
        }
    }

    bool RDP::CheckFileDescriptor()
    {
        return ((freerdp_check_fds(m_freerdp) == 0) ? false : true);
    }

    void RDP::SendInputSynchronizeEvent(uint32_t flags)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        freerdp_input_send_synchronize_event(m_rdpInput, flags);
    }

    void RDP::SendInputKeyboardEvent(uint16_t flags, uint16_t code)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        freerdp_input_send_keyboard_event(m_rdpInput, flags, code);
    }

    void RDP::SendInputUnicodeKeyboardEvent(uint16_t flags, uint16_t code)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        freerdp_input_send_unicode_keyboard_event(m_rdpInput, flags, code);
    }

    void RDP::SendInputMouseEvent(uint16_t flags, uint16_t x, uint16_t y)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        freerdp_input_send_mouse_event(m_rdpInput, flags, x, y);
    }

    void RDP::SendInputExtendedMouseEvent(uint16_t flags, uint16_t x, uint16_t y)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        freerdp_input_send_extended_mouse_event(m_rdpInput, flags, x, y);
    }

    // private
    void RDP::ContextNew(freerdp *inst, rdpContext *ctx)
    {
        log::debug << "RDP::ContextNew" << endl;
        inst->PreConnect = cbPreConnect;
        inst->PostConnect = cbPostConnect;
        m_rdpContext = ctx;
        m_rdpInput = inst->input;
        m_rdpSettings = inst->settings;
    }

    // private
    void RDP::ContextFree(freerdp *, rdpContext *)
    {
        log::debug << "RDP::ContextFree" << endl;
    }

    // private
    boolean RDP::PreConnect(freerdp *rdp)
    {
        log::debug << "RDP::PreConnect" << endl;
        m_pUpdate->Register(rdp);
        m_pPrimary->Register(rdp);

        // Settings for RFX:
#if 0
        m_rdpSettings->rfx_codec = 1;
        m_rdpSettings->fastpath_output = 1;
        m_rdpSettings->color_depth = 32;
        m_rdpSettings->frame_acknowledge = 0;
        m_rdpSettings->performance_flags = PERF_FLAG_NONE;
        m_rdpSettings->large_pointer = true;
#endif

        // TODO: configurable 
        m_rdpSettings->disable_wallpaper = 0;
        m_rdpSettings->disable_full_window_drag = 0;
        m_rdpSettings->disable_menu_animations = 0;
        m_rdpSettings->disable_theming = 0;
        // ? settings->dektop_composition = 0;

        m_rdpSettings->rfx_codec = 0;
        m_rdpSettings->fastpath_output = 1;
        m_rdpSettings->color_depth = 16;
        m_rdpSettings->frame_acknowledge = 0;
        m_rdpSettings->performance_flags = 0;
        m_rdpSettings->large_pointer = 1;
        m_rdpSettings->glyph_cache = 0;
        m_rdpSettings->bitmap_cache = 0;
        m_rdpSettings->offscreen_bitmap_cache = 0;

        m_rdpSettings->order_support[NEG_DSTBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_PATBLT_INDEX] = 1;
        m_rdpSettings->order_support[NEG_SCRBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MEMBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MEM3BLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_ATEXTOUT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_AEXTTEXTOUT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_DRAWNINEGRID_INDEX] = 0;
        m_rdpSettings->order_support[NEG_LINETO_INDEX] = 1;
        m_rdpSettings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = 0;
        m_rdpSettings->order_support[NEG_OPAQUE_RECT_INDEX] = 1;
        m_rdpSettings->order_support[NEG_SAVEBITMAP_INDEX] = 0;
        m_rdpSettings->order_support[NEG_WTEXTOUT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MEMBLT_V2_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MEM3BLT_V2_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MULTIDSTBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MULTIPATBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MULTISCRBLT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_MULTIOPAQUERECT_INDEX] = 1;
        m_rdpSettings->order_support[NEG_FAST_INDEX_INDEX] = 0;
        m_rdpSettings->order_support[NEG_POLYGON_SC_INDEX] = 0;
        m_rdpSettings->order_support[NEG_POLYGON_CB_INDEX] = 0;
        m_rdpSettings->order_support[NEG_POLYLINE_INDEX] = 0;
        m_rdpSettings->order_support[NEG_FAST_GLYPH_INDEX] = 0;
        m_rdpSettings->order_support[NEG_ELLIPSE_SC_INDEX] = 0;
        m_rdpSettings->order_support[NEG_ELLIPSE_CB_INDEX] = 0;
        m_rdpSettings->order_support[NEG_GLYPH_INDEX_INDEX] = 0;
        m_rdpSettings->order_support[NEG_GLYPH_WEXTTEXTOUT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_GLYPH_WLONGTEXTOUT_INDEX] = 0;
        m_rdpSettings->order_support[NEG_GLYPH_WLONGEXTTEXTOUT_INDEX] = 0;

        reinterpret_cast<wsgContext *>(m_freerdp->context)->clrconv =
            freerdp_clrconv_new(CLRCONV_ALPHA|CLRCONV_INVERT);

        return 1;
    }

    // private
    boolean RDP::PostConnect(freerdp *rdp)
    {
        log::debug << "RDP::PostConnect " << hex << rdp << dec << endl;
        // gdi_init(rdp, CLRCONV_ALPHA | CLRCONV_INVERT | CLRBUF_16BPP | CLRBUF_32BPP, NULL);
        return 1;
    }

    // private
    boolean RDP::Authenticate(freerdp *, char**, char**, char**)
    {
        log::debug << "RDP::Authenticate" << endl;
        return 1;
    }

    // private
    boolean RDP::VerifyCertificate(freerdp *, char*, char*, char*)
    {
        log::debug << "RDP::VerifyCertificate" << endl;
        return 1;
    }

    // private
    void RDP::addError(const std::string &msg)
    {
        if (!m_errMsg.empty()) {
            m_errMsg.append("\n");
        }
        m_errMsg.append("E:").append(msg);
    }

    // private
    void RDP::ThreadFunc()
    {
        while (m_bThreadLoop) {
            uint32_t e = freerdp_error_info(m_freerdp);
            if (0 != e) {
                if (m_lastError != e) {
                    m_lastError = e;
                    switch (m_lastError) {
                        case 1:
                            // No really an error
                            // (Happens when you invoke Disconnect in Start-Menu)
                            m_bThreadLoop = false;
                            break;
                        case 5:
                            addError("Another user connected to the server,\nforcing the disconnection of the current connection.");
                            break;
                        default:
                            {
                                ostringstream oss;
                                oss << "Server reported error 0x" << hex << m_lastError;
                                addError(oss.str());
                            }
                            break;
                    }
                }
            }
            if (!m_errMsg.empty()) {
                log::debug << m_errMsg << endl;
                m_wshandler->send_text(m_errMsg);
                m_errMsg.clear();
            }
            if (freerdp_shall_disconnect(m_freerdp)) {
                break;
            }
            switch (m_State) {
                case STATE_CONNECTED:
                    CheckFileDescriptor();
                    break;
                case STATE_CONNECT:
                    if (freerdp_connect(m_freerdp)) {
                        m_State = STATE_CONNECTED;
                        continue;
                    }
                    m_State = STATE_INITIAL;
                    addError("Could not connect to RDP backend.");
                    break;
                case STATE_INITIAL:
                case STATE_CLOSED:
                    break;
            }
            usleep(100);
        }
        log::debug << "RDP client thread terminated" << endl;
        if (STATE_CONNECTED == m_State) {
            m_wshandler->send_text("T:");
        }
    }

    // private C callback
    void RDP::cbContextNew(freerdp *inst, rdpContext *ctx)
    {
        RDP *self = m_instances[inst];
        if (self) {
            self->ContextNew(inst, ctx);
        }
    }

    void *RDP::cbThreadFunc(void *ctx)
    {
        RDP *self = reinterpret_cast<RDP *>(ctx);
        if (self) {
            self->ThreadFunc();
        }
        return NULL;
    }

    // private C callback
    void RDP::cbContextFree(freerdp *inst, rdpContext *ctx)
    {
        RDP *self = m_instances[inst];
        if (self) {
            self->ContextFree(inst, ctx);
        }
    }

    // private C callback
    boolean RDP::cbPreConnect(freerdp *inst)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->PreConnect(inst);
        }
        return 0;
    }

    // private C callback
    boolean RDP::cbPostConnect(freerdp *inst)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->PostConnect(inst);
        }
        return 0;
    }

    // private C callback
    boolean RDP::cbAuthenticate(freerdp *inst, char** user, char** pass, char** domain)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->Authenticate(inst, user, pass, domain);
        }
        return 0;
    }

    // private C callback
    boolean RDP::cbVerifyCertificate(freerdp *inst, char* subject, char* issuer, char* fprint)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->VerifyCertificate(inst, subject, issuer, fprint);
        }
        return 0;
    }

}
