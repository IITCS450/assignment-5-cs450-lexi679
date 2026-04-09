#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#define MAXTHREADS 8
#define STACKSIZE 4096

// TODO: Implement cooperative user-level threads.

enum tstate {
    T_FREE,
    T_RUNNING,
    T_RUNNABLE,
    T_ZOMBIE
};

struct context {
    uint edi;
    uint esi;
    uint ebx;
    uint ebp;
    uint eip;
};

struct thread {
    tid_t tid;
    enum tstate state;
    void *stack;
    struct context *context;
    void (*fn)(void*);
    void *arg;
};

static struct thread threads[MAXTHREADS];
static int current = 0;
static int initialized = 0;
static int next_tid = 1;

static void thread_stub(void);
static int pick_next(void);

void thread_init(void){
    int i;
    for(i = 0; i < MAXTHREADS; i++){
        threads[i].tid = -1;
        threads[i].state = T_FREE;
        threads[i].stack = 0;
        threads[i].context = 0;
        threads[i].fn = 0;
        threads[i].arg = 0;
    }
    threads[0].tid = 0;
    threads[0].state = T_RUNNING;
    threads[0].stack = 0;
    threads[0].context = 0;
    threads[0].fn = 0;
    threads[0].arg = 0;

    current = 0;
    initialized = 1;
}

static void thread_stub(void) {
    int me = current;

    threads[me].fn(threads[me].arg);
    threads[me].state = T_ZOMBIE;
    thread_yield();

    exit();
}
tid_t thread_create(void (*fn)(void*), void *arg){
    int i;
    char *stack;
    char *sp;
    struct context *ctx;

    if(!initialized) thread_init();
    
    for(i = 1; i < MAXTHREADS; i++){
        if(threads[i].state == T_FREE) break;
    }

    if(i == MAXTHREADS) return -1;

    stack = malloc(STACKSIZE);
    if(stack == 0) return -1;

    sp = stack + STACKSIZE;
    sp -= sizeof(struct context);
    ctx = (struct context*)sp;

    ctx->edi = 0;
    ctx->esi = 0;
    ctx->ebx = 0;
    ctx->ebp = 0;
    ctx->eip = (uint)thread_stub;

    threads[i].tid = next_tid++;
    threads[i].state = T_RUNNABLE;
    threads[i].stack = stack;
    threads[i].context = ctx;
    threads[i].fn = fn;
    threads[i].arg = arg;

    return threads[i].tid;
}

static int pick_next(void){
    int i;
    int idx;
    for(i = 1; i <= MAXTHREADS; i++){
        idx = (current + i) % MAXTHREADS;
        if(threads[idx].state == T_RUNNABLE) return idx;
    }
    return -1;
}
    
void thread_yield(void){
    int prev;
    int next;

    next = pick_next();
    if(next < 0) return;

    prev = current;

    if(threads[prev].state == T_RUNNING) threads[prev].state = T_RUNNABLE;
    threads[next].state = T_RUNNING;
    current = next;

    uswtch(&threads[prev].context, threads[next].context);
}

int thread_join(tid_t tid){
    int i;
    
    for(;;) {
        for(i = 0; i < MAXTHREADS; i++){
            if(threads[i].tid == tid){
                if(threads[i].state == T_ZOMBIE){
                    if(threads[i].stack) free(threads[i].stack);

                    threads[i].tid = -1;
                    threads[i].state = T_FREE;
                    threads[i].stack = 0;
                    threads[i].context = 0;
                    threads[i].fn = 0;
                    threads[i].arg = 0;
                    return 0;
                }
                thread_yield();
                break;
            }
        }
        if(i == MAXTHREADS) return -1;
    }
}
