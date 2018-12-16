#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>

#define MAX_FILES     16
#define NAME_SIZE   	100
#define INFO_SIZE			5

int capacity;
int freeMemory;
int fileCount = 0;
char FSAbsolutePath[200];

char fileNames[MAX_FILES][NAME_SIZE];
//0 - position in FS
//1 - filesize
//2 - is directory
//3 - read allowed
//4 - write allowed
char fileInfos[MAX_FILES][5];

//list of free memory blocks
struct inodeFree
{
	//position in FS and size
  int base, size;
  struct inodeFree *next;
}head;

void sortDescriptors();
void updateMemory();

//TODO: concurrency
void createFS(char name[], int size)
{
  strcpy(FSAbsolutePath, name);
	//check if enough space for metadata
  if(size < sizeof(int)+MAX_FILES*(5+NAME_SIZE))
		exit(1);

  capacity = size;
  freeMemory = capacity - (sizeof(int)+MAX_FILES*(5+NAME_SIZE));
  
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

//TODO: concurrency
void readFS(char name[])
{
  strcpy(FSAbsolutePath, name);
  FILE *file = fopen(name, "r");
  if(file == NULL) 
		exit(1);
  
	//read FS capacity from metadata and calculate metadata size
  fread(&capacity, sizeof(int), 1, file);
  freeMemory = capacity - (sizeof(int)+MAX_FILES*(5+NAME_SIZE));
  
	//read inode table
	fileCount = 0;
  for(int i = 0; i < MAX_FILES; i++)
  {
		//read filename
    fread(&fileNames[i][0], sizeof(char), NAME_SIZE, file);
    //read position and size
		fread(&fileInfos[i][0], sizeof(char), 5, file);

		freeMemory -= fileInfos[i][1];
		//check file exitence
    if(fileInfos[i][1] != 0) 
			fileCount++;
  }
  updateMemory();
  fclose(file);
}

int deleteFS(char name[])
{
  return remove(name);
}

void showDir()
{
  for(int i = 0; i < MAX_FILES; i++)
  {
  	if(fileInfos[i][1] == 0) continue;
		printf("%s\n", fileNames[i]);
  }
}

void updateMemory()
{
  sortDescriptors();
  int i = 0;
  if(fileInfos[0][1] == 0)
  { 
		//there are no files                                                                                                                          
    head.base = sizeof(int)+MAX_FILES*(5+NAME_SIZE)+1;
    head.size = capacity - head.base + 1;
    head.next = NULL;
  	return;
  }
  else
  {
  	if(fileInfos[0][0] == sizeof(int)+MAX_FILES*(5+NAME_SIZE)+1)
  	{
			//first file is just after metadata
  	  while(i < fileCount-1 && fileInfos[i][0] + fileInfos[i][1] == fileInfos[i+1][0])
    	{
				//find file followed by free memory
      	i++;
    	}
			//position of first free memory segment
    	head.base = fileInfos[i][0] + fileInfos[i][1];
    	if(i == fileCount-1)
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
  	  head.base = sizeof(int)+MAX_FILES*(5+NAME_SIZE)+1;
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

void showMem()
{
	printf("Used:\n");
	for(int i = 0; i < fileCount; i++)
	{
		printf("%d at %d - %s\n", fileInfos[i][1], fileInfos[i][0], fileNames[i]);
	}
	
	struct inodeFree *temp = &head;
	printf("Free:\n");
	do
	{
		printf("%d at %d\n", temp->size, temp->base);
		temp = temp->next;
	}
	while(temp != NULL);
}

//TODO: concurrency
//sort files descriptors according to thier position in FS
void sortDescriptors()
{
  char tempI[5];
  char tempN[NAME_SIZE];
  for(int i = 0; i < fileCount; i++)
  {
    int min = i;
  	for(int j = i; j < MAX_FILES; j++)
  	{
  		if(fileInfos[j][0] != 0 
				&& (fileInfos[min][0] == 0 || fileInfos[min][0] > fileInfos[j][0])) 
					min = j;
  	}

  	strncpy(tempI, fileInfos[i], 5);
  	strncpy(fileInfos[i], fileInfos[min], 5);
  	strncpy(fileInfos[min], tempI, 5);
  	
  	strncpy(tempN, fileNames[i], NAME_SIZE);
  	strncpy(fileNames[i], fileNames[min], NAME_SIZE);
  	strncpy(fileNames[min], tempN, NAME_SIZE);
  }
}

//TODO: concurrency
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
  	fileInfos[0][0] = sizeof(int)+MAX_FILES*(5+NAME_SIZE)+1;
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
    fwrite(&fileInfos[i][0], sizeof(char), 5, FS);
  }
	
	fclose(FS);
	updateMemory();
}

//deprecated
void writeFileToFS(char name[])
{
	if(strlen(name) > NAME_SIZE) return;
	readFS(FSAbsolutePath);

	if(fileCount >= MAX_FILES) return;
	struct stat st;
	if(stat(name, &st) != 0) return;
	if(st.st_size > freeMemory) return;
	
	//check if file already exists
	for(int i = 0; i < fileCount; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0) return;
	}
 	
	//find free memory for file
 	struct inodeFree *temp = &head;
	while(temp != NULL)
	{
		if(temp->size >= st.st_size)break;
		temp = temp->next;
	}
	if(temp->size < st.st_size) 
	{
		defragment();
		temp = &head;
	}
	
	FILE *file = fopen(name, "r");
  if(file == NULL) exit(1);
 	
 	FILE *FS = fopen(FSAbsolutePath, "r+");
  if(FS == NULL) exit(1);
	
	//file metadata
	strcpy(fileNames[fileCount], name);
	fileInfos[fileCount][0] = temp->base;
	fileInfos[fileCount][1] = st.st_size;
	
	//write file to FS
	char *buffer = malloc(st.st_size);
	fread(buffer, sizeof(char), st.st_size, file);
	fseek(FS, temp->base, SEEK_SET);
	fwrite(buffer, sizeof(char), st.st_size, FS);
	
	fileCount++;
	sortDescriptors();
	updateMemory();
	freeMemory -= st.st_size;
	
	//update metadata
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
  {
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), 5, FS);
  }
	
	fclose(file);
	fclose(FS);
}

//deprecated
//write file from FS to host
void writeFileFromFS(char name[])
{
	if(strlen(name) > NAME_SIZE) 
		return;
	readFS(FSAbsolutePath);
	
	for(int i = 0; i < fileCount; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0)
	  {
	    FILE *file = fopen(name, "w");
  		if(file == NULL) exit(1);
 	
 			FILE *FS = fopen(FSAbsolutePath, "r");
  		if(FS == NULL) exit(1);
  		
  		char *buffer = malloc(fileInfos[i][1]);
  		fseek(FS, fileInfos[i][0], SEEK_SET);
			fread(buffer, sizeof(char), fileInfos[i][1], FS);
			fwrite(buffer, sizeof(char), fileInfos[i][1], file);
  		
  		fclose(file);
  		fclose(FS);
	  	return;
	  }
	}
}

int simplefs_open(char* name, int mode)
{
	//TODO
}

//TODO: concurrency and return status
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
		  memset(fileInfos[i], 0, 5);
		  
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
    		fwrite(&fileInfos[i][0], sizeof(char), 5, FS);
  		}
	
			fclose(FS);
			return;
		}
	}
}

int simplefs_mkdir(char* name)
{
	//TODO
}

int simplefs_creat(char* name int mode)
{
	//TODO
}

int simplefs_read(int fd, char* buf, int len)
{
	//TODO
}

int simplefs_write(int fd, char* buf, int len)
{
	//TODO
}

int simplefs_lseek(int fd, int whence, int offset)
{
	//TODO
}

int main(int argc, char** argv)
{
  //main tylko do testowania
  return 0;
}
