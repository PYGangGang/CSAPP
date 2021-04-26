# 异常控制流 / Exceptional Control Flow / ECF

**异常的基本概念**

---

我们可以设想的最简单的控制流，是平滑的，即每条指令依次存放在内存中，程序计数器依次地执行并指向下一个位置。而加上简单的跳转、调用和返回，我们给控制流加上了突变。以上，都是由应用程序操纵的控制流，而我们需要让程序和操作系统能够对系统状态的转变做出举动，由此引入了异常控制流（ECF）。简言之，我们有一个记录异常的寄存器；在任意情况下（可能是执行当前指令引起，也可能不是），该寄存器中的值发生变化，我们称为一次异常发生了；处理器会通过查询异常表（其中记录了异常号 -> 异常处理程序地址的映射）执行对应的异常处理程序；根据不同的规则，执行异常处理程序后我们可能 重新执行当前指令，继续执行下一条指令，直接结束程序。

* 异常的分类

|类型|原因|同步/异步|返回行为|例子|
|中断（interrupt）|I/O设备的信号|异步|总是返回下一条指令|定时器芯片定时将 CPU 上某个引脚置高电位来表明到了一次 interval|
|陷阱（trap）|有意的异常（是执行当前指令的结果）|同步|总是返回下一条指令|执行一次系统调用|
|故障（fault）|潜在可恢复异常（是执行当前指令导致的）|同步|可能返回当前指令|缺页异常|
|终止（abort）|不可恢复的异常|同步|不会返回|硬件错误|

* 异常的性质
    * 异常一部分由 CPU 架构定义，一部分由操作系统定义；如 x86-64 中有 256 种不同的异常类型，其中 0 - 31 由 Intel 定义，32 - 255 是由操作系统定义的陷阱和中断
    * 异常处理程序运行在内核态中；所以，调用系统函数是用户态程序陷入内核的方法之一（此处涉及操作系统中的内核态和用户态）

**进程的基本概念 —— 使用较低层次异常机制之上的上下文切换 异常控制流**

---

CS: APP 中说：异常是允许操作系统内核提供进程（process）概念的基本构造块，**进程是计算机科学中最深刻，最成功的概念之一。**

进程是**一个执行中的程序的实例**。系统中的每个进程运行在其上下文中（context），上下文由程序正确运行所需要的状态组成。

进程提供了两种关键抽象：

* 一个独立的逻辑控制流：仿佛我们的程序独占 CPU
* 一个私有的地址空间：仿佛我们的程序独占 内存

内核模式和用户模式：通过控制寄存器中的模式位（mode bit）来描述当前进程享有的特权；用户模式中的进程不允许执行特权指令，只能通过中断、故障或者系统调用，进入内核模式后才行

* 注意：没有单独的内核进程。可以这样描述，内核正代表进程 A 在内核模式下执行指令

上下文切换：在进程执行的某些时刻，内核通过调度器（scheduler）进行进程的切换；总结起来，中断（异步异常）或使用陷阱等（同步异常）都可能会导致调度（控制流的变化）

**进程的控制 —— Unix 为例**

---

Unix 提供了大量的从 C 程序中操作进程的**系统调用**。

关于 Unix 系统调用的封装函数，被定义在 unistd.h 头文件中

[1] 获取进程 ID

```
pid_t getpid(void); // 获取调用进程的 PID
pid_t getppid(void); // 获取父进程的 PID
```

[2] 创建和终止进程

进程可以被看作始终处于以下三种状态：

* 运行：正在执行或等待被执行且最终会被内核调度
* 挂起：暂停执行，且不会被调度，直到收到继续执行的信号（SIGCONT）
* 终止：永远的停止（收到停止信号、return、exit）

终止进程：

```
void exit(int status); // 以 status 退出状态结束
// 或者从 main 函数返回
```

创建进程：

```
#include <sys/types.h> // pid_t 等被定义在这个头文件中
pid_t fork(void);
```

* fork 函数很特殊，一次调用，会产生两个返回值；我们用一个例子说明

```
int main() {
    pid_t = pid;

    pid = fork();
    if (pid == 0) {
        /* ****这一段只可能在子进程中执行**** */
        cout << "this is child proc" << endl;
        exit(0);
        /* ******************************** */
    }
    /* ****子进程不会到达这里**** */
    cout << "this is parent proc" << endl;
    return 0;
}
```

* 两个返回值：在父进程中，fork 的返回值为其创建的子进程的 PID；在子进程中，fork 的返回值为 0
        * ***注意： 对于所有的系统调用，我们都需要检测其返回值，避免不可预知的错误，上面是 bad code

* 父进程和子进程拥有相同但是互相独立的地址空间；可以理解为完全一样的两个进程，但是 PID 不同
* 父进程和子进程将会并发地执行，我们不能保证，也不应该假设 fork 之后的执行顺序
* 子进程共享父进程所打开的文件

[3] 回收子进程

当一个进程因任何原因结束运行之后，他将处于**终止状态**，而不是立即被内核清除，直到他被其父进程回收（reaped）

僵尸进程：当一个子进程处于终止状态而没被回收；它的 task_struct 中保存了它的一系列值（如退出码，PID等）

孤儿进程：在子进程被回收前，父进程退出了；init 进程将会托管并回收他们

* init 进程是系统启动时创建的，它的 PID 为 1

回收进程：

* waitpid 函数（定义在 sys/wait.h 中）
    * 默认情况下，它会挂起调用进程，直到它等待 wait set 中的一个子进程终止（如果已经有子进程提前终止，则不需要等待）

```
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *statusp, int options);
```

* 参数 pid 指定了 wait set ：> 0 即为该 PID 的子进程；-1 即为父进程的所有子进程
    * 参数 statusp 指向用来存子进程退出状态的变量，具体的一些宏定义在 wait.h 中（可以为空）
    * 参数 options 定义了一些可以的操作
        * 具体的宏见 《CS: APP》 p517

* wait 函数

```
pid_t wait(int *statusp) //  等价于 waitpid 的默认形式：waitpid(-1, &status, 0);
```

[4] 让进程休眠

* sleep(int secs)：让进程挂起 secs 秒
* pause(void)：让进程挂起，直到收到一个信号

[5] 加载并运行程序

* execve

```
int execve(const char *filename, const char *argv[], const char *envp[]);
```

* 通过传入文件名、参数和环境变量来启动一个程序
    * ***注意：execve 加载进来的程序会直接在调用进程中执行，即覆盖掉当前进程中的程序
    * execve 只会在执行失败的时候有返回值，执行成功后就不是一个程序了，物是人非

**信号 —— 更高层的软件形式的异常（软中断）**

---

首先我们要搞清楚信号和低级别异常的区别。硬件产生的中断或者故障异常通常由内核处理，对于用户程序是不可见的；而信号可以理解为内核对一系列低层次异常的抽象，并将其告知给用户进程。举一个我理解的例子：Linux 上我们按 crtl + c 可以退出程序，实际上这只会触发两次硬件的中断（键盘键入），而内核相当于有一个自定义的中断处理函数，将这种中断的组合行为抽象为了一个 SIGINT 信号，并告知给用户进程。所以，**信号可以理解为 Unix 系统面向用户进程提供的一种高级别软件中断机制**。

[1] 信号的流转机制

信号的处理分为 发送信号 和 接收信号

* 发送信号：更新目的进程上下文中的某个状态，向其发送一个信号
* 接收信号：目的进程被内核强迫对某个信号做出了反应
    * 反应可以为忽略、终止或者执行信号处理程序

* 待处理信号（pending signal）：发送了，但是还没有被处理的信号
    * 一种类型的信号至多有一个待处理信号：如父进程暂时阻塞了所有信号，这个时候有 n 个子进程都结束了，发来了 n 个 SIGCHLD 信号，此时 pending 上并不会有队列，而是被理解为有 1 个待处理的 SIGCHLD 信号，父进程从阻塞中恢复时只会处理 SIGCHLD 一次
    * pending 位向量中维护着所有的待处理信号；blocked 位向量中维护着所有的阻塞信号（被阻塞的信号仍可以发过来，但是不会被接收，直到阻塞取消）

[2] 信号发送

* 用 /bin/kill 发送任意信号：/bin/kill -n PID （向 ID 为 PID 的进程发送信号 n）；/bin/kill -n -PGID （向 ID 为 PGID 的进程组发送信号 n）
    * 特殊的，发送 SIGKILL 信号：/bin/kill -9 PID

* 从键盘发送信号：ctrl + c 发送 SIGINT 信号；ctrl + z 发送 SIGTSTP 信号，给 前台进程组中的每个进程
* 用 kill 函数发送任意信号：定义在 signal.h 中
* 用 alarm 函数向自己发送 SIGALRM 信号：unsigned int alarm(unsigned int secs);

[3] 信号接收

当内核将进程 p 从内核模式切换到用户模式时（如从系统调用返回，或完成上下文切换），它都会检测未被阻塞的待处理信号集合（pending &~ blocked），先完成对信号的处理，再将控制流交给用户程序。

每个信号都有一个默认的处理逻辑，但除 SIGSTOP 和 SIGKILL 以外的信号，我们都可以修改其默认行为；详细的默认行为定义见：《CS: APP》p527

```
#include <signal.h>
typedef void (*sighandler_t)(int)

sighandler_t signal(int signum, sighandler_t handler); // 使用 signal 函数，传入一个函数指针来修改某信号的默认行为

// 以下为例子

void sigint_handler(int sig) {
    cout << "this is sigint costmized handler" << endl;
    exit(0);
}

int main() {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        unix_error("signal error");
    }
    pause();
    return 0;
}
```

[4] 阻塞和解除阻塞信号

如上文所说的 pending 和 blocked 位向量，信号的阻塞其实就是操作 blocked 位向量，设置其某些位，以使该位代表的信号被阻塞；signal.h 中提供了一系列函数来操作：

```
#include <signal.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *old_set);

/* **设置 set 位向量的函数** */
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);

/* **判断该信号是否在该 set 中** */
int sigismember(const sigset_t *set, int signum);
```

* 我们通过指定 how 来决定怎么改变当前的阻塞规则
    * SIG_BLOCK：阻塞 set 中的信号
    * SIG_UNBLOCK：解除阻塞 set 中的信号
    * SIG_SETMASK：直接将 blocked 设置为 set

* 用一个例子来说用法：

```
sigset_t mask, old_mask;

// 将 SIGINT 信号添加到 set 中去
sigemptyset(mask);
sigaddset(mask, SIGINT);

// 阻塞 SIGINT 信号
sigprocmask(SIG_BLOCK, mask, old_mask);

/* **do something** */

// 解除阻塞 SIGINT 信号，即用 old_mask 去做恢复
sigprocmask(SIG_SETMASK, old_mask, NULL);
```

**信号处理程序 —— 并发编程的基础思想**

---

注意事项：

[1] 处理程序尽可能简单：如只是设置一些全局标志位就返回

[2] 处理程序中只用异步信号安全的函数：类似多线程中的线程安全的概念，不应该有竞争；详见：《CS: APP》p534

[3] 保存和恢复 errno：主程序可能会用到；该全局变量定义在 errno.h 中

[4] 访问共享的全局数据结构时，阻塞所有的信号

[5] 使用 volatile 关键字声明全局变量：这会告诉编译器不要缓存这个变量，而每次从内存中去读取这个变量，以让这个变量的变化及时被所有人发现

[6] 用 sig_atomic_t 声明标志符：对于 [1] 中说到的情况，用 sig_atomic_t 做标志位的话，读写是一次原子操作，不会被打断

正确的信号处理：

主要的矛盾点在于**信号在目的进程上是不会排队的**，所以我们不能假设所有发送的信号都会被顺序处理。

错误的例子：在一个回收子进程的信号处理程序中，子进程发送了 SIGCHLD 信号后，我们就执行 wait 系统调用去回收子进程，并每次只回收一个；这时会发生一个细微的错误：若正在回收第一个子进程，第二个和第三个子进程都到了，那么第二个子进程发送来的 SIGCHLD 信号会被“忽略”，因为 pending 并不会排队；最终，我们只执行了两次 wait，那么有一个子进程就成为了没有被回收的僵尸进程。

正确的例子：我们把收到的 SIGCHLD 理解为——至少有一个子进程结束了，而非有一个子进程结束了，那我们每次 wait 的时候，把所有处于终止状态的子进程都回收了就好了

可移植的信号处理：

<span style="background-color: #ffaaaa">sigaction 函数 和 Signal 包装类</span>

多进程编程的简单注意事项

---

[1] 同步流以避免并发错误

简言之：当信号处理函数和主程序需要操作同一个全局变量时，我们应该总是在完整的操作上（如 fork 子进程之前）就阻塞所有的信号；抽象一下，我们不能假设不同进程之间的调度顺序，不能说父进程一定比子进程先执行；

[2] 显示地等待信号

* 用全局标志位来表明信号的到来：在信号处理程序中改变标志位，以告知主程序，主程序则循环等待

```
// 主程序第一次到这儿时，flag 为 0
// 信号到了过后，信号处理程序会将 flag 置 1
while (!flag); // 这就是一个空循环，一直忙等
... // 等到信号过后的操作
```

* 这样的操作太浪费资源了，我们可以用 sleep 或 pause 来减少浪费

```
while (!flag) {
    sleep(1);
}
// or
while (!flag) {
    pause();
}
```

* 这样也会有时间上的浪费

* 用 sigsuspend 函数：这是一个原子操作，挂起当前进程，等待 mask 中没有阻塞的信号

```
#include <signal.h>

int sigsuspend(const sigset_t *mask);

// 可以看作以下操作的原子形态
sigprocmask(SIG_SETMASK, &mask, &prev); // 阻塞
pause(); // 挂起，只等待未阻塞信号
sigprocmask(SIT_SETMASK, &prev, NULL); // 解除阻塞
```

**非本地跳转（nonlocal jump） —— 软件实现应用层异常处理的基础**

---

两个特殊的函数实现：setjmp 和 longjmp

```
#include <setjmp.h>

int setjmp(jmpbuf env);
int sigsetjmp(sigjmp_buf env, int savesigs); // 适用于信号处理程序的版本
/* ***正常的 setjmp 返回 0；从 longjmp 中返回时，返回值是 1 *** */

void longjmp(jmpbuf env, int retval);
void siglongjmp(sigjmpbuf env, int retval);
```

* 使用 setjmp 存下当前运行状态到全局变量 buf 中，在调用 longjmp 时，程序会重新回到调用 setjmp 的那个地方，返回值为 1；
* 谨慎使用，以下为一个实现程序软重启的例子

```
sigjmp_buf buf;

void handler(int sig)
{
    siglongjmp(buf, 1);
}

int main()
{
    if (!sigsetjmp(buf, 1)) {
        Signal(SIGINT, handler);
        Sio_puts("starting\n");
    }
    else
        Sio_puts("restarting\n");

    while(1) {
        Sleep(1);
        Sio_puts("processing...\n");
    }
    exit(0); /* Control never reaches here */
}
/* $end restart */
```

**小结**

---

学完这一章，主要的收获在对异常控制流的概念有了清晰认识，也对各种高级的软件异常有了一个自下而上的认识（知道他们是内核根据底层异常的组合来实现的）；另外就是一些细节的 Unix 中的系统函数的功能认识；其中比较关键的也是对于并发程序的常见问题和注意事项有了一个 mind map
