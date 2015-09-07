#ifndef _WS_GATE_SERVICE_
#define _WS_GATE_SERVICE_

#include "NTService.hpp"

namespace wsgate{

    class WsGateService : public NTService {

        public:
            WsGateService();
            bool ParseSpecialArgs(int argc, char **argv);

        protected:
            bool OnServiceStop();
            bool OnServiceShutdown();
            void RunService();    
    };
}

#endif //_WS_GATE_SERVICE_