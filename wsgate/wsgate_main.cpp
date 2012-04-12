/* $Id$
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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
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

using namespace std;
using boost::algorithm::to_lower_copy;
using boost::algorithm::ends_with;
using boost::algorithm::replace_all;
using boost::algorithm::to_upper_copy;
using boost::algorithm::trim_right_copy_if;
using boost::algorithm::is_any_of;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
using boost::filesystem::path;

namespace wsgate {

    static const char * const ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

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
                , m_sHostname("localhost")
                , m_sDocumentRoot()
                , m_bDebug(false)
                {
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
                log::debug << "Request from " << request->RemoteAddress()
                    << ": " << uri << endl;

                if (0 == uri.compare("/wsgate")) {
                    string rdphost(request->FormValues("host").m_sBody);
                    string rdpuser(request->FormValues("user").m_sBody);
                    string rdppass(request->FormValues("pass").m_sBody);
                    uint16_t rdpport = 3389;
                    if (!request->FormValues("port").m_sBody.empty()) {
                        try {
                            int tmp = boost::lexical_cast<int>(request->FormValues("port").m_sBody);
                            rdpport = tmp;
                        } catch (const boost::bad_lexical_cast & e) { }
                    }
                    string mode(request->FormValues("mode").m_sBody);
                    if (mode.empty()) {
                        mode = "echo";
                    }
                    response->SetBody("", 0);
                    if (0 != request->HttpVersion().compare("1.1")) {
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
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    if (!MultivalHeaderContains(wsupg, "websocket")) {
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    if (0 != wshost.compare(m_sHostname)) {
                        log::warn << "wshost:'" << wshost << "' hostname:'" << m_sHostname << "'" << endl;
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    string wskey_decoded(base64_decode(wskey));
                    if (16 != wskey_decoded.length()) {
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
                    SHA1 sha1;
                    uint32_t digest[5];
                    sha1 << wskey.c_str() << ws_magic;
                    if (!sha1.Result(digest)) {
                        return HTTPRESPONSECODE_500_INTERNALSERVERERROR;
                    }
                    // Handle endianess
                    for (int i = 0; i < 5; ++i) {
                        digest[i] = htonl(digest[i]);
                    }

                    if (!MultivalHeaderContains(wsver, "13")) {
                        response->SetHeader("Sec-WebSocket-Version", "13");
                        return HTTPRESPONSECODE_426_UPGRADE_REQUIRED;
                    }

                    MyRawSocketHandler *sh = dynamic_cast<MyRawSocketHandler*>(GetRawSocketHandler());
                    if (!sh) {
                        throw tracing::runtime_error("No raw socket handler available");
                    }
                    if (!sh->Prepare(request->Connection(), mode, rdphost, rdpport, rdpuser,rdppass)) {
                        return HTTPRESPONSECODE_503_SERVICEUNAVAILABLE;
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
                    return HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS;
                }

                path p(m_sDocumentRoot);
                p /= uri;
                if (ends_with(uri, "/")) {
                    p /= (m_bDebug ? "/index-debug.html" : "/index.html");
                }
                p.normalize();

                if (!exists(p)) {
                    log::warn << "Does not exist: '" << p << "'" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }
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
                        return HTTPRESPONSECODE_304_NOT_MODIFIED;
                    }
                }
                response->SetLastModified(mtime);

                fs::ifstream f(p, ios::binary);
                if (f.fail()) {
                    log::warn << "Could not open '" << p << "'" << endl;
                    return HTTPRESPONSECODE_404_NOTFOUND;
                }
                f.seekg (0, ios::end);
                size_t fsize = f.tellg();
                f.seekg (0, ios::beg);
                char *buf = new char [fsize];
                f.read (buf,fsize);
                f.close();
                string rbuf(buf, fsize);
                delete buf;
                MimeType mt = simpleMime(to_lower_copy(p.filename().generic_string()));
                if (HTML == mt) {
                    ostringstream oss;
                    oss << (request->Secure() ? "wss://" : "ws://")
                        << m_sHostname << ":" << request->LocalPort() << "/wsgate";
                    replace_all(rbuf, "%WSURI%", oss.str());
                    replace_all(rbuf, "%JSDEBUG%", (m_bDebug ? "-debug" : ""));
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
                response->SetBody(rbuf.data(), rbuf.length());
                return HTTPRESPONSECODE_200_OK;
            }

            void SetHostname(const string &name) {
                m_sHostname = name;
            }

            void SetDocumentRoot(const string &name) {
                m_sDocumentRoot = name;
            }

            void SetDebug(const bool debug) {
                m_bDebug = debug;
            }

        private:
            string m_sHostname;
            string m_sDocumentRoot;
            bool m_bDebug;
    };

#ifndef _WIN32
    // Bind helper is not needed on win32, because win32 does not
    // have a concept of privileged ports.
    class MyBindHelper : public PrivilegedBindHelper
    {
        public:
            MyBindHelper() : mutex(pthread_mutex_t())
        {
            pthread_mutex_init(&mutex, NULL);
        }

            virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port)
            {
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
                            if (0 != status)
                                log::err << BINDHELPER_PATH << ": " << strerror(WEXITSTATUS(status)) << endl;
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
            typedef enum {
                SimpleEcho,
                SpeedTest,
                Rdp
            } Mode;

            MyWsHandler(EHSConnection *econn, EHS *ehs)
                : m_econn(econn)
                  , m_ehs(ehs)
                  , m_emode(SimpleEcho)
        {}

            void SetMode(Mode mode) {
                m_emode = mode;
            }
            virtual void on_message(std::string, std::string data) {
                switch (m_emode) {
                    case SimpleEcho:
                        log::debug << "GOT Message: '" << data << "'" << endl;
                        data.insert(0, "Yes, ");
                        send_text(data);
                        break;
                    case SpeedTest:
                        log::debug << "GOT Message, size: " << data.length() << endl;
                        send_binary(data);
                        break;
                    case Rdp:
                        log::debug << "GOT Message, size: " << data.length() << endl;
                        break;
                }
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
                switch (m_emode) {
                    case SimpleEcho:
                        log::debug << "Send WS response '" << data << "'" << endl;
                        break;
                    case SpeedTest:
                        log::debug << "Send WS response, size: " << data.length() << endl;
                        break;
                    case Rdp:
                        // log::debug << "Send WS response, size: " << data.length() << endl;
                        break;
                }
                ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
                r->SetBody(data.data(), data.length());
                m_ehs->AddResponse(ehs_move(r));
            }

        private:
            MyWsHandler(const MyWsHandler&);
            MyWsHandler& operator=(const MyWsHandler&);
            EHSConnection *m_econn;
            EHS *m_ehs;
            Mode m_emode;
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
        m_cmap.erase(conn);
    }

    bool MyRawSocketHandler::Prepare(EHSConnection *conn, const string mode,
            const string host, uint16_t port, const string user, const string pass)
    {
        log::debug << "Mode:         '" << mode << "'" << endl;
        log::debug << "RDP Host:     '" << host << "'" << endl;
        log::debug << "RDP Port:     '" << port << "'" << endl;
        log::debug << "RDP User:     '" << user << "'" << endl;
        log::debug << "RDP Password: '" << pass << "'" << endl;

        handler_ptr h(new MyWsHandler(conn, m_parent));
        conn_ptr c(new wspp::wsendpoint(h.get()));
        rdp_ptr r(new RDP(h.get()));
        m_cmap[conn] = conn_tuple(c, h, r);
        if (0 == mode.compare("echo")) {
            dynamic_cast<MyWsHandler*>(h.get())->SetMode(MyWsHandler::SimpleEcho);
        }
        if (0 == mode.compare("speed")) {
            dynamic_cast<MyWsHandler*>(h.get())->SetMode(MyWsHandler::SpeedTest);
        }
        if (0 == mode.compare("rdp")) {
            dynamic_cast<MyWsHandler*>(h.get())->SetMode(MyWsHandler::Rdp);
            r->Connect(host, port, user, string() /*domain*/, pass);
        }
        return true;
    }

}

// basic main that creates a threaded EHS object and then
//   sleeps forever and lets the EHS thread do its job.
int main (int argc, char **argv)
{
    wsgate::logger log("wsgate");

    po::options_description desc("Supported options");
    desc.add_options()
        ("help,h", "Show this message and exit.")
        ("version,V", "Show version information and exit.")
        ("config,c", po::value<string>()->default_value(DEFAULTCFG), "Specify config file")
        ;

    po::options_description cfg("");
    cfg.add_options()
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
        ;

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const po::error &e) {
        cerr << e.what() << endl << "Hint: Use --help option." << endl;
        return -1;
    }

    if (vm.count("help")) {
        cout << desc << endl;
        return 0;
    }
    if (vm.count("version")) {
        cout << "wsgate v" << VERSION << endl << getEHSconfig() << endl;
        return 0;
    }
    if (vm.count("config")) {
        string cfgname(vm["config"].as<string>());
        ifstream f(cfgname.c_str());
        if (f.fail()) {
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
        cerr << "Mandatory option --config <filename> is missing." << endl;
        return -1;
    }

    wsgate::WsGate srv;

    // Examine values from config file
    if (vm.count("global.debug")) {
        srv.SetDebug(0 == vm["global.debug"].as<string>().compare("true"));
    }

    if (vm.count("global.logmask")) {
        log.setmaskByName(to_upper_copy(vm["global.logmask"].as<string>()));
    }
    if (vm.count("global.logfacility")) {
        log.setfacilityByName(to_upper_copy(vm["global.logfacility"].as<string>()));
    }
    if (0 == (vm.count("global.port") + vm.count("ssl.port"))) {
        cerr << "No listening ports defined." << endl;
        return -1;
    }
    if (0 == (vm.count("global.hostname"))) {
        cerr << "No hostname defined." << endl;
        return -1;
    }
    if (0 == (vm.count("http.documentroot"))) {
        cerr << "No documentroot defined." << endl;
        return -1;
    }


    string hostname(vm["global.hostname"].as<string>());
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
    srv.SetHostname(hostname);
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

    try {
        wsgate::log::info << "wsgate v" << VERSION << " starting" << endl;
        srv.StartServer(oSP);

        wsgate::log::info << "Listening on " << oSP["bindaddress"].GetCharString() << ":" << oSP["port"].GetInt() << endl;

        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            if (sleepInLoop) {
                usleep(1000);
            } else {
                srv.HandleData(1000);
            }
        }

        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }

    return 0;
}
