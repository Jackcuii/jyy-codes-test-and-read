#include <am.h>
#include <klib.h>
#include <klib-macros.h>

typedef union thread {
    struct {
        const char    *name;
        void          (*entry)(void *);
        Context       context;
        union thread  *next;
        char          end[0];
    }; //C的一个特性，一个长度为0的array，这样就可以获得尾部的指针。
    uint8_t stack[8192];//线程数据结构就在栈的头部（这是union)
} Thread;

void T1(void *);
void T2(void *);
void T3(void *);

Thread threads[] = {
    // Context for the bootstrap code:
    { .name = "_", .entry = NULL, .next = &threads[1] },
    // 最后也并没有解释为什么一定要一个0号啊
    // Thread contests:
    { .name = "1", .entry = T1, .next = &threads[2] },
    { .name = "2", .entry = T2, .next = &threads[3] },
    { .name = "3", .entry = T3, .next = &threads[1] },
}; // 三个线程的
Thread *current = &threads[0];


// 发生中断之后，直接进入AM的代码，保存了各种寄存器到栈上（哪个栈？），并且获得栈上这个现场数据结构（trap_frame）的头指针, (AM也做了操作系统的工作的一部分
// 这个trap_frame里面既有硬件帮助保存的现场，也有操作系统保存的现场)，之后查询对应的中断处理程序表，并且跳转user_handler，也就是这个被init的on_interrupt
Context *on_interrupt(Event ev, Context *ctx) {  //中断处理程序
    // Save context. 把传过来的当前的现场，存进代表当前线程的数据结构里面。
    // 为什么jyy画的这个图是在这个栈？AM怎么知道的？
    current->context = *ctx;//这是完整的复制，这是指针解引用！

    // Thread schedule.
    current = current->next;
    //把current 指向下一个进程

    // Restore current thread's context.
    return &current->context; // 这个返回值会被拿去加载到处理器上，也就是下一个要进行的进程的现场。
			      // 会被拿去先进行OS层面的现场恢复，再进行iret指令，也就是硬件提供的那部分现场恢复
}

int main() {
    cte_init(on_interrupt);

    for (int i = 1; i < LENGTH(threads); i++) {
        Thread *t = &threads[i];
        t->context = *kcontext(
            // a Thread object:
            // +--------------------------------------------+
            // | name, ... end[0] | Kernel stack ...        |
            // +------------------+-------------------------+
            // ^                  ^                         ^     
            // t                  &t->end                   t + 1
            (Area) { .start = &t->end, .end = t + 1, },
            t->entry, NULL
        );
    }  //初始化维护线程的数据结构

    yield();
    assert(0);  // Never returns.
}


void delay() {
    for (int volatile i = 0;
         i < 10000000; i++);
}

void T1(void *arg) { while (1) { putch('A'); delay(); } }
void T2(void *arg) { while (1) { putch('B'); delay(); } }
void T3(void *arg) { while (1) { putch('C'); delay(); } }
