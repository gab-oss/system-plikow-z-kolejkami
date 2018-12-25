#ifndef FS_H
#define FS_H

// Global constants
#define MAX_FILES               16
#define NAME_SIZE               100
#define INFO_SIZE               5

// Argument flags
#define FS_RDONLY               1	// 001 - Read only
#define FS_WRONLY               2	// 010 - Write only
#define FS_RDWR                 3	// 011 - Read and write
#define FS_CREAT                4	// 100 - Crate file

// Error codes
#define SFS_LOCK_MUTEX_ERROR    -1
#define SFS_UNLOCK_MUTEX_ERROR  -2


int simplefs_open(char* name, int mode);

int simplefs_close(int fd);

int simplefs_unlink(char* name);

int simplefs_mkdir(char* name);

int simplefs_creat(char* name, int mode);

int simplefs_read(int fd, char* buf, int len);

int simplefs_write(int fd, char* buf, int len);

int simplefs_lseek(int fd, int whence, int offset);

int simplefs_mount(char* name, int size, int inodes);

int simplefs_unmount(char* name);

#endif //FS_H
