#ifndef DINE_H
#define DINE_H

// Required for printf and other standard I/O
#include <stdio.h> 
// Required for the sem_t type
#include <semaphore.h>
// Required for general types like pthread_t
#include <pthread.h>

// Define NUM_PHILOSOPHERS, default to 5
#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif

// States for philosophers
#define CHANGING 0 // Has not yet acquired both forks or not yet set down both forks
#define EAT 1
#define THINK 2

// Dawdle factor constant (default 1000ms)
#ifndef DAWDLEFACTOR
#define DAWDLEFACTOR 1000
#endif

// Structure to hold philosopher state
typedef struct philosopher_st {
    int id;               // Philosopher index (0 to N-1)
    long cycles_left;     // How many more eat-think cycles to complete
    int status;           // Current state: CHANGING, EAT, or THINK
    int holding_lfork;    // 1 if holding left fork (fork i)
    int holding_rfork;    // 1 if holding right fork (fork (i + 1) % N)
    int lfork_idx;        // Index of the left fork (i)
    int rfork_idx;        // Index of the right fork ((i + 1) % N)
} phil;

// Function prototypes
void printHeader();
void printStatus(int changing_phil_id, const char *change_desc);
void *philosopher_cycle(void *arg);
void dawdle(); // Sleeps for a random amount of time

// Global variables externed from dine.c
extern sem_t forkArray[NUM_PHILOSOPHERS]; // Semaphores for forks
extern phil philArray[NUM_PHILOSOPHERS];  // Array of philosopher states
extern sem_t status_mutex;                // Mutex for synchronized printing/state updating

#endif // DINE_H