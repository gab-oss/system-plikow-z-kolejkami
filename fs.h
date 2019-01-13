#ifndef FS_H
#define FS_H

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>

// Global constants
#define MAX_FILES     16
#define NAME_SIZE    100
#define INFO_SIZE            5

// Argument flags
#define FS_RDONLY               1    // 001 - Read only
#define FS_WRONLY               2    // 010 - Write only
#define FS_RDWR                 3    // 011 - Read and write
#define FS_CREAT                4
// ========= MESSAGE MUTEX START ===========

//#define SFS_QUEUE_KEY "simplefs_queue"
//
//#define SFSQ_OK                 0
//#define SFSQ_MSGGET_ERROR       -1
//#define SFSQ_MSGRCV_ERROR       -2
//#define SFSQ_MSGSND_ERROR       -3
//#define SFSQ_MSGCREAT_ERROR     -4
//
///*
// * Message type for queue messages.
// */
//struct sfs_msg {
//    long fd;
//};
//
///*
// * Blocks the execution of the process until mutex for a file with
// * with file descriptor fd is unlocked.
// * Returns SFSQ_OK if no errors occurred.
// */
//int mutex_lock(int fd) {
//    int qid = msgget(SFS_QUEUE_KEY, 0666);
//    if(qid < 0) {
//        return SFSQ_MSGGET_ERROR;
//    }
//    struct sfs_msg message;
//    ssize_t ret = msgrcv(qid, &message, sizeof(message), fd, MSG_NOERROR);
//    if(ret < 0) {
//        return SFSQ_MSGRCV_ERROR;
//    }
//    return SFSQ_OK;
//}
//
///*
// * Unblocks mutex access to the given file with file descriptor fd.
// * Returns SFSQ_OK if no errors occurred.
// */
//int mutex_unlock(int fd) {
//    int qid = msgget(SFS_QUEUE_KEY, 0666);
//    if(qid < 0) {
//        return SFSQ_MSGGET_ERROR;
//    }
//    struct sfs_msg message;
//    message.fd = fd;
//    ssize_t ret = msgsnd(qid, &message, sizeof(message), MSG_NOERROR);
//    if(ret < 0) {
//        return SFSQ_MSGSND_ERROR;
//    }
//    return SFSQ_OK;
//}
//
///*
// * Initialises a message queue for simple fs management.
// * Returns SFSQ_OK if no errors occurred.
// */
//int queue_init() {
//    int qid = msgget(SFS_QUEUE_KEY, 0666 | IPC_CREAT);
//    if(qid < 0) {
//        return SFSQ_MSGCREAT_ERROR;
//    }
//}


// ========= MESSAGE MUTEX END ===========

int capacity;
int freeMemory;
int fileCount = 0;
char FSAbsolutePath[200];

char fileNames[MAX_FILES][NAME_SIZE];
char fileInfos[MAX_FILES][INFO_SIZE];
//0 - position in FS
//1 - filesize
//2 - is directory
//3 - read allowed
//4 - write allowed

char posInFile[MAX_FILES];

//list of free memory blocks
struct inodeFree {
    //position in FS and size
    int base, size;
    struct inodeFree *next;
} head;

void updateMemory();

void readFS(char name[]);

int check_path(char *name, char *_filename);

int simplefs_write(int, char *, int);

int simplefs_lseek(int, int, int);

//get file indexes in order as written in FS
int *getSortedOrder() {
    static int order[MAX_FILES];
    int min = fileInfos[0][0];
    for (int i = 0; i < MAX_FILES; i++) {
        order[i] = 0;
        for (int j = 0; j < MAX_FILES; j++) {
            if (fileInfos[j][1] != 0 && fileInfos[j][0] > min && fileInfos[j][0] < fileInfos[order[i]][0]) {
                order[i] = j;
            }
        }
        min = fileInfos[order[i]][0];
    }
    return order;
}

void updateMetadata(FILE *FS) {
    fseek(FS, sizeof(int), SEEK_SET);
    for (int i = 0; i < MAX_FILES; i++) {
        fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
        fwrite(&fileInfos[i][0], sizeof(char), INFO_SIZE, FS);
    }
}

//TODO: mutexy
int simplefs_mount(char *name, int size) {
    if (access(name, F_OK) != -1)
        readFS(name);
    return 0;

    strcpy(FSAbsolutePath, name);
    //check if enough space for metadata and first file
    if (size < sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1)
        return 1;

    capacity = size;
    freeMemory = capacity - (sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1);

    //first dir
    strcpy(fileNames[0], ".");
    fileInfos[0][0] = 0;
    fileInfos[0][1] = 1;
    fileInfos[0][2] = 1;
    fileInfos[0][3] = 1;
    fileInfos[0][4] = 1;

    FILE *file = fopen(name, "w");

    //write size of FS to metadata
    fwrite(&size, sizeof(int), 1, file);
    updateMetadata(file);

    fclose(file);
    return 0;
}

//TODO: mutexy
void readFS(char name[]) {
    strcpy(FSAbsolutePath, name);
    FILE *file = fopen(name, "r");
    if (file == NULL)
        exit(1);

    //read FS capacity from metadata and calculate metadata size
    fread(&capacity, sizeof(int), 1, file);
    freeMemory = capacity - (sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE));

    //read inode table
    fileCount = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        //read filename
        fread(&fileNames[i][0], sizeof(char), NAME_SIZE, file);
        //read position and size
        fread(&fileInfos[i][0], sizeof(char), INFO_SIZE, file);

        freeMemory -= fileInfos[i][1];
        //check file existence
        if (fileInfos[i][1] != 0)
            fileCount++;
    }
    updateMemory();
    fclose(file);
}

//TODO: mutexy
int simplefs_unmount(char *name) {
    return remove(name);
}

//TODO: mutexy, sortowanie
void updateMemory() {
    int i = 0;
    int *ord = getSortedOrder();

    if (fileCount < 1) {
        //there are no files
        head.base = sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1;
        head.size = capacity - head.base + 1;
        head.next = NULL;
        return;
    } else {
        if (fileInfos[ord[0]][0] == sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1) {
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
            head.base = sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1;
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

//TODO: mutexy
int simplefs_defragment() {
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return -1;

    int *ord = getSortedOrder();

    if (fileCount > 0) {
        //write first file to buffer
        char *buffer = (char *) malloc(fileInfos[ord[0]][1]);
        fseek(FS, fileInfos[ord[0]][0], SEEK_SET);
        fread(buffer, sizeof(char), fileInfos[ord[0]][1], FS);

        //move first file to after metadata
        fileInfos[ord[0]][0] = sizeof(int) + MAX_FILES * (INFO_SIZE + NAME_SIZE) + 1;
        fseek(FS, fileInfos[ord[0]][0], SEEK_SET);
        fwrite(buffer, sizeof(char), fileInfos[ord[0]][1], FS);
    }
    for (int i = 1; i < fileCount; i++) {
        //move file to after previous one
        char *buffer = (char *) malloc(fileInfos[ord[i]][1]);
        fseek(FS, fileInfos[ord[i]][0], SEEK_SET);
        fread(buffer, sizeof(char), fileInfos[ord[i]][1], FS);

        fileInfos[ord[i]][0] = fileInfos[ord[i - 1]][0] + fileInfos[ord[i - 1]][1] + 1;
        fseek(FS, fileInfos[ord[i]][0], SEEK_SET);
        fwrite(buffer, sizeof(char), fileInfos[ord[i]][1], FS);
    }

    updateMetadata(FS);
    updateMemory();
    fclose(FS);
    return 0;
}

int simplefs_open(char *name, int mode) {

    char filename[NAME_SIZE];
    int fileId = check_path(name, filename);
    if (fileId < 0 || fileInfos[fileId][2] == 1) {
        //no file or file is dir
        return -1;
    }

//	if(mutex_lock(fileId) != SFSQ_OK) {
//        return SFS_UNLOCK_MUTEX_ERROR;
//    }

    return fileId;
}

int simplefs_close(int fd) {
//    if(mutex_unlock(fd) != 0) {
//        return SFS_UNLOCK_MUTEX_ERROR;
//    }
    return 0;
}

//TODO: mutexy
int simplefs_unlink(char *name) {
    char filename[NAME_SIZE];
    int fileId = check_path(name, filename);
    if (fileId == 0 || fileId == -1 || (fileInfos[fileId][2] == 1 && fileInfos[fileId][1] == 1)) {
        //no file or file is non-empty dir
        return -1;
    }

    //clear filename and info of deleted file
    fileCount--;
    freeMemory += fileInfos[fileId][1];
    memset(fileNames[fileId], 0, NAME_SIZE);
    memset(fileInfos[fileId], 0, INFO_SIZE);

    updateMemory();

    //save changes to FS
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return 1;

    updateMetadata(FS);

    fclose(FS);
    return 0;


}

int simplefs_mkdir(char *name) {
    char filename[NAME_SIZE];
    int fileId = check_path(name, filename);
    if (fileId == -1 || fileInfos[fileId][1] != 0)
        //wrong path or file exists
        return -1;

    fileId = -1 * (fileId + 2);

    if (strlen(filename) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < 1)
        return -1;

    //find free memory for file
    struct inodeFree *temp = &head;

    //open FS file
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return 1;

    int i;
    for (int i = 0; i < MAX_FILES; i++) {
        //find empty descriptor
        if (fileInfos[i][1] == 0)
            break;
    }

    //file metadata
    strcpy(fileNames[i], name);
    fileInfos[i][0] = temp->base;    //position
    fileInfos[i][1] = 1;                    //file length
    fileInfos[i][2] = 1;                    //directory
    fileInfos[i][3] = 1;                    //read permission
    fileInfos[i][4] = 1;                    //write permission

    fileCount++;
    updateMemory();
    freeMemory -= 1;

    //add dir to dir
    char buf[1];
    buf[0] = i;
    simplefs_lseek(fileno(FS), SEEK_SET, fileInfos[fileId][1]);
    //fseek(FS, SEEK_SET, fileInfos[fileId][1]);
    simplefs_write(fileId, (char *) buf, 1);

    updateMetadata(FS);
    fclose(FS);

    return 0;
}

int simplefs_creat(char *name, int mode) //name is a full path
{
    char filename[NAME_SIZE];
    int dirdesc = check_path(name, filename);
    if (dirdesc < -1) //name = path, filename = resulting filename
        return -1;

    dirdesc = -1 * (dirdesc + 2);

    printf("name: %s, mode: %d, fileCount: %d, MAX_FILES: %d, freeMemory: %d\n", name, mode, fileCount, MAX_FILES,
           freeMemory);
    if (strlen(name) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < 1)
        return -2;

    //find free memory for file
    struct inodeFree *temp = &head;

    //open FS file
    FILE *FS = fopen(FSAbsolutePath, "r+");
    if (FS == NULL)
        return 1;

    //permissions
    char readPerm = 0, writePerm = 0;
    if (mode == FS_RDONLY || mode == FS_RDWR)
        readPerm = 1;
    if (mode == FS_WRONLY || mode == FS_RDWR)
        writePerm = 1;

    int i;
    for (int i = 0; i < MAX_FILES; i++) {
        //find empty descriptor
        if (fileInfos[i][1] == 0)
            break;
    }

    //file metadata
    strcpy(fileNames[i], name);
    fileInfos[i][0] = temp->base;    //position
    fileInfos[i][1] = 1;            //file length
    fileInfos[i][2] = 0;            //not a directory
    fileInfos[i][3] = readPerm;        //read permission
    fileInfos[i][4] = writePerm;    //write permission

    fileCount++;
    updateMemory();
    freeMemory -= 1;

    //add file to dir
    char buf[1];
    buf[0] = i;
    simplefs_lseek(fileno(FS), SEEK_SET, fileInfos[dirdesc][1]);
    //fseek(FS, SEEK_SET, fileInfos[dirdesc][1]);
    simplefs_write(dirdesc, (char *) buf, 1);

    updateMetadata(FS);
    fclose(FS);

    return 0;
}

int simplefs_read(int fd, char *buf, int len) {
    //check permissions, if file is not dir, can be read and is long enoungh
    if (fileInfos[fd][1] < 1 && fileInfos[fd][2] != 0 && fileInfos[fd][3] != 1 && fileInfos[fd][1] < len)
        return -1;

    FILE *FS = fopen(FSAbsolutePath, "r+");

    //set position to file position + offset
    fseek(FS, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
    fread(buf, sizeof(char), len, FS);
    posInFile[fd] += len;

    fclose(FS);
    return 0;
}

int simplefs_write(int fd, char *buf, int len) {
    //check if there's enough memory
    if (freeMemory < sizeof(char) * len) {
        return -1;
    }

    //fd is a valid descriptor
    if (fd >= fileCount) {
        return -1;
    }

    //write permission's set
    if (!fileInfos[fd][4]) {
        return -1;
    }

    //file is not a directory
    //if(fileInfos[fd][2]) {return -1;}

    //first position after file
    int eofpos = fileInfos[fd][0] + fileInfos[fd][1];

    FILE *file = fopen(FSAbsolutePath, "r+");

    //size from current position to eof
    int restfile = fileInfos[fd][1] - fileInfos[fd][0] - posInFile[fd];
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
            fwrite(buf, sizeof(char), len, file);
            posInFile[fd] += sizeof(char) * len;
            fileInfos[fd][1] += sizeinc;

            updateMemory();
            return 0;
        }
        temp = temp->next;
    }

    //look for enough free space
    temp = &head;
    while (temp != NULL) {
        if (temp->size >= fileInfos[fd][1] + sizeinc) {
            //move file
            char *tempbuf;
            fseek(file, fileInfos[fd][0], SEEK_SET);
            fread(tempbuf, fileInfos[fd][1], 1, file);
            fseek(file, temp->base, SEEK_SET);
            fwrite(tempbuf, fileInfos[fd][1], 1, file);
            //write
            fwrite(buf, sizeof(char), len, file);

            fileInfos[fd][0] = temp->base;
            fileInfos[fd][1] += len * sizeof(char);
            posInFile[fd] += sizeof(char) * len;

            updateMemory();
            return 0;
        }

        temp = temp->next;
    }

    //no block large enough
    return -1;

}

int simplefs_lseek(int fd, int whence, int offset) {
    if (whence == SEEK_SET)
        posInFile[fd] += fileInfos[fd][0] + offset;
    else if (whence == SEEK_CUR)
        posInFile[fd] += offset;
    else if (whence == SEEK_END)
        posInFile[fd] += fileInfos[fd][0] + fileInfos[fd][1] - offset;
    else
        return -1;

    return 0;
}

int simplefs_ls(char name[]) {

    char filename[NAME_SIZE];
    if (check_path(name, filename) < 0) { //name = path
        return -1;
    }

    strcpy(filename, name); //name = filename
    for (int i = 0; i < fileCount; ++i) {
        if (strcmp(name, fileNames[i]) == 0) {
            char *buf;
            simplefs_lseek(i, SEEK_SET, 0);
            simplefs_read(i, buf, fileInfos[i][1]);

            //first byte is just flag, not id
            for (int j = 1; j < fileInfos[i][1]; ++j) {
                printf("%s \n", fileNames[buf[j]]);
            }

            return 0;
        }
    }
    //directory not found
    return -1;
}

int check_prev_dir(int prevdesc, char *dir) {
    //check if previous directory contains dir and return dirdesc
    if (prevdesc < 0) { //prev is home
        if (strcmp(dir, fileNames[0]))
            return 0;
        return -1;
    }

    char *buf;
    simplefs_lseek(prevdesc, SEEK_SET, 0);
    simplefs_read(prevdesc, buf, fileInfos[prevdesc][1]); //read prev to buf
    //first byte is just flag, not id
    for (int j = 1; j < fileInfos[prevdesc][1]; ++j) {
        if (strcmp(fileNames[buf[j]], dir) == 0) { //dir found in prev
            return buf[j];
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
    return -1;
}

#endif //FS_H