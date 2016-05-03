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
std::string m_sHyperVHostUsername;
std::string m_sHyperVHostPassword;

bool readConfigFile(std::map<std::string, std::string> & result){
    std::string errStr;
    std::stringstream err(errStr);
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
                if (pt.get_optional<std::string>("hyperv.hostusername")) {
                    m_sHyperVHostUsername.assign(pt.get<std::string>("hyperv.hostusername"));
                }
                else {
                    m_sHyperVHostUsername.clear();
                }
                if (pt.get_optional<std::string>("hyperv.hostpassword")) {
                    m_sHyperVHostPassword.assign(pt.get<std::string>("hyperv.hostpassword"));
                }
                else {
                    m_sHyperVHostPassword.clear();
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
    result["err"] = result["err"] + errStr;
    return returnValue;
}

bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result){
    bool returnValue = false;
    std::string debugStr;
    std::string errStr;
    std::stringstream debug(debugStr);
    std::stringstream err(errStr);

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
                    tokenId);

                debug << "Host: " << info.host << " Port: " << info.port
                    << " Internal access path: " << info.internal_access_path
                    << std::endl;

                result["rdphost"] = info.host;
                result["rdpport"] = boost::lexical_cast<std::string>(info.port);
                result["rdppcb"] = info.internal_access_path;

                result["rdpuser"] = m_sHyperVHostUsername;
                result["rdppass"] = m_sHyperVHostPassword;

                returnValue = true;

                std::cout << formValues["token"] << std::endl;
                std::cout << formValues["title"] << std::endl;
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
    result["err"] = result["err"] + errStr;
    result["debug"] = result["debug"] + debugStr;
    return returnValue;
}