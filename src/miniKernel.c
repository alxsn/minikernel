#include "minikernel.h"
#include "stdlib.h"

void* thread_pipeline(void *arg) {
    TCB *tcb = (TCB*) arg;
    // ... lógica das threads simuladas ...
    free(tcb);
    return NULL;
}

void* scheduler_monoprocessador(void *arg) {
    MiniKernel *kernel = (MiniKernel*) arg;
    // Lógica do Escalonador Mono
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

            // Pega a thread que ainda não rodou
            for(int i = 0; i < processo->num_threads; i++){
                if(processo->tcb_array[i].state == STATE_READY){
                    thread = &processo->tcb_array[i];
                }
            }  
        }

        // STEP 2: Se não, olha o processo associado a thread do outro processador.
        // Tem thread sobrando nele? Se sim, pega.
        if (processo == NULL) {
            PCB *processo_vizinho = kernel->current_thread_cpu[outra_cpu]->pcb;
            if (processo_vizinho != NULL) {
                // Tem thread sobrando nele? Se sim, pega.
                for(int i = 0; i < processo_vizinho->num_threads; i++){
                    if(processo_vizinho->tcb_array[i].state == STATE_READY){
                        processo = processo_vizinho;
                        thread = &processo->tcb_array[i];
                    }
                }                
            }
        }

        // Se conseguimos um processo (seja da fila ou por cooperação), pulamos a espera
        if (processo == NULL) {
            // STEP 3: Se não, olha se todos os processos já chegaram. Se sim, esse processador termina.
            int n_processos_not_arrived = 0;
            for(int i = 0; i < kernel->process_count; i++){
                if(kernel->pcb_array[i].state == STATE_NOT_ARRIVED) n_processos_not_arrived +=1;
            }
            if (n_processos_not_arrived == 0) {
                pthread_mutex_unlock(&kernel->kernel_mutex);
                break; // Fim da simulação para esta CPU
            }

            // STEP 4: Se não chegaram todos, se bloqueia na variável de condição
            // Ao acordar, o 'while(1)' reinicia o fluxo inteiro (olha fila, olha vizinho...)
            pthread_cond_wait(&kernel->scheduler_cv, &kernel->kernel_mutex);
            pthread_mutex_unlock(&kernel->kernel_mutex);
            continue; 
        }

        // 5. Ocupa com a thread o slot deste processador no Kernel
        kernel->current_thread_cpu[cpu_id] = thread;

        thread->state = STATE_RUNNING;

        int quantum = kernel->quantum;

        // IMPRESSÃO EXATA DO ARQUIVO DE TESTE
        // Escrita do Log conforme Seção 4.2
        FILE *log_file = fopen(kernel->log_file, "a");
        if (log_file) {
            fprintf(log_file, "[%s] Executando processo PID %d // processador %d\n", policy_name, processo->pid, cpu_id);
            fclose(log_file);
        }

        pthread_mutex_unlock(&kernel->kernel_mutex);

        // --- Simulação da Execução da Thread do Processo ---
        // Como todo o processo de pegar a thread foi feito sob mutex, não é necessario manter
        // a trava aqui, pois se tem a certeza de a thread estar em um unico processador por vez
        
        const char* policy_name = (kernel->policy == 1) ? "FCFS" : (kernel->policy == 2) ? "RR" : "PRIO";

        int sleeping_time;

        // Calculo do tempo que a thread "rodará"
        if(kernel->policy == POLICY_FCFS || kernel->policy == POLICY_PRIORITY){
            sleeping_time = thread->remaining_time;
        }else{// POLICY_RR
            sleeping_time = quantum;
        }

        // "Rodando"
        usleep(sleeping_time);

        pthread_mutex_lock(&kernel_mutex);

        thread->remaining_time = thread->remaining_time - sleeping_time;

        // Verifica finalização da thread
        if (thread->remaining_time <= 0) {
            thread->state = STATE_FINISHED;
        }else{// Fica disponivel para rodar novamente
            thread->state = STATE_READY;
        }

        // ----- Estado do processo -------
        // Verifica estado das threads para decidir o estado do processo
        // decide se o processo é STATE_FINISHED ou
        // o recoloca na fila de prontos ou não faz nada caso esteja rodando no outro processador
        // n_threads_running > 0 - não faz nada
        // n_threads_running == 0 && n_threads_ready > 0 - recoloca na fila de prontos
        // n_threads_finished == num_threads - STATE_FINISHED

        int n_threads_ready = 0, n_threads_running = 0, n_threads_finished = 0;
        
        for(int i = 0; i < processo->num_threads; i++){
            if(processo->tcb_array[i].state == STATE_READY) n_threads_ready += 1;
            if(processo->tcb_array[i].state == STATE_RUNNING) n_threads_running += 1;
            if(processo->tcb_array[i].state == STATE_FINISHED) n_threads_finished += 1;
        }

        if(n_threads_running == 0 && n_threads_ready > 0){// Volta pra fila de prontos
            queue_push_back(kernel->ready_queue, processo);
        }
        else if(n_threads_finished == processo->num_threads){// Processo terminou
            processo->state = STATE_FINISHED;

            // Escrita do Log conforme Seção 4.2
            FILE *log_file = fopen(kernel->log_file, "a");
            if (log_file) {
                fprintf(log_file, "[%s] Processo PID %d finalizado\n", policy_name, processo->pid);
                fclose(log_file);
            }
        }
        //else - n_threads_running > 0 - não faz nada
        
        // 6. Desaloca o slot do processador antes do próximo ciclo
        kernel->current_process_cpu[cpu_id] = NULL;

        pthread_mutex_unlock(&kernel->kernel_mutex);
    }

    return NULL;
}