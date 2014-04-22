/* $Id: networkabstraction.h 120 2012-04-05 22:03:59Z felfert $
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

#ifndef NETWORK_ABSTRACTION_H
#define NETWORK_ABSTRACTION_H

#include <string>
#include <cstdlib>

class PrivilegedBindHelper;

#ifdef _WIN32
typedef SOCKET ehs_socket_t;
#else
typedef int ehs_socket_t;
#endif

/**
 * Abstracts different socket types.
 * This interface abstracts the differences between normal
 * sockets and SSL sockets. There are two implementations:
 * <ul>
 *  <li>Socket is the standard socket<br>
 *  <li>SecureSocket is the SSL implementation<br>
 * </ul>
 */
class NetworkAbstraction {

    public:

        /**
         * Registers a PrivilegedBindHelper for use by this instance.
         * @param helper The PrivilegedBindHelper to be used by this instance.
         */
        virtual void RegisterBindHelper(PrivilegedBindHelper *helper) = 0;

        /**
         * Sets the bind address of the socket.
         * @param bindAddress The address to bind to in quad-dotted format.
         */
        virtual void SetBindAddress(const char * bindAddress) = 0;

        /**
         * Retrieves the peer address.
         * @return The address of the connected peer in quad-dotted format.
         */
        virtual std::string GetRemoteAddress() const = 0;

        /**
         * Retrieves the peer's port of a connection.
         * @return The peer port.
         */
        virtual int GetRemotePort() const = 0;

        /**
         * Retrieves the peer address.
         * @return The address of the connected peer in quad-dotted format.
         */
        virtual std::string GetLocalAddress() const = 0;

        /**
         * Retrieves the peer's port of a connection.
         * @return The peer port.
         */
        virtual int GetLocalPort() const = 0;

        DEPRECATED("Use GetRemoteAddress()")
            /**
             * Retrieves the peer address.
             * @return The address of the connected peer in quad-dotted format.
             * @deprecated Use GetRemoteAddress()
             */
            virtual std::string GetAddress() const { return GetRemoteAddress(); }

        DEPRECATED("Use GetRemotePort()")
            /**
             * Retrieves the peer's port of a connection.
             * @return The peer port.
             * @deprecated Use GetRemotePort()
             */
            virtual int GetPort() const { return GetRemotePort(); }

        /**
         * Combination of GetRemoteAddress and GetRemotePort.
         * @return The address of the connected peer in quad-dotted format,
         *   followed by the port, separated by a colon.
         */
        virtual std::string GetPeer() const = 0;

        /**
         * Initializes a listening socket.
         * If listening should be restricted to a specific address, SetBindAddress
         * has to be called in advance.
         * @param port The port to listen on.
         * @throws A std:runtime_error if initialization fails.
         */
        virtual void Init(int port) = 0;

        /// Destructor
        virtual ~NetworkAbstraction() { }

        /**
         * Retrieves the underlying file descriptor.
         * @return The FD/Socket of the listening socket.
         */
        virtual ehs_socket_t GetFd() const = 0;

        /**
         * Performs a read from the underlying socket.
         * @param buf Pointer to a buffer that receives the incoming data.
         * @param bufsize The maximum number of bytes to read.
         * @return The actual number of bytes that have been received or -1 if an error occured.
         */
        virtual int Read(void *buf, int bufsize) = 0;

        /**
         * Performs a send on the underlying socket.
         * @param buf Pointer to the data to be sent.
         * @param buflen The number of bytes to send.
         * @param flags Additional flags for the system call.
         * @return The actual number of byte that have been sent or -1 if an error occured.
         */
        virtual int Send(const void *buf, size_t buflen, int flags = 0) = 0;

        /// Closes the underlying socket.
        virtual void Close() = 0;

        /** Waits for an incoming connection.
         * @return A new NetworkAbstraction instance which represents the client connetion.
         * @throws A std:runtime_error on failure.
         */
        virtual NetworkAbstraction *Accept() = 0;

        /// Determines, whether the underlying socket is socure.
        /// @return true, if SSL is used; false otherwise.
        virtual bool IsSecure() const = 0;

        /// Handles thread specific clean up (used by OpenSSL).
        virtual void ThreadCleanup() = 0;
};

#endif // NETWORK_ABSTRACTION_H
