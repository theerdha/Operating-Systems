#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

char* myfs;
int blocks_for_superblock = 0;
int block_size = 256;
int pwd_inode = 0;

typedef struct superblockdata{
    int filesystem_size;
    int max_inodes;
    int used_inodes;
    int max_disk_blocks;
    int used_disk_blocks;
} superblockdata;

void update_filesystem_size(int size){
    int* myfs_ = (int*)myfs;
    *myfs_ = size;
}

void update_max_inodes(int no){
    int* myfs_ = (int*)(myfs+4);
    *myfs_ = no;
}

void update_used_inodes(int no){
    int* myfs_ = (int*)(myfs+8);
    *myfs_ = no;
}

void update_max_data_blocks(int no){
    int* myfs_ = (int*)(myfs+12);
    *myfs_ = no;
}


void update_used_data_blocks(int no){
    int* myfs_ = (int*)(myfs+16);
    *myfs_ = no;
}

void update_inode_filesize(int size,int inode_no){
    superblocksize sb;
    load_super_block_data(&sb);
    int* myfs_ = (int*)(myfs + (sb.max_inodes+ blocks_for_superblock + inode_no)*block_size );
    *myfs_ = size;
}

int update_inode_datalist(int inode_no,int data_index){
    superblocksize sb;
    int vacant_datalist_index,i;
    load_super_block_data(&sb);
    int* myfs_ = (int*)(myfs + (sb.max_inodes+ blocks_for_superblock + inode_no)*block_size + 58);
    // Update in direct list
    for(i = 0; i < 8; i++){
        if(*myfs_ != NULL){
            *myfs_ = data_index;
            return 1;
        }
        myfs_ += 1;
    }
    // update in indirect list
    // Check is indirect llist is already made. If not create an indirect list and find empty data block and assign 
}

// Assuming the system to be byte addressable
int create_myfs(int size,int max_inodes){
    // Size is measured in Mbytes
    // max_inodes gives no of inode blocks 
    time_t currenttime;
    struct tm* local_time;
    int i;
    myfs = (char*) malloc(size*1024*1024*sizeof(char));
    if(myfs == NULL){
        fprintf(stderr,"Error allocating memory\n");
        return -1;
    }
    char* myfs_ = myfs;
    int no_of_data_blocks;
    /* Initialize super block */

    // Initialize file system size - 4bytes
    update_filesystem_size(size);

    // Initialize max no of indoes - 4bytes
    update_max_inodes(max_inodes);
    
    // Initialize no of inodes used - 4bytes
    update_used_inodes(1);

    // Initialize max no of disk blocks - 4bytes
    // computing size of superblock 
    no_of_data_blocks = (size*1024*4 - max_inodes);
    no_of_data_blocks = no_of_data_blocks - ceil(no_of_data_blocks/(8*block_size) + max_inodes/8 + double(20)/block_size);
    blocks_for_superblock = size*1024*4 - no_of_data_blocks - max_inodes;
    
    update_max_data_blocks(no_of_data_blocks);
    
    // Initialize no of diskblock used - 4bytes
    update_used_data_blocks(1);

    // Initialize list of free disks blocks
    myfs_ += 20;
    *myfs_ = 0x80;
    myfs_ += 1;
    for(i = 1; i < ceil(double(no_of_data_blocks)/8); i++){
        *myfs_ = 0x00;
        myfs_ += 1;
    }
    //cout << "Created Free disk blocks bitmask with " << ceil(no_of_data_blocks/8) << " bytes\n";

    // Initilize list of free inode blocks
    // First Inode contains the root directory
    *myfs_ = 0x80;
    myfs_ += 1;
    for(i = 1; i < ceil(double(max_inodes)/8); i++){
        *myfs_ = 0x00;
        myfs_ += 1;
    }
    printf("Created superblock with %d blocks\n", blocks_for_superblock);
    printf("Created inode list with %d blocks\n", max_inodes);
    printf("Created data block with %d blocks\n", no_of_data_blocks);

    /* Initializing Root Inode and setting other inodes to null*/

    myfs_ = myfs;
    myfs_ += block_size*blocks_for_superblock;

    // 2 bytes for access permission and file type d-rwx-rwx-rwx 9 bits so 2 bytes
    *myfs_ = 0x03;
    myfs_ += 1 ;
    *myfs_ = 0xFF;
    myfs_ += 1 ;

    // Space for file size in bytes - 4 bytes
    update_inode_filesize(32,0);
    myfs_ += 4;

    // Setting time last modified and time last read - 26 bytes each total 52 bytes
    currenttime = time(NULL);
    local_time = localtime(&currenttime);
    asctime_r(local_time, myfs_);
    myfs_ += 26;
    asctime_r(local_time, myfs_);
    myfs_ += 26;

    // Setting pointer to data
    // total 8 direct pointers 1 indirect pointer 1 doubly indirect - total 10*4 bytes
    *myfs_ = int(0);
    myfs_ += 4;
    for(i = 0; i < 9; i++){
        myfs_ = NULL;
        myfs_ += 4;
    }

    /* Initializing the root directory */
    // Each directory entry is stored as 32 bytes data. 30 bytes for file/directory name and 2 bytes for inode number.
    myfs_ = myfs;
    myfs_ += block_size*(max_inodes + blocks_for_superblock);
    strcpy(myfs_, ".");
    myfs_ += 30;
    *myfs_ = 0x0000;
    myfs_ += 2;

    for(i = 0; i < 7; i++){
        myfs_ = NULL;
        myfs_ += 32;
    }
    return 1;
}

void load_super_block_data(superblockdata *sb){
    int* myfs_ = (int*) myfs;
    sb->filesystem_size = *(myfs_);
    sb->max_inodes = *(myfs_ + 1);
    sb->used_inodes = *(myfs_ + 2);
    sb->max_disk_blocks = *(myfs_ + 3);
    sb->used_disk_blocks = *(myfs_ + 4);
}

int getfreeinodeindex(){
    int i, free_inode_index = 0;
    char* myfs_ = myfs;
   
    superblockdata sb;
    load_super_block_data(&sb);
    
    myfs_ += 20;
    if(sb.max_inodes == sb.used_inodes)
        return -1;
    myfs_ += int(ceil(double(sb.max_disk_blocks)/8));
    for(i = 0; i < ceil(double(sb.max_inodes)/8); i++){
        if( !(*myfs_ ^ 0xFF)){
            free_inode_index += 8;
            myfs_ += 1;
        }
        else{
            if(!(*myfs_ & 0x80)){
                *myfs_ = *myfs_ | 0x80;
                free_inode_index += 0;
            }
            else if(!(*myfs_ & 0x40)){
                *myfs_ = *myfs_ | 0x40;
                free_inode_index += 1;
            }
            else if(!(*myfs_ & 0x20)){
                *myfs_ = *myfs_ | 0x20;
                free_inode_index += 2;
            }
            else if(!(*myfs_ & 0x10)){
                *myfs_ = *myfs_ | 0x10;
                free_inode_index += 3;
            }
            else if(!(*myfs_ & 0x08)){
                *myfs_ = *myfs_ | 0x08;
                free_inode_index += 4;
            }
            else if(!(*myfs_ & 0x04)){
                *myfs_ = *myfs_ | 0x04;
                free_inode_index += 5;
            }
            else if(!(*myfs_ & 0x02)){
                *myfs_ = *myfs_ | 0x02;
                free_inode_index += 6;
            }
            else if(!(*myfs_ & 0x01)){
                *myfs_ = *myfs_ | 0x01;
                free_inode_index += 7;
            } 
            break;
        }    
    }
    return free_inode_index;
}

int copy_pc2myfs(char* source,char* dest){
    int inode_index;
    int i, free_inode_index = 0,file_size;
    superblockdata sb;
    char* myfs_ = myfs;
    struct stat st;
    
    // Get file size
    stat(source,&st);
    file_size = st.st_size;
    
    // Load data from super block
    load_super_block_data(&sb);
     
    printf("MAX DISK BLOCKS : %d\nMAX INODES : %d\n",sb.max_disk_blocks,sb.max_inodes);
    // Check disk space availability
    if(ceil(double(file_size)/block_size) > sb.max_disk_blocks - sb.used_disk_blocks){
        fprintf(stderr,"Insufficient disk space\n");
        return -1;
    }
    if((free_inode_index = getfreeinodeindex()) == -1){ 
        fprintf(stderr,"No free inodes found\n");
        return -1;
    }
    
    printf("Free inode index %d",free_inode_index);
    return 1;
    
}

/*
   int ls_myfs(){
   for(int i = 0; i < 10; i++){

   }
   }
   */
