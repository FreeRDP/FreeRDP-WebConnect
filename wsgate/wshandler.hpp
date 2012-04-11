#ifndef WSHANDLER_H
#define WSHANDLER_H

#include <string>
#include "wsgate.h"

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
            void send_text(const std::string data);

            /**
             * Send a binary message to the remote client.
             * @param data The payload to send.
             */
            void send_binary(const std::string data);

            /// Constructor
            wshandler() : m_endpoint(0) {}

            /// Destructor
            virtual ~wshandler() {}

        private:
            virtual void on_message(std::string header, std::string data) = 0;
            virtual void on_close() = 0;
            virtual bool on_ping(const std::string data) = 0;
            virtual void on_pong(const std::string data) = 0;
            virtual void do_response(const std::string data) = 0;

            // void send(const std::string& payload, frame::opcode::value op);

            wshandler(const wspp::wshandler&);
            wshandler& operator=(const wspp::wshandler&);

            wsendpoint *m_endpoint;
            friend class wsendpoint;
    };

}

#endif
