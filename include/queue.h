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
} Queue;

void queue_init(Queue *q);
void queue_push_back(Queue *q, PCB *pcb);
PCB* queue_pop_front(Queue *q);
PCB* queue_remove_highest_priority(Queue *q); // Utilizado na preempção por prioridade
void queue_destroy(Queue *q);

#endif // QUEUE_H
