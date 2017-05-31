#ifndef _WSGATE_EHS_
#define _WSGATE_EHS_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ehs/ehs.h>
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
#include <boost/optional/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>

#include "common.hpp"
#include "btexception.hpp"
#include "base64.hpp"
#include "sha1.hpp"
#include "logging.hpp"
#include "wsendpoint.hpp"
#include "myrawsocket.hpp"
#include "nova_token_auth.hpp"

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

namespace wsgate{
    // subclass of EHS that defines a custom HTTP response.
    class WsGate : public EHS
    {
        public:
            WsGate(EHS *parent = NULL, std::string registerpath = "");
            virtual ~WsGate();
            HttpResponse *HandleThreadException(ehs_threadid_t, HttpRequest *request, exception &ex);
            void CheckForPredefined(string& rdpHost, string& rdpUser, string& rdpPass);
            bool ConnectionIsAllowed(string rdphost);
            void LogInfo(std::basic_string<char> remoteAdress, string uri, const char response[]);
            ResponseCode HandleRobotsRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost);
            ResponseCode HandleCursorRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost);
            ResponseCode HandleRedirectRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost);
            int CheckIfWSocketRequest(HttpRequest *request, HttpResponse *response, string uri, string thisHost);
            ResponseCode HandleWsgateRequest(HttpRequest *request, HttpResponse *response, std::string uri, std::string thisHost);
            ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response);
            ResponseCode HandleHTTPRequest(HttpRequest *request, HttpResponse *response, bool tokenAuth = false);
            boost::property_tree::ptree GetConfig();
            const string & GetConfigFile();
            bool GetEnableCore();
            bool SetConfigFile(const string &name);
            bool ReadConfig(wsgate::log *logger = NULL);
            bool GetDaemon();
            void SetPidFile(const string &name);
            void RegisterRdpSession(rdp_ptr rdp);
            void UnregisterRdpSession(rdp_ptr rdp);
            WsRdpOverrideParams getOverrideParams();
        private:
            typedef enum {
                TEXT,
                HTML,
                PNG,
                ICO,
                JAVASCRIPT,
                JSON,
                CSS,
                OGG,
                CUR,
                BINARY
            } MimeType;
            typedef map<string, rdp_ptr> SessionMap;
            typedef boost::tuple<time_t, string> cache_entry;
            typedef map<path, cache_entry> StaticCache;

            string m_sHostname;
            string m_sDocumentRoot;
            string m_sPidFile;
            bool m_bDebug;
            bool m_bEnableCore;
            SessionMap m_SessionMap;
            vector<boost::regex> m_allowedHosts;
            vector<boost::regex> m_deniedHosts;
            WsRdpOverrideParams overrideParams;
            bool m_bOrderDenyAllow;
            string m_sConfigFile;
            boost::property_tree::ptree m_ptIniConfig;
            bool m_bDaemon;
            bool m_bRedirect;
            StaticCache m_StaticCache;
            string m_sOpenStackAuthUrl;
            string m_sOpenStackUsername;
            string m_sOpenStackPassword;
            string m_sOpenStackProjectName;
            string m_sOpenStackProjectId;
            string m_sOpenStackProjectDomainName;
            string m_sOpenStackUserDomainName;
            string m_sOpenStackProjectDomainId;
            string m_sOpenStackUserDomainId;
            string m_sOpenStackKeystoneVersion;
            string m_sOpenStackRegion;
            string m_sHyperVHostUsername;
            string m_sHyperVHostPassword;

            // Non-copyable
            WsGate(const WsGate&);
            WsGate & operator=(const WsGate&);

            MimeType simpleMime(const string & filename);
            template <typename T> std::vector<T> as_vector(boost::property_tree::ptree const& pt, boost::property_tree::ptree::key_type const& key);
            bool notModified(HttpRequest *request, HttpResponse *response, time_t mtime);
            int str2bint(const string &s);
            bool str2bool(const string &s);
            void wc2pat(string &wildcards);
            void setHostList(const vector<string> &hosts, vector<boost::regex> &hostlist);
            void setAclOrder(const string &order);
    };
}

#endif