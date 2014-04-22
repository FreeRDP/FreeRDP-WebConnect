/* $Id: ehsserver.h 81 2012-03-20 12:03:05Z felfert $
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

#ifndef _EHSSERVER_H_
#define _EHSSERVER_H_

/**
 * EHSServer contains all the network related services for EHS.
 * It is responsible for accepting new connections and getting
 * EHSConnection objects set up.
 */
class EHSServer {

    private:

        EHSServer(const EHSServer &);

        EHSServer & operator=(const EHSServer &);

    public:

        /**
         * Constucts a new instance.
         * Takes parameters out of the parent's EHSServerParameters.
         * @param ipoTopLevelEHS The parent EHS instance.
         */
        EHSServer(EHS *ipoTopLevelEHS);

        /// Destructor
        virtual ~EHSServer();

        /**
         * Stops the server.
         * This method blocks, until all server threads have terminated.
         */
        void EndServerThread();

        /**
         * Main method that deals with client connections and getting data.
         * @param timeout select timeout in milliseconds.
         * @param tid Thread Id of the thread calling this method.
         */
        void HandleData(int timeout, ehs_threadid_t tid = 0); 

        /// Enumeration on the current running status of the EHSServer
        enum ServerRunningStatus {
            SERVERRUNNING_INVALID = 0,
            SERVERRUNNING_NOTRUNNING,
            SERVERRUNNING_SINGLETHREADED,
            SERVERRUNNING_THREADPOOL,
            SERVERRUNNING_ONETHREADPERREQUEST,
            SERVERRUNNING_SHOULDTERMINATE
        };

        /**
         * Retrieves the current running status of this instance.
         * @return The current status.
         */
        ServerRunningStatus RunningStatus() const { return m_nServerRunningStatus; }

        /**
         * Retrieve accept status.
         * @return true if this instance just has accepted a new connection.
         */
        bool AcceptedNewConnection() const { return m_bAcceptedNewConnection; }

        /// Returns number of requests pending
        int RequestsPending() const { return m_nRequestsPending; }

        /**
         * Static pthread worker.
         * Required by pthread as thread routine.
         * @param ipData Opaque pointer to this instance.
         */
        static void *PthreadHandleData_ThreadedStub(void *ipData);

    private:

        /// Gets a pending request
        HttpRequest *GetNextRequest();

        /// Increments the number of pending requests
        void IncrementRequestsPending() { m_nRequestsPending++; }

        /**
         * Removes the specified EHSConnection object.
         * @param ipoEHSConnection Pointer to the connection to remove.
         */
        void RemoveEHSConnection(EHSConnection *ipoEHSConnection);

        /// this runs in a loop until told to stop by StopServer()
        /// runs off it's own thread created by StartServer_Threaded
        void HandleData_Threaded();

        /**
         * Disconnects idle connections.
         */
        void ClearIdleConnections();

        /// check clients that are already connected
        void CheckClientSockets();

        /// check the listen socket for a new connection
        void CheckAcceptSocket();

        /// creates the poll array with the accept socket and any connections present
        int CreateFdSet();

        /**
         * Removes all connections from the server that are no longer active.
         */
        void RemoveFinishedConnections();

        /// Current running status of the EHSServer
        ServerRunningStatus m_nServerRunningStatus;

        /// Pointer back up to top-most level EHS object
        EHS * m_poTopLevelEHS;

        /// Whether we accepted a new connection last time through
        bool m_bAcceptedNewConnection;

        /// Mutex for controlling who is accepting or processing jobs
        pthread_mutex_t m_oMutex;

        /// Condition for when a thread is done accepting and there may be more jobs to process
        pthread_cond_t m_oDoneAccepting;

        /// number of requests waiting to be processed
        int m_nRequestsPending;

        /// Flag: Are we currently accepting requests?
        bool m_bAccepting;

        /// this is the server name sent out in the response headers
        std::string m_sServerName;

        /// this is the read set for sending to select(2)
        fd_set m_oReadFds;

        /// List of all connections currently attached to the server
        EHSConnectionList m_oEHSConnectionList;

        /// the network abstraction this server is listening on
        NetworkAbstraction * m_poNetworkAbstraction;

        /// pthread identifier for the accept thread -- only used when started in threaded mode
        ehs_threadid_t m_nAcceptThreadId;

        /// number of seconds a connection can be idle before disconnect
        int m_nIdleTimeout;

        /// Number of currently running threads
        int m_nThreads;

        /// Map of currently handled request (per thread)
        CurrentRequestMap m_oCurrentRequest;

        /// Thread creation attributes (for setting stack size)
        pthread_attr_t m_oThreadAttr;

        friend class EHSConnection;
};

#endif // _EHSSERVER_H_
