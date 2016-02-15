#ifndef _WS_GATE_SERVICE_
#define _WS_GATE_SERVICE_

#ifdef _WIN32

#include "NTService.hpp"

namespace wsgate{

    class WsGateService : public NTService {
        public:
            WsGateService();
            bool ParseSpecialArgs(int argc, char **argv);

            static bool g_signaled;
        protected:
            bool OnServiceStop();
            bool OnServiceShutdown();
            void RunService();    
    };
}

#endif //_WIN32

#endif //_WS_GATE_SERVICE_
