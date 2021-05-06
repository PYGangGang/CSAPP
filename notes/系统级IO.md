# 系统级 I/O

**Unix I/O**

---

I/O 是指在主存和外部设备（包括磁盘、终端、网络）之间的复制数据的过程。在 Linux 上可以使用内核提供的系统级 Unix I/O 实现，而各类高级语言使用的 I/O 库也是基于此建设的。

在 Linux 中，一切皆为文件。我们将设备、磁盘文件、网络都抽象为文件，通过统一的打开、改变位置、读、写、关闭操作，来实现输入、输出。进程可以用以下 Unix 提供的系统调用：

```cpp
// 相关头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
```

* 文件描述符：每个进程有一个文件描述符列表，每个文件描述符用来标识一个特定的打开文件

* 打开文件：若打开 or 创建成功，则会绑定空闲的文件描述符中最小的，并返回

```cpp
int open(char *filename, int flags, mode_t mode);
/* flags：如何访问文件的一个多位掩码设置 */
// O_RDONLY：只读
// O_WRONLY：只写
// O_RDWR：读写

// O_CREAT：若不存在则创建
// O_TRUNC：若存在则截断
// O_APPEND：在文件后面接上

/* mode：设置访问权限 */
```

* 关闭文件

```cpp
int close(int fd);
```

* 读文件：将 n 个字节从文件拷贝到 buf 开始的内存

```cpp
ssize_t read(int fd, void *buf, size_t n);
```

* 写文件：将从 buf 开始的 n 个字节的内存拷贝到文件

```cpp
ssize_t write(int fd, const void *buf, size_t n);
```

* 改变位置：lseek 函数，暂不讨论

**从不足值看 Unix I/O 的不足**

---

从 read 和 write 的函数形式就能看出，它可能不会按照应用程序的希望执行，即传输的字节小于要求值：

* 读时 EOF，此时会返回 0
* 从终端读取文本每次传送一个文本行，不能完美匹配
* 读写 socket 因为网络延迟也会有不匹配

再试想一个按行读取文件的程序，用单纯的 Unix I/O 实现的话，我们需要一个一个地取字符，来检测是否取到了一个换行符，而每次读取一个字符，都是一次陷入内核的系统调用，这个开销相对较大。

我们可以用带缓冲的输入输出来解决：它们会将文件内容缓存在一个应用级缓冲区，而读取时首先从缓冲区拿数据，如果缓冲区用完了，再由包装函数去填充缓冲区。

《CS: APP》中给出了一种带缓冲的线程安全的读写库：RIO；我将其总结一下，不再细讲：

* 最底层的 rio_write, rio_read 是对系统调用 read, write 的包装，它们主要注意了：
  * 处理中断：当系统调用被中断时，将重置整个读写过程，相当于重新调用
  * 读写计数：记录特定的一次系统调用的读写字符数，用是否有剩余未读写字符来决定是否继续发起读写
* 读缓冲区用一个 rio_t 数据结构来指明：fd、未读字节数、未读起始地址、缓冲区数据
* 高层的读操作 rio_readlineb, rio_readnb 则首先充 cache（若没有缓冲了），其次从缓冲中直接 memcpy

**读取其他数据**

---

除了基本的文件操作，还有几种系统调用：

* 读文件元数据：读取关于文件的信息，一个叫做 stat 的数据结构

```cpp
#include <sys/stat.h>

int stat(const char *filename, struct stat *buf);
int fstat(int fd, struct stat *buf);
```

其中，st_mode 成员包含了文件的字节数大小；st_mode 编码了文件访问许可位，用 sys/stat.h 里的宏：

```cpp
S_ISREG(st_mode); // 是一个普通文件吗？
S_ISDIR(st_mode); // 是一个目录文件吗？
S_ISSOCK(st_mode); // 是一个 socket 吗？
```

* 读取目录

```cpp
#include <sys/types.h>
#include <dirent.h>

DIR *opendir(const char *name); // 返回一个目录流指针
struct dirent *readdir(DIR *dirp); // 每次调用返回目录下的下一个文件，没了就返回 NULL
int closedir(DIR *dirp); // 关闭目录
```

**共享文件 & 重定向**

---

如前面所说，每个进程都维护了一张**文件描述符表**，每个表项指向了一个由所有进程共享的**文件表**中的一个文件项，这个文件项又指向了一个由所有进程共享的 **v-node 表**。

* 文件描述符表：表示该进程打开的文件集合，fd -> file table
* 文件表：表示在操作系统中一次打开操作，每次不同的打开就有一个新的文件表项，记录了当前的文件位置（offset）、引用计数（如果不再有进程引用这个“打开”了，就会被删除），file table -> v-node
* v-node：这才表示一个文件

两个描述符怎么共享磁盘上的一个文件呢？显然，只需要一个 v-node，而用两个文件表项来表示当前的读写情况（offset）等。

父子进程之间直接复制文件描述符表，读写位置之类的显然一样的。

**重定向**的含义即是用一个文件描述符 old_fd 指向的文件表项来覆盖一个新的文件描述符 new_fd。比如在 Linux 里我们经常把一些本该打印到终端的输出，搞到一个文件里去：

```bash
ls > foo.dat
```

这里就是一次重定向，我们可以用 dup2 系统调用实现：

```cpp
#include <unistd.h>

int dup2(int oldfd, int newfd);
```

**标准 I/O**

---

标准输入输出流，用流来抽象如 RIO 包中的缓冲的概念。

这里《CS: APP》讲的比较少，再看下补充。