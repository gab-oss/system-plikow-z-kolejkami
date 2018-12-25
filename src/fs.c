#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fs.h"
#include "queue.h"


int simplefs_open(char* name, int mode) {
    int fd = 1; // @TODO
    if(mutex_lock(fd) != 0){
        return SFS_LOCK_MUTEX_ERROR;
    }
    return fd;
}

int simplefs_close(int fd) {
    if(mutex_unlock(fd) != 0){
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return 0;
}

int simplefs_unlink(char* name) {
    // @TODO
}

int simplefs_mkdir(char* name) {
    // @TODO
}

int simplefs_creat(char* name, int mode) {
    return simplefs_open(name, mode | FS_CREAT);
}

int simplefs_read(int fd, char* buf, int len) {
    // @TODO
}

int simplefs_write(int fd, char* buf, int len) {
    // @TODO
}

int simplefs_lseek(int fd, int whence, int offset) {
    // @TODO
}

int simplefs_mount(char* name, int size, int inodes) {
    // @TODO
}

int simplefs_unmount(char* name) {
    // @TODO
}