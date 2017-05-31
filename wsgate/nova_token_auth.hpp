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

#ifndef _NOVA_TOKEN_AUTH_
#define _NOVA_TOKEN_AUTH_

#include <exception>
#include <string>

#define KEYSTONE_V2 "v2.0"
#define KEYSTONE_V3 "v3"

namespace wsgate {

    class http_exception: public std::exception {
    private:
        unsigned status_code;
        std::string message;
    public:
        http_exception(unsigned status_code, const std::string& message) :
            status_code(status_code), message(message)
        {
        }

        unsigned get_status_code()
        {
            return status_code;
        }

        virtual const char* what() const throw()
        {
            return message.c_str();
        }
    };


    class nova_console_info
    {
    public:
        std::string host;
        unsigned port;
        std::string internal_access_path;

        nova_console_info() :
            port(0)
        {
        }
    };


    class nova_console_token_auth
    {
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
                                                   std::string osRegion) = 0;
    };


    class nova_console_token_auth_factory
    {
    private:
        static nova_console_token_auth* instance;
    public:
        static nova_console_token_auth* get_instance();
    };
};

#endif
