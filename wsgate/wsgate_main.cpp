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

#include "common.h"
#include "btexception.h"
#include "base64.h"
#include "sha1.h"
#include "logging.h"
#include "wsendpoint.h"
#include "wsgate.h"

using namespace std;
using boost::algorithm::to_lower_copy;
using boost::algorithm::ends_with;
using boost::algorithm::replace_all;
using boost::algorithm::to_upper_copy;
namespace po = boost::program_options;

namespace wsgate {

    static const char * const ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // subclass of EHS that defines a custom HTTP response.
    class WsGate : public EHS
    {
        public:

            WsGate(EHS *parent = NULL, std::string registerpath = "")
                : EHS(parent, registerpath)
                , m_sHostname("localhost")
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
                if (0 == request->Uri().compare("/wsgate")) {
                    response->SetBody("", 0);
                    if (REQUESTMETHOD_GET != request->Method()) {
                        return HTTPRESPONSECODE_400_BADREQUEST;
                    }
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

                    response->RemoveHeader("Content-Type");
                    response->RemoveHeader("Content-Length");
                    response->RemoveHeader("Last-Modified");
                    response->RemoveHeader("Cache-Control");

                    if (!MultivalHeaderContains(wsver, "13")) {
                        response->SetHeader("Sec-WebSocket-Version", "13");
                        return HTTPRESPONSECODE_426_UPGRADE_REQUIRED;
                    }
                    if (0 < wsproto.length()) {
                        response->SetHeader("Sec-WebSocket-Protocol", wsproto);
                    }
                    response->SetHeader("Upgrade", "websocket");
                    response->SetHeader("Connection", "Upgrade");
                    response->SetHeader("Sec-WebSocket-Accept",
                            base64_encode(reinterpret_cast<const unsigned char *>(digest), 20));
                    return HTTPRESPONSECODE_101_SWITCHING_PROTOCOLS;
                }
                std::string path(m_sDocumentRoot);
                path.append(request->Uri());
                if (ends_with(path, "/")) {
                    path.append("index.html");
                }
                ifstream f(path.c_str(), ios::binary);
                if (f.fail()) {
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
                ostringstream oss;
                oss << (request->Secure() ? "wss://" : "ws://")
                    << m_sHostname << ":" << request->LocalPort() << "/wsgate";
                replace_all(rbuf, "%WSURI%", oss.str());
                response->SetBody(rbuf.data(), rbuf.length());
                return HTTPRESPONSECODE_200_OK;
            }

            void SetHostname(const string &name) {
                m_sHostname = name;
            }

            void SetDocumentRoot(const string &name) {
                m_sDocumentRoot = name;
            }

        private:
            string m_sHostname;
            string m_sDocumentRoot;
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
                                log::err << "bind: " << strerror(WEXITSTATUS(status)) << endl;
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

    typedef boost::shared_ptr<wspp::wsendpoint> conn_ptr;
    typedef boost::shared_ptr<wspp::wshandler> handler_ptr;
    typedef std::map<EHSConnection *, conn_ptr> conn_map;
    typedef std::map<EHSConnection *, handler_ptr> handler_map;

    class MyWsHandler : public wspp::wshandler
    {
        public:
            MyWsHandler(EHSConnection *econn, EHS *ehs)
                : m_econn(econn)
                  , m_ehs(ehs)
        {}

            virtual void on_message(std::string, std::string data) {
                log::debug << "GOT Message: '" << data << "'" << endl;
                data.insert(0, "Yes, ");
                send_text(data);
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
                log::debug << "Send WS response '" << data << "'" << endl;
                ehs_autoptr<GenericResponse> r(new GenericResponse(0, m_econn));
                r->SetBody(data.data(), data.length());
                m_ehs->AddResponse(ehs_move(r));
            }

        private:
            MyWsHandler(const MyWsHandler&);
            MyWsHandler& operator=(const MyWsHandler&);
            EHSConnection *m_econn;
            EHS *m_ehs;
    };

    class MyRawSocketHandler : public RawSocketHandler
    {
        public:
            MyRawSocketHandler(WsGate *parent)
                : m_parent(parent)
                  , m_cmap(conn_map())
                  , m_hmap(handler_map())
        { }

            virtual bool OnData(EHSConnection *conn, std::string data)
            {
                if (m_cmap.end() != m_cmap.find(conn)) {
                    m_cmap[conn]->AddRxData(data);
                    return true;
                }
                return false;
            }

            virtual void OnConnect(EHSConnection *conn)
            {
                log::debug << "GOT WS CONNECT" << endl;
                handler_ptr h(new MyWsHandler(conn, m_parent));
                conn_ptr c(new wspp::wsendpoint(h.get()));
                m_hmap[conn] = h;
                m_cmap[conn] = c;
            }

            virtual void OnDisconnect(EHSConnection *conn)
            {
                log::debug << "GOT WS DISCONNECT" << endl;
                conn_map::iterator ic = m_cmap.find(conn);
                if (m_cmap.end() != ic) {
                    m_cmap.erase(ic);
                }
                handler_map::iterator ih = m_hmap.find(conn);
                if (m_hmap.end() != ih) {
                    m_hmap.erase(ih);
                }
            }

        private:
            MyRawSocketHandler(const MyRawSocketHandler&);
            MyRawSocketHandler& operator=(const MyRawSocketHandler&);

            WsGate *m_parent;
            conn_map m_cmap;
            handler_map m_hmap;

    };

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

    wsgate::WsGate srv;

    string hostname(vm["global.hostname"].as<string>());
    int port = -1;
    bool need2 = false;
    bool https = false;
    if (vm.count("global.port")) {
        port = vm["global.port"].as<uint16_t>();
        if (vm.count("ssl.port")) {
            need2 = true;
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
    if (vm.count("threading.mode")) {
        string mode(vm["threading.mode"].as<string>());
        if (0 == mode.compare("single")) {
            oSP["mode"] = "singlethreaded";
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

        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            srv.HandleData(1000);
        }

        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }

    return 0;
}
