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

// File Table entry
typedef struct file_table_entry{
    int mode;
    int offset;
} file_table_entry;

// FILE TABLE
typedef struct filetable{
    file_table_entry* ft;
    int max_index;
} filetable;

/*
 * Global variables
 */

char* myfs; // Actual In memory File system 
int pwd_inode = 0; // Present working directory inode
filesystem vfs;
filetable vft;

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
    vft.max_index = 0;
    vfs.sb.sb.filesystem_size = size * 1024 * 1024;
    vfs.sb.sb.blocksize = BLOCKSIZE;
    vfs.inodeList = (inode*)malloc(sizeof(inode)*max_inodes);
    vfs.blocks_for_inodelist = max_inodes; 
    no_of_data_blocks = (size*1024*4 - max_inodes);
    vfs.blocks_for_datablocks = no_of_data_blocks - ceil(double(no_of_data_blocks)/(8*vfs.sb.sb.blocksize) + max_inodes/8 + double(20)/vfs.sb.sb.blocksize);
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

    // 2 bytes for access permission and file type d-rwx-rwx-rwx 10 bits so 2 bytes
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

void status_myfs(){
    printf("\n######## File System Info #########\nCreated superblock with %d blocks\n", vfs.blocks_for_superblock);
    printf("Blocks left in inode list with %d blocks\n", vfs.sb.sb.max_inodes - vfs.sb.sb.used_inodes);
    printf("Blocks left in Data blocks with %d blocks\n", vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks);
}

int getfreeindex(char* mask, int length){
    int i, free_index = 0;
    if(length == ceil(double(vfs.blocks_for_inodelist)/8))
        vfs.sb.sb.used_inodes += 1;
    else if(length == ceil(double(vfs.blocks_for_datablocks)/8))
        vfs.sb.sb.used_disk_blocks += 1;

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

void insertDataIndex(inode* in,int dIndex){
    int filesize = in->file_size,di,free_db_index,di_2;
    int index = ceil(double(filesize)/vfs.sb.sb.blocksize);
    //printf("Location to be inserted %d\n", dIndex);
    int* x;
    if(index < 8){
        in->dataList[index] = dIndex;
    }
    else if(index < 72){
        if (index == 8){
            free_db_index = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            in->dataList[8] = free_db_index;
        }
        di = in->dataList[8];    
        x = (int*) (vfs.db[di].data + (index-8)*4);
        *x = dIndex;
        syncdata(&vfs.sb,&vfs.db[di],di);
    }
    else{
        if(index == 72){
            di = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            in->dataList[9] = di;
            di_2 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*) (vfs.db[di].data);
            *x = di_2;
        }
        di = in->dataList[9];
        di_2 = *((int*)(vfs.db[di].data + ((index-72)/64)*4));
        x = (int*)(vfs.db[di_2].data + ((index-72)%64)*4);
        *x = dIndex;
        syncdata(&vfs.sb,&vfs.db[di],di);
        syncdata(&vfs.sb,&vfs.db[di_2],di_2);
    }
    return;
}

void insertFileInode(int pwd_inode_,char* filename,int fileInode){
    inode* tempInode = &vfs.inodeList[pwd_inode_];
    int filesize = tempInode->file_size;
    int index = tempInode->file_size/vfs.sb.sb.blocksize;
    int dIndex, dIndex_1, dIndex_2;
    int offset, offset1, offset2, free_db_index;
    int* x;
    short* n;
    if(index < 8){
        dIndex = tempInode->dataList[index];
        if(tempInode->file_size%256 != 0){
            offset = tempInode->file_size - index*vfs.sb.sb.blocksize;
            strcpy(vfs.db[dIndex].data + offset,filename);
            n = (short*) (vfs.db[dIndex].data + offset + 30);
            *n = (short) fileInode;
            syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        }
        else{
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[index] = dIndex;
            strcpy(vfs.db[dIndex].data ,filename);
            n = (short*) (vfs.db[dIndex].data + 30);
            *n = (short) fileInode;
            syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        }
    }
    else if(index < 72){
        if(index == 8 && tempInode->file_size%256 == 0){
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[8] = dIndex;
            dIndex_1 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*) (vfs.db[dIndex].data + (index-8)*4);
            *x = dIndex_1;
            offset = 0;
        }
        else if(index > 8 && tempInode->file_size%256 == 0){
            dIndex = tempInode->dataList[8];
            dIndex_1 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*) (vfs.db[dIndex].data + (index-8)*4);
            *x = dIndex_1;
            offset = 0;
        }
        else{
            offset = tempInode->file_size%vfs.sb.sb.blocksize;
            dIndex = tempInode->dataList[8];
            x = (int*)(vfs.db[dIndex].data + (index-8)*4);
            dIndex_1 = *x;
        }
        strcpy(vfs.db[dIndex_1].data + offset,filename);
        n = (short*) (vfs.db[dIndex_1].data + offset + 30);
        *n = (short) fileInode;
        syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        syncdata(&vfs.sb,&vfs.db[dIndex_1],dIndex_1);
    }
    else{
        if(index == 72 && filesize%256 == 0){
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[9] = dIndex;
            x = (int*)(vfs.db[dIndex].data);
            dIndex_1 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            *x = dIndex_1;
            dIndex_2 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*)(vfs.db[dIndex_1].data);
            *x = dIndex_2;
            offset = 0;
            offset1 = 0;
            offset2 = 0;
        }
        else if(index > 72 && filesize%256 == 0 ){
            if((index-72)%64 == 0){
                offset = ((index-72)/16);
                offset1 = 0;
                offset2 = 0;
                dIndex = tempInode->dataList[9];
                dIndex_1 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
                x = (int*)(vfs.db[dIndex].data+offset);
                *x = dIndex_1;
                dIndex_2 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
                x = (int*)(vfs.db[dIndex_1].data+offset1);
                *x = dIndex_2;
            }
            else{
                offset = (index-72)/16;
                offset1 = ((index-72)%64)*4;
                offset2 = 0;
                dIndex = tempInode->dataList[9];
                dIndex_1 = *((int*)(vfs.db[dIndex].data + offset));
                dIndex_2 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
                x = (int*)(vfs.db[dIndex_1].data + offset1);
                *x = dIndex_2;
            }
        }
        else{
            dIndex = tempInode->dataList[9];
            offset = (index-72)/16;
            offset1 = ((index-72)%64)*4;
            offset2 = filesize%vfs.sb.sb.blocksize;
            dIndex_1 = *((int*)(vfs.db[dIndex].data +offset));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data +offset1));
        }
        strcpy(vfs.db[dIndex_2].data + offset2,filename);
        n = (short*) (vfs.db[dIndex_2].data + offset2 + 30);
        *n = (short) fileInode;
        syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        syncdata(&vfs.sb,&vfs.db[dIndex_1],dIndex_1);
        syncdata(&vfs.sb,&vfs.db[dIndex_2],dIndex_2);
    }
    tempInode->file_size += 32;
    return;
}

int getfilename_inode(char* filename){
    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,index = 0,offset = 0,dIndex, dIndex_1, dIndex_2, fileIndex;   
    while(filesize != 0){
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            fileIndex = *((short*)(vfs.db[dIndex].data+offset+30));
            if(strcmp(vfs.db[dIndex].data+offset,filename) == 0){
                return fileIndex;
            }
            //printf("%s %dB\n",vfs.db[dIndex].data+offset,vfs.inodeList[fileIndex].file_size);
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            }
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4 ));
            fileIndex = *((short*)(vfs.db[dIndex_1].data + offset + 30));
            if(strcmp(vfs.db[dIndex].data+offset,filename) == 0){
                return fileIndex;
            }
            //printf("%s %dB\n",vfs.db[dIndex_1].data+offset,vfs.inodeList[fileIndex].file_size);
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            }
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4 ));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4 ));
            fileIndex = *((short*)(vfs.db[dIndex_2].data + offset + 30));
            if(strcmp(vfs.db[dIndex].data+offset,filename) == 0){
                return fileIndex;
            }
            //printf("%s %dB\n",vfs.db[dIndex_2].data+offset,vfs.inodeList[fileIndex].file_size);
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            } 
        }
    }
    return -1;
}

void restore_index(char* bitmask,int bitmask_size,int index){

    if(bitmask_size == ceil(double(vfs.blocks_for_inodelist)/8))
        vfs.sb.sb.used_inodes -= 1;
    else if(bitmask_size == ceil(double(vfs.blocks_for_datablocks)/8))
        vfs.sb.sb.used_disk_blocks -= 1;

    char* pointer = bitmask + (index/8);
    if(index%8 == 0)
        *pointer = *pointer & (~(0x80));
    else if(index%8 == 1)
        *pointer = *pointer & (~(0x40));
    else if(index%8 == 2)
        *pointer = *pointer & (~(0x20));
    else if(index%8 == 3)
        *pointer = *pointer & (~(0x10));
    else if(index%8 == 4)
        *pointer = *pointer & (~(0x08));
    else if(index%8 == 5)
        *pointer = *pointer & (~(0x04));
    else if(index%8 == 6)
        *pointer = *pointer & (~(0x02));
    else if(index%8 == 7)
        *pointer = *pointer & (~(0x01));
    return;
}

int ls_myfs(){
    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,index = 0,offset = 0,dIndex, dIndex_1, dIndex_2, fileIndex;    
    printf("\n#####Listing current directory #######\n");
    //printf("FILESIZE %d\n",filesize);
    while(filesize != 0){
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            fileIndex = *((short*)(vfs.db[dIndex].data+offset+30));
            printf("%s %dB\n",vfs.db[dIndex].data+offset,vfs.inodeList[fileIndex].file_size);
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            }
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4 ));
            fileIndex = *((short*)(vfs.db[dIndex_1].data + offset + 30));
            printf("%s %dB\n",vfs.db[dIndex_1].data+offset,vfs.inodeList[fileIndex].file_size);
            filesize -= 32;
            offset += 32;
            if(offset == vfs.sb.sb.blocksize){
                offset = 0;
                index += 1;
            }
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4 ));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4 ));
            fileIndex = *((short*)(vfs.db[dIndex_2].data + offset + 30));
            printf("%s %dB\n",vfs.db[dIndex_2].data+offset,vfs.inodeList[fileIndex].file_size);
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

int rm_myfs(char* filename){
    int fileInode = getfilename_inode(filename);
    if(fileInode == -1)
        return -1;
    inode* tempInode = &vfs.inodeList[fileInode];
    int index_count = ceil(double(tempInode->file_size)/vfs.sb.sb.blocksize);
    int index = 0, dIndex, dIndex_1, dIndex_2;
    while(index < index_count){
        if(index < 8){
            dIndex = tempInode->dataList[index];
            restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,dIndex);
            index++;
        }
        else if(index < 72){
            dIndex = tempInode->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4 ));
            restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,dIndex_1);
            index++;
        }
        else{
            if(index == 72){
                restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,tempInode->dataList[8]);
            }
            dIndex = tempInode->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index - 72)/64)*4 ));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index - 72)%64)*4 ));
            restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,dIndex_2);
            index++;
            if((index-72)%64 == 0)
                restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,dIndex_1);
        }
    }
    //restore_index(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size,fileInode);
    // TODO remove filename from pwd_inode

    return 1;
}

int rmdir_myfs(char* dirname){

}

int showfile_myfs(char* filename){
    int index = 0, dIndex, dIndex_1, dIndex_2;
    int fileInode = getfilename_inode(filename);
    printf("\n#####Showing File Inode %d with file Name %s#######\n", fileInode,filename);
    if(fileInode == -1)
        return -1;
    inode* inodeTemp = &vfs.inodeList[fileInode];
    int index_count = ceil(double(inodeTemp->file_size)/vfs.sb.sb.blocksize);
    //printf("Max inodes to get %d\n",index_count);
    
    while(index < index_count){
        //printf("<<<<<<<<<<<<<<<Showing Index %d",index);
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex);
            printf("%s",vfs.db[dIndex].data);
            index++;
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_1);
            printf("%s",vfs.db[dIndex_1].data);
            index++;
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_2);
            printf("%s",vfs.db[dIndex_2].data);
            index++;
        }
    }
    printf("\n");
    return fileInode;
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
    tempInode = &vfs.inodeList[free_inode_index];
    initInode(tempInode);
    tempInode->filetype_permission[0] = 0x01; 
    fd = fopen(source,"r");
    while(feof(fd) == 0){
        free_db_index = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
        n = fread(vfs.db[free_db_index].data,1,vfs.sb.sb.blocksize-1,fd);
        vfs.db[free_db_index].data[n] = '\0';
        //printf("Free DB INDEX%d\n",free_db_index);
        //printf("%s",vfs.db[free_db_index]);
        syncdata(&vfs.sb,&vfs.db[free_db_index],free_db_index);
        insertDataIndex(tempInode,free_db_index); 
        tempInode->file_size += n+1;
    }
    //printf("File size : %d\n",tempInode->file_size);
    insertFileInode(pwd_inode,dest,free_inode_index);
    syncInode(&vfs.sb,tempInode,free_inode_index); 
    syncInode(&vfs.sb,&vfs.inodeList[pwd_inode],pwd_inode);
    syncSB(&vfs.sb);
    fclose(fd);
    /*
    printf("%s %d\n",vfs.db[0].data,*((short*)(vfs.db[0].data+30)));
    printf("%s %d\n",vfs.db[0].data+32,*((short*)(vfs.db[0].data+32+30)));
    printf("%s %d\n",vfs.db[0].data+64,*((short*)(vfs.db[0].data+64+30)));
    */
    return 1;
}

int copy_myfs2pc(char* source, char* dest){
    FILE* fd = fopen(dest,"w+");
    int index = 0, dIndex, dIndex_1, dIndex_2;
    int fileInode = getfilename_inode(source);
    printf("\n#####Copying File Inode %d with file Name %s#######\n", fileInode,source);
    if(fileInode == -1)
        return -1;
    inode* inodeTemp = &vfs.inodeList[fileInode];
    int index_count = ceil(double(inodeTemp->file_size)/vfs.sb.sb.blocksize);
    //printf("Max inodes to get %d\n",index_count);
    
    while(index < index_count){
        //printf("<<<<<<<<<<<<<<<Showing Index %d",index);
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex);
            fwrite(vfs.db[dIndex].data,1,strlen(vfs.db[dIndex].data),fd);
            index++;
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_1);
            fwrite(vfs.db[dIndex_1].data,1,strlen(vfs.db[dIndex_1].data),fd);
            index++;
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_2);
            fwrite(vfs.db[dIndex_2].data,1,strlen(vfs.db[dIndex_2].data),fd);
            index++;
        }
    }
    fclose(fd);
    return 1;
}

int mkdir_myfs(char* dirname){
    int free_inode_index;
    inode* tempInode;
    if((1 > vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks) && vfs.sb.sb.max_inodes - vfs.sb.sb.used_inodes >= 1){
        fprintf(stderr,"Insufficient disk space for data.\n");
        return -1;
    }
    free_inode_index = getfreeindex(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    tempInode = &vfs.inodeList[free_inode_index];
    initInode(tempInode);
    insertFileInode(free_inode_index,".",free_inode_index);
    insertFileInode(free_inode_index,"..",pwd_inode);
    insertFileInode(pwd_inode, dirname,free_inode_index);
    syncInode(&vfs.sb,tempInode,free_inode_index); 
    syncInode(&vfs.sb,&vfs.inodeList[pwd_inode],pwd_inode);
    syncSB(&vfs.sb);
    return 1;
}

int chdir_myfs(char* dirname){
    int fileInode = getfilename_inode(dirname);
    printf("\n#####Changing directory to Inode %d with file Name %s#######\n", fileInode,dirname);
    if(fileInode == -1)
        return -1;
    pwd_inode = fileInode;
    return 1;
}

int open_myfs(char* filename,char mode){
    int fileInode = getfilename_inode(filename);
    int max_index_1 = vft.max_index;
    if(fileInode == -1)
        return -1;
    if(vft.max_index == 0){
        vft.ft = (file_table_entry*) malloc(sizeof(file_table_entry));
        vft.ft[0].offset = -1;
    }

    for(int i = 0; i < max_index_1; i++){
        if(vft.ft[i].offset == -1){
            vft.ft[i].mode = (int) mode; 
            vft.ft[i].offset = 0;
            vft.max_index += 1;
            return i;        
        }
    }
    vft.ft = (file_table_entry*) realloc(vft.ft,sizeof(file_table_entry)*(max_index_1 + 1));
    vft.ft[max_index_1].mode = (int) mode; 
    vft.ft[max_index_1].offset = 0;
    vft.max_index += 1;
    return max_index_1;
}

int close_myfs(int fd){
    if(vft.ft[fd].offset == -1)
        return -1;
    else
        vft.ft[fd].offset = -1;
    return 0;
}
