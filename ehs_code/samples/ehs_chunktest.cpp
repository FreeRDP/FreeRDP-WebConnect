/* $Id: ehs_chunktest.cpp 70 2011-12-12 17:11:44Z felfert $
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

#include <ehs.h>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <cstdlib>
#include "common.h"

using namespace std;

// subclass of EHS that defines a custom HTTP response.
class ChunkTest : public EHS
{
    // generates a page, containing the request body for testing
    // chunked transfer encoding.
    ResponseCode HandleRequest(HttpRequest *request, HttpResponse *response)
    {
        response->SetBody(request->Body().c_str(), request->Body().length());
        response->SetHeader("Content-Type", "text/plain");
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
    ChunkTest srv;

    EHSServerParameters oSP;
    oSP["port"] = argv[1];
    oSP["mode"] = "threadpool";

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
