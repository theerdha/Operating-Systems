#include <unistd.h>
#include "myfs.h"

int comp(const void* a,const void* b){
    return (*((int*)a)) - (*((int*)b));
}

int main(){
    int status,i;
    int n = restore_myfs("mydump-38.backup"),fd,bytes = 0,number,count = 1;
    int* array = (int*) malloc(sizeof(int));
    char* buf_temp = (char*)malloc(100);
    char* buf = (char*)malloc(1);
    status_myfs();
    showfile_myfs("mytest.txt");
    fd = open_myfs("mytest.txt",'r');    
    
    while(eof_myfs(fd) == 0){
        bytes += read_myfs(fd,1,buf);
        //printf("bytes %d\n",bytes);
    }
    close_myfs(fd);
    
    free(buf);
    buf = (char*) malloc(bytes+1);
    fd = open_myfs("mytest.txt",'r');    
    read_myfs(fd,bytes,buf);
    close_myfs(fd);
    buf[bytes] = '\0';
    
    while(bytes != 0){
        sscanf(buf,"%d\n",&number);
        sprintf(buf_temp,"%d\n",number);
        array[count-1] = number;
        count++;
        bytes -= strlen(buf_temp);
        buf += strlen(buf_temp);
        //printf("%d\n", number);
        array = (int*)realloc(array,sizeof(int)*count);
    }

    qsort(array,count-1,4,comp);
    for(i = 0; i < count-1; i++)
        printf("%d\n",array[i]);

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


    return 0;
}