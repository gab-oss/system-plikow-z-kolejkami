#ifndef FS_H
#define FS_H

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>

// Global constants
#define MAX_FILES     16
#define NAME_SIZE   	100
#define INFO_SIZE			5

// Argument flags
#define FS_RDONLY               1	// 001 - Read only
#define FS_WRONLY               2	// 010 - Write only
#define FS_RDWR                 3	// 011 - Read and write

// Error codes
#define SFS_LOCK_MUTEX_ERROR    -1
#define SFS_UNLOCK_MUTEX_ERROR  -2

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
struct inodeFree
{
	//position in FS and size
  int base, size;
  struct inodeFree *next;
}head;

void sortDescriptors();
void updateMemory();

//TODO: mutexy
int simplefs_mount(char* name, int size) {
{
  strcpy(FSAbsolutePath, name);
	//check if enough space for metadata
  if(size < sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE))
		exit(1);

  capacity = size;
  freeMemory = capacity - (sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE));
  
  FILE *file = fopen(name, "w");
  
	//write size of FS to metadata
  fwrite(&size, sizeof(int), 1, file);
  
	char buf = 0;
	//fill FS with 0s
  for(int i = 0; i < size - sizeof(int); i++)
  {
  	fwrite(&buf, sizeof(char), sizeof(buf), file);
  }
  fclose(file);
}

//TODO: mutexy
void readFS(char name[])
{
  strcpy(FSAbsolutePath, name);
  FILE *file = fopen(name, "r");    
  if(file == NULL) 
		exit(1);
  
	//read FS capacity from metadata and calculate metadata size
  fread(&capacity, sizeof(int), 1, file);
  freeMemory = capacity - (sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE));
  
	//read inode table
	fileCount = 0;
  for(int i = 0; i < MAX_FILES; i++)
  {
		//read filename
    fread(&fileNames[i][0], sizeof(char), NAME_SIZE, file);
    //read position and size
		fread(&fileInfos[i][0], sizeof(char), INFO_SIZE, file);

		freeMemory -= fileInfos[i][1];
		//check file existence
    if(fileInfos[i][1] != 0) 
			fileCount++;
  }
  updateMemory();
  fclose(file);
}

//TODO: mutexy
int simplefs_unmount(char* name)
{
  return remove(name);
}

//TODO: mutexy
void updateMemory()
{
  sortDescriptors();
  int i = 0;
  if(fileInfos[0][1] == 0)
  { 
		//there are no files                                                                                                                          
    head.base = sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE)+1;
    head.size = capacity - head.base + 1;
    head.next = NULL;
  	return;
  }
  else
  {
  	if(fileInfos[0][0] == sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE)+1)
  	{
			//first file is just after metadata
  	  while(i < fileCount-1 && fileInfos[i][0] + fileInfos[i][1] == fileInfos[i+1][0])
    	{
				//find file followed by free memory
      	i++;
    	}
			//position of first free memory segment
    	head.base = fileInfos[i][0] + fileInfos[i][1];
    	if (i == fileCount-1)
    	{
				//all free memory follows last file
    		head.size = capacity - head.base + 1;
    		head.next = NULL;
      	return;
    	}
    	else
    	{
				//there is free memory between files
    	  head.size = fileInfos[i+1][0] - head.base;
    	  head.next = NULL;
    	}
  	}
  	else
  	{
			//there is free memory between metadata and first file
  	  head.base = sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE)+1;
  	  head.size = fileInfos[0][0] - head.base;
  	}
  }
  
  struct inodeFree *prev = &head;
  //add free memory segments to list
	for(i; i < fileCount; i++)
  {
		if(fileInfos[i][0] + fileInfos[i][1] == fileInfos[i+1][0])
			//there is no free memory between files
			continue;
		
		struct inodeFree *temp = malloc(sizeof(struct inodeFree));
		temp->base = fileInfos[i][0] + fileInfos[i][1];
		if(temp->base > capacity) 
			//no free memory after last file
			break;
		
		if(i!=MAX_FILES-1 && fileInfos[i+1][1] != 0)
		{
			//there are files after current one
			temp->size = fileInfos[i+1][0] - temp->base;
		}
		else
		{
			//there is only free memory after current file
			temp->size = capacity - temp->base + 1;
		}
		
		temp->next = NULL;
		prev->next = temp;
		prev = temp;
  }
}

//TODO: mutexy
//sort files descriptors according to thier position in FS
void sortDescriptors()
{
  char tempI[INFO_SIZE];
  char tempN[NAME_SIZE];
	char tempP;
  for(int i = 0; i < fileCount; i++)
  {
    int min = i;
  	for(int j = i; j < MAX_FILES; j++)
  	{
  		if(fileInfos[j][0] != 0 
				&& (fileInfos[min][0] == 0 || fileInfos[min][0] > fileInfos[j][0])) 
					min = j;
  	}

		//switch elements of fileInfos
  	strncpy(tempI, fileInfos[i], INFO_SIZE);
  	strncpy(fileInfos[i], fileInfos[min], INFO_SIZE);
  	strncpy(fileInfos[min], tempI, INFO_SIZE);
  	
		//switch elements of fileNames
  	strncpy(tempN, fileNames[i], NAME_SIZE);
  	strncpy(fileNames[i], fileNames[min], NAME_SIZE);
  	strncpy(fileNames[min], tempN, NAME_SIZE);

		//switch elements of posInFile
		tempP = posInFile[i];
  	posInFile[i] = posInFile[min];
  	posInFile[min] = tempP;
  }
}

//TODO: mutexy
void defragment()
{
	FILE *FS = fopen(FSAbsolutePath, "r+");
  if(FS == NULL) 
		exit(1);
	
  if(fileCount > 0)
  {
		//write first file to buffer
    char *buffer = malloc(fileInfos[0][1]);
		fseek(FS, fileInfos[0][0], SEEK_SET);
		fread(buffer, sizeof(char), fileInfos[0][1], FS);
		
		//move first file to after metadata
  	fileInfos[0][0] = sizeof(int)+MAX_FILES*(INFO_SIZE+NAME_SIZE)+1;
		fseek(FS, fileInfos[0][0], SEEK_SET);
		fwrite(buffer, sizeof(char), fileInfos[0][1], FS);
  }
	for(int i = 1; i < fileCount; i++)
	{
		//move file to after previous one
		char *buffer = malloc(fileInfos[i][1]);
		fseek(FS, fileInfos[i][0], SEEK_SET);
		fread(buffer, sizeof(char), fileInfos[i][1], FS);
		
  	fileInfos[i][0] = fileInfos[i-1][0]+fileInfos[i-1][1]+1;
		fseek(FS, fileInfos[i][0], SEEK_SET);
		fwrite(buffer, sizeof(char), fileInfos[i][1], FS);
	}

	//update metadata	
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
  {
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), INFO_SIZE, FS);
  }
	
	fclose(FS);
	updateMemory();
}

int simplefs_open(char* name, int mode) {
    int fd = 1; // @TODO
    if(mutex_lock(fd) != 0) {
        return SFS_LOCK_MUTEX_ERROR;
    }
		//set position to beginning of file
		posInFile[fd] = 0;
    return fd;
}

int simplefs_close(int fd) {
    if(mutex_unlock(fd) != 0) {
        return SFS_UNLOCK_MUTEX_ERROR;
    }
    return 0;
}

//TODO: mutexy
int simplefs_unlink(char* name)
{
	for(int i = 0; i < MAX_FILES; i++)
	{
		if(strcmp(name, fileNames[i]) == 0)
		{
			//clear filename and info of deleted file
		  fileCount--;
		  freeMemory += fileInfos[i][1];
		  memset(fileNames[i], 0, NAME_SIZE);
		  memset(fileInfos[i], 0, INFO_SIZE);
		  
		  sortDescriptors();
		  updateMemory();
		  
			//save changes to FS
		  FILE *FS = fopen(FSAbsolutePath, "r+");
  		if(FS == NULL) 
				exit(1);
		  
			//update metadata
		  fseek(FS, sizeof(int), SEEK_SET);
			for(int i = 0; i < MAX_FILES; i++)
  		{
    		fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    		fwrite(&fileInfos[i][0], sizeof(char), INFO_SIZE, FS);
  		}
	
			fclose(FS);
			return 0;
		}
	}
}

int simplefs_mkdir(char* name)
{
	if(strlen(name) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < 1)
		return -1;

	//check if directory already exists
	for(int i = 0; i < fileCount; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0 && fileInfos[i][2] == 1) 
		//mutex jak w open?
			return i;
	}

	//find free memory for file
	struct inodeFree *temp = &head;

	//open FS file
	FILE *FS = fopen(FSAbsolutePath, "r+");
  if(FS == NULL) 
		exit(1);

	//file metadata
	strcpy(fileNames[fileCount], name);
	fileInfos[fileCount][0] = temp->base;	//position
	fileInfos[fileCount][1] = 1;					//file length
	fileInfos[fileCount][2] = 1;					//directory
	fileInfos[fileCount][3] = 1;					//read permission
	fileInfos[fileCount][4] = 1;					//write permission

	fileCount++;
	sortDescriptors();
	updateMemory();
	freeMemory -= 1;
	
	//update metadata
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
	{
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), INFO_SIZE, FS);
  }
	fclose(FS);

	return 0;
}

int simplefs_creat(char* name, int mode)
{
	if(strlen(name) > NAME_SIZE || fileCount >= MAX_FILES || freeMemory < 1)
		return -1;

	//check if file already exists
	for(int i = 0; i < fileCount; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0 && fileInfos[i][2] == 0) 
		//mutex jak w open?
			return i;
	}

	//find free memory for file
	struct inodeFree *temp = &head;

	//open FS file
	FILE *FS = fopen(FSAbsolutePath, "r+");
  if(FS == NULL) 
		exit(1);
	
	//permissions
	char readPerm = 0, writePerm = 0;
	if(mode == FS_RDONLY || mode == FS_RDWR)
		readPerm = 1;
	if(mode == FS_WRONLY || mode == FS_RDWR)
		writePerm = 1;

	//file metadata
	strcpy(fileNames[fileCount], name);
	fileInfos[fileCount][0] = temp->base;	//position
	fileInfos[fileCount][1] = 1;					//file length
	fileInfos[fileCount][2] = 0;					//not a directory
	fileInfos[fileCount][3] = readPerm;		//read permission
	fileInfos[fileCount][4] = writePerm;	//write permission

	fileCount++;
	sortDescriptors();
	updateMemory();
	freeMemory -= 1;
	
	//update metadata
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
	{
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), INFO_SIZE, FS);
  }
	fclose(FS);

	return 0;
}

int simplefs_read(int fd, char* buf, int len)
{	
	//check permissions, if file is not dir, can be read and is long enoungh
	if(fileInfos[fd][1] < 1 && fileInfos[fd][2] != 0 && fileInfos[fd][3] != 1 && fileInfos[fd][1] < len) 
		return -1;
	
	FILE *FS = fopen(FSAbsolutePath, "r+");

	//set position to file position + offset
	fseek(FS, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
	fread(buf, sizeof(char), len, FS);
	posInFile[fd] += len;

	fclose(FS);
	return 0;
}

//0 - position in FS
//1 - filesize
//2 - is directory
//3 - read allowed
//4 - write allowed

int simplefs_write(int fd, char* buf, int len)
{

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
	if(fileInfos[fd][2]) {
		return -1;
	}

	//first position after file
	int eofpos = fileInfos[fd][0] + fileInfos[fd][1];

  FILE * file = fopen(FSAbsolutePath, "r+");

	//size from current position to eof
	int restfile = fileInfos[fd][1] - fileInfos[fd][0] - posInFile;
	//check if there's enough space right after the file
	struct inodeFree * temp = &head;
	while (temp != NULL) {
		if (temp->base == eofpos && temp->size >= sizeof(char) * len) {
			//temporary save the part of the file after posInFile
			char * restbuf;
			fseek(file, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
			fread(restbuf, restfile, 1, file);
			//write
			fwrite(buf, sizeof(char), len, file);
			fwrite(restbuf, restfile, 1, file);
			posInFile[fd] += sizeof(char) * len;
			return 0;
		}
		temp = temp->next;
	}

	//look for enough free space
	temp = &head;
	while (temp != NULL) {
		if (inode.size > fileInfos[fd][1] + buf * len){
			//move file
			char * tempbuf;
			char * restbuf;
			fseek(file, fileInfos[fd][0], SEEK_SET);
			fread(tempbuf, fileInfos[fd][1], 1, file);
			fseek(file, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
			fread(restbuf, restfile, 1, file);
			fseek(file, temp->base, SEEK_SET);
			fwrite(tempbuf, fileInfos[fd][1], 1, file);
			//write
			fwrite(buf, sizeof(char), len, file);
			//add rest of the file
			fwrite(restbuf, restfile, 1, file);

			fileInfos[fd][0] = temp->base;
			fileInfos[fd][1] += len * sizeof(char);
			posInFile[fd] += sizeof(char) * len;

			return 0;
		}

		temp = temp->next;
	}

	//no block large enough
	return -1;

}

int simplefs_lseek(int fd, int whence, int offset)
{

	if (whence == SEEK_SET)
		fseek(file, fileInfos[fd][0], SEEK_SET);
	else if (whence == SEEK_CUR)
		fseek(file, fileInfos[fd][0] + posInFile[fd], SEEK_SET);
	else if (whence == SEEK_END) 
		fseek (file, fileInfos[fd][0] + fileInfos[fd][1], SEEK_SET);
	else
		return -1;
	
	return 0;
}

int simplefs_ls(char name[]){
	return 0;
}

#endif //FS_H