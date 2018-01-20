#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(){
	while(1){
		vector<vector<string> > comm(2);
		string command[2],piece,type;
		pid_t x;
		int status = 0;
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
		getline(cin,command[0]);
		

        char** args[2];
        bool printhelp = false;
        

        ////////////// INPUT PARSING ///////////////////
        //
        //
        // Pre parsing for filedesciptor redirecting and pipe
        if(type.compare("C") == 0 or type.compare("D") == 0){
        
        }
        // Pre Parisng for pipe. Splits command having pipe into two distinct commands 
        else if(type.compare("F") == 0){
            int len = command[0].length();
            for(int i = 0; i < len; i++){
                if(command[0][i] == '|'){
                   
                    for(int j = i+1; j < len; j++){
                        if(command[0][j] != ' '){
                            command[1] = command[0].substr(j,len - j);
                            break;
                        }
                    }

                    for(int j = i-1; j >= 0; j--){
                        if(command[0][j] != ' '){
                            command[0] = command[0].substr(0,j+1);
                            break;
                        }
                    }
                    
                    break;
                }
            }
        }


        // command parsing 
		int loop_count = 1;
        if(type.compare("F") == 0)
            loop_count = 2;
        
        for(int id = 0; id < loop_count; id++){
            stringstream stream;
            stream.str(command[id]);
            // Split the input command
            while(stream >> piece){
                comm[id].push_back(piece);
            }
            // Generic parsing and conversion into char*
            args[id] = new char*[comm[id].size()+1];
            for(int i = 0; i < comm[id].size(); i++){
                args[id][i] = new char[1000];
                comm[id][i].copy(args[id][i],comm[id][i].length());
                args[id][i][comm[id][i].length()] = '\0';
                //printf("%s ", args[id][i]);
            }
            args[id][comm[id].size()] = NULL;
        }

        // Additional parsing for handling file locations for internal commands
        if (type.compare("A") == 0){
            // Get required locations including absolute location to home and present working directory
            char* pwd_ = get_current_dir_name();
            const char* home = "HOME";
            char* home_dir = new char[1000];
            char* home_env = getenv(home);
            strcpy(home_dir,home_env);
                
            // Handling usage if no path specified
            if(args[0][1] == NULL){
                args[0][1] = new char[1000];
                args[0][1][0] = '\0';
                strcat(home_dir,args[0][1]);
                strcpy(args[0][1],home_dir);
               // printf("%s\n",args[0][1]);
            }
            // Handling '~' if path begins with it
            else if(args[0][1][0] == '~'){
                args[0][1] = args[0][1]+1;
                strcat(home_dir,args[0][1]);
                strcpy(args[0][1],home_dir);
               // printf("%s\n",args[0][1]);
            }
            // Handling local directory
            else if(args[0][1][0] != '/'){
                char* pwd = new char[1000];
                strcpy(pwd,pwd_);
                const char* slash = "/\0";
                strcat(pwd,slash);
                strcat(pwd,args[0][1]);
                strcpy(args[0][1],pwd);
               // printf("%s\n",args[0][1]);
            }
        }    

        ////////////// COMMAND EXECUTION //////////////// 
        //
        //
        // Internal Commands
        if(type.compare("A") == 0){
            // chdir case
            if(comm[0][0].compare("chdir") == 0 or comm[0][0].compare("cd") == 0){
                if(comm[0].size() <=  2)    
                   status = chdir(args[0][1]);
                else
                    cout << "invalid no of arguments passed.\nCorrect usage '$chdir path'.\n";
            }

            // mkdir case
            else if(comm[0][0].compare("mkdir") == 0 ){
                mkdir(args[0][1],700);
            }
            // rmdir case
            else if(comm[0][0].compare("rmdir") == 0){
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
        else if(type.compare("C") == 0){
            
        }
        else if(type.compare("F") == 0){
            int pipe1[2];
            pipe(pipe1);
            x = fork();
            if(x == 0){
                dup2(pipe1[1],STDOUT_FILENO);
                status = execvp(args[0][0],args[0]);
                close(pipe1[1]);
                _exit(0);
            }
            else{
                x = fork();
                if(x == 0){
                    dup2(pipe1[0],STDIN_FILENO);
                    status = execvp(args[1][0],args[1]);
                    close(pipe1[0]);
                    _exit(0);
                }
            }
        }

        // Wait for child process to complete
        cout << "waiting" << endl;
        wait(&status);
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
