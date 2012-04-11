#ifndef _MYRAWSOCKET_H_
#define _MYRAWSOCKET_H_

namespace wsgate {

    class WsGate;

    typedef boost::shared_ptr<wspp::wsendpoint> conn_ptr;
    typedef boost::shared_ptr<wspp::wshandler> handler_ptr;
    typedef boost::tuple<conn_ptr, handler_ptr, std::string, std::string, std::string> conn_tuple;
    typedef std::map<EHSConnection *, conn_tuple> conn_map;

    class MyRawSocketHandler : public RawSocketHandler
    {
        public:
            MyRawSocketHandler(WsGate *parent);

            virtual bool OnData(EHSConnection *conn, std::string data);

            virtual void OnConnect(EHSConnection *conn);

            virtual void OnDisconnect(EHSConnection *conn);

            void Prepare(EHSConnection *conn, const std::string mode,
                    const std::string rdphost, const std::string rdpuser,
                    const std::string rdppass);

        private:
            MyRawSocketHandler(const MyRawSocketHandler&);
            MyRawSocketHandler& operator=(const MyRawSocketHandler&);

            WsGate *m_parent;
            conn_map m_cmap;
    };

}

#endif
