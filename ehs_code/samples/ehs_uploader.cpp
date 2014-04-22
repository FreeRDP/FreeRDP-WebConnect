/* $Id: ehs_uploader.cpp 70 2011-12-12 17:11:44Z felfert $
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

/*
  This is a sample EHS program that allows you to upload
  a file to the server 
*/
#include <ehs.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "common.h"
#ifdef _WIN32
#include <io.h>
#endif
using namespace std;

class FileUploader : public EHS {
    public:
        FileUploader ( ) {}
        ResponseCode HandleRequest ( HttpRequest *, HttpResponse *);
};

ResponseCode FileUploader::HandleRequest ( HttpRequest * request, HttpResponse * response )
{
    string sUri = request->Uri();
    cerr << "Request-URI: " << sUri << endl;

    if ( sUri == "/" ) {
        string sMsg  = request->Cookies("ehs_upload_status");
        ostringstream oss;
        oss << "<html><head><title>ehs uploader</title></head>" << endl << "<body>" << endl
            << "  <form method=\"POST\" action=\"/upload.html\" enctype=\"multipart/form-data\">" << endl
            << "    Upload file:<br />" << endl
            << "    <input type=\"file\" name=\"file\"><br />" << endl
            << "    <input type=\"submit\" value=\"submit\">" << endl
            << "  </form>" << endl;
        if (!sMsg.empty()) {
            oss << "  <p>Status: " << sMsg << "<br />" << endl;
        }
        oss << "</body></html>" << endl;
        response->SetBody(oss.str().c_str(), oss.str().length());
        return HTTPRESPONSECODE_200_OK;
    }

    if (sUri == "/upload.html") {
        int nFileSize = request->FormValues("file").m_sBody.length();
        string sFileName = request->FormValues("file").
            m_oContentDisposition.m_oContentDispositionHeaders["filename"];
        cerr << "nFileSize = " << nFileSize << endl;
        cerr << "sFileName = '" << sFileName << "'" << endl;
        CookieParameters statusCookie;
        statusCookie["name"] = "ehs_upload_status";
        statusCookie["value"] = "";
        if (0 != nFileSize) {
            size_t lastSlash = sFileName.find_last_of("/\\");
            sFileName = sFileName.substr(lastSlash + 1);
            cerr << "stripped sFileName = '" << sFileName << "'" << endl;
            if (!sFileName.empty()) {
                // For safety reasons, we do not allow writing to existing files.
#ifdef _WIN32
				if (0 != _access(sFileName.c_str(), 0)) {
#else
                if (0 != access(sFileName.c_str(), F_OK)) {
#endif
                    cerr << "Writing " << nFileSize << " bytes to file" << endl;
                    ofstream outfile(sFileName.c_str(), ios::out | ios::trunc | ios::binary );
                    outfile.write(request->FormValues("file").m_sBody.c_str(), nFileSize);
                    outfile.close();
                    statusCookie["value"] = "Uploaded file successfully.";
                } else {
                    statusCookie["value"] = "No permission to overwrite a file.";
                }
            } else {
                statusCookie["value"] = "No filename received.";
                cerr << "NO FILENAME FOUND" << endl;
            }
        } else {
            statusCookie["value"] = "No content received.";
        }
        response->SetCookie(statusCookie);
        response->SetBody("", 0);
        response->SetHeader("Location", "/");
        return HTTPRESPONSECODE_301_MOVEDPERMANENTLY;
    }

    return HTTPRESPONSECODE_404_NOTFOUND;
}

int main(int argc, char ** argv)
{
    cout << getEHSconfig() << endl;
    if (argc < 2) {
        cout << "usage: " << basename(argv[0]) << " <port>" << endl;
        return 0;
    }

    FileUploader srv;
    EHSServerParameters oSP;
    oSP["port"] = argv[1];
    oSP["mode"] = "threadpool";
    oSP["maxrequestsize"] = 1024 * 1024 * 10;
    try {
        srv.StartServer(oSP);
        kbdio kbd;
        cout << "Press q to terminate ..." << endl;
        while (!(srv.ShouldTerminate() || kbd.qpressed())) {
            usleep(300000);
        }
        srv.StopServer();
    } catch (exception &e) {
        cerr << "ERROR: " << e.what() << endl;
    }
    return 0;
}
