#include "miniKernel.h"

void* thread_pipeline(void *arg) {
    TCB *tcb = (TCB*) arg;
    // ... lógica das threads simuladas ...
    free(tcb);
    return NULL;
}

void* scheduler_monoprocessador(void *arg) {
    MiniKernel *kernel = (MiniKernel*) arg;
    // Lógica do Escalonador Mono
    return NULL;
}

void* scheduler_multiprocessador(void *arg) {
    MiniKernel *kernel = (MiniKernel*) arg;
    // Lógica do Escalonador Multi (2 CPUs consumindo da mesma ready_queue)
    return NULL;
}