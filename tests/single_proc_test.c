#include <stdio.h>
#include <string.h>
#include "../fs.h"

#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KRESET "\x1B[0m"

#define TRUE 1
#define FALSE 0
/*
* Runs single process test for FS 
* File for mounting FS and size can be given as startup arguments
* Default mount params are : 
*   File: /tmp/test.fs
*   Size: 20000
*
*/
void print_test_res(int res, char *err_msg)
{
    if (res < 0)
    {
        printf(KRED "\t%s (error code: %d)\n" KRESET, err_msg, res);
    }
    else
        printf(KGRN "\t[OK]\n" KRESET);
}
void print_red(const char *msg){printf(KRED "\t%s\n" KRESET, msg);}
void print_green(const char *msg){printf(KGRN "\t%s\n" KRESET, msg);}

void run_tests(char* fname, char no_creat)
{ // when flag no creat is true files won't be created in FS
  // flag is used when running test on re-mounted FS
    char wr_buf[11] = "abcdefghij";
    char rd_buf[11];
    printf("Creating using simplefs_creat:");
    int r;
    if(no_creat){
        printf("No creat is on.");
    }
    else {
        r = simplefs_creat(fname, FS_RDWR);
        print_test_res(r, "Creation failed!");
    }
    
    //WRITE
    printf("Write tests:\n");
    int fd = simplefs_open(fname, FS_WRONLY);
    printf("Open FS_WRONLY: ");
    print_test_res(fd, "Open failed!");
    r = simplefs_write(fd, wr_buf, 10);
    if( r != 10){
        char msg[100];
        sprintf(msg,"Write return value incorrect expected 10 - written %d\n",r);
        print_red(msg);
    }
    else
        print_green("Return value OK\n");
    simplefs_close(fd);
    
    //READ
    printf("Read test: \nOpen:");
    fd = simplefs_open(fname, FS_RDONLY);
    print_test_res(fd,"Open FS_RDONLY failed!");
    r = simplefs_read(fd, rd_buf, 10);
    if( r != 10){
        char msg[100];
        sprintf(msg,"Return value incorrect expected 10 - read %d\n",r);
        print_red(msg);
    }
    else
        print_green("Return value OK\n");
    rd_buf[r] = '\0';
    if(strcmp(rd_buf, wr_buf)!=0){
        char msg[256];
        sprintf(msg,"Read error: expected %s read: %s \n",wr_buf, rd_buf);
        print_red(msg);
    }
    else
        print_green("Read OK\n");
    r = simplefs_read(fd, rd_buf, 10);
    if( r != 0){
        char msg[100];
        sprintf(msg,"Read at file end incorrect retunr value expected 0 - read %d\n",r);
        print_red(msg);
    }
    else
        print_green("Read at file end OK\n");
    simplefs_close(fd);

     // READ CHUNK 
    printf("Read chunk test: \nOpen:");
    fd = simplefs_open(fname, FS_RDONLY);
    print_test_res(fd,"Open FS_RDONLY failed!");
    for (int k = 0; k < 2; ++k)
    {
        r = simplefs_read(fd, rd_buf, 5);
        if (r != 5)
        {
            char msg[100];
            sprintf(msg, "Return value incorrect expected 5 - read %d\n", r);
            print_red(msg);
        }
        else
            print_green("Return value OK\n");
        rd_buf[r] = '\0';
        int cmp_res;
        if(k==0)
            cmp_res = strcmp(rd_buf, "abcde");
        else //2nd read
            cmp_res = strcmp(rd_buf, "fghij");

        if (cmp_res != 0)
        {
            char msg[256];
            sprintf(msg, "Read error: expected abcde read: %s \n", rd_buf);
            print_red(msg);
        }
        else
            print_green("Read OK\n");
    }
    simplefs_close(fd);
}

void run_open_tests()
{
    //OPEN
    printf("Test open non existing file: \n Mode FS_RDONLY: ");
    int fd = simplefs_open("./zzz", FS_RDONLY);
    if( fd < 0 )
        print_green("[OK]");
    else 
        print_red("Failed! Expected error return value!");
    printf("\nMode FS_WRONLY: ");
    fd = simplefs_open("./zzz", FS_RDONLY);
    if( fd < 0 )
        print_green("[OK]");
    else 
        print_red("Failed! Expected error return value!");
    printf("\nMode FS_RDWR: ");
    fd = simplefs_open("./zzz", FS_RDONLY);
    if( fd < 0 )
        print_green("[OK]");
    else 
        print_red("Failed! Expected error return value!\n");
}

void run_remove_tests()
{
    printf("Test remove non existing file: \n");
    int r = simplefs_unlink("./sss");
    if( r < 0 )
        print_green("[OK]");
    else 
        print_red("Failed! Expected error return value!\n");
    
    printf("Remove existing file: ");
    simplefs_creat("./abc", FS_RDWR);
    r = simplefs_unlink("./abc");
    if(r == 0)
        print_green("[OK]");
    else
    {
        char msg[256];
        sprintf(msg,"Failed! Expected 0 return value got %d\n", r);
        print_red(msg);
    }

    printf("Try to remove non empty dir: ");
    r = simplefs_unlink("./dir");
    if( r < 0 )
        print_green("[OK]");
    else 
        print_red("Failed! Expected error return value!\n");
    
    printf("Try to remove empty dir: ");
    simplefs_mkdir("./d1");
    r = simplefs_unlink("./d1");
    if(r == 0)
        print_green("[OK]");
    else
    {
        char msg[256];
        sprintf(msg,"Failed! Expected 0 return value got %d\n", r);
        print_red(msg);
    }
}

int main(int argc, char **argv)
{
    char *mount_path = (argc > 1) ? argv[1] : "/tmp/test.fs";
    int fs_size = (argc > 2) ? atoi(argv[2]) : 20000;
    printf("Running FS test for single process\n");

    //MOUNT
    printf("Mounting FS in: %s, Size: %d", mount_path, fs_size);
    int r = simplefs_mount(mount_path, fs_size);
    print_test_res(r, "Mount error! Exiting");
    if(r < 0)
        exit(-1);
    
    printf("***Running tests in / directory***\n");
    run_tests("./a.txt", FALSE);
    
    printf("***Running tests in / subdirectory***\n");
    r = simplefs_mkdir("./dir");
    print_test_res(r, "Couldn't create dieectory!");
    run_tests("./dir/b.txt", FALSE);   

    printf("***Running simplefs_open tests***\n");
    run_open_tests();

    printf("***Running simplefs_unlink tests***\n");
    run_remove_tests();

    printf("***Unmounting FS***");
    simplefs_unmount(mount_path);

    //RE-MOUNT
    printf("\nRe-mounting FS in: %s, Size: %d", mount_path, fs_size);
    r = simplefs_mount(mount_path, fs_size);
    print_test_res(r, "Mount error! Exiting");
    if(r < 0)
        exit(-1);
        
    printf("\n***************************************\n");
    printf("***    Run tests on remounted FS    ***");
    printf("***************************************\n");
    // Re-run test on FS mounted from existing file
    printf("***Running tests in / directory***\n");
    run_tests("./a.txt", TRUE);
    
    printf("***Running tests in / subdirectory***\n");
    r = simplefs_mkdir("./dir");
    print_test_res(r, "Couldn't create dieectory!");
    run_tests("./dir/b.txt", TRUE);   

    printf("***Running simplefs_open tests***\n");
    run_open_tests();

    printf("***Running simplefs_unlink tests***\n");
    run_remove_tests();
    printf("Tests finished!\n");
}

