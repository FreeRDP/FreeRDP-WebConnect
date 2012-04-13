#ifndef _MYRAWSOCKET_H_
#define _MYRAWSOCKET_H_

#include "RDP.hpp"

namespace wsgate {

    class WsGate;
    class RDP;

    typedef boost::shared_ptr<wspp::wsendpoint> conn_ptr;
    typedef boost::shared_ptr<wspp::wshandler> handler_ptr;
    typedef boost::shared_ptr<RDP> rdp_ptr;
    typedef boost::tuple<conn_ptr, handler_ptr, rdp_ptr> conn_tuple;
    typedef std::map<EHSConnection *, conn_tuple> conn_map;

    class MyRawSocketHandler : public RawSocketHandler
    {
        public:
            MyRawSocketHandler(WsGate *parent);

            virtual bool OnData(EHSConnection *conn, std::string data);

            virtual void OnConnect(EHSConnection *conn);

            virtual void OnDisconnect(EHSConnection *conn);

            bool Prepare(EHSConnection *conn, const std::string mode,
                    const std::string host, uint16_t port,
                    const std::string user, const std::string pass);

            void OnMessage(EHSConnection *conn, const std::string & data);

        private:
            MyRawSocketHandler(const MyRawSocketHandler&);
            MyRawSocketHandler& operator=(const MyRawSocketHandler&);

            WsGate *m_parent;
            conn_map m_cmap;
    };

}

#endif
