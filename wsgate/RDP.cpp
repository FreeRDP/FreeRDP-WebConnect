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

#include <sstream>
#include <iomanip>

#include <pthread.h>

#include "RDP.hpp"
#include "Update.hpp"
#include "Primary.hpp"
#include "Png.hpp"

#include "btexception.hpp"
extern "C" {
#include <freerdp/locale/keyboard.h>
}


#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

namespace wsgate {

    using namespace std;

    const UINT32 ASCII_TO_SCANCODE[256] =
{
    0, /* 0 */
    0, /* 1 */
    0, /* 2 */
    0, /* 3 */
    0, /* 4 */
    0, /* 5 */
    0, /* 6 */
    0, /* 7 */
    VK_KEY_A, /* 8 */
    RDP_SCANCODE_TAB, /* 9 */
    VK_KEY_D, /* 10 */
    VK_KEY_F, /* 11 */
    VK_KEY_H, /* 12 */
    RDP_SCANCODE_RETURN, /* 13 */
    RDP_SCANCODE_BACKSPACE, /* 14 */
    VK_KEY_X, /* 15 */
    VK_KEY_C, /* 16 */
    RDP_SCANCODE_LCONTROL, /* 17 */
    RDP_SCANCODE_LMENU, /* 18 */
    RDP_SCANCODE_PAUSE, /* 19 */
    VK_KEY_Q, /* 20 */
    VK_KEY_W, /* 21 */
    VK_KEY_E, /* 22 */
    VK_KEY_R, /* 23 */
    VK_KEY_Y, /* 24 */
    VK_KEY_A, /* 25 */
    VK_KEY_1, /* 26 */
    RDP_SCANCODE_ESCAPE, /* 27 */
    VK_KEY_3, /* 28 */
    VK_KEY_4, /* 29 */
    VK_KEY_6, /* 30 */
    VK_KEY_5, /* 31 */
    RDP_SCANCODE_SPACE, /* 32 */
    RDP_SCANCODE_KEY_1, /* 33 */
    RDP_SCANCODE_OEM_7, /* 34 */
    RDP_SCANCODE_KEY_3, /* 35 */
    RDP_SCANCODE_KEY_4, /* 36 */
    RDP_SCANCODE_KEY_5, /* 37 */
    RDP_SCANCODE_KEY_7, /* 38 */
    RDP_SCANCODE_OEM_7, /* 39 */
    RDP_SCANCODE_KEY_9, /* 40 */
    RDP_SCANCODE_KEY_0, /* 41 */
    RDP_SCANCODE_MULTIPLY, /* 42 */
    RDP_SCANCODE_ADD, /* 43 */
    RDP_SCANCODE_OEM_COMMA, /* 44 */
    RDP_SCANCODE_OEM_MINUS, /* 45 */
    RDP_SCANCODE_OEM_PERIOD, /* 46 */
    RDP_SCANCODE_DIVIDE, /* 47 */
    RDP_SCANCODE_KEY_0, /* 48 */
    RDP_SCANCODE_KEY_1, /* 49 */
    RDP_SCANCODE_KEY_2, /* 50 */
    RDP_SCANCODE_KEY_3, /* 51 */
    RDP_SCANCODE_KEY_4, /* 52 */
    RDP_SCANCODE_KEY_5, /* 53 */
    RDP_SCANCODE_KEY_6, /* 54 */
    RDP_SCANCODE_KEY_7, /* 55 */
    RDP_SCANCODE_KEY_8, /* 56 */
    RDP_SCANCODE_KEY_9, /* 57 */
    RDP_SCANCODE_OEM_1, /* 58 */
    RDP_SCANCODE_OEM_1, /* 59 */
    RDP_SCANCODE_OEM_COMMA, /* 60 */
    RDP_SCANCODE_OEM_PLUS, /* 61 */
    RDP_SCANCODE_OEM_PERIOD, /* 62 */
    RDP_SCANCODE_DIVIDE, /* 63 */
    RDP_SCANCODE_KEY_2, /* 64 */
    RDP_SCANCODE_KEY_A, /* 65 */
    RDP_SCANCODE_KEY_B, /* 66 */
    RDP_SCANCODE_KEY_C, /* 67 */
    RDP_SCANCODE_KEY_D, /* 68 */
    RDP_SCANCODE_KEY_E, /* 69 */
    RDP_SCANCODE_KEY_F, /* 70 */
    RDP_SCANCODE_KEY_G, /* 71 */
    RDP_SCANCODE_KEY_H, /* 72 */
    RDP_SCANCODE_KEY_I, /* 73 */
    RDP_SCANCODE_KEY_J, /* 74 */
    RDP_SCANCODE_KEY_K, /* 75 */
    RDP_SCANCODE_KEY_L, /* 76 */
    RDP_SCANCODE_KEY_M, /* 77 */
    RDP_SCANCODE_KEY_N, /* 78 */
    RDP_SCANCODE_KEY_O, /* 79 */
    RDP_SCANCODE_KEY_P, /* 80 */
    RDP_SCANCODE_KEY_Q, /* 81 */
    RDP_SCANCODE_KEY_R, /* 82 */
    RDP_SCANCODE_KEY_S, /* 83 */
    RDP_SCANCODE_KEY_T, /* 84 */
    RDP_SCANCODE_KEY_U, /* 85 */
    RDP_SCANCODE_KEY_V, /* 86 */
    RDP_SCANCODE_KEY_W, /* 87 */
    RDP_SCANCODE_KEY_X, /* 88 */
    RDP_SCANCODE_KEY_Y, /* 89 */
    RDP_SCANCODE_KEY_Z, /* 90 */
    RDP_SCANCODE_OEM_4, /* 91 */
    RDP_SCANCODE_OEM_5, /* 92 */
    RDP_SCANCODE_OEM_6, /* 93 */
    RDP_SCANCODE_KEY_6, /* 94 */
    RDP_SCANCODE_OEM_MINUS, /* 95 */
    VK_NUMPAD6, /* 96 */
    RDP_SCANCODE_KEY_A, /* 97 */
    RDP_SCANCODE_KEY_B, /* 98 */
    RDP_SCANCODE_KEY_C, /* 99 */
    RDP_SCANCODE_KEY_D, /* 100 */
    RDP_SCANCODE_KEY_E, /* 101 */
    RDP_SCANCODE_KEY_F, /* 102 */
    RDP_SCANCODE_KEY_G, /* 103 */
    RDP_SCANCODE_KEY_H, /* 104 */
    RDP_SCANCODE_KEY_I, /* 105 */
    RDP_SCANCODE_KEY_J, /* 106 */
    RDP_SCANCODE_KEY_K, /* 107 */
    RDP_SCANCODE_KEY_L, /* 108 */
    RDP_SCANCODE_KEY_M, /* 109 */
    RDP_SCANCODE_KEY_N, /* 110 */
    RDP_SCANCODE_KEY_O, /* 111 */
    RDP_SCANCODE_KEY_P, /* 112 */
    RDP_SCANCODE_KEY_Q, /* 113 */
    RDP_SCANCODE_KEY_R, /* 114 */
    RDP_SCANCODE_KEY_S, /* 115 */
    RDP_SCANCODE_KEY_T, /* 116 */
    RDP_SCANCODE_KEY_U, /* 117 */
    RDP_SCANCODE_KEY_V, /* 118 */
    RDP_SCANCODE_KEY_W, /* 119 */
    RDP_SCANCODE_KEY_X, /* 120 */
    RDP_SCANCODE_KEY_Y, /* 121 */
    RDP_SCANCODE_KEY_Z, /* 122 */
    RDP_SCANCODE_OEM_4, /* 123 */
    RDP_SCANCODE_OEM_5, /* 124 */
    RDP_SCANCODE_OEM_6, /* 125 */
    VK_F4, /* 126 */
    VK_END, /* 127 */
    VK_F2, /* 128 */
    VK_NEXT, /* 129 */
    VK_F1, /* 130 */
    VK_LEFT, /* 131 */
    VK_RIGHT, /* 132 */
    VK_DOWN, /* 133 */
    VK_UP, /* 134 */
    0, /* 135 */
    0, /* 136 */
    0, /* 137 */
    0, /* 138 */
    0, /* 139 */
    0, /* 140 */
    0, /* 141 */
    0, /* 142 */
    0, /* 143 */
    0, /* 144 */
    0, /* 145 */
    0, /* 146 */
    0, /* 147 */
    0, /* 148 */
    0, /* 149 */
    0, /* 150 */
    0, /* 151 */
    0, /* 152 */
    0, /* 153 */
    0, /* 154 */
    0, /* 155 */
    0, /* 156 */
    0, /* 157 */
    0, /* 158 */
    0, /* 159 */
    0, /* 160 */
    0, /* 161 */
    0, /* 162 */
    0, /* 163 */
    0, /* 164 */
    0, /* 165 */
    0, /* 166 */
    0, /* 167 */
    0, /* 168 */
    0, /* 169 */
    0, /* 170 */
    0, /* 171 */
    0, /* 172 */
    0, /* 173 */
    0, /* 174 */
    0, /* 175 */
    0, /* 176 */
    0, /* 177 */
    0, /* 178 */
    0, /* 179 */
    0, /* 180 */
    0, /* 181 */
    0, /* 182 */
    0, /* 183 */
    0, /* 184 */
    0, /* 185 */
    0, /* 186 */
    0, /* 187 */
    0, /* 188 */
    0, /* 189 */
    0, /* 190 */
    0, /* 191 */
    0, /* 192 */
    0, /* 193 */
    0, /* 194 */
    0, /* 195 */
    0, /* 196 */
    0, /* 197 */
    0, /* 198 */
    0, /* 199 */
    0, /* 200 */
    0, /* 201 */
    0, /* 202 */
    0, /* 203 */
    0, /* 204 */
    0, /* 205 */
    0, /* 206 */
    0, /* 207 */
    0, /* 208 */
    0, /* 209 */
    0, /* 210 */
    0, /* 211 */
    0, /* 212 */
    0, /* 213 */
    0, /* 214 */
    0, /* 215 */
    0, /* 216 */
    0, /* 217 */
    0, /* 218 */
    0, /* 219 */
    0, /* 220 */
    0, /* 221 */
    0, /* 222 */
    0, /* 223 */
    0, /* 224 */
    0, /* 225 */
    0, /* 226 */
    0, /* 227 */
    0, /* 228 */
    0, /* 229 */
    0, /* 230 */
    0, /* 231 */
    0, /* 232 */
    0, /* 233 */
    0, /* 234 */
    0, /* 235 */
    0, /* 236 */
    0, /* 237 */
    0, /* 238 */
    0, /* 239 */
    0, /* 240 */
    0, /* 241 */
    0, /* 242 */
    0, /* 243 */
    0, /* 244 */
    0, /* 245 */
    0, /* 246 */
    0, /* 247 */
    0, /* 248 */
    0, /* 249 */
    0, /* 250 */
    0, /* 251 */
    0, /* 252 */
    0, /* 253 */
    0, /* 254 */
    0 /* 255 */
};


    map<freerdp *, RDP *> RDP::m_instances;

    typedef struct {
        rdpPointer pointer;
        uint32_t id;
    } MyPointer;

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
          , m_ptrId(1)
          , m_cursorMap()
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

    bool RDP::Connect(string host, string pcb, string user, string domain, string pass,
            const WsRdpParams &params)
    {
        if (!m_rdpSettings) {
            throw tracing::runtime_error("m_rdpSettings is NULL");
        }
        if (!m_bThreadLoop) {
            throw tracing::runtime_error("worker thread has terminated");
        }

         m_rdpSettings->ServerPort = 3389;

        if(pcb != "")
        {
        	m_rdpSettings->SendPreconnectionPdu = TRUE;
        	m_rdpSettings->PreconnectionBlob = strdup(pcb.c_str());
            m_rdpSettings->ServerPort=2179;
        }

        m_rdpSettings->DesktopWidth = params.width;
        m_rdpSettings->DesktopHeight = params.height;


        //THIS DOES NOT WORK!!!     ---- >>>    m_rdpSettings->ServerPort = params.port;

        //log::info << "ServerPort = " << m_rdpSettings->ServerPort << " & params.port = " << params.port << endl;

        m_rdpSettings->IgnoreCertificate = TRUE;
        m_rdpSettings->NegotiateSecurityLayer = FALSE;
        m_rdpSettings->ServerHostname = strdup(host.c_str());
        m_rdpSettings->Username = strdup(user.c_str());
        if (!domain.empty()) {
            m_rdpSettings->Domain = strdup(domain.c_str());
        }
        if (!pass.empty()) {
            m_rdpSettings->Password = strdup(pass.c_str());
        } else {
            m_rdpSettings->Authentication = 0;
        }
        switch (params.perf) {
            case 0:
                // LAN
                m_rdpSettings->PerformanceFlags = PERF_FLAG_NONE;
                m_rdpSettings->ConnectionType = CONNECTION_TYPE_LAN;
                break;
            case 1:
                // Broadband
                m_rdpSettings->PerformanceFlags = PERF_DISABLE_WALLPAPER;
                m_rdpSettings->ConnectionType = CONNECTION_TYPE_BROADBAND_HIGH;
                break;
            case 2:
                // Modem
                m_rdpSettings->PerformanceFlags =
                    PERF_DISABLE_WALLPAPER | PERF_DISABLE_FULLWINDOWDRAG |
                    PERF_DISABLE_MENUANIMATIONS | PERF_DISABLE_THEMING;
                m_rdpSettings->ConnectionType = CONNECTION_TYPE_MODEM;
                break;
        }
        if (params.nowallp) {
            m_rdpSettings->DisableWallpaper = 1;
            m_rdpSettings->PerformanceFlags |= PERF_DISABLE_WALLPAPER;
        }
        if (params.nowdrag) {
            m_rdpSettings->DisableFullWindowDrag = 1;
            m_rdpSettings->PerformanceFlags |= PERF_DISABLE_FULLWINDOWDRAG;
        }
        if (params.nomani) {
            m_rdpSettings->DisableMenuAnims = 1;
            m_rdpSettings->PerformanceFlags |= PERF_DISABLE_MENUANIMATIONS;
        }
        if (params.notheme) {
            m_rdpSettings->DisableThemes = 1;
            m_rdpSettings->PerformanceFlags |= PERF_DISABLE_THEMING;
        }
        if (params.notls) {
            m_rdpSettings->TlsSecurity = 0;
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
                        log::info << "K" << ((m->down) ? "down" : "up") << ": c=" << m->code << endl;
                        uint32_t tcode = 0;
                        switch (m->code) {
                            case 8:
                                tcode = RDP_SCANCODE_BACKSPACE;
                                break;
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
                                tcode = RDP_SCANCODE_CAPSLOCK;
                                break;
                            case 93:
                                tcode = RDP_SCANCODE_LWIN; // Win-Key
                                break;
                            case 144:
                                // numlock
                                 tcode = RDP_SCANCODE_NUMLOCK;
                                break;
                            case 145:
                                // scrolllock
                                tcode = RDP_SCANCODE_SCROLLLOCK;
                                break;
                        }
                        if (0 < tcode) {

                        	log::info << "257 >> tcode: " << tcode << "\n";


                            uint32_t tflag = RDP_SCANCODE_EXTENDED(tcode) ? KBD_FLAGS_EXTENDED : 0;
                            tcode = RDP_SCANCODE_CODE(tcode);
                            //SendInputKeyboardEvent((m->down ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE)|tflag, tcode, TRUE);
                            //SendInputKeyboardEvent((m->down ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE)|tflag, tcode, FALSE);

                            //SendInputKeyboardEvent(KBD_FLAGS_DOWN|tflag, tcode, TRUE);
                            //SendInputKeyboardEvent(KBD_FLAGS_RELEASE|tflag, tcode, FALSE);

                            freerdp_input_send_keyboard_event(m_rdpInput, (m->down ? KBD_FLAGS_DOWN : KBD_FLAGS_RELEASE)|tflag, tcode);
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
                        log::info << "Kpress c=0x" << hex << m->code << ", ss=0x" << m->shiftstate << dec << endl;
                        if (0x20 < m->code) {
                            log::debug << "Kp1" << endl;
                            if (m->shiftstate & 6) {
                                // Control and or Alt: Must use scan-codes since unicode-event can't handle these
                                if (((64 < tcode) && (91 > tcode)) || ((96 < tcode) && (123 > tcode))) {
                                    tcode -= (m->shiftstate & 1) ? 0 : 32;

                                	log::info << "shiftstate: " << m->shiftstate << endl;

                                    tcode = freerdp_keyboard_get_rdp_scancode_from_virtual_key_code(tcode);
                                    //tcode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(tcode);

                                    log::info << "Kpress oc=" << tcode << endl;
                                    if (0 < tcode) {
                                    	log::info << "282\n";
                                        //SendInputKeyboardEvent(KBD_FLAGS_DOWN, tcode, true);
                                    	SendInputUnicodeKeyboardEvent(KBD_FLAGS_DOWN, ASCII_TO_SCANCODE[tcode]);
                                        //SendInputKeyboardEvent(KBD_FLAGS_RELEASE, tcode, false);
                                    	SendInputUnicodeKeyboardEvent(KBD_FLAGS_RELEASE, ASCII_TO_SCANCODE[tcode]);
                                    }
                                }
                            } else {
                                if (0 < tcode) {
                                	//uint32_t tflag = RDP_SCANCODE_EXTENDED(tcode) ? KBD_FLAGS_EXTENDED : 0;
                                	//tcode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(tcode);
                                	//tcode = RDP_SCANCODE_CODE(tcode);
                                    //tcode = 48;

                                	//SendInputKeyboardEvent(KBD_FLAGS_DOWN|tflag, tcode, true);
                                	//SendInputKeyboardEvent(KBD_FLAGS_RELEASE|tflag, tcode, false);
                                    //SendInputUnicodeKeyboardEvent(KBD_FLAGS_DOWN, tcode);
                                    //SendInputUnicodeKeyboardEvent(KBD_FLAGS_RELEASE, tcode);

                                	log::info << "308 :" << " " << KBD_FLAGS_DOWN << " " << KBD_FLAGS_RELEASE << "\n";
                                	log::info << "tcode: " << tcode << " m->code: " << m->code << endl;
                                	//freerdp_input_send_unicode_keyboard_event(m_rdpInput, KBD_FLAGS_DOWN|tflag, tcode);
                                	//working on w8:
                                	//freerdp_input_send_unicode_keyboard_event(m_rdpInput, KBD_FLAGS_DOWN, tcode);
                                	//working on w8:
                                	//freerdp_input_send_unicode_keyboard_event(m_rdpInput, KBD_FLAGS_RELEASE, tcode);

                                    //freerdp_input_send_unicode_keyboard_event(m_rdpInput, KBD_FLAGS_DOWN|tflag, tcode);
                                    //freerdp_input_send_unicode_keyboard_event(m_rdpInput, KBD_FLAGS_RELEASE|tflag, tcode);

                                	freerdp_input_send_keyboard_event(m_rdpInput, KBD_FLAGS_DOWN, ASCII_TO_SCANCODE[tcode]);
                                    freerdp_input_send_keyboard_event(m_rdpInput, KBD_FLAGS_RELEASE, ASCII_TO_SCANCODE[tcode]);

                                }
                            }
                        } else {
                            log::info << "Kp2" << " tcode: " << tcode << endl;
                            
                            //Work in progress: Switching all the keys to their ASCII based value

                            /*switch (tcode) {
                                case 0x21:
                                    tcode = RDP_SCANCODE_PRIOR; // Page up
                                    break;
                                case 0x22:
                                    tcode = RDP_SCANCODE_NEXT; // Page down
                                    break;
                                case 0x23:
                                    tcode = RDP_SCANCODE_END;
                                    break;
                                case 0x24:
                                    tcode = RDP_SCANCODE_HOME;
                                    break;
                                case 0x25:
                                    tcode = RDP_SCANCODE_LEFT;
                                    break;
                                case 0x26:
                                    tcode = RDP_SCANCODE_UP;
                                    break;
                                case 0x27:
                                    tcode = RDP_SCANCODE_RIGHT;
                                    break;
                                case 0x28:
                                    tcode = RDP_SCANCODE_DOWN;
                                    break;
                                case 0x2C:
                                    tcode = RDP_SCANCODE_PRINTSCREEN;
                                    break;
                                case 0x2D:
                                    tcode = RDP_SCANCODE_INSERT;
                                    break;
                                case 0x2E:
                                    tcode = RDP_SCANCODE_DELETE;
                                    break;
                            }
                            */
                            if (0 < tcode) {
                            	//tflag initially uint32_t
                            	uint32_t tflag = RDP_SCANCODE_EXTENDED(tcode) ? KBD_FLAGS_EXTENDED : 0;
                                tcode = RDP_SCANCODE_CODE(tcode);

                                log::info << "353 tcode: " << tcode <<" & tflag: " << tflag << "\n";
                                //SendInputUnicodeKeyboardEvent(KBD_FLAGS_DOWN, tcode);
                                //SendInputUnicodeKeyboardEvent(KBD_FLAGS_RELEASE, tcode);

                                //SendInputKeyboardEvent(KBD_FLAGS_DOWN|tflag, tcode, true);

                                freerdp_input_send_keyboard_event(m_rdpInput, KBD_FLAGS_DOWN, ASCII_TO_SCANCODE[tcode]);
                                freerdp_input_send_keyboard_event(m_rdpInput, KBD_FLAGS_RELEASE, ASCII_TO_SCANCODE[tcode]);
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

    void RDP::SendInputKeyboardEvent(uint32_t flags, uint32_t code, BOOL down)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        //freerdp_input_send_keyboard_event(m_rdpInput, flags, code);
        log::info << "377\n";
        //freerdp_input_send_keyboard_event_ex(m_rdpInput, down, code);
        //freerdp_input_send_keyboard_event_ex(m_rdpInput, down, code);

        if(code == RDP_SCANCODE_UNKNOWN)
        {
        	log::info << "Unknown key with keycode: " << code << "\n";
        	//freerdp_input_send_keyboard_event_ex(m_rdpInput, down, code);
        }

        else
        {
        	freerdp_input_send_keyboard_event_ex(m_rdpInput, down, code);
        	//freerdp_input_send_unicode_keyboard_event(m_rdpInput, flags, code);
        	//freerdp_input_send_keyboard_event_ex(m_rdpInput, down, code);
        }



    }

    void RDP::SendInputUnicodeKeyboardEvent(uint32_t flags, uint32_t code)
    {
        if (!m_rdpInput) {
            throw tracing::runtime_error("m_rdpInput is NULL");
        }
        log::info << "387\n";
        //freerdp_input_send_keyboard_event(m_rdpInput, flags, code);
        //removed the send_unicode_keyboard function
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

    RDP::cursor RDP::GetCursor(uint32_t cid)
    {
        CursorMap::iterator it = m_cursorMap.find(cid);
        if (it != m_cursorMap.end()) {
            return it->second;
        }
        return cursor(0, string());
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
    void RDP::ContextFree(freerdp *, rdpContext *ctx)
    {
        log::debug << "RDP::ContextFree" << endl;
        if (NULL != ctx->cache) {
            cache_free(ctx->cache);
            ctx->cache = NULL;
        }
        wsgContext *wctx = reinterpret_cast<wsgContext *>(ctx);
        if (NULL != wctx->clrconv) {
            freerdp_clrconv_free(wctx->clrconv);
            wctx->clrconv = NULL;
        }
    }

    // private
    BOOL RDP::PreConnect(freerdp *rdp)
    {
        //log::info << "RDP::PreConnect" << endl;
        m_pUpdate->Register(rdp);
        m_pPrimary->Register(rdp);

        // Settings for RFX:
#if 0
        m_rdpSettings->RemoteFxCodec = 1;
        m_rdpSettings->FastPathOutput = 1;
        m_rdpSettings->ColorDepth = 32;
        m_rdpSettings->FrameAcknowledge = 0;
        m_rdpSettings->LargePointerFlag = true;
#endif

        // ? settings->AllowDesktopComposition = 0;

        m_rdpSettings->RemoteFxCodec = 0;
        m_rdpSettings->FastPathOutput = 1;
        m_rdpSettings->ColorDepth = 16;
        m_rdpSettings->FrameAcknowledge = 1;
        m_rdpSettings->LargePointerFlag = 1;
        m_rdpSettings->BitmapCacheV3Enabled = 0;
        m_rdpSettings->BitmapCachePersistEnabled = 0;

        m_rdpSettings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_PATBLT_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_MULTIPATBLT_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_MULTISCRBLT_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_LINETO_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_POLYLINE_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_MEMBLT_INDEX] = FALSE;

        m_rdpSettings->OrderSupport[NEG_MEM3BLT_INDEX] = FALSE;

        m_rdpSettings->OrderSupport[NEG_MEMBLT_V2_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_SAVEBITMAP_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_FAST_INDEX_INDEX] = TRUE;
        m_rdpSettings->OrderSupport[NEG_FAST_GLYPH_INDEX] = TRUE;

        m_rdpSettings->OrderSupport[NEG_POLYGON_SC_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_POLYGON_CB_INDEX] = FALSE;

        m_rdpSettings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = FALSE;
        m_rdpSettings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = FALSE;

        m_rdpSettings->GlyphSupportLevel = GLYPH_SUPPORT_NONE;

        reinterpret_cast<wsgContext*>(m_freerdp->context)->clrconv =
            freerdp_clrconv_new(CLRCONV_ALPHA|CLRCONV_INVERT);

        m_freerdp->context->cache = cache_new(m_freerdp->settings);

        return TRUE;

    }

    // private
    BOOL RDP::PostConnect(freerdp *rdp)
    {
        //log::info << "RDP::PostConnect 0x" << hex << rdp << dec << endl;
        //gdi_init(rdp, CLRCONV_ALPHA | CLRCONV_INVERT | CLRBUF_16BPP | CLRBUF_32BPP, NULL);

        ostringstream oss;
        oss << "S:" << hex << this;
        m_wshandler->send_text(oss.str());
        rdpPointer p; 
        memset(&p, 0, sizeof(p));
        p.size = sizeof(MyPointer);
        p.New = cbPointer_New;
        p.Free = cbPointer_Free;
        p.Set = cbPointer_Set;
        p.SetNull = cbPointer_SetNull;
        p.SetDefault = cbPointer_SetDefault;
        graphics_register_pointer(rdp->context->graphics, &p);
        pointer_cache_register_callbacks(rdp->update);

        //log::info << "Now returning from postconnect;" << endl;

        return true;
    }

    // private
    BOOL RDP::Authenticate(freerdp *, char**, char**, char**)
    {
        log::debug << "RDP::Authenticate" << endl;
        return true;
    }

    // private
    BOOL RDP::VerifyCertificate(freerdp *, char*, char*, char*)
    {
        log::debug << "RDP::VerifyCertificate" << endl;
        return true;
    }

    // private
    void RDP::Pointer_New(rdpContext* context, rdpPointer* pointer)
    {
        // log::debug << __PRETTY_FUNCTION__ << endl;
#ifdef DBGLOG_POINTER_NEW
        log::debug << "PN id=" << m_ptrId
            << " w=" << pointer->width << " h=" << pointer->height
            << " hx=" << pointer->xPos << " hy=" << pointer->yPos << endl;
#endif
        HCLRCONV hclrconv = reinterpret_cast<wsgContext *>(context)->clrconv;
        size_t psize = pointer->width * pointer->height * 4;

        MyPointer *p = reinterpret_cast<MyPointer *>(pointer);
        p->id = m_ptrId++;
        uint8_t *pixels = new uint8_t[psize];
        memset(pixels, 0, psize);
        if ((pointer->andMaskData != 0) && (pointer->xorMaskData != 0)) {
            freerdp_alpha_cursor_convert(pixels,
                    pointer->xorMaskData, pointer->andMaskData,
                    pointer->width, pointer->height, pointer->xorBpp, hclrconv);
        }
        Png png;
        m_cursorMap[p->id] =
            cursor(time(NULL), png.GenerateFromARGB(pointer->width, pointer->height, pixels));
        delete []pixels;
        struct {
            uint32_t op;
            uint32_t id;
            uint32_t hx;
            uint32_t hy;
        } tmp = {
            WSOP_SC_PTR_NEW,
            p->id,
            pointer->xPos,
            pointer->yPos
        };
        string buf(reinterpret_cast<const char *>(&tmp), sizeof(tmp));
        m_wshandler->send_binary(buf);
    }

    // private
    void RDP::Pointer_Free(rdpContext*, rdpPointer* pointer)
    {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        MyPointer *p = reinterpret_cast<MyPointer *>(pointer);
#ifdef DBGLOG_POINTER_FREE
        if (p->id) {
            log::debug << "PF " << p->id << endl;
        } else {
            log::debug << "PF ???" << endl;
        }
#endif
        if (p->id) {
            struct {
                uint32_t op;
                uint32_t id;
            } tmp = {
                WSOP_SC_PTR_FREE,
                p->id
            };
            m_cursorMap.erase(p->id);
            p->id = 0;
            string buf(reinterpret_cast<const char *>(&tmp), sizeof(tmp));
            m_wshandler->send_binary(buf);
        }
    }

    // private
    void RDP::Pointer_Set(rdpContext*, rdpPointer* pointer)
    {
        // log::debug << __PRETTY_FUNCTION__ << endl;
        MyPointer *p = reinterpret_cast<MyPointer *>(pointer);
#ifdef DBGLOG_POINTER_SET
        log::debug << "PS " << p->id << endl;
#endif
        struct {
            uint32_t op;
            uint32_t id;
        } tmp = {
            WSOP_SC_PTR_SET,
            p->id
        };
        string buf(reinterpret_cast<const char *>(&tmp), sizeof(tmp));
        m_wshandler->send_binary(buf);
    }

    // private
    void RDP::Pointer_SetNull(rdpContext*)
    {
        // log::debug << __PRETTY_FUNCTION__ << endl;
#ifdef DBGLOG_POINTER_SETNULL
        log::debug << "PN" << endl;
#endif
        uint32_t op = WSOP_SC_PTR_SETNULL;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
    }

    // private
    void RDP::Pointer_SetDefault(rdpContext*)
    {
        // log::debug << __PRETTY_FUNCTION__ << endl;
#ifdef DBGLOG_POINTER_SETDEFAULT
        log::debug << "PD" << endl;
#endif
        uint32_t op = WSOP_SC_PTR_SETDEFAULT;
        string buf(reinterpret_cast<const char *>(&op), sizeof(op));
        m_wshandler->send_binary(buf);
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
                        case 2:
                        case 7:
                        case 9:
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
                    //if (freerdp_connect(m_freerdp)) {
                	if(freerdp_connect(m_freerdp)) {
                        m_State = STATE_CONNECTED;
                        continue;
                    }
                    m_State = STATE_INITIAL;
                    //log::info << "line 720" << endl;
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
    BOOL RDP::cbPreConnect(freerdp *inst)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->PreConnect(inst);
        }
        return false;
    }

    // private C callback
    BOOL RDP::cbPostConnect(freerdp *inst)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->PostConnect(inst);
        }
        return false;
    }

    // private C callback
    BOOL RDP::cbAuthenticate(freerdp *inst, char** user, char** pass, char** domain)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->Authenticate(inst, user, pass, domain);
        }
        return false;
    }

    // private C callback
    BOOL RDP::cbVerifyCertificate(freerdp *inst, char* subject, char* issuer, char* fprint)
    {
        RDP *self = reinterpret_cast<wsgContext *>(inst->context)->pRDP;
        if (self) {
            return self->VerifyCertificate(inst, subject, issuer, fprint);
        }
        return false;
    }

    // private C callback
    void RDP::cbPointer_New(rdpContext* context, rdpPointer* pointer)
    {
        RDP *self = reinterpret_cast<wsgContext *>(context)->pRDP;
        if (self) {
            self->Pointer_New(context, pointer);
        }
    }

    // private C callback
    void RDP::cbPointer_Free(rdpContext* context, rdpPointer* pointer)
    {
        RDP *self = reinterpret_cast<wsgContext *>(context)->pRDP;
        if (self) {
            self->Pointer_Free(context, pointer);
        }
    }

    // private C callback
    void RDP::cbPointer_Set(rdpContext* context, rdpPointer* pointer)
    {
        RDP *self = reinterpret_cast<wsgContext *>(context)->pRDP;
        if (self) {
            self->Pointer_Set(context, pointer);
        }
    }

    // private C callback
    void RDP::cbPointer_SetNull(rdpContext* context)
    {
        RDP *self = reinterpret_cast<wsgContext *>(context)->pRDP;
        if (self) {
            self->Pointer_SetNull(context);
        }
    }

    // private C callback
    void RDP::cbPointer_SetDefault(rdpContext* context)
    {
        RDP *self = reinterpret_cast<wsgContext *>(context)->pRDP;
        if (self) {
            self->Pointer_SetDefault(context);
        }
    }

}
