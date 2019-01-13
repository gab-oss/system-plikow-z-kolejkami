#include <iostream>
#include <string>
#include <climits>
#include <stdio.h>
#include "StringSplitter.h"
#include "../fs.h"

using namespace std;
//
// Simple terminal for filesystem provides bash-like interface
//

int main(int argc, char** argv)
{
    string PROMPT = ">";
    string input; // commandd read from cin
    string pwd ="./"; // working directory
    while(true)
    {
        cout<<endl<<pwd<<PROMPT;
        getline(cin, input);
        vector<string> cmd_parts = splitWithDelimiter(input, ' ');
        string cmd = cmd_parts[0];
        if(cmd == "touch")
        { // touch <fname>
            if( cmd_parts.size() < 2)
            {
                cout<<"Usage: touch <path>";
                continue;
            }
            string path = pwd +cmd_parts[1];
            int fd = simplefs_creat((char*)path.c_str(), FS_CREAT);
            if( fd < 0)
                cout<<"Couldn't creat file "<<path<<" (error: "<<fd<<")";
            else
                simplefs_close(fd);
        }
        else if(cmd == "mkdir")
        {//mkdir <path>
            if( cmd_parts.size() < 2)
            {
                cout<<"Usage: mkdir <path>";
                continue;
            }
            string path = pwd +cmd_parts[1];
            int r = simplefs_mkdir((char*)path.c_str()); 
            if( r < 0)
                cout<<"Couldn't create directory"<<path<<"(error: "<<r<<")";
        }
        else if(cmd == "rm")
        {
            if( cmd_parts.size() < 2)
            {
                cout<<"Usage: rm <path>";
                continue;
            }
            string path = pwd +cmd_parts[1];
            if (simplefs_unlink((char *)path.c_str()) < 0)
                cout<<"rm error";
        }
        else if(cmd == "write")
        {// write <path> <line>
            if(cmd_parts.size() < 3)
            {
                cout<<"Usage: write <path> <line>";
            }
            string path = pwd +cmd_parts[1];
            int fd = simplefs_open((char*)path.c_str(), FS_WRONLY);
            if(fd < 0)
            {
                cout<<"Counldn't open "<<cmd_parts[1];
                continue;
            }
            string line = cmd_parts[2];
            for(int i=3; i<cmd_parts.size();++i)
                line+=" "+cmd_parts[i];
            if(simplefs_write(fd, (char*)line.c_str(), line.size()) < 0)    
                cout<<"Write error";
            simplefs_close(fd);
        }

        else if(cmd == "cat")
        {// cat <path> [whence] [offset]
            if(cmd_parts.size() < 2)
            {
                cout<<"Usage: cat <path> [whence] [offset]";
                continue;
            }
            string path = pwd +cmd_parts[1];
            int fd = simplefs_open((char*)path.c_str(), FS_RDONLY);
            if( fd < 0)
            {
                cout<<"Couldn't open "<<cmd_parts[1];
                continue;
            }
            int whence=INT_MIN, offset=INT_MIN;
            try{
            for (int i = 2; (i < cmd_parts.size() && i < 4); ++i)
            {
                if(i == 2)
                    whence = stoi(cmd_parts[i]);
                if(i == 3)
                    offset = stoi(cmd_parts[i]);
            }
            }
            catch(...)
            {
                cout <<"Invalid options";
                simplefs_close(fd);
                continue;
            }

            if(offset != INT_MIN && whence != INT_MIN)
                simplefs_lseek(fd, whence, offset);
            int read_size = 100;
            char buf[read_size];
            int rval;
            while((rval = simplefs_read(fd, buf, read_size)) > 0)
                printf("%s", buf);
            if(rval < 0 )
                cout<<"Read error!";
            simplefs_close(fd);
        }

        else if(cmd == "ls")
        {
            //TO DO
        }
        
        else if(cmd == "unmount")
        { // unmount <fs file path>
            if(cmd_parts.size() < 2)
            {
                cout<<"Usage: unmount <fs file path>";
                continue;
            }
            if(simplefs_unmount((char*)cmd_parts[1].c_str()) < 0)
                cout<<"Unmount error";
        }

        else if(cmd == "mount")
        {
            if(cmd_parts.size() < 3)
            {
                //cout<<"Usage: mount <fs file path> <size> <inodes>";
                cout<<"Usage: mount <fs file path> <size>";
                continue;
            }
            int size = stoi(cmd_parts[2]);
            // int inodes = stoi(cmd_parts[3]);
            if(simplefs_mount((char*)cmd_parts[1].c_str(), size) < 0)
                cout<<"Mount error";
        }
        
        else if(cmd == "cd")
        {
            if( cmd_parts.size() < 2)
            {
                cout<<"Usage: cd <path>";
                continue;
            }
            pwd = cmd_parts[1];
        }
        else if(cmd == "quit")
        {
            break;
        }
        else{
            cout<<"Unrecognized command";
        }
    }   
    return 0;
}
