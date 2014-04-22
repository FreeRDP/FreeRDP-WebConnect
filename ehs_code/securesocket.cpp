/* $Id: securesocket.cpp 151 2012-06-07 15:43:04Z felfert $
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

#ifdef COMPILE_WITH_SSL

#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif

#include "ehs.h"
#include "securesocket.h"
#include "debug.h"

#include "mutexhelper.h"

#include <iostream>
#include <sstream>
#include <cerrno>
#include <stdexcept>

using namespace std;

// static class variables
DynamicSslLocking * SecureSocket::s_pDynamicSslLocking = NULL;
StaticSslLocking * SecureSocket::s_pStaticSslLocking = NULL;
SslError * SecureSocket::s_pSslError = NULL;
SSL_CTX * SecureSocket::s_pSslCtx = NULL;
int SecureSocket::s_refcount = 0;
pthread_mutex_t SecureSocket::s_mutex = PTHREAD_MUTEX_INITIALIZER;

SecureSocket::SecureSocket (const std::string & certfile,
        PassphraseHandler *handler) : 
    Socket(),
    m_pSsl(NULL),
    m_sCertFile(certfile),
    m_poPassphraseHandler(handler)
{ 
    pthread_mutex_lock(&s_mutex);
    s_refcount++;
    pthread_mutex_unlock(&s_mutex);
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket constructor A" << std::endl;
#endif
}

SecureSocket::SecureSocket (SSL *ssl, ehs_socket_t fd, sockaddr_in *peer) :
    Socket(fd, peer),
    m_pSsl(ssl),
    m_sCertFile(""),
    m_poPassphraseHandler(NULL)
{
    pthread_mutex_lock(&s_mutex);
    s_refcount++;
    pthread_mutex_unlock(&s_mutex);
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket constructor B" << std::endl;
#endif
}

SecureSocket::~SecureSocket ( )
{
#ifdef EHS_DEBUG
    std::cerr << "calling SecureSocket destructor" << std::endl;
#endif
    Close();
    if (m_pSsl) {
        SSL_free(m_pSsl);
    }

    pthread_mutex_lock(&s_mutex);
    int rc = --s_refcount;
    pthread_mutex_unlock(&s_mutex);
    if (0 == rc) {
#ifdef EHS_DEBUG
        std::cerr << "Deleting static members" << std::endl;
#endif
        if (NULL != s_pSslCtx) {
            SSL_CTX_free(s_pSslCtx);
        }
        delete s_pSslError;
        CRYPTO_cleanup_all_ex_data();
        EVP_cleanup();
        delete s_pStaticSslLocking;
        delete s_pDynamicSslLocking;
    }
}

void SecureSocket::ThreadCleanup()
{
    if (s_pSslError)
        s_pSslError->ThreadCleanup();
}

void SecureSocket::Init(int port)
{
    MutexHelper mh(&s_mutex);

    // Initializes OpenSSL
    SSL_library_init();

    if (NULL == s_pDynamicSslLocking) {
        s_pDynamicSslLocking = new DynamicSslLocking;
    }
    if (NULL == s_pStaticSslLocking) {
        s_pStaticSslLocking = new StaticSslLocking;
    }
    if (NULL == s_pSslError) {
        s_pSslError = new SslError;
    }
    if (NULL == s_pSslCtx) {
        s_pSslCtx = InitializeCertificates();
    }

    return Socket::Init(port);
}

NetworkAbstraction *SecureSocket::Accept() 
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
        switch (errno) {
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
    }

    // TCP connection is ready. Do server side SSL.
    SSL *ssl = SSL_new(s_pSslCtx);
    if (NULL == ssl) {
        s_pSslError->GetError(sError);
#ifdef _WIN32
        closesocket(fd);
        WSACleanup();
#else
        close(fd);
#endif
        throw runtime_error(sError.insert(0, "Error while creating SSL session context: "));
    }
    SSL_set_fd(ssl, fd);
retry_handshake:
    int ret = SSL_accept(ssl);
    if (1 != ret) {
        ostringstream oss;
        int syserr = errno;
        int sslerr = SSL_get_error(ssl, ret);
        switch (sslerr) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_ACCEPT:
                goto retry_handshake;
                break;
            case SSL_ERROR_SYSCALL:
                if ((syserr == EAGAIN) || (syserr == EINTR)) {
                    goto retry_handshake;
                }
                s_pSslError->GetError(sError);
                oss << "Error during SSL handshake: " << sError
                    << " (syscall:" << ::strerror(syserr) << ") from " << GetPeer();
                break;
            default:
                s_pSslError->GetError(sError);
                oss << "Error during SSL handshake: " << sError
                    << " (SSL_err:" << sslerr << ") from " << GetPeer();
        }
        SSL_free(ssl);
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        throw runtime_error(oss.str());
    }

    return new SecureSocket(ssl, fd, &m_peer);
}

int PeerCertificateVerifyCallback (int inOk, X509_STORE_CTX *
#ifdef EHS_DEBUG
        ipoStore
#endif
        )
{

#ifdef EHS_DEBUG
    if (!inOk) {
        char psBuffer [ 256 ];
        X509 * poCertificate = X509_STORE_CTX_get_current_cert(ipoStore);
        int nDepth = X509_STORE_CTX_get_error_depth(ipoStore);
        int nError = X509_STORE_CTX_get_error(ipoStore);

        cerr << "Error in certificate at depth: " << nDepth << endl;
        X509_NAME_oneline(X509_get_issuer_name(poCertificate), psBuffer, 256);
        cerr << "  issuer  = '" << psBuffer << "'" << endl;
        X509_NAME_oneline(X509_get_subject_name(poCertificate), psBuffer, 256);
        cerr << "  subject = '" << psBuffer << "'" << endl;
        cerr << "  error " << nError << "," <<  X509_verify_cert_error_string(nError) << endl;
    }
#endif

    return inOk;
}

SSL_CTX *SecureSocket::InitializeCertificates()
{
    string sError;
    // set up the CTX object in compatibility mode.
    //   We'll remove SSLv2 compatibility later for security reasons
    SSL_CTX *ret = SSL_CTX_new(SSLv23_method());
    if (NULL == ret) {
        s_pSslError->GetError(sError);
        throw runtime_error(sError.insert(0, "Could not initialize SSL: "));
    }

#ifdef EHS_DEBUG
    if (m_sCertFile.empty()) {
        cerr << "No filename for server certificate specified" << endl;
    } else {
        cerr << "Using '" << m_sCertFile << "' for certificate" << endl;
    }
#endif


#if 0
    // this sets default locations for trusted CA certificates.  This is not
    //   necessary since we're not dealing with client-side certificates
    if ((!m_sCertFile.empty() || !m_sCertificateDirectory.empty())) {
        if (SSL_CTX_load_verify_locations(poCtx, 
                    m_sCertFile.c_str(),
                    m_sCertificateDirectory.empty() ? NULL : m_sCertificateDirectory.c_str()) != 1 ) {
            s_pSslError.GetError(sError);
            cerr << "Error loading trusted certs: '" << sError << "'" << endl;
            delete poCtx;
            return NULL;
        }
    }

    // Unknown what this does
    if (SSL_CTX_set_default_verify_paths(poCtx) != 1) {
        cerr << "Error loading default certificate file and/or directory" << endl;
        return NULL;
    }
#endif

    // set the callback
    SSL_CTX_set_default_passwd_cb(ret, PassphraseCallback);
    // set the data
    SSL_CTX_set_default_passwd_cb_userdata(ret, reinterpret_cast<void *>(this));

    if (m_sCertFile.empty()) {
        throw runtime_error("No certificate specified.");
    } else {
        if (SSL_CTX_use_certificate_chain_file(ret, m_sCertFile.c_str()) != 1) {
            s_pSslError->GetError(sError);
            SSL_CTX_free(ret);
            throw runtime_error(sError.insert(0, "Error while loading certificate: "));
        }
        if (1 != SSL_CTX_use_PrivateKey_file(ret, m_sCertFile.c_str(), SSL_FILETYPE_PEM)) {
            s_pSslError->GetError(sError);
            SSL_CTX_free(ret);
            throw runtime_error(sError.insert(0, "Error while loading private key: "));
        }
    }

    SSL_CTX_set_verify(ret, 0, PeerCertificateVerifyCallback);

    //SSL_CTX_set_verify_depth(s_pSslCtx, 4);

    // set all workarounds for buggy clients and turn
    // off SSLv2 because it's insecure.
    SSL_CTX_set_options(ret, SSL_OP_ALL|SSL_OP_NO_SSLv2);

    if (1 != SSL_CTX_set_cipher_list(ret, CIPHER_LIST)) {
        s_pSslError->GetError(sError);
        SSL_CTX_free(ret);
        throw runtime_error(sError.insert(0, "Error while setting cipher list: "));
    }

    return ret;
}


int SecureSocket::Read(void *buf, int bufsize)
{
    int ret = 0;
    if (bufsize > 0) {
again:
        ret = SSL_read(m_pSsl, buf, bufsize);
        if (ret <= 0) {
            switch (SSL_get_error(m_pSsl, ret)) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    goto again;
                    break;
                case SSL_ERROR_SYSCALL:
                    if ((errno == EAGAIN) || (errno == EINTR)) {
                        goto again;
                    }
                    break;
            }
        }
    }
    return ret;
}

int SecureSocket::Send (const void *buf, size_t buflen, int)
{
    int ret = 0;
    if (buflen > 0) {
again:
        ret = SSL_write(m_pSsl, buf, buflen);
        if (ret <= 0) {
            switch (SSL_get_error(m_pSsl, ret)) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    goto again;
                    break;
                case SSL_ERROR_SYSCALL:
                    if ((errno == EAGAIN) || (errno == EINTR)) {
                        goto again;
                    }
                    break;
            }
        }
    }
    return ret;
}

void SecureSocket::Close()
{
    Socket::Close();
}

    int 
SecureSocket::PassphraseCallback(char * buf, int bufsize, int rwflag, void * userdata)
{
#ifdef EHS_DEBUG
    cerr << "Invoking certificate passphrase callback" << endl;
#endif
    SecureSocket *s = reinterpret_cast<SecureSocket *>(userdata);
    int ret = 0;
    if (NULL != s->m_poPassphraseHandler) {
        string passphrase = s->m_poPassphraseHandler->GetPassphrase((1 == rwflag));
        ret = passphrase.length();
        if (ret > 0) {
            ret = ((bufsize - 1) < ret) ? (bufsize - 1): ret;
            strncpy(buf, passphrase.c_str(), ret);
            buf[ret + 1] = 0;
        }
    }
    return ret;
}

#endif // COMPILE_WITH_SSL
