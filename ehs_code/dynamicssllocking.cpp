/* $Id: dynamicssllocking.cpp 69 2011-11-22 15:03:58Z felfert $
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

#include "dynamicssllocking.h"
#include <stdexcept>

using namespace std;

CRYPTO_dynlock_value * DynamicSslLocking::DynamicLockCreateCallback ( const char *, int )
{
	CRYPTO_dynlock_value *poDynlock = new CRYPTO_dynlock_value;

    if (NULL == poDynlock) {
        throw runtime_error ( "DynamicSslLocking::DynamicLockCreateCallback: Could not allocate dynamic CRYPTO mutex" );
    }
	if (0 != pthread_mutex_init(&poDynlock->oMutex, NULL)) {
        throw runtime_error ( "DynamicSslLocking::DynamicLockCreateCallback: Could not initialize dynamic CRYPTO mutex" );
    }
	return poDynlock;

}

void DynamicSslLocking::DynamicLockCallback ( int inMode,
											  CRYPTO_dynlock_value * ipoDynlock,
											  const char *,
											  int )
{
	if ( inMode & CRYPTO_LOCK ) {
		pthread_mutex_lock(&ipoDynlock->oMutex);
	} else {
		pthread_mutex_unlock ( &ipoDynlock->oMutex );
	}
}

void DynamicSslLocking::DynamicLockCleanupCallback ( struct CRYPTO_dynlock_value * ipoDynlock,
													 const char * ,
													 int )
{
	if (0 != pthread_mutex_destroy ( &ipoDynlock->oMutex )) {
        throw runtime_error("DynamicSslLocking::DynamicLockCleanupCallback: Could not destroy dynamic CRYPTO mutex");
    }
	delete ipoDynlock;
}


DynamicSslLocking::DynamicSslLocking ( )
{
	CRYPTO_set_dynlock_create_callback(DynamicLockCreateCallback);
	CRYPTO_set_dynlock_lock_callback(DynamicLockCallback);
	CRYPTO_set_dynlock_destroy_callback(DynamicLockCleanupCallback);
}

DynamicSslLocking::~DynamicSslLocking ( )
{
	CRYPTO_set_dynlock_create_callback(NULL);
	CRYPTO_set_dynlock_lock_callback(NULL);
	CRYPTO_set_dynlock_destroy_callback(NULL);
}

#endif // COMPILE WITH SSL
