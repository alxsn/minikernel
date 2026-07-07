#include "stdlib.h"
#include "pthread.h"
#include "queue.h"

// Inicializa a fila de prontos
void queue_init(Queue *q){
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_push_back(Queue *q, PCB *pcb){       
    // Cria novo nó
    QueueNode *node = (QueueNode*) malloc(sizeof(QueueNode));
    // Inicializa os atributos do nó
    node->pcb = pcb;
    node->next = NULL;

    // Verifica se já existem elementos na fila
    if(q->size == 0){
        q->head = node;
        q->tail = node;
    }
    else{
        q->tail->next = node;
        q->tail = node;
    }
    q->size++;
}

PCB* queue_pop_front(Queue *q){
    if(q->size == 0){
        return NULL;   
    }
    else if(q->size == 1){
        // Retira o primeiro nó da fila
        QueueNode *node = q->head;
        PCB *pcb = node->pcb;
        q->head = NULL;
        q->tail = NULL;
        q->size--;

        // Libera memória do nó retirado
        free(node);

        return pcb;  
    }

    // Retira o primeiro nó da fila
    QueueNode *node = q->head;
    PCB *pcb = node->pcb;
    q->head = q->head->next;
    q->size--;

    // Libera memória do nó retirado
    free(node);

    return pcb;    
}

PCB* queue_remove_highest_priority(Queue *q) {
    if (q == NULL || q->size == 0) return NULL;

    QueueNode *current = q->head;
    QueueNode *prev = NULL;
    
    QueueNode *highest = q->head;
    QueueNode *highest_prev = NULL;

    // 1. Localiza o nó de maior prioridade (menor valor numérico)
    while (current != NULL) {
        if (current->pcb->priority < highest->pcb->priority) {
            highest = current;
            highest_prev = prev;
        }
        prev = current;
        current = current->next;
    }

    // 2. Remove o nó encontrado ajustando os ponteiros da fila
    if (highest == q->head) {
        q->head = q->head->next;
        if (q->head == NULL) {
            q->tail = NULL;
        }
    } else if (highest == q->tail) {
        highest_prev->next = NULL;
        q->tail = highest_prev;
    } else {
        highest_prev->next = highest->next;
    }

    q->size--;
    PCB *pcb = highest->pcb;
    free(highest);
    return pcb;
}

void queue_destroy(Queue *q){
    QueueNode *node;
    int aux = q->size;
    for(int i=0; i < aux; i++){
        node = q->head;
        q->head = q->head->next;
        if(q->size == 1){
            q->tail = NULL;
        }
        q->size--;
        free(node);
    }
}