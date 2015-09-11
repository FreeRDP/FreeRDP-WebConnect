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

#include "wsendpoint.hpp"

#ifndef HAVE_BOOST_LOCK_GUARD
    MutexHelper::MutexHelper(pthread_mutex_t *mutex, bool locknow) :
        m_pMutex(mutex), m_bLocked(false)
    {
        if (locknow)
            Lock();
    }

    MutexHelper::~MutexHelper()
    {
        if (m_bLocked)
            pthread_mutex_unlock(m_pMutex);
    }
    void MutexHelper::Lock()
    {
        pthread_mutex_lock(m_pMutex);
        m_bLocked = true;
    }

    void MutexHelper::Unlock()
    {
        m_bLocked = false;
        pthread_mutex_unlock(m_pMutex);
    }
#endif

namespace wspp {
    using wsgate::log;
    wsendpoint::wsendpoint(wshandler *h)
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
    wsendpoint::~wsendpoint() { pthread_mutex_destroy(&m_lock); }
#endif

    void wsendpoint::AddRxData(std::string data)
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

    void wsendpoint::send(const std::string& payload, frame::opcode::value op) {
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

    void wsendpoint::process_data() {
        m_handler->on_message(m_parser.get_header_str(), m_parser.get_payload_str());
    }

            
    void wsendpoint::shutdown() {
#ifdef HAVE_BOOST_LOCK_GUARD
        boost::lock_guard<boost::recursive_mutex> lock(m_lock);
#else
        MutexHelper((pthread_mutex_t *)&m_lock);
#endif

        if (m_state == session::state::CLOSED) {return;}

        m_state = session::state::CLOSED;
        m_handler->on_close();
    }

    void wsendpoint::pong(const std::vector<unsigned char> & payload) {
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

    void wsendpoint::send_close(close::status::value code, const std::string& reason) {
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

    void wsendpoint::send_close_ack(close::status::value remote_close_code, std::string remote_close_reason) {
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

    void wsendpoint::process_control() {
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
}