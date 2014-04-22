/* $Id: httprequest.cpp 161 2013-02-27 00:02:04Z felfert $
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

#include "ehs.h"
#include "ehsconnection.h"
#include "debug.h"

#include <string>
#include <algorithm>
#include <set>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>

using namespace std;

bool MultivalHeaderContains(const std::string &header, const std::string &value)
{
    std::set<std::string> tokens;
    boost::split_regex(tokens, boost::to_lower_copy(header), boost::regex(",\\s*"));
    return (tokens.find(boost::to_lower_copy(value)) != tokens.end());
}

bool IsMultivalHeader(const std::string &header)
{
    static std::set<std::string> multivalHeaders;
    if (multivalHeaders.empty()) {
        multivalHeaders.insert("accept-charset");
        multivalHeaders.insert("accept-encoding");
        multivalHeaders.insert("accept-language");
        multivalHeaders.insert("cache-control");
        multivalHeaders.insert("connection");
        multivalHeaders.insert("expect");
        multivalHeaders.insert("if-match");
        multivalHeaders.insert("if-none-match");
        multivalHeaders.insert("pragma");
        multivalHeaders.insert("sec-websocket-extensions");
        multivalHeaders.insert("sec-websocket-protocol");
        multivalHeaders.insert("sec-websocket-version");
        multivalHeaders.insert("trailer");
        multivalHeaders.insert("transfer-encoding");
        multivalHeaders.insert("upgrade");
        multivalHeaders.insert("via");
    }
    bool ret = multivalHeaders.find(boost::to_lower_copy(header)) != multivalHeaders.end();
    EHS_TRACE("IsMultivalHeader (%s) returning %d", header.c_str(), ret);
    return ret;
}

void HttpRequest::GetFormDataFromString(const string & irsString)
{
    boost::regex re("[?]?([^?=]*)=([^&]*)&?");

    std::string::const_iterator start, end; 
    start = irsString.begin(); 
    end = irsString.end(); 
    boost::match_results<string::const_iterator> res; 
    boost::match_flag_type flags = boost::match_default; 
    while (regex_search(start, end, res, re, flags)) { 
        string name(res[1].first, res[1].second);
        string value(res[2].first,res[2].second); 
        start = res[0].second; 
        EHS_TRACE("Info: Got form data: '%s' => '%s'", name.c_str(), value.c_str());
        ContentDisposition oContentDisposition;
        m_oFormValueMap[name] = FormValue(value, oContentDisposition);
        // update flags: 
        flags |= boost::match_prev_avail; 
        flags |= boost::match_not_bob;
    }
}

// this parses a single piece of a multipart form body
// here are two examples -- first is an uploaded file, second is a
//   standard text box html form
/*
   -----------------------------7d2248207500d4
   Content-Disposition: form-data; name="DFWFilename2"; filename="C:\Documents and Settings\administrator\Desktop\contacts.dat"
   Content-Type: application/octet-stream

   Default Screen Name
   -----------------------------7d2248207500d4


   -----------------------------7d2248207500d4
   Content-Disposition: form-data; name="foo"

   asdfasdf
   -----------------------------7d2248207500d4
   */


HttpRequest::ParseSubbodyResult HttpRequest::ParseSubbody(string isSubbody)
{
    // find the spot after the headers in the body
    string::size_type nBlankLinePosition = isSubbody.find("\r\n\r\n");
    // if there's no double blank line, then this isn't a valid subbody
    if (nBlankLinePosition == string::npos) {
        EHS_TRACE("Invalid subbody, couldn't find double blank line", "");
        return PARSESUBBODY_INVALIDSUBBODY;
    }
    // create a string from the beginning to the blank line
    string sHeaders(isSubbody, 0, nBlankLinePosition);
    // First line MUST be the content-disposition header line, so that
    // we know what the name of the field is.. otherwise, we're in trouble
    boost::smatch match;
    boost::regex re("Content-Disposition:[ ]?([^;]+);[ ]?(.*)");
    if (boost::regex_match(sHeaders, match, re)) {
        string sContentDisposition(match[1]);
        string sPairs(match[2]);
        StringCaseMap oStringCaseMap;
        boost::regex nvre("[ ]?([^= ]+)=\"([^\"]+)\"[;]?");
        boost::sregex_iterator i(sPairs.begin(), sPairs.end(), nvre);
        boost::sregex_iterator end;
        while (i != end) {
            string sName((*i)[1]);
            string sValue((*i)[2]);
            EHS_TRACE("Subbody header found: '%s' => '%s'", sName.c_str(), sValue.c_str());
            oStringCaseMap[sName] = sValue;
            ++i;
        }

        // Take oStringCaseMap and actually fill the right object with its data
        FormValue & roFormValue = m_oFormValueMap[oStringCaseMap["name"]];
        // Copy the headers in now that we know the name
        roFormValue.m_oContentDisposition.m_oContentDispositionHeaders = oStringCaseMap;
        // Grab out the body
        roFormValue.m_sBody = isSubbody.substr(nBlankLinePosition + 4);
    } else {
        // couldn't find content-disposition line -- FATAL ERROR
        EHS_TRACE("ERROR: Couldn't find content-disposition line sHeaders='%s'", sHeaders.c_str());
        return PARSESUBBODY_INVALIDSUBBODY;
    }
    return PARSESUBBODY_SUCCESS;
}

HttpRequest::ParseMultipartFormDataResult HttpRequest::ParseMultipartFormData ( )
{
    if (m_oRequestHeaders["Content-Type"].empty()) {
        return PARSEMULTIPARTFORMDATA_FAILED;
    }

#ifdef EHS_DEBUG
    cerr << "looking for boundary in '" << m_oRequestHeaders["Content-Type"] << "'" << endl;
#endif
    // find the boundary string
    boost::regex re("multipart/[^;]+;[ ]*boundary=([^\"]+)$");
    boost::smatch match;
    if (boost::regex_match(m_oRequestHeaders["Content-Type"], match, re)) {
        // the actual boundary has two dashes prepended to it
        string sBoundary("--");
        sBoundary.append(match[1]);
        string::size_type blen = sBoundary.length();

#ifdef EHS_DEBUG
        cerr
            << "[EHS_DEBUG] Info: Found boundary of '" << sBoundary << "'" << endl
            << "[EHS_DEBUG] Info: Looking for boundary to match (" << dec << m_sBody.length()
            << ") '" << m_sBody.substr(0, blen) << "'" << endl;
#endif

        // check to make sure we started at the boundary
        if (m_sBody.substr(0, blen) != sBoundary) {
#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Error: Misparsed multi-part form data for unknown reason - first bytes weren't the boundary string" << endl;
#endif
            return PARSEMULTIPARTFORMDATA_FAILED;
        }

        // go past the initial boundary and it's terminating CRLF
        string sRemainingBody = m_sBody.substr(blen + 2);

        // while we're at a boundary after we grab a part, keep going
        string::size_type nNextPartPosition;
        // From here on, we need CRLF prepended to the boundary string
        sBoundary.insert(0, "\r\n");
        while ((nNextPartPosition = sRemainingBody.find(sBoundary)) != string::npos) {
#ifdef EHS_DEBUG
            cerr << "[EHS_DEBUG] Info: Found subbody from pos 0 to " << dec << nNextPartPosition << endl;
#endif
            if (sRemainingBody.length() < (blen + nNextPartPosition)) {
#ifdef EHS_DEBUG
                cerr << dec << "[EHS_DEBUG] Error: Remaining length (" << sRemainingBody.length()
                    << ") < calculated length (" << blen + nNextPartPosition
                    << ")" << endl;
#endif
                return PARSEMULTIPARTFORMDATA_FAILED;
            }
            ParseSubbody(sRemainingBody.substr(0, nNextPartPosition));
            // skip past the boundary at the end and look for the next one
            nNextPartPosition += blen;
            sRemainingBody = sRemainingBody.substr(nNextPartPosition);
        }
    } else {
#ifdef EHS_DEBUG
        cerr << "[EHS_DEBUG] Error: Couldn't find boundary specification in content-type header" << endl;
#endif
        return PARSEMULTIPARTFORMDATA_FAILED;
    }

    return PARSEMULTIPARTFORMDATA_SUCCESS;
}

// A cookie header looks like: Cookie: username=xaxxon; password=comoesta
//   everything after the : gets passed in in irsData
int HttpRequest::ParseCookieData(string & irsData)
{
    int ccount = 0;
    boost::regex re("\\s*([^=]+)=([^;]+)(;|$)*");
    std::string::const_iterator start, end; 

    EHS_TRACE("Looking for cookie data in '%s'", irsData.c_str());
    start = irsData.begin(); 
    end = irsData.end(); 
    boost::match_results<string::const_iterator> res; 
    boost::match_flag_type flags = boost::match_default; 
    while (regex_search(start, end, res, re, flags)) { 
        string name(res[1].first, res[1].second);
        string value(res[2].first,res[2].second); 
        start = res[0].second; 
        m_oCookieMap[name] = value;
        ccount++;
        // update flags: 
        flags |= boost::match_prev_avail; 
        flags |= boost::match_not_bob;
    }
    return ccount;
}

// takes data and tries to figure out what it is.  it will loop through
//   irsData as many times as it can as long as it gets a full line each time.
//   Depending on how much data it's gotten already, it will handle a line
//   differently..
HttpRequest::HttpParseStates HttpRequest::ParseData ( string & irsData ///< buffer to look in for more data
        )
{
    string sLine;
    string sName;
    string sValue;
    boost::regex reHeader("^([^:\\s]+):\\s+(.*)\\r\\n$");
    boost::regex reHeaderCont("^\\s+(.*)\\r\\n$");
    boost::smatch match;
    bool bDone = false;

    EHS_TRACE("called", "");
    while ( ! bDone &&
            m_nCurrentHttpParseState != HTTPPARSESTATE_INVALIDREQUEST &&
            m_nCurrentHttpParseState != HTTPPARSESTATE_COMPLETEREQUEST &&
            m_nCurrentHttpParseState != HTTPPARSESTATE_INVALID ) {

        EHS_TRACE("parsestate=%d", m_nCurrentHttpParseState);
        switch ( m_nCurrentHttpParseState ) {

            case HTTPPARSESTATE_REQUEST:

                // get the request line
                sLine = GetNextLine(irsData);

                // if we got a line, parse out the data..
                if (sLine.empty() || sLine == "\r\n") {
                    bDone = true;
                } else {
                    // if we got a line, look for a request line

                    // everything must be uppercase according to RFC 2616
                    boost::regex re("^([A-Z]{3,8}) ([^ ]+) HTTP/(\\d+\\.\\d+)\\r\\n$");
                    boost::smatch match;
                    if (boost::regex_match(sLine, match, re, boost::match_single_line)) {
                        string mreq(match[1]);
                        string muri(match[2]);
                        string mver(match[3]);
                        // get the info from the request line
                        m_nRequestMethod = GetRequestMethodFromString(mreq);
                        if (REQUESTMETHOD_UNKNOWN == m_nRequestMethod) {
                            // Unsupported/Unknown request
                            m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        } else {
                            m_sUri = muri;
                            m_sOriginalUri = muri;
                            m_sHttpVersionNumber = mver;
                            // Check to see if the uri appeared to have form data in it
                            GetFormDataFromString(m_sUri);
                            // Continue parsing the headers
                            m_nCurrentHttpParseState = HTTPPARSESTATE_HEADERS;
                            m_bChunked = false;
                        }
                    } else {
                        // the regex failed
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    } // end match on request line
                } // end whether we got a line

                break;

            case HTTPPARSESTATE_HEADERS:

                // get the next line
                sLine = GetNextLine(irsData);

                // if we didn't get a full line of data
                if (sLine.empty()) {
                    bDone = true;
                } else if (sLine == "\r\n" ) {
                    // check to see if we're done with headers

                    // Check for WebSocket header and skip parsing body if found.
                    if (MultivalHeaderContains(Headers("connection"), "upgrade") &&
                            MultivalHeaderContains(Headers("upgrade"), "websocket")) {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                    } else {
                        m_bChunked = MultivalHeaderContains(Headers("transfer-encoding"), "chunked");
                        // Not a WebSocket Header, proceed normally:
                        // if content length is found
                        if ((m_oRequestHeaders.find("Content-Length") !=
                                    m_oRequestHeaders.end()) || m_bChunked) {
                            m_nCurrentHttpParseState = HTTPPARSESTATE_BODY;
                        } else {
                            m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                        }
                    }

                    // if this is an HTTP/1.1 request, then it MUST have a Host: header
                    if (m_sHttpVersionNumber == "1.1" &&
                            m_oRequestHeaders.find("Host") == m_oRequestHeaders.end()) {
                        EHS_TRACE("Missing Host header in HTTP/1.1 request", "");
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    }
                    if (m_oRequestHeaders.find("Cookie") != m_oRequestHeaders.end()) {
                        ParseCookieData(m_oRequestHeaders["Cookie"]);
                    }

                } else if (boost::regex_match(sLine, match, reHeader)) {
                    sName.assign(match[1]);
                    sValue.assign(match[2]);
                    EHS_TRACE("header fullmatch '%s' => '%s'", sName.c_str(), sValue.c_str());
                    // else if there is still data

                    boost::trim(sValue);
                    if (m_oRequestHeaders.end() == m_oRequestHeaders.find(sName)) {
                        m_oRequestHeaders[sName] = sValue;
                    } else {
                        // Multiple header of same name (RFC2616, Section 4.2)
                        // Check, if specific header is allowed (defined as comma-sparated
                        // multi-value header.
                        if (!IsMultivalHeader(sName)) {
                            EHS_TRACE("Multi-Value for %s not allowed", sName.c_str());
                            m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                            bDone = true;
                        } else {
                            string tmp(m_oRequestHeaders[sName].append(", ").append(sValue));
                            m_oRequestHeaders[sName] = tmp;
                        }
                    }
                    m_sLastHeaderName = sName;
                } else if (boost::regex_match(sLine, match, reHeaderCont)) {
                    sValue.assign(match[1]);
                    EHS_TRACE("headerCont fullmatch", "");
                    // Header continuation using leading space
                    if (m_sLastHeaderName.empty()) {
                        EHS_TRACE("Header-Continuation without preceeding Header", "");
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        bDone = true;
                    } else {
                        boost::trim(sValue);
                        string tmp(m_oRequestHeaders[m_sLastHeaderName].append(" ").append(sValue));
                        m_oRequestHeaders[m_sLastHeaderName] = tmp;
                    }
                } else {
                    // else we had some sort of error -- bail out
                    EHS_TRACE("Invalid header line: '%s'", sLine.c_str());
                    m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                    bDone = true;
                }
                break;

            case HTTPPARSESTATE_BODYTRAILER:
                sLine = GetNextLine(irsData);
                if (sLine.empty()) {
                    bDone = true;
                } else {
                    if (sLine == "\r\n" ) {
                        // Empty trailer, push CRLF back so that next state
                        // finds it immediately.
                        irsData.insert(0, "\r\n");
                    }
                    m_nCurrentHttpParseState = HTTPPARSESTATE_BODYTRAILER2;
                    continue;
                }
                break;

            case HTTPPARSESTATE_BODYTRAILER2:
                sLine = GetNextLine(irsData);
                if (sLine.empty()) {
                    bDone = true;
                } else if (sLine == "\r\n") {
                    // if we're dealing with multi-part form attachments
                    if (m_oRequestHeaders["Content-Type"].substr(0, 9) == "multipart") {
                        // handle the body as if it's multipart form data
                        if (ParseMultipartFormData() == PARSEMULTIPARTFORMDATA_SUCCESS) {
                            m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                        } else {
#ifdef EHS_DEBUG
                            cerr << "[EHS_DEBUG] Error: Mishandled multi-part form data for unknown reason" << endl;
#endif
                            m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        }
                    } else {
                        // else the body is just one piece
                        // check for any form data
                        if(m_sParseContentType.empty() ||
                            m_oRequestHeaders["Content-Type"].compare(m_sParseContentType) == 0) {
                            GetFormDataFromString(m_sBody);
                        }
#ifdef EHS_DEBUG
                        cerr << "Done with body, done with entire chunked request" << endl;
#endif
                        m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                    }
                } else {
#ifdef EHS_DEBUG
                    cerr << "[EHS_DEBUG] Error: junk after chunk trailer" << endl;
#endif
                    m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                }
                bDone = true;
                break;

            case HTTPPARSESTATE_BODYCHUNK:
                if (0 == m_nChunkLen) {
                    // At end of chunk, expect exactly one CRLF
                    sLine = GetNextLine(irsData);
                    if (sLine.empty()) {
                        bDone = true;
                    } else {
                        if (sLine == "\r\n") {
                            m_nCurrentHttpParseState = HTTPPARSESTATE_BODY;
                        } else {
                            m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        }
                    }
                } else {
                    size_t l = irsData.length();
                    m_sBody.append(irsData.substr(0, m_nChunkLen));
                    if (l > m_nChunkLen) {
                        irsData = irsData.substr(m_nChunkLen);
                        m_nChunkLen = 0;
                        continue;
                    } else {
                        m_nChunkLen -= l;
                        bDone = true;
                    }
                }
                break;

            case HTTPPARSESTATE_BODY:
                if (m_bChunked) {
                    // get the next line
                    sLine = GetNextLine(irsData);
                    if (!sLine.empty()) {
#ifdef EHS_DEBUG
                        cerr << "[EHS_DEBUG] Look for chunk header in :'" << sLine << "'" << endl;
#endif
                        boost::regex reChunkHeader("^([[:xdigit:]]+)[^\\r]*\\r\\n$");
                        boost::smatch match;
                        if (boost::regex_match(sLine, match, reChunkHeader)) {
                            char *eptr = NULL;
                            string tmp(match[1]);
                            unsigned long len = strtoul(tmp.c_str(), &eptr, 16);
                            if (eptr && (*eptr)) {
                                m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                                continue;
                            }

                            if (0 == len) {
                                // Got end of chunked transfer
#ifdef EHS_DEBUG
                                cerr << "[EHS_DEBUG] match, last-chunk" << endl;
#endif
                                m_nCurrentHttpParseState = HTTPPARSESTATE_BODYTRAILER;
                                continue;
                            } else {
#ifdef EHS_DEBUG
                                cerr << "[EHS_DEBUG] match, chunklen: " << len << endl;
#endif
                                m_nChunkLen = len;
                                m_nCurrentHttpParseState = HTTPPARSESTATE_BODYCHUNK;
                                continue;
                            }
                        } else {
#ifdef EHS_DEBUG
                            cerr << "[EHS_DEBUG] NO MATCH" << endl;
#endif
                            m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        }
                    }
                } else {
                    // if a content length wasn't specified, we can't be here (we
                    //   don't know chunked encoding)
                    if (m_oRequestHeaders.find("Content-Length") ==
                            m_oRequestHeaders.end()) {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        continue;
                    }

                    // get the content length
                    unsigned int nContentLength = 0;
                    try {
                        nContentLength = boost::lexical_cast<unsigned int>(m_oRequestHeaders["Content-Length"]);
                    } catch (const boost::bad_lexical_cast &e) {
                        m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                        continue;
                    }

                    // else if we haven't gotten all the data we're looking for,
                    //   just hold off and try again when we get more
                    if (irsData.length() < nContentLength) {
#ifdef EHS_DEBUG
                        cerr
                            << "[EHS_DEBUG] Info: Not enough data yet -- "
                            << irsData.length() << " < " << nContentLength << endl;
#endif
                        bDone = true;
                    } else {
                        // otherwise, we've gotten enough data from the client, handle it now

                        // grab out the actual body from the request and leave the rest
                        m_sBody.assign(irsData.substr(0, nContentLength));
                        irsData.erase(0, nContentLength);

                        // if we're dealing with multi-part form attachments
                        if (m_oRequestHeaders["Content-Type"].substr(0, 9) == "multipart") {
                            // handle the body as if it's multipart form data
                            if (ParseMultipartFormData() == PARSEMULTIPARTFORMDATA_SUCCESS) {
                                m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                            } else {
#ifdef EHS_DEBUG
                                cerr << "[EHS_DEBUG] Error: Mishandled multi-part form data for unknown reason" << endl;
#endif
                                m_nCurrentHttpParseState = HTTPPARSESTATE_INVALIDREQUEST;
                            }
                        } else {
                            // else the body is just one piece

                            // check for any form data
                            if(m_sParseContentType.empty() ||
                                m_oRequestHeaders["Content-Type"].compare(m_sParseContentType) == 0) {
                                GetFormDataFromString(m_sBody);
                            }
#ifdef EHS_DEBUG
                            cerr << "Done with body, done with entire request" << endl;
#endif
                            m_nCurrentHttpParseState = HTTPPARSESTATE_COMPLETEREQUEST;
                        }

                    }
                }
                bDone = true;
                break;


            case HTTPPARSESTATE_INVALID:
            default:
#ifdef EHS_DEBUG
                cerr << "[EHS_DEBUG] Critical error: Invalid internal state: " << m_nCurrentHttpParseState << endl;
#endif
                break;
        }
    }
    return m_nCurrentHttpParseState;
}

HttpRequest::HttpRequest (int inRequestId,
        EHSConnection * ipoSourceEHSConnection,
        const string & irsParseContentType) :
    m_nCurrentHttpParseState(HTTPPARSESTATE_REQUEST),
    m_nRequestMethod(REQUESTMETHOD_UNKNOWN),
    m_sUri(""),
    m_sOriginalUri(""),
    m_sHttpVersionNumber(""),
    m_sBody(""),
    m_sLastHeaderName(""),
    m_bSecure(false),
    m_oRequestHeaders(StringCaseMap()),
    m_oFormValueMap(FormValueMap()),
    m_oCookieMap(CookieMap()),
    m_nRequestId(inRequestId),
    m_poSourceEHSConnection(ipoSourceEHSConnection),
    m_bChunked(false),
    m_nChunkLen(0),
    m_sParseContentType(irsParseContentType)
{
    if (NULL == m_poSourceEHSConnection) {
#ifdef EHS_DEBUG
        cerr << "Not allowed to have null source connection" << endl;
#endif
        throw runtime_error("Not allowed to have null source connection");
    }
}

HttpRequest::~HttpRequest ( )
{
}

// HELPER FUNCTIONS

string GetNextLine(string & buffer)
{
    string ret;
    size_t pos = buffer.find("\r\n");
    if (string::npos != pos) {
        pos += 2;
        ret.assign(buffer.substr(0, pos));
        buffer.erase(0, pos);
    }
    EHS_TRACE("ret='%s'", ret.c_str());
    return ret;
}

RequestMethod GetRequestMethodFromString(const string &method)
{
    static map<string, RequestMethod> methods = boost::assign::map_list_of
        ("OPTIONS", REQUESTMETHOD_OPTIONS)
        ("GET",     REQUESTMETHOD_GET)
        ("HEAD",    REQUESTMETHOD_HEAD)
        ("POST",    REQUESTMETHOD_POST)
        ("PUT",     REQUESTMETHOD_PUT)
        ("DELETE",  REQUESTMETHOD_DELETE)
        ("TRACE",   REQUESTMETHOD_TRACE)
        ("CONNECT", REQUESTMETHOD_CONNECT);
    map<string, RequestMethod>::const_iterator i = methods.find(method);
    return (methods.end() == i) ? REQUESTMETHOD_UNKNOWN : i->second;
}

string HttpRequest::RemoteAddress()
{
    return m_poSourceEHSConnection->GetRemoteAddress();
}

int HttpRequest::RemotePort()
{
    return m_poSourceEHSConnection->GetRemotePort();
}


string HttpRequest::LocalAddress()
{
    return m_poSourceEHSConnection->GetLocalAddress();
}

int HttpRequest::LocalPort()
{
    return m_poSourceEHSConnection->GetLocalPort();
}


bool HttpRequest::ClientDisconnected()
{
    return m_poSourceEHSConnection->Disconnected();
}
