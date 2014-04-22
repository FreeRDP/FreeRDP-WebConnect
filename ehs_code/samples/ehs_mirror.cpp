/* $Id: ehs_mirror.cpp 121 2012-04-05 22:39:22Z felfert $
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
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "common.h"

using namespace std;

class MyEHS : public EHS { 

	ResponseCode HandleRequest ( HttpRequest * request, HttpResponse * response ) {

        ostringstream oss;
        oss << "ehs_mirror: Secure - " << (request->Secure() ? "yes" : "no") << endl
            << request->RemoteAddress() << ":" << request->RemotePort() << endl;
		response->SetBody ( oss.str().c_str(), oss.str().length() );
		return HTTPRESPONSECODE_200_OK;
	}

};

int main ( int argc, char ** argv )
{

    cout << getEHSconfig() << endl;
	if ((argc != 2) && (argc != 5)) {
        string cmd(basename(argv[0]));
		cout << "Usage: " << cmd << " <port> [<sslport> <certificate file> <passphrase>]" << endl; 
        return 0;
	}

	MyEHS plainEHS;
    MyEHS *sslEHS = NULL;

	EHSServerParameters oSP;
	oSP["port"] = argv[1];
	oSP["mode"] = "threadpool";

    try {
        plainEHS.StartServer(oSP);
        if (argc == 5) {
            sslEHS = new MyEHS;
            oSP["port"] = argv[2];
            oSP["https"] = 1;
            oSP["mode"] = "threadpool";
            oSP["certificate"] = argv[3];
            oSP["passphrase"] = argv[4];
            sslEHS->SetSourceEHS(plainEHS);
            sslEHS->StartServer(oSP);
        }

        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(plainEHS.ShouldTerminate() ||
                    (sslEHS && sslEHS->ShouldTerminate()) ||
                    kbd.qpressed()))
        {
            usleep(300000);
        }
        plainEHS.StopServer();
        if (NULL != sslEHS) {
            sslEHS->StopServer();
        }
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }
    delete sslEHS;

    return 0;
}
