#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "../include/queue.h"

int mutex_lock(int fd) {
    int qid = msgget(SFS_QUEUE_KEY, 0666);
    if(qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    ssize_t ret = msgrcv(qid, &message, sizeof(message), fd, MSG_NOERROR);
    if(ret < 0) {
        return SFSQ_MSGRCV_ERROR;
    }
    return SFSQ_OK;
}

int mutex_unlock(int fd) {
    int qid = msgget(SFS_QUEUE_KEY, 0666);
    if(qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    message.fd = fd;
    ssize_t ret = msgsnd(qid, &message, sizeof(message), MSG_NOERROR);
    if(ret < 0) {
        return SFSQ_MSGSND_ERROR;
    }
    return SFSQ_OK;
}

int queue_init() {
    int qid = msgget(SFS_QUEUE_KEY, 0666 | IPC_CREAT);
    if(qid < 0) {
        return SFSQ_MSGCREAT_ERROR;
    }
}