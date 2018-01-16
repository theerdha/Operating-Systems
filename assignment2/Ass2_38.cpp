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
		vector<string> comm;
		string command,piece,type;
		pid_t x;
		int status = 0;
		stringstream stream;
		// Prompt on terminal
		cout << "\nSelet one of the following:\nA. Run an internal command.\nB. Run an external command.\nC. Run an external command by redirecting standard input from a file.\nD. Run an external command by redirecting standard output to a file.\nE. Run an external command in background.\nF. Run several external commands in pipe mode.\nG. Quit the shell.\n";
        getline(cin, type);
        cin.clear();

        // Check for quit
        if (type.compare("G") == 0)
            break;

        // Terminal prompt
        cout <<"G38shell$ ";

		// Read input command from terminal
		getline(cin,command);
		stream.str(command);

        if(type.compare("B") == 0 or type.compare("E") == 0 or type.compare("A") == 0){
		    // Split the input command
            bool printhelp_ = false;
            while(stream >> piece)
                comm.push_back(piece);

            // Generic parsing and conversion into char*
            char** args = new char*[comm.size()+1];
            for(int i = 0; i < comm.size(); i++){
                args[i] = new char[comm[i].length()+1];
                comm[i].copy(args[i],comm[i].length());
                args[i][comm[i].length()] = '\0';
            }
            args[comm.size()] = NULL;
            
            // External commands are handled here
            if(type.compare("B") == 0 or type.compare("E") == 0){
                x = fork();
                if(x == 0){
                    status = execvp(args[0],args);
                    _exit(0);
                }
            }
            else{
                // Additional parsing for internal commands for usage of ~ 
                char* pwd_ = get_current_dir_name();
                
               
                // Handling usage of ~ or no path specified
                if(args[1] == NULL or args[1][0] == '~'){
                    const char* home = "HOME";
                    char* home_dir = new char[1000];
                    char* home_env = getenv(home);
                    strcpy(home_dir,home_env);
                    if(args[1][0] == '~')
                        args[1] = args[1]+1;
                    else{
                        args[1] = new char[1];
                        args[1][0] = '\0';
                    }
                    strcat(home_dir,args[1]);
                    strcpy(args[1],home_dir);
                }
                // Handling local directory
                else if(args[1][0]!='/'){
                    char* pwd = new char[sizeof(pwd_)];
                    strcpy(pwd,pwd_);
                    args[1] = new char[1000];
                    comm[1].copy(args[1],comm[1].length());
                    args[1][comm[1].length()] = '\0';
                    char slash = '/';
                    strcat(pwd,&slash);
                    strcat(pwd,args[1]);
                    strcpy(args[1],pwd);
                }
                

                // chdir case
                if(comm[0].compare("chdir") == 0 or comm[0].compare("cd") == 0){
                    if(comm.size() <=  2)    
                       status = chdir(args[1]);
                    else
                        cout << "invalid no of arguments passed.\nCorrect usage '$chdir path'.\n";
                }

                // mkdir case
                else if(comm[0].compare("mkdir") == 0 ){
                    mkdir(args[1],700);
                }
                // rmdir case
                else if(comm[0].compare("rmdir") == 0){
                    rmdir(args[1]);
                }
                free(pwd_);
            }
        }
        else if(type.compare("C") == 0){
            
        }

        // ERROR handling
        if(status == -1){
            switch(errno){
                case EACCES:
                    cout << "Command failed with the error EACCES.\n";
                    break;
                case ENOEXEC:
                    cout << "Command failed with the error ENOEXEC.\n";
                    break;
                default:
                    cout << "Unexpected error.\n";
            }
        }
        // Wait for child process to complete
        if (type.compare("B") == 0){
            waitpid(-1,&status,0);
        }
	}
	return 0;
}
