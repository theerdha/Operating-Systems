#include <unistd.h>
#include "myfs.h"
int main(){
	int status,fd,fd1,bytes,n,number;
    char* buf = (char*) malloc(1024);
    char* filename = (char*) malloc(1024);

    // CREATING FILE SYSTEM
    if((status = create_myfs(10,512)) == -1)
        fprintf(stderr,"Error creating filesystem\n");

    // SHOWING FILE SYSTEM STATUS
    status_myfs();
    int temp = 1,i;
    
    fd = open_myfs("mytest.txt",'w');    
    //ls_myfs();
    n = 0;
    while(n < 100){
        number = rand();            
        sprintf(buf,"%d\n",number);    
        bytes = write_myfs(fd,strlen(buf),buf);    
        n++; 
    }
    close_myfs(fd);
    //ls_myfs();
    //showfile_myfs("mytest.txt");
    printf("Enter a number : ");
    scanf("%d",&n);
    
    for(i = 0; i < n; i++){
        sprintf(filename,"mytest-%d.txt",i+1);
        fd1 = open_myfs("mytest.txt",'r');
        fd = open_myfs(filename,'w');
        while(eof_myfs(fd1) == 0){
            bytes = read_myfs(fd1,1024,buf);
            write_myfs(fd,bytes,buf);
        }
        close_myfs(fd);    
        close_myfs(fd1);
    }
    ls_myfs();
    dump_myfs("mydump-38.backup");

    return 1;
}