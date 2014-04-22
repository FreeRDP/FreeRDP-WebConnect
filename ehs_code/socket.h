/* $Id: socket.h 119 2012-04-05 21:49:58Z felfert $
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

#ifndef SOCKET_H
#define SOCKET_H

#ifdef _MSC_VER
# pragma warning(disable : 4786)
#endif

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif

#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include <cstdio>

#ifdef _WIN32
typedef unsigned long in_addr_t;
typedef size_t socklen_t;
# define sleep(seconds) (Sleep(seconds * 1000))
#endif

extern const char *net_strerror();
#ifdef _WIN32
# define net_errno WSAGetLastError()
#else
# define INVALID_SOCKET -1
# define net_errno errno
#endif

#include "networkabstraction.h"

/// plain socket implementation of NetworkAbstraction
class Socket : public NetworkAbstraction {

    private:

        Socket(const Socket &);

        Socket & operator=(const Socket &);

    protected:
        /**
         * Constructs a new Socket, connected to a client.
         * @param fd The socket descriptor of this connection.
         * @param peer The peer address of this socket
         */
        Socket(ehs_socket_t fd, sockaddr_in *peer);

    public:

        /**
         * Default constructor
         */
        Socket();

        virtual void RegisterBindHelper(PrivilegedBindHelper *helper);

        virtual void Init(int port);

        virtual ~Socket();

        virtual void SetBindAddress(const char * bindAddress);

        virtual ehs_socket_t GetFd() const { return m_fd; }

        virtual int Read(void *buf, int bufsize);

        virtual int Send(const void *buf, size_t buflen, int flags = 0);

        virtual void Close();

        virtual NetworkAbstraction *Accept();

        /// Determines, whether the underlying socket is secure.
        /// @return false, because this instance does not use SSL.
        virtual bool IsSecure() const { return false; }

        virtual void ThreadCleanup() { }

    protected:

        int GetLocalPort() const;

        int GetRemotePort() const;

        std::string GetLocalAddress() const;

        std::string GetRemoteAddress() const;

        std::string GetPeer() const;

        /// The file descriptor of the socket on which this connection came in
        ehs_socket_t m_fd;

        /// Stores the peer address of the current connection
        sockaddr_in m_peer;

        /// Stores the bind address
        sockaddr_in m_bindaddr;

        /// Our bind helper
        PrivilegedBindHelper *m_pBindHelper;

};

#endif // SOCKET_H
