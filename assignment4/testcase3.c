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
    return 0;
}