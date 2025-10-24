#include "dine.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>    
#include <string.h>     
#include <time.h>     
#include <sys/time.h>
#include <errno.h>

// Global declarations
sem_t forkArray[NUM_PHILOSOPHERS];
phil philArray[NUM_PHILOSOPHERS];
sem_t status_mutex; // Mutex for synchronized status printing and state update
long eat_think_cycles = 1; // Number of cycles to run (default 1)

// --- Utility Functions ---

void dawdle() {
    struct timespec tv;
    int msec = (int)((((double)random()) / RAND_MAX) * DAWDLEFACTOR);
    
    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec; 
    
    if (-1 == nanosleep(&tv, NULL)) {
        if (errno != EINTR) { 
             perror("nanosleep");
        }
    }
}

// Executes P(s) or DOWN(s) operation (sem_wait).
 
void lock(sem_t *s_ptr) {
    if (sem_wait(s_ptr) != 0) {
        perror("Semaphore wait failed");
        exit(EXIT_FAILURE);
    }
}

// Executes V(s) or UP(s) operation (sem_post).
 
void unlock(sem_t *s_ptr) {
    if (sem_post(s_ptr) != 0) {
        perror("Semaphore post failed");
        exit(EXIT_FAILURE);
    }
}

// --- Printing Functions ---

void printHeader() {
    int i;
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("=============|");
    }
    printf("\n");
    
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("      %c      |", 'A' + i);
    }
    printf("\n");
    
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("=============|");
    }
    printf("\n");
}

/**
 * Prints a status line showing the state and forks held by all philosophers.
 * The status_mutex must be held by the caller when this function is invoked.
 */
void printStatus(int changing_phil_id, const char *change_desc) {
    int i, j;
    // Caller holds status_mutex, no internal locking/unlocking.

    // for(k = 0; k < NUM_PHILOSOPHERS; k++){
        
        for(i = 0; i < NUM_PHILOSOPHERS; i++) {
            phil *p = &philArray[i];
            printf("| ");
            for(j = 0; j < NUM_PHILOSOPHERS; j++){
                if((p->lfork_idx == j) && p->holding_lfork){
                    // if pos = 1.5, lfork = 1
                    printf("%d", p->lfork_idx);
                }
                else if((p->rfork_idx == j) && p->holding_rfork){
                    // if pos = 1.5, lfork = 1
                    printf("%d", p->rfork_idx);
                }
                else{
                    printf("-");
                }
            }
            
            if(p->status == EAT){
                printf(" EAT   ");
            }
            else if(p->status == THINK){
                printf(" THINK ");
            }
            else{
                printf("       ");
            }

            // printf("|");
        }
        
    printf("|\n");
    // printf("| Change: P%c %s\n", 'A' + changing_phil_id, change_desc); 
}

// --- Main Philosopher Logic ---

void *phil_cycle(void *arg) {
    phil *p = (phil *)arg;
    int phil_id = p->id;
    
    int lfork = p->lfork_idx;
    int rfork = p->rfork_idx;
    
    sem_t *first_sem, *second_sem;
    int *first_flag, *second_flag;
    // strings used for status debugging
    const char *first_name, *second_name;

    if (phil_id % 2 == 0) { // Even ID: Right fork (rfork) first
        first_sem = &forkArray[rfork];
        second_sem = &forkArray[lfork];
        first_flag = &p->holding_rfork;
        second_flag = &p->holding_lfork;
        first_name = "picks up R fork";
        second_name = "picks up L fork";
    } else { // Odd ID: Left fork (lfork) first
        first_sem = &forkArray[lfork];
        second_sem = &forkArray[rfork];
        first_flag = &p->holding_lfork;
        second_flag = &p->holding_rfork;
        first_name = "picks up L fork";
        second_name = "picks up R fork";
    }

    // Initial state: Start out hungry in CHANGING state
    // Must lock status mutex before printing so no other process prints
    lock(&status_mutex);
    printStatus(phil_id, "starts hungry (CHANGING)"); 
    unlock(&status_mutex);

    while (p->cycles_left > 0) {
        
        // 1. Thinking Period
        lock(&status_mutex);
        p->status = THINK;
        printStatus(phil_id, "changed state to THINK");
        unlock(&status_mutex);
        dawdle();
        
        // 2. Transitioning to Eat (CHANGING)
        lock(&status_mutex);
        p->status = CHANGING;
        printStatus(phil_id, "changed state to CHANGING (hungry)");
        unlock(&status_mutex);

        // 3. Picking up Forks (First Fork)
        lock(first_sem); 
        
        lock(&status_mutex);
        *first_flag = 1; 
        printStatus(phil_id, first_name);
        unlock(&status_mutex);

        // 4. Picking up Forks (Second Fork)
        lock(second_sem); 
        
        lock(&status_mutex);
        *second_flag = 1; 
        printStatus(phil_id, second_name);
        unlock(&status_mutex);
        
        // 5. Eating Period
        lock(&status_mutex);
        p->status = EAT;
        printStatus(phil_id, "changed state to EAT");
        unlock(&status_mutex);
        dawdle();

        // 6. Transitioning to Think (CHANGING)
        lock(&status_mutex);
        p->status = CHANGING;
        printStatus(phil_id, "changed state to CHANGING (putting down forks)");
        unlock(&status_mutex);
        
        // 7. Setting down Forks
        
        // Release the second fork acquired
        lock(&status_mutex); // Lock printer/state mutex FIRST
        *second_flag = 0;    // Update state flags
        // Log the consistent state
        printStatus(phil_id, "sets down second fork"); 
        unlock(second_sem); // Release fork semaphore LAST
        unlock(&status_mutex); // Release printer/state mutex

        // Release the first fork acquired
        lock(&status_mutex);
        *first_flag = 0;
        printStatus(phil_id, "sets down first fork");
        unlock(first_sem);
        unlock(&status_mutex);

        p->cycles_left--;
    }
    
    lock(&status_mutex);
    printStatus(phil_id, "cycle complete, exiting");
    unlock(&status_mutex);
    
    pthread_exit(NULL);
}

// --- Initialization and Main ---

void init_sems() {
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&forkArray[i], 0, 1) != 0) { 
            perror("Fork semaphore initialization failed");
            exit(EXIT_FAILURE);
        }
    }
    
    if (sem_init(&status_mutex, 0, 1) != 0) {
        perror("Status mutex initialization failed");
        exit(EXIT_FAILURE);
    }
}

void destroy_sems() {
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_destroy(&forkArray[i]);
    }
    sem_destroy(&status_mutex);
}

int main(int argc, char *argv[]) {
    int i;
    pthread_t p_threads[NUM_PHILOSOPHERS];

    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_sec + tv.tv_usec); 
    
    if (argc > 1) {
        char *endptr;
        long cycles_arg = strtol(argv[1], &endptr, 10);
        
        if (*endptr != '\0' || cycles_arg <= 0) {
            fprintf(stderr, \
        "Error: Invalid or non-positive integer for eat-think cycles.\n");
            return EXIT_FAILURE;
        }
        eat_think_cycles = cycles_arg;
    }

    init_sems();
    printHeader();

    for(i = 0; i < NUM_PHILOSOPHERS; i++){
        philArray[i].id = i;
        philArray[i].cycles_left = eat_think_cycles;
        philArray[i].status = CHANGING;
        philArray[i].holding_lfork = 0;
        philArray[i].holding_rfork = 0;
        philArray[i].lfork_idx = i;
        // Edge case for last philosopher's right fork
        philArray[i].rfork_idx = (i + 1) % NUM_PHILOSOPHERS;
    }

    // Create new thread for each philosopher
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        int res = \
    pthread_create(&p_threads[i], NULL, phil_cycle, (void *)&philArray[i]);
        if (res != 0) {
            fprintf(stderr, \
    "Error creating thread for philosopher %c: %s\n", 'A' + i, strerror(res));
            destroy_sems();
            return EXIT_FAILURE;
        }
    }

    // Waits for all threads to complete their cycles and terminates them.
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_join(p_threads[i], NULL) != 0) {
            perror("pthread_join failed");
        }
    }

    destroy_sems();

    fprintf(stderr,\
    "\nAll philosophers finished their %ld cycle(s).\n", eat_think_cycles);
    return EXIT_SUCCESS;
}