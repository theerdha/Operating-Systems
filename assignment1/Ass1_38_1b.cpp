#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <stdio.h>

using namespace std;

int main(){
  while(1){
    vector<string> comm;
    string command,piece;
    pid_t x;
    int status;
    stringstream stream;
    // Prompt on terminal
    cout <<"G76shell$ ";

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
      char** args = new char*[comm.size()];
      char null = '\0';
      for(int i = 0; i < comm.size(); i++){
        args[i] = new char[comm[i].length()+1];
        comm[i].copy(args[i],comm[i].length());
      }
      status = execvp(comm[0].c_str(),args);
      return 0;
    }
    else{
      waitpid(-1,&status,0);
    }
  }
  return 0;
}
