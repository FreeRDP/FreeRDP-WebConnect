#ifndef _MYRAWSOCKET_H_
#define _MYRAWSOCKET_H_

#include "RDP.hpp"

namespace wsgate {

    class WsGate;
    class RDP;

    /// A shared pointer to our server-side WebSocket connection endpoint.
    typedef boost::shared_ptr<wspp::wsendpoint> conn_ptr;
    /// A shared pointer to our server-side WebSocket event handler.
    typedef boost::shared_ptr<wspp::wshandler> handler_ptr;
    /// A shared pointer to our server-side RDP client instance.
    typedef boost::shared_ptr<RDP> rdp_ptr;
    /// Combinded tuple of involved instances of an RDP session
    typedef boost::tuple<conn_ptr, handler_ptr, rdp_ptr> conn_tuple;
    /// A map for finding our session related instances by EHSConnection
    typedef std::map<EHSConnection *, conn_tuple> conn_map;

    /**
     * This class is our specialization of RawSocketHandler which
     * handles all WebSocket I/O events.
     */
    class MyRawSocketHandler : public RawSocketHandler
    {
        public:
            /**
             * Constructor
             * @param parent The corresponding WsGate instance which creted this instance.
             */
            MyRawSocketHandler(WsGate *parent);

            /**
             * @see RawSocketHandler::OnData
             */
            virtual bool OnData(EHSConnection *conn, std::string data);

            /**
             * @see RawSocketHandler::OnConnect
             */
            virtual void OnConnect(EHSConnection *conn);

            /**
             * @see RawSocketHandler::OnDisconnect
             */
            virtual void OnDisconnect(EHSConnection *conn);

            /**
             * Creates an RDP session and instantiates the relevant
             * handler classes.
             * @param conn The EHSConnection which triggered thsi action.
             * @param mode The endpoint operation mode.
             * @param host The RDP host to connect to
             * @param port The RDP port to connect to
             * @param user The user name to be used for the RDP session.
             * @param pass The password to be used for the RDP session.
             * @param width The desktop width to be used for the RDP session.
             * @param height The desktop height to be used for the RDP session.
             */
            bool Prepare(EHSConnection *conn, const std::string mode,
                    const std::string host, uint16_t port,
                    const std::string user, const std::string pass,
                    const int &width, const int &height);

            /**
             * Event handler for WebSocket message events.
             * Gets invoked from the WebSockets codec whenever a message is
             * received from the client.
             * @param conn The EHSConnection which received this message.
             * @param data The payload of the message.
             */
            void OnMessage(EHSConnection *conn, const std::string & data);

        private:
            MyRawSocketHandler(const MyRawSocketHandler&);
            MyRawSocketHandler& operator=(const MyRawSocketHandler&);

            WsGate *m_parent;
            conn_map m_cmap;
    };

}

#endif
