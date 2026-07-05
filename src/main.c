#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "minikernel.h"

int main(int argc, char *argv[]) {
    if (argc < 2) return EXIT_FAILURE;

    FILE *entry_file = fopen(argv[1], "r");
    if (!entry_file){
        printf("Erro: Nao foi possivel abrir o arquivo de entrada '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Inicializa a instância central do nosso sistema
    MiniKernel kernel;
    kernel.quantum = 500;
    pthread_mutex_init(&kernel.kernel_mutex, NULL);
    pthread_cond_init(&kernel.scheduler_cv, NULL);
    queue_init(&kernel.ready_queue);
    kernel.generator_done = 0;
    kernel.current_thread_cpu[0] = NULL;
    kernel.current_thread_cpu[1] = NULL;
    kernel.log_file = "log_execucao_minikernel.txt";

    if (fscanf(entry_file, "%d", &kernel.process_count) != 1) {
        fclose(entry_file);
        return EXIT_FAILURE;
    }

    kernel.pcb_array = (PCB*) malloc(kernel.process_count * sizeof(PCB));

    for (int i = 0; i < kernel.process_count; i++) {
        kernel.pcb_array[i].pid = i + 1;
        fscanf(entry_file, "%d", &kernel.pcb_array[i].process_len);
        fscanf(entry_file, "%d", &kernel.pcb_array[i].priority);
        fscanf(entry_file, "%d", &kernel.pcb_array[i].num_threads);
        fscanf(entry_file, "%d", &kernel.pcb_array[i].start_time);
        
        kernel.pcb_array[i].remaining_time = kernel.pcb_array[i].process_len;
        kernel.pcb_array[i].state = STATE_NOT_ARRIVED;
        
        pthread_mutex_init(&kernel.pcb_array[i].mutex, NULL);
        pthread_cond_init(&kernel.pcb_array[i].cv, NULL);
        kernel.pcb_array[i].tcb_array = (TCB*) malloc(kernel.pcb_array[i].num_threads * sizeof(TCB));
    }

    int policy_code;
    fscanf(entry_file, "%d", &policy_code);
    kernel.policy = (SchedulerPolicy)policy_code;
    fclose(entry_file);

    // ALOCAÇÃO DINÂMICA NA HEAP PARA EVITAR CORRUPÇÃO DE PILHA
    Processador *cpu0 = (Processador*) malloc(sizeof(Processador));
    cpu0->cpu_id = 0;
    cpu0->kernel = &kernel;

    Processador *cpu1 = (Processador*) malloc(sizeof(Processador));
    cpu1->cpu_id = 1;
    cpu1->kernel = &kernel;

    pthread_t scheduler_cpu1;
    
    #ifdef VERSION_MULTIPROCESSOR
        pthread_t scheduler_cpu2;
        pthread_create(&scheduler_cpu1, NULL, scheduler_multiprocessador, cpu0);
        pthread_create(&scheduler_cpu2, NULL, scheduler_multiprocessador, cpu1);
    #else
        pthread_create(&scheduler_cpu1, NULL, scheduler_monoprocessador, cpu0);
    #endif

    // --- SUA LÓGICA: Organiza os processos em ordem crescente por tempo de chegada ---
    for (int i = 0; i < kernel.process_count - 1; i++) {
        for (int j = 0; j < kernel.process_count - i - 1; j++) {
            if (kernel.pcb_array[j].start_time > kernel.pcb_array[j+1].start_time) {
                PCB temp = kernel.pcb_array[j];
                kernel.pcb_array[j] = kernel.pcb_array[j+1];
                kernel.pcb_array[j+1] = temp;
            }
        }
    }

    // --- SUA LÓGICA: Controla a chegada real por usleep ---
    int tempo_atual_real = 0;

    for (int i = 0; i < kernel.process_count; i++) {
        int tempo_espera = kernel.pcb_array[i].start_time - tempo_atual_real;
        
        if (tempo_espera > 0) {
            // Dorme a diferença de tempo necessária até o processo chegar
            usleep(tempo_espera); 
            tempo_atual_real = kernel.pcb_array[i].start_time;
        }

        pthread_mutex_lock(&kernel.kernel_mutex);

        // Prepara o processo e suas threads para rodar
        kernel.pcb_array[i].state = STATE_READY;
        for (int j = 0; j < kernel.pcb_array[i].num_threads; j++) {
            kernel.pcb_array[i].tcb_array[j].state = STATE_READY;
            kernel.pcb_array[i].tcb_array[j].pcb = &kernel.pcb_array[i];
            kernel.pcb_array[i].tcb_array[j].remaining_time = 
                kernel.pcb_array[i].process_len / kernel.pcb_array[i].num_threads;
        }

        // Coloca na fila de prontos e sinaliza os processadores que estão em cond_wait
        queue_push_back(&kernel.ready_queue, &kernel.pcb_array[i]);
        pthread_cond_broadcast(&kernel.scheduler_cv);

        pthread_mutex_unlock(&kernel.kernel_mutex);
    }

    // Informa que todos os processos do arquivo de entrada já foram despachados
    pthread_mutex_lock(&kernel.kernel_mutex);
    kernel.generator_done = 1;
    pthread_cond_broadcast(&kernel.scheduler_cv); // Acorda CPUs ociosas caso estejam esperando
    pthread_mutex_unlock(&kernel.kernel_mutex);

    // Aguarda o término do(s) escalonador(es)
    pthread_join(scheduler_cpu1, NULL);
    #ifdef VERSION_MULTIPROCESSOR
        pthread_join(scheduler_cpu2, NULL);
    #endif

    // Escrita do Log final conforme Seção 4.2
    FILE *log_file = fopen(kernel.log_file, "a");
    if (log_file) {
        fprintf(log_file, "Escalonador terminou execução de todos processos\n");
        fclose(log_file);
    }

    // Limpeza absoluta da memória
    queue_destroy(&kernel.ready_queue);
    for (int i = 0; i < kernel.process_count; i++) {
        free(kernel.pcb_array[i].tcb_array);
    }
    free(kernel.pcb_array);
    pthread_mutex_destroy(&kernel.kernel_mutex);
    pthread_cond_destroy(&kernel.scheduler_cv);

    free(cpu0);
    free(cpu1);

    return EXIT_SUCCESS;
}