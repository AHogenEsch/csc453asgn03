#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif

// prints header, including philosopher character, 'A' + i
void printHeader(){
    int i = 0;
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++){
        printf("=============|");
    }
    printf("\n");
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++){
        printf("      %c      |", 'A' + i);
    }
    printf("\n");
    printf("|");
    for(i = 0; i < NUM_PHILOSOPHERS; i++){
        printf("=============|");
    }
    printf("\n");
}

