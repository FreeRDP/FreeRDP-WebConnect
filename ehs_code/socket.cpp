/* $Id: socket.cpp 151 2012-06-07 15:43:04Z felfert $
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

/*
 * Solaris 2.6 troubles
 */

#ifdef sun
#ifndef BSD_COMP 
#define BSD_COMP 1
#endif
#ifndef _XPG4_2
#define _XPG4_2 1
#endif
#ifndef __EXTENSIONS__
#define __EXTENSIONS__ 1
#endif
#ifndef __PRAGMA_REDEFINE_EXTNAME
#define __PRAGMA_REDEFINE_EXTNAME 1
#endif
#endif // sun

#include "ehs.h"
#include "socket.h"
#include "debug.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 // no support
#endif // MSG_NOSIGNAL

#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <stdexcept>

#ifdef _WIN32
const char *net_strerror() {
    static char ret[1024];
    int err = net_errno;
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_FROM_HMODULE,
            GetModuleHandleA("wsock32.dll"), err, 0, ret, sizeof(ret), NULL)) {
#ifdef _WIN32
		_snprintf(ret, sizeof(ret), "Unknown error %d (0x%08x)", err, err);
#else
        snprintf(ret, sizeof(ret), "Unknown error %d (0x%08x)", err, err);
#endif
    }
    return ret;
}
#else
const char *net_strerror() {
    return strerror(net_errno);
}
#endif

using namespace std;

    Socket::Socket()
  : m_fd(-1),
    m_peer(sockaddr_in()),
    m_bindaddr(sockaddr_in()),
    m_pBindHelper(NULL)
{
    memset(&m_peer, 0, sizeof(m_peer));
    memset(&m_bindaddr, 0, sizeof(m_bindaddr));
}

Socket::Socket(ehs_socket_t fd, sockaddr_in * peer) :
    m_fd(fd),
    m_peer(*peer),
    m_bindaddr(sockaddr_in()),
    m_pBindHelper(NULL)
{
    memcpy(&m_peer, peer, sizeof(m_peer));
    memset(&m_bindaddr, 0, sizeof(m_bindaddr));
    socklen_t len = sizeof(m_bindaddr);
    getsockname(fd, reinterpret_cast<struct sockaddr *>(&m_bindaddr),
#ifdef _WIN32
            reinterpret_cast<int *>(&len) 
#else
            &len 
#endif
            );
}

Socket::~Socket()
{
    Close();
}

void Socket::RegisterBindHelper(PrivilegedBindHelper *helper)
{
    m_pBindHelper = helper;
}

void Socket::SetBindAddress(const char *bindAddress)
{
    in_addr_t baddr =
        (bindAddress && strlen(bindAddress)) ? inet_addr(bindAddress) : INADDR_ANY;
    memcpy(&(m_bindaddr.sin_addr), &baddr, sizeof(m_bindaddr.sin_addr));
}

void Socket::Init(int port) 
{
    string sError;
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);

    // Found a usable winsock dll?
    if (0 != WSAStartup(wVersionRequested, &wsaData)) {
        throw runtime_error("Socket::Init: WSAStartup() returned an error");
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion)) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        throw runtime_error("Socket::Init: Could not find a suitable (v 2.2) Winsock DLL");
    }

    /* The WinSock DLL is acceptable. Proceed. */

#endif // End WIN32-specific network initialization code

    // need to create a socket for listening for new connections
    if (INVALID_SOCKET != m_fd) {
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error("Socket::Init: Socket already initialized");
    }

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == m_fd) {
        sError.assign("socket: ").append(net_strerror());
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error(sError);
    }

#ifdef _WIN32
    u_long one = 1;
    ioctlsocket(m_fd, FIONBIO, &one);
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&one), sizeof(int));
#else
    int one = 1;
    ioctl(m_fd, FIONBIO, &one);
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void *>(&one), sizeof(int));
#endif

    // bind the socket to the appropriate port
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    int nResult = -1;

    sa.sin_family = AF_INET;
    memcpy(&(sa.sin_addr), &m_bindaddr.sin_addr, sizeof(sa.sin_addr));
    sa.sin_port = htons(port);

    if ((1024 > port) && m_pBindHelper) {
        struct in_addr in;
        memcpy(&in, &m_bindaddr.sin_addr, sizeof(in));
        string ba(inet_ntoa(in));
        nResult = m_pBindHelper->BindPrivilegedPort(m_fd, ba.c_str(), port) ? 0 : -1;
    } else {
        nResult = ::bind(m_fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa));
    }

    if (-1 == nResult) {
        sError.assign("bind: ").append(net_strerror());
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error(sError);
    }

    // listen 
    nResult = listen(m_fd, 20);
    if (0 != nResult) {
        sError.assign("listen: ").append(net_strerror());
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error(sError);
    }
}


int Socket::Read(void *buf, int bufsize)
{
    int ret = 0;
    if (bufsize > 0) {
again:
        ret = recv(m_fd, 
#ifdef _WIN32
                reinterpret_cast<char *>(buf),
#else
                buf, 
#endif
                bufsize, 0);
        if (ret < 0) {
            int err = net_errno;
            switch (err) {
                case EAGAIN:
                case EINTR:
#ifdef _WIN32
                case WSAEWOULDBLOCK:
#endif
                    goto again;
                    break;
#ifdef _WIN32
                default:
                    EHS_TRACE("NET_ERR: %d", err);
                    break;
#endif
            }
        }
    }
    return ret;
}

int Socket::Send(const void *buf, size_t buflen, int flags)
{
    int ret = 0;
    if (buflen > 0) {
again:
        ret = send(m_fd, 
#ifdef _WIN32
                reinterpret_cast<const char *>(buf),
#else
                buf, 
#endif	
                buflen, flags|MSG_NOSIGNAL);
        if (ret < 0) {
            switch (net_errno) {
                case EAGAIN:
                case EINTR:
#ifdef _WIN32
                case WSAEWOULDBLOCK:
#endif
                    goto again;
            }
        }
    }
    return ret;
}

void Socket::Close()
{
    if (INVALID_SOCKET == m_fd)
        return;
#ifdef _WIN32
    closesocket(m_fd);
#else
    close(m_fd);
#endif
}

NetworkAbstraction *Socket::Accept()
{
    string sError;
    socklen_t addrlen = sizeof(m_peer);
retry:
    ehs_socket_t fd = accept(m_fd, reinterpret_cast<sockaddr *>(&m_peer),
#ifdef _WIN32
            reinterpret_cast<int *>(&addrlen) 
#else
            &addrlen 
#endif
            );
    EHS_TRACE("Got a connection from %s:%hu\n",
            GetRemoteAddress().c_str(), ntohs(m_peer.sin_port));

    if (INVALID_SOCKET == fd) {
        switch (net_errno) {
            case EAGAIN:
            case EINTR:
#ifdef _WIN32
            case WSAEWOULDBLOCK:
#endif
                goto retry;
                break;
        }
        sError.assign("accept: ").append(net_strerror());
#ifdef _WIN32
        WSACleanup();
#endif
        throw runtime_error(sError);
        return NULL;
    }
    return new Socket(fd, &m_peer);
}

string Socket::GetPeer() const
{
    char buf[20];
    string ret(GetRemoteAddress());
#ifdef _WIN32
	_snprintf(buf, 20, ":%d", GetRemotePort());
#else
    snprintf(buf, 20, ":%d", GetRemotePort());
#endif
    ret.append(buf);
    return ret;
}

string Socket::GetRemoteAddress() const
{
    struct in_addr in;
    memcpy (&in, &m_peer.sin_addr.s_addr, sizeof(in));
    return string(inet_ntoa(in));
}


int Socket::GetRemotePort() const
{
    return ntohs(m_peer.sin_port);
}

string Socket::GetLocalAddress() const
{
    struct in_addr in;
    memcpy (&in, &m_bindaddr.sin_addr.s_addr, sizeof(in));
    return string(inet_ntoa(in));
}

int Socket::GetLocalPort() const
{
    return ntohs(m_bindaddr.sin_port);
}

