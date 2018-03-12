#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#define BLOCKSIZE 256

// These data structures for creating a virtual file system in the user space and modify and update the actual file system by synchronizing
typedef struct superblock_{
    int filesystem_size;
    int blocksize;
    int max_inodes;
    int used_inodes;
    int max_disk_blocks;
    int used_disk_blocks;
} superblock_;

typedef struct superblock_additional{
    int disk_bitmask_size;
    int inode_bitmask_size;
    char* free_disk_bitmask;
    char* free_inode_bitmask;
} superblock_mask;

typedef struct superblock{
    superblock_ sb;
    superblock_mask mask;
} superblock;

// Data structure of inode
typedef struct inode{
    int file_size;
    int dataList[8];
    char filetype_permission[2];
    char timeLastModified[26];
    char timeLastRead[26];
} inode;

// Data structure of datablock
typedef struct datablock{
    char data[BLOCKSIZE];
} datablock;

// Virtual file system
typedef struct filesystem{
    superblock sb;
    inode* inodeList;
    datablock* db;
    int blocks_for_superblock;
    int blocks_for_inodelist;
    int blocks_for_datablocks;
} filesystem;

/*
 * Global variables
 */

char* myfs; // Actual In memory File system 
int pwd_inode = 0; // Present working directory inode
filesystem vfs;


/* 
 * Synchronization operations for superblock, inodes and datablock   
 */

// Initialize Inode

void initInode(inode* in){
    int i;
    time_t currenttime;
    struct tm* local_time;

    in->filetype_permission[0] = 0x03; 
    in->filetype_permission[1] = 0xFF; 
    currenttime = time(NULL);
    local_time = localtime(&currenttime);
    asctime_r(local_time, in->timeLastModified);
    asctime_r(local_time, in->timeLastRead);
    in->file_size = 0;
    for(i = 0; i < 10; i++)
        in->dataList[i] = NULL;
}

// Synchronize super block with filesystem superblock
void syncSB(superblock* sb){
    superblock_* sb_ = (superblock_*) myfs;
    *sb_ = (sb->sb);
    char* myfs_ = myfs;
    
    myfs_ += sizeof(superblock_);
    bcopy(sb->mask.free_disk_bitmask,myfs_,sb->mask.disk_bitmask_size);
    myfs_ += sb->mask.disk_bitmask_size;
    bcopy(sb->mask.free_inode_bitmask,myfs_,sb->mask.inode_bitmask_size);
    myfs_ += sb->mask.inode_bitmask_size;
    /*
    printf("1. %d\n",*((int*)(myfs)));
    printf("2. %d\n",*((int*)(myfs+4)));
    printf("3. %d\n",*((int*)(myfs+8)));
    printf("4. %d\n",*((int*)(myfs+12)));
    printf("5. %d\n",*((int*)(myfs+16)));
    printf("6. %d\n",*((int*)(myfs+20)));
    */
    return;
}

// Synchronize inode with given index
void syncInode(superblock* sb,inode* in,int index){
    char* myfs_ = myfs;
    char* temp;
    int* temp1;
    inode* in_;
    myfs_ += (index + vfs.blocks_for_superblock)*sb->sb.blocksize;
    in_ = (inode*) myfs_;
    *in_ = *in;
    /*
    printf("Size of inode %d\n",sizeof(inode));
    printf("1. %d\n",*((int*)(myfs_)));
    printf("2. %d\n",*((int*)(myfs_+4)));
    printf("3. %d\n",*((char*)(myfs_+44)));
    */
    return;
}

// Synchronize datablock with given index
void syncdata(superblock* sb, datablock* db, int index){
    char* myfs_ = myfs;
    datablock* db_;
    myfs_ += sb->sb.blocksize*(vfs.blocks_for_superblock+vfs.blocks_for_inodelist + index);
    db_ = (datablock*)myfs_;
    *db_ = *db;
    /*
    printf("1. %s\n",myfs_);
    printf("2. %d\n",*(unsigned int*)(myfs_+30));
    printf("3. %s\n",myfs_ + 32);
    printf("4. %d\n",*(unsigned int*)(myfs_+62));
    */
    return;
}

// Assuming the system to be byte addressable
int create_myfs(int size,int max_inodes){
    // Size is measured in Mbytes
    // max_inodes gives no of inode blocks 
    time_t currenttime;
    struct tm* local_time;
    int i, no_of_data_blocks;
    char* myfs_;
    
    myfs = (char*) malloc(size*1024*1024*sizeof(char));
    if(myfs == NULL){
        fprintf(stderr,"Error allocating memory\n");
        return -1;
    }
    
    vfs.sb.sb.filesystem_size = size * 1024 * 1024;
    vfs.sb.sb.blocksize = BLOCKSIZE;
    vfs.inodeList = (inode*)malloc(sizeof(inode)*max_inodes);
    vfs.blocks_for_inodelist = max_inodes; 
    no_of_data_blocks = (size*1024*4 - max_inodes);
    vfs.blocks_for_datablocks = no_of_data_blocks - ceil(no_of_data_blocks/(8*vfs.sb.sb.blocksize) + max_inodes/8 + double(20)/vfs.sb.sb.blocksize);
    vfs.blocks_for_superblock = size*1024*4 - vfs.blocks_for_datablocks - vfs.blocks_for_inodelist;
    vfs.db = (datablock*) malloc(sizeof(datablock)*no_of_data_blocks);
    myfs_ = myfs;
    
    /* Initialize super block */

    vfs.sb.sb.max_inodes = max_inodes;
    vfs.sb.sb.used_inodes = 1;
    vfs.sb.sb.max_disk_blocks = vfs.blocks_for_datablocks;
    vfs.sb.sb.used_disk_blocks = 1;
    vfs.sb.mask.disk_bitmask_size = ceil(double(vfs.blocks_for_datablocks)/8);
    vfs.sb.mask.inode_bitmask_size = ceil(double(vfs.blocks_for_inodelist)/8);
    vfs.sb.mask.free_disk_bitmask = (char*)malloc(vfs.sb.mask.disk_bitmask_size);
    vfs.sb.mask.free_inode_bitmask = (char*) malloc(vfs.sb.mask.inode_bitmask_size);
    bzero(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
    bzero(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    vfs.sb.mask.free_inode_bitmask[0] = 0x80; 
    vfs.sb.mask.free_disk_bitmask[0] = 0x80;
    syncSB(&vfs.sb);
   
    printf("Created superblock with %d blocks\n", vfs.blocks_for_superblock);
    printf("Created inode list with %d blocks\n", vfs.blocks_for_inodelist);
    printf("Created data block with %d blocks\n", vfs.blocks_for_datablocks);

    /* Initializing Root Inode and setting other inodes to null*/

    // 2 bytes for access permission and file type d-rwx-rwx-rwx 9 bits so 2 bytes
    inode* in_temp = &vfs.inodeList[0];
    initInode(in_temp);
    in_temp->file_size = 32;
    in_temp->dataList[0] = 0;
    syncInode(&vfs.sb,in_temp,0);
    
    /* Initializing the root directory */
    // Each directory entry is stored as 32 bytes data. 30 bytes for file/directory name and 2 bytes for inode number.
    strcpy(vfs.db[0].data,".");
    bzero(vfs.db[0].data+30,2);
    syncdata(&vfs.sb,&vfs.db[0],0);
    return 1;
}

int getfreeindex(char* mask, int length){
    int i, free_index = 0;
    for(i = 0; i < length; i++){
        if( mask[i] == (char)0xFF ){
            free_index += 8;
        }
        else{
            if(!(mask[i] & 0x80)){
                mask[i] = mask[i] | 0x80;
                free_index += 0;
            }
            else if(!(mask[i] & 0x40)){
                mask[i] = mask[i] | 0x40;
                free_index += 1;
            }
            else if(!(mask[i] & 0x20)){
                mask[i] = mask[i] | 0x20;
                free_index += 2;
            }
            else if(!(mask[i] & 0x10)){
                mask[i] = mask[i] | 0x10;
                free_index += 3;
            }
            else if(!(mask[i] & 0x08)){
                mask[i] = mask[i] | 0x08;
                free_index += 4;
            }
            else if(!(mask[i] & 0x04)){
                mask[i] = mask[i] | 0x04;
                free_index += 5;
            }
            else if(!(mask[i] & 0x02)){
                mask[i] = mask[i] | 0x02;
                free_index += 6;
            }
            else if(!(mask[i] & 0x01)){
                mask[i] = mask[i] | 0x01;
                free_index += 7;
            }
            break;
        }    
    }
    return free_index;
}

void insertDataIndex(inode* in,int index){

}

void insertFileInode(char* filename,int fileInode){
    inode* tempInode = &vfs.inodeList[pwd_inode];
    int filesize = tempInode->file_size;
    int index = tempInode->file_size/vfs.sb.sb.blocksize;
    int dIndex = tempInode->dataList[index];
    int offset, free_db_index;
    short* n;
    if(index < 8){
        if(dIndex != NULL){
            offset = tempInode->file_size - ((index >= 1)?((index-1)*vfs.sb.sb.blocksize):0);
            strcpy(vfs.db[dIndex].data + offset,filename);
            n = (short*) vfs.db[dIndex].data + offset + 30;
            *n = (short) fileInode;
            syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        }
        else{
            if(vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks  == 0){
                fprintf(stderr,"Insufficient disk space for stroring directory entry.\n");
            }
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[index] = dIndex;
            strcpy(vfs.db[dIndex].data,filename);
            n = (short*) vfs.db[dIndex].data + 30;
            *n = (short) fileInode;
            syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
            syncSB(&vfs.sb);
        }
    }
    else{

    }
    tempInode->file_size += 32;
    syncInode(&vfs.sb,tempInode,pwd_inode);
    return;
}

int ls_myfs(){
    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,index = 0,offset = 0,dIndex;
    printf("data %d\n",inodeTemp->dataList[0]);
    while(filesize != 0){
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            printf("%s %d\n",vfs.db[dIndex].data+offset,*((short*)(vfs.db[dIndex].data+offset+30)));
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            }
        }
    }
    return 1;
}

int copy_pc2myfs(char* source,char* dest){
    int inode_index,n;
    int i, free_inode_index = 0,free_db_index,file_size;
    FILE* fd;
    char* myfs_ = myfs;
    struct stat st;
    inode* tempInode;
    
    // Get file size
    stat(source,&st);
    file_size = st.st_size;
     
    // Check disk space availability
    if((ceil(double(file_size)/vfs.sb.sb.blocksize) > vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks) && vfs.sb.sb.max_inodes - vfs.sb.sb.used_inodes >= 1){
        fprintf(stderr,"Insufficient disk space for data.\n");
        return -1;
    }
    free_inode_index = getfreeindex(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    vfs.sb.sb.used_inodes += 1;
    tempInode = &vfs.inodeList[free_inode_index] ;
    initInode(tempInode);
    fd = fopen(source,"r");
    while(feof(fd) == 0){
        free_db_index = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
        n = fread(vfs.db[free_db_index].data,vfs.sb.sb.blocksize,1,fd);
        //printf("Free DB INDEX%d\n",free_db_index);
        //printf("%s",vfs.db[free_db_index]);
        syncdata(&vfs.sb,&vfs.db[free_db_index],free_db_index);
        vfs.sb.sb.used_disk_blocks += 1;
        tempInode->file_size += vfs.sb.sb.blocksize;
        insertDataIndex(tempInode,free_db_index); // TODO
    }
    insertFileInode(dest,free_inode_index);
    syncInode(&vfs.sb,tempInode,free_inode_index); // TODO
    syncSB(&vfs.sb);
    ls_myfs();
    fclose(fd);
    return 1;
}


