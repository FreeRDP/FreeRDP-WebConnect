#include "wsgateEHS.hpp"
#include "wsgate.hpp"

namespace wsgate{
    WsGate::MimeType WsGate::simpleMime(const string & filename)
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
        if (ends_with(filename, ".json"))
            return JSON;
        if (ends_with(filename, ".css"))
            return CSS;
        if (ends_with(filename, ".cur"))
            return CUR;
        if (ends_with(filename, ".ogg"))
            return OGG;
        return BINARY;
    }

    WsGate::WsGate(EHS *parent, std::string registerpath)
        : EHS(parent, registerpath)
        , m_sHostname()
        , m_sDocumentRoot()
        , m_sPidFile()
        , m_bDebug(false)
        , m_bEnableCore(false)
        , m_SessionMap()
        , m_allowedHosts()
        , m_deniedHosts()
        , m_bOrderDenyAllow(true)
        , m_sConfigFile()
        , m_ptIniConfig()
        , m_bDaemon(false)
        , m_bRedirect(false)
        , m_StaticCache()
        {
            overrideParams.m_bOverrideRdpHost = false;
            overrideParams.m_bOverrideRdpPort = false;
            overrideParams.m_bOverrideRdpUser = false;
            overrideParams.m_bOverrideRdpPass = false;
            overrideParams.m_bOverrideRdpPcb = false;
            overrideParams.m_bOverrideRdpPerf = false;
            overrideParams.m_bOverrideRdpNowallp = false;
            overrideParams.m_bOverrideRdpNowdrag = false;
            overrideParams.m_bOverrideRdpNomani = false;
            overrideParams.m_bOverrideRdpNotheme = false;
            overrideParams.m_bOverrideRdpNotls = false;
            overrideParams.m_bOverrideRdpNonla = false;
            overrideParams.m_bOverrideRdpFntlm = false;
        }

    WsGate::~WsGate()
    {
        if (!m_sPidFile.empty()) {
            unlink(m_sPidFile.c_str());
        }
    }

    HttpResponse* WsGate::HandleThreadException(ehs_threadid_t, HttpRequest *request, exception &ex)
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
        } else {
            ret = HttpResponse::Error(HTTPRESPONSECODE_500_INTERNALSERVERERROR, request);
        }
        return ret;
    }

    void WsGate::CheckForPredefined(string& rdpHost, string& rdpUser, string& rdpPass)
    {
        if (this->overrideParams.m_bOverrideRdpHost)
            rdpHost.assign(this->overrideParams.m_sRdpOverrideHost);

        if (this->overrideParams.m_bOverrideRdpUser)
            rdpUser.assign(this->overrideParams.m_sRdpOverrideUser);

        if (this->overrideParams.m_bOverrideRdpPass)
            rdpPass.assign(this->overrideParams.m_sRdpOverridePass);
    }

    bool WsGate::ConnectionIsAllowed(string rdphost)
    {
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
            return true;
    }

    void WsGate::LogInfo(std::basic_string<char> remoteAdress, string uri, const char response[])
    {
        log::info << "Request FROM: " << remoteAdress << " replied with " << response << endl;
    }

    ResponseCode WsGate::HandleRobotsRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost)
    {
        response->SetHeader("Content-Type", "text/plain");
        response->SetBody("User-agent: *\nDisallow: /\n", 26);
        return HTTPRESPONSECODE_200_OK;
    }

    /* =================================== CURSOR HANDLING =================================== */
    ResponseCode WsGate::HandleCursorRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost)
    {
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
                RDP::cursor c = it->second->GetCursor(cid);
                time_t ct = c.get<0>();
                if (0 != ct) {
                    if (notModified(request, response, ct)) {
                        return HTTPRESPONSECODE_304_NOT_MODIFIED;
                    }
                    string png = c.get<1>();
                    if (!png.empty()) {
                        response->SetHeader("Content-Type", "image/cur");
                        response->SetLastModified(ct);
                        response->SetBody(png.data(), png.length());
                        LogInfo(request->RemoteAddress(), uri, "200 OK");
                        return HTTPRESPONSECODE_200_OK;
                    }
                }
            }
        }
        LogInfo(request->RemoteAddress(), uri, "404 Not Found");
        return HTTPRESPONSECODE_404_NOTFOUND;
    }

    /* =================================== REDIRECT REQUEST =================================== */
    ResponseCode WsGate::HandleRedirectRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost)
    {
        string dest(boost::starts_with(uri, "/wsgate?") ? "wss" : "https");
        //adding the sslPort to the dest Location
        if (this->m_ptIniConfig.get_optional<uint16_t>("ssl.port"))
        {
            stringstream sslPort;
            sslPort << this->m_ptIniConfig.get<uint16_t>("ssl.port");

            //Replace the http port with the ssl one
            string thisSslHost = thisHost.substr(0, thisHost.find(":")) + ":" + sslPort.str();      

            //append the rest of the uri
            dest.append("://").append(thisSslHost).append(uri);
            response->SetHeader("Location", dest);
            LogInfo(request->RemoteAddress(), uri, "301 Moved permanently");
            return HTTPRESPONSECODE_301_MOVEDPERMANENTLY;
        }
        else
        {
            LogInfo(request->RemoteAddress(), uri, "404 Not found");
            return HTTPRESPONSECODE_404_NOTFOUND;
        }

    }

    /* =================================== HANDLE WSGATE REQUEST =================================== */
    int WsGate::CheckIfWSocketRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost)
    {
        if (0 != request->HttpVersion().compare("1.1"))
        {
            LogInfo(request->RemoteAddress(), uri, "400 (Not HTTP 1.1)");
            return 400;
        }

        string wshost(to_lower_copy(request->Headers("Host")));
        string wsconn(to_lower_copy(request->Headers("Connection")));
        string wsupg(to_lower_copy(request->Headers("Upgrade")));
        string wsver(request->Headers("Sec-WebSocket-Version"));
        string wskey(request->Headers("Sec-WebSocket-Key"));

        string wsproto(request->Headers("Sec-WebSocket-Protocol"));
        string wsext(request->Headers("Sec-WebSocket-Extension"));

        if (!MultivalHeaderContains(wsconn, "upgrade"))
        {
            LogInfo(request->RemoteAddress(), uri, "400 (No upgrade header)");

            return 400;
        }
        if (!MultivalHeaderContains(wsupg, "websocket"))
        {
            LogInfo(request->RemoteAddress(), uri, "400 (Upgrade header does not contain websocket tag)");
            return 400;
        }
        if (0 != wshost.compare(thisHost))
        {
            LogInfo(request->RemoteAddress(), uri, "400 (Host header does not match)");
            return 400;
        }
        string wskey_decoded(base64_decode(wskey));

        if (16 != wskey_decoded.length())
        {
            LogInfo(request->RemoteAddress(), uri, "400 (Invalid WebSocket key)");
            return 400;
        }

        if (!MultivalHeaderContains(wsver, "13"))
        {
            response->SetHeader("Sec-WebSocket-Version", "13");
            LogInfo(request->RemoteAddress(), uri, "426 (Protocol version not 13)");
            return 426;
        }

        return 0;
    }

    ResponseCode WsGate::HandleWsgateRequest(HttpRequest *request, HttpResponse *response, std::string uri, std::string thisHost)
    {
        //FreeRDP Params
        string dtsize;
        string rdphost;
        string rdppcb;
        string rdpuser;
        int rdpport = 0;
        string rdppass;
        WsRdpParams params;
        bool setCookie = true;
        EmbeddedContext embeddedContext = CONTEXT_PLAIN;

        if(boost::starts_with(uri, "/wsgate?token="))
        {
            // OpenStack console authentication
            setCookie = false;
            try
            {
                log::info << "Starting OpenStack token authentication" << endl;

                string tokenId = request->FormValues("token").m_sBody;

                nova_console_token_auth* token_auth = nova_console_token_auth_factory::get_instance();

                nova_console_info info = token_auth->get_console_info(m_sOpenStackAuthUrl, m_sOpenStackUsername,
                                                                        m_sOpenStackPassword, m_sOpenStackProjectName,
                                                                        m_sOpenStackProjectId,
                                                                        m_sOpenStackProjectDomainName, m_sOpenStackUserDomainName,
                                                                        m_sOpenStackProjectDomainId, m_sOpenStackUserDomainId,
                                                                        tokenId, m_sOpenStackKeystoneVersion, m_sOpenStackRegion);

                log::info << "Host: " << info.host << " Port: " << info.port
                            << " Internal access path: " << info.internal_access_path
                            << endl;

                rdphost = info.host;
                rdpport = info.port;
                rdppcb = info.internal_access_path;

                rdpuser = m_sHyperVHostUsername;
                rdppass = m_sHyperVHostPassword;

                embeddedContext = CONTEXT_EMBEDDED;
            }
            catch(exception& ex)
            {
                log::err << "OpenStack token authentication failed: " << ex.what() << endl;
                return HTTPRESPONSECODE_400_BADREQUEST;
            }
        }

        params =
        {
            rdpport,
            1024,
            768,
            this->overrideParams.m_bOverrideRdpPerf ? this->overrideParams.m_RdpOverrideParams.perf : nFormValue(request, "perf", 0),
            this->overrideParams.m_bOverrideRdpFntlm ? this->overrideParams.m_RdpOverrideParams.fntlm : nFormValue(request, "fntlm", 0),
            this->overrideParams.m_bOverrideRdpNotls ? this->overrideParams.m_RdpOverrideParams.notls : nFormValue(request, "notls", 0),
            this->overrideParams.m_bOverrideRdpNonla ? this->overrideParams.m_RdpOverrideParams.nonla : nFormValue(request, "nonla", 0),
            this->overrideParams.m_bOverrideRdpNowallp ? this->overrideParams.m_RdpOverrideParams.nowallp : nFormValue(request, "nowallp", 0),
            this->overrideParams.m_bOverrideRdpNowdrag ? this->overrideParams.m_RdpOverrideParams.nowdrag : nFormValue(request, "nowdrag", 0),
            this->overrideParams.m_bOverrideRdpNomani ? this->overrideParams.m_RdpOverrideParams.nomani : nFormValue(request, "nomani", 0),
            this->overrideParams.m_bOverrideRdpNotheme ? this->overrideParams.m_RdpOverrideParams.notheme : nFormValue(request, "notheme", 0),
        };

        CheckForPredefined(rdphost, rdpuser, rdppass);

        if( !ConnectionIsAllowed(rdphost) ){
            LogInfo(request->RemoteAddress(), rdphost, "403 Denied by access rules");
            return HTTPRESPONSECODE_403_FORBIDDEN;
        }

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

        int wsocketCheck = CheckIfWSocketRequest(request, response, uri, thisHost);
        if(wsocketCheck != 0)
        {
            //using a switch in case of new errors being thrown from the wsocket check
            switch(wsocketCheck)
            {
                case 400:
                {
                    return HTTPRESPONSECODE_400_BADREQUEST;
                };
                case 426:
                {
                    return HTTPRESPONSECODE_426_UPGRADE_REQUIRED;
                };
            }
        }

        string wskey(request->Headers("Sec-WebSocket-Key"));
        SHA1 sha1;
        uint32_t digest[5];
        sha1 << wskey.c_str() << ws_magic;
        if (!sha1.Result(digest))
        {
            LogInfo(request->RemoteAddress(), uri, "500 (Digest calculation failed)");
            return HTTPRESPONSECODE_500_INTERNALSERVERERROR;
        }
        // Handle endianess
        for (int i = 0; i < 5; ++i)
        {
            digest[i] = htonl(digest[i]);
        }
        MyRawSocketHandler *sh = dynamic_cast<MyRawSocketHandler*>(GetRawSocketHandler());
        if (!sh)
        {
            throw tracing::runtime_error("No raw socket handler available");
        }
        response->EnableIdleTimeout(false);
        response->EnableKeepAlive(true);
        try
        {
            if (!sh->Prepare(request->Connection(), rdphost, rdppcb, rdpuser, rdppass, params, embeddedContext))
            {
                LogInfo(request->RemoteAddress(), uri, "503 (RDP backend not available)");
                response->EnableIdleTimeout(true);
                return HTTPRESPONSECODE_503_SERVICEUNAVAILABLE;
            }
        }
        catch(...)
        {
            log::info << "caught exception!" << endl;
        }

        response->RemoveHeader("Content-Type");
        response->RemoveHeader("Content-Length");
        response->RemoveHeader("Last-Modified");
        response->RemoveHeader("Cache-Control");

        string wsproto(request->Headers("Sec-WebSocket-Protocol"));
        if (0 < wsproto.length())
        {
            response->SetHeader("Sec-WebSocket-Protocol", wsproto);
        }
        response->SetHeader("Upgrade", "websocket");
        response->SetHeader("Connection", "Upgrade");
        response->SetHeader("Sec-WebSocket-Accept", base64_encode(reinterpret_cast<const unsigned char *>(digest), 20));

        LogInfo(request->RemoteAddress(), uri, "101");
        return HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS;
    }

    // generates a page for each http request
    ResponseCode WsGate::HandleRequest(HttpRequest *request, HttpResponse *response)
    {
        //Connection Params
        string uri = request->Uri();
        string thisHost = m_sHostname.empty() ? request->Headers("Host") : m_sHostname;

        //add new behaviour note:
        //those requests that have the same beggining have to be placed with an if else condition

        if(request->Method() != REQUESTMETHOD_GET)
            return HTTPRESPONSECODE_400_BADREQUEST;

        if (m_bRedirect && (!request->Secure()))
        {
            return HandleRedirectRequest(request, response, uri, thisHost);
        }

        if(boost::starts_with(uri, "/robots.txt"))
        {
            return HandleRobotsRequest(request, response, uri, thisHost);
        }
        if(boost::starts_with(uri, "/cur/"))
        {
            return HandleCursorRequest(request, response, uri, thisHost);
        }

        if(boost::starts_with(uri, "/wsgate"))
        {
            return HandleWsgateRequest(request, response, uri, thisHost);
        }

        if(boost::starts_with(uri, "/connect?"))
        {
            //handle a direct connection via queryString here
            return HTTPRESPONSECODE_200_OK;
        }
        return HandleHTTPRequest(request, response);
    }

    ResponseCode WsGate::HandleHTTPRequest(HttpRequest *request, HttpResponse *response, bool tokenAuth)
    {
        string uri(request->Uri());
        string thisHost(m_sHostname.empty() ? request->Headers("Host") : m_sHostname);

        // Regular (non WebSockets) request
        bool bDynDebug = m_bDebug;
        if (!bDynDebug) {
            // Enable debugging by using a custom UserAgent header
            if (iequals(request->Headers("X-WSGate-Debug"), "true")) {
                bDynDebug = true;
            }
        }
        if (0 != thisHost.compare(request->Headers("Host"))) {
            LogInfo(request->RemoteAddress(), uri, "404 Not found");
            return HTTPRESPONSECODE_404_NOTFOUND;
        }

        path p(m_sDocumentRoot);
        p /= uri;
        if (ends_with(uri, "/")) {
            p /= (bDynDebug ? "/index-debug.html" : "/index.html");
        }
        p.normalize();
        bool externalRequest = false;

        if (!exists(p)) {
            p = m_sDocumentRoot;
            p /= "index.html";
        }

        if (!is_regular_file(p)) {
            LogInfo(request->RemoteAddress(), uri, "403 Forbidden");
            log::warn << "Request from " << request->RemoteAddress()
                << ": " << uri << " => 403 Forbidden" << endl;

            p = m_sDocumentRoot;
            p /= "index.html";
        }

        // Handle If-modified-sice request header
        time_t mtime = last_write_time(p);
        if (notModified(request, response, mtime)) {
            return HTTPRESPONSECODE_304_NOT_MODIFIED;
        }
        response->SetLastModified(mtime);

        string body;
        StaticCache::iterator ci = m_StaticCache.find(p);
        if ((m_StaticCache.end() != ci) && (ci->second.get<0>() == mtime)) {
            body.assign(ci->second.get<1>());
        } else {
            fs::ifstream f(p, ios::binary);
            if (f.fail()) {
                log::warn << "Request from " << request->RemoteAddress()
                    << ": " << uri << " => 404 (file '" << p << "' unreadable)" << endl;
                return HTTPRESPONSECODE_404_NOTFOUND;
            }
            body.assign((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
            f.close();
            m_StaticCache[p] = cache_entry(mtime, body);
        }

#ifdef BOOST_FILESYSTEM_VERSION
#if (BOOST_FILESYSTEM_VERSION >= 3)
        string basename(p.filename().generic_string());
#else
        string basename(p.filename());
#endif
#else
        // Not defined at all: old API
        string basename(p.filename());
#endif

        MimeType mt = simpleMime(to_lower_copy(basename));
        if (HTML == mt) {
            ostringstream oss;

            oss << (request->Secure() ? "wss://" : "ws://") << thisHost << "/wsgate";

            replace_all(body, "%WSURI%", oss.str());
            replace_all(body, "%JSDEBUG%", (bDynDebug ? "-debug" : ""));
            string tmp;
            if(externalRequest)
            {
                string dtsize(request->FormValues("dtsize").m_sBody);
                string port(request->FormValues("port").m_sBody);

                replace_all(body, "%COOKIE_LASTUSER%", request->FormValues("user").m_sBody);
                replace_all(body, "%COOKIE_LASTPASS%",  base64_decode(request->FormValues("pass").m_sBody)); // Passw0rd
                replace_all(body, "%COOKIE_LASTHOST%", request->FormValues("host").m_sBody);
                replace_all(body, "%COOKIE_LASTPCB%", request->FormValues("pcb").m_sBody);
                replace_all(body, "var externalConnection = false;", "var externalConnection = true;");
            }
            else
            {
                tmp.assign(this->overrideParams.m_bOverrideRdpUser ? "<predefined>" : request->Cookies("lastuser"));
                replace_all(body, "%COOKIE_LASTUSER%", tmp);
                tmp.assign(this->overrideParams.m_bOverrideRdpUser ? "disabled=\"disabled\"" : "");
                replace_all(body, "%DISABLED_USER%", tmp);
                tmp.assign(this->overrideParams.m_bOverrideRdpPass ? "SomthingUseless" : base64_decode(request->Cookies("lastpass")));
                replace_all(body, "%COOKIE_LASTPASS%", tmp);
                tmp.assign(this->overrideParams.m_bOverrideRdpPass ? "disabled=\"disabled\"" : "");
                replace_all(body, "%DISABLED_PASS%", tmp);

                tmp.assign(this->overrideParams.m_bOverrideRdpHost ? "<predefined>" : request->Cookies("lasthost"));
                replace_all(body, "%COOKIE_LASTHOST%", tmp);
                tmp.assign(this->overrideParams.m_bOverrideRdpHost ? "disabled=\"disabled\"" : "");
                replace_all(body, "%DISABLED_HOST%", tmp);

                tmp.assign(this->overrideParams.m_bOverrideRdpPcb ? "<predefined>" : request->Cookies("lastpcb"));
                replace_all(body, "%COOKIE_LASTPCB%", tmp);
                tmp.assign(this->overrideParams.m_bOverrideRdpPcb ? "disabled=\"disabled\"" : "");
                replace_all(body, "%DISABLED_PCB%", tmp);
            }

            tmp.assign(VERSION).append(".").append(GITREV);
            replace_all(body, "%VERSION%", tmp);

            //The new Port Selector
            if (this->overrideParams.m_bOverrideRdpPort) {
                replace_all(body, "%DISABLED_PORT%", "disabled=\"disabled\"");
            } else {
                replace_all(body, "%DISABLED_PORT%", "");
            }

            tmp.assign(this->overrideParams.m_bOverrideRdpPort ? boost::lexical_cast<string>(this->overrideParams.m_RdpOverrideParams.port) : "3389");
            replace_all(body, "%DEFAULT_PORT%", tmp);

            //The Desktop Resolution
            if (this->overrideParams.m_bOverrideRdpPerf) {
                replace_all(body, "%DISABLED_PERF%", "disabled=\"disabled\"");
                replace_all(body, "%SELECTED_PERF0%", (0 == this->overrideParams.m_RdpOverrideParams.perf) ? "selected" : "");
                replace_all(body, "%SELECTED_PERF1%", (1 == this->overrideParams.m_RdpOverrideParams.perf) ? "selected" : "");
                replace_all(body, "%SELECTED_PERF2%", (2 == this->overrideParams.m_RdpOverrideParams.perf) ? "selected" : "");
            } else {
                replace_all(body, "%DISABLED_PERF%", "");
                replace_all(body, "%SELECTED_PERF0%", "");
                replace_all(body, "%SELECTED_PERF1%", "");
                replace_all(body, "%SELECTED_PERF2%", "");
            }


            if (this->overrideParams.m_bOverrideRdpFntlm) {
                replace_all(body, "%DISABLED_FNTLM%", "disabled=\"disabled\"");
                replace_all(body, "%SELECTED_FNTLM0%", (0 == this->overrideParams.m_RdpOverrideParams.fntlm) ? "selected" : "");
                replace_all(body, "%SELECTED_FNTLM1%", (1 == this->overrideParams.m_RdpOverrideParams.fntlm) ? "selected" : "");
                replace_all(body, "%SELECTED_FNTLM2%", (2 == this->overrideParams.m_RdpOverrideParams.fntlm) ? "selected" : "");
            } else {
                replace_all(body, "%DISABLED_FNTLM%", "");
                replace_all(body, "%SELECTED_FNTLM0%", "");
                replace_all(body, "%SELECTED_FNTLM1%", "");
                replace_all(body, "%SELECTED_FNTLM2%", "");
            }
            if (this->overrideParams.m_bOverrideRdpNowallp) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.nowallp) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NOWALLP%", tmp);
            if (this->overrideParams.m_bOverrideRdpNowdrag) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.nowdrag) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NOWDRAG%", tmp);
            if (this->overrideParams.m_bOverrideRdpNomani) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.nomani) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NOMANI%", tmp);
            if (this->overrideParams.m_bOverrideRdpNotheme) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.notheme) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NOTHEME%", tmp);
            if (this->overrideParams.m_bOverrideRdpNotls) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.notls) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NOTLS%", tmp);
            if (this->overrideParams.m_bOverrideRdpNonla) {
                tmp.assign("disabled=\"disabled\"").append((this->overrideParams.m_RdpOverrideParams.nonla) ? " checked=\"checked\"" : "");
            } else {
                tmp.assign("");
            }
            replace_all(body, "%CHECKED_NONLA%", tmp);
        }
        switch (mt) {
            case TEXT:
                response->SetHeader("Content-Type", "text/plain");
                response->SetHeader("Cache-Control", "no-cache, private");
                break;
            case HTML:
                response->SetHeader("Content-Type", "text/html");
                response->SetHeader("Cache-Control", "no-cache, private");
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
            case JSON:
                response->SetHeader("Content-Type", "application/json");
                break;
            case CSS:
                response->SetHeader("Content-Type", "text/css");
                break;
            case OGG:
                response->SetHeader("Content-Type", "audio/ogg");
                break;
            case CUR:
                response->SetHeader("Content-Type", "image/cur");
                break;
            case BINARY:
                response->SetHeader("Content-Type", "application/octet-stream");
                break;
        }
        response->SetBody(body.data(), body.length());

        LogInfo(request->RemoteAddress(), uri, "200 OK");
        return HTTPRESPONSECODE_200_OK;
    }

    boost::property_tree::ptree WsGate::GetConfig() {
        return this->m_ptIniConfig;
    }

    const string & WsGate::GetConfigFile() {
        return m_sConfigFile;
    }

    bool WsGate::GetEnableCore() {
        return m_bEnableCore;
    }

    bool WsGate::SetConfigFile(const string &name) {
        if (name.empty()) {
#ifdef _WIN32
            wsgate::log::err << "Config filename is empty." << endl;
#endif
            cerr << "Config filename is empty." << endl;
            return false;
        }
        m_sConfigFile = name;
        return true;
    }

    bool WsGate::ReadConfig(wsgate::log *logger) {
        // config file options
        po::options_description cfg("");
        cfg.add_options()
            ("global.daemon", po::value<string>(), "enable/disable daemon mode")
            ("global.pidfile", po::value<string>(), "path of PID file in daemon mode")
            ("global.debug", po::value<string>(), "enable/disable debugging")
            ("global.enablecore", po::value<string>(), "enable/disable coredumps")
            ("global.hostname", po::value<string>(), "specify host name")
            ("global.port", po::value<uint16_t>(), "specify listening port")
            ("global.bindaddr", po::value<string>(), "specify bind address")
            ("global.redirect", po::value<string>(), "Flag: Always redirect non-SSL to SSL")
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
            ("openstack.authurl", po::value<string>(), "OpenStack authentication URL")
            ("openstack.username", po::value<string>(), "OpenStack username")
            ("openstack.password", po::value<string>(), "OpenStack password")
            ("openstack.tenantname", po::value<string>(), "OpenStack tenant name")
            ("hyperv.hostusername", po::value<string>(), "Hyper-V username")
            ("hyperv.hostpassword", po::value<string>(), "Hyper-V user's password")
            ;

        try {
            boost::property_tree::ini_parser::read_ini(m_sConfigFile, this->m_ptIniConfig);
            boost::property_tree::ptree pt = this->m_ptIniConfig;

            try {
                // Examine values from config file
                m_bDaemon = str2bool(pt.get<std::string>("global.daemon","false"));
                m_bDebug = str2bool(pt.get<std::string>("global.debug","false"));
                m_bEnableCore = str2bool(pt.get<std::string>("global.enablecore","false"));
                if (pt.get_optional<std::string>("global.logmask")) {
                    if (NULL != logger) {
                        logger->setmaskByName(to_upper_copy(pt.get<std::string>("global.logmask")));
                    }
                }
                if (pt.get_optional<std::string>("global.logfacility")) {
                    if (NULL != logger) {
                        logger->setfacilityByName(to_upper_copy(pt.get<std::string>("global.logfacility")));
                    }
                }
                if (!pt.get_optional<std::string>("global.port") && !pt.get_optional<std::string>("ssl.port")) {
                    throw tracing::invalid_argument("No listening ports defined.");
                }

                if (!pt.get_optional<std::string>("http.documentroot")) {
                    throw tracing::invalid_argument("No documentroot defined.");
                }
                m_sDocumentRoot.assign(pt.get<std::string>("http.documentroot"));
                if (m_sDocumentRoot.empty()) {
                    throw tracing::invalid_argument("documentroot is empty.");
                }

                m_bRedirect = str2bool(pt.get<std::string>("global.redirect","false"));
                        
                if (pt.get_optional<std::string>("acl.order")) {
                    setAclOrder(pt.get<std::string>("acl.order"));
                }
                if (pt.get_child_optional("acl.allow")) {
                    setHostList(as_vector<std::string>(pt, "acl.allow"), m_allowedHosts);
                }
                if (pt.get_child_optional("acl.deny")) {
                    setHostList(as_vector<std::string>(pt, "acl.deny"), m_deniedHosts);
                }

                if (pt.get_optional<std::string>("rdpoverride.host")) {
                    this->overrideParams.m_sRdpOverrideHost.assign(pt.get<std::string>("rdpoverride.host"));
                    this->overrideParams.m_bOverrideRdpHost = true;
                } else {
                    this->overrideParams.m_bOverrideRdpHost = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.user")) {
                    this->overrideParams.m_sRdpOverrideUser.assign(pt.get<std::string>("rdpoverride.user"));
                    this->overrideParams.m_bOverrideRdpUser = true;
                } else {
                    this->overrideParams.m_bOverrideRdpUser = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.pass")) {
                    this->overrideParams.m_sRdpOverridePass.assign(pt.get<std::string>("rdpoverride.pass"));
                    this->overrideParams.m_bOverrideRdpPass = true;
                } else {
                    this->overrideParams.m_bOverrideRdpPass = false;
                }

                if (pt.get_optional<std::string>("rdpoverride.pcb")) {
                    this->overrideParams.m_sRdpOverridePcb.assign(pt.get<std::string>("rdpoverride.pcb"));
                    this->overrideParams.m_bOverrideRdpPcb = true;
                }
                else {
                    this->overrideParams.m_bOverrideRdpPcb = false;
                }

                if (pt.get_optional<int>("rdpoverride.port")) {
                    int n = pt.get<int>("rdpoverride.port");
                    if ((0 > n) || (65536 < n)) {
                        throw tracing::invalid_argument("Invalid port value.");
                    }
                    this->overrideParams.m_RdpOverrideParams.port = n;
                    this->overrideParams.m_bOverrideRdpPort = true;
                } else {
                    this->overrideParams.m_bOverrideRdpPort = false;
                }

                if (pt.get_optional<int>("rdpoverride.performance")) {
                    int n = pt.get<int>("rdpoverride.performance");
                    if ((0 > n) || (2 < n)) {
                        throw tracing::invalid_argument("Invalid performance value.");
                    }
                    this->overrideParams.m_RdpOverrideParams.perf = n;
                    this->overrideParams.m_bOverrideRdpPerf = true;
                } else {
                    this->overrideParams.m_bOverrideRdpPerf = false;
                }
                if (pt.get_optional<int>("rdpoverride.forcentlm")) {
                    int n = pt.get<int>("rdpoverride.forcentlm");
                    if ((0 > n) || (2 < n)) {
                        throw tracing::invalid_argument("Invalid forcentlm value.");
                    }
                    this->overrideParams.m_RdpOverrideParams.fntlm = n;
                    this->overrideParams.m_bOverrideRdpFntlm = true;
                } else {
                    this->overrideParams.m_bOverrideRdpFntlm = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.nowallpaper")) {
                    this->overrideParams.m_RdpOverrideParams.nowallp = str2bint(pt.get<std::string>("rdpoverride.nowallpaper"));
                    this->overrideParams.m_bOverrideRdpNowallp = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNowallp = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.nofullwindowdrag")) {
                    this->overrideParams.m_RdpOverrideParams.nowdrag = str2bint(pt.get<std::string>("rdpoverride.nofullwindowdrag"));
                    this->overrideParams.m_bOverrideRdpNowdrag = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNowdrag = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.nomenuanimation")) {
                    this->overrideParams.m_RdpOverrideParams.nomani = str2bint(pt.get<std::string>("rdpoverride.nomenuanimation"));
                    this->overrideParams.m_bOverrideRdpNomani = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNomani = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.notheming")) {
                    this->overrideParams.m_RdpOverrideParams.notheme = str2bint(pt.get<std::string>("rdpoverride.notheming"));
                    this->overrideParams.m_bOverrideRdpNotheme = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNotheme = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.notls")) {
                    this->overrideParams.m_RdpOverrideParams.notls = str2bint(pt.get<std::string>("rdpoverride.notls"));
                    this->overrideParams.m_bOverrideRdpNotls = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNotls = false;
                }
                if (pt.get_optional<std::string>("rdpoverride.nonla")) {
                    this->overrideParams.m_RdpOverrideParams.nonla = str2bint(pt.get<std::string>("rdpoverride.nonla"));
                    this->overrideParams.m_bOverrideRdpNonla = true;
                } else {
                    this->overrideParams.m_bOverrideRdpNonla = false;
                }
                if (pt.get_optional<std::string>("global.hostname")) {
                    m_sHostname.assign(pt.get<std::string>("global.hostname"));
                } else {
                    m_sHostname.clear();
                }
                if (pt.get_optional<std::string>("openstack.authurl")) {
                    m_sOpenStackAuthUrl.assign(pt.get<std::string>("openstack.authurl"));
                } else {
                    m_sOpenStackAuthUrl.clear();
                }
                if (pt.get_optional<std::string>("openstack.username")) {
                    m_sOpenStackUsername.assign(pt.get<std::string>("openstack.username"));
                } else {
                    m_sOpenStackUsername.clear();
                }
                if (pt.get_optional<std::string>("openstack.password")) {
                    m_sOpenStackPassword.assign(pt.get<std::string>("openstack.password"));
                } else {
                    m_sOpenStackPassword.clear();
                }
                if (pt.get_optional<std::string>("openstack.projectname")) {
                    m_sOpenStackProjectName.assign(pt.get<std::string>("openstack.projectname"));
                }
                else if (pt.get_optional<std::string>("openstack.tenantname")) {
                    m_sOpenStackProjectName.assign(pt.get<std::string>("openstack.tenantname"));
                }
                else {
                    m_sOpenStackProjectName.clear();
                }
                if (pt.get_optional<std::string>("openstack.projectid")) {
                    m_sOpenStackProjectId.assign(pt.get<std::string>("openstack.projectid"));
                }
                else {
                    m_sOpenStackProjectId.clear();
                }
                if (pt.get_optional<std::string>("openstack.projectdomainname")) {
                    m_sOpenStackProjectDomainName.assign(pt.get<std::string>("openstack.projectdomainname"));
                }
                else {
                    m_sOpenStackProjectDomainName.assign("default");
                }
                if (pt.get_optional<std::string>("openstack.userdomainname")) {
                    m_sOpenStackUserDomainName.assign(pt.get<std::string>("openstack.userdomainname"));
                }
                else {
                    m_sOpenStackUserDomainName.assign("default");
                }
                if (pt.get_optional<std::string>("openstack.projectdomainid")) {
                    m_sOpenStackProjectDomainId.assign(pt.get<std::string>("openstack.projectdomainid"));
                }
                else {
                    m_sOpenStackProjectDomainId.clear();
                }
                if (pt.get_optional<std::string>("openstack.userdomainid")) {
                    m_sOpenStackUserDomainId.assign(pt.get<std::string>("openstack.userdomainid"));
                }
                else {
                    m_sOpenStackUserDomainId.clear();
                }
                if (pt.get_optional<std::string>("openstack.keystoneversion")) {
                    m_sOpenStackKeystoneVersion.assign(pt.get<std::string>("openstack.keystoneversion"));
                }
                else {
                    m_sOpenStackKeystoneVersion = KEYSTONE_V2;
                }
                if (pt.get_optional<std::string>("openstack.region")) {
                    m_sOpenStackRegion.assign(pt.get<std::string>("openstack.region"));
                }
                else {
                    m_sOpenStackRegion.clear();
                }
                if (pt.get_optional<std::string>("hyperv.hostusername")) {
                    m_sHyperVHostUsername.assign(pt.get<std::string>("hyperv.hostusername"));
                } else {
                    m_sHyperVHostUsername.clear();
                }
                if (pt.get_optional<std::string>("hyperv.hostpassword")) {
                    m_sHyperVHostPassword.assign(pt.get<std::string>("hyperv.hostpassword"));
                } else {
                    m_sHyperVHostPassword.clear();
                }
            } catch (const tracing::invalid_argument & e) {
                cerr << e.what() << endl;
                wsgate::log::err << e.what() << endl;
                wsgate::log::err << e.where() << endl;
                return false;
            }

        } catch (const boost::property_tree::ini_parser_error &e) {
            cerr << e.what() << endl;
            return false;
        }
        return true;
    }

    template <typename T> std::vector<T> WsGate::as_vector(boost::property_tree::ptree const& pt, boost::property_tree::ptree::key_type const& key){
        std::vector<T> r;
        for (auto& item : pt.get_child(key))
            r.push_back(item.second.get_value<T>());
        return r;
    }

    bool WsGate::notModified(HttpRequest *request, HttpResponse *response, time_t mtime)
    {
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
                    << ": " << request->Uri() << " => 304 Not modified" << endl;
                return true;
            }
        }
        return false;
    }

    int WsGate::str2bint(const string &s) {
        string v(s);
        trim(v);
        if (!v.empty()) {
            if (iequals(v, "true")) {
                return 1;
            }
            if (iequals(v, "yes")) {
                return 1;
            }
            if (iequals(v, "on")) {
                return 1;
            }
            if (iequals(v, "1")) {
                return 1;
            }
            if (iequals(v, "false")) {
                return 0;
            }
            if (iequals(v, "no")) {
                return 0;
            }
            if (iequals(v, "off")) {
                return 0;
            }
            if (iequals(v, "0")) {
                return 0;
            }
        }
        throw tracing::invalid_argument("Invalid boolean value");
    }

    bool WsGate::str2bool(const string &s) {
        return (1 == str2bint(s));
    }

    void WsGate::wc2pat(string &wildcards) {
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

    void WsGate::setHostList(const vector<string> &hosts, vector<boost::regex> &hostlist) {
        vector<string> tmp(hosts);
        vector<string>::iterator i;
        hostlist.clear();
        for (i = tmp.begin(); i != tmp.end(); ++i) {
            wc2pat(*i);
            boost::regex re(*i, boost::regex::icase);
            hostlist.push_back(re);
        }
    }

    void WsGate::setAclOrder(const string &order) {
        vector<string> parts;
        boost::split(parts, order, is_any_of(","));
        if (2 == parts.size()) {
            trim(parts[0]);
            trim(parts[1]);
            if (iequals(parts[0],"deny") && iequals(parts[1],"allow")) {
                m_bOrderDenyAllow = true;
                return;
            }
            if (iequals(parts[0],"allow") && iequals(parts[1],"deny")) {
                m_bOrderDenyAllow = false;
                return;
            }
        }
        throw tracing::invalid_argument("Invalid acl order value.");
    }

    bool WsGate::GetDaemon() {
        return m_bDaemon;
    }

    void WsGate::SetPidFile(const string &name) {
        m_sPidFile = name;
    }

    void WsGate::RegisterRdpSession(rdp_ptr rdp) {
        ostringstream oss;
        oss << hex << rdp.get();
        m_SessionMap[oss.str()] = rdp;
    }

    void WsGate::UnregisterRdpSession(rdp_ptr rdp) {
        ostringstream oss;
        oss << hex << rdp.get();
        m_SessionMap.erase(oss.str());
    }

    WsRdpOverrideParams WsGate::getOverrideParams(){
        return this->overrideParams;
    }
}
