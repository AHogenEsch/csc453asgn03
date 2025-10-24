#include "dine.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define EAT 1
#define THINK 2
#define CHANGING 0 

int onOdds = 0;
// A semaphore value of 1 means the fork is available.
sem_t forkArray[NUM_PHILOSOPHERS];

typedef struct philosopher_st {
    double pos;
    int holding_rfork;
    int holding_lfork;
    int status;
} phil;

phil philArray[NUM_PHILOSOPHERS];

void init_sems() {
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        // sem_init(semaphore, pshared, value)
        // pshared=0 means the semaphore is local to this process
        // value=1 means the resource  is initially available
        if (sem_init(&forkArray[i], 0, 1) != 0) {
            perror("Semaphore initialization failed");
            exit(EXIT_FAILURE);
        }
    }
}

// P(s) or DOWN(s)
void lock(sem_t sem){
    // returns non zero on error
    if (sem_wait(&forkArray[sem]) != 0) {
        perror("Semaphore wait failed");
    }
}

// V(s) or UP(s)
void unlock(sem_t sem){
    if (sem_post(&forkArray[sem]) != 0) {
        perror("Semaphore wait failed");
    }
}

void printStatus(phil phil){
    int i = 0;
    printf("| ");
    // edge case if phil is the first one at the table
    if(phil.pos < 1){
        if(phil.holding_lfork){
            printf("1");
        }
        if(phil.holding_rfork){
            printf("2");
        }
    }
    for(i = 0; i < phil.pos ; i++){
        if(phil.holding_lfork){
            // if pos = 1.5, lfork = 1
            printf("%d", i);
        }
    }
    // i++ is now the rfork
    if(phil.holding_rfork){
        printf("%d", i);
    }
    for(i < NUM_PHILOSOPHERS; i++){
        printf("|");
    }
    if(phil.status == EAT){
        printf(" EAT   ");
    }
    else if(phil.status == THINK){
        printf(" THINK ");
    }
    else{
        printf("       ");
    }
}

main(char * argv, int argc){
    int i = 0;

    phil philArray[NUM_PHILOSOPHERS] = (*phil)malloc(sizeof(int) * NUM_PHILOSOPHERS);    
    for(i = 0; i< NUM_PHILOSOPHERS; i++){
        philArray[i].pos = i + 0.5;
        philArray[i].holding_lfork = philArray[i].holding_rfork = philArray[i].status 0;

        // for every philospher, put them somewhere. Create 1 fork per 
    }
    while(17){
        for(i = 0; i < NUM_PHILOSOPHERS; i++){
            if(onOdds){
                if(isOdd(philArray[i].pos - 0.5)){
                    // have the odd ones choose a fork first. Choosing the left fork 
                    if(!forkArray[i]){
                        forkArray[i] = 1;
                        // print the corresponding philosopher and their fork pickup
                        philArray[i].holding_lfork = 1;
                        forkCheck(&philArray[i]);
                    }
                }
            }
        }
        
    }
}

// returns 1 if even, 0 if not even
int isOdd(int num){
    return num % 2;
}

void forkCheck(phil *p){
    if(p->holding_lfork && p->holding_rfork){
        p->status = EAT;
        // print status change
    }
}

// implement semaphore