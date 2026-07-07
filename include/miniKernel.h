#ifndef MINIKERNEL_H
#define MINIKERNEL_H

#include <pthread.h>
#include "types.h"
#include "queue.h"

// Estrutura de Contexto Central para evitar variáveis globais soltas (Seção 2.7)
typedef struct {
    int process_count;              // Quantidade total de processos lidos
    SchedulerPolicy policy;         // Política (1 = FCFS, 2 = RR, 3 = Prioridade)
    PCB *pcb_array;                // Array de processos
    Queue ready_queue;              // Fila de processos prontos
    int generator_done;             // Sinaliza que todos os processos já foram criados/enfileirados
    
    TCB *current_thread_cpu[2];    // Threads em execução em cada processador
    int quantum;                   // Quantum usado no Round Robin (500ms)

    // Sincronização global do sistema (Seção 2.5)
    pthread_mutex_t kernel_mutex;   // Mutex global do Mini-Kernel
    pthread_cond_t scheduler_cv;    // scheduler_condition_variable (Escalonador dorme se fila vazia)

    char *log_file;
    int quantum_time_step;
} MiniKernel;

typedef struct{
    MiniKernel *kernel;
    int cpu_id;
} Processador;

// Protótipos das funções que controlam o ciclo de vida das threads (Seção 2.3 e 2.4)

// Thread escalonadora para o modo Monoprocessador
void* scheduler_monoprocessador(void *arg);

// Threads escalonadoras para o modo Multiprocessador
void* scheduler_multiprocessador(void *arg);

#endif // MINIKERNEL_H