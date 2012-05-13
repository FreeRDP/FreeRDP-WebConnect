#ifndef _WSGATE_RDP_H_
#define _WSGATE_RDP_H_

#include <pthread.h>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

#include "rdpcommon.hpp"

namespace wsgate {

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

            typedef std::map<uint32_t, std::string> CursorMap;

            /**
             * Constructor
             * @param h The WebSockets handler to be used for communication with the client.
             */
            RDP(wspp::wshandler *h);
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
            bool Connect(std::string host, std::string user, std::string domain,
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
             * Retrieves a custom cursor image by ID.
             * @param cid Unique cursor ID (valid for current session).
             * @return A PNG-encoded RGBA image or an empty String, if no cursor image is defined for the given Id.
             */
            std::string GetCursorPng(uint32_t cid);

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
            void SendInputKeyboardEvent(uint16_t flags, uint16_t code);
            /**
             * Wraps the corresponding FreeRDP API call.
             * @param flags The flags as defined by the FreeRDP API.
             * @param code The character code as defined by the FreeRDP API.
             */
            void SendInputUnicodeKeyboardEvent(uint16_t flags, uint16_t code);
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

            void ContextNew(freerdp *inst, rdpContext *ctx);
            void ContextFree(freerdp *inst, rdpContext *ctx);
            boolean PreConnect(freerdp *inst);
            boolean PostConnect(freerdp *inst);
            boolean Authenticate(freerdp *inst, char** user, char** pass, char** domain);
            boolean VerifyCertificate(freerdp *inst, char* subject, char* issuer, char* fprint);
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
            std::string m_errMsg;
            State m_State;
            Update *m_pUpdate;
            Primary *m_pPrimary;
            uint32_t m_lastError;
            uint32_t m_ptrId;
            CursorMap m_cursorMap;

            // Callbacks from C pthreads - Must be static in order t be assigned to C fnPtrs.
            static void *cbThreadFunc(void *ctx);
            // Callbacks from C - Must be static in order t be assigned to C fnPtrs.
            static void cbContextNew(freerdp *inst, rdpContext *ctx);
            static void cbContextFree(freerdp *inst, rdpContext *ctx);
            static boolean cbPreConnect(freerdp *inst);
            static boolean cbPostConnect(freerdp *inst);
            static boolean cbAuthenticate(freerdp *inst, char** user, char** pass, char** domain);
            static boolean cbVerifyCertificate(freerdp *inst, char* subject, char* issuer,
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
