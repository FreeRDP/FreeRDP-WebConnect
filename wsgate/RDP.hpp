#ifndef _WSGATE_RDP_H_
#define _WSGATE_RDP_H_

#include <pthread.h>
#include <map>
#include <string>
#include "wshandler.hpp"

typedef struct rdp_freerdp freerdp;
typedef struct rdp_context rdpContext;
typedef struct rdp_input rdpInput;
typedef struct rdp_settings rdpSettings;

namespace wsgate {

    class RDP {

        public:
            typedef enum {
                STATE_INITIAL,
                STATE_CONNECT,
                STATE_CONNECTED,
                STATE_CLOSED
            } State;

            RDP(wspp::wshandler *h);
            virtual ~RDP();

            void SetError(std::string msg);
            bool Connect(std::string host, int port, std::string user, std::string domain,
                    std::string pass);
            bool Disconnect();
            bool CheckFileDescriptor();
            void SendInputSynchronizeEvent(uint32_t flags);
            void SendInputKeyboardEvent(uint16_t flags, uint16_t code);
            void SendInputUnicodeKeyboardEvent(uint16_t flags, uint16_t code);
            void SendInputMouseEvent(uint16_t flags, uint16_t x, uint16_t y);
            void SendInputExtendedMouseEvent(uint16_t flags, uint16_t x, uint16_t y);

        private:
            // Non-copyable
            RDP(const RDP &);
            RDP & operator=(const RDP &);

            void ThreadFunc();

            void ContextNew(freerdp *inst, rdpContext *ctx);
            void ContextFree(freerdp *inst, rdpContext *ctx);
            int PreConnect(freerdp *inst);
            int PostConnect(freerdp *inst);
            int Authenticate(freerdp *inst, char** user, char** pass, char** domain);
            int VerifyCertificate(freerdp *inst, char* subject, char* issuer, char* fprint);
            int SendChannelData(freerdp *inst, int chId, uint8_t* data, int size);
            int ReceiveChannelData(freerdp* inst, int chId, uint8_t* data, int size,
                    int flags, int total_size);

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

            // Callbacks from C pthreads - Must be static in order t be assigned to C fnPtrs.
            static void *cbThreadFunc(void *ctx);
            // Callbacks from C - Must be static in order t be assigned to C fnPtrs.
            static void cbContextNew(freerdp *inst, rdpContext *ctx);
            static void cbContextFree(freerdp *inst, rdpContext *ctx);
            static int cbPreConnect(freerdp *inst);
            static int cbPostConnect(freerdp *inst);
            static int cbAuthenticate(freerdp *inst, char** user, char** pass, char** domain);
            static int cbVerifyCertificate(freerdp *inst, char* subject, char* issuer,
                    char* fprint);
            static int cbSendChannelData(freerdp *inst, int chId, uint8_t* data, int size);
            static int cbReceiveChannelData(freerdp* inst, int chId, uint8_t* data, int size,
                    int flags, int total_size);
    };
}

#endif
