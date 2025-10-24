#include "dine.h"
#include <stdlib.h>     // for exit, strtol, random, srandom
#include <unistd.h>     // for getpid (used in example)
#include <pthread.h>    // for pthread_create, pthread_join, pthread_exit
#include <string.h>     // for strerror
#include <time.h>       // for struct timespec, nanosleep
#include <sys/time.h>   // for gettimeofday
#include <errno.h>      // for errno

// Global declarations (externed in dine.h)
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

/**
 * @brief Executes P(s) or DOWN(s) operation (sem_wait).
 */
void lock(sem_t *s_ptr) {
    if (sem_wait(s_ptr) != 0) {
        perror("Semaphore wait failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Executes V(s) or UP(s) operation (sem_post).
 */
void unlock(sem_t *s_ptr) {
    if (sem_post(s_ptr) != 0) {
        perror("Semaphore post failed");
        exit(EXIT_FAILURE);
    }
}

// --- Printing Functions ---

void printHeader() {
    int i;
    printf("+");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("-------------+");
    }
    printf("\n");
    
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("      %c      |", 'A' + i);
    }
    printf("\n");
    
    printf("+");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("-------------+");
    }
    printf("\n");
}

/**
 * @brief Prints a status line showing the state and forks held by all philosophers.
 * NOTE: The status_mutex must be held by the caller when this function is invoked.
 */
void printStatus(int changing_phil_id, const char *change_desc) {
    int i;
    // Caller holds status_mutex, no internal locking/unlocking.

    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++) {
        phil *p = &philArray[i];

        // Print Left Fork status (fork i)
        if (p->holding_lfork) {
            printf(" %d", p->lfork_idx);
        } else {
            printf("  ");
        }
        
        // Print State
        switch(p->status) {
            case EAT:
                printf("  Eat  ");
                break;
            case THINK:
                printf(" Think ");
                break;
            case CHANGING:
            default:
                printf("       ");
                break;
        }

        // Print Right Fork status (fork (i+1)%N)
        if (p->holding_rfork) {
            printf("%d ", p->rfork_idx);
        } else {
            printf("  ");
        }
        
        printf("|");
    }
    printf(" Change: P%c %s\n", 'A' + changing_phil_id, change_desc); 
}

// --- Main Philosopher Logic ---

void *philosopher_cycle(void *arg) {
    phil *p = (phil *)arg;
    int phil_id = p->id;
    
    int lfork = p->lfork_idx;
    int rfork = p->rfork_idx;
    
    sem_t *first_sem, *second_sem;
    int *first_flag, *second_flag;
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
    lock(&status_mutex);
    printStatus(phil_id, "starts hungry (CHANGING)"); 
    unlock(&status_mutex);

    while (p->cycles_left > 0) {
        
        // --- 1. Thinking Period ---
        lock(&status_mutex);
        p->status = THINK;
        printStatus(phil_id, "changed state to THINK");
        unlock(&status_mutex);
        dawdle();
        
        // --- 2. Transitioning to Eat (CHANGING) ---
        lock(&status_mutex);
        p->status = CHANGING;
        printStatus(phil_id, "changed state to CHANGING (hungry)");
        unlock(&status_mutex);

        // --- 3. Picking up Forks (First Fork) ---
        lock(first_sem); 
        
        lock(&status_mutex);
        *first_flag = 1; 
        printStatus(phil_id, first_name);
        unlock(&status_mutex);

        // --- 4. Picking up Forks (Second Fork) ---
        lock(second_sem); 
        
        lock(&status_mutex);
        *second_flag = 1; 
        printStatus(phil_id, second_name);
        unlock(&status_mutex);
        
        // --- 5. Eating Period ---
        lock(&status_mutex);
        p->status = EAT;
        printStatus(phil_id, "changed state to EAT");
        unlock(&status_mutex);
        dawdle();

        // --- 6. Transitioning to Think (CHANGING) ---
        lock(&status_mutex);
        p->status = CHANGING;
        printStatus(phil_id, "changed state to CHANGING (putting down forks)");
        unlock(&status_mutex);
        
        // --- 7. Setting down Forks (FIXED LOGIC) ---
        
        // Release the second fork acquired
        lock(&status_mutex); // Lock printer/state mutex FIRST
        *second_flag = 0;    // Update state flags
        printStatus(phil_id, "sets down second fork"); // Log the consistent state
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
            fprintf(stderr, "Error: Invalid or non-positive integer for eat-think cycles.\n");
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
        philArray[i].rfork_idx = (i + 1) % NUM_PHILOSOPHERS;
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        int res = pthread_create(&p_threads[i], NULL, philosopher_cycle, (void *)&philArray[i]);
        if (res != 0) {
            fprintf(stderr, "Error creating thread for philosopher %c: %s\n", 'A' + i, strerror(res));
            destroy_sems();
            return EXIT_FAILURE;
        }
    }

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_join(p_threads[i], NULL) != 0) {
            perror("pthread_join failed");
        }
    }

    destroy_sems();

    fprintf(stderr, "\nAll philosophers finished their %ld cycle(s).\n", eat_think_cycles);
    return EXIT_SUCCESS;
}