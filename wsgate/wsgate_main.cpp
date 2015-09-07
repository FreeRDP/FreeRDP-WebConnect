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

#include "wsgate.hpp"
#include "wsgateEHS.hpp"
#include "myWsHandler.hpp"

namespace wsgate {
    

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

    bool MyRawSocketHandler::Prepare(EHSConnection *conn, const string host, const string pcb,
            const string user, const string pass, const WsRdpParams &params, EmbeddedContext embeddedContext)
    {
        try
        {
            handler_ptr h(new MyWsHandler(conn, m_parent, this));
            conn_ptr c(new wspp::wsendpoint(h.get()));
            rdp_ptr r(new RDP(h.get(), this));
            m_cmap[conn] = conn_tuple(c, h, r);

            r->setEmbeddedContext(embeddedContext);

            this->conn = conn;
            if (embeddedContext == CONTEXT_EMBEDDED){
                PrepareRDP(host, pcb, user, pass, params);
            }
        }
        catch(...)
        {
            log::info << "Attemtped double connection to the same machine" << endl;
            return false;
        }
        return true;
    }

    void MyRawSocketHandler::PrepareRDP(const std::string host, const std::string pcb, const std::string user, const std::string pass, const WsRdpParams &params){
        string username;
        string domain;
        SplitUserDomain(user, username, domain);

        rdp_ptr r = this->m_cmap[this->conn].get<2>();
        r->Connect(host, pcb, username, domain, pass, params);
        m_parent->RegisterRdpSession(r);

        log::debug << "RDP Host:              '" << host << "'" << endl;
        log::debug << "RDP Pcb:               '" << pcb << "'" << endl;
        log::debug << "RDP User:              '" << user << "'" << endl;
        log::info << "RDP Port:               '" << params.port << "'" << endl;
        log::debug << "RDP Desktop size:       " << params.width << "x" << params.height << endl;
        log::debug << "RDP Performance:        " << params.perf << endl;
        log::debug << "RDP No wallpaper:       " << params.nowallp << endl;
        log::debug << "RDP No full windowdrag: " << params.nowdrag << endl;
        log::debug << "RDP No menu animation:  " << params.nomani << endl;
        log::debug << "RDP No theming:         " << params.nomani << endl;
        log::debug << "RDP Disable TLS:        " << params.notls << endl;
        log::debug << "RDP Disable NLA:        " << params.nonla << endl;
        log::debug << "RDP NTLM auth:          " << params.fntlm << endl;
    }

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

#ifndef _WIN32
static wsgate::WsGate *g_srv = NULL;
static wsgate::WsGate *g_psrv = NULL;
static void reload(int)
{
    wsgate::log::info << "Got SIGHUP, reloading config file." << endl;
    if (NULL != g_srv) {
        g_srv->ReadConfig();
    }
    if (NULL != g_psrv) {
        g_psrv->ReadConfig();
    }
    signal(SIGHUP, reload);
}
#endif

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
        ("config,c", po::value<string>()->default_value(DEFAULTCFG), "Specify config file");

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
    wsgate::WsGate srv;
    if (vm.count("config")) {
        if (!srv.SetConfigFile(vm["config"].as<string>())) {
            return -1;
        }
    } else {
#ifdef _WIN32
        wsgate::log::err << "Mandatory option --config <filename> is missing." << endl;
#endif
        cerr << "Mandatory option --config <filename> is missing." << endl;
        return -1;
    }


    if (!srv.ReadConfig(&log)) {
        return -1;
    }

    boost::property_tree::ptree pt = srv.GetConfig();

    int port = -1;
    bool https = false;
    bool need2 = false;
    if (pt.get_optional<uint16_t>("ssl.port")) {
        port = pt.get<uint16_t>("ssl.port");
        https = true;
        if (pt.get_optional<uint16_t>("global.port")) {
            need2 = true;
        }
    } else if (pt.get_optional<uint16_t>("global.port")) {
        port = pt.get<uint16_t>("global.port");
    }

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
        if (pt.get_optional<std::string>("ssl.bindaddr")) {
            oSP["bindaddress"] = pt.get<std::string>("ssl.bindaddr");
        }
        oSP["https"] = 1;
        if (pt.get_optional<std::string>("ssl.certfile")) {
            oSP["certificate"] = pt.get<std::string>("ssl.certfile");
        }
        if (pt.get_optional<std::string>("ssl.certpass")) {
            oSP["passphrase"] = pt.get<std::string>("ssl.certpass");
        }
    } else {
        if (pt.get_optional<std::string>("global.bindaddr")) {
            oSP["bindaddress"] = pt.get<std::string>("global.bindaddr");
        }
    }
    if (pt.get_optional<unsigned long>("http.maxrequestsize")) {
        oSP["maxrequestsize"] = pt.get<unsigned long>("http.maxrequestsize");
    }
    bool sleepInLoop = true;
    if (pt.get_optional<std::string>("threading.mode")) {
        string mode(pt.get<std::string>("threading.mode"));
        if (0 == mode.compare("single")) {
            oSP["mode"] = "singlethreaded";
            sleepInLoop = false;
        } else if (0 == mode.compare("pool")) {
            oSP["mode"] = "threadpool";
            if (pt.get_optional<int>("threading.poolsize")) {
                oSP["threadcount"] = pt.get<int>("threading.poolsize");
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
    if (pt.get_optional<std::string>("ssl.certfile")) {
        oSP["certificate"] = pt.get<std::string>("ssl.certfile");
    }
    if (pt.get_optional<std::string>("ssl.certpass")) {
        oSP["passphrase"] = pt.get<std::string>("ssl.certpass");
    }

#ifdef _WIN32
    bool daemon = (pt.get_child_optional("foreground")) ? false : g_service_background;
#else
    bool daemon = false;
    if (pt.get_child_optional("global.daemon") && !pt.get_child_optional("foreground")) {
        daemon = srv.GetDaemon();
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
                        if (pt.get_child_optional("global.pidfile")) {
                            const string pidfn(pt.get<std::string>("global.pidfile"));
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
    g_srv = &srv;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, reload);
    if (srv.GetEnableCore()) {
        struct rlimit rlim;
        rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
        if (-1 == setrlimit(RLIMIT_CORE, &rlim)) {
            cerr << "Could not raise core dump limit: " << strerror(errno) << endl;
        }
    }
#endif
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);

    wsgate::WsGate *psrv = NULL;
    try {
        wsgate::log::info << "wsgate v" << VERSION << "." << GITREV << " starting" << endl;
        srv.StartServer(oSP);
        wsgate::log::info << "Listening on " << oSP["bindaddress"].GetCharString() << ":" << oSP["port"].GetInt() << endl;

        if (need2) {
            // Add second instance on insecure port
            psrv = new wsgate::WsGate();
#ifndef _WIN32
            psrv->SetBindHelper(&h);
#endif
            psrv->SetConfigFile(srv.GetConfigFile());
            psrv->ReadConfig();
            oSP["https"] = 0;
            oSP["port"] = pt.get<uint16_t>("global.port");
            if (pt.get_child_optional("global.bindaddr")) {
                oSP["bindaddress"] = pt.get<std::string>("global.bindaddr");
            }
            psrv->SetSourceEHS(srv);
            wsgate::MyRawSocketHandler *psh = new wsgate::MyRawSocketHandler(psrv);
            psrv->SetRawSocketHandler(psh);
            psrv->StartServer(oSP);
#ifndef _WIN32
            g_psrv = psrv;
#endif
            wsgate::log::info << "Listening on " << oSP["bindaddress"].GetCharString() << ":" << oSP["port"].GetInt() << endl;
        }

        if (daemon) {
            while (!(srv.ShouldTerminate() || (psrv && psrv->ShouldTerminate()) || g_signaled)) {
                if (sleepInLoop) {
                    usleep(50000);
                } else {
                    srv.HandleData(1000);
                    if (NULL != psrv) {
                        psrv->HandleData(1000);
                    }
                }
            }
        } else {
            wsgate::kbdio kbd;
            cout << "Press q to terminate ..." << endl;
            while (!(srv.ShouldTerminate()  || (psrv && psrv->ShouldTerminate()) || g_signaled || kbd.qpressed()))
            	{
                if (sleepInLoop)
					{
						usleep(1000);
					}
                else
					{
						srv.HandleData(1000);
						if (NULL != psrv)
							{
								psrv->HandleData(1000);
							}
					}
            }
        }
        wsgate::log::info << "terminating" << endl;
        srv.StopServer();
        if (NULL != psrv) {
            psrv->StopServer();
        }
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
        wsgate::log::err << e.what() << endl;
    }

    delete psrv;
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
