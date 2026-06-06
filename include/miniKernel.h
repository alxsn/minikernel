#ifndef MINIKERNEL_H
#define MINIKERNEL_H

#include <pthread.h>
#include "types.h"
#include "queue.h"

// Estrutura de Contexto Central para evitar variáveis globais soltas (Seção 2.7)
typedef struct {
    PCB *pcb_list;                  // Lista de todos os processos do sistema
    int process_count;              // Quantidade total de processos lidos
    SchedulerPolicy policy;         // Política (1 = FCFS, 2 = RR, 3 = Prioridade)
    ReadyQueue ready_queue;         // Fila de prontos circular protegida
    int generator_done;             // Sinaliza que todos os processos já foram criados/enfileirados
    
    PCB *current_process;           // Processo atualmente em execução (Monoprocessador)
    PCB *current_process_cpu2;      // Segundo processo em execução (Multiprocessador)
    int quantum;                    // Quantum usado no Round Robin (500ms)

    // Sincronização global do sistema (Seção 2.5)
    pthread_mutex_t kernel_mutex;   // Mutex global do Mini-Kernel
    pthread_cond_t scheduler_cv;    // scheduler_condition_variable (Escalonador dorme se fila vazia)
} MiniKernel;

// Protótipos das funções que controlam o ciclo de vida das threads (Seção 2.3 e 2.4)

// Pipeline executado por cada thread associada a um processo
void* thread_pipeline(void *arg);

// Thread escalonadora para o modo Monoprocessador
void* scheduler_monoprocessador(void *arg);

// Threads escalonadoras para o modo Multiprocessador
void* scheduler_multiprocessador(void *arg);

#endif // MINIKERNEL_H