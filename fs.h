#ifndef FS_H
#define FS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

// Global constants
#define MAX_FILES     16
#define NAME_SIZE    100
#define INFO_SIZE      5

//size of FS + descriptors table
#define METADATA_SIZE (sizeof(int) + MAX_FILES * (sizeof(int) * INFO_SIZE + NAME_SIZE))

// Argument flags
#define FS_RDONLY               1    // 001 - Read only
#define FS_WRONLY               2    // 010 - Write only
#define FS_RDWR                 3    // 011 - Read and write

#define SFS_LOCK_MUTEX_ERROR    -22
#define SFS_UNLOCK_MUTEX_ERROR  -23
#define SFS_CREATE_QUEUE_ERROR  -24
#define SFS_DESTROY_QUEUE_ERROR -25
#define SFS_LOCK_FILE_ERROR     -26
#define SFS_UNLOCK_FILE_ERROR   -27
#define SFS_FILE_LOCKED_ERROR   -28

// ========= MESSAGE MUTEX START ===========

#define SFS_QUEUE_KEY           "/simplefs_key"

#define SFSQ_OK                 0
#define SFSQ_MSGGET_ERROR       -10
#define SFSQ_MSGRCV_ERROR       -12
#define SFSQ_MSGSND_ERROR       -13
#define SFSQ_MSGCREAT_ERROR     -14
#define SFSQ_MSGDESTROY_ERROR   -15
#define SFSQ_FILE_LOCKED_ERROR  -16

#define SFSQ_FS_MUTEX_TYPE      100

/*
 * Message type for queue messages.
 *
 * type=1 for filesystem mutex
 */
struct sfs_msg {
    long type;
    pid_t pid;
};


int send_msg(int qid, long type) {
    struct sfs_msg message;
    message.type = type;
    message.pid = getpid();
    return msgsnd(qid, &message, sizeof(message), MSG_NOERROR);
}

/*
 */
int lock_file(int fd) {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    int ret = send_msg(qid, fd + MAX_FILES + 1);
    if (ret < 0) {
        return SFSQ_MSGSND_ERROR;
    }
    return SFSQ_OK;
}


/*
 */
int access_file(int fd) {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    ssize_t ret = msgrcv(qid, &message, sizeof(message), fd + MAX_FILES + 1, IPC_NOWAIT);
    if (ret < 0 && errno != ENOMSG) {
        return SFSQ_MSGRCV_ERROR;
    }
    if(errno != ENOMSG && message.pid != getpid()){
        msgsnd(qid, &message, sizeof(message), MSG_NOERROR);
        return SFSQ_FILE_LOCKED_ERROR;
    }
    return SFSQ_OK;
}

int unlock_file(int fd) {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    ssize_t ret = msgrcv(qid, &message, sizeof(message), fd + MAX_FILES + 1, IPC_NOWAIT);
    if (ret < 0 && errno != ENOMSG) {
        return SFSQ_MSGRCV_ERROR;
    }
    return SFSQ_OK;
}

/*
 */
int mutex_lock_file(int fd) {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    ssize_t ret = msgrcv(qid, &message, sizeof(message), fd, MSG_NOERROR);
    if (ret < 0) {
        return SFSQ_MSGRCV_ERROR;
    }
    return SFSQ_OK;
}


/*
 */
int mutex_unlock_file(int fd) {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    int ret = send_msg(qid, fd);
    if (ret < 0) {
        return SFSQ_MSGSND_ERROR;
    }
    return SFSQ_OK;
}


/*
 * Blocks the execution of the process until mutex for a file with
 * with file descriptor fd is unlocked.
 * Returns SFSQ_OK if no errors occurred.
 */
int mutex_lock() {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    struct sfs_msg message;
    ssize_t ret = msgrcv(qid, &message, sizeof(message), SFSQ_FS_MUTEX_TYPE, MSG_NOERROR);
    if (ret < 0) {
        return SFSQ_MSGRCV_ERROR;
    }
    return SFSQ_OK;
}

/*
 * Unblocks mutex access to the given file with file descriptor fd.
 * Returns SFSQ_OK if no errors occurred.
 */
int mutex_unlock() {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT);
    if (qid < 0) {
        return SFSQ_MSGGET_ERROR;
    }
    ssize_t ret = send_msg(qid, SFSQ_FS_MUTEX_TYPE);
    if (ret < 0) {
        return SFSQ_MSGSND_ERROR;
    }
    return SFSQ_OK;
}

/*
 * Initialises a message queue for simple fs management.
 * Returns SFSQ_OK if no errors occurred.
 */
int queue_init() {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
    if (qid < 0) {
        return SFSQ_MSGCREAT_ERROR;
    }
    ssize_t ret = send_msg(qid, SFSQ_FS_MUTEX_TYPE);
    if (ret < 0) {
        return SFSQ_MSGSND_ERROR;
    }
    return SFSQ_OK;
}


int queue_destroy() {
    key_t key = ftok(SFS_QUEUE_KEY, 65);
    int qid = msgctl(key, IPC_RMID, NULL);
    if (qid < 0) {
        return SFSQ_MSGDESTROY_ERROR;
    }
}



// ========= MESSAGE MUTEX END ===========

int capacity;
int freeMemory;
int fileCount = 0;
char FSAbsolutePath[200];

char fileNames[MAX_FILES][NAME_SIZE];
int fileInfos[MAX_FILES][INFO_SIZE];
//0 - position in FS
//1 - filesize
//2 - is directory
//3 - read allowed
//4 - write allowed

int posInFile[MAX_FILES];

//list of free memory blocks
struct inodeFree {
    //position in FS and size
    int base, size;
    struct inodeFree *next;
} head;



// ========= INTERNAL FUNCTIONS START ===========


void updateMemory();

void readFS(char name[]);

int check_path(char *name, char *_filename);

//get file indexes in order as written in FS
void getSortedOrder(int order[]) {
    for(int i = 0; i< MAX_FILES; ++i)
        order[i] = i;

    for (int i = 0; i < MAX_FILES-1; i++) {
        int min = i;
        for (int j = i+1; j < MAX_FILES; j++) {
            if (fileInfos[order[j]][1] != 0 && fileInfos[order[j]][0] < fileInfos[order[min]][0])
            {
                min = order[j];
            }
        }
        int temp = order[i];
        order[i] = order[min];
        order[min] = temp;
    }
}

void updateMetadata(FILE *FS) {
    fseek(FS, sizeof(int), SEEK_SET);
    for (int i = 0; i < MAX_FILES; i++) {
        //write rows of file descriptor table
        fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
        fwrite(&fileInfos[i][0], sizeof(int), INFO_SIZE, FS);
    }
}

void readFS(char name[]) {
    strcpy(FSAbsolutePath, name);
    FILE *file = fopen(name, "r");
    if (file == NULL) {
        return;
    }


    //read FS capacity from metadata and calculate metadata size
    fread(&capacity, sizeof(int), 1, file);
    freeMemory = capacity - METADATA_SIZE;

    //read inode table
    fileCount = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        //read filename
        fread(&fileNames[i][0], sizeof(char), NAME_SIZE, file);
        //read position and size
        fread(&fileInfos[i][0], sizeof(int), INFO_SIZE, file);

        freeMemory -= fileInfos[i][1];
        //check file existence
        if (fileInfos[i][1] != 0)
            fileCount++;
    }
    updateMemory();
    fclose(file);
}

void updateMemory() {
    int i = 0;
    int ord[MAX_FILES];
    getSortedOrder(ord);

    if (fileCount < 1) {
        //there are no files
        head.base = METADATA_SIZE;
        head.size = capacity - head.base + 1;
        head.next = NULL;
        return;
    } else {
        if (fileInfos[ord[0]][0] == METADATA_SIZE) {
            //first file is just after metadata
            while (i < fileCount - 1 && fileInfos[ord[i]][0] + fileInfos[ord[i]][1] == fileInfos[ord[i + 1]][0]) {
                //find file followed by free memory
                i++;
            }
            //position of first free memory segment
            head.base = fileInfos[ord[i]][0] + fileInfos[ord[i]][1];
            if (i == fileCount - 1) {
                //all free memory follows last file
                head.size = capacity - head.base + 1;
                head.next = NULL;
                return;
            } else {
                //there is free memory between files
                head.size = fileInfos[ord[i + 1]][0] - head.base;
                head.next = NULL;
            }
        } else {
            //there is free memory between metadata and first file
            head.base = METADATA_SIZE;
            head.size = fileInfos[ord[0]][0] - head.base;
        }
    }

    struct inodeFree *prev = &head;
    //add free memory segments to list
    for (i; i < fileCount; i++) {
        if (fileInfos[ord[i]][0] + fileInfos[ord[i]][1] == fileInfos[ord[i + 1]][0])
            //there is no free memory between files
            continue;

        struct inodeFree *temp = (struct inodeFree *) malloc(sizeof(struct inodeFree));
        temp->base = fileInfos[ord[i]][0] + fileInfos[ord[i]][1];
        if (temp->base > capacity)
            //no free memory after last file
            break;

        if (i != MAX_FILES - 1 && fileInfos[ord[i + 1]][1] != 0) {
            //there are files after current one
            temp->size = fileInfos[ord[i + 1]][0] - temp->base;
        } else {
            //there is only free memory after current file
            temp->size = capacity - temp->base + 1;
        }

        temp->next = NULL;
        prev->next = temp;
        prev = temp;
    }
}

int internal_lseek(int fd, int whence, int offset) {
    if (whence == SEEK_SET)
        posInFile[fd] = offset;
    else if (whence == SEEK_CUR)
        posInFile[fd] += offset;
    else if (whence == SEEK_END)
        posInFile[fd] = fileInfos[fd][1] - offset;
    else
        return -1;
    return 0;
}

int internal_write(int fd, char *buf, int len) {

    //check if there's enough memory
    if (freeMemory < sizeof(char) * len) {
        return -1;
    }

    //fd is a valid descriptor
    if (fd >= fileCount) {
        return -2;
    }

    //write permission's set
    if (!fileInfos[fd][4]) {
        return -3;
    }

    //file is not a directory
    //if(fileInfos[fd][2]) {return -1;}

    //first position after file
    int eofpos = fileInfos[fd][0] + fileInfos[fd][1];

    FILE *file = fopen(FSAbsolutePath, "r+");

    //size from current position to eof
    int restfile = fileInfos[fd][1] - posInFile[fd];
    //how much will the size increase
    int sizeinc = 0;
    if (restfile < sizeof(char) * len) {
        sizeinc = sizeof(char) * len - restfile;
    }
    //check if there's enough space right after the file
    struct inodeFree *temp = &head;
    while (temp != NULL) {
        if (temp->base == eofpos && temp->size >= sizeinc) {
            //temporary save the part of the file after posInFile
            fseek(file, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
            //write
            int wr = fwrite(buf, sizeof(char), len, file);
            posInFile[fd] += sizeof(char) * len;
            fileInfos[fd][1] += sizeinc;

            updateMemory();
            updateMetadata(file);
            fclose(file);
            return wr;
        }
        temp = temp->next;
    }

    //look for enough free space
    temp = &head;
    while (temp != NULL) {
        if (temp->size >= fileInfos[fd][1] + sizeinc) {
            //move file
            char *tempbuf = (char*)malloc(fileInfos[fd][1]);
            fseek(file, fileInfos[fd][0], SEEK_SET);
            fread(tempbuf, sizeof(char), fileInfos[fd][1], file);
            fseek(file, temp->base, SEEK_SET);
            fwrite(tempbuf, sizeof(char),fileInfos[fd][1], file);

            free(tempbuf);
            //write
            int wr = fwrite(buf, sizeof(char), len, file);

            fileInfos[fd][0] = temp->base;
            fileInfos[fd][1] += len * sizeof(char);
            posInFile[fd] += sizeof(char) * len;

            updateMemory();
            updateMetadata(file);
            fclose(file);
            return wr;
        }

        temp = temp->next;
    }

    fclose(file);
    //no block large enough
    return -1;

}

int internal_read(int fd, char *buf, int len) {
    //check permissions, if file is not dir, can be read and is long enoungh
    if (fileInfos[fd][1] < 1 && fileInfos[fd][2] != 0 && fileInfos[fd][3] != 1)
        return -1;

    if(len > fileInfos[fd][1] - posInFile[fd])
        len = fileInfos[fd][1] - posInFile[fd];

    FILE *FS = fopen(FSAbsolutePath, "r+");

    //set position to file position + offset
    fseek(FS, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
    int bytes_read = fread(buf, sizeof(char), len, FS);
    posInFile[fd] += bytes_read;
    fclose(FS);
    return bytes_read;
}

int check_prev_dir(int prevdesc, char dir[]) {
    //check if previous directory contains dir and return dirdesc
    if (prevdesc < 0) { //prev is home
        if (strcmp(dir, fileNames[0]))
            return 0;
        return -1;
    }

    //dir empty
    if(fileInfos[prevdesc][1] <= sizeof(int))
        return -1;

    char *buf = (char*)malloc(fileInfos[prevdesc][1] - sizeof(int));
    internal_lseek(prevdesc, SEEK_SET, sizeof(int));
    internal_read(prevdesc, buf, fileInfos[prevdesc][1] - sizeof(int)); //read prev to buf
    //first byte is just flag, not id
    int *bufI = (int*)malloc(fileInfos[prevdesc][1] - sizeof(int));
    memcpy(bufI, buf, fileInfos[prevdesc][1]-sizeof(int));
    for (int j = 0; j < fileInfos[prevdesc][1]/sizeof(int) - 1; ++j) {
        if (strcmp(fileNames[bufI[j]], dir) == 0) { //dir found in prev
            free(buf);
            return bufI[j];
        }
    }

    return -1;
}

int check_path(char *name, char *_filename) {
    char filename[NAME_SIZE];
    int dirdesc = -1;

    int n_i = 0, f_i = 0;            //name i, filename i

    while (name[n_i] != '\0') {
        if (f_i >= NAME_SIZE) //one of the names in the path is too long
            return -1;

        if (name[n_i] == '/') { //directory name read to the filename array
            filename[f_i] = '\0'; //close string
            if (dirdesc == -1) { //top directory
                for (int i = 0; i < MAX_FILES; ++i) {
                    if (strcmp(filename, fileNames[i]) == 0) { //dir existis
                        dirdesc = i;
                        f_i = 0;
                        break;
                    }
                    if (i == MAX_FILES - 1)
                        return -1;
                }
            } else {
                int fileId = check_prev_dir(dirdesc, filename);
                if (fileId > 0) { //check if dir is in prev dir
                    dirdesc = fileId;
                    f_i = 0;
                } else {
                    return -1;
                }
            }
        } else {
            filename[f_i] = name[n_i]; //read next character to the filename
            ++f_i;
        }

        ++n_i;
    }
    filename[f_i] = '\0'; //close filename
    strcpy(_filename, filename);

    if (dirdesc == -1)
        return -1;

    int fileId = check_prev_dir(dirdesc, filename);
    if (fileId > 0)
        return fileId;
    else
        return -1 * dirdesc - 2;
}

int internal_defragment() {
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL){
        return -1;
    }

    int ord[MAX_FILES];
    getSortedOrder(ord);

    if (fileCount > 0) {
        //write first file to buffer
        char *buffer = (char *) malloc(fileInfos[ord[0]][1]);
        fseek(FS, fileInfos[ord[0]][0], SEEK_SET);
        fread(buffer, sizeof(char), fileInfos[ord[0]][1], FS);

        //move first file to after metadata
        fileInfos[ord[0]][0] = METADATA_SIZE;
        fseek(FS, fileInfos[ord[0]][0], SEEK_SET);
        fwrite(buffer, sizeof(char), fileInfos[ord[0]][1], FS);

        free(buffer);
    }
    for (int i = 1; i < fileCount; i++) {
        //move file to after previous one
        char *buffer = (char *) malloc(fileInfos[ord[i]][1]);
        fseek(FS, fileInfos[ord[i]][0], SEEK_SET);
        fread(buffer, sizeof(char), fileInfos[ord[i]][1], FS);

        fileInfos[ord[i]][0] = fileInfos[ord[i - 1]][0] + fileInfos[ord[i - 1]][1] + 1;
        fseek(FS, fileInfos[ord[i]][0], SEEK_SET);
        fwrite(buffer, sizeof(char), fileInfos[ord[i]][1], FS);
        free(buffer);
    }

    updateMetadata(FS);
    updateMemory();
    fclose(FS);
    return 0;
}

int internal_creat(char *name, int mode){
    char filename[NAME_SIZE];
    int dirdesc = check_path(name, filename);
    if (dirdesc > -2) //name = path, filename = resulting filename
        return -1;

    dirdesc = -1 * (dirdesc + 2);

    if (strlen(name) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < sizeof(int))
        return -2;

    //find free memory for file
    struct inodeFree *temp = &head;

    //permissions
    char readPerm = 0, writePerm = 0;
    if (mode == FS_RDONLY || mode == FS_RDWR)
        readPerm = 1;
    if (mode == FS_WRONLY || mode == FS_RDWR)
        writePerm = 1;

    int i;
    for (i = 0; i < MAX_FILES; i++) {
        //find empty descriptor
        if (fileInfos[i][1] == 0)
            break;
    }

    //add file to dir
    int buf[1];
    buf[0] = i;
    internal_lseek(dirdesc, SEEK_SET, fileInfos[dirdesc][1]);
    internal_write(dirdesc, (char *) buf, sizeof(int));

    //file metadata
    strcpy(fileNames[i], filename);
    fileInfos[i][0] = temp->base;    //position
    fileInfos[i][1] = sizeof(int);   //file length
    fileInfos[i][2] = 0;            //not a directory
    fileInfos[i][3] = readPerm;        //read permission
    fileInfos[i][4] = writePerm;    //write permission

    fileCount++;
    updateMemory();
    freeMemory -= sizeof(int);

    //open FS file
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return -1;

    updateMetadata(FS);
    fclose(FS);

    return i;
}


// ========= INTERNAL FUNCTIONS END =============

// ========= EXTERNAL API START =================

int simplefs_unmount(char *name) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    int ret = remove(name);
    // No need to unlock, just destroy
    if (queue_destroy() != SFSQ_OK) {
        return SFS_DESTROY_QUEUE_ERROR;
    }
    return ret;
}

int simplefs_read(int fd, char *buf, int len) {
    if (mutex_lock_file(fd) != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    readFS(FSAbsolutePath);
    int ret = internal_read(fd, buf, len);
    updateMemory();
    if (mutex_unlock_file(fd) != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return ret;
}

int simplefs_write(int fd, char *buf, int len) {
    if (mutex_lock_file(fd) != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    readFS(FSAbsolutePath);
    int unlock_file_ret = access_file(fd);
    if(unlock_file_ret == SFSQ_FILE_LOCKED_ERROR){
        if (mutex_unlock_file(fd) != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return SFS_FILE_LOCKED_ERROR;
    }
    if(unlock_file_ret != SFSQ_OK){
        if (mutex_unlock_file(fd) != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    int ret = internal_write(fd, buf, len);
    if (mutex_unlock_file(fd) != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return ret;

}

int simplefs_lseek(int fd, int whence, int offset) {
    if (mutex_lock_file(fd) != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    readFS(FSAbsolutePath);
    int ret = internal_lseek(fd, whence, offset);
    updateMemory();
    if (mutex_unlock_file(fd) != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return ret;
}


int simplefs_close(int fd) {
    if (unlock_file(fd) != SFSQ_OK) {
        return SFS_UNLOCK_FILE_ERROR;
    }
    return 0;
}


int simplefs_defragment() {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    int ret = internal_defragment();
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return ret;
}

int simplefs_creat(char *name, int mode) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    readFS(FSAbsolutePath);
    int ret = 0;
    int fd = internal_creat(name, mode);
    if(fd >= 0) {
        if (mutex_unlock_file(fd) != SFSQ_OK) {
            ret = SFS_UNLOCK_FILE_ERROR;
        }
    } else {
        ret = fd;
    }
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return ret;
}


// ========= EXTERNAL API END ===================



int simplefs_mount(char *name, int size) {
    if (access(name, F_OK) != -1) {
        readFS(name);
        return 0;
    }
    // Initialize queue
    if (queue_init() != SFSQ_OK) {
        return SFS_CREATE_QUEUE_ERROR;
    }
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    strcpy(FSAbsolutePath, name);
    //check if enough space for metadata and first file
    if (size < METADATA_SIZE + sizeof(int)) {
        if (mutex_unlock() != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return -1;
    }

    capacity = size;
    freeMemory = capacity - (METADATA_SIZE + sizeof(char));

    //first dir
    fileCount++;

    strcpy(fileNames[0], ".");
    fileInfos[0][0] = METADATA_SIZE;
    fileInfos[0][1] = sizeof(int);
    fileInfos[0][2] = 1;
    fileInfos[0][3] = 1;
    fileInfos[0][4] = 1;

    FILE *file = fopen(name, "w");

    //write size of FS to metadata
    fwrite(&size, sizeof(int), 1, file);
    updateMetadata(file);
    updateMemory();

    fclose(file);
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return 0;
}

int simplefs_open(char *name, int mode) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    char filename[NAME_SIZE];
    int fileId = check_path(name, filename);
    if (fileId < 0 || fileInfos[fileId][2] == 1) {
        //no file or file is dir
        if (mutex_unlock() != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return -1;
    }
    posInFile[fileId] = 0; //set postion if file to 0
    if (fileInfos[fileId][4] && (mode == FS_WRONLY || mode == FS_RDWR)) {
        int unlock_file_ret = access_file(fileId);
        if(unlock_file_ret == SFSQ_FILE_LOCKED_ERROR){
            if (mutex_unlock() != SFSQ_OK) {
                return SFS_UNLOCK_MUTEX_ERROR;
            }
            return SFS_FILE_LOCKED_ERROR;
        }
        if(unlock_file_ret != SFSQ_OK){
            if (mutex_unlock() != SFSQ_OK) {
                return SFS_UNLOCK_MUTEX_ERROR;
            }
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        lock_file(fileId);
    }
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return fileId;
}



int simplefs_unlink(char *name) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    char filename[NAME_SIZE];
    int fileId = check_path(name, filename);
    if (fileId <= 0 || (fileInfos[fileId][2] == 1 && fileInfos[fileId][1] != sizeof(int))) {
        //no file or file is non-empty dir
        if (mutex_unlock() != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return -1;
    }

    if(fileInfos[fileId][2] != 0 && fileInfos[fileId][4]) {
        int ret = access_file(fileId);
        if(ret != SFSQ_OK){
            return SFS_UNLOCK_FILE_ERROR;
        }
    }

    //clear filename and info of deleted file
    fileCount--;
    freeMemory += fileInfos[fileId][1];
    memset(fileNames[fileId], 0, NAME_SIZE);
    memset(fileInfos[fileId], 0, INFO_SIZE * sizeof(int));

    //find dir with file
    char *dirname;
    char *temp = strtok(name, "/");
    while(temp != NULL) {
        dirname = temp;
        temp = strtok(NULL, "/");
        if(strcmp(temp,filename) == 0)
            break;
    }
    //find dir index
    int dirIdx = 0;
    for(;dirIdx < MAX_FILES; ++dirIdx)
    {
        if(strcmp(dirname, fileNames[dirIdx]) == 0)
            break;
    }
    //rewrite dir

    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return 1;

    //read parent dir
    int dirSize = fileInfos[dirIdx][1] - sizeof(int);
    int *buf = (int*)malloc(dirSize);
    fseek(FS, fileInfos[dirIdx][0] + sizeof(int),SEEK_SET);
    fread(buf, dirSize, 1, FS);

    //find position of file to be removed
    int i = 0;
    while(buf[i] != fileId)
        ++i;

    //move rest of file to delete
    memmove(buf+i, buf+i+1,dirSize - i - 1);

    //save change to dir
    fseek(FS, fileInfos[dirIdx][0] + sizeof(int),SEEK_SET);
    fwrite(buf, dirSize - sizeof(int), 1,FS);
    fileInfos[dirIdx][1] -= sizeof(int);

    free(buf);

    updateMemory();

    //save changes to FS

    updateMetadata(FS);

    fclose(FS);
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return 0;


}

int simplefs_mkdir(char *name) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    char filename[NAME_SIZE];
    int dirId = check_path(name, filename);
    if (dirId >= -1)
        //wrong path or file exists
        return -1;

    dirId = -1 * (dirId + 2);

    if (strlen(filename) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < sizeof(int))
        return -1;

    //find free memory for file
    struct inodeFree *temp = &head;

    int i;
    for (i = 0; i < MAX_FILES; i++) {
        //find empty descriptor
        if (fileInfos[i][1] == 0)
            break;
    }

    //add dir to dir
    int buf[1];
    buf[0] = i;
    internal_lseek(dirId, SEEK_SET, fileInfos[dirId][1]);
    internal_write(dirId, (char *) buf, sizeof(int));

    //file metadata
    strcpy(fileNames[i], filename);
    fileInfos[i][0] = temp->base;    //position
    fileInfos[i][1] = sizeof(int);   //file length
    fileInfos[i][2] = 1;             //directory
    fileInfos[i][3] = 1;             //read permission
    fileInfos[i][4] = 1;             //write permission

    fileCount++;
    updateMemory();
    freeMemory -= sizeof(int);

    //open FS file
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return 1;

    updateMetadata(FS);
    fclose(FS);
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return 0;
}




int simplefs_ls(char name[]) {
    if (mutex_lock() != SFSQ_OK) {
        return SFS_LOCK_MUTEX_ERROR;
    }
    char filename[NAME_SIZE];
    if (check_path(name, filename) < 0 && strcmp(name, fileNames[0]) != 0) { //name = path
        if (mutex_unlock() != SFSQ_OK) {
            return SFS_UNLOCK_MUTEX_ERROR;
        }
        return -1;
    }
    else if(strcmp(name, fileNames[0]) == 0)
    {//dir is root
        strcpy(filename, fileNames[0]);
    }

    //strcpy(filename, name); //name = filename
    for (int i = 0; i < fileCount; ++i) {
        if (strcmp(filename, fileNames[i]) == 0) {
            char *buf = (char*)malloc(fileInfos[i][1]-sizeof(int));
            internal_lseek(i, SEEK_SET, sizeof(int));
            internal_read(i, buf, fileInfos[i][1]-sizeof(int));

            //first byte is just flag, not id
            int *bufI = (int*)malloc(fileInfos[i][1]-sizeof(int));
            memcpy(bufI, buf, fileInfos[i][1]-sizeof(int));
            for (int j = 0; j < fileInfos[i][1]/sizeof(int)-1; ++j) {
                printf("\n%s", fileNames[bufI[j]]);
            }

            free(buf);
            free(bufI);
            if (mutex_unlock() != SFSQ_OK) {
                return SFS_UNLOCK_MUTEX_ERROR;
            }
            return 0;
        }
    }
    //directory not found
    if (mutex_unlock() != SFSQ_OK) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return -1;
}



#endif //FS_H