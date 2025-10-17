#include "dine.h"
#define EAT 1
#define THINK 2
#define CHANGING 0 

int onOdds = 0;

typedef struct philosopher_st {
    double pos;
    int holding_rfork;
    int holding_lfork;
    int status;
} phil;
// assume NUM_PHILOSOPHERS

main(char * argv, int argc){
    int i = 0;
    int forkArray[NUM_PHILOSOPHERS] = (* int)malloc(sizeof(int) * NUM_PHILOSOPHERS);
    memset(forkArray, 0, sizeof(int) * NUM_PHILOSOPHERS);

    phil philArray[NUM_PHILOSOPHERS] = (phil)malloc(sizeof(int) * NUM_PHILOSOPHERS);    
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

// implement semaphores