/* $Id: ehs_exception.cpp 81 2012-03-20 12:03:05Z felfert $
 *
 * EHS is a library for embedding HTTP(S) support into a C++ application
 *
 * Copyright (C) 2004 Zachary J. Hansen
 *
 * Code cleanup, new features and bugfixes: Copyright (C) 2010 Fritz Elfert
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1 as published by the Free Software Foundation;
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    This can be found in the 'COPYING' file.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ehs.h>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <typeinfo>
#include <cstdlib>
#include "btexception.h"
#include "common.h"

using namespace std;

// subclass of EHS that defines a custom HTTP response.
class TestException : public EHS
{
    HttpResponse *HandleThreadException(ehs_threadid_t, HttpRequest *request, exception &ex)
    {
        HttpResponse *ret = NULL;
        string msg(ex.what());
        cerr << "##################### Catched " << msg << endl;
        cerr << "request: " << hex << request << dec << endl;
        tracing::exception *btx =
            dynamic_cast<tracing::exception*>(&ex);
        if (NULL != btx) {
            string tmsg = btx->where();
            cerr << "Backtrace:" << endl << tmsg;
            if (0 != msg.compare("fatal")) {
                ret = HttpResponse::Error(HTTPRESPONSECODE_500_INTERNALSERVERERROR, request);
                string body(ret->GetBody());
                tmsg.insert(0, "<br>\n<pre>").append("</pre><p><a href=\"/\">Back to main page</a>");
                body.insert(body.find("</body>"), tmsg);
                ret->SetBody(body.c_str(), body.length());
            }
        }
        return ret;
    }

    // generates a page for each http request
    ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response)
    {
        if (0 == request->Uri().compare("/runtime")) {
            throw runtime_error("test exception");
        }
        if (0 == request->Uri().compare("/nonfatal")) {
            throw tracing::runtime_error("regular");
        }
        if (0 == request->Uri().compare("/fatal")) {
            throw tracing::runtime_error("fatal");
        }
        string body("<html><head><title>TestException</title></head><body>");
        body.append("<h2>Regular output</h2>");
        body.append("Try the following:");
        body.append("<ul><li><a href=\"/nonfatal\">Show an exception an continue</a>\n");
        body.append("<li><a href=\"/fatal\">Backtrace in console and terminate</a>\n");
        body.append("<li><a href=\"/runtime\">Runtime error in console and terminate</a>\n");
        body.append("</ul></body></html>");
        response->SetBody(body.c_str(), body.length());
        return HTTPRESPONSECODE_200_OK;
    }
};

// basic main that creates a threaded EHS object and then
//   sleeps forever and lets the EHS thread do its job.
int main (int argc, char **argv)
{
    cout << getEHSconfig() << endl;
    if (argc != 2) {
        cerr << "Usage: " << basename(argv[0]) << " [port]" << endl;
        return 0;
    }

    cerr << "binding to " << atoi(argv[1]) << endl;
    TestException srv;

    EHSServerParameters oSP;
    oSP["port"] = argv[1];
    oSP["mode"] = "threadpool";
    oSP["threadcount"] = 10;

    try {
        srv.StartServer(oSP);

        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
#ifdef _WIN32
			Sleep(300);
#else
			usleep(300000);
#endif
        }

        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }

    return 0;
}
