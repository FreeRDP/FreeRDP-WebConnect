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

using namespace pplx;
using namespace std;
using namespace web;
using namespace wsgate;


class nova_console_token_auth_impl : public nova_console_token_auth
{
private:
    web::json::value execute_request_and_get_json_value(
        web::http::client::http_client& client,
        web::http::http_request& request);

    web::json::value get_auth_token_data(std::string osAuthUrl,
                                         std::string osUserName,
                                         std::string osPassword,
                                         std::string osTenantName);

    web::json::value get_console_token_data(std::string authToken,
                                            std::string novaUrl,
                                            std::string consoleToken);

public:
    virtual nova_console_info get_console_info(std::string osAuthUrl,
                                               std::string osUserName,
                                               std::string osPassword,
                                               std::string osTenantName,
                                               std::string consoleToken);
};



json::value nova_console_token_auth_impl::execute_request_and_get_json_value(
    http::client::http_client& client,
    http::http_request& request)
{
    auto response_task = client.request(request);
    response_task.wait();
    auto response = response_task.get();

    if(response.status_code() >= 400)
    {
        throw http_exception(response.status_code(), response.reason_phrase());
    }

    auto json_task = response.extract_json();
    json_task.wait();

    return json_task.get();
}

json::value nova_console_token_auth_impl::get_auth_token_data(
    string osAuthUrl, string osUserName,
    string osPassword, string osTenantName)
{
    auto jsonRequestBody = json::value::object();
    auto auth = json::value::object();
    auto cred = json::value::object();
    cred[U("username")] = json::value::string(osUserName);
    cred[U("password")] = json::value::string(osPassword);
    auth[U("passwordCredentials")] = cred;
    auth[U("tenantName")] = json::value::string(osTenantName);
    jsonRequestBody[U("auth")] = auth;

    http::http_request request(http::methods::POST);
    request.set_request_uri(U("tokens"));
    request.headers().add(http::header_names::accept, U("application/json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonRequestBody);

    http::client::http_client client(osAuthUrl);

    return execute_request_and_get_json_value(client, request);
}

json::value nova_console_token_auth_impl::get_console_token_data(
    string authToken, string novaUrl, string consoleToken)
{
    http::client::http_client client(novaUrl);

    auto jsonRequestBody = json::value::object();
    jsonRequestBody[U("os-getConsoleConnectInfo")] = json::value::null();

    http::uri_builder console_token_uri;
    console_token_uri.append(U("console-auth-tokens"));
    console_token_uri.append(consoleToken);
    console_token_uri.append(U("action"));

    http::http_request request(http::methods::POST);
    request.set_request_uri(console_token_uri.to_string());
    request.headers().add(U("X-Auth-Token"), authToken);
    request.headers().add(http::header_names::accept, U("application/json"));
    request.headers().set_content_type(U("application/json"));
    request.set_body(jsonRequestBody);

    return execute_request_and_get_json_value(client, request);
}

nova_console_info nova_console_token_auth_impl::get_console_info(
    std::string osAuthUrl, std::string osUserName,
    std::string osPassword, std::string osTenantName,
    std::string consoleToken)
{
    auto token_data = get_auth_token_data(osAuthUrl, osUserName,
                                          osPassword, osTenantName);

    auto novaUrl = token_data[U("access")][U("serviceCatalog")][0]
                             [U("endpoints")][0]
                             [U("adminURL")].as_string();

    auto authToken = token_data[U("access")][U("token")]
                               [U("id")].as_string();

    auto consoleTokenData = get_console_token_data(authToken, novaUrl,
                                                   consoleToken);

    nova_console_info info;

    info.host = consoleTokenData[U("console")][U("host")].as_string();

    auto portValue = consoleTokenData[U("console")][U("port")];
    if(portValue.is_string())
    {
        istringstream(portValue.as_string()) >> info.port;
    }
    else
    {
        info.port = portValue.as_integer();
    }

    auto internalAccessPathValue = consoleTokenData[U("console")]
        [U("internal_access_path")];

    if(internalAccessPathValue.is_string())
    {
        info.internal_access_path = internalAccessPathValue.as_string();
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
