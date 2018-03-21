#include <unistd.h>
#include "myfs.h"

int main(){
	int status,x;
	if((status = create_myfs(10,512)) == -1)
        fprintf(stderr,"Error creating filesystem\n");
    status_myfs();
    mkdir_myfs("mydocs");
    mkdir_myfs("mycode");
    chdir_myfs("mydocs");
    mkdir_myfs("mytext");
    mkdir_myfs("mypapers");
    ls_myfs();
    chdir_myfs("..");
    ls_myfs();
    /*x = fork();
    if(x == 0){

    }*/
}