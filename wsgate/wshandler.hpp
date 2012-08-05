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
#ifndef WSHANDLER_H
#define WSHANDLER_H

#include <string>
#include "wsgate.hpp"

namespace wspp {
    class wsendpoint;

    using wsgate::log;

    /**
     * Event handler interface for the server-side
     * WebSockets endpoint.
     *
     * This class is used as the API for working with a server-side
     * WebSockets endpoint. An application has to derive a specific
     * implementation from this class, implementing the various pure
     * virtual methods.
     */
    class wshandler {
        public:
            /**
             * Send a text message to the remote client.
             * @param data The payload to send.
             */
            void send_text(const std::string & data);

            /**
             * Send a binary message to the remote client.
             * @param data The payload to send.
             */
            void send_binary(const std::string & data);

            /// Constructor
            wshandler() : m_endpoint(0) {}

            /// Destructor
            virtual ~wshandler() {}

        private:
            virtual void on_message(std::string header, std::string data) = 0;
            virtual void on_close() = 0;
            virtual bool on_ping(const std::string & data) = 0;
            virtual void on_pong(const std::string & data) = 0;
            virtual void do_response(const std::string & data) = 0;

            // void send(const std::string& payload, frame::opcode::value op);

            wshandler(const wspp::wshandler&);
            wshandler& operator=(const wspp::wshandler&);

            wsendpoint *m_endpoint;
            friend class wsendpoint;
    };

}

#endif
