#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
int main() { return -EINVAL; }
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int main(int argc, char**argv)
{
    if (1 == argc) {
        unsigned int sock;
        in_addr_t ia;
        unsigned short port;
        if (3 == sscanf(argv[0], "%8x%8x%4hx", &sock, &ia, &port)) {
            struct sockaddr_in sa;
            if (sock >= 16384) /* glibc's limit of FDs */
                return EINVAL;
            if (port >= 1024)
                return EINVAL;
            memset(&sa, 0, sizeof(sa));
            memcpy(&sa.sin_addr, &ia, sizeof(sa.sin_addr));
            sa.sin_family = AF_INET;
            sa.sin_port = ntohs(port);
            if (bind(sock, (struct sockaddr *)&sa, sizeof(sa))) {
                return errno;
            }
            return 0;
        }
    }
    return EINVAL;
}
#endif
