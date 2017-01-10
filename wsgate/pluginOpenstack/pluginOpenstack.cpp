#include "../pluginTemplate/pluginCommon.hpp"
#include "nova_token_auth.hpp"
#include <sstream>
#include <exception>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/lexical_cast.hpp>

using namespace wsgate;

std::string m_sOpenStackAuthUrl;
std::string m_sOpenStackUsername;
std::string m_sOpenStackPassword;
std::string m_sOpenStackTenantName;
std::string m_sOpenStackKeystoneVersion;
std::string m_sHyperVHostUsername;
std::string m_sHyperVHostPassword;

bool readConfigFile(std::map<std::string, std::string> & result){
    std::stringstream err;
    bool returnValue = false;

    if (result.count("configfile")){
        try {
            boost::property_tree::ptree pt;
            boost::property_tree::ini_parser::read_ini(result["configfile"], pt);

            try {
                if (pt.get_optional<std::string>("openstack.authurl")) {
                    m_sOpenStackAuthUrl.assign(pt.get<std::string>("openstack.authurl"));
                }
                else {
                    m_sOpenStackAuthUrl.clear();
                }
                if (pt.get_optional<std::string>("openstack.username")) {
                    m_sOpenStackUsername.assign(pt.get<std::string>("openstack.username"));
                }
                else {
                    m_sOpenStackUsername.clear();
                }
                if (pt.get_optional<std::string>("openstack.password")) {
                    m_sOpenStackPassword.assign(pt.get<std::string>("openstack.password"));
                }
                else {
                    m_sOpenStackPassword.clear();
                }
                if (pt.get_optional<std::string>("openstack.tenantname")) {
                    m_sOpenStackTenantName.assign(pt.get<std::string>("openstack.tenantname"));
                }
                else {
                    m_sOpenStackTenantName.clear();
                }
                if (pt.get_optional<std::string>("openstack.keystoneversion")) {
                    m_sOpenStackKeystoneVersion.assign(pt.get<std::string>("openstack.keystoneversion"));
                }
                else {
                    m_sOpenStackKeystoneVersion = KEYSTONE_V2;
                }
                returnValue = true;
            }
            catch (const std::exception & e) {
                err << e.what() << std::endl;
                err << e.what() << std::endl;
            }

        }
        catch (const boost::property_tree::ini_parser_error &e) {
            err << e.what() << std::endl;
        }
    }
    else{
        err << "Plugin got no config file path" << std::endl;
    }
    result["err"] = result["err"] + err.str();
    return returnValue;
}

bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result){
    bool returnValue = false;
    std::stringstream debug;
    std::stringstream err;

    if (readConfigFile(result)){
        if (formValues.count("token") && formValues.count("title"))
        {
            // OpenStack console authentication
            try
            {
                debug << "Starting OpenStack token authentication" << std::endl;

                std::string tokenId = formValues["token"];

                nova_console_token_auth* token_auth = nova_console_token_auth_factory::get_instance();

                nova_console_info info = token_auth->get_console_info(m_sOpenStackAuthUrl, m_sOpenStackUsername,
                    m_sOpenStackPassword, m_sOpenStackTenantName,
                    tokenId, m_sOpenStackKeystoneVersion);

                debug << "Host: " << info.host << " Port: " << info.port
                    << " Internal access path: " << info.internal_access_path
                    << std::endl;

                result["rdphost"] = info.host;
                result["rdpport"] = boost::lexical_cast<std::string>(info.port);
                result["rdppcb"] = info.internal_access_path;

                result["rdpuser"] = m_sHyperVHostUsername;
                result["rdppass"] = m_sHyperVHostPassword;

                returnValue = true;
            }
            catch (std::exception& ex)
            {
                err << "OpenStack token authentication failed: " << ex.what() << std::endl;
            }
        }
    }
    else{
        err << "Error parsing config file." << std::endl;
    }

    if (!err.str().empty())
        result["err"] = err.str();

    if (!debug.str().empty())
        result["debug"] = debug.str();
    return returnValue;
}