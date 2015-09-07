/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 * This file has been partially derived from the WebSockets++ project at
 * https://github.com/zaphoyd/websocketpp which is licensed under a BSD-license.
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

#ifndef WSENDPOINT_H
#define WSENDPOINT_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include "wsframe.hpp"
#include "wsgate.hpp"
#include "wshandler.hpp"

#ifndef HAVE_BOOST_LOCK_GUARD
#include <pthread.h>

/**
 * Automatically unlocks a mutex if destroyed.
 */
class MutexHelper {
    public:
        /**
         * Constructs a new instance.
         * The associated mutex is locked by default.
         * @param mutex The mutex to use.
         * @param locknow false, if the mutex should not be locked initially
         */
        MutexHelper(pthread_mutex_t *mutex, bool locknow = true);

        /**
         * Unlocks the associated mutex.
         */
        ~MutexHelper();

        /**
         * Locks the associated mutex.
         */
        void Lock();

        /**
         * Unlocks the associated mutex.
         */
        void Unlock();
    private:
        pthread_mutex_t *m_pMutex;
        bool m_bLocked;

        MutexHelper(const MutexHelper &);
        MutexHelper & operator=(const MutexHelper &);
};
#endif

namespace wspp {

    using wsgate::log;

    /**
     * This class implements a server-side WebSockets endpoint.
     */
    class wsendpoint {
        private:
            // Non-copyable
            wsendpoint(const wspp::wsendpoint&);
            wsendpoint& operator=(const wspp::wsendpoint&);

        public:
            /**
             * Constructor
             * @param h The corresponding wshandler instance.
             */
            wsendpoint(wshandler *h);

#ifndef HAVE_BOOST_LOCK_GUARD
            ~wsendpoint();
#endif

            /**
             * Processes incoming data from the client.
             * The incoming data is decoded, according to RFC6455.
             * If any message is completely assembled, the on_message
             * method of the corresponding wshandler is invoked. For
             * internal replys (e.g. PONG responses) the do_response
             * method of the corresponding wshandler is used. All other
             * on_xxx methods are called when the corresponding events occur.
             *
             * @param data the raw data, received from the client.
             */
            void AddRxData(std::string data);

            /**
             * Send a data message.
             * This method is invoked from the corresponding wshandler
             * in order to send TEXT and BINARY payloads.
             * @param payload The payload data.
             * @param op The opcode according to RFC6455
             */
            void send(const std::string& payload, frame::opcode::value op);

        private:
            void process_data();

            /// Ends the connection by cleaning up based on current state
            /** Terminate will review the outstanding resources and close each
             * appropriately. Attached handlers will recieve an on_fail or on_close call
             * 
             * TODO: should we protect against long running handlers?
             * 
             * Visibility: protected
             * State: Valid from any state except CLOSED.
             * Concurrency: Must be called from within m_strand
             *
             */
            void shutdown();

            /**
             * Send Pong
             * Initiates a pong with the given payload.
             * 
             * There is no feedback from pong.
             * 
             * Visibility: public
             * State: Valid from OPEN, ignored otherwise
             * Concurrency: callable from any thread
             *
             * @param payload Payload to be used for the pong
             */
            void pong(const std::vector<unsigned char> & payload);

            /// Send a close frame
            /**
             * Initiates a close handshake by sending a close frame with the given code 
             * and reason. 
             * 
             * Visibility: protected
             * State: Valid for OPEN, ignored otherwise.
             * Concurrency: Must be called within m_strand
             *
             * @param code The code to send
             * @param reason The reason to send
             */
            void send_close(close::status::value code, const std::string& reason);

            /// send an acknowledgement close frame
            /** 
             * Visibility: private
             * State: no state checking, should only be called within process_control
             * Concurrency: Must be called within m_stranded method
             */
            void send_close_ack(close::status::value remote_close_code, std::string remote_close_reason);

            void process_control();
        private:
            simple_rng m_rng;
            frame::parser<simple_rng> m_parser;
            session::state::value m_state;
#ifdef HAVE_BOOST_LOCK_GUARD
            mutable boost::recursive_mutex m_lock;
#else
            mutable pthread_mutex_t m_lock;
#endif
            wshandler *m_handler;
    };

    

}

#endif
