#include "../pluginTemplate/pluginCommon.h"
#include <sstream>
#include <exception>

bool entryPoint(std::map<std::string, std::string> formValues, std::map<std::string, std::string> & result){
    bool returnValue = false;
    std::string debugStr;
    std::string errStr;
    std::stringstream debug(debugStr);
    std::stringstream err(errStr);

    if (formValues.count("wsgate") && formValues.count("token"))
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
            result["rdpport"] = info.port;
            result["rdppcb"] = info.internal_access_path;

            result["rdpuser"] = m_sHyperVHostUsername;
            result["rdppass"] = m_sHyperVHostPassword;

            returnValue = true;
        }
        catch (std::exception& ex)
        {
            err << "OpenStack token authentication failed: " << ex.what() << std::endl;
            returnValue = false;
        }
    }
    else{
        returnValue = false;
    }
    result["err"] = result["err"] + errStr;
    result["debug"] = result["debug"] + debugStr;
    return returnValue;
}