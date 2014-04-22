/* $Id: dynamicssllocking.h 28 2010-06-05 03:20:36Z felfert $
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

#ifndef DYNAMIC_SSL_LOCKING
#define DYNAMIC_SSL_LOCKING

#include <openssl/crypto.h>
#include <openssl/ssl.h>

#include <pthread.h>

/// used for dynamic locking callback
struct CRYPTO_dynlock_value
{
    pthread_mutex_t oMutex;
};


/// dynamic locking mechanism for OpenSSL.  Created as needed.
class DynamicSslLocking
{
    public:

        /// default constructor
        DynamicSslLocking ( );

        /// destructor
        ~DynamicSslLocking ( );


    protected:

        /// callback for creating a CRYPTO_dynlock_value object for later use
        static CRYPTO_dynlock_value * DynamicLockCreateCallback ( const char * ipsFile,
                int inLine );

        /// callback for locking/unlocking CRYPTO_dynlock_value
        static void DynamicLockCallback ( int inMode,
                CRYPTO_dynlock_value * ipoDynlock,
                const char * ipsFile,
                int inLine );

        /// callback for freeing CRYPTO_dynlock_value
        static void DynamicLockCleanupCallback ( struct CRYPTO_dynlock_value * ipoDynlock,
                const char * ipsFile,
                int inLine );

};

#endif
