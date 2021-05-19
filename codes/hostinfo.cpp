#include "include/csapp.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "usage: " << argv[0] << " <domain name>!" << endl;
        exit(0);
    }

    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    if ((rc = getaddrinfo(argv[1], NULL, NULL, &listp)) != 0) {
        cout << "getaddrinfo failed!" << endl;
        exit(0);
    }

    flags = NI_NUMERICHOST; // 指定打印地址而非域名
    for (p = listp; p != NULL; p = p->ai_next) {
        getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        cout << buf << endl;
    }
    freeaddrinfo(listp);
    return 0;    
}