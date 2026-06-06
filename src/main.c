#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "miniKernel.h"

int main(int argc, char *argv[]) {
    if (argc < 2) return EXIT_FAILURE;

    FILE *entry_file = fopen(argv[1], "r");
    if (!entry_file) return EXIT_FAILURE;

    // Inicializa a instância central do nosso sistema
    MiniKernel kernel;
    kernel.generator_done = 0;
    pthread_mutex_init(&kernel.kernel_mutex, NULL);
    pthread_cond_init(&kernel.scheduler_cv, NULL);
    queue_init(&kernel.ready_queue);

    if (fscanf(entry_file, "%d", &kernel.process_count) != 1) {
        fclose(entry_file);
        return EXIT_FAILURE;
    }

    kernel.pcb_list = (PCB*) malloc(kernel.process_count * sizeof(PCB));

    for (int i = 0; i < kernel.process_count; i++) {
        kernel.pcb_list[i].pid = i + 1;
        fscanf(entry_file, "%d", &kernel.pcb_list[i].process_len);
        fscanf(entry_file, "%d", &kernel.pcb_list[i].priority);
        fscanf(entry_file, "%d", &kernel.pcb_list[i].num_threads);
        fscanf(entry_file, "%d", &kernel.pcb_list[i].start_time);
        
        kernel.pcb_list[i].remaining_time = kernel.pcb_list[i].process_len;
        kernel.pcb_list[i].state = STATE_READY;
        
        pthread_mutex_init(&kernel.pcb_list[i].mutex, NULL);
        pthread_cond_init(&kernel.pcb_list[i].cv, NULL);
        kernel.pcb_list[i].thread_ids = (pthread_t*) malloc(kernel.pcb_list[i].num_threads * sizeof(pthread_t));
    }

    int policy_code;
    fscanf(entry_file, "%d", &policy_code);
    kernel.policy = (SchedulerPolicy)policy_code;
    fclose(entry_file);

    // Identifica qual versão compilar baseado na flag do Makefile (-DVERSION_MONOPROCESSOR)
    pthread_t scheduler_cpu1, scheduler_cpu2;
    
    #ifdef VERSION_MULTIPROCESSOR
        // Cria duas threads escalonadoras simulando 2 CPUs (45% da nota!)
        pthread_create(&scheduler_cpu1, NULL, scheduler_multiprocessador, &kernel);
        pthread_create(&scheduler_cpu2, NULL, scheduler_multiprocessador, &kernel);
    #else
        // Cria apenas uma thread escalonadora (Monoprocessador)
        pthread_create(&scheduler_cpu1, NULL, scheduler_monoprocessador, &kernel);
    #endif

    // --- TODO: Lógica de Simulação de Tempo Real (Seção 2.2) ---
    // Aguardar o start_time de cada processo e dar o disparo das suas respectivas threads.

    // Aguarda o término do(s) escalonador(es)
    pthread_join(scheduler_cpu1, NULL);
    #ifdef VERSION_MULTIPROCESSOR
        pthread_join(scheduler_cpu2, NULL);
    #endif

    // Escrita do Log conforme Seção 4.2
    FILE *log_file = fopen("log_execucao_minikernel.txt", "w");
    if (log_file) {
        // A lógica do escalonador preencherá um buffer interno que será descarregado aqui
        fprintf(log_file, "Escalonador terminou execução de todos processos\n");
        fclose(log_file);
    }

    // Limpeza absoluta da memória (Proteção total contra descontos do Valgrind)
    for (int i = 0; i < kernel.process_count; i++) {
        free(kernel.pcb_list[i].thread_ids);
        pthread_mutex_destroy(&kernel.pcb_list[i].mutex);
        pthread_cond_destroy(&kernel.pcb_list[i].cv);
    }
    free(kernel.pcb_list);
    queue_destroy(&kernel.ready_queue);
    pthread_mutex_destroy(&kernel.kernel_mutex);
    pthread_cond_destroy(&kernel.scheduler_cv);

    return EXIT_SUCCESS;
}

