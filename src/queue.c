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

PCB* queue_remove_highest_priority(Queue *q){
    QueueNode *highest_priority_node, *current_node, *previous_node, *previous_highest_priority_node;
    PCB *pcb;

    if(q->size == 0){
        return NULL;
    }
    else if(q->size == 1){
        current_node = q->head;
        q->head = NULL;
        q->tail = NULL;
        q->size--;

        pcb = current_node->pcb;
        free(current_node);
        return pcb;
    }
    
    // q->size >= 2
    current_node = q->head;
    highest_priority_node = current_node;
    previous_highest_priority_node = NULL;
    for(int i = 0; i < q->size - 1; i++){
        if(i==0){// primeira passagem
            previous_node = highest_priority_node;
            if(highest_priority_node->pcb->priority > current_node->next->pcb->priority){
                previous_highest_priority_node = highest_priority_node;
                highest_priority_node = current_node->next;
            }
            current_node = current_node->next;
        }
        else{// segunda passagem em diante
            previous_node = previous_node->next;
            if(highest_priority_node->pcb->priority > current_node->next->pcb->priority){
                previous_highest_priority_node = previous_node;
                highest_priority_node = current_node->next;
            }
            current_node = current_node->next;
        }
    }

    // head tem maior prioridade
    if(highest_priority_node == q->head){
        q->head = q->head->next;
    }
    else if(highest_priority_node == q->tail){// tail tem maior prioridade
        previous_highest_priority_node->next = NULL;
        q->tail = previous_highest_priority_node;
    }
    else{// alguem que não é o head nem o tail tem maior prioridade
        previous_highest_priority_node->next = highest_priority_node->next;
    }
    
    q->size--;
    
    pcb = highest_priority_node->pcb;
    free(highest_priority_node);
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