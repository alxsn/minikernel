#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>

// Definição das Políticas de Escalonamento
typedef enum {
    POLICY_FCFS = 1,
    POLICY_RR = 2,
    POLICY_PRIORITY = 3
} SchedulerPolicy;

// Estados possíveis de um processo
typedef enum {
    STATE_NOT_ARRIVED,
    STATE_READY,
    STATE_RUNNING,
    STATE_FINISHED
} ProcessState;

// DECLARAÇÕES ANTECIPADAS DAS ESTRUTURAS PURAS
struct TCB;
struct PCB;

// Estrutura BCP (Bloco de Controle de Processo)
typedef struct PCB {
    int pid;                        // Identificador único (lido da entrada)
    int process_len;                // Duração total do processo (ms)
    int remaining_time;             // Tempo restante de execução (ms)
    int priority;                   // Nível de prioridade (1 a 5, 1 é a maior)
    int num_threads;                // Quantidade de threads do processo
    int start_time;                 // Tempo de chegada (ms relativo ao início)
    ProcessState state;             // READY, RUNNING, FINISHED

    struct TCB *tcb_array;          // Vetor com threads do processo
    
    pthread_mutex_t mutex;          // Mutex exclusivo do processo
    pthread_cond_t cv;              // Variável de condição para acordar as threads do processo
} PCB;

// Estrutura TCB (Task Control Block)
typedef struct TCB {
    struct PCB* pcb;                // Ponteiro para o processo pai            
    int remaining_time;             // Quanto tempo esta thread específica precisa rodar
    ProcessState state;             // READY, RUNNING, FINISHED
} TCB;

#endif // TYPES_H