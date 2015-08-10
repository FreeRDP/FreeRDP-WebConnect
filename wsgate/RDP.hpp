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
#ifndef _WSGATE_RDP_H_
#define _WSGATE_RDP_H_

#include <pthread.h>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "rdpcommon.hpp"

namespace wsgate {

    /**
     * Possible embedded contexts
     */
    typedef enum {
        CONTEXT_PLAIN
       ,CONTEXT_EMBEDDED
    } EmbeddedContext;

    class MyRawSocketHandler;
    /**
     * This class serves as a wrapper around the
     * main FreeRDP API.
     */
    class RDP {

        public:
            /**
             * Our connection states.
             */
            typedef enum {
                STATE_INITIAL,
                STATE_CONNECT,
                STATE_CONNECTED,
                STATE_CLOSED
            } State;
            

            /// Map for storing cursor images of this session
            typedef boost::tuple<time_t, std::string> cursor;
            typedef std::map<uint32_t, cursor> CursorMap;

            /**
             * Constructor
             * @param h The WebSockets handler to be used for communication with the client.
             * @param rsh The Raw Socket handler that is used for starting the RDP session
             */
            RDP(wspp::wshandler *h, MyRawSocketHandler *rsh, EmbeddedContext embeddedContext = CONTEXT_PLAIN);
            /// Destructor
            virtual ~RDP();

            /**
             * Sets the error message for the last error.
             * @param msg The message.
             */
            void SetError(std::string msg);
            /**
             * Initiates the actual RDP session.
             * @param host The RDP host to connect to.
             * @param user The user name to be used for the RDP session.
             * @param domain The domain name to be used for the RDP session.
             * @param pass The password to be used for the RDP session.
             * @param params Additional parameters for the RDP session.
             * @return true on success.
             */
            bool Connect(std::string host, std::string pcb, std::string user, std::string domain,
                    std::string pass, const WsRdpParams &params);
            /**
             * Actively terminates a session.
             * @return true on success.
             */
            bool Disconnect();
            /**
             * Wraps the corresponding FreeRDP API call
             * @return true on success.
             */
            bool CheckFileDescriptor();
            /**
             * Handler for incoming WebSockets messages.
             * Called from the WebSockets codec, whenever the client sent a message.
             * @param data The binary payload of the incoming message.
             */
            void OnWsMessage(const std::string & data);
            /**
             * Retrieves a custom cursor tuple by ID.
             * @param cid Unique cursor ID (valid for current session).
             * @return A tuple, containing the creation time and PNG-encoded RGBA image or a tuple containing 0 and an empty String, if no cursor image is defined for the given Id.
             */
            cursor GetCursor(uint32_t cid);
            /**
             * Sets the context of the web-connect app
             * @param kind of ebdedded context
             */
            void setEmbeddedContext(EmbeddedContext embeddedContext){this->m_embeddedContext = embeddedContext;}
            /**
             * Gets the context of the web-connect app
             * @return the context
             */
            EmbeddedContext getEmbeddedContext(){return this->m_embeddedContext;}

        private:
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             */
            void SendInputSynchronizeEvent(uint32_t flags);
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             * @param code The scan code as defined by the FreeRDP API.
             */
            void SendInputKeyboardEvent(uint32_t flags, uint32_t code, BOOL down);
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             * @param code The character code as defined by the FreeRDP API.
             */
            void SendInputUnicodeKeyboardEvent(uint32_t flags, uint32_t code);
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             * @param x, y The mouse coordinates as defined by the FreeRDP API.
             */
            void SendInputMouseEvent(uint16_t flags, uint16_t x, uint16_t y);
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             * @param x, y The mouse coordinates as defined by the FreeRDP API.
             */
            void SendInputExtendedMouseEvent(uint16_t flags, uint16_t x, uint16_t y);

            // Non-copyable
            RDP(const RDP &);
            RDP & operator=(const RDP &);

            void ThreadFunc();
            void addError(const std::string &msg);

            int ContextNew(freerdp *inst, rdpContext *ctx);
            void ContextFree(freerdp *inst, rdpContext *ctx);
            BOOL PreConnect(freerdp *inst);
            BOOL PostConnect(freerdp *inst);
            BOOL Authenticate(freerdp *inst, char** user, char** pass, char** domain);
            BOOL VerifyCertificate(freerdp *inst, char* subject, char* issuer, char* fprint);
            int SendChannelData(freerdp *inst, int chId, uint8_t* data, int size);
            int ReceiveChannelData(freerdp* inst, int chId, uint8_t* data, int size,
                    int flags, int total_size);

            void Pointer_New(rdpContext* context, rdpPointer* pointer);
            void Pointer_Free(rdpContext* context, rdpPointer* pointer);
            void Pointer_Set(rdpContext* context, rdpPointer* pointer);
            void Pointer_SetNull(rdpContext* context);
            void Pointer_SetDefault(rdpContext* context);

            static std::map<freerdp *, RDP *> m_instances;
            freerdp *m_freerdp;
            rdpContext *m_rdpContext;
            rdpInput *m_rdpInput;
            rdpSettings *m_rdpSettings;
            bool m_bThreadLoop;
            pthread_t m_worker;
            wspp::wshandler *m_wshandler;
            MyRawSocketHandler *m_rsh;
            std::string m_errMsg;
            State m_State;
            Update *m_pUpdate;
            Primary *m_pPrimary;
            uint32_t m_lastError;
            uint32_t m_ptrId;
            CursorMap m_cursorMap;
            EmbeddedContext m_embeddedContext;

            // Callbacks from C pthreads - Must be static in order t be assigned to C fnPtrs.
            static void *cbThreadFunc(void *ctx);
            // Callbacks from C - Must be static in order t be assigned to C fnPtrs.
            static int cbContextNew(freerdp *inst, rdpContext *ctx);
            static void cbContextFree(freerdp *inst, rdpContext *ctx);
            static BOOL cbPreConnect(freerdp *inst);
            static BOOL cbPostConnect(freerdp *inst);
            static BOOL cbAuthenticate(freerdp *inst, char** user, char** pass, char** domain);
            static BOOL cbVerifyCertificate(freerdp *inst, char* subject, char* issuer,
                    char* fprint);
            static int cbSendChannelData(freerdp *inst, int chId, uint8_t* data, int size);
            static int cbReceiveChannelData(freerdp* inst, int chId, uint8_t* data, int size,
                    int flags, int total_size);

            static void cbPointer_New(rdpContext* context, rdpPointer* pointer);
            static void cbPointer_Free(rdpContext* context, rdpPointer* pointer);
            static void cbPointer_Set(rdpContext* context, rdpPointer* pointer);
            static void cbPointer_SetNull(rdpContext* context);
            static void cbPointer_SetDefault(rdpContext* context);
    };
}

#endif
