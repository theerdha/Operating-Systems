#include <unistd.h>
#include "myfs.h"
#include <sys/wait.h>
int main(){
	int status,x,n,fd,number, bytes;
    char* buf = (char*) malloc(1024);
	if((status = create_myfs(10,512)) == -1)
        fprintf(stderr,"Error creating filesystem\n");
    status_myfs();
    
    // COMMANDS
    mkdir_myfs("mydocs");
    mkdir_myfs("mycode");
    chdir_myfs("mydocs");
    mkdir_myfs("mytext");
    mkdir_myfs("mypapers");
    ls_myfs();
    chdir_myfs("..");
    ls_myfs();
    ////////////
    x = fork();
    if(x == 0){
        printf("\n######### Child Process active ##########\n");
    	sem = sem_open("/tmp/sem",O_EXCL,S_IRWXU,1);
	    myfs = (char*)shmat(shmid,0,0);
	    sync_shared_myfs(); 

        chdir_myfs("mycode");
        copy_pc2myfs("test.cpp","test.cpp");
        ls_myfs();
        showfile_myfs("test.cpp");
        shmdt(myfs);
        exit(0);
    }
    else{
        chdir_myfs("mydocs");
        chdir_myfs("mytext");
        fd = open_myfs("mytext.txt",'w');    
        //ls_myfs();
        n = 0;
        while(n < 26){
            number = rand();            
            sprintf(buf,"%c\n",'A'+n);    
            bytes = write_myfs(fd,strlen(buf),buf);    
            n++; 
        }
        close_myfs(fd);
        ls_myfs();
        showfile_myfs("mytext.txt");
        wait(NULL);
        shmdt(myfs);
    }
    shmctl(shmid,IPC_RMID,NULL);
}