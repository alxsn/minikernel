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

// Estrutura BCP (Bloco de Controle de Processo) conforme especificação
typedef struct {
    int pid;                        // Identificador único (lido da entrada)
    int process_len;                // Duração total do processo (ms)
    int remaining_time;             // Tempo restante de execução (ms) - Precisa de Mutex!
    int priority;                   // Nível de prioridade (1 a 5, 1 é a maior)
    int num_threads;                // Quantidade de threads do processo
    int start_time;                 // Tempo de chegada (ms relativo ao início)
    ProcessState state;             // READY, RUNNING, FINISHED - Precisa de Mutex!

    TCB *tcb_array;                  //vetor com threads do processo
    
    pthread_mutex_t mutex;          // Mutex exclusivo do processo
    pthread_cond_t cv;              // Variável de condição para acordar as threads do processo
    // pthread_t *thread_ids;          // Vetor com os IDs das threads reais do POSIX
} PCB;

// Estrutura TCB (Task Control Block) conforme especificação
typedef struct {
    // Herda a prioridade do processo
    PCB* pcb;                       // Ponteiro para o processo pai            
    // int tid;            // ID da thread (1, 2, 3...) (0 até num_threads-1)
    int remaining_time; // Quanto tempo esta thread específica precisa rodar
    ProcessState state;             // READY, RUNNING, FINISHED
} TCB;

#endif // TYPES_H
