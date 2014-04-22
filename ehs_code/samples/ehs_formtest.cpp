/* $Id: ehs_formtest.cpp 160 2012-08-07 13:36:13Z felfert $
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
#include <string>

#include <cstdlib>

#include "common.h"

using namespace std;

class FormTester : public EHS {

public:

	FormTester ( ) : m_oNameList ( StringList ( ) ) { }

	ResponseCode HandleRequest ( HttpRequest *, HttpResponse * );

	StringList m_oNameList;
};


// creates a page based on user input -- either displays data from
//   form or presents a form for users to submit data.
ResponseCode FormTester::HandleRequest ( HttpRequest * request, HttpResponse * response )
{
    ostringstream oss;

    oss << "<html><head><title>StringList</title></head>" << endl << "<body>" << endl;
	// if we got data from the user, show it
	if ( request->FormValues ( "user" ).m_sBody.length ( ) ||
		 request->FormValues ( "existinguser" ).m_sBody.length ( ) ) {
			
		string sName = request->FormValues ( "existinguser" ).m_sBody;
		if ( request->FormValues ( "user" ).m_sBody.length() ) {
			sName = request->FormValues ( "user" ).m_sBody;
		}
		cerr << "Got name of " << sName << endl;
			
        oss << "Hi " << sName << "<p><a href=\"/\">Back to login form</a></body></html>";
		m_oNameList.push_back ( sName );
			
		response->SetBody( oss.str().c_str(), oss.str().length() );
		return HTTPRESPONSECODE_200_OK;

	} else {

		// otherwise, present the form to the user to fill in
		cerr << "Got no form data" << endl;

        oss << "<p>Please log in</p>" << endl << "<form action = \"/\" method=\"POST\">" << endl
            << "User name: <input type=\"text\" name=\"user\"><br />" << endl
            << "<select name=\"existinguser\" width=\"20\">" << endl;
		for ( StringList::iterator i = m_oNameList.begin(); i != m_oNameList.end ( ); ++i ) {
            oss << "<option>" << i->substr ( 0, 150 ) << endl;
		}
		oss << "</select> <input type=\"submit\">" << endl << "</form>" << endl;
	}
    oss << "</body>" << endl << "</html>";
    response->SetBody( oss.str().c_str(), oss.str().length() );
    return HTTPRESPONSECODE_200_OK;

}

// create a multithreaded EHS object with HTTPS support based on command line
//   options.
int main (int argc, char ** argv)
{

    cout << getEHSconfig() << endl;
	if (argc != 2 && argc != 4) {
        cout << "usage: " << basename(argv[0]) << " <port> [<certificate_file> <certificate_passphrase>]" << endl;
        cout << "\tIf you specify the last 2 parameters, it will run in https mode" << endl;
        return 0;
    }

    FormTester srv;

    EHSServerParameters oSP;
    oSP["port"] = argv[1];
    oSP["mode"] = "threadpool";

    if (argc == 4) {
        cout << "in https mode" << endl;
        oSP["https"] = 1;
        oSP["certificate"] = argv [ 2 ];
        oSP["passphrase"] = argv [ 3 ];

    }	

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
        srv.StopServer ( );
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }

    return 0;
}

