
/* this code cannot run! just for test and record! */

#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define SIZE 500

int main() {
    int fd1;
    fd1 = open("test.dat", O_RDWR);

    /**
     * 关于私有对象的探讨
    **/
    // fork 之后，都是私有的写时复制，不会互相影响了
    char *ptr = (char *)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd1, 0);
    pid_t pid = fork();
    if (pid == 0) {
        for (int i = 0; i < SIZE; ++i) {
            ptr[i] = 'f';
        }
        exit(0);
    }
    for (int i = 0; i < SIZE; ++i) {
        cout << ptr[i];
    }

    /**
     * 关于共享对象写回的探讨
    **/
    // 共享对象是会写回磁盘的，以下验证了一下，确实
    char *ptr = (char *)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
    for (int i = 0; i < SIZE; ++i) {
        ptr[i] = '0';
    }

    return 0;
}