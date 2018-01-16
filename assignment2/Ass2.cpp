#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <stdio.h>
#include <cstdlib>
#include <fcntl.h>

using namespace std;

int main(){
	while(1){
		vector<string> comm;
		string command,piece,type;
		pid_t x;
		int status;
		stringstream stream;
		// Prompt on terminal
		cout << "\nSelet one of the following:\nA. Run an internal command.\nB. Run an external command.\nC. Run an external command by redirecting standard input from a file.\nD. Run an external command by redirecting standard output to a file.\nE. Run an external command in background.\nF. Run several external commands in pipe mode.\nG. Quit the shell.\n";
        getline(cin, type);
        cin.clear();
        // Check for quit
        if (type.compare("G") == 0)
            break;

        cout <<"G38shell$ ";

		// Read input command from terminal
		getline(cin,command);
		stream.str(command);

        if(type.compare("B") == 0 or type.compare("E") == 0 or type.compare("A") == 0){
		    // Split the input command
            while(stream >> piece)
                comm.push_back(piece);

            char** args = new char*[comm.size()+1];
            for(int i = 0; i < comm.size(); i++){
                args[i] = new char[comm[i].length()+1];
                comm[i].copy(args[i],comm[i].length());
                args[i][comm[i].length()] = '\0';
            }
            args[comm.size()] = NULL;
            
            if(type.compare("B") == 0 or type.compare("E") == 0){
                x = fork();
                if(x == 0){
                    execvp(args[0],args);
                    _exit(0);
                }
            }
            else{
                if(comm[0].compare("chdir") == 0 or comm[0].compare("cd") == 0){
                    chdir(args[1]);
                }
                else if(comm[0].compare("mkdir") == 0 ){
                    //mkdir(args[1]);
                }
            }
        }
        else if(type.compare("D") == 0){
                int count;
                while(stream >> piece)
                comm.push_back(piece);
				for(count = 0; count < comm.size(); count++)
				{
					if (comm[count].compare(">") == 0)break;
				}
				char** args = new char*[count];
				for(int i = 0; i < count; i++)
				{
					args[i] = new char[comm[i].length()+1];
					comm[i].copy(args[i],comm[i].length());
					args[i][comm[i].length()] = '\0';
				}
				args[count] = NULL;

				 close(1); 
				 int file_desc = open(comm[count + 1].c_str(), O_WRONLY | O_APPEND |  O_CREAT , S_IRWXU);
				 dup(file_desc);
				 execvp(args[0],args);
				_exit(0);
        }
        if (type.compare("B") == 0 or type.compare("A") == 0 or type.compare("D") == 0 ){
            waitpid(-1,&status,0);
        }
	}
	return 0;
}
