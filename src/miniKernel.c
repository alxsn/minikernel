#include "miniKernel.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

void* scheduler_monoprocessador(void *arg) {
    Processador *proc = (Processador*) arg;
    MiniKernel *kernel = proc->kernel;
    int cpu_id = 0; 

    while (1) {
        pthread_mutex_lock(&kernel->kernel_mutex);

        PCB *processo = NULL;
        TCB *thread = NULL;

        // STEP 1: Busca o próximo processo da fila
        if (kernel->ready_queue.size > 0) {
            if (kernel->policy == POLICY_PRIORITY) {
                processo = queue_remove_highest_priority(&kernel->ready_queue);
            } else {
                processo = queue_pop_front(&kernel->ready_queue);
            }

            if (processo != NULL) {
                for (int i = 0; i < processo->num_threads; i++) {
                    if (processo->tcb_array[i].state == STATE_READY) {
                        thread = &processo->tcb_array[i];
                        break;
                    }
                }  
            }
        }

        // Se a CPU está ociosa
        if (processo == NULL || thread == NULL) {
            if (kernel->generator_done && kernel->ready_queue.size == 0 &&
                kernel->current_thread_cpu[0] == NULL) {
                
                pthread_mutex_unlock(&kernel->kernel_mutex);
                break; 
            }

            pthread_cond_wait(&kernel->scheduler_cv, &kernel->kernel_mutex);
            pthread_mutex_unlock(&kernel->kernel_mutex);
            continue; 
        }

        int passou_primeira_vez = 0;
        int quantum = kernel->quantum;
        const char* policy_name = (kernel->policy == 1) ? "FCFS" : (kernel->policy == 2) ? "RR" : "PRIORITY";

        // Laço de execução: Se for FCFS, roda todas as threads seguidas. 
        // Se for RR ou PRIORITY, o loop roda apenas uma vez (graças ao break no final)
        while (thread != NULL) {
            
            // Registra a thread no slot da CPU 0
            kernel->current_thread_cpu[cpu_id] = thread;
            thread->state = STATE_RUNNING;

            // Controle estrito de Log
            if (kernel->policy == POLICY_RR) { 
                FILE *log_file = fopen(kernel->log_file, "a");
                if (log_file) {
                    fprintf(log_file, "[%s] Executando processo PID %d com quantum %dms\n", policy_name, processo->pid, quantum);
                    fclose(log_file);
                }
            } else if (kernel->policy == POLICY_PRIORITY) {
                FILE *log_file = fopen(kernel->log_file, "a");
                if (log_file) {
                    fprintf(log_file, "[%s] Executando processo PID %d prioridade %d\n", policy_name, processo->pid, processo->priority);
                    fclose(log_file);
                }
            } else if (kernel->policy == POLICY_FCFS && passou_primeira_vez == 0) {
                // FCFS printa uma única vez por processo inteiro
                FILE *log_file = fopen(kernel->log_file, "a");
                if (log_file) {
                    fprintf(log_file, "[%s] Executando processo PID %d\n", policy_name, processo->pid);
                    fclose(log_file);
                }
                passou_primeira_vez = 1;
            }

            pthread_mutex_unlock(&kernel->kernel_mutex);

            // --- Simulação do Tempo de Execução ---
            int sleeping_time;
            if (kernel->policy == POLICY_FCFS || kernel->policy == POLICY_PRIORITY) {
                sleeping_time = thread->remaining_time;
            } else { 
                sleeping_time = (thread->remaining_time > quantum) ? quantum : thread->remaining_time;
            }

            usleep(sleeping_time);

            pthread_mutex_lock(&kernel->kernel_mutex);

            thread->remaining_time = thread->remaining_time - sleeping_time;

            if (thread->remaining_time <= 0) {
                thread->state = STATE_FINISHED;
            } else {
                thread->state = STATE_READY;
            }

            // Limpa o slot da CPU
            kernel->current_thread_cpu[cpu_id] = NULL;

            // ----- Avaliação do Estado do Processo -----
            int n_threads_ready = 0, n_threads_running = 0, n_threads_finished = 0;
            for (int i = 0; i < processo->num_threads; i++) {
                if (processo->tcb_array[i].state == STATE_READY) n_threads_ready += 1;
                if (processo->tcb_array[i].state == STATE_RUNNING) n_threads_running += 1;
                if (processo->tcb_array[i].state == STATE_FINISHED) n_threads_finished += 1;
            }

            // Caso o processo tenha terminado por completo
            if (n_threads_finished == processo->num_threads) {
                processo->state = STATE_FINISHED;

                FILE *log_file = fopen(kernel->log_file, "a");
                if (log_file) {
                    fprintf(log_file, "[%s] Processo PID %d finalizado\n", policy_name, processo->pid);
                    fclose(log_file);
                }
                thread = NULL; // Termina o laço interno
            } 
            // Se ainda restam threads prontas neste processo
            else if (n_threads_ready > 0) {
                if (kernel->policy == POLICY_FCFS) {
                    // FCFS: Pega imediatamente a próxima thread PRONTA do mesmo processo sem soltar a CPU
                    thread = NULL;
                    for (int i = 0; i < processo->num_threads; i++) {
                        if (processo->tcb_array[i].state == STATE_READY) {
                            thread = &processo->tcb_array[i];
                            break;
                        }
                    }
                } else {
                    // RR e PRIORITY: Devolvem o processo à fila global e liberam a CPU
                    queue_push_back(&kernel->ready_queue, processo);
                    thread = NULL; // Quebra o laço interno para reavaliar no while(1)
                }
            } else {
                thread = NULL;
            }
        }

        pthread_cond_broadcast(&kernel->scheduler_cv);
        pthread_mutex_unlock(&kernel->kernel_mutex);
    }

    return NULL;
}



//escalonador multiprocessador
void* scheduler_multiprocessador(void *arg) {
    Processador *proc = (Processador*) arg;
    MiniKernel *kernel = proc->kernel;
    int cpu_id = proc->cpu_id;          
    int outra_cpu = 1 - cpu_id;         

    while (1) {
        pthread_mutex_lock(&kernel->kernel_mutex);

        PCB *processo = NULL;
        TCB *thread = NULL;

        // 1. OLHA A FILA GLOBAL: Tem processo? Pega.
        if (kernel->ready_queue.size > 0) {
            if (kernel->policy == POLICY_PRIORITY) {
                processo = queue_remove_highest_priority(&kernel->ready_queue);
            } else {
                processo = queue_pop_front(&kernel->ready_queue);
            }

            if (processo != NULL) {
                for (int i = 0; i < processo->num_threads; i++) {
                    if (processo->tcb_array[i].state == STATE_READY) {
                        thread = &processo->tcb_array[i];
                        break;
                    }
                }  
            }
        }

        // 2. NÃO TEM NA FILA: Olha o outro processador e vê se tem thread READY para roubar (Work Stealing)
        if (processo == NULL && kernel->current_thread_cpu[outra_cpu] != NULL) {
            PCB *processo_vizinho = kernel->current_thread_cpu[outra_cpu]->pcb;
            if (processo_vizinho != NULL) {
                for (int i = 0; i < processo_vizinho->num_threads; i++) {
                    if (processo_vizinho->tcb_array[i].state == STATE_READY) {
                        processo = processo_vizinho;
                        thread = &processo_vizinho->tcb_array[i];
                        break;
                    }
                }                
            }
        }

        // 3. NÃO TEM EM LUGAR NENHUM: Dorme (ou encerra se tudo acabou)
        if (processo == NULL || thread == NULL) {
            if (kernel->generator_done && kernel->ready_queue.size == 0 &&
                kernel->current_thread_cpu[0] == NULL && kernel->current_thread_cpu[1] == NULL) {
                
                pthread_cond_broadcast(&kernel->scheduler_cv); 
                pthread_mutex_unlock(&kernel->kernel_mutex);
                break; 
            }

            pthread_cond_wait(&kernel->scheduler_cv, &kernel->kernel_mutex);
            pthread_mutex_unlock(&kernel->kernel_mutex);
            continue; 
        }

        // Aloca o slot da CPU e altera o estado da thread antes de liberar o mutex
        kernel->current_thread_cpu[cpu_id] = thread;
        thread->state = STATE_RUNNING;

        int quantum = kernel->quantum;
        const char* policy_name = (kernel->policy == 1) ? "FCFS" : (kernel->policy == 2) ? "RR" : "PRIO";

        // --- Escrita Direta do Log ---
        FILE *log_file = fopen(kernel->log_file, "a");
        if (log_file) {
            if (kernel->policy == 2) { // RR
                fprintf(log_file, "[%s] Executando processo PID %d com quantum %dms // processador %d\n", policy_name, processo->pid, quantum, cpu_id);
            } else if (kernel->policy == 3) { // PRIORITY
                fprintf(log_file, "[%s] Executando processo PID %d prioridade %d // processador %d\n", policy_name, processo->pid, processo->priority, cpu_id);
            } else if (kernel->policy == 1) { // FCFS
                // Print plano: pegou a thread para rodar no FCFS, gera o log da CPU correspondente
                fprintf(log_file, "[%s] Executando processo PID %d // processador %d\n", policy_name, processo->pid, cpu_id);
            }
            fclose(log_file);
        }

        pthread_mutex_unlock(&kernel->kernel_mutex);

        // --- Simulação da Execução da Thread ---
        int sleeping_time;
        if (kernel->policy == POLICY_FCFS || kernel->policy == POLICY_PRIORITY) {
            sleeping_time = thread->remaining_time;
        } else { 
            sleeping_time = (thread->remaining_time > quantum) ? quantum : thread->remaining_time;
        }

        usleep(sleeping_time);

        pthread_mutex_lock(&kernel->kernel_mutex);

        thread->remaining_time = thread->remaining_time - sleeping_time;

        if (thread->remaining_time <= 0) {
            thread->state = STATE_FINISHED;
        } else { 
            thread->state = STATE_READY;
        }

        // Libera o slot da CPU
        kernel->current_thread_cpu[cpu_id] = NULL;

        // ----- Avaliação de Término e Retorno à Fila -----
        int n_threads_ready = 0, n_threads_running = 0, n_threads_finished = 0;
        for (int i = 0; i < processo->num_threads; i++) {
            if (processo->tcb_array[i].state == STATE_READY) n_threads_ready += 1;
            if (processo->tcb_array[i].state == STATE_RUNNING) n_threads_running += 1;
            if (processo->tcb_array[i].state == STATE_FINISHED) n_threads_finished += 1;
        }

        if (n_threads_finished == processo->num_threads) {
            processo->state = STATE_FINISHED;

            FILE *log_file = fopen(kernel->log_file, "a");
            if (log_file) {
                fprintf(log_file, "[%s] Processo PID %d finalizado\n", policy_name, processo->pid);
                fclose(log_file);
            }
        }
        // Se ainda restam threads prontas e NENHUMA outra CPU está tocando esse processo atualmente
        else if (n_threads_ready > 0 && n_threads_running == 0) {
            queue_push_back(&kernel->ready_queue, processo);
        }
        
        pthread_cond_broadcast(&kernel->scheduler_cv);
        pthread_mutex_unlock(&kernel->kernel_mutex);
    }

    return NULL;
}