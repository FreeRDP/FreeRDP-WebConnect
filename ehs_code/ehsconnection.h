/* $Id: ehsconnection.h 161 2013-02-27 00:02:04Z felfert $
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifndef _EHSCONNECTION_H_
#define _EHSCONNECTION_H_

#include "ehstypes.h"

class EHSServer;
class NetworkAbstraction;

/**
 * EHSConnection abstracts the concept of a connection to an EHS application.  
 * It stores file descriptor information, unhandled data, and the current 
 * state of the request
 */
class EHSConnection {

    private:

        EHSConnection ( const EHSConnection & );

        EHSConnection & operator = ( const EHSConnection & );

    private:

        bool m_bRawMode; ///< Flag: we are in raw IO mode

        bool m_bDoneReading; ///< we're never reading from this again

        bool m_bDisconnected; ///< client has closed connection on us

        bool m_bIdleHandling; ///< Shall this connection be closed on idle-timeout.

        HttpRequest * m_poCurrentHttpRequest; ///< request we're currently parsing

        EHSServer * m_poEHSServer; ///< server with which this is associated

        time_t m_nLastActivity; ///< time at which the last activity occured

        int m_nRequests; ///< holds id of last request received

        int m_nResponses; ///< holds id of last response sent

        int m_nActiveRequests; ///< Number of currently processing requests

        /// file descriptor associated with this client
        NetworkAbstraction * m_poNetworkAbstraction;	

        /// raw data received from client that doesn't comprise a full request
        std::string m_sBuffer;

        /// holds out-of-order httpresponses that aren't ready to go out yet
        ResponseQueue m_oResponseQueue;

        /// holds pending requests
        HttpRequestList m_oHttpRequestList;

        /// remote address from which the connection originated
        std::string m_sRemoteAddress;

        /// local address from which the connection originated
        std::string m_sLocalAddress;

        /// remote port from which the connection originated
        int m_nRemotePort;

        /// local port from which the connection originated
        int m_nLocalPort;

        size_t m_nMaxRequestSize;

        /// parse form data for content type given here - always if string is empty
        std::string m_sParseContentType;

        pthread_mutex_t m_oMutex; ///< mutex protecting entire object

    public:

        /// returns the remote address of the connection.
        std::string GetRemoteAddress() const { return m_sRemoteAddress; }

        /// returns the remote port of the connection.
        int GetRemotePort() const { return m_nRemotePort; }

        /// returns the local address of the connection.
        std::string GetLocalAddress() const { return m_sLocalAddress; }

        /// returns the local port of the connection.
        int GetLocalPort() const { return m_nLocalPort; }

        DEPRECATED("Use GetRemoteAddress()")
            /**
             * returns address of the connection.
             * @deprecated Use GetRemoteAddress()
             */
            std::string GetAddress() const { return GetRemoteAddress(); }

        DEPRECATED("Use GetRemotePort()")
            /**
             * returns client port of the connection.
             * @deprecated Use GetRemotePort()
             */
            int GetPort() const { return GetRemotePort(); }

        /// returns whether the client has disconnected from us.
        bool Disconnected() const { return m_bDisconnected; }

        /// returns whether the this connection is in raw mode.
        bool IsRaw() const { return m_bRawMode; }

        /// adds a response to the response list and sends as many responses as are ready
        void AddResponse(ehs_autoptr<GenericResponse> ehs_rvref response);

        /**
         * Enable/Disable idle-timeout handling for this connection.
         * @param enable If true, idle-timeout handling is enabled,
         *   otherwise the socket may stay open forever.
         * When creating an EHSConnection, this is initially enabled.
         */
        void EnableIdleTimeout(bool enable = true) { m_bIdleHandling = enable; }

        /**
         * Enable/Disable TCP keepalive on the underlying socket.
         * This enables detection of "dead" sockets, even when
         * idle-timeout handling is disabled.
         * @param enable If true, enable TCP keepalive.
         * When creating an EHSConnection, this is initially disabled.
         */
        void EnableKeepAlive(bool enable = true);

    private:

        /// Constructor
        EHSConnection(NetworkAbstraction * ipoNetworkAbstraction,
                EHSServer * ipoEHSServer);

        /// destructor
        ~EHSConnection();

        /// updates the last activity to the current time
        void UpdateLastActivity() { m_nLastActivity = time(NULL); }

        /// returns the time of last activity
        time_t LastActivity() { return m_bIdleHandling ? m_nLastActivity : time(NULL); }

        /// returns whether we're still reading from this socket -- mutex must be locked
        bool StillReading() { return !m_bDoneReading; }

        /// call when no more reads will be performed on this object.
        ///  ibDisconnected is true when client has disconnected
        void DoneReading ( bool ibDisconnected );

        /// gets the next request object
        HttpRequest *GetNextRequest();

        /// returns true if object should be deleted
        int CheckDone();

        /// Enumeration result for AddBuffer
        enum AddBufferResult {
            ADDBUFFER_INVALID = 0,
            ADDBUFFER_OK,
            ADDBUFFER_INVALIDREQUEST,
            ADDBUFFER_TOOBIG,
            ADDBUFFER_NORESOURCE
        };

        /// adds new data to psBuffer
        AddBufferResult AddBuffer(char * ipsData, int inSize);

        /**
         * Sends the actual data back to the client
         * @param response Pointer to the response to be sent.
         * If the pointer is <b>not</b> up-castable to HttpResponse <b>and</b>
         * the body is empty, then a close of this connection will be triggered.
         */ 
        void SendResponse(GenericResponse *response);

        /// returns true of httprequestlist is not empty
        int RequestsPending() { return (0 != m_nActiveRequests) || !m_oHttpRequestList.empty(); }

        /// returns underlying network abstraction
        NetworkAbstraction * GetNetworkAbstraction();

        /// Sets the maximum request size
        void SetMaxRequestSize(size_t n) { m_nMaxRequestSize = n; }

        /// Sets the content type to parse form data for
        void SetParseContentType(const std::string & s) { m_sParseContentType = s; }

        friend class EHSServer;
};

#endif // _EHSCONNECTION_H_
