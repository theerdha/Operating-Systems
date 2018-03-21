#include <unistd.h>
#include <sys/wait.h>
#include "myfs.h"

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
	    sync_shared_myfs(); 

        chdir_myfs("mycode");
        copy_pc2myfs("testcase1.c","test.cpp");
        ls_myfs();
        showfile_myfs("test.cpp");
        shmdt(myfs);

        shmdt(inode_sem_no);

        shmdt(data_sem_no);

        shmdt(super_sem_no);

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
    
    sem_close(sem_inode);
    sem_close(mut_inode);

    sem_close(sem_data);
    sem_close(mut_data);
    
    sem_close(sem_super);
    sem_close(mut_super);
    

    sem_unlink("sem_inode_2");
    sem_unlink("mut_inode_2");
    sem_unlink("sem_super_2");
    sem_unlink("mut_super_2");
    sem_unlink("sem_data_2");
    sem_unlink("mut_data_2");
    
    shmdt(myfs);
    shmctl(shmid,IPC_RMID,NULL);

    shmdt(inode_sem_no);
    shmctl(shmid_1,IPC_RMID,NULL);

    shmdt(data_sem_no);
    shmctl(shmid_2,IPC_RMID,NULL);

    shmdt(super_sem_no);
    shmctl(shmid_3,IPC_RMID,NULL);

    return 1;
}