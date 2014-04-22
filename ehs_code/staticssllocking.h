/* $Id: staticssllocking.h 28 2010-06-05 03:20:36Z felfert $
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

#ifndef STATIC_SSL_LOCKING_H
#define STATIC_SSL_LOCKING_H

#ifdef COMPILE_WITH_SSL

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <pthread.h>

/// static locking mechanism for OpenSSL
class StaticSslLocking
{

    public:

        /// constructor
        StaticSslLocking ( );

        /// destructor
        ~StaticSslLocking ( );

        /// static mutex array
        static pthread_mutex_t * poMutexes;

    protected:

        /// callback for locking/unlocking a mutex in the array
        static void SslStaticLockCallback ( int inMode, 
                int inMutex,
                const char * ipsFile,
                int inLine );

        /// returns the id of the thread from which it is called
        static unsigned long SslThreadIdCallback ( );

};

#endif // COMPILE_WITH_SSL

#endif // STATIC_SSL_LOCKING_H
