#include "fs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>


int simplefs_open(char* name, int mode) {
    // @TODO
}

int simplefs_close(int fd) {
    // @TODO
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