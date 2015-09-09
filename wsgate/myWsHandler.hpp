#ifndef _MY_WS_HANDLER_
#define _MY_WS_HANDLER_

#include "wshandler.hpp"
#include "myrawsocket.hpp"

namespace wsgate{
    class MyWsHandler : public wspp::wshandler
    {
        public:
            MyWsHandler(EHSConnection *econn, EHS *ehs, MyRawSocketHandler *rsh);
            virtual void on_message(std::string hdr, std::string data);
            virtual void on_close();
            virtual bool on_ping(const std::string & data);
            virtual void on_pong(const std::string & data);
            virtual void do_response(const std::string & data);
        private:
            // Non-copyable
            MyWsHandler(const MyWsHandler&);
            MyWsHandler& operator=(const MyWsHandler&);

            EHSConnection *m_econn;
            EHS *m_ehs;
            MyRawSocketHandler *m_rsh;
    };
}

#endif //_MY_WS_HANDLER_