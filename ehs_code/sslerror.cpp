/* $Id: sslerror.cpp 72 2011-12-13 11:01:45Z felfert $
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

#include "sslerror.h"
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

bool SslError::s_bMessagesLoaded = false;

SslError::~SslError()
{
    if (s_bMessagesLoaded) {
        ERR_free_strings();
    }
    ERR_remove_state(0);
}

void SslError::ThreadCleanup()
{
    ERR_remove_state(0);
}

int SslError::GetErrorString(string & irsReport, int nError)
{
	// do we need to load the strings?
	if (!s_bMessagesLoaded) {
		SSL_load_error_strings();
		s_bMessagesLoaded = true;
	}

	char psBuffer[256];
	ERR_error_string_n(nError, psBuffer, 256);
	irsReport.append(psBuffer);
    return nError;
}

int SslError::GetError(string & irsReport, bool inPeek)
{
	// get the error code
	unsigned long nError = inPeek ? ERR_peek_error() : ERR_get_error();

	// if there is no error, or we don't want full text, return the error
	//   code now
	if (nError == 0 || irsReport == "noreport") {
		irsReport.clear();
		return nError;
	}

    GetErrorString(irsReport, nError);
    if (!inPeek) {
        unsigned long e;
        while (0 != (e = ERR_get_error())) {
            GetErrorString(irsReport, e);
        }
    }
    return nError;
}

int SslError::PeekError(string & irsReport)
{
	return GetError(irsReport, true);
}

#endif // COMPILE_WITH_SSL
