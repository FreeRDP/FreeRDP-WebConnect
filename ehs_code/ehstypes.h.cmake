/* $Id: ehstypes.h.in 132 2012-04-15 18:25:58Z felfert $
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

#ifndef EHSTYPES_H
#define EHSTYPES_H

#include <string>
#include <cstring>
#include <memory>
#include <map>
#include <deque>
#include <list>

class EHSServer;
class EHSConnection;
class EHS;
class Datum;
class FormValue;
class HttpResponse;
class HttpRequest;


#define strcasecmp @STRCASE_CMP_REDEFINITION@

/**
 * Caseless Compare class for case insensitive map
 */
struct __caseless
{
    /**
     * case-insensitive comparator
     */
    bool operator() ( const std::string & s1, const std::string & s2 ) const
    {
        return strcasecmp( s1.c_str(), s2.c_str() ) < 0;
    }
};

#if @USE_UNIQUE_PTR@
# define ehs_autoptr std::unique_ptr
# define ehs_move(x) std::move(x)
# define ehs_rvref &&
#else
# include <boost/shared_ptr.hpp>
# define ehs_autoptr boost::shared_ptr
# define ehs_move(x) (x)
# define ehs_rvref
#endif

#define DEPRECATED(x) @DEPRECATED@

#ifdef _WIN32
#include <pthread.h>
typedef unsigned long ehs_threadid_t;
extern ehs_threadid_t THREADID(pthread_t t);
#else
typedef pthread_t ehs_threadid_t;
#define THREADID
#endif

/**
 * This class represents what is sent back to the client.
 * It contains the actual body only.
 * Any HTTP specific additions like headers or the response
 * code are handled in the drived class HttpResponse.
 */
class GenericResponse {
    private:
        GenericResponse( const GenericResponse & );
        GenericResponse &operator = (const GenericResponse &);

    public:
        /**
         * Constructs a new instance.
         * @param inResponseId A unique Id (normally derived from the corresponding request Id).
         * @param ipoEHSConnection The connection, on which this response should be sent.
         */
        GenericResponse(int inResponseId, EHSConnection * ipoEHSConnection)
            : m_nResponseId(inResponseId)
            , m_sBody("")
            , m_poEHSConnection(ipoEHSConnection)
    { }

        /**
         * Sets the body of this instance.
         * @param ipsBody The content to set.
         * @param inBodyLength The length of the body.
         */
        void SetBody(const char *ipsBody, size_t inBodyLength) {
            m_sBody = std::string(ipsBody, inBodyLength);
        }

        /**
         * retrieves the body of this response.
         * @return The current content of the body.
         */
        std::string & GetBody() { return m_sBody; };

        /**
         * retrieves the EHSConnection, on which this response
         * is supposed to be send.
         * @return The EHSConnection of this instance.
         */
        EHSConnection * GetConnection() { return m_poEHSConnection; }

        /// Destructor
        virtual ~GenericResponse() { }

        /**
         * Enable/Disable idle-timeout handling for the current connection.
         * @param enable If true, idle-timeout handling is enabled,
         *   otherwise the socket may stay open forever.
         * When creating an EHSConnection, this is initially enabled.
         */
        void EnableIdleTimeout(bool enable = true);

        /**
         * Enable/Disable TCP keepalive on the underlying socket of the current connection.
         * This enables detection of "dead" sockets, even when
         * idle-timeout handling is disabled.
         * @param enable If true, enable TCP keepalive.
         * When creating an EHSConnection, this is initially disabled.
         */
        void EnableKeepAlive(bool enable = true);


    protected:
        /// response id for making sure we send responses in the right order
        int m_nResponseId;

        /// the actual body to be sent back
        std::string m_sBody;

        /// ehs connection object this response goes back on
        EHSConnection * m_poEHSConnection;

        friend class EHSConnection;
        friend class EHSServer;
};

/// generic std::string => std::string map used by many things
typedef std::map < std::string, std::string > StringMap;

/// std::string => std::string map with case-insensitive search
typedef std::map < std::string, std::string, __caseless > StringCaseMap;

/// generic list of std::strings
typedef std::list < std::string > StringList;

/// list of EHSConnection objects to handle all current connections
typedef std::list < EHSConnection * > EHSConnectionList;

/// map for registered EHS objects on a path
typedef std::map < std::string, EHS * > EHSMap;

/// map type for storing EHSServer parameters
typedef std::map < std::string, Datum > EHSServerParameters;

/// cookies that come in from the client, mapped by name
typedef std::map < std::string, std::string > CookieMap;

/// describes a form value that came in from a client
typedef std::map < std::string, FormValue > FormValueMap;

/// describes a cookie to be sent back to the client
typedef std::map < std::string, Datum > CookieParameters;

/// holds respose objects not yet ready to send
typedef std::deque <ehs_autoptr<GenericResponse> > ResponseQueue;

/// holds the currently handled request for each thread
typedef std::map < ehs_threadid_t, HttpRequest * > CurrentRequestMap;

/// holds a list of pending requests
typedef std::list < HttpRequest * > HttpRequestList;

#endif
