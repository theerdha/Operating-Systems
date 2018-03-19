#include <iostream>
#include "myfs.h"

using namespace std;

int main(){
    int status;
    int n = restore_myfs("myfs.backup");
    printf("Returned %d\n",n);
    char* buf = (char*) malloc(1024);
    //if((status = create_myfs(10,512)) == -1)
    //    cerr << "Error creating filesystem\n";
    status_myfs();
    showfile_myfs("myfs2.h");
    chdir_myfs("buridi");
    ls_myfs();
    chdir_myfs("..");
    ls_myfs();
    status_myfs();
    int fd = open_myfs("myfs2.h",'r');
    printf("File descriptor : %d\n",fd);
    int bytes = read_myfs(fd,1023,buf);
    buf[bytes] = '\0';
    printf("%s\n",buf);
    printf("\nbytes read : %d\n",bytes);
    bytes = read_myfs(fd,1023,buf);
    buf[bytes] = '\0';
    printf("%s\n",buf);
    printf("\nbytes read : %d\n",bytes);
    return 0;
}
