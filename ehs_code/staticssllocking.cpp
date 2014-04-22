/* $Id: staticssllocking.cpp 163 2013-03-05 13:49:06Z vaxpower $
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

#include "staticssllocking.h"
#include "ehstypes.h"
#include <stdexcept>

using namespace std;

// deal with static variables
pthread_mutex_t * StaticSslLocking::poMutexes;


void StaticSslLocking::SslStaticLockCallback ( int inMode,
        int inMutex,
        const char *,
        int )
{
    if (inMode & CRYPTO_LOCK) {
        pthread_mutex_lock(&StaticSslLocking::poMutexes[inMutex]);
    } else {
        pthread_mutex_unlock(&StaticSslLocking::poMutexes[inMutex]);
    }
}

unsigned long StaticSslLocking::SslThreadIdCallback ( )
{
#ifdef __APPLE__
    // TODO pthread_t is a pointer on OSX, so this conversion is everything else than clean
    return reinterpret_cast<unsigned long>(THREADID(pthread_self()));
#else // __APPLE__
    return static_cast<unsigned long>(THREADID(pthread_self()));
#endif // __APPLE__
}


StaticSslLocking::StaticSslLocking ( )
{
    // allocate the needed number of locks
    StaticSslLocking::poMutexes = new pthread_mutex_t[CRYPTO_num_locks()];

    if ( NULL == StaticSslLocking::poMutexes ) {
        throw runtime_error("StaticSslLocking::StaticSslLocking: Could not allocate CRYPTO mutexes") ;
    }

    // initialize the mutexes
    for (int i = 0; i < CRYPTO_num_locks(); i++) {
        if (0 != pthread_mutex_init(&StaticSslLocking::poMutexes[i], NULL)) {
            throw runtime_error("StaticSslLocking::StaticSslLocking: Could not initialize CRYPTO mutex") ;
        }
    }
    // set callbacks
    CRYPTO_set_id_callback(StaticSslLocking::SslThreadIdCallback);
    CRYPTO_set_locking_callback(StaticSslLocking::SslStaticLockCallback);
}

StaticSslLocking::~StaticSslLocking()
{
    if (NULL == StaticSslLocking::poMutexes) {
        // Destructors don't thow
        // throw runtime_error("StaticSslLocking::~StaticSslLocking: poMutexes is NULL");
    }

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    for ( int i = 0; i < CRYPTO_num_locks(); i++) {
        if (0 != pthread_mutex_destroy(&StaticSslLocking::poMutexes[i])) {
            // Destructors don't thow
            // throw runtime_error("StaticSslLocking::~StaticSslLocking: Could not destroy CRYPTO mutex");
        }
    }
    delete [] StaticSslLocking::poMutexes;
    StaticSslLocking::poMutexes = NULL;
}

#endif // COMPILE_WITH_SSL
