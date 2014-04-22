/* $Id: httpresponse.h 150 2012-06-07 14:57:34Z felfert $
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

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "ehstypes.h"

/// different response codes and their corresponding phrases -- defined in EHS.cpp
enum ResponseCode {
    HTTPRESPONSECODE_INVALID = 0,
    HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS = 101,
    HTTPRESPONSECODE_200_OK = 200,
    HTTPRESPONSECODE_301_MOVEDPERMANENTLY = 301,
    HTTPRESPONSECODE_302_FOUND = 302,
    HTTPRESPONSECODE_304_NOT_MODIFIED = 304,
    HTTPRESPONSECODE_400_BADREQUEST = 400,
    HTTPRESPONSECODE_401_UNAUTHORIZED = 401,
    HTTPRESPONSECODE_403_FORBIDDEN = 403,
    HTTPRESPONSECODE_404_NOTFOUND = 404,
    HTTPRESPONSECODE_413_TOOLARGE = 413,
    HTTPRESPONSECODE_426_UPGRADE_REQUIRED = 426,
    HTTPRESPONSECODE_500_INTERNALSERVERERROR = 500,
    HTTPRESPONSECODE_503_SERVICEUNAVAILABLE = 503
};

/**
 * This class represents what is sent back to the client.
 * It contains the actual body, any headers specified,
 * and the response code.
 */
class HttpResponse : public GenericResponse {

    private:
        HttpResponse( const HttpResponse & );
        HttpResponse &operator = (const HttpResponse &);

    public:
        /**
         * Constructs a new instance.
         * @param inResponseId A unique Id (normally derived from the corresponding request Id).
         * @param ipoEHSConnection The connection, on which this response should be sent.
         */
        HttpResponse(int inResponseId, EHSConnection * ipoEHSConnection);

        /**
         * Constructs a new standardized error response.
         * @param code The HTTP error code.
         * @param inResponseId A unique Id (normally derived from the corresponding request Id).
         * @param ipoEHSConnection The connection, on which this response should be sent.
         * @return The new response.
         */
        static HttpResponse *Error(ResponseCode code, int inResponseId, EHSConnection * ipoEHSConnection);

        /**
         * Constructs a new standardized error response.
         * @param code The HTTP error code.
         * @param request The http request to which this response refers.
         *   (Used for initializing the Id and the outgoing connection).
         * @return The new response.
         */
        static HttpResponse *Error(ResponseCode code, HttpRequest *request);

        /**
         * Helper function for translating response codes into
         * the corresponding text message.
         * @param code The HTTP result code to be translated.
         * @return The text message, representing the provided result code.
         */
        static const char *GetPhrase(ResponseCode code);

        /// Destructor
        virtual ~HttpResponse ( ) { }

        /**
         * Sets the body of this instance.
         * @param ipsBody The content to set.
         * @param inBodyLength The length of the body. This parameter also
         *   sets the value of the Content-Length HTTP header.
         */
        void SetBody(const char *ipsBody, size_t inBodyLength);

        /**
         * Sets cookies for this response.
         * @param iroCookieParameters The cookies to set.
         */
        void SetCookie(CookieParameters & iroCookieParameters);

        /**
         * Sets the response code for this response.
         * @param code The desired HTTP response code.
         */
        void SetResponseCode(ResponseCode code) { m_nResponseCode = code; }

        /**
         * Retrieves the status code of this this response.
         */
        ResponseCode GetResponseCode() { return m_nResponseCode; }

        /**
         * Retrieves the headers of this this response.
         */
        StringCaseMap& GetHeaders() { return m_oResponseHeaders; }

        /**
         * Retrieves the cookies of this this response.
         */
        StringList& GetCookies() { return m_oCookieList; }

        /**
         * Sets an HTTP header.
         * @param name The name of the HTTP header to set.
         * @param value The value of the HTTP header to set.
         */
        void SetHeader(const std::string & name, const std::string & value)
        {
            m_oResponseHeaders[name] = value;
        }

        /**
         * Removes an HTTP header.
         * @param name The case insensitive name of the HTTP header to remove.
         */
        void RemoveHeader(const std::string & name)
        {
            m_oResponseHeaders.erase(name);
        }

        /**
         * Retrieves the status string of this this response.
         * @return The current status as "<i>number</i> <i>description</i>".
         */
        std::string GetStatusString();

        /**
         * Sets the HTTP Date header.
         * @param stamp A UNIX timestamp, representing the desired time.
         */
        void SetDate(time_t stamp);

        /**
         * Sets the HTTP Last-Modified header.
         * @param stamp A UNIX timestamp, representing the desired time.
         */
        void SetLastModified(time_t stamp);

        /**
         * Utility function for converting a UNIX timestamp into an RFC-conformant
         * HTTP time string.
         * @param stamp A UNIX timestamp, representing the desired time.
         * @return A string, containing the HTTP time.
         */
        std::string HttpTime(time_t stamp);

        /**
         * Retrieves a specific HTTP header.
         * @param name The name of the HTTP header to be retrieved.
         * @return The value of the specified header.
         */
        std::string Header(const std::string & name)
        {
            if (m_oResponseHeaders.find(name) != m_oResponseHeaders.end()) {
                return m_oResponseHeaders[name];
            }
            return std::string();
        }

    private:

        /// the response code to be sent back
        ResponseCode m_nResponseCode;

        /// these are the headers sent back to the client in the http response.
        /// Things like content-type and content-length
        StringCaseMap m_oResponseHeaders;

        /// cookies waiting to be sent
        StringList m_oCookieList;
};

#endif // HTTPRESPONSE_H
