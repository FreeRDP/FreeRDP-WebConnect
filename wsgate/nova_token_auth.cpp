/* vim: set et ts=4 sw=4 cindent:
 *
 * Copyright 2013 Cloudbase Solutions Srl
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

#include <sstream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include "nova_token_auth.hpp"
#include <iostream>

using namespace pplx;
using namespace std;
using namespace web;
using namespace wsgate;
using namespace utility::conversions;

class nova_console_token_auth_impl : public nova_console_token_auth
{
private:
    web::http::http_response execute_request_and_get_response(
        web::http::client::http_client& client,
        web::http::http_request& request);

    web::json::value get_json_from_response(web::http::http_response response);

    std::pair<std::string, std::string> get_auth_token_data_v2(std::string osAuthUrl,
                                                                           std::string osUserName,
                                                                           std::string osPassword,
                                                                           std::string osProjectName,
                                                                           std::string osRegion);

    std::pair<std::string, std::string> get_auth_token_data_v3(std::string osAuthUrl,
                                                                           std::string osUserName,
                                                                           std::string osPassword,
                                                                           std::string osProjectName,
                                                                           std::string osProjectId,
                                                                           std::string osProjectDomainName,
                                                                           std::string osUserDomainName,
                                                                           std::string osProjectDomainId,
                                                                           std::string osUserDomainId,
                                                                           std::string osRegion);

    web::json::value get_console_token_data(std::string authToken,
                                            std::string novaUrl,
                                            std::string consoleToken);

public:
    virtual nova_console_info get_console_info(std::string osAuthUrl,
                                               std::string osUserName,
                                               std::string osPassword,
                                               std::string osProjectName,
                                               std::string osProjectId,
                                               std::string osProjectDomainName,
                                               std::string osUserDomainName,
                                               std::string osProjectDomainId,
                                               std::string osUserDomainId,
                                               std::string consoleToken,
                                               std::string keystoneVersion,
                                               std::string osRegion);
};



http::http_response nova_console_token_auth_impl::execute_request_and_get_response(
    http::client::http_client& client,
    http::http_request& request)
{
    auto response_task = client.request(request);
    response_task.wait();
    auto response = response_task.get();

    if(response.status_code() >= 400)
    {
        throw http_exception(response.status_code(), to_utf8string(response.reason_phrase()));
    }

    return response;
}

json::value nova_console_token_auth_impl::get_json_from_response(
    http::http_response response)
{
    auto json_task = response.extract_json();
    json_task.wait();

    return json_task.get();
}

std::pair<std::string, std::string> nova_console_token_auth_impl::get_auth_token_data_v2(
    string osAuthUrl, string osUserName,
    string osPassword, string osProjectName,
    string osRegion)
{
    auto jsonRequestBody = json::value::object();
    auto auth = json::value::object();
    auto cred = json::value::object();
    cred[U("username")] = json::value::string(to_string_t(osUserName));
    cred[U("password")] = json::value::string(to_string_t(osPassword));
    auth[U("passwordCredentials")] = cred;
    auth[U("tenantName")] = json::value::string(to_string_t(osProjectName));
    jsonRequestBody[U("auth")] = auth;
    http::http_request request(http::methods::POST);
    request.set_request_uri(U("tokens"));
    request.headers().add(http::header_names::accept, U("application/json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonRequestBody);

    http::client::http_client client(to_string_t(osAuthUrl));
    auto response_json = get_json_from_response(execute_request_and_get_response(client, request));

    utility::string_t authToken;
    utility::string_t novaUrl;
    //get the authentication token
    authToken = response_json[U("access")][U("token")][U("id")].as_string();

    //get the nova api endpoint
    for (auto serviceCatalog : response_json[U("access")][U("serviceCatalog")].as_array())
        if (serviceCatalog[U("name")].as_string() == U("nova")){
            if (osRegion.empty()){
                novaUrl = serviceCatalog[U("endpoints")][0][U("adminURL")].as_string();
            }
            else{
                for (auto endpoint : serviceCatalog[U("endpoints")].as_array()){
                    if (endpoint[U("region")].as_string() == to_string_t(osRegion)){
                        novaUrl = endpoint[U("adminURL")].as_string();
                    }
                }
            }
        }
            

    return std::pair<std::string, std::string>(
        to_utf8string(authToken),
        to_utf8string(novaUrl));
}

std::pair<std::string, std::string> nova_console_token_auth_impl::get_auth_token_data_v3(
    string osAuthUrl, string osUserName,
    string osPassword, string osProjectName,
    string osProjectId,
    string osProjectDomainName, string osUserDomainName,
    string osProjectDomainId, string osUserDomainId,
    string osRegion)
{
    auto jsonRequestBody = json::value::object();
    auto auth = json::value::object();
    auto identity = json::value::object();
    auto methods = json::value::array();
    methods[0] = json::value::string(to_string_t("password"));

    auto password = json::value::object();
    auto user = json::value::object();
    user[U("name")] = json::value::string(to_string_t(osUserName));
    user[U("password")] = json::value::string(to_string_t(osPassword));

    auto userDomain = json::value::object();
    if (!osUserDomainId.empty()){
        userDomain[U("id")] = json::value::string(to_string_t(osUserDomainId));
    }
    else{
        userDomain[U("name")] = json::value::string(to_string_t(osUserDomainName));
    }
    user[U("domain")] = userDomain;
    password[U("user")] = user;
    identity[U("methods")] = methods;
    identity[U("password")] = password;

    auto scope = json::value::object();
    auto project = json::value::object();
    if (!osProjectId.empty())
    {
        project[U("id")] = json::value::string(to_string_t(osProjectId));
    }
    else if (!osProjectName.empty())
    {
        project[U("name")] = json::value::string(to_string_t(osProjectName));
    }

    auto projectDomain = json::value::object();
    if (!osProjectDomainId.empty()){
        projectDomain[U("id")] = json::value::string(to_string_t(osProjectDomainId));
    }
    else{
        projectDomain[U("name")] = json::value::string(to_string_t(osProjectDomainName));
    }
    project[U("domain")] = projectDomain;
    scope[U("project")] = project;

    auth[U("identity")] = identity;
    auth[U("scope")] = scope;
    jsonRequestBody[U("auth")] = auth;

    http::http_request request(http::methods::POST);
    request.set_request_uri(U("auth/tokens"));
    request.headers().add(http::header_names::accept, U("application/json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonRequestBody);

    http::client::http_client client(to_string_t(osAuthUrl));
    auto response = execute_request_and_get_response(client, request);
    auto response_json = get_json_from_response(response);

    utility::string_t authToken;
    utility::string_t novaUrl;
    //get the authentication token
    authToken = response.headers()[U("X-Subject-Token")];

    //get the nova api endpoint
    for (auto serviceCatalog : response_json[U("token")][U("catalog")].as_array())
        if (serviceCatalog[U("name")].as_string() == U("nova"))
            for (auto endpoint : serviceCatalog[U("endpoints")].as_array())
                if (endpoint[U("interface")].as_string() == U("admin") &&
                    (endpoint[U("region")].as_string() == to_string_t(osRegion) || osRegion.empty())){
                        novaUrl = endpoint[U("url")].as_string();
                }

    return std::pair<std::string, std::string>(
        to_utf8string(authToken),
        to_utf8string(novaUrl));
}

json::value nova_console_token_auth_impl::get_console_token_data(
    string authToken, string novaUrl, string consoleToken)
{
    http::client::http_client client(to_string_t(novaUrl));
    http::uri_builder console_token_uri;
    console_token_uri.append(U("os-console-auth-tokens"));
    console_token_uri.append(to_string_t(consoleToken));

    http::http_request request(http::methods::GET);
    request.set_request_uri(console_token_uri.to_string());
    request.headers().add(U("X-Auth-Token"), to_string_t(authToken));
    request.headers().add(http::header_names::accept, U("application/json"));
    request.headers().set_content_type(U("application/json"));

    return get_json_from_response(execute_request_and_get_response(client, request));
}

nova_console_info nova_console_token_auth_impl::get_console_info(
    std::string osAuthUrl, std::string osUserName,
    std::string osPassword,
    std::string osProjectName, std::string osProjectId,
    std::string osProjectDomainName, std::string osUserDomainName,
    std::string osProjectDomainId, std::string osUserDomainId,
    std::string consoleToken, std::string keystoneVersion,
    std::string osRegion)
{
    std::string authToken;
    std::string novaUrl;

    if (keystoneVersion == KEYSTONE_V2){
        std::tie(authToken, novaUrl) = get_auth_token_data_v2(osAuthUrl,
                                                              osUserName,
                                                              osPassword,
                                                              osProjectName,
                                                              osRegion);
    }
    else if (keystoneVersion == KEYSTONE_V3){
        std::tie(authToken, novaUrl) = get_auth_token_data_v3(osAuthUrl,
                                                              osUserName,
                                                              osPassword,
                                                              osProjectName,
                                                              osProjectId,
                                                              osProjectDomainName,
                                                              osUserDomainName,
                                                              osProjectDomainId,
                                                              osUserDomainId,
                                                              osRegion);
    }
    else{
        throw std::invalid_argument("Unknown Keystone version");
    }

    nova_console_info info;

    auto consoleTokenData = get_console_token_data(
        authToken,
        novaUrl,
        consoleToken);

    info.host = to_utf8string(consoleTokenData[U("console")][U("host")].as_string());

    auto portValue = consoleTokenData[U("console")][U("port")];
    if(portValue.is_string())
    {
        istringstream(to_utf8string(portValue.as_string())) >> info.port;
    }
    else
    {
        info.port = portValue.as_integer();
    }

    auto internalAccessPathValue = consoleTokenData[U("console")]
        [U("internal_access_path")];

    if(internalAccessPathValue.is_string())
    {
        info.internal_access_path = to_utf8string(internalAccessPathValue.as_string());
    }

    return info;
}


nova_console_token_auth* nova_console_token_auth_factory::instance = NULL;


nova_console_token_auth* nova_console_token_auth_factory::get_instance()
{
    if(!instance)
        instance = new nova_console_token_auth_impl();

    return instance;
}
