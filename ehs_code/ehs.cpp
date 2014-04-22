/* $Id: ehs.cpp 162 2013-02-27 00:34:38Z felfert $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif

#include <pthread.h>

#include "ehs.h"
#include "networkabstraction.h"
#include "ehsconnection.h"
#include "ehsserver.h"
#include "socket.h"
#include "securesocket.h"
#include "debug.h"
#include "mutexhelper.h"
#include "ehstypes.h"

#include <boost/regex.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cerrno>

#ifdef COMPILE_WITH_SSL
# include <openssl/opensslv.h>

#define VERSION "1"
#define SVNREV "1"
#endif
static const char * const EHSconfig = "EHS_CONFIG:SSL="
#ifdef COMPILE_WITH_SSL
"\"" OPENSSL_VERSION_TEXT "\""
#else
"DISABLED"
#endif
",DEBUG="
#ifdef EHS_DEBUG
"1"
#else
"0"
#endif
",VERSION=" VERSION ",RELEASE=" SVNREV ",BUILD=" __DATE__ " " __TIME__;

const char * getEHSconfig()
{
    return EHSconfig;
}

using namespace std;

class EHSThreadHandlerHelper
{
    public:
        EHSThreadHandlerHelper(EHS *ehs)
            : m_pEHS(ehs)
        {
            if (m_pEHS) {
                if (!m_pEHS->ThreadInitHandler()) {
                    m_pEHS = NULL;
                }
            }
        }

        ~EHSThreadHandlerHelper()
        {
            if (m_pEHS) {
                m_pEHS->ThreadExitHandler();
                m_pEHS = NULL;
            }
        }

        bool IsOK()
        {
            return (NULL != m_pEHS);
        }

    private:
        EHS *m_pEHS;
        // Disabled copy constructor
        EHSThreadHandlerHelper(const EHSThreadHandlerHelper& other) : m_pEHS(other.m_pEHS) { }
        EHSThreadHandlerHelper & operator=(const EHSThreadHandlerHelper& other) { m_pEHS = other.m_pEHS;  return *this; }
};

int EHSServer::CreateFdSet()
{
    // don't lock mutex, as this is only called from within a locked section
    FD_ZERO(&m_oReadFds);
    // add the accepting FD	
    FD_SET(m_poNetworkAbstraction->GetFd(), &m_oReadFds);
    ehs_socket_t nHighestFd = m_poNetworkAbstraction->GetFd();
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); ++i) {
        /// skip this one if it's already been used
        if ((*i)->StillReading()) {
            ehs_socket_t nCurrentFd = (*i)->GetNetworkAbstraction()->GetFd();
            // EHS_TRACE("Adding %d to FD SET", nCurrentFd);
            FD_SET(nCurrentFd, &m_oReadFds);
            // store the highest FD in the set to return it
            if (nCurrentFd > nHighestFd) {
                nHighestFd = nCurrentFd;
            }
        } else {
            EHS_TRACE("FD %d isn't reading anymore",
                    (*i)->GetNetworkAbstraction()->GetFd());
            if (time(NULL) - (*i)->LastActivity() > (5 * m_nIdleTimeout)) {
            }
        }
    }
    return (int)nHighestFd;
}

void EHSServer::ClearIdleConnections()
{
    // don't lock mutex, as this is only called from within locked sections
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); ++i) {

        MutexHelper mh(&(*i)->m_oMutex);
        // if it's been more than N seconds since a response has been
        //   sent and there are no pending requests
        if ((*i)->StillReading() &&
                time(NULL) - (*i)->LastActivity() > m_nIdleTimeout &&
                (!(*i)->RequestsPending())) {
            EHS_TRACE("Done reading because of idle timeout", "");
            mh.Unlock();
            (*i)->DoneReading(false);
            mh.Lock();
        }
    }
    RemoveFinishedConnections();
}

void EHSServer::RemoveFinishedConnections ( )
{
    // don't lock mutex, as this is only called from within locked sections
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); ) {
        if ((*i)->CheckDone()) {
            EHS_TRACE("Found connection to delete: %p", *i);
            RemoveEHSConnection(*i);
            i = m_oEHSConnectionList.begin();
        } else {
            ++i;
        }
    }
}


EHSConnection::EHSConnection(NetworkAbstraction *ipoNetworkAbstraction,
        EHSServer * ipoEHSServer) :
    m_bRawMode(false),
    m_bDoneReading(false),
    m_bDisconnected(false),
    m_bIdleHandling(true),
    m_poCurrentHttpRequest(NULL),
    m_poEHSServer(ipoEHSServer),
    m_nLastActivity(0),
    m_nRequests(0),
    m_nResponses(0),
    m_nActiveRequests(0),
    m_poNetworkAbstraction(ipoNetworkAbstraction),
    m_sBuffer(""),
    m_oResponseQueue(ResponseQueue()),
    m_oHttpRequestList(HttpRequestList()),
    m_sRemoteAddress(ipoNetworkAbstraction->GetRemoteAddress()),
    m_sLocalAddress(ipoNetworkAbstraction->GetLocalAddress()),
    m_nRemotePort(ipoNetworkAbstraction->GetRemotePort()),
    m_nLocalPort(ipoNetworkAbstraction->GetLocalPort()),
    m_nMaxRequestSize(MAX_REQUEST_SIZE_DEFAULT),
    m_sParseContentType(""),
    m_oMutex(pthread_mutex_t())
{
    UpdateLastActivity();
    // initialize mutex for this object
    pthread_mutex_init(&m_oMutex, NULL);
}


EHSConnection::~EHSConnection()
{
    if (m_bRawMode) {
        RawSocketHandler *rsh = m_poEHSServer->m_poTopLevelEHS->GetRawSocketHandler();
        if (rsh) {
            rsh->OnDisconnect(this);
        }
    }
    delete m_poCurrentHttpRequest;
    delete m_poNetworkAbstraction;
    pthread_mutex_destroy(&m_oMutex);
}

void EHSConnection::EnableKeepAlive(bool enable)
{
    MutexHelper mh(&m_oMutex);
    if (m_poNetworkAbstraction) {
        ehs_socket_t s = m_poNetworkAbstraction->GetFd();
        int one = enable ? 1 : 0;
#ifdef _WIN32
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char *>(&one), sizeof(int));
#else
        setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const void *>(&one), sizeof(int));
#endif
    }
}

NetworkAbstraction *EHSConnection::GetNetworkAbstraction()
{
    return m_poNetworkAbstraction;
}


// adds data to the current buffer for this connection
    EHSConnection::AddBufferResult
EHSConnection::AddBuffer(char *ipsData, ///< new data to be added
        int inSize ///< size of new data
        )
{
    MutexHelper mh(&m_oMutex);
    // make sure we actually got some data
    if ( inSize <= 0 ) {
        return ADDBUFFER_INVALID;
    }
    // make sure the buffer doesn't grow too big
    if ((m_sBuffer.length() + inSize) > m_nMaxRequestSize) {
        EHS_TRACE("MaxRequestSize (%lu) exceeded.", m_nMaxRequestSize);
        return ADDBUFFER_TOOBIG;
    }
    // this is binary safe -- only the single argument char* constructor looks for NULL
    m_sBuffer += string ( ipsData, inSize );
    // need to run through our buffer until we don't get a full result out
    do {
        // if we need to make a new request object, do that now
        if (NULL == m_poCurrentHttpRequest ||
                m_poCurrentHttpRequest->m_nCurrentHttpParseState == HttpRequest::HTTPPARSESTATE_COMPLETEREQUEST ) {
            // if we have one already, toss it on the list
            if (NULL != m_poCurrentHttpRequest) {
                m_oHttpRequestList.push_back(m_poCurrentHttpRequest);
                m_poEHSServer->IncrementRequestsPending();
                // wake up everyone
                pthread_cond_broadcast(& m_poEHSServer->m_oDoneAccepting);
                if (m_poEHSServer->m_nServerRunningStatus == EHSServer::SERVERRUNNING_ONETHREADPERREQUEST ) {
                    // create a thread if necessary
                    pthread_t oThread;
                    mh.Unlock();
                    if (0 != pthread_create(&oThread, &(m_poEHSServer->m_oThreadAttr),
                                EHSServer::PthreadHandleData_ThreadedStub,
                                (void *)m_poEHSServer)) {
                        return ADDBUFFER_NORESOURCE;
                    }
                    EHS_TRACE("Created thread with TID=0x%x, NULL, func=0x%x, data=0x%x\n",
                            oThread, EHSServer::PthreadHandleData_ThreadedStub, (void *) m_poEHSServer);
                    pthread_detach(oThread);
                    mh.Lock();
                }
            }
            // create the initial request
            m_poCurrentHttpRequest = new HttpRequest(++m_nRequests, this, m_sParseContentType);
            m_poCurrentHttpRequest->m_bSecure = m_poNetworkAbstraction->IsSecure();
        }
        // parse through the current data
        m_poCurrentHttpRequest->ParseData(m_sBuffer);
    } while (m_poCurrentHttpRequest->m_nCurrentHttpParseState ==
            HttpRequest::HTTPPARSESTATE_COMPLETEREQUEST);
    if ( m_poCurrentHttpRequest->m_nCurrentHttpParseState == HttpRequest::HTTPPARSESTATE_INVALIDREQUEST ) {
        return ADDBUFFER_INVALIDREQUEST;
    }
    return ADDBUFFER_OK;
}

/// call when no more reads will be performed on this object.  inDisconnected is true when client has disconnected
void EHSConnection::DoneReading(bool ibDisconnected)
{
    MutexHelper mh(&m_oMutex);
    m_bDoneReading = true;
    m_bDisconnected = ibDisconnected;
}

HttpRequest * EHSConnection::GetNextRequest()
{
    HttpRequest *ret = NULL;
    MutexHelper mh(&m_oMutex);
    if (!m_oHttpRequestList.empty()) {
        ret = m_oHttpRequestList.front();
        // increment active requests before removing the request from the list to avoid idle-detection race conditions
        ++m_nActiveRequests;
        m_oHttpRequestList.pop_front();
    }
    return ret;
}


int EHSConnection::CheckDone()
{
    // if we're not still reading, we may want to drop this connection
    if ( !StillReading ( ) ) {
        // if we're done with all our responses (-1 because the next (unused) request is already created)
        if (m_nRequests - 1 <= m_nResponses) {
            // if we haven't disconnected, do that now
            if (!m_bDisconnected) {
                EHS_TRACE ("Closing connection", "");
                m_poNetworkAbstraction->Close();
            }
            return 1;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////
// EHS SERVER
////////////////////////////////////////////////////////////////////

EHSServer::EHSServer (EHS *ipoTopLevelEHS) :
    m_nServerRunningStatus(SERVERRUNNING_NOTRUNNING),
    m_poTopLevelEHS(ipoTopLevelEHS),
    m_bAcceptedNewConnection(false),
    m_oMutex(pthread_mutex_t()),
    m_oDoneAccepting(pthread_cond_t()),
    m_nRequestsPending(0),
    m_bAccepting(false),
    m_sServerName(""),
    m_oReadFds(fd_set()),
    m_oEHSConnectionList(EHSConnectionList()),
    m_poNetworkAbstraction(NULL),
    m_nAcceptThreadId(0),
    m_nIdleTimeout(15),
    m_nThreads(0),
    m_oCurrentRequest(CurrentRequestMap()),
    m_oThreadAttr(pthread_attr_t())
{
    // you HAVE to specify a top-level EHS object
    if (NULL == m_poTopLevelEHS) {
        throw invalid_argument("EHSServer::EHSServer: Pointer to toplevel EHS object is NULL.");
    }

    // grab out the parameters for less typing later on
    EHSServerParameters & params = ipoTopLevelEHS->m_oParams;

    pthread_mutex_init(&m_oMutex, NULL);
    pthread_cond_init(&m_oDoneAccepting, NULL);
    pthread_attr_init(&m_oThreadAttr);
    {
        // Set minimum stack size
        size_t stacksize;
        size_t min_stacksize = (unsigned long)params["stacksize"];
        pthread_attr_getstacksize (&m_oThreadAttr, &stacksize);
        EHS_TRACE ("Default thread stack size is %li", stacksize);
        if (stacksize < min_stacksize) {
            EHS_TRACE ("Setting thread stack size to %li", min_stacksize);
            pthread_attr_setstacksize(&m_oThreadAttr, min_stacksize);
        }
    }
    // whether to run with https support
    int nHttps = params["https"];
    if (nHttps) {
        EHS_TRACE("EHSServer running in HTTPS mode", "");
    } else {
        EHS_TRACE("EHSServer running in plain-text mode (no HTTPS)", "");
    }
    try {
        // are we using secure sockets?
        if (nHttps) {
#ifdef COMPILE_WITH_SSL		
            EHS_TRACE("Trying to create secure socket with certificate='%s' and passphrase='%s'",
                    (const char*)params["certificate"],
                    (const char*)params["passphrase"]);
            m_poNetworkAbstraction = new SecureSocket(params["certificate"],
                    reinterpret_cast<PassphraseHandler *>(ipoTopLevelEHS));
#else // COMPILE_WITH_SSL
            throw runtime_error("EHSServer::EHSServer: EHS not compiled with SSL support. Cannot create HTTPS server.");
#endif // COMPILE_WITH_SSL
        } else {
            m_poNetworkAbstraction = new Socket();
        }

        if (NULL == m_poNetworkAbstraction) {
            throw runtime_error("EHSServer::EHSServer: Could not allocate socket.");
        }
        // initialize the socket
        if (params["bindaddress"] != "") {
            m_poNetworkAbstraction->SetBindAddress(params["bindaddress"]);
        }
        m_poNetworkAbstraction->RegisterBindHelper(m_poTopLevelEHS->GetBindHelper());
        m_poNetworkAbstraction->Init(params["port"]); // initialize socket stuff
        if (params["mode"] == "threadpool") {
            // need to set this here because the thread will check this to make
            // sure it's supposed to keep running
            m_nServerRunningStatus = SERVERRUNNING_THREADPOOL;
            // create a pthread
            int nThreadsToStart = params["threadcount"].GetInt();
            if (nThreadsToStart <= 0) {
                nThreadsToStart = 1;
            }
            EHS_TRACE ("Starting %d threads in pool", nThreadsToStart);
            for (int i = 0; i < nThreadsToStart; i++) {
                // create new thread and detach so we don't have to join on it
                pthread_t thread;
                if (0 == pthread_create(&thread, &m_oThreadAttr,
                            EHSServer::PthreadHandleData_ThreadedStub, (void *)this)) {
                    m_nAcceptThreadId = THREADID(thread);
                    EHS_TRACE("Created thread with ID=0x%x, NULL, func=0x%x, this=0x%x",
                            m_nAcceptThreadId, EHSServer::PthreadHandleData_ThreadedStub, this);
                    pthread_detach(thread);
                } else {
                    m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
                    throw runtime_error("EHSServer::EHSServer: Unable to create threads");
                }
            }
        } else if (params["mode"] == "onethreadperrequest") {
            m_nServerRunningStatus = SERVERRUNNING_ONETHREADPERREQUEST;
            // spawn off one thread just to deal with basic stuff
            pthread_t thread;
            if (0 == pthread_create(&thread, &m_oThreadAttr,
                        EHSServer::PthreadHandleData_ThreadedStub, (void *)this)) {
                m_nAcceptThreadId = THREADID(thread);
                EHS_TRACE("Created thread with ID=0x%x, NULL, func=0x%x, this=0x%x",
                        m_nAcceptThreadId, EHSServer::PthreadHandleData_ThreadedStub, this);
                pthread_detach(thread);
            } else {
                m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
                throw runtime_error("EHSServer::EHSServer: Unable to create listener thread");
            }
        } else if (params["mode"] == "singlethreaded") {
            // we're single threaded
            m_nServerRunningStatus = SERVERRUNNING_SINGLETHREADED;
        } else {
            throw runtime_error("EHSServer::EHSServer: invalid mode specified");
        }
    } catch (...) {
        delete m_poNetworkAbstraction;
        throw;
    }
    switch (m_nServerRunningStatus) {
        case SERVERRUNNING_THREADPOOL:
            EHS_TRACE("EHS Server running in threadpool mode with %s threads",
                    params["threadcount"] == "" ? "1" :
                    params[ "threadcount"].GetCharString());
            break;
        case SERVERRUNNING_ONETHREADPERREQUEST:
            EHS_TRACE("EHS Server running with one thread per request", "");
            break;
        case SERVERRUNNING_SINGLETHREADED:
            EHS_TRACE("EHS Server running in singlethreaded mode", "");
            break;
        default:
            EHS_TRACE("EHS Server not running. Server initialization failed.", "");
            break;
    }
    return;
}


EHSServer::~EHSServer ( )
{
    delete m_poNetworkAbstraction;
    // Delete all elements in our connection list
    while ( ! m_oEHSConnectionList.empty() ) {
        delete m_oEHSConnectionList.front ( );
        m_oEHSConnectionList.pop_front ( );
    }
    pthread_mutex_destroy(&m_oMutex);
}

HttpRequest * EHSServer::GetNextRequest()
{
    // don't lock because this is only called from within locked sections
    HttpRequest * poNextRequest = NULL;
    // pick a random connection if the list isn't empty
    if (!m_oEHSConnectionList.empty()) {
        // pick a random connection, so no one takes too much time
        int nWhich = (int)(((double)m_oEHSConnectionList.size()) * rand() / (RAND_MAX + 1.0));
        // go to that element
        EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
        int nCounter;
        for (nCounter = 0; nCounter < nWhich; ++nCounter) {
            ++i;
        }
        // now get the next available request treating the list as circular
        EHSConnectionList::iterator iStartPoint = i;
        int nFirstTime = 1;
        while (poNextRequest == NULL && !(iStartPoint == i && nFirstTime == 0)) {
            // check this one to see if it has anything
            poNextRequest = (*i)->GetNextRequest();
            if (++i == m_oEHSConnectionList.end()) {
                i = m_oEHSConnectionList.begin();
            }
            nFirstTime = 0;
            // decrement the number of pending requests
            if (poNextRequest != NULL) {
                m_nRequestsPending--;
            }
        }
    }
    if (poNextRequest == NULL) {
        //		EHS_TRACE ("No request found\n");
    } else {
        EHS_TRACE ("Found request", "");
    }
    return poNextRequest;
}

void EHSServer::RemoveEHSConnection(EHSConnection * ipoEHSConnection)
{
    // don't lock as this is only called from within locked sections
    if (NULL == ipoEHSConnection) {
        throw invalid_argument("EHSServer::RemoveEHSConnection: argument is NULL");
    }
    bool removed = false;
    // go through the list and find all occurances of ipoEHSConnection
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); /* no third part */) {
        if (*i == ipoEHSConnection) {
            if (removed) {
                throw runtime_error("EHSServer::RemoveEHSConnection: Deleting a second element");
            }
            removed = true;
            // destroy the connection and remove it from the list
            // erase() returns an iterator pointing to the following element.
            delete *i;
            i = m_oEHSConnectionList.erase(i);
        } else {
            ++i;
        }
    }
    EHS_TRACE("%d connections remaining", m_oEHSConnectionList.size());
}

bool EHS::ThreadInitHandler()
{
    EHS_TRACE("called", "");
    return true;
}

void EHS::ThreadExitHandler()
{
    EHS_TRACE("called", "");
}

const std::string EHS::GetPassphrase(bool /* twice */)
{
#ifdef _WIN32
    return m_oParams["passphrase"].GetCharString();
#else
	return m_oParams["passphrase"];
#endif
}

void EHS::StartServer(EHSServerParameters &params)
{
    m_oParams = params;
    if (m_poEHSServer != NULL) {
        throw runtime_error("EHS::StartServer: already running");
    } else {
        // associate a EHSServer object to this EHS object
        m_bNoRouting = (m_oParams.find("norouterequest") != m_oParams.end());
        try {
            m_poEHSServer = new EHSServer(this);
        } catch (...) {
            delete m_poEHSServer;
            throw;
        }
    }
}

// this is the function specified to pthread_create under UNIX
//   because you can't start a thread directly into a class method
void * EHSServer::PthreadHandleData_ThreadedStub(void * ipParam ///< EHSServer object cast to a void pointer
        )
{
    EHSServer *self = reinterpret_cast<EHSServer *>(ipParam);
    MutexHelper mh(&self->m_oMutex);
    self->m_nThreads++;
    mh.Unlock();
    self->HandleData_Threaded();
    mh.Lock();
    self->m_nThreads--;
    return NULL;
}

void EHS::StopServer()
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::StopServer: Invalid state");
    }

    if (m_poParent) {
        m_poParent->StopServer();
    } else if (m_poEHSServer) {
        m_poEHSServer->EndServerThread();
        delete m_poEHSServer;
        m_poEHSServer = NULL;
    }
}

bool EHS::ShouldTerminate() const
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::StopServer: Invalid state");
    }

    bool ret = false;
    if (m_poParent) {
        ret = m_poParent->ShouldTerminate();
    } else if (m_poEHSServer) {
        ret = (EHSServer::SERVERRUNNING_SHOULDTERMINATE == m_poEHSServer->RunningStatus());
    }
    return ret;
}

void EHSServer::HandleData_Threaded()
{
    EHSThreadHandlerHelper thh(m_poTopLevelEHS);
    if (thh.IsOK()) {
        const ehs_threadid_t self = THREADID(pthread_self());
        do {
            bool catched = false;
            ehs_autoptr<GenericResponse> eResponse;

            try {
                HandleData(1000, self); // 1000ms select timeout
            } catch (exception &e) {
                catched = true;
                eResponse.reset(m_poTopLevelEHS->HandleThreadException(self, m_oCurrentRequest[self], e));
            } catch (...) {
                catched = true;
                runtime_error e("unspecified");
                eResponse.reset(m_poTopLevelEHS->HandleThreadException(self, m_oCurrentRequest[self], e));
            }
            if (catched) {
                if (NULL != eResponse.get()) {
                    eResponse->GetConnection()->AddResponse(ehs_move(eResponse));
                    MutexHelper mutex(&m_oMutex);
                    delete m_oCurrentRequest[self];
                    m_oCurrentRequest[self] = NULL;
                } else {
                    m_nServerRunningStatus = SERVERRUNNING_SHOULDTERMINATE;
                    m_nAcceptThreadId = 0;
                    MutexHelper mutex(&m_oMutex);
                    delete m_oCurrentRequest[self];
                    m_oCurrentRequest[self] = NULL;
                }
            }
        } while (m_nServerRunningStatus == SERVERRUNNING_THREADPOOL ||
                self == m_nAcceptThreadId);
        m_poNetworkAbstraction->ThreadCleanup();
    }
}

void EHSServer::HandleData (int inTimeoutMilliseconds, ehs_threadid_t tid)
{
    MutexHelper mutex(&m_oMutex);
    // determine if there are any jobs waiting if this thread should --
    //   if we're running one-thread-per-request and this is the accept thread
    //   we don't look for requests
    m_oCurrentRequest[tid] = NULL;
    if (m_nServerRunningStatus != SERVERRUNNING_ONETHREADPERREQUEST ||
            tid != m_nAcceptThreadId ) {
        m_oCurrentRequest[tid] = GetNextRequest();
    }
    // if we got a request to handle
    if (NULL != m_oCurrentRequest[tid]) {
        HttpRequest *req = m_oCurrentRequest[tid];
        // handle the request and post it back to the connection object
        mutex.Unlock();
        // route the request
        ehs_autoptr<GenericResponse> response(m_poTopLevelEHS->RouteRequest(req));
        response->GetConnection()->AddResponse(ehs_move(response));
        delete req;
        mutex.Lock();
        m_oCurrentRequest[tid] = NULL;
    } else {
        // otherwise, no requests are pending

        // if something is already accepting, sleep
        if (m_bAccepting) {
            // wait until something happens
            // it's ok to not recheck our condition here, as we'll come back in the same way and recheck then
            EHS_TRACE("Waiting on m_oDoneAccepting condition TID=%p", pthread_self());
            pthread_cond_wait(&m_oDoneAccepting, &m_oMutex);
            EHS_TRACE("Done waiting on m_oDoneAccepting condition TID=%p", pthread_self());
        } else {
            // if no one is accepting, we accept
            m_bAcceptedNewConnection = false;
            // we're now accepting
            m_bAccepting = true;
            mutex.Unlock();
            // set up the timeout and normalize
            timeval tv = { 0, inTimeoutMilliseconds * 1000 };
            tv.tv_sec = tv.tv_usec / 1000000;
            tv.tv_usec %= 1000000;
            // create the FD set for select
            int nHighestFd = CreateFdSet();
            // call select
            int nSocketCount = select(nHighestFd + 1, &m_oReadFds, NULL, NULL, &tv);
            // handle select error
            if (nSocketCount ==
#ifdef _WIN32
                    SOCKET_ERROR
#else // NOT _WIN32
                    -1
#endif // _WIN32
               )
            {
#ifdef _WIN32
                throw runtime_error("EHSServer::HandleData: select() failed.");
#else // NOT _WIN32
                if (errno != EINTR) {
                    throw runtime_error("EHSServer::HandleData: select() failed.");
                }
#endif // _WIN32
            }
            // if no sockets have data to read, clear accepting flag and return
            if (nSocketCount > 0) {
                // Check the accept socket for a new connection
                CheckAcceptSocket();
                // check client sockets for data
                CheckClientSockets();
            }
            mutex.Lock();
            ClearIdleConnections();
            m_bAccepting = false;
        } // END ACCEPTING
    } // END NO REQUESTS PENDING
}

void EHSServer::CheckAcceptSocket ( )
{
    // see if we got data on this socket
    if (FD_ISSET(m_poNetworkAbstraction->GetFd(), &m_oReadFds)) {
        NetworkAbstraction *poNewClient = NULL;
        try {
            poNewClient = m_poNetworkAbstraction->Accept();
        } catch (runtime_error &e) {
            string emsg(e.what());
            // SSL handshake errors are acceptable.
            if (emsg.find("SSL") == string::npos) {
                throw;
            }
            std::cerr << emsg << endl;
            return;
        }
        // create a new EHSConnection object and initialize it
        EHSConnection * poEHSConnection = new EHSConnection ( poNewClient, this );
        if (m_poTopLevelEHS->m_oParams.find("maxrequestsize") !=
                m_poTopLevelEHS->m_oParams.end()) {
            unsigned long n = m_poTopLevelEHS->m_oParams["maxrequestsize"];
            EHS_TRACE("Setting connections MaxRequestSize to %lu\n", n);
            poEHSConnection->SetMaxRequestSize ( n );
        }
        if (m_poTopLevelEHS->m_oParams.find("parsecontenttype") !=
                m_poTopLevelEHS->m_oParams.end()) {
            
#ifdef _WIN32
			Datum pct = m_poTopLevelEHS->m_oParams["parsecontenttype"];
			EHS_TRACE("Setting parse content type to %s\n", pct.GetCharString());
#else
			string pct = m_poTopLevelEHS->m_oParams["parsecontenttype"];
            EHS_TRACE("Setting parse content type to %s\n", pct.c_str());
#endif
            poEHSConnection->SetParseContentType ( pct );
        }
        {
            MutexHelper mutex(&m_oMutex);
            m_oEHSConnectionList.push_back(poEHSConnection);
            m_bAcceptedNewConnection = true;
        }
        EHS_TRACE("Accepted new connection %p\n", poEHSConnection);
    } // end FD_ISSET ( )
}

void EHSServer::CheckClientSockets ( )
{
    // go through all the sockets from which we're still reading
    for (EHSConnectionList::iterator i = m_oEHSConnectionList.begin();
            i != m_oEHSConnectionList.end(); ++i) {
        if (FD_ISSET((*i)->GetNetworkAbstraction()->GetFd(), &m_oReadFds)) {
            // do the actual read
            char buf[8192];
            int nBytesReceived = (*i)->GetNetworkAbstraction()->Read(buf, sizeof(buf));

            if ((*i)->IsRaw()) {
                if (0 > nBytesReceived) {
                    (*i)->DoneReading(true);
                } else {
                    (*i)->UpdateLastActivity();
                    RawSocketHandler *sh = m_poTopLevelEHS->GetRawSocketHandler(); 
                    if (0 < nBytesReceived) {
                        EHS_TRACE("$$$$$ Got RAW data: len=%d", nBytesReceived);
                    }
                    if (sh && (0 < nBytesReceived)) {
                        if (! sh->OnData(*i, string(buf, nBytesReceived))) {
                            (*i)->DoneReading(false);
                        }
                    }
                }
                continue;
            }
            EHS_TRACE("$$$$$ Got data on client connection: len=%d", nBytesReceived);

            // if we received a disconnect
            if (nBytesReceived <= 0) {
                // we're done reading and we received a disconnect
                EHS_TRACE("Read result = %d", nBytesReceived);
                (*i)->DoneReading(true);
            } else {
                // otherwise we got data
                // take the data we got and append to the connection's buffer
                EHSConnection::AddBufferResult nAddBufferResult =
                    (*i)->AddBuffer(buf, nBytesReceived);
                // if add buffer failed, don't read from this connection anymore
                switch (nAddBufferResult) {
                    case EHSConnection::ADDBUFFER_INVALIDREQUEST:
                        {
                            // Immediately send a 400 response, then close the connection
                            ehs_autoptr<GenericResponse> tmp(HttpResponse::Error(HTTPRESPONSECODE_400_BADREQUEST, 0, *i));
                            (*i)->SendResponse(tmp.get());
                            (*i)->DoneReading(false);
                            EHS_TRACE("Done reading because we got a bad request", "");
                        }
                        break;
                    case EHSConnection::ADDBUFFER_TOOBIG:
                        {
                            // Immediately send a configurable response (Default: 413), then close the connection
                            ResponseCode rc = HTTPRESPONSECODE_413_TOOLARGE;
                            if (m_poTopLevelEHS->m_oParams.find("code413") !=
                                    m_poTopLevelEHS->m_oParams.end()) {
                                unsigned long n = m_poTopLevelEHS->m_oParams["code413"];
                                rc = (ResponseCode)n;
                            }
                            ehs_autoptr<GenericResponse> tmp(HttpResponse::Error(rc, 0, *i));
                            (*i)->SendResponse(tmp.get());
                            (*i)->DoneReading(false);
#ifdef SPECIAL_STDERR
                            std::cerr << "EHS Warning: Request size exceeded. Returning " << tmp.GetStatusString() << "." << std::endl;
#endif
                            EHS_TRACE("Done reading because we got a too large request", "");
                        }
                        break;
                    case EHSConnection::ADDBUFFER_NORESOURCE:
                        {
                            // Immediately send a 503 response, then close the connection
                            ehs_autoptr<GenericResponse> tmp(HttpResponse::Error(HTTPRESPONSECODE_503_SERVICEUNAVAILABLE, 0, *i));
                            (*i)->SendResponse(tmp.get());
                            (*i)->DoneReading(false);
#ifdef SPECIAL_STDERR
                            std::cerr << "EHS Warning: No ressources available. Returning " << tmp.GetStatusString() << "." << std::endl;
#endif
                            EHS_TRACE("Done reading because we are out of ressources", "");
                        }
                        break;
                    default:
                        break;
                }
            } // end nBytesReceived
        } // FD_ISSET
    } // for loop through connections
}

void EHSConnection::AddResponse(ehs_autoptr<GenericResponse> ehs_rvref response)
{
    MutexHelper mutex(&m_oMutex);
    // push the object on to the list
    m_oResponseQueue.push_back(ehs_move(response));
    while (!m_oResponseQueue.empty()) {
        ehs_autoptr<GenericResponse> tmp = ehs_move(m_oResponseQueue.front());
        m_oResponseQueue.pop_front();
        mutex.Unlock();
        SendResponse(tmp.get());
        mutex.Lock();
        // set last activity to the current time for idle purposes
        UpdateLastActivity();
        EHS_TRACE("Sending %d response(s) to %x", m_nResponses, this);
    }
}

void EHSConnection::SendResponse(GenericResponse *gresp)
{
    MutexHelper mutex(&m_oMutex);
    bool forceClose = false;
    int r = 0;

    HttpResponse *response = dynamic_cast<HttpResponse *>(gresp);
    bool processed = false;
    if (NULL == response) {
        // only send it if the client isn't disconnected
        if (Disconnected()) {
            return;
        }
        size_t len = gresp->GetBody().length();
        mutex.Unlock();
        if (0 < len) {
            EHS_TRACE("Sending GENERIC response", "");
            r = m_poNetworkAbstraction->Send(
                    reinterpret_cast<const void *>(gresp->GetBody().data()), gresp->GetBody().length());
            EHS_TRACE("Sent GENERIC response r=%d", r);
        } else {
            // Special case: "sending" a zero-sized body triggers a close.
            DoneReading(false);
        }
    } else {
        processed = true;
        // only send it if the client isn't disconnected
        if (Disconnected()) {
            --m_nActiveRequests;
            ++m_nResponses;
            return;
        }
        EHS_TRACE("Sending HTTP response", "");
        HttpResponse *response = reinterpret_cast<HttpResponse *>(gresp);
        forceClose = ((0 == response->Header("connection").compare("close")) &&
                (HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS != response->GetResponseCode()));
        ostringstream oss;
        // add in the response code
        oss << "HTTP/1.1 " << response->GetResponseCode()
            << " " << HttpResponse::GetPhrase(response->GetResponseCode()) << "\r\n";

        // now go through all the entries in the responseheaders string map
        StringCaseMap::iterator ith = response->GetHeaders().begin();
        while (ith != response->GetHeaders().end()) {
            oss << ith->first << ": " << ith->second << "\r\n";
            ++ith;
        }

        // now push out all the cookies
        StringList::iterator itl = response->GetCookies().begin ( );
        while (itl != response->GetCookies().end()) {
            oss << "Set-Cookie: " << *itl << "\r\n";
            ++itl;
        }

        // extra line break signalling end of headers
        oss << "\r\n";
        mutex.Unlock();
        r = m_poNetworkAbstraction->Send(
                reinterpret_cast<const void *>(oss.str().c_str()), oss.str().length());

        if (-1 != r) {
            // Switch protocols if necessary
            if (HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS == response->GetResponseCode()) {
                EHS_TRACE("Switching connection to RAW mode", "");
                m_bRawMode = true;
                RawSocketHandler *rsh = m_poEHSServer->m_poTopLevelEHS->GetRawSocketHandler();
                if (rsh) {
                    rsh->OnConnect(this);
                }
                mutex.Lock();
                --m_nActiveRequests;
                ++m_nResponses;
                return;
            }

            // now send the body
            int blen = atoi(response->Header("content-length").c_str());
            if (blen > 0) {
                EHS_TRACE("Sending %d bytes in thread %08x", blen, pthread_self());
                int r = m_poNetworkAbstraction->Send(response->GetBody().data(), blen);
                EHS_TRACE("Done sending %d bytes in thread %08x r=%d", blen, pthread_self(), r);
            }
        }
    }
    mutex.Lock();
    if (forceClose || (-1 == r)) {
        DoneReading(false);
    }

    // moved here to avoid race conditions deleting the connection during sending the response
    if(processed) {
        --m_nActiveRequests;
        ++m_nResponses;
    }
}

void EHSServer::EndServerThread()
{
    pthread_mutex_lock(&m_oMutex);
    m_nServerRunningStatus = SERVERRUNNING_NOTRUNNING;
    m_nAcceptThreadId = 0;
    pthread_mutex_unlock(&m_oMutex);
    while (m_nThreads > 0) {
        EHS_TRACE ("Waiting for %d threads to terminate", m_nThreads);
        pthread_cond_broadcast(&m_oDoneAccepting);
        sleep(1);
    }
    EHS_TRACE ("all threads terminated", "");
}

EHS::EHS (EHS *ipoParent, string isRegisteredAs) :
    m_oEHSMap(EHSMap()),
    m_poParent(ipoParent),
    m_sRegisteredAs(isRegisteredAs),
    m_poEHSServer(NULL),
    m_poSourceEHS(NULL),
    m_poBindHelper(NULL),
    m_poRawSocketHandler(NULL),
    m_bNoRouting(false),
    m_oParams(EHSServerParameters())
{
    EHS_TRACE("TID=%p", pthread_self());
}

EHS::~EHS ()
{
    try {
        if (m_poParent) {
            // need to clean up all its registered interfaces
            m_poParent->UnregisterEHS(m_sRegisteredAs.c_str());
        }
    } catch (...) {
        // destructors don't trow
    }
    delete m_poEHSServer;
}

void EHS::RegisterEHS(EHS *ipoEHS, const char *ipsRegisterPath)
{
    ipoEHS->m_poParent = this;
    ipoEHS->m_sRegisteredAs = ipsRegisterPath;
    if (m_oEHSMap[ipsRegisterPath]) {
        throw runtime_error("EHS::RegisterEHS: Already registered");
    }
    m_oEHSMap[ipsRegisterPath] = ipoEHS;
}

void EHS::UnregisterEHS (const char *ipsRegisterPath)
{
    if (!m_oEHSMap[ipsRegisterPath]) {
        throw runtime_error("EHS::UnregisterEHS: Not registered");
    }
    m_oEHSMap.erase(ipsRegisterPath);
}

void EHS::HandleData(int inTimeoutMilliseconds)
{
    // make sure we're in a sane state
    if ((NULL == m_poParent) && (NULL == m_poEHSServer)) {
        throw runtime_error("EHS::HandleData: Invalid state");
    }

    if (m_poParent) {
        m_poParent->HandleData(inTimeoutMilliseconds);
    } else {
        // if we're in single threaded mode, handle data until there are no more jobs left
        if (m_poEHSServer->RunningStatus() == EHSServer::SERVERRUNNING_SINGLETHREADED) {
            do {
                m_poEHSServer->HandleData(inTimeoutMilliseconds, THREADID(pthread_self()));
            } while (m_poEHSServer->RequestsPending() ||
                    m_poEHSServer->AcceptedNewConnection());
        }
    }
}

string GetNextPathPart(string &irsUri)
{
    boost::regex re("^/{0,1}([^/]+)/(.*)$");
    boost::smatch match;
    if (boost::regex_match(irsUri, match, re)) {
        irsUri = match[2];
        return match[1];
    }
    return string("");
}

    ehs_autoptr<HttpResponse>
EHS::RouteRequest(HttpRequest *request ///< request info for service
        )
{
    // get the next path from the URI
    string sNextPathPart;
    if (!m_bNoRouting) {
        sNextPathPart = GetNextPathPart(request->m_sUri);
    }
    // We use an auto_ptr here, so that in case of an exception, the
    // target gets deleted.
    ehs_autoptr<HttpResponse> invalid_response;

    // if there is no more path, call HandleRequest on this EHS object with
    //   whatever's left - or if we're not routing
    if (m_bNoRouting || sNextPathPart.empty()) {
        // create an HttpRespose object for the client
        ehs_autoptr<HttpResponse> response(new HttpResponse(request->m_nRequestId,
                    request->m_poSourceEHSConnection));
        // get the actual response and return code
        if (0 == request->HttpVersion().compare("1.0")) {
            if (0 == request->Headers("Connection").compare("keep-alive")) {
                response->SetHeader("Connection", "keep-alive");
            } else {
                response->SetHeader("Connection", "close");
            }
        }
        response->SetResponseCode(HandleRequest(request, response.get()));
        return ehs_move(response);
    }
    EHS_TRACE ("Trying to route: '%s'", sNextPathPart.c_str());
    // if the path exists, check it against the map of EHSs
    if (m_oEHSMap[sNextPathPart]) {
        // if it exists, call its RouteRequest with the new shortened path
        return ehs_move(m_oEHSMap[sNextPathPart]->RouteRequest(request));
    } else {
        // if it doesn't exist, send an error back up saying resource doesn't exist
        EHS_TRACE("Routing failed. Most likely caused by an invalid URL, not internal error", "");
        // send back a 404 response
        return ehs_move(ehs_autoptr<HttpResponse>(HttpResponse::Error(HTTPRESPONSECODE_404_NOTFOUND, request)));
    }
    throw runtime_error("EHS::RouteRequest: Invalid response.");
    return ehs_move(invalid_response);
}

// default handle request returns time as text
ResponseCode EHS::HandleRequest(HttpRequest *request,
        HttpResponse * response)
{
    // if we have a source EHS specified, use it
    if (m_poSourceEHS != NULL) {
        return m_poSourceEHS->HandleRequest(request, response);
    }
    // otherwise, just send back the current time
    ostringstream oss;
    oss << time(NULL);
    response->SetBody(oss.str().c_str(), oss.str().length());
    response->SetHeader("Content-Type", "text/plain");
    return HTTPRESPONSECODE_200_OK;
}

void EHS::SetSourceEHS(EHS & iroSourceEHS)
{
    m_poSourceEHS = &iroSourceEHS;
}

HttpResponse *EHS::HandleThreadException(ehs_threadid_t tid, HttpRequest *, exception &ex)
{
    std::cerr << "Caught an exception in thread "
        << hex << tid << ": " << ex.what() << endl;
    return NULL;
}

void EHS::AddResponse(ehs_autoptr<GenericResponse> ehs_rvref response)
{
    EHSConnection *conn = response->GetConnection();
    if (conn) {
        conn->AddResponse(ehs_move(response));
    }
}
