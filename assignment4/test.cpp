#include <iostream>
#include "myfs.h"

using namespace std;

int main(){
    int status;
    char* buf = (char*) malloc(1024);
    if((status = create_myfs(10,512)) == -1)
        cerr << "Error creating filesystem\n";
    status_myfs();
    int temp = 1;
    //printf("file name%s\n",vfs.db[0].data);
    ls_myfs();
    //printf("file name%s\n",vfs.db[0].data);
    copy_pc2myfs("myfs.h","myfs1.h");
    //printf("file name%s\n",vfs.db[0].data);
    ls_myfs();
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
	status_myfs();
    rm_myfs("myfs11.h");
    status_myfs();
    int fd = open_myfs("test.cpp",'r');
    int fd1 = open_myfs("myfs2.h",'w');
    printf("File descriptor : %d\n",fd);
    int bytes = read_myfs(fd,1023,buf);
    bytes = write_myfs(fd1,1023,buf);
    bytes = write_myfs(fd1,1023,buf);
    printf("\nbytes written : %d\n",bytes);
    bytes = write_myfs(fd1,1023,buf);
    printf("\nbytes written : %d\n",bytes);
    showfile_myfs("myfs2.h");
    buf[bytes] = '\0';
    //printf("%s\n",buf);
    printf("\nbytes read : %d\n",bytes);
    bytes = read_myfs(fd,1023,buf);
    buf[bytes] = '\0';
    //printf("%s\n",buf);
    printf("\nbytes read : %d\n",bytes);
    dump_myfs("myfs.backup");
    return 0;
}
