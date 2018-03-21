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

    // COPYING 12 files from system to MRFS
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
    copy_pc2myfs("testcase4.c","test.cpp");

    // LISTING CURRENT DIRECTORY
    ls_myfs();

    // COPYING FILE FROM MRFS TO system
    //copy_myfs2pc("test.cpp","test1.cpp");

    // MAKING DIRECTORY
    mkdir_myfs("buridi");
    ls_myfs();

    // SHOWING FILE
    //showfile_myfs("myfs1.h");

    // CHANGING DIRECTORY
    chdir_myfs("buridi");
    ls_myfs();
    chdir_myfs("..");
   
   	// REMOVING DIRECTORY
    rmdir_myfs("buridi");
    ls_myfs();

    // REMOVING FILE
    printf("Enter file name to delete.\n");
    scanf("%s",buf);
    rm_myfs(buf);

    // LISTING
    ls_myfs();
  	return 0;
}