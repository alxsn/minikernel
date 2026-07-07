#include "minikernel.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"

void* scheduler_monoprocessador(void *arg) {
    Processador *proc = (Processador*) arg;
    MiniKernel *kernel = proc->kernel;
    int cpu_id = 0; 

    printf("[MINIKERNEL] Inicializado Processador %d (Mono) com sucesso.\n", cpu_id);

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
                // Pega a primeira thread pronta deste processo
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

        // Controle para printar a execução do processo apenas uma vez no FCFS/PRIO
        int passou_primeira_vez = 0;

        // Executa todas as threads deste processo sequencialmente se for FCFS/PRIO
        while (thread != NULL) {
            
            // Registra a thread no slot da CPU 0
            kernel->current_thread_cpu[cpu_id] = thread;
            thread->state = STATE_RUNNING;

            int quantum = kernel->quantum;
            const char* policy_name = (kernel->policy == 1) ? "FCFS" : (kernel->policy == 2) ? "RR" : "PRIORITY";

            // Só grava o log de execução se for a primeira thread do processo OU se for Round Robin
            if (kernel->policy == 2 || passou_primeira_vez == 0) {
                FILE *log_file = fopen(kernel->log_file, "a");
                if (log_file) {
                    if (kernel->policy == 2) { 
                        fprintf(log_file, "[%s] Executando processo PID %d com quantum %dms\n", policy_name, processo->pid, quantum);
                    } else {
                        fprintf(log_file, "[%s] Executando processo PID %d\n", policy_name, processo->pid);
                    }
                    fclose(log_file);
                }
                passou_primeira_vez = 1; // Garante que não printará nas próximas threads do mesmo processo (FCFS/PRIO)
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

            // Limpa o slot para a próxima avaliação
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
                thread = NULL; // Quebra o laço interno deste processo
            } 
            // Se ainda restam threads prontas
            else if (n_threads_running == 0 && n_threads_ready > 0) {
                if (kernel->policy == POLICY_FCFS || kernel->policy == POLICY_PRIORITY) {
                    // FCFS/PRIO: Busca imediatamente a próxima thread PRONTA do próprio processo
                    thread = NULL;
                    for (int i = 0; i < processo->num_threads; i++) {
                        if (processo->tcb_array[i].state == STATE_READY) {
                            thread = &processo->tcb_array[i];
                            break;
                        }
                    }
                } else {
                    // Round Robin: Devolve o processo para o fim da fila e cede o processador
                    queue_push_back(&kernel->ready_queue, processo);
                    thread = NULL; // Quebra o laço para pegar o próximo da fila global
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

    printf("[MINIKERNEL] Inicializado Processador %d com sucesso.\n", cpu_id);

    while (1) {
        pthread_mutex_lock(&kernel->kernel_mutex);

        PCB *processo = NULL;
        TCB *thread = NULL;

        // STEP 1: Olha a fila. Tem? Se sim, pega.
        if (kernel->ready_queue.size > 0) {
            if (kernel->policy == POLICY_PRIORITY) {
                processo = queue_remove_highest_priority(&kernel->ready_queue);
            } else {
                processo = queue_pop_front(&kernel->ready_queue);
            }

            if (processo != NULL) {
                // Pega a thread que ainda não rodou
                for(int i = 0; i < processo->num_threads; i++){
                    if(processo->tcb_array[i].state == STATE_READY){
                        thread = &processo->tcb_array[i];
                        break;
                    }
                }  
            }
        }

        // STEP 2: Se não, olha o processo associado a thread do outro processador.
        // Tem thread sobrando nele? Se sim, pega.
        if (processo == NULL && kernel->current_thread_cpu[outra_cpu] != NULL) {
            PCB *processo_vizinho = kernel->current_thread_cpu[outra_cpu]->pcb;
            if (processo_vizinho != NULL) {
                // Tem thread sobrando nele? Se sim, pega.
                for(int i = 0; i < processo_vizinho->num_threads; i++){
                    if(processo_vizinho->tcb_array[i].state == STATE_READY){
                        processo = processo_vizinho;
                        thread = &processo_vizinho->tcb_array[i];
                        break;
                    }
                }                
            }
        }

        // Se a CPU está ociosa
        if (processo == NULL || thread == NULL) {
            // STEP 3: Só termina se o gerador acabou, a fila está zerada E nenhuma CPU está executando threads pendentes
            if (kernel->generator_done && kernel->ready_queue.size == 0 &&
                kernel->current_thread_cpu[0] == NULL && kernel->current_thread_cpu[1] == NULL) {
                
                pthread_cond_broadcast(&kernel->scheduler_cv); // Garante acordar a outra CPU para finalizar também
                pthread_mutex_unlock(&kernel->kernel_mutex);
                break; // Fim da simulação para esta CPU
            }

            // STEP 4: Se ainda há trabalho sendo feito por outra CPU, espera uma notificação
            pthread_cond_wait(&kernel->scheduler_cv, &kernel->kernel_mutex);
            pthread_mutex_unlock(&kernel->kernel_mutex);
            continue; 
        }

        // 5. Ocupa com a thread o slot deste processador no Kernel
        kernel->current_thread_cpu[cpu_id] = thread;
        thread->state = STATE_RUNNING;

        int quantum = kernel->quantum;
        const char* policy_name = (kernel->policy == 1) ? "FCFS" : (kernel->policy == 2) ? "RR" : "PRIO";

        // Escrita do Log modificada para abarcar a string do quantum caso seja Round Robin
        FILE *log_file = fopen(kernel->log_file, "a");
        if (log_file) {
            if (kernel->policy == 2) { // POLICY_RR
                fprintf(log_file, "[%s] Executando processo PID %d com quantum %dms // processador %d\n", policy_name, processo->pid, quantum, cpu_id);
            } else {
                fprintf(log_file, "[%s] Executando processo PID %d // processador %d\n", policy_name, processo->pid, cpu_id);
            }
            fclose(log_file);
        }

        pthread_mutex_unlock(&kernel->kernel_mutex);

        // --- Simulação da Execução da Thread do Processo ---
        int sleeping_time;

        // Calculo do tempo que a thread "rodará"
        if (kernel->policy == POLICY_FCFS || kernel->policy == POLICY_PRIORITY) {
            sleeping_time = thread->remaining_time;
        } else { // POLICY_RR
            sleeping_time = kernel->quantum;
        }

        // "Rodando" pelo tempo estipulado (garante o uso do quantum cheio no RR)
        usleep(sleeping_time);

        pthread_mutex_lock(&kernel->kernel_mutex);

        // Subtrai o tempo real que ela precisava para não gerar número negativo
        if (kernel->policy == POLICY_RR && thread->remaining_time < sleeping_time) {
            thread->remaining_time = 0;
        } else {
            thread->remaining_time = thread->remaining_time - sleeping_time;
        }

        // Verifica finalização da thread
        if (thread->remaining_time <= 0) {
            thread->state = STATE_FINISHED;
        } else { // Fica disponivel para rodar novamente
            thread->state = STATE_READY;
        }

        // ----- Estado do processo -------
        int n_threads_ready = 0, n_threads_running = 0, n_threads_finished = 0;
        
        for(int i = 0; i < processo->num_threads; i++){
            if(processo->tcb_array[i].state == STATE_READY) n_threads_ready += 1;
            if(processo->tcb_array[i].state == STATE_RUNNING) n_threads_running += 1;
            if(processo->tcb_array[i].state == STATE_FINISHED) n_threads_finished += 1;
        }

        if(n_threads_running == 0 && n_threads_ready > 0){// Volta pra fila de prontos
            queue_push_back(&kernel->ready_queue, processo);
            pthread_cond_broadcast(&kernel->scheduler_cv);
        }
        else if(n_threads_finished == processo->num_threads){// Processo terminou
            processo->state = STATE_FINISHED;

            // Escrita do Log conforme Seção 4.2 (Usando estritamente log_file)
            log_file = fopen(kernel->log_file, "a");
            if (log_file) {
                fprintf(log_file, "[%s] Processo PID %d finalizado\n", policy_name, processo->pid);
                fclose(log_file);
            }
        }
        
        // 6. Desaloca o slot do processador antes do próximo ciclo
        kernel->current_thread_cpu[cpu_id] = NULL;
        pthread_cond_broadcast(&kernel->scheduler_cv);
        pthread_mutex_unlock(&kernel->kernel_mutex);
    }

    return NULL;
}