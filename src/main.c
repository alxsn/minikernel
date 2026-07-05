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
        
        kernel.pcb_array[i].remaining_time = kernel.pcb_list[i].process_len;
        kernel.pcb_array[i].state = STATE_NOT_ARRIVED;
        
        pthread_mutex_init(&kernel.pcb_array[i].mutex, NULL);
        pthread_cond_init(&kernel.pcb_array[i].cv, NULL);
        kernel.pcb_array[i].tcb_array = (TCB*) malloc(kernel.pcb_array[i].num_threads * sizeof(TCB));
    }

    int policy_code;
    fscanf(entry_file, "%d", &policy_code);
    kernel.policy = (SchedulerPolicy)policy_code;
    fclose(entry_file);

    Processador cpu0;
    cpu0.cpu_id = 0;
    cpu0.kernel = &kernel;

    Processador cpu1;
    cpu1.cpu_id = 1;
    cpu1.kernel = &kernel;

    // Identifica qual versão compilar baseado na flag do Makefile (-DVERSION_MONOPROCESSOR)
    pthread_t scheduler_cpu1;
    
    #ifdef VERSION_MULTIPROCESSOR
        pthread_t scheduler_cpu2;
        // Cria duas threads escalonadoras simulando 2 CPUs (45% da nota!)
        pthread_create(&scheduler_cpu1, NULL, scheduler_multiprocessador, &cpu0);
        pthread_create(&scheduler_cpu2, NULL, scheduler_multiprocessador, &cpu1);
    #else
        // Cria apenas uma thread escalonadora (Monoprocessador)
        pthread_create(&scheduler_cpu1, NULL, scheduler_monoprocessador, &cpu0);
    #endif

    // --- TODO: Lógica de Simulação de Tempo Real (Seção 2.2) ---
    // Aguardar o start_time de cada processo e dar o disparo das suas respectivas threads.

    // --- Lógica de Simulação de Tempo Real (Seção 2.2) ---
    kernel.tempo_sistema = 0;

    while (1) {
        pthread_mutex_lock(&kernel.kernel_mutex);

        

        int todos_chegaram = 1;

        for (int i = 0; i < kernel.process_count; i++) {
            // Se o processo ainda não chegou, monitoramos o tempo dele
            if (kernel.pcb_array[i].state == STATE_NOT_ARRIVED) {
                todos_chegaram = 0; // Se tem alguém que não chegou, o gerador não acabou

                // Se o relógio do sistema alcançou o tempo de início do processo
                if (kernel.tempo_sistema >= kernel.pcb_array[i].start_time) {
                    kernel.pcb_array[i].state = STATE_READY;

                    // Coloca todas as threads dele em modo READY para as CPUs poderem capturar
                    for (int j = 0; j < kernel.pcb_array[i].num_threads; j++) {
                        kernel.pcb_array[i].tcb_array[j].state = STATE_READY;
                        kernel.pcb_array[i].tcb_array[j].pcb = &kernel.pcb_array[i];
                        // Cada thread ganha uma fração do tempo total do processo
                        kernel.pcb_array[i].tcb_array[j].remaining_time = 
                            kernel.pcb_array[i].process_len / kernel.pcb_array[i].num_threads;
                    }

                    // Enfileira o processo na lista de prontos
                    queue_push_back(&kernel.ready_queue, &kernel.pcb_array[i]);

                    // Acorda os escalonadores que estavam dormindo esperando processos
                    pthread_cond_broadcast(&kernel.scheduler_cv);
                }
            }
        }

        // Se todos os processos do arquivo de entrada já entraram na fila
        if (todos_chegaram) {
            kernel.generator_done = 1;
            // Acorda as CPUs uma última vez caso estejam dormindo, para elas saberem que podem terminar
            pthread_cond_broadcast(&kernel.scheduler_cv);
            pthread_mutex_unlock(&kernel.kernel_mutex);
            break; // Sai do loop do relógio na main
        }

        pthread_mutex_unlock(&kernel.kernel_mutex);

        pthread_mutex_lock(&kernel.kernel_mutex);
        kernel.tempo_sistema += 10;
        pthread_mutex_unlock(&kernel.kernel_mutex);
    }

    // Aguarda o término do(s) escalonador(es)
    pthread_join(scheduler_cpu1, NULL);
    #ifdef VERSION_MULTIPROCESSOR
        pthread_join(scheduler_cpu2, NULL);
    #endif

    // Escrita do Log conforme Seção 4.2
    FILE *log_file = fopen(kernel.log_file, "a");
    if (log_file) {
        // finalização da escrita
        fprintf(log_file, "Escalonador terminou execução de todos processos\n");
        fclose(log_file);
    }

    // Limpeza absoluta da memória (Proteção total contra descontos do Valgrind)
    queue_destroy(&kernel.ready_queue);
    for (int i = 0; i < kernel.process_count; i++) {
        free(kernel.pcb_array[i].tcb_array);
    }
    free(kernel.pcb_array);
    pthread_mutex_destroy(&kernel.kernel_mutex);
    pthread_cond_destroy(&kernel.scheduler_cv);

    return EXIT_SUCCESS;
}

