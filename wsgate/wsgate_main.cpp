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
#include "wsGateService.hpp"
#include "myBindHelper.hpp"

namespace wsgate{

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

    void SplitUserDomain(const string& fullUsername, string& username, string& domain)
    {
        std::vector<string> strs;
        boost::split(strs, fullUsername, boost::is_any_of("\\"));
        if (strs.size() > 1)
        {
            username = strs[1];
            domain = strs[0];
        }
        else
        {
            strs.clear();
            boost::split(strs, fullUsername, boost::is_any_of("@"));
            if (strs.size() > 1)
            {
                username = strs[0];
                domain = strs[1];
            }
            else
            {
                username = fullUsername;
                domain = "";
            }
        }
    }
}

#ifdef _WIN32
static bool g_service_background = true;
#endif

static void terminate(int)
{
#ifdef _WIN32
    wsgate::WsGateService::g_signaled = true;
#endif
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
int _service_main (int argc, char **argv)
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
#ifdef _WIN32
            while (!(srv.ShouldTerminate() || (psrv && psrv->ShouldTerminate()) || wsgate::WsGateService::g_signaled)) {
#else
            while (!(srv.ShouldTerminate() || (psrv && psrv->ShouldTerminate()))) {
#endif
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
#ifdef _WIN32
            while (!(srv.ShouldTerminate() || (psrv && psrv->ShouldTerminate()) || wsgate::WsGateService::g_signaled || kbd.qpressed()))
#else
            while (!(srv.ShouldTerminate() || (psrv && psrv->ShouldTerminate()) || kbd.qpressed()))
#endif
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
