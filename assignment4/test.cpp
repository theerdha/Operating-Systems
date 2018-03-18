#include <iostream>
#include "myfs.h"

using namespace std;

int main(){
    int status;
    if((status = create_myfs(10,512)) == -1)
        cerr << "Error creating filesystem\n";
    status_myfs();
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
	status_myfs();
    rm_myfs("myfs11.h");
    status_myfs();
    return 0;
}
