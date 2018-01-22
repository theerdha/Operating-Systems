#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(){
    while(1){
        vector<vector<string> > comm;
        char*** args;
        vector<string> command;
        string piece,type;
        pid_t x = 0;
        string fullCommand;
        int status = 0,mode = 0;
        
        // Prompt on terminal
        cout << "\nSelet one of the following:\nA. Run an internal command.\nB. Run an external command.\nC. Run an external command by redirecting standard input from a file.\nD. Run an external command by redirecting standard output to a file.\nE. Run an external command in background.\nF. Run several external commands in pipe mode.\nG. Quit the shell.\n> ";
        getline(cin, type);
        cin.clear();

        // Check for quit
        if (type.compare("G") == 0)
            break;

        // Terminal prompt
        cout <<"\nG38shell$ ";

        // Read input command from terminal
        getline(cin,fullCommand);

        bool printhelp = false;

        ////////////// INPUT PARSING ///////////////////
        //
        //
        // Pre parsing for filedesciptor redirecting and pipe
        
        int len = fullCommand.length();
        int previousCommandIndex = 0;

        // PRE PARSING FOR FILE REDIRECTION
        if(type.compare("C") == 0 or type.compare("D") == 0){
            for(int i = 0; i < len-1; i++){
                if((fullCommand[i] == '<' && fullCommand[i+1] != '<') || (fullCommand[i] == '>' && fullCommand[i+1] != '>')){
                    command.push_back(fullCommand.substr(0,i));
                    command.push_back(fullCommand.substr(i+1,len-i-1));
                    mode = 1;
                    break;
                }
                if(fullCommand[i] == '>' && fullCommand[i+1] == '>'){
                    command.push_back(fullCommand.substr(0,i));
                    command.push_back(fullCommand.substr(i+2,len-i-2));
                    mode = 2;
                    break;
                }
            }  
            //cout << command[0] << " " << command[1] << endl;
        }
        // Parsing the input command for pipe and breaking into multiple commands
        else if(type.compare("F") == 0){
            for(int i = 0; i < len-1; i++){
                if(fullCommand[i] == '|'){
                    command.push_back(fullCommand.substr(previousCommandIndex,i-previousCommandIndex));
                    previousCommandIndex = i+1;
                }
            }
            
            command.push_back(fullCommand.substr(previousCommandIndex,len-previousCommandIndex));
            //for(int i = 0; i < command.size(); i++)
            //    cout << command[i] << endl;
        }
        // Preparsing for remaining cases
        else{
            command.push_back(fullCommand);
        }
        
        args = new char**[command.size()];
        comm = vector<vector<string> >(command.size());
        
        // Changing each command into array of char*
        for(int id = 0; id < command.size(); id++){
            stringstream stream;
            int i = 0;
            args[id] = new char*[20];

            stream.str(command[id]);
            // Split the input command
            while(stream >> piece){
                args[id][i] = new char[200];
                piece.copy(args[id][i],piece.length());
                args[id][i][piece.length()] = '\0'; 
                //printf("%s\n",args[id][i]);
                i++;
            }
            //printf("DONE");
            args[id][i] = NULL;
        }

        // Additional parsing for handling file locations for internal commands
        if (type.compare("A") == 0 || type.compare("C") == 0 || type.compare("D") == 0){
            // Get required locations including absolute location to home and present working directory
            char* pwd_ = get_current_dir_name();
            const char* home = "HOME";
            char* home_dir = new char[1000];
            char* home_env = getenv(home);
            strcpy(home_dir,home_env);

            char* path;

            if(type.compare("A") == 0)
                path = args[0][1];
            else 
                path = args[1][0];

            // Handling usage if no path specified
            if(path == NULL){
                path = new char[1000];
                path[0] = '\0';
                strcat(home_dir,path);
                strcpy(path,home_dir);
                // printf("%s\n",args[0][1]);
            }
            // Handling '~' if path begins with it
            else if(path[0] == '~'){
                path = path+1;
                strcat(home_dir,path);
                strcpy(path,home_dir);
                // printf("%s\n",args[0][1]);
            }
            // Handling local directory
            else if(path[0] != '/'){
                char* pwd = new char[1000];
                strcpy(pwd,pwd_);
                const char* slash = "/\0";
                strcat(pwd,slash);
                strcat(pwd,path);
                strcpy(path,pwd);
                // printf("%s\n",args[0][1]);
            }
            if(type.compare("A") == 0)
                args[0][1] = path;
            else 
                args[1][0] = path;
        }    

        ////////////// COMMAND EXECUTION //////////////// 
        //
        //
        // Internal Commands
        if(type.compare("A") == 0){
            // chdir case
            char ch[10] = "chdir";
            char cd[10] = "cd";
            char mkd[10] = "mkdir";
            char rmd[10] = "rmdir";
            if(strcmp(args[0][0],ch) == 0 or strcmp(args[0][0],cd) == 0){
                status = chdir(args[0][1]);
            }

            // mkdir case
            else if(strcmp(args[0][0],mkd) == 0 ){
                mkdir(args[0][1],0700);
            }
            // rmdir case
            else if(strcmp(args[0][0],rmd) == 0){
                rmdir(args[0][1]);
            }
            //free(pwd_);
        }

        // External commands 
        else if(type.compare("B") == 0 or type.compare("E") == 0){
            x = fork();
            if(x == 0){
                status = execvp(args[0][0],args[0]);
                _exit(0);
            }
        }
        else if(type.compare("C") == 0 || type.compare("D") == 0){
            int fd;
            x = fork();
            if(x == 0){
                if(type.compare("D") == 0){
                    if (mode == 1) 
                        fd = open(args[1][0], O_WRONLY |  O_CREAT , S_IRWXU);
                    else 
                        fd = open(args[1][0], O_WRONLY | O_APPEND |  O_CREAT , S_IRWXU);
                    dup2(fd,fileno(stdout));
                }
                else {
                    fd = open(args[1][0], O_RDONLY , S_IRWXU);
                    dup2(fd,STDIN_FILENO);

                }
                execvp(args[0][0],args[0]);
                _exit(0);
            }   
        }
        else if(type.compare("F") == 0){
            int fd[2];
            int inputfd;
            for(int i = 0; i < command.size(); i++){
                pipe(fd);
                x = fork();
                if(x == 0){
                    if(i != command.size()-1){
                        dup2(fd[1],STDOUT_FILENO);
                        close(fd[1]);
                    }
                    if(i != 0){
                        dup2(inputfd,STDIN_FILENO);
                        close(inputfd);
                    }
                    status = execvp(args[i][0],args[i]);
                    _exit(status);
                }
                close(fd[1]);
                inputfd = fd[0];        
                if(wait(&status)<0){
                    cout << "unexpected error\n";
                }

            }
        }
        // Wait for child process to complete
        // cout << "waiting" << endl;
        if(type.compare("E") != 0 )
            waitpid(-1,&status,0);
        ////////////// ERROR HANDLING ////////////
        //
        //
        if(status == -1){
            switch(errno){
                case EACCES:
                    cout << "Command failed with the error EACCES.\n";
                    break;
                case ENOEXEC:
                    cout << "Command failed with the error ENOEXEC.\n";
                    break;
                case EFAULT:
                    cout << "Command failed with the error EFAULT.\n";
                    break;
                case EIO:
                    cout << "Command failed with the error EIO.\n";
                    break;
                case ELOOP:
                    cout << "Command failed with the error ELOOP.\n";
                    break;
                case ENAMETOOLONG:
                    cout << "Command failed with the error ENAMETOOLONG.\n";
                    break;
                case ENOENT:
                    cout << "Command failed with the error ENOENT.\n";
                    break;
                case ENOMEM:
                    cout << "Command failed with the error ENOMEM.\n";
                    break;
                case ENOTDIR:
                    cout << "Command failed with the error ENOTDIR.\n";
                    break;
                default:
                    cout << "Unexpected error.\n";
            }
        }

    }
    return 0;
}
