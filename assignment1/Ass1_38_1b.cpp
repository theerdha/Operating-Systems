#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <stdio.h>
#include <cstdlib>

using namespace std;

int main(){
	while(1){
		vector<string> comm;
		string command,piece;
		pid_t x;
		int status;
		stringstream stream;
		// Prompt on terminal
		cout <<"G38shell$ ";

		// Read input command from terminal
		getline(cin,command);
		stream.str(command);

		// Split the input command
		while(stream >> piece)
			comm.push_back(piece);

		// Check for quit
		if (command.compare("quit") == 0)
			break;

		// Create new process
		x = fork();

		// Execute the command inputed in the new child process
		if(x == 0){
			char** args = new char*[comm.size()+1];
			for(int i = 0; i < comm.size(); i++){
				args[i] = new char[comm[i].length()+1];
				comm[i].copy(args[i],comm[i].length());
				args[i][comm[i].length()] = '\0';
			}
			args[comm.size()] = NULL;
			if(comm[0].compare("cd") == 0)
				chdir(args[1]);
			else
				execvp(args[0],args);
			_exit(0);
		}
		else{
			waitpid(-1,&status,0);
		}
	}
	return 0;
}
