#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../fs.h"


void * queue_server(void * args) {

    while(1) {
        mutex_lock();
        printf("SERVER\n");
        usleep(0.725 * 1e6);
        mutex_unlock();
    }
    return NULL;
}


void * queue_client(void * args) {


    while(1) {
        mutex_lock();
        printf("CLIENT\n");
        fflush(stdout);
        usleep(7.33 * 1e6);
        mutex_unlock();
    }


    return NULL;
}


int main(int argc, char** argv) {

    pthread_t client, server;

    printf("Start...\n");
    queue_init();

    pthread_create(&server, NULL, &queue_server, NULL);
    pthread_create(&client, NULL, &queue_client, NULL);

    pthread_join(server, NULL);
    pthread_join(client, NULL);

    printf("Done...\n");

    return (EXIT_SUCCESS);
}