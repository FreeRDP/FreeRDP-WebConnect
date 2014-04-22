/* $Id: sslerror.h 71 2011-12-13 10:20:45Z felfert $
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

#ifndef SSL_ERROR
#define SSL_ERROR

#ifdef COMPILE_WITH_SSL

#include <string>

/**
 * A wrapper for the OpenSSL error mechanism.
 */
class SslError 
{

    public:

        /// Destructor
        ~SslError();

        /**
         * Retrieves info about the last OpenSSL error and optionally
         * removes that info from the OpenSSL error queue.
         * @param irsReport Gets filled with the OpenSSL error info.
         * @param inPeek If true, the info is removed from the OpenSSL error queue.
         * @return The OpenSSL error code (0 == no error).
         */
        int GetError(std::string & irsReport, bool inPeek = false);

        /**
         * Retrieves info about the OpenSSL error specified in nError.
         * @param irsReport Gets filled with the OpenSSL error info.
         * @param nError The error to get info for.
         * @return The OpenSSL error code (nError).
         */
        int GetErrorString(std::string & irsReport, int nError);

        /**
         * Retrieves info about the last OpenSSL error and leaves it on OpenSSL error queue.
         * @param irsReport Gets filled with the OpenSSL error info.
         * @return The OpenSSL error code (0 == no error).
         */
        int PeekError(std::string & irsReport);

        void ThreadCleanup();

    protected:

        /// Flag: Represents whether the OpenSSL error strings have been loaded.
        static bool s_bMessagesLoaded;
};

#endif // COMPILE_WITH_SSL

#endif // SSL_ERROR

