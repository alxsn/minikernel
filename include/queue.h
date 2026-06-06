#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"

// Estrutura de um nó da fila circular ou encadeada simples
typedef struct QueueNode {
    PCB *pcb;
    struct QueueNode *next;
} QueueNode;

// Estrutura da Fila de Prontos (Ready Queue) Protegida
typedef struct {
    QueueNode *head;
    QueueNode *tail;
    int size;
    pthread_mutex_t mutex;
} ReadyQueue;

void queue_init(ReadyQueue *q);
void queue_push_back(ReadyQueue *q, PCB *pcb);
PCB* queue_pop_front(ReadyQueue *q);
PCB* queue_remove_highest_priority(ReadyQueue *q); // Utilizado na preempção por prioridade
void queue_destroy(ReadyQueue *q);

#endif // QUEUE_H
