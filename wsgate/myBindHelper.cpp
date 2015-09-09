#ifndef _WIN32

#include "myBindHelper.hpp"
#include "wsgate.hpp"

using namespace std;

namespace wsgate {
    MyBindHelper::MyBindHelper()
        :mutex(pthread_mutex_t()){
            pthread_mutex_init(&mutex, NULL);
    }

    bool MyBindHelper::BindPrivilegedPort(int socket, const char *addr, const unsigned short port){
        bool ret = false;
        pid_t pid;
        int status;
        char buf[32];
        pthread_mutex_lock(&mutex);
        switch (pid = fork()) {
            case 0:
                sprintf(buf, "%08x%08x%04x", socket, inet_addr(addr), port);
                execl(BINDHELPER_PATH, buf, ((void *)NULL));
                exit(errno);
                break;
            case -1:
                break;
            default:
                if (waitpid(pid, &status, 0) != -1) {
                    ret = (0 == status);
                    if (0 != status) {
                        log::err << BINDHELPER_PATH << " reports: " << strerror(WEXITSTATUS(status)) << endl;
                        errno = WEXITSTATUS(status);
                    }
                }
                break;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }
}

#endif // _WIN32