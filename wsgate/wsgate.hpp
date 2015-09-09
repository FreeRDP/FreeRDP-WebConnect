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
#ifndef _WSGATE_H_
#define _WSGATE_H_

#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <ehs/ehs.h>
#include <boost/lexical_cast.hpp>
#include "logging.hpp"

using namespace std;

namespace wsgate {

    /// Our global logging instance
    typedef wsgate::logger log;

    static const char * const ws_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    int nFormValue(HttpRequest *request, const string & name, int defval);
    void SplitUserDomain(const string& fullUsername, string& username, string& domain);
}

#endif
