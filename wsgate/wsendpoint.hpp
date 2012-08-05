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
        MutexHelper(pthread_mutex_t *mutex, bool locknow = true) :
            m_pMutex(mutex), m_bLocked(false)
        {
            if (locknow)
                Lock();
        }

        /**
         * Unlocks the associated mutex.
         */
        ~MutexHelper()
        {
            if (m_bLocked)
                pthread_mutex_unlock(m_pMutex);
        }

        /**
         * Locks the associated mutex.
         */
        void Lock()
        {
            pthread_mutex_lock(m_pMutex);
            m_bLocked = true;
        }

        /**
         * Unlocks the associated mutex.
         */
        void Unlock()
        {
            m_bLocked = false;
            pthread_mutex_unlock(m_pMutex);
        }
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
            wsendpoint(wshandler *h)
                : m_rng(simple_rng())
                  , m_parser(frame::parser<simple_rng>(m_rng))
                  , m_state(session::state::OPEN)
                  , m_lock()
                  , m_handler(h)
        {
#ifndef HAVE_BOOST_LOCK_GUARD
            pthread_mutexattr_t mattr;
            pthread_mutexattr_init(&mattr);
            pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_lock, &mattr);
            pthread_mutexattr_destroy(&mattr);
#endif
            m_handler->m_endpoint = this;
        }

#ifndef HAVE_BOOST_LOCK_GUARD
            ~wsendpoint() { pthread_mutex_destroy(&m_lock); }
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
            void AddRxData(std::string data)
            {
                std::istringstream s(data);
                while (m_state != session::state::CLOSED && s.rdbuf()->in_avail()) {
                    try {
                        m_parser.consume(s);
                        if (m_parser.ready()) {
                            if (m_parser.is_control()) {
                                process_control();
                            } else {
                                process_data();
                            }                   
                            m_parser.reset();
                        }
                    } catch (const tracing::wserror & e) {
                        if (m_parser.ready()) {
                            m_parser.reset();
                        }
                        switch(e.code()) {
                            case tracing::wserror::PROTOCOL_VIOLATION:
                                send_close(close::status::PROTOCOL_ERROR,e.what());
                                break;
                            case tracing::wserror::PAYLOAD_VIOLATION:
                                send_close(close::status::INVALID_PAYLOAD,e.what());
                                break;
                            case tracing::wserror::INTERNAL_ENDPOINT_ERROR:
                                send_close(close::status::INTERNAL_ENDPOINT_ERROR,e.what());
                                break;
                            case tracing::wserror::SOFT_ERROR:
                                continue;
                            case tracing::wserror::MESSAGE_TOO_BIG:
                                send_close(close::status::MESSAGE_TOO_BIG,e.what());
                                break;
                            case tracing::wserror::OUT_OF_MESSAGES:
                                // we need to wait for a message to be returned by the
                                // client. We exit the read loop. handle_read_frame
                                // will be restarted by recycle()
                                //m_read_state = WAITING;
                                //m_endpoint.wait(type::shared_from_this());
                                return;
                            default:
                                // Fatal error, forcibly end connection immediately.
                                log::warn
                                    << "Dropping TCP due to unrecoverable exception: " << e.code()
                                    << " (" << e.what() << ")" << std::endl;
                                shutdown();
                        }
                        break;
                    }
                }
            }

            /**
             * Send a data message.
             * This method is invoked from the corresponding wshandler
             * in order to send TEXT and BINARY payloads.
             * @param payload The payload data.
             * @param op The opcode according to RFC6455
             */
            void send(const std::string& payload, frame::opcode::value op) {
#ifdef HAVE_BOOST_LOCK_GUARD
                boost::lock_guard<boost::recursive_mutex> lock(m_lock);
#else
                MutexHelper((pthread_mutex_t *)&m_lock);
#endif

                if (m_state != session::state::OPEN) {
                    log::err << "send: enpoint-state not OPEN";
                    return;
                }
                frame::parser<simple_rng> control(m_rng);
                control.set_opcode(op);
                control.set_fin(true);
                control.set_masked(false);
                control.set_payload(payload);

                std::string tmp(control.get_header_str());
                tmp.append(control.get_payload_str());
                m_handler->do_response(tmp);
            }

        private:
            void process_data() {
                m_handler->on_message(m_parser.get_header_str(), m_parser.get_payload_str());
            }

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
            void shutdown() {
#ifdef HAVE_BOOST_LOCK_GUARD
                boost::lock_guard<boost::recursive_mutex> lock(m_lock);
#else
                MutexHelper((pthread_mutex_t *)&m_lock);
#endif

                if (m_state == session::state::CLOSED) {return;}

                m_state = session::state::CLOSED;
                m_handler->on_close();
            }

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
            void pong(const std::vector<unsigned char> & payload) {
#ifdef HAVE_BOOST_LOCK_GUARD
                boost::lock_guard<boost::recursive_mutex> lock(m_lock);
#else
                MutexHelper((pthread_mutex_t *)&m_lock);
#endif

                if (m_state != session::state::OPEN) {return;}

                // TODO: optimize control messages and handle case where 
                // endpoint is out of messages
                frame::parser<simple_rng> control(m_rng);
                control.set_opcode(frame::opcode::PONG);
                control.set_fin(true);
                control.set_masked(false);
                control.set_payload(payload);

                std::string tmp(control.get_header_str());
                tmp.append(control.get_payload_str());
                m_handler->do_response(tmp);
            }

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
            void send_close(close::status::value code, const std::string& reason) {
#ifdef HAVE_BOOST_LOCK_GUARD
                boost::lock_guard<boost::recursive_mutex> lock(m_lock);
#else
                MutexHelper((pthread_mutex_t *)&m_lock);
#endif

                if (m_state != session::state::OPEN) {
                    log::err << "Tried to disconnect a session that wasn't open" << std::endl;
                    return;
                }

                if (close::status::invalid(code)) {
                    log::err << "Tried to close a connection with invalid close code: " 
                        << code << std::endl;
                    return;
                } else if (close::status::reserved(code)) {
                    log::err << "Tried to close a connection with reserved close code: " 
                        << code << std::endl;
                    return;
                }

                m_state = session::state::CLOSING;

                frame::parser<simple_rng> control(m_rng);
                control.set_opcode(frame::opcode::CLOSE);
                control.set_fin(true);
                control.set_masked(false);
                if (code != close::status::NO_STATUS) {
                    const uint16_t payload = htons(code);
                    std::string pl(reinterpret_cast<const char*>(&payload), 2);
                    pl.append(reason);
                    control.set_payload(pl);
                }

                std::string tmp(control.get_header_str());
                tmp.append(control.get_payload_str());
                m_handler->do_response(tmp);
            }

            /// send an acknowledgement close frame
            /** 
             * Visibility: private
             * State: no state checking, should only be called within process_control
             * Concurrency: Must be called within m_stranded method
             */
            void send_close_ack(close::status::value remote_close_code, std::string remote_close_reason) {
                close::status::value local_close_code;
                std::string local_close_reason;
                // echo close value unless there is a good reason not to.
                if (remote_close_code == close::status::NO_STATUS) {
                    local_close_code = close::status::NORMAL;
                    local_close_reason = "";
                } else if (remote_close_code == close::status::ABNORMAL_CLOSE) {
                    // TODO: can we possibly get here? This means send_close_ack was
                    //       called after a connection ended without getting a close
                    //       frame
                    throw "shouldn't be here";
                } else if (close::status::invalid(remote_close_code)) {
                    // TODO: shouldn't be able to get here now either
                    local_close_code = close::status::PROTOCOL_ERROR;
                    local_close_reason = "Status code is invalid";
                } else if (close::status::reserved(remote_close_code)) {
                    // TODO: shouldn't be able to get here now either
                    local_close_code = close::status::PROTOCOL_ERROR;
                    local_close_reason = "Status code is reserved";
                } else {
                    local_close_code = remote_close_code;
                    local_close_reason = remote_close_reason;
                }

                // TODO: check whether we should cancel the current in flight write.
                //       if not canceled the close message will be sent as soon as the
                //       current write completes.


                frame::parser<simple_rng> control(m_rng);
                control.set_opcode(frame::opcode::CLOSE);
                control.set_fin(true);
                control.set_masked(false);
                if (local_close_code != close::status::NO_STATUS) {
                    const uint16_t payload = htons(local_close_code);
                    std::string pl(reinterpret_cast<const char*>(&payload), 2);
                    pl.append(local_close_reason);
                    control.set_payload(pl);
                }

                std::string tmp(control.get_header_str());
                tmp.append(control.get_payload_str());
                m_handler->do_response(tmp);
                shutdown();
            }

            void process_control() {
                switch (m_parser.get_opcode()) {
                    case frame::opcode::PING:
                        if (m_handler->on_ping(m_parser.get_payload_str())) {
                            pong(m_parser.get_payload());
                        }
                        break;
                    case frame::opcode::PONG:
                        m_handler->on_pong(m_parser.get_payload_str());
                        break;
                    case frame::opcode::CLOSE:
                        // check that the codes we got over the wire are valid
                        if (m_state == session::state::OPEN) {
                            // other end is initiating
                            log::debug << "sending close ack" << std::endl;

                            // TODO:
                            send_close_ack(m_parser.get_close_code(), m_parser.get_close_reason());
                        } else if (m_state == session::state::CLOSING) {
                            // ack of our close
                            log::debug << "got close ack" << std::endl;
                            shutdown();
                        }
                        break;
                    default:
                        throw tracing::wserror("Invalid Opcode",
                                tracing::wserror::PROTOCOL_VIOLATION);
                        break;
                }
            }

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

    void wshandler::send_text(const std::string & data) {
        if (m_endpoint) {
            m_endpoint->send(data, frame::opcode::TEXT);
        }
    }
    void wshandler::send_binary(const std::string & data) {
        if (m_endpoint) {
            m_endpoint->send(data, frame::opcode::BINARY);
        }
    }

}

#endif
