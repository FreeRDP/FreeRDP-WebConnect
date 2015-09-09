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
#ifndef _MYRAWSOCKET_H_
#define _MYRAWSOCKET_H_

#include "RDP.hpp"
#include <ehs/ehs.h>

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
             * Handle raw data.
             * Called by EHS, if an EHSConnection is in raw mode. 
             * @see RawSocketHandler::OnData
             * @param conn The EHSConnection on which the data was received.
             * @param data The received data.
             * @return true, if the connection should be kept open.
             */
            virtual bool OnData(EHSConnection *conn, std::string data);

            /**
             * Handle connect event.
             * Called by EHS, if an EHSConnection has switched into raw mode.
             * @see RawSocketHandler::OnConnect
             * @param conn The EHSConnection on which the event happened.
             */
            virtual void OnConnect(EHSConnection *conn);

            /**
             * Handle disconnect event.
             * Called by EHS, if an EHSConnection is about to be closed.
             * @see RawSocketHandler::OnDisconnect
             * @param conn The EHSConnection on which the event happened.
             */
            virtual void OnDisconnect(EHSConnection *conn);

            /**
             * Instantiates the relevant handler classes.
             * @param conn The EHSConnection which triggered thsi action.
             * @param host The RDP host to connect to
             * @param user The user name to be used for the RDP session.
             * @param pass The password to be used for the RDP session.
             * @param params Additional RDP parameters.
             * @param embeddedContext Tells the purpose of the connection
             * @return true on success.
             */
            bool Prepare(EHSConnection *conn, const std::string host, const std::string pcb,
                    const std::string user, const std::string pass,
                    const WsRdpParams &params, EmbeddedContext embeddedContext);
            /**
             * Creates an RDP session using parameters specified to wsgate::MyRawSocketHandler::Prepare
             */
            void PrepareRDP(const std::string host, const std::string pcb, const std::string user, const std::string pass, const WsRdpParams &params);

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
            EHSConnection *conn;
    };

}

#endif
