#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

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
      char** args = NULL;
      if(comm.size() > 1)
        args = new char*[comm.size()-1];
      for(int i = 0; i < comm.size()-1; i++)
        args[i] = (char*) comm[i+1].c_str();
      status = execvp(comm[0].c_str(),args);
      return 0;
    }
  }
  return 0;
}
