#include <iostream>
#include "myfs.h"

using namespace std;

int main(){
    int status,fd,fd1,bytes,n,number;
    char* buf = (char*) malloc(1024);
    char* filename = (char*) malloc(1024);
    if((status = create_myfs(10,512)) == -1)
        cerr << "Error creating filesystem\n";
    status_myfs();
    int temp = 1,i;
    ls_myfs();
    copy_pc2myfs("myfs.h","myfs1.h");
    copy_pc2myfs("myfs.h","myfs2.h");
    copy_pc2myfs("myfs.h","myfs3.h");
    copy_pc2myfs("myfs.h","myfs4.h");
    copy_pc2myfs("myfs.h","myfs5.h");
    copy_pc2myfs("myfs.h","myfs6.h");
    copy_pc2myfs("myfs.h","myfs7.h");
    copy_pc2myfs("myfs.h","myfs8.h");
    copy_pc2myfs("myfs.h","myfs9.h");
    copy_pc2myfs("myfs.h","myfs10.h");
    copy_pc2myfs("myfs.h","myfs11.h");
    copy_pc2myfs("test.cpp","test.cpp");
    copy_myfs2pc("test.cpp","test1.cpp");
    mkdir_myfs("buridi");
    //showfile_myfs("myfs1.h");
    chdir_myfs("buridi");
    ls_myfs();
    chdir_myfs("..");
    ls_myfs();
    printf("Enter file name to delete.\n");
    scanf("%s",buf);
    rm_myfs(buf);
    ls_myfs();
    
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
    return 0;
}
