#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../fs.h"


int main(){
    printf("queue_init\n");
    int qid = queue_init();
    if(qid < 0){
        return -1;
    }
    printf("mutex_lock\n");
    if(mutex_lock() != SFSQ_OK) {
        return -1;
    }
    printf("mutex_unlock\n");
    if(mutex_unlock() != SFSQ_OK) {
        return -1;
    }
    printf("queue_destroy\n");
    queue_destroy();
    printf("return\n");
    return 0;
}
