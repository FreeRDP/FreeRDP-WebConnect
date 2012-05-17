/* vim: set et ts=4 sw=4 cindent:
 *
 * FreeRDP-WebConnect,
 * A gateway for seamless access to your RDP-Sessions in any HTML5-compliant browser.
 *
 * Copyright 2012 Fritz Elfert <wsgate@fritz-elfert.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ehs.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <typeinfo>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/regex.hpp>

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "common.hpp"
#include "btexception.hpp"
#include "base64.hpp"
#include "sha1.hpp"
#include "logging.hpp"
#include "wsendpoint.hpp"
#include "wsgate.hpp"
#include "myrawsocket.hpp"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <shlobj.h>
# include "NTService.hpp"
#endif

using namespace std;
using boost::algorithm::iequals;
using boost::algorithm::to_lower_copy;
using boost::algorithm::ends_with;
using boost::algorithm::replace_all;
using boost::algorithm::to_upper_copy;
using boost::algorithm::trim_right_copy_if;
using boost::algorithm::trim;
using boost::algorithm::is_any_of;
using boost::algorithm::split;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using boost::filesystem::path;

namespace wsgate {

    static const char * const ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    int nFormValue(HttpRequest *request, const string & name, int defval) {
        string tmp(request->FormValues(name).m_sBody);
        int ret = defval;
        if (!tmp.empty()) {
            try {
                ret = boost::lexical_cast<int>(tmp);
            } catch (const boost::bad_lexical_cast & e) {  ret = defval; }
        }
        return ret;
    }

    // subclass of EHS that defines a custom HTTP response.
    class WsGate : public EHS
    {
        private:
            typedef enum {
                TEXT,
                HTML,
                PNG,
                ICO,
                JAVASCRIPT,
                CSS,
                BINARY
            } MimeType;

            typedef map<string, rdp_ptr> SessionMap;

            MimeType simpleMime(const string & filename)
            {
                if (ends_with(filename, ".txt"))
                    return TEXT;
                if (ends_with(filename, ".html"))
                    return HTML;
                if (ends_with(filename, ".png"))
                    return PNG;
                if (ends_with(filename, ".ico"))
                    return ICO;
                if (ends_with(filename, ".js"))
                    return JAVASCRIPT;
                if (ends_with(filename, ".css"))
                    return CSS;
                return BINARY;
            }

        public:

            WsGate(EHS *parent = NULL, std::string registerpath = "")
                : EHS(parent, registerpath)
                , m_sHostname()
                , m_sDocumentRoot()
                , m_sPidFile()
                , m_bDebug(false)
                , m_SessionMap()
                , m_allowedHosts()
                , m_deniedHosts()
                , m_bOrderDenyAllow(true)
                , m_bOverrideRdpHost(false)
                , m_bOverrideRdpPort(false)
                , m_bOverrideRdpUser(false)
                , m_bOverrideRdpPass(false)
                , m_bOverrideRdpPerf(false)
                , m_bOverrideRdpNowallp(false)
                , m_bOverrideRdpNowdrag(false)
                , m_bOverrideRdpNomani(false)
                , m_bOverrideRdpNotheme(false)
                , m_bOverrideRdpNotls(false)
                , m_bOverrideRdpNonla(false)
                , m_bOverrideRdpFntlm(false)
                , m_sRdpOverrideHost()
                , m_sRdpOverrideUser()
                , m_sRdpOverridePass()
                , m_RdpOverrideParams()
                {
                }

            virtual ~WsGate()
            {
                if (!m_sPidFile.empty()) {
                    unlink(m_sPidFile.c_str());
                }
            }

            HttpResponse *HandleThreadException(ehs_threadid_t, HttpRequest *request, exception &ex)
            {
                HttpResponse *ret = NULL;
                string msg(ex.what());
                log::err << "##################### Catched " << msg << endl;
                log::err << "request: " << hex << request << dec << endl;
                tracing::exception *btx =
                    dynamic_cast<tracing::exception*>(&ex);
                if (NULL != btx) {
                    string tmsg = btx->where();
                    log::err << "Backtrace:" << endl << tmsg;
                    if (0 != msg.compare("fatal")) {
                        ret = HttpResponse::Error(HTTPRESPONSECODE_500_INTERNALSERVERERROR, request);
                        string body(ret->GetBody());
                        tmsg.insert(0, "<br>\n<pre>").append(msg).append("</pre><p><a href=\"/\">Back to main page</a>");
                        body.insert(body.find("</body>"), tmsg);
                        ret->SetBody(body.c_str(), body.length());
                    }
                }
                return ret;
            }

            // generates a page for each http request
            ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response)
            {
                if (REQUESTMETHOD_GET != request->Method()) {
                    // We cuurently need to support GET requests only.
                    return HTTPRESPONSECODE_400_BADREQUEST;
                }

                string uri(request->Uri());
                size_t pos = uri.find('?');
                if (uri.npos != pos) {
                    uri.erase(pos);
                }
                string thisHost(m_sHostname.empty() ? request->Headers("Host") : m_sHostname);

                // Request-URI for cursor image: /cur/0123456789ABCDEF/1
                if (boost::starts_with(uri, "/cur/")) {
                    string idpart(uri.substr(5));
                    vector<string> parts;
                    boost::split(parts, idpart, is_any_of("/"));
                    SessionMap::iterator it = m_SessionMap.find(parts[0]);
                    if (it != m_SessionMap.end()) {
                        uint32_t cid = 0;
                        try {
                            cid = boost::lexical_cast<uint32_t>(parts[1]);
                        } catch (const boost::bad_lexical_cast & e) { cid = 0; }
                        if (cid) {
                            string png = it->second->GetCursorPng(cid);
                            if (!png.empty()) {
                                response->SetHeader("Content-Type", "image/png");
                                response->SetBody(png.data(), png.length());
                                log::info << "Request from " << request->RemoteAddress()
                                    << ": " << uri << " => 200 OK" << endl;
                                return HTTPRESPONSECODE_200_OK;
                            }
                        }
                    }
                    log::warn << "Request from " << request->RemoteAddress()
                        << ": " << uri << " => 404 Not found" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }
                if (0 == uri.compare("/wsgate")) {
                    string dtsize(request->FormValues("dtsize").m_sBody);
                    string rdphost(request->FormValues("host").m_sBody);
                    string rdpuser(request->FormValues("user").m_sBody);
                    string rdppass(base64_decode(request->FormValues("pass").m_sBody));

                    if (m_bOverrideRdpHost) {
                        rdphost.assign(m_sRdpOverrideHost);
                    }
                    if (m_bOverrideRdpUser) {
                        rdpuser.assign(m_sRdpOverrideUser);
                    }
                    if (m_bOverrideRdpPass) {
                        rdppass.assign(m_sRdpOverridePass);
                    }
                    bool denied = true;
                    vector<boost::regex>::iterator ri;
                    if (m_bOrderDenyAllow) {
                        denied = false;
                        for (ri = m_deniedHosts.begin(); ri != m_deniedHosts.end(); ++ri) {
                            if (regex_match(rdphost, *ri)) {
                                denied = true;
                                break;
                            }
                        }
                        for (ri = m_allowedHosts.begin(); ri != m_allowedHosts.end(); ++ri) {
                            if (regex_match(rdphost, *ri)) {
                                denied = false;
                                break;
                            }
                        }
                    } else {
                        for (ri = m_allowedHosts.begin(); ri != m_allowedHosts.end(); ++ri) {
                            if (regex_match(rdphost, *ri)) {
                                denied = false;
                                break;
                            }
                        }
                        for (ri = m_deniedHosts.begin(); ri != m_deniedHosts.end(); ++ri) {
                            if (regex_match(rdphost, *ri)) {
                                denied = true;
                                break;
                            }
                        }
                    }
                    if (denied) {
                        log::warn << "Connect from " << request->RemoteAddress()
                            << " to " << rdphost << " denied by access rules" << endl;
                        return HTTPRESPONSECODE_403_FORBIDDEN;
                    }

                    WsRdpParams params = {
                        nFormValue(request, "port", 3389),
                        1024,
                        768,
                        m_bOverrideRdpPerf ? m_RdpOverrideParams.perf : nFormValue(request, "perf", 0),
                        m_bOverrideRdpFntlm ? m_RdpOverrideParams.fntlm : nFormValue(request, "fntlm", 0),
                        m_bOverrideRdpNotls ? m_RdpOverrideParams.notls : nFormValue(request, "notls", 0),
                        m_bOverrideRdpNonla ? m_RdpOverrideParams.nonla : nFormValue(request, "nonla", 0),
                        m_bOverrideRdpNowallp ? m_RdpOverrideParams.nowallp : nFormValue(request, "nowallp", 0),
                        m_bOverrideRdpNowdrag ? m_RdpOverrideParams.nowdrag : nFormValue(request, "nowdrag", 0),
                        m_bOverrideRdpNomani ? m_RdpOverrideParams.nomani : nFormValue(request, "nomani", 0),
                        m_bOverrideRdpNotheme ? m_RdpOverrideParams.notheme : nFormValue(request, "notheme", 0),
                    };

                    if (!dtsize.empty()) {
                        try {
                            vector<string> wh;
                            boost::split(wh, dtsize, is_any_of("x"));
                            if (wh.size() == 2) {
                                params.width = boost::lexical_cast<int>(wh[0]);
                                params.height = boost::lexical_cast<int>(wh[1]);
                            }
                        } catch (const exception &e) {
                            params.width = 1024;
                            params.height = 768;
                        }
                    }
                    response->SetBody("", 0);
                    if (0 != request->HttpVersion().compare("1.1")) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 400 (Not HTTP 1.1)" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    string wshost(to_lower_copy(request->Headers("Host")));
                    string wsconn(to_lower_copy(request->Headers("Connection")));
                    string wsupg(to_lower_copy(request->Headers("Upgrade")));
                    string wsver(request->Headers("Sec-WebSocket-Version"));
                    string wskey(request->Headers("Sec-WebSocket-Key"));
                    string wsproto(request->Headers("Sec-WebSocket-Protocol"));
                    string wsext(request->Headers("Sec-WebSocket-Extension"));
                    if (!MultivalHeaderContains(wsconn, "upgrade")) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 400 (No upgrade header)" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    if (!MultivalHeaderContains(wsupg, "websocket")) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 400 (Upgrade header does not contain websocket tag)" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    if (0 != wshost.compare(thisHost)) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 400 (Host header does not match)" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    string wskey_decoded(base64_decode(wskey));
                    if (16 != wskey_decoded.length()) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 400 (Invalid WebSocket key)" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    SHA1 sha1;
                    uint32_t digest[5];
                    sha1 << wskey.c_str() << ws_magic;
                    if (!sha1.Result(digest)) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 500 (Digest calculation failed)" << endl;
                        return HTTPRESPONSECODE_500_INTERNALSERVERERROR;
                    }
                    // Handle endianess
                    for (int i = 0; i < 5; ++i) {
                        digest[i] = htonl(digest[i]);
                    }

                    if (!MultivalHeaderContains(wsver, "13")) {
                        response->SetHeader("Sec-WebSocket-Version", "13");
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 426 (Protocol version not 13)" << endl;
                        return HTTPRESPONSECODE_426_UPGRADE_REQUIRED;
                    }

                    MyRawSocketHandler *sh = dynamic_cast<MyRawSocketHandler*>(GetRawSocketHandler());
                    if (!sh) {
                        throw tracing::runtime_error("No raw socket handler available");
                    }
                    response->EnableIdleTimeout(false);
                    response->EnableKeepAlive(true);
                    if (!sh->Prepare(request->Connection(), rdphost, rdpuser, rdppass, params)) {
                        log::warn << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 503 (RDP backend not available)" << endl;
                        response->EnableIdleTimeout(true);
                        return HTTPRESPONSECODE_503_SERVICEUNAVAILABLE;
                    }

                    CookieParameters setcookie;
                    setcookie["path"] = "/";
                    setcookie["host"] = thisHost;
                    setcookie["max-age"] = "864000";
                    if (request->Secure()) {
                        setcookie["secure"] = "";
                    }
                    CookieParameters delcookie;
                    delcookie["path"] = "/";
                    delcookie["host"] = thisHost;
                    delcookie["max-age"] = "0";
                    if (request->Secure()) {
                        delcookie["secure"] = "";
                    }

                    if (rdphost.empty()) {
                        delcookie["name"] = "lasthost";
                        delcookie["value"] = "%20";
                        response->SetCookie(delcookie);
                    } else {
                        setcookie["name"] = "lasthost";
                        setcookie["value"] = rdphost;
                        response->SetCookie(setcookie);
                    }
                    if (rdpuser.empty()) {
                        delcookie["name"] = "lastuser";
                        delcookie["value"] = "%20";
                        response->SetCookie(delcookie);
                    } else {
                        setcookie["name"] = "lastuser";
                        setcookie["value"] = rdpuser;
                        response->SetCookie(setcookie);
                    }
                    if (m_bOverrideRdpPass) {
                        // Don't expose real RDP password to browser
                        setcookie["name"] = "lastpass";
                        setcookie["value"] = base64_encode(
                                reinterpret_cast<const unsigned char *>("SomthingUseless"), 15);
                        response->SetCookie(setcookie);
                    } else {
                        if (rdppass.empty()) {
                            delcookie["name"] = "lastpass";
                            delcookie["value"] = base64_encode(
                                    reinterpret_cast<const unsigned char *>("%20"), 3);
                            response->SetCookie(delcookie);
                        } else {
                            setcookie["name"] = "lastpass";
                            setcookie["value"] = base64_encode(
                                    reinterpret_cast<const unsigned char *>(rdppass.c_str()),
                                    rdppass.length());
                            response->SetCookie(setcookie);
                        }
                    }

                    response->RemoveHeader("Content-Type");
                    response->RemoveHeader("Content-Length");
                    response->RemoveHeader("Last-Modified");
                    response->RemoveHeader("Cache-Control");

                    if (0 < wsproto.length()) {
                        response->SetHeader("Sec-WebSocket-Protocol", wsproto);
                    }
                    response->SetHeader("Upgrade", "websocket");
                    response->SetHeader("Connection", "Upgrade");
                    response->SetHeader("Sec-WebSocket-Accept",
                            base64_encode(reinterpret_cast<const unsigned char *>(digest), 20));
                    log::info << "Request from " << request->RemoteAddress()
                        << ": " << uri << " => 101" << endl;
                    return HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS;
                }

                // Regular (non WebSockets request
                if (0 != thisHost.compare(request->Headers("Host"))) {
                    log::warn << "Request from " << request->RemoteAddress()
                        << ": " << uri << " => 404 Not found" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }
                path p(m_sDocumentRoot);
                p /= uri;
                if (ends_with(uri, "/")) {
                    p /= (m_bDebug ? "/index-debug.html" : "/index.html");
                }
                p.normalize();

                if (!exists(p)) {
                    log::warn << "Request from " << request->RemoteAddress()
                        << ": " << uri << " => 404 Not found" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }

                // Handle If-modified-sice request header
                time_t mtime = last_write_time(p);
                string ifms(request->Headers("if-modified-since"));
                if (!ifms.empty()) {
                    pt::ptime file_time(pt::from_time_t(mtime));
                    pt::ptime req_time;
                    istringstream iss(ifms);
                    iss.imbue(locale(locale::classic(),
                                new pt::time_input_facet("%a, %d %b %Y %H:%M:%S GMT")));
                    iss >> req_time;
                    if (file_time <= req_time) {
                        response->RemoveHeader("Content-Type");
                        response->RemoveHeader("Content-Length");
                        response->RemoveHeader("Last-Modified");
                        response->RemoveHeader("Cache-Control");
                        log::info << "Request from " << request->RemoteAddress()
                            << ": " << uri << " => 304 Not modified" << endl;
                        return HTTPRESPONSECODE_304_NOT_MODIFIED;
                    }
                }
                response->SetLastModified(mtime);

                fs::ifstream f(p, ios::binary);
                if (f.fail()) {
                    log::warn << "Request from " << request->RemoteAddress()
                        << ": " << uri << " => 404 (file '" << p << "' unreadable)" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }
                string body((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();

#ifdef BOOST_FILESYSTEM_VERSION
# if (BOOST_FILESYSTEM_VERSION >= 3)
                string basename(p.filename().generic_string());
# else
                string basename(p.filename());
# endif
#else
                // Not defined at all: old API
                string basename(p.filename());
#endif

                MimeType mt = simpleMime(to_lower_copy(basename));
                if (HTML == mt) {
                    ostringstream oss;
                    oss << (request->Secure() ? "wss://" : "ws://")
                        << thisHost << ":"
                        << request->LocalPort() << "/wsgate";
                    replace_all(body, "%WSURI%", oss.str());
                    replace_all(body, "%JSDEBUG%", (m_bDebug ? "-debug" : ""));
                    string tmp;
                    tmp.assign(m_bOverrideRdpUser ? m_sRdpOverrideUser : request->Cookies("lastuser"));
                    replace_all(body, "%COOKIE_LASTUSER%", tmp);
                    tmp.assign(m_bOverrideRdpPass ? "SomthingUseless" : base64_decode(request->Cookies("lastpass")));
                    replace_all(body, "%COOKIE_LASTPASS%", tmp);
                    tmp.assign(m_bOverrideRdpHost ? m_sRdpOverrideHost : request->Cookies("lasthost"));
                    replace_all(body, "%COOKIE_LASTHOST%", tmp);
                    tmp.assign(VERSION).append(".").append(GITREV);
                    replace_all(body, "%VERSION%", tmp);
                    if (m_bOverrideRdpPerf) {
                        replace_all(body, "%DISABLED_PERF%", "disabled=\"disabled\"");
                        replace_all(body, "%SELECTED_PERF0%", (0 == m_RdpOverrideParams.perf) ? "selected" : "");
                        replace_all(body, "%SELECTED_PERF1%", (1 == m_RdpOverrideParams.perf) ? "selected" : "");
                        replace_all(body, "%SELECTED_PERF2%", (2 == m_RdpOverrideParams.perf) ? "selected" : "");
                    } else {
                        replace_all(body, "%DISABLED_PERF%", "");
                        replace_all(body, "%SELECTED_PERF0%", "");
                        replace_all(body, "%SELECTED_PERF1%", "");
                        replace_all(body, "%SELECTED_PERF2%", "");
                    }
                    if (m_bOverrideRdpFntlm) {
                        replace_all(body, "%DISABLED_FNTLM%", "disabled=\"disabled\"");
                        replace_all(body, "%SELECTED_FNTLM0%", (0 == m_RdpOverrideParams.fntlm) ? "selected" : "");
                        replace_all(body, "%SELECTED_FNTLM1%", (1 == m_RdpOverrideParams.fntlm) ? "selected" : "");
                        replace_all(body, "%SELECTED_FNTLM2%", (2 == m_RdpOverrideParams.fntlm) ? "selected" : "");
                    } else {
                        replace_all(body, "%DISABLED_FNTLM%", "");
                        replace_all(body, "%SELECTED_FNTLM0%", "");
                        replace_all(body, "%SELECTED_FNTLM1%", "");
                        replace_all(body, "%SELECTED_FNTLM2%", "");
                    }
                    if (m_bOverrideRdpNowallp) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.nowallp) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NOWALLP%", tmp);
                    if (m_bOverrideRdpNowdrag) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.nowdrag) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NOWDRAG%", tmp);
                    if (m_bOverrideRdpNomani) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.nomani) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NOMANI%", tmp);
                    if (m_bOverrideRdpNotheme) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.notheme) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NOTHEME%", tmp);
                    if (m_bOverrideRdpNotls) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.notls) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NOTLS%", tmp);
                    if (m_bOverrideRdpNonla) {
                        tmp.assign("disabled=\"disabled\"").append((m_RdpOverrideParams.nonla) ? " checked=\"checked\"" : "");
                    } else {
                        tmp.assign("");
                    }
                    replace_all(body, "%CHECKED_NONLA%", tmp);
                }
                switch (mt) {
                    case TEXT:
                        response->SetHeader("Content-Type", "text/plain");
                        break;
                    case HTML:
                        response->SetHeader("Content-Type", "text/html");
                        break;
                    case PNG:
                        response->SetHeader("Content-Type", "image/png");
                        break;
                    case ICO:
                        response->SetHeader("Content-Type", "image/x-icon");
                        break;
                    case JAVASCRIPT:
                        response->SetHeader("Content-Type", "text/javascript");
                        break;
                    case CSS:
                        response->SetHeader("Content-Type", "text/css");
                        break;
                    case BINARY:
                        response->SetHeader("Content-Type", "application/octet-stream");
                        break;
                }
                response->SetBody(body.data(), body.length());
                log::info << "Request from " << request->RemoteAddress()
                    << ": " << uri << " => 200 OK" << endl;
                return HTTPRESPONSECODE_200_OK;
            }

            void ConvertToPattern(string &wildcards) {
                boost::replace_all(wildcards, "\\", "\\\\");
                boost::replace_all(wildcards, "^", "\\^");
                boost::replace_all(wildcards, ".", "\\.");
                boost::replace_all(wildcards, "$", "\\$");
                boost::replace_all(wildcards, "|", "\\|");
                boost::replace_all(wildcards, "(", "\\(");
                boost::replace_all(wildcards, ")", "\\)");
                boost::replace_all(wildcards, "[", "\\[");
                boost::replace_all(wildcards, "]", "\\]");
                boost::replace_all(wildcards, "*", "\\*");
                boost::replace_all(wildcards, "+", "\\+");
                boost::replace_all(wildcards, "?", "\\?");
                boost::replace_all(wildcards, "/", "\\/");
                boost::replace_all(wildcards, "\\?", ".");
                boost::replace_all(wildcards, "\\*", ".*");
                wildcards.insert(0, "^").append("$");
            }

            bool SetOrder(const string &order) {
                vector<string> parts;
                boost::split(parts, order, is_any_of(","));
                if (2 == parts.size()) {
                    trim(parts[0]);
                    trim(parts[1]);
                    if (iequals(parts[0],"deny") && iequals(parts[1],"allow")) {
                        m_bOrderDenyAllow = true;
                        return true;
                    }
                    if (iequals(parts[0],"allow") && iequals(parts[1],"deny")) {
                        m_bOrderDenyAllow = false;
                        return true;
                    }
                }
                return false;
            }

            void SetAllowedHosts(const vector<string> &hosts) {
                vector<string> tmp(hosts);
                vector<string>::iterator i;
                for (i = tmp.begin(); i != tmp.end(); ++i) {
                    ConvertToPattern(*i);
                    boost::regex re(*i, boost::regex::icase);
                    m_allowedHosts.push_back(re);
                }
            }

            void SetDeniedHosts(const vector<string> &hosts) {
                vector<string> tmp(hosts);
                vector<string>::iterator i;
                for (i = tmp.begin(); i != tmp.end(); ++i) {
                    ConvertToPattern(*i);
                    boost::regex re(*i, boost::regex::icase);
                    m_deniedHosts.push_back(re);
                }
            }

            void SetHostname(const string &name) {
                m_sHostname = name;
            }

            void SetDocumentRoot(const string &name) {
                m_sDocumentRoot = name;
            }

            void SetPidFile(const string &name) {
                m_sPidFile = name;
            }

            void SetDebug(const bool debug) {
                m_bDebug = debug;
            }

            void SetOverrideHost(const string &s) {
                m_sRdpOverrideHost = s;
                m_bOverrideRdpHost = true;
            }

            void SetOverrideUser(const string &s) {
                m_sRdpOverrideUser = s;
                m_bOverrideRdpUser = true;
            }

            void SetOverridePass(const string &s) {
                m_sRdpOverridePass = s;
                m_bOverrideRdpPass = true;
            }

            void SetOverridePort(int n) {
                m_RdpOverrideParams.port = n;
                m_bOverrideRdpPort = true;
            }

            void SetOverridePerf(int n) {
                m_RdpOverrideParams.perf = n;
                m_bOverrideRdpPerf = true;
            }

            void SetOverrideFntlm(int n) {
                m_RdpOverrideParams.fntlm = n;
                m_bOverrideRdpFntlm = true;
            }

            void SetOverrideNotls(bool b) {
                m_RdpOverrideParams.notls = b ? 1 : 0;
                m_bOverrideRdpNotls = true;
            }

            void SetOverrideNonla(bool b) {
                m_RdpOverrideParams.nonla = b ? 1 : 0;
                m_bOverrideRdpNonla = true;
            }

            void SetOverrideNowallp(bool b) {
                m_RdpOverrideParams.nowallp = b ? 1 : 0;
                m_bOverrideRdpNowallp = true;
            }

            void SetOverrideNowdrag(bool b) {
                m_RdpOverrideParams.nowdrag = b ? 1 : 0;
                m_bOverrideRdpNowdrag = true;
            }

            void SetOverrideNomani(bool b) {
                m_RdpOverrideParams.nomani = b ? 1 : 0;
                m_bOverrideRdpNomani = true;
            }

            void SetOverrideNotheme(bool b) {
                m_RdpOverrideParams.notheme = b ? 1 : 0;
                m_bOverrideRdpNotheme = true;
            }

            void RegisterRdpSession(rdp_ptr rdp) {
                ostringstream oss;
                oss << hex << rdp.get();
                m_SessionMap[oss.str()] = rdp;
            }

            void UnregisterRdpSession(rdp_ptr rdp) {
                ostringstream oss;
                oss << hex << rdp.get();
                m_SessionMap.erase(oss.str());
            }

        private:
            string m_sHostname;
            string m_sDocumentRoot;
            string m_sPidFile;
            bool m_bDebug;
            SessionMap m_SessionMap;
            vector<boost::regex> m_allowedHosts;
            vector<boost::regex> m_deniedHosts;
            bool m_bOrderDenyAllow;
            bool m_bOverrideRdpHost;
            bool m_bOverrideRdpPort;
            bool m_bOverrideRdpUser;
            bool m_bOverrideRdpPass;
            bool m_bOverrideRdpPerf;
            bool m_bOverrideRdpNowallp;
            bool m_bOverrideRdpNowdrag;
            bool m_bOverrideRdpNomani;
            bool m_bOverrideRdpNotheme;
            bool m_bOverrideRdpNotls;
            bool m_bOverrideRdpNonla;
            bool m_bOverrideRdpFntlm;
            string m_sRdpOverrideHost;
            string m_sRdpOverrideUser;
            string m_sRdpOverridePass;
            WsRdpParams m_RdpOverrideParams;
    };

#ifndef _WIN32
    // Bind helper is not needed on win32, because win32 does not
    // have a concept of privileged ports.
    class MyBindHelper : public PrivilegedBindHelper {

        public:
            MyBindHelper() : mutex(pthread_mutex_t()) {
                pthread_mutex_init(&mutex, NULL);
            }

            bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port) {
                bool ret = false;
                pid_t pid;
                int status;
                char buf[32];
                pthread_mutex_lock(&mutex);
                switch (pid = fork()) {
                    case 0:
                        sprintf(buf, "%08x%08x%04x", socket, inet_addr(addr), port);
                        execl(BINDHELPER_PATH, buf, ((void *)NULL));
                        exit(errno);
                        break;
                    case -1:
                        break;
                    default:
                        if (waitpid(pid, &status, 0) != -1) {
                            ret = (0 == status);
                            if (0 != status) {
                                log::err << BINDHELPER_PATH << " reports: " << strerror(WEXITSTATUS(status)) << endl;
                                errno = WEXITSTATUS(status);
                            }
                        }
                        break;
                }
                pthread_mutex_unlock(&mutex);
                return ret;
            }

        private:
            pthread_mutex_t mutex;
    };
#endif

    class MyWsHandler : public wspp::wshandler
    {
        public:
            MyWsHandler(EHSConnection *econn, EHS *ehs, MyRawSocketHandler *rsh)
                : m_econn(econn)
                  , m_ehs(ehs)
                  , m_rsh(rsh)
        {}

            virtual void on_message(std::string, std::string data) {
                m_rsh->OnMessage(m_econn, data);
            }
            virtual void on_close() {
                log::debug << "GOT Close" << endl;
                ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
                m_ehs->AddResponse(ehs_move(r));
            }
            virtual bool on_ping(const std::string data) {
                log::debug << "GOT Ping: '" << data << "'" << endl;
                return true;
            }
            virtual void on_pong(const std::string data) {
                log::debug << "GOT Pong: '" << data << "'" << endl;
            }
            virtual void do_response(const std::string data) {
                // log::debug << "Send WS response, size: " << data.length() << endl;
                ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
                r->SetBody(data.data(), data.length());
                m_ehs->AddResponse(ehs_move(r));
            }

        private:
            // Non-copyable
            MyWsHandler(const MyWsHandler&);
            MyWsHandler& operator=(const MyWsHandler&);

            EHSConnection *m_econn;
            EHS *m_ehs;
            MyRawSocketHandler *m_rsh;
    };

    MyRawSocketHandler::MyRawSocketHandler(WsGate *parent)
        : m_parent(parent)
          , m_cmap(conn_map())
    { }

    bool MyRawSocketHandler::OnData(EHSConnection *conn, std::string data)
    {
        if (m_cmap.end() != m_cmap.find(conn)) {
            m_cmap[conn].get<0>()->AddRxData(data);
            return true;
        }
        return false;
    }

    void MyRawSocketHandler::OnConnect(EHSConnection * /* conn */)
    {
        log::debug << "GOT WS CONNECT" << endl;
    }

    void MyRawSocketHandler:: OnDisconnect(EHSConnection *conn)
    {
        log::debug << "GOT WS DISCONNECT" << endl;
        m_parent->UnregisterRdpSession(m_cmap[conn].get<2>());
        m_cmap.erase(conn);
    }

    void MyRawSocketHandler::OnMessage(EHSConnection *conn, const std::string & data)
    {
        if (m_cmap.end() != m_cmap.find(conn)) {
            m_cmap[conn].get<2>()->OnWsMessage(data);
        }
    }

    bool MyRawSocketHandler::Prepare(EHSConnection *conn, const string host,
            const string user, const string pass, const WsRdpParams &params)
    {
        log::debug << "RDP Host:         '" << host << "'" << endl;
        log::debug << "RDP Port:         '" << params.port << "'" << endl;
        log::debug << "RDP User:         '" << user << "'" << endl;
        log::debug << "RDP Password:     '" << pass << "'" << endl;
        log::debug << "RDP Desktop size: " << params.width << "x" << params.height << endl;

        handler_ptr h(new MyWsHandler(conn, m_parent, this));
        conn_ptr c(new wspp::wsendpoint(h.get()));
        rdp_ptr r(new RDP(h.get()));
        m_cmap[conn] = conn_tuple(c, h, r);
        r->Connect(host, user, string() /*domain*/, pass, params);
        m_parent->RegisterRdpSession(r);
        return true;
    }

}

static bool str2bool(const string &s) {
    string v(s);
    trim(v);
    if (!v.empty()) {
        if (iequals(v, "true")) {
            return true;
        }
        if (iequals(v, "yes")) {
            return true;
        }
        if (iequals(v, "on")) {
            return true;
        }
        if (iequals(v, "1")) {
            return true;
        }
        if (iequals(v, "false")) {
            return false;
        }
        if (iequals(v, "no")) {
            return false;
        }
        if (iequals(v, "off")) {
            return false;
        }
        if (iequals(v, "0")) {
            return false;
        }
    }
    throw tracing::invalid_argument("Invalid boolean value");
}

static bool g_signaled = false;
#ifdef _WIN32
static bool g_service_background = true;
#endif

static void terminate(int)
{
    g_signaled = true;
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
}

#ifdef _WIN32
static int _service_main (int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
    wsgate::logger log("wsgate");

    // commandline options
    po::options_description desc("Supported options");
    desc.add_options()
        ("help,h", "Show this message and exit.")
        ("version,V", "Show version information and exit.")
#ifndef _WIN32
        ("foreground,f", "Run in foreground.")
#endif
        ("config,c", po::value<string>()->default_value(DEFAULTCFG), "Specify config file")
        ;

    // config file options
    po::options_description cfg("");
    cfg.add_options()
        ("global.daemon", po::value<string>(), "enable/disable daemon mode")
        ("global.pidfile", po::value<string>(), "path of PID file in daemon mode")
        ("global.debug", po::value<string>(), "enable/disable debugging")
        ("global.hostname", po::value<string>(), "specify host name")
        ("global.port", po::value<uint16_t>(), "specify listening port")
        ("global.bindaddr", po::value<string>(), "specify bind address")
        ("global.logmask", po::value<string>(), "specify syslog mask")
        ("global.logfacility", po::value<string>(), "specify syslog facility")
        ("ssl.port", po::value<uint16_t>(), "specify listening port for SSL")
        ("ssl.bindaddr", po::value<string>(), "specify bind address for SSL")
        ("ssl.certfile", po::value<string>(), "specify certificate file")
        ("ssl.certpass", po::value<string>(), "specify certificate passphrase")
        ("threading.mode", po::value<string>(), "specify threading mode")
        ("threading.poolsize", po::value<int>(), "specify threading pool size")
        ("http.maxrequestsize", po::value<unsigned long>(), "specify maximum http request size")
        ("http.documentroot", po::value<string>(), "specify http document root")
        ("acl.allow", po::value<vector<string>>()->multitoken(), "Allowed destination hosts or nets")
        ("acl.deny", po::value<vector<string>>()->multitoken(), "Denied destination hosts or nets")
        ("acl.order", po::value<string>(), "Order (deny,allow or allow,deny)")
        ("rdpoverride.host", po::value<string>(), "Predefined RDP destination host")
        ("rdpoverride.port", po::value<uint16_t>(), "Predefined RDP port")
        ("rdpoverride.user", po::value<string>(), "Predefined RDP user")
        ("rdpoverride.pass", po::value<string>(), "Predefined RDP password")
        ("rdpoverride.performance", po::value<int>(), "Predefined RDP performance")
        ("rdpoverride.nowallpaper", po::value<string>(), "Predefined RDP flag: No wallpaper")
        ("rdpoverride.nofullwindowdrag", po::value<string>(), "Predefined RDP flag: No full window drag")
        ("rdpoverride.nomenuanimation", po::value<string>(), "Predefined RDP flag: No full menu animation")
        ("rdpoverride.notheming", po::value<string>(), "Predefined RDP flag: No theming")
        ("rdpoverride.notls", po::value<string>(), "Predefined RDP flag: Disable TLS")
        ("rdpoverride.nonla", po::value<string>(), "Predefined RDP flag: Disable NLA")
        ("rdpoverride.forcentlm", po::value<int>(), "Predefined RDP flag: Force NTLM")
        ("rdpoverride.size", po::value<string>(), "Predefined RDP desktop size")
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const po::error &e) {
#ifdef _WIN32
        wsgate::log::err << e.what() << endl << "Hint: Use --help option." << endl;
#endif
        cerr << e.what() << endl << "Hint: Use --help option." << endl;
        return -1;
    }

    if (vm.count("help")) {
        cout << desc << endl;
        return 0;
    }
    if (vm.count("version")) {
        cout << "wsgate v" << VERSION << "." << GITREV << endl << getEHSconfig() << endl;
        return 0;
    }
    if (vm.count("config")) {
        string cfgname(vm["config"].as<string>());
        ifstream f(cfgname.c_str());
        if (f.fail()) {
#ifdef _WIN32
            wsgate::log::err << "Could not read config file '" << cfgname << "'." << endl;
#endif
            cerr << "Could not read config file '" << cfgname << "'." << endl;
            return -1;
        }
        try {
            po::store(po::parse_config_file(f, cfg, true), vm);
            po::notify(vm);
        } catch (const po::error &e) {
            cerr << e.what() << endl;
            return -1;
        }
    } else {
#ifdef _WIN32
        wsgate::log::err << "Mandatory option --config <filename> is missing." << endl;
#endif
        cerr << "Mandatory option --config <filename> is missing." << endl;
        return -1;
    }

    wsgate::WsGate srv;

    // Examine values from config file
    if (vm.count("global.debug")) {
        srv.SetDebug(str2bool(vm["global.debug"].as<string>()));
    }

    if (vm.count("global.logmask")) {
        log.setmaskByName(to_upper_copy(vm["global.logmask"].as<string>()));
    }
    if (vm.count("global.logfacility")) {
        log.setfacilityByName(to_upper_copy(vm["global.logfacility"].as<string>()));
    }
    if (0 == (vm.count("global.port") + vm.count("ssl.port"))) {
#ifdef _WIN32
        wsgate::log::err << "No listening ports defined." << endl;
#endif
        cerr << "No listening ports defined." << endl;
        return -1;
    }
    if (0 == (vm.count("http.documentroot"))) {
#ifdef _WIN32
        wsgate::log::err << "No documentroot defined." << endl;
#endif
        cerr << "No documentroot defined." << endl;
        return -1;
    }

    if (vm.count("acl.order")) {
        if (!srv.SetOrder(vm["acl.order"].as<string>())) {
            cerr << "Invalid order directive." << endl;
            return -1;
        }
    }

    if (vm.count("acl.allow")) {
        srv.SetAllowedHosts(vm["acl.allow"].as<vector <string>>());
    }
    if (vm.count("acl.deny")) {
        srv.SetDeniedHosts(vm["acl.allow"].as<vector <string>>());
    }
    if (vm.count("rdpoverride.host")) {
        srv.SetOverrideHost(vm["rdpoverride.host"].as<string>());
    }

    try {
        if (vm.count("rdpoverride.user")) {
            srv.SetOverrideUser(vm["rdpoverride.user"].as<string>());
        }
        if (vm.count("rdpoverride.pass")) {
            srv.SetOverridePass(vm["rdpoverride.pass"].as<string>());
        }
        if (vm.count("rdpoverride.port")) {
            srv.SetOverridePort(vm["rdpoverride.port"].as<int>());
        }
        if (vm.count("rdpoverride.performance")) {
            srv.SetOverridePerf(vm["rdpoverride.performance"].as<int>());
        }
        if (vm.count("rdpoverride.forcentlm")) {
            srv.SetOverrideFntlm(vm["rdpoverride.forcentlm"].as<int>());
        }
        if (vm.count("rdpoverride.nowallpaper")) {
            srv.SetOverrideNowallp(str2bool(vm["rdpoverride.nowallpaper"].as<string>()));
        }
        if (vm.count("rdpoverride.nofullwindowdrag")) {
            srv.SetOverrideNowdrag(str2bool(vm["rdpoverride.nofullwindowdrag"].as<string>()));
        }
        if (vm.count("rdpoverride.nomenuanimation")) {
            srv.SetOverrideNomani(str2bool(vm["rdpoverride.nomenuanimation"].as<string>()));
        }
        if (vm.count("rdpoverride.notheming")) {
            srv.SetOverrideNotheme(str2bool(vm["rdpoverride.notheming"].as<string>()));
        }
        if (vm.count("rdpoverride.notls")) {
            srv.SetOverrideNotls(str2bool(vm["rdpoverride.notls"].as<string>()));
        }
        if (vm.count("rdpoverride.nonla")) {
            srv.SetOverrideNonla(str2bool(vm["rdpoverride.nonla"].as<string>()));
        }
    } catch (const tracing::invalid_argument & e) {
        cerr << e.what() << endl;
        return -1;
    }

    if (vm.count("global.hostname")) {
        srv.SetHostname(vm["global.hostname"].as<string>());
    }
    int port = -1;
    bool https = false;
    // bool need2 = false;
    if (vm.count("global.port")) {
        port = vm["global.port"].as<uint16_t>();
        if (vm.count("ssl.port")) {
            // need2 = true;
        }
    } else if (vm.count("ssl.port")) {
        port = vm["ssl.port"].as<uint16_t>();
        https = true;
    }
    srv.SetDocumentRoot(vm["http.documentroot"].as<string>());

#ifndef _WIN32
    wsgate::MyBindHelper h;
    srv.SetBindHelper(&h);
#endif
    wsgate::MyRawSocketHandler sh(&srv);
    srv.SetRawSocketHandler(&sh);

    EHSServerParameters oSP;
    oSP["port"] = port;
    oSP["bindaddress"] = "0.0.0.0";
    oSP["norouterequest"] = 1;
    if (https) {
        if (vm.count("ssl.bindaddr")) {
            oSP["bindaddress"] = vm["ssl.bindaddr"].as<string>();
        }
        oSP["https"] = 1;
        if (vm.count("ssl.certfile")) {
            oSP["certificate"] = vm["ssl.certfile"].as<string>();
        }
        if (vm.count("ssl.certpass")) {
            oSP["passphrase"] = vm["ssl.certpass"].as<string>();
        }
    } else {
        if (vm.count("global.bindaddr")) {
            oSP["bindaddress"] = vm["global.bindaddr"].as<string>();
        }
    }
    if (vm.count("http.maxrequestsize")) {
        oSP["maxrequestsize"] = vm["http.maxrequestsize"].as<unsigned long>();
    }
    bool sleepInLoop = true;
    if (vm.count("threading.mode")) {
        string mode(vm["threading.mode"].as<string>());
        if (0 == mode.compare("single")) {
            oSP["mode"] = "singlethreaded";
            sleepInLoop = false;
        } else if (0 == mode.compare("pool")) {
            oSP["mode"] = "threadpool";
            if (vm.count("threading.poolsize")) {
                oSP["threadcount"] = vm["threading.poolsize"].as<int>();
            }
        } else if (0 == mode.compare("perrequest")) {
            oSP["mode"] = "onethreadperrequest";
        } else {
            cerr << "Invalid threading mode '" << mode << "'." << endl;
            return -1;
        }
    } else {
        oSP["mode"] = "onethreadperrequest";
    }
    if (vm.count("ssl.certfile")) {
        oSP["certificate"] = vm["ssl.certfile"].as<string>();
    }
    if (vm.count("ssl.certpass")) {
        oSP["passphrase"] = vm["ssl.certpass"].as<string>();
    }

#ifdef _WIN32
    bool daemon = (vm.count("foreground")) ? false : g_service_background;
#else
    bool daemon = false;
    if (vm.count("global.daemon") && (0 == vm.count("foreground"))) {
        daemon = (str2bool(vm["global.daemon"].as<string>()));
        if (daemon) {
            pid_t pid = fork();
            switch (pid) {
                case 0:
                    // child
                    {
                        int nfd = open("/dev/null", O_RDWR);
                        dup2(nfd, 0);
                        dup2(nfd, 1);
                        dup2(nfd, 2);
                        close(nfd);
                        (void)chdir("/");
                        setsid();
                        if (vm.count("global.pidfile")) {
                            const string pidfn(vm["global.pidfile"].as<string>());
                            if (!pidfn.empty()) {
                                ofstream pidfile(pidfn.c_str());
                                pidfile << getpid() << endl;
                                pidfile.close();
                                srv.SetPidFile(pidfn);
                            }
                        }
                    }
                    break;
                case -1:
                    cerr << "Could not fork" << endl;
                    return -1;
                default:
                    return 0;
            }
        }
    }
#endif

#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);

    try {
        wsgate::log::info << "wsgate v" << VERSION << "." << GITREV << " starting" << endl;
        srv.StartServer(oSP);

        wsgate::log::info << "Listening on " << oSP["bindaddress"].GetCharString() << ":" << oSP["port"].GetInt() << endl;

        if (daemon) {
            while (!(srv.ShouldTerminate() || g_signaled)) {
                if (sleepInLoop) {
                    usleep(10000);
                } else {
                    srv.HandleData(1000);
                }
            }
        } else {
            wsgate::kbdio kbd;
            cout << "Press q to terminate ..." << endl;
            while (!(srv.ShouldTerminate() || g_signaled || kbd.qpressed())) {
                if (sleepInLoop) {
                    usleep(1000);
                } else {
                    srv.HandleData(1000);
                }
            }
        }
        wsgate::log::info << "terminating" << endl;
        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
        wsgate::log::err << e.what() << endl;
    }

    return 0;
}

#ifdef _WIN32
// Windows Service implementation

namespace wsgate {

    class WsGateService : public NTService {

        public:

            WsGateService() : NTService("FreeRDP-WebConnect", "FreeRDP WebConnect")
        {
            m_dwServiceStartupType = SERVICE_AUTO_START;
            m_sDescription.assign("RDP Web access gateway");
            AddDependency("Eventlog");
        }

        protected:

            bool OnServiceStop()
            {
                g_signaled = true;
                return true;
            }

            bool OnServiceShutdown()
            {
                g_signaled = true;
                return true;
            }

            void RunService()
            {
                g_signaled = false;
                // On Windows, always set out working dir to ../ relatively seen from
                // the binary's path.
                path p(m_sModulePath);
                string wdir(p.branch_path().branch_path().string());
                chdir(wdir.c_str());
                g_signaled = false;
                char *argv[] = {
                    strdup("wsgate"),
                    strdup("-c"),
                    strdup("etc/wsgate.ini"),
                    NULL
                };
                int r = _service_main(3, argv);
                if (0 != r) {
                    m_ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
                    m_ServiceStatus.dwServiceSpecificExitCode = r;
                }
                free(argv[0]);
                free(argv[1]);
                free(argv[2]);
            }

        public:

            bool ParseSpecialArgs(int argc, char **argv)
            {
                if (argc < 2) {
                    return false;
                }
                bool installed = false;
                try {
                    installed = IsServiceInstalled();
                } catch (const tracing::runtime_error &e) {
                    cerr << e.what() << endl;
                    return true;
                }
                if (0 == strcmp(argv[1], "--query")) {
                    // Report version of installed service
                    cout << "The service is " << (installed ? "currently" : "not")
                        << " installed." << endl;
                    return true;
                }
                if (0 == strcmp(argv[1], "--start")) {
                    // Start the service
                    try {
                        Start();
                    } catch (const tracing::runtime_error &e) {
                        cerr << "Failed to start " << m_sServiceName << endl;
                        cerr << e.what() << endl;
                    }
                    return true;
                }
                if (0 == strcmp(argv[1], "--stop")) {
                    // Start the service
                    try {
                        Stop();
                    } catch (const tracing::runtime_error &e) {
                        cerr << "Failed to stop " << m_sServiceName << endl;
                        cerr << e.what() << endl;
                    }
                    return true;
                }
                if (0 == strcmp(argv[1], "--install")) {
                    // Install the service
                    if (installed) {
                        cout << m_sServiceName << " is already installed." << endl;
                    } else {
                        try {
                            InstallService();
                            cout << m_sServiceName << " installed." << endl;
                        } catch (const tracing::runtime_error &e) {
                            cerr << "Failed to install " << m_sServiceName << endl;
                            cerr << e.what() << endl;
                        }
                    }
                    return true;
                }
                if (0 == strcmp(argv[1], "--remove")) {
                    // Remove the service
                    if (!installed) {
                        cout << m_sServiceName << " is not installed." << endl;
                    } else {
                        try {
                            UninstallService();
                            cout << m_sServiceName << " removed." << endl;
                        } catch (const tracing::runtime_error &e) {
                            cerr << "Failed to remove " << m_sServiceName << endl;
                            cerr << e.what() << endl;
                        }
                    }
                    return true;
                }
                return false;
            }

    };
}

int main (int argc, char **argv)
{
    wsgate::WsGateService s;
    if (!s.ParseSpecialArgs(argc, argv)) {
        try {
            if (!s.Execute()) {
                return _service_main(argc, argv);
            }
        } catch (const tracing::runtime_error &e) {
            cerr << "Failed to execute service" << endl;
            cerr << e.what() << endl;
        }
    }
    return s.ServiceExitCode();
}

#endif
