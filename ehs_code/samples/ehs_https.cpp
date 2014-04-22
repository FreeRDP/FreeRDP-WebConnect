/* $Id: ehs_https.cpp 81 2012-03-20 12:03:05Z felfert $
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
#include <iostream> 
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "common.h"

using namespace std;

#ifndef _WIN32
// Bind helper is not needed on win32, because win32 does not
// have a concept of privileged ports.
class MyHelper : public PrivilegedBindHelper
{
    public:

        virtual bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port)
        {
            bool ret = false;
            pid_t pid;
            int status;
            char buf[32];
            pthread_mutex_lock(&mutex);
            switch (pid = fork()) {
                case 0:
                    sprintf(buf, "%08x%08x%04x", socket, inet_addr(addr), port);
                    execl("bindhelper", buf, ((void *)NULL));
                    exit(errno);
                    break;
                case -1:
                    break;
                default:
                    if (waitpid(pid, &status, 0) != -1) {
                        ret = (0 == status);
                        if (0 != status)
                            cerr << "bind: " << strerror(WEXITSTATUS(status)) << endl;
                    }
                    break;
            }
            pthread_mutex_unlock(&mutex);
            return ret;
        }

    private:
        static pthread_mutex_t mutex;
};

pthread_mutex_t MyHelper::mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

int main(int argc, char ** argv)
{

    cout << getEHSconfig() << endl;
	if (argc != 4) {
		cout << "Usage: " << basename(argv[0]) << " <port> <certificate file> <passphrase>" << endl; 
        return 0;
	}

	EHS srv;
#ifndef _WIN32
    MyHelper h;
    srv.SetBindHelper(&h);
#endif

	EHSServerParameters oSP;
	oSP["port"] = argv[1];
	oSP["https"] = 1;
	oSP["certificate"] = argv[2];
	oSP["passphrase"] = argv[3];
	oSP["mode"] = "threadpool";

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
