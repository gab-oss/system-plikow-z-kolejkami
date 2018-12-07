#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>

#define MAX_FILES     8
#define NAME_SIZE   	4

int SIZE;
int freeMem;
int files = 0;
char FSname[30];

char fileNames[MAX_FILES][NAME_SIZE];
char fileInfos[MAX_FILES][2];

struct inodeFree
{
  int base, size;
  struct inodeFree *next;
}head;

void sortDescriptors();
void updateMem();

void createFS(char name[], int size)
{
  strcpy(FSname, name);
  if(size < sizeof(int)+MAX_FILES*(2+NAME_SIZE))
    exit(1);
  SIZE = size;
  freeMem = SIZE - (sizeof(int)+MAX_FILES*(2+NAME_SIZE));
  
  FILE *file = fopen(name, "w");
  
  fwrite(&size, sizeof(int), 1, file);
  char buf = 0;
  for(int i = 0; i < size - sizeof(int); i++)
  {
  	fwrite(&buf, sizeof(char), sizeof(buf), file);
  }
  fclose(file);
}

void readFS(char name[])
{
  strcpy(FSname, name);
  FILE *file = fopen(name, "r");
  if(file == NULL) exit(1);
  
  fread(&SIZE, sizeof(int), 1, file);
  freeMem = SIZE - (sizeof(int)+MAX_FILES*(2+NAME_SIZE));
  files = 0;
  for(int i = 0; i < MAX_FILES; i++)
  {
    fread(&fileNames[i][0], sizeof(char), NAME_SIZE, file);
    fread(&fileInfos[i][0], sizeof(char), 2, file);
    freeMem -= fileInfos[i][1];
    if(fileInfos[i][1] != 0) files++;
  }
  updateMem();
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

void updateMem()
{
  sortDescriptors();
  int i = 0;
  if(fileInfos[0][1] == 0)
  {                                                                                                                           
    head.base = sizeof(int)+MAX_FILES*(2+NAME_SIZE)+1;
    head.size = SIZE - head.base + 1;
    head.next = NULL;
  	return;
  }
  else
  {
  	if(fileInfos[0][0] == sizeof(int)+MAX_FILES*(2+NAME_SIZE)+1)
  	{
  	  while(i < files-1 && fileInfos[i][0] + fileInfos[i][1] == fileInfos[i+1][0])
    	{
      	i++;
    	}
    	head.base = fileInfos[i][0] + fileInfos[i][1];
    	if(i == files-1)
    	{
    		head.size = SIZE - head.base + 1;
    		head.next = NULL;
      	return;
    	}
    	else
    	{
    	  head.size = fileInfos[i+1][0] - head.base;
    	  head.next = NULL;
    	}
  	}
  	else
  	{
  	  head.base = sizeof(int)+MAX_FILES*(2+NAME_SIZE)+1;
  	  head.size = fileInfos[0][0] - head.base;
  	}
  }
  
  struct inodeFree *prev = &head;
  for(i; i < files; i++)
  {
  	 if(fileInfos[i][0] + fileInfos[i][1] == fileInfos[i+1][0])
  	 continue;
  	 struct inodeFree *temp = malloc(sizeof(struct inodeFree));
  	 temp->base = fileInfos[i][0] + fileInfos[i][1];
  	 if(temp->base > SIZE) break;
     if(i!=7 && fileInfos[i+1][1] != 0)
     {
       temp->size = fileInfos[i+1][0] - temp->base;
     }
     else
     {
       temp->size = SIZE - temp->base + 1;
     }
     
     temp->next = NULL;
     prev->next = temp;
     prev = temp;
  }
}

void showMem()
{
	printf("Used:\n");
	for(int i = 0; i < files; i++)
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

void sortDescriptors()
{
  char tempI[2];
  char tempN[NAME_SIZE];
  for(int i = 0; i < files; i++)
  {
    int min = i;
  	for(int j = i; j < MAX_FILES; j++)
  	{
  		if(fileInfos[j][0] != 0 && (fileInfos[min][0] == 0 || fileInfos[min][0] > fileInfos[j][0])) min = j;
  	}
  	strncpy(tempI, fileInfos[i], 2);
  	strncpy(fileInfos[i], fileInfos[min], 2);
  	strncpy(fileInfos[min], tempI, 2);
  	
  	strncpy(tempN, fileNames[i], NAME_SIZE);
  	strncpy(fileNames[i], fileNames[min], NAME_SIZE);
  	strncpy(fileNames[min], tempN, NAME_SIZE);
  }
}

void defragment()
{
	FILE *FS = fopen(FSname, "r+");
  if(FS == NULL) exit(1);
	
  if(files > 0)
  {
    char *buffer = malloc(fileInfos[0][1]);
		fseek(FS, fileInfos[0][0], SEEK_SET);
		fread(buffer, sizeof(char), fileInfos[0][1], FS);
		
  	fileInfos[0][0] = sizeof(int)+MAX_FILES*(2+NAME_SIZE)+1;
		fseek(FS, fileInfos[0][0], SEEK_SET);
		fwrite(buffer, sizeof(char), fileInfos[0][1], FS);
  }
	for(int i = 1; i < files; i++)
	{
		char *buffer = malloc(fileInfos[i][1]);
		fseek(FS, fileInfos[i][0], SEEK_SET);
		fread(buffer, sizeof(char), fileInfos[i][1], FS);
		
  	fileInfos[i][0] = fileInfos[i-1][0]+fileInfos[i-1][1]+1;
		fseek(FS, fileInfos[i][0], SEEK_SET);
		fwrite(buffer, sizeof(char), fileInfos[i][1], FS);
	}
		
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
  {
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), 2, FS);
  }
	
	fclose(FS);
	updateMem();
}

void writeFileToFS(char name[])
{
	if(strlen(name) > NAME_SIZE) return;
	readFS(FSname);

	if(files >= MAX_FILES) return;
	struct stat st;
	if(stat(name, &st) != 0) return;
	if(st.st_size > freeMem) return;
	
	for(int i = 0; i < files; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0) return;
	}
 	
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
 	
 	FILE *FS = fopen(FSname, "r+");
  if(FS == NULL) exit(1);
	
	strcpy(fileNames[files], name);
	fileInfos[files][0] = temp->base;
	fileInfos[files][1] = st.st_size;
	
	char *buffer = malloc(st.st_size);
	fread(buffer, sizeof(char), st.st_size, file);
	fseek(FS, temp->base, SEEK_SET);
	fwrite(buffer, sizeof(char), st.st_size, FS);
	
	files++;
	sortDescriptors();
	updateMem();
	freeMem -= st.st_size;
	
	fseek(FS, sizeof(int), SEEK_SET);
	for(int i = 0; i < MAX_FILES; i++)
  {
    fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    fwrite(&fileInfos[i][0], sizeof(char), 2, FS);
  }
	
	fclose(file);
	fclose(FS);
}

void writeFileFromFS(char name[])
{
	if(strlen(name) > NAME_SIZE) return;
	readFS(FSname);
	
	for(int i = 0; i < files; i++)
	{
	  if(strcmp(name, fileNames[i]) == 0)
	  {
	    FILE *file = fopen(name, "w");
  		if(file == NULL) exit(1);
 	
 			FILE *FS = fopen(FSname, "r");
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

void deleteFile(char name[])
{
	for(int i = 0; i < MAX_FILES; i++)
	{
		if(strcmp(name, fileNames[i]) == 0)
		{
		  files--;
		  freeMem += fileInfos[i][1];
		  memset(fileNames[i], 0, NAME_SIZE);
		  memset(fileInfos[i], 0, 2);
		  
		  sortDescriptors();
		  updateMem();
		  
		  FILE *FS = fopen(FSname, "r+");
  		if(FS == NULL) exit(1);
		  
		  fseek(FS, sizeof(int), SEEK_SET);
			for(int i = 0; i < MAX_FILES; i++)
  		{
    		fwrite(&fileNames[i][0], sizeof(char), NAME_SIZE, FS);
    		fwrite(&fileInfos[i][0], sizeof(char), 2, FS);
  		}
	
			fclose(FS);
			return;
		}
	}
}

int main(int argc, char** argv)
{
  extern char* optarg;
  char opt;
  
  while((opt = getopt(argc, argv, "c:r:mdi:o:X:x:")) != -1) 
  {
    switch(opt) 
    {
      case 'c':
        createFS(optarg, 100);
        break;
      case 'r':
        readFS(optarg);
        break;
      case 'm':
        showMem();
        break;
      case 'd':
        showDir();
        break;
      case 'i':
        writeFileToFS(optarg);
        break;
      case 'o':
        writeFileFromFS(optarg);
        break;
      case 'X':
        deleteFS(optarg);
        break;
      case 'x':
        deleteFile(optarg);
        break;
    }
  }
  
  return 0;
}
