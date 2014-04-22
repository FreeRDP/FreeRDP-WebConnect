/* $Id: securesocket.h 95 2012-03-31 21:08:13Z felfert $
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

#ifndef SECURE_SOCKET_H
#define SECURE_SOCKET_H

#ifdef COMPILE_WITH_SSL

#include <openssl/ssl.h>
#include <openssl/rand.h>

#include <cstring>
#include <string>
#include <iostream>

#include "socket.h"
#include "dynamicssllocking.h"
#include "staticssllocking.h"
#include "sslerror.h"


/** use all cipers except adh=anonymous ciphers, low=64 or 54-bit cipers,
 * exp=export crippled ciphers, or md5-based ciphers that have known weaknesses
 * @STRENGTH means order by number of bits
 */
#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

class PassphraseHandler;

/// Secure socket implementation used for HTTPS
class SecureSocket : public Socket 
{
    private:
        SecureSocket(const SecureSocket &);

        SecureSocket & operator=(const SecureSocket &);

    public:

        virtual void Init(int port);

        /**
         * Constructs a new listener socket.
         * @param certfile The filename of the server certificate to use.
         * @param handler The PassphraseHandler implementation to use.
         */
        SecureSocket(const std::string & certfile = "",
                PassphraseHandler *handler = NULL);

        /// Destructor
        virtual ~SecureSocket();

        virtual NetworkAbstraction *Accept();

        /// Determines, whether the underlying socket is secure.
        /// @return true because this socket is considered secure.
        virtual bool IsSecure() const { return true; }

        virtual int Read(void *buf, int bufsize);

        virtual int Send(const void *buf, size_t buflen, int flags = 0);

        virtual void Close();

        virtual void ThreadCleanup();

    private:

        /**
         * Passphrase callback for OpenSSL.
         * Our implementation uses userdata for obtaining an instance pointer and then
         * simply invokes the GetPassphrase method of our PassphraseHandler. 
         * @param buf The buffer that shall be filled with the passphrase.
         * @param bufsize Maximum size of the buffer.
         * @param rwflag Set to nonzero, if the request is for encryption. This implies
         *   asking the user for a passphrase twice.
         * @param userdata An opaque pointer to user data.
         */
        static int PassphraseCallback(char * buf, int bufsize, int rwflag, void * userdata);

        /**
         * Constructs a new Socket, connected to a client.
         * @param ssl The OpenSSL context, associated with this instance.
         * @param fd The socket descriptor of this connection.
         * @param peer The peer address of this socket
         */
        SecureSocket(SSL *ssl, ehs_socket_t fd, sockaddr_in *peer);

        /// Initializes the SSL context and provides it with certificates.
        SSL_CTX *InitializeCertificates();

    protected:

        /// The OpenSSL instance associated with this socket instance.
        SSL *m_pSsl;

        /// The filename of the certificate file
        std::string m_sCertFile;

        /// The PassPhraseHandler to use for retrieving certificate passphrases.
        PassphraseHandler * m_poPassphraseHandler;

    private:

        /// Dynamic portion of the SSL locking mechanism.
        static DynamicSslLocking * s_pDynamicSslLocking;

        /// Static portion of the SSL locking mechanism.
        static StaticSslLocking * s_pStaticSslLocking;

        /// Error object for getting openssl error messages
        static SslError * s_pSslError;

        /// Internal OpenSSL context
        static SSL_CTX * s_pSslCtx;

        /// Internal reference counter for releasing OpenSSL ressources.
        static int s_refcount;

        /// Mutex for Init() an s_refcount;
        static pthread_mutex_t s_mutex;
};

#endif // COMPILE_WITH_SSL

#endif // SECURE_SOCKET_H
