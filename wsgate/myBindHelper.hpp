#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
 #include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifndef _WIN32

#ifndef _MY_BIND_HELPER_
#define _MY_BIND_HELPER_

#include <ehs/ehs.h>

namespace wsgate {

    // Bind helper is not needed on win32, because win32 does not
    // have a concept of privileged ports.
    class MyBindHelper : public PrivilegedBindHelper {
        public:
            MyBindHelper();
            bool BindPrivilegedPort(int socket, const char *addr, const unsigned short port);
        private:
            pthread_mutex_t mutex;
    };
}

#endif // _MY_BIND_HELPER_

#endif // _WIN32