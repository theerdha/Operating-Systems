#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <stdlib.h>
#define BLOCKSIZE 256

// These data structures for creating a virtual file system in the user space and modify and update the actual file system by synchronizing
// Motivated from kernel virtual file system data structures
// superblock_ and superblock_additional are seperated as we can easily type cast superblock_ and write in memory and we cannot do the same with
// superblock_additional as it contains pointers to arrays
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
// Specification as explained in Lab and Assignment problem statement
typedef struct inode{
    int file_size;
    int dataList[10];
    char timeLastModified[28];
    char timeLastRead[28];
    char filetype_permission[4];
} inode;

// Data structure of datablock
typedef struct datablock{
    char data[BLOCKSIZE];
} datablock;

// Virtual file system
typedef struct filesystem{
    superblock sb; // Super block
    inode* inodeList; // Inode List
    datablock* db; // Data blocks
    // just for convenience
    int blocks_for_superblock;
    int blocks_for_inodelist;
    int blocks_for_datablocks;
} filesystem;

// File Table entry
// For keeping track for open file's identified by file inodes
typedef struct file_table_entry{
    int fileInode; // Inode index corresponds to the file
    int mode; // Tells read or write mode
    int offset; // Offset till which read or write is done
} file_table_entry;

// FILE TABLE
typedef struct filetable{
    file_table_entry* ft; // file descriptor acts as the index to this array
    int max_index; // Tells the highest file descriptor used effectively telling the size of the above array
} filetable;

/*
 * Global variables
 */

char* myfs; // Actual In memory File system
int pwd_inode = 0; // Present working directory inode
filesystem vfs; // Instance of file system
filetable vft; // Instance of file table
int shmid; // Shared memory id for Memory resident file system
int shmid_1; // Shared memory for inode_sem_no
int shmid_2;  // Shared memory for data_sem_no
int shmid_3;  // Shared memory for super_sem_no

// Semaphores for reader writer problem. Check class notes for details
// Considered inode list, super block and data blocks as seperate shared memory and used different semaphores for each
sem_t* sem_inode;
sem_t* mut_inode;
int *inode_sem_no;

sem_t* sem_data;
sem_t* mut_data;
int *data_sem_no;

sem_t* sem_super;
sem_t* mut_super;
int* super_sem_no;

/*
 * Synchronization operations for superblock, inodes and datablock
 */

// Initialize Inode
void initInode(inode* in){
    int i;
    time_t currenttime;
    struct tm* local_time;
    // 0x corresponds to hexadecimal notatoin i.e 4 bits so 0x03 corresponds to the binary number 0000 0011 (8 bits) and note 1 char is 8 bits.
    in->filetype_permission[0] = 0x03;
    in->filetype_permission[1] = 0xFF;
    currenttime = time(NULL);
    local_time = localtime(&currenttime);
    // Converts struct tm into a char* i.e string
    asctime_r(local_time, in->timeLastModified);
    asctime_r(local_time, in->timeLastRead);
    in->file_size = 0;
}

// Synchronize super block with filesystem superblock
// When ever we update the superblock we need to update the superblock present in the memory resident file system
void syncSB(superblock* sb){
    // Acquire lock on super block
    sem_wait(sem_super);
    // Store the superblock_ segment in MRFS
    superblock_* sb_ = (superblock_*) myfs;
    *sb_ = (sb->sb);
    char* myfs_ = myfs;

    // Store the array data in the MRFS
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

    sem_post(sem_super);
    return;
}


// ########################## IGNORE THE COMMENTED CODE BELOW  #######################################

// Synchronize inode with given index
/*
void //syncInode(superblock* sb,inode* in,int index){
    char* myfs_ = myfs;
    char* temp;
    int* temp1;
    inode* in_;
    myfs_ += (index + vfs.blocks_for_superblock)*sb->sb.blocksize;
    in_ = (inode*) myfs_;
    *in_ = *in;
    printf("Size of inode %d\n",sizeof(inode));
    printf("1. %d\n",*((int*)(myfs_)));
    printf("2. %d\n",*((int*)(myfs_+4)));
    printf("3. %d\n",*((char*)(myfs_+44)));

    return;
}
*/
// Synchronize datablock with given index
/*
void syncdata(superblock* sb, datablock* db, int index){
    char* myfs_ = myfs;
    datablock* db_;
    myfs_ += sb->sb.blocksize*(vfs.blocks_for_superblock+vfs.blocks_for_inodelist + index);
    db_ = (datablock*)myfs_;
    *db_ = *db;

    printf("1. %s\n",myfs_);
    printf("2. %d\n",*(unsigned int*)(myfs_+30));
    printf("3. %s\n",myfs_ + 32);
    printf("4. %d\n",*(unsigned int*)(myfs_+62));

    return;
}
*/
// ################################################################################################

// Assuming the system to be byte addressable
int create_myfs(int size,int max_inodes){
    // Size is measured in Mbytes
    // max_inodes gives no of inode blocks
    time_t currenttime;
    struct tm* local_time;
    int i, no_of_data_blocks;
    char* myfs_;
    // Create named semaphores Read about named semaphores in man page
    sem_inode = sem_open("sem_inode_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_inode = sem_open("mut_inode_2",O_CREAT|O_EXCL,S_IRWXU,1);
    sem_super = sem_open("sem_super_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_super = sem_open("mut_super_2",O_CREAT|O_EXCL,S_IRWXU,1);
    sem_data = sem_open("sem_data_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_data = sem_open("mut_data_2",O_CREAT|O_EXCL,S_IRWXU,1);

    // Error checking
    if((sem_inode == (void *)-1) || (mut_inode == (void *)-1) || (sem_inode == (void *)-1) || (sem_super == (void *)-1) || (mut_inode == (void *)-1) || (mut_super == (void *)-1)) {
        printf("sem_open() failed");
        exit(1);
    }

    // Create shared memory segments
    shmid = shmget(IPC_PRIVATE,(size*1024*1024),IPC_CREAT|0700);
    shmid_1 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    shmid_2 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    shmid_3 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    if(shmid == -1)
        perror("Unable to get shared memory.\n");

    // Attaching shared memory and storing the pointer in appropriate variables
    myfs = (char*)shmat(shmid,0,0);
    inode_sem_no = (int*)shmat(shmid_1,0,0);
    data_sem_no = (int*)shmat(shmid_2,0,0);
    super_sem_no = (int*)shmat(shmid_3,0,0);

    *inode_sem_no = 0;
    *data_sem_no = 0;
    *super_sem_no = 0;

    //myfs = (char*) malloc(size*1024*1024*sizeof(char));
    if(myfs == NULL){
        fprintf(stderr,"Error allocating memory\n");
        return -1;
    }
    // Computing the file system size and initializing the super block instance
    myfs_ = myfs;
    vft.max_index = 0;
    vfs.sb.sb.filesystem_size = size * 1024 * 1024;
    vfs.sb.sb.blocksize = BLOCKSIZE;

    // IGNORE //vfs.inodeList = (inode*)malloc(sizeof(inode)*max_inodes);

    // Taking each inode required a single block
    vfs.blocks_for_inodelist = max_inodes;

    // Computing the no of data blocks
    no_of_data_blocks = (size*1024*4 - max_inodes); // Total blocks - blocks for inodes
    vfs.blocks_for_datablocks = no_of_data_blocks - ceil(((double) no_of_data_blocks)/(8*vfs.sb.sb.blocksize) + max_inodes/8 +              ((double) 20)/vfs.sb.sb.blocksize); // Above expression - (blocks required for storing the super block data)
    //                                                                Size of bitmask of data blocks             bitmask size of inode list   size of remaining data in super block
    // Computing the number of blocks for super block
    vfs.blocks_for_superblock = size*1024*4 - vfs.blocks_for_datablocks - vfs.blocks_for_inodelist;
    myfs_ += BLOCKSIZE*vfs.blocks_for_superblock;
    // Pointing the inode list pointer in the vfs to mrfs location
    vfs.inodeList = (inode*)myfs_;
    myfs_ = myfs;
    myfs_ += (vfs.blocks_for_inodelist + vfs.blocks_for_superblock)*BLOCKSIZE;
    //vfs.db = (datablock*) malloc(sizeof(datablock)*no_of_data_blocks);
    // Pointing the data blocks pointer in vfs to mrfs location
    vfs.db = (datablock*)myfs_;

    /* Initialize super block */
    vfs.sb.sb.max_inodes = max_inodes;
    vfs.sb.sb.used_inodes = 1;
    vfs.sb.sb.max_disk_blocks = vfs.blocks_for_datablocks;
    vfs.sb.sb.used_disk_blocks = 1;
    vfs.sb.mask.disk_bitmask_size = ceil(((double)vfs.blocks_for_datablocks)/8);
    vfs.sb.mask.inode_bitmask_size = ceil(((double)vfs.blocks_for_inodelist)/8);
    /*
    myfs_ = myfs;
    myfs_ += sizeof(superblock_);
    vfs.sb.mask.free_disk_bitmask = myfs_;
    myfs_ += vfs.sb.mask.disk_bitmask_size;
    vfs.sb.mask.free_inode_bitmask = myfs_;
    myfs_ += vfs.sb.mask.inode_bitmask_size;
    */
    vfs.sb.mask.free_disk_bitmask = (char*)malloc(vfs.sb.mask.disk_bitmask_size);
    vfs.sb.mask.free_inode_bitmask = (char*) malloc(vfs.sb.mask.inode_bitmask_size);
    bzero(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
    bzero(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    // 0x80 as 1 inode and 1 data block are used for creating root directory inode and root directory data
    vfs.sb.mask.free_inode_bitmask[0] = 0x80;
    vfs.sb.mask.free_disk_bitmask[0] = 0x80;

    // Sync super block
    syncSB(&vfs.sb);
    //printf("inode size %d\n",sizeof(inode));
    //printf("Created superblock with %d blocks\n", vfs.blocks_for_superblock);
    //printf("Created inode list with %d blocks\n", vfs.blocks_for_inodelist);
    //printf("Created data block with %d blocks\n", vfs.blocks_for_datablocks);

    /* ######### Initializing Root Inode #############*/

    // 2 bytes for access permission and file type d-rwx-rwx-rwx 10 bits so 2 bytes
    inode* in_temp = &vfs.inodeList[0];
    initInode(in_temp);
    in_temp->file_size = 32;
    in_temp->dataList[0] = 0;
    //syncInode(&vfs.sb,in_temp,0);

    /* Initializing the root directory */
    // Each directory entry is stored as 32 bytes data. 30 bytes for file/directory name and 2 bytes for inode number.
    strcpy(vfs.db[0].data,".");
    bzero(vfs.db[0].data+30,2);
    //syncdata(&vfs.sb,&vfs.db[0],0);

    return 1;
}

void status_myfs(){
    printf("\n######## File System Info #########\nCreated superblock with %d blocks\n", vfs.blocks_for_superblock);
    printf("Blocks left in inode list with %d blocks\n", vfs.sb.sb.max_inodes - vfs.sb.sb.used_inodes);
    printf("Blocks left in Data blocks with %d blocks\n", vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks);
}

int getfreeindex(char* mask, int length){
    int i, free_index = 0;
    sem_wait(sem_super);
    if(length == ceil(((double)vfs.blocks_for_inodelist)/8)){
        vfs.sb.sb.used_inodes += 1;
    }
    else if(length == ceil(((double)vfs.blocks_for_datablocks)/8)){
        vfs.sb.sb.used_disk_blocks += 1;
    }

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
    sem_post(sem_super);
    return free_index;
}

void insertDataIndex(inode* in,int dIndex){
    int filesize = in->file_size,di,free_db_index,di_2;
    int index = ceil(((double)filesize)/vfs.sb.sb.blocksize);
    //printf("Location to be inserted %d\n", dIndex);
    int* x;
    sem_wait(sem_inode);
    sem_wait(sem_data);
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
        //syncdata(&vfs.sb,&vfs.db[di],di);
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
        if(index != 72 && (index-72)%64 == 0){
            di_2 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*) (vfs.db[di].data + ((index-72)/64)*4);
            *x = di_2;
        }
        di_2 = *((int*)(vfs.db[di].data + ((index-72)/64)*4));
        x = (int*)(vfs.db[di_2].data + ((index-72)%64)*4);
        *x = dIndex;
        //syncdata(&vfs.sb,&vfs.db[di],di);
        //syncdata(&vfs.sb,&vfs.db[di_2],di_2);
    }

    sem_post(sem_data);
    sem_post(sem_inode);
    return;
}

void insertFileInode(int pwd_inode_,char* filename,int fileInode){
    sem_wait(sem_inode);
    sem_wait(sem_data);
    inode* tempInode = &vfs.inodeList[pwd_inode_];
    int filesize = tempInode->file_size;
    int index = tempInode->file_size/vfs.sb.sb.blocksize;
    int dIndex, dIndex_1, dIndex_2;
    int offset, offset1, offset2, free_db_index;
    int* x;
    short* n;
    if(index < 8){
        dIndex = tempInode->dataList[index];
        if(filesize%vfs.sb.sb.blocksize != 0){
            offset = filesize%vfs.sb.sb.blocksize;
            strcpy(vfs.db[dIndex].data + offset,filename);
            n = (short*) (vfs.db[dIndex].data + offset + 30);
            *n = (short)(fileInode);
            //printf("filenmae in %d off %d %s\n",dIndex,offset,vfs.db[0].data);
            //syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        }
        else{
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[index] = dIndex;
            strcpy(vfs.db[dIndex].data ,filename);
            n = (short*) (vfs.db[dIndex].data + 30);
            *n = (short) (fileInode);
            //syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        }
    }
    else if(index < 72){
        if(index == 8 && tempInode->file_size%vfs.sb.sb.blocksize == 0){
            dIndex = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            tempInode->dataList[8] = dIndex;
            dIndex_1 = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
            x = (int*) (vfs.db[dIndex].data + (index-8)*4);
            *x = dIndex_1;
            offset = 0;
        }
        else if(index > 8 && tempInode->file_size%vfs.sb.sb.blocksize == 0){
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
        *n = (short) (fileInode);
        //syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        //syncdata(&vfs.sb,&vfs.db[dIndex_1],dIndex_1);
    }
    else{
        if(index == 72 && filesize%vfs.sb.sb.blocksize == 0){
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
        else if(index > 72 && filesize%vfs.sb.sb.blocksize == 0 ){
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
        //syncdata(&vfs.sb,&vfs.db[dIndex],dIndex);
        //syncdata(&vfs.sb,&vfs.db[dIndex_1],dIndex_1);
        //syncdata(&vfs.sb,&vfs.db[dIndex_2],dIndex_2);
    }
    tempInode->file_size += 32;

    sem_post(sem_data);
    sem_post(sem_inode);
    return;
}

int getfilename_inode(char* filename){

    sem_wait(mut_inode);
    if((*inode_sem_no) == 0){
        (*inode_sem_no)++;
        sem_wait(sem_inode);
    }
    sem_post(mut_inode);

    sem_wait(mut_data);
    if((*data_sem_no) == 0){
        (*data_sem_no)++;
        sem_wait(sem_data);
    }
    sem_post(mut_data);

    int status = 0;
    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,index = 0,offset = 0,dIndex, dIndex_1, dIndex_2, fileIndex = -1;
    while(filesize != 0 && status == 0){
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            fileIndex = *((short*)(vfs.db[dIndex].data+offset+30));
            if(strcmp(vfs.db[dIndex].data+offset,filename) == 0){
                status = 1;
                continue;
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
                status = 1;
                continue;
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
                status = 1;
                continue;
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

    sem_wait(mut_data);
    (*data_sem_no)--;
    if((*data_sem_no) == 0){
        sem_post(sem_data);
    }
    sem_post(mut_data);

    sem_wait(mut_inode);
    (*inode_sem_no)--;
    if((*inode_sem_no) == 0){
        sem_post(sem_inode);
    }
    sem_post(mut_inode);
    if(status == 1)
        return fileIndex;
    return -1;
}

void restore_index(char* bitmask,int bitmask_size,int index){
    sem_wait(sem_super);
    if(bitmask_size == ceil(((double)vfs.blocks_for_inodelist)/8))
        vfs.sb.sb.used_inodes -= 1;
    else if(bitmask_size == ceil(((double)vfs.blocks_for_datablocks)/8))
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
    sem_post(sem_super);
    return;
}

int ls_myfs(){

    sem_wait(mut_inode);
    if((*inode_sem_no) == 0){
        (*inode_sem_no)++;
        sem_wait(sem_inode);
    }
    sem_post(mut_inode);

    sem_wait(mut_data);
    if((*data_sem_no) == 0){
        (*data_sem_no)++;
        sem_wait(sem_data);
    }
    sem_post(mut_data);


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

    sem_wait(mut_data);
    (*data_sem_no)--;
    if((*data_sem_no) == 0){
        sem_post(sem_data);
    }
    sem_post(mut_data);

    sem_wait(mut_inode);
    (*inode_sem_no)--;
    if((*inode_sem_no) == 0){
        sem_post(sem_inode);
    }
    sem_post(mut_inode);

    return 1;
}

char* getInodeOffsetPWD(int index,int offset){
    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,dIndex, dIndex_1, dIndex_2, fileIndex;
    int status = 0;
    if(index < 8){
        dIndex = inodeTemp->dataList[index];
        return (vfs.db[dIndex].data+offset);
    }
    else if(index < 72){
        dIndex = inodeTemp->dataList[8];
        dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4 ));
        return (vfs.db[dIndex_1].data + offset);
    }
    else{
        dIndex = inodeTemp->dataList[9];
        dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4 ));
        dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4 ));
        return (vfs.db[dIndex_2].data + offset);
    }
}

int remove_filename_pwd(char* filename){

    inode* inodeTemp = &vfs.inodeList[pwd_inode];
    int filesize = inodeTemp->file_size,index = 0,offset = 0,dIndex, dIndex_1, dIndex_2, fileIndex;
    int status = 0;
    int pass1,pass2;
    char* da;
    while(filesize != 0){
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            if(status == 1 && filesize != 32){
                if(offset+32 == vfs.sb.sb.blocksize){
                    pass1 = index+1;
                    pass2 = 0;
                }
                else{
                    pass1 = index;
                    pass2 = offset + 32;
                }
                da = getInodeOffsetPWD(pass1,pass2);
                bcopy(da,vfs.db[dIndex].data + offset,32);

            }
            else if(status == 0 && strcmp(vfs.db[dIndex].data+offset,filename) == 0){
                status = 1 ;
                continue;
            }
            else if(filesize == 32){
                break;
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
            if(status == 1 && filesize != 32){
                if(offset+32 == vfs.sb.sb.blocksize){
                    pass1 = index+1;
                    pass2 = 0;
                }
                else{
                    pass1 = index;
                    pass2 = offset + 32;
                }
                da = getInodeOffsetPWD(pass1,pass2);
                bcopy(da,vfs.db[dIndex_1].data + offset,32);
            }
            else if(status == 0 && strcmp(vfs.db[dIndex_1].data+offset,filename) == 0){
                status = 1;
                continue;
            }
            else if(filesize == 32){
                break;
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
            if(status == 1 && filesize != 32){
                if(offset+32 == vfs.sb.sb.blocksize){
                    pass1 = index+1;
                    pass2 = 0;
                }
                else{
                    pass1 = index;
                    pass2 = offset + 32;
                }
                da = getInodeOffsetPWD(pass1,pass2);
                bcopy(da,vfs.db[dIndex_2].data + offset,32);
            }
            else if(status == 0 && strcmp(vfs.db[dIndex_2].data+offset,filename) == 0){
                status = 1;
                continue;
            }
            else if(filesize == 32){
                break;
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
    inodeTemp->file_size -= 32;

    return 1;
}

int rm_myfs(char* filename){

    int fileInode = getfilename_inode(filename);

    if(fileInode == -1)
        return -1;

    sem_wait(sem_inode);
    sem_wait(sem_data);

    inode* tempInode = &vfs.inodeList[fileInode];
    int index_count = ceil(((double)tempInode->file_size)/vfs.sb.sb.blocksize);
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
    remove_filename_pwd(filename);
    restore_index(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size,fileInode);

    sem_post(sem_data);
    sem_post(sem_inode);

    return 1;
}

int rmdir_myfs(char* dirname){

    int fileInode = getfilename_inode(dirname);

    if(fileInode == -1)
        return -1;

    sem_wait(sem_inode);
    sem_wait(sem_data);

    inode* tempInode = &vfs.inodeList[fileInode];
    if(tempInode->file_size != 64){
        printf("Directory not empty\n");

        sem_post(sem_data);
        sem_post(sem_inode);

        return -1;
    }
    int dIndex;
    dIndex = tempInode->dataList[0];
    restore_index(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size,dIndex);
    remove_filename_pwd(dirname);
    restore_index(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size,fileInode);

    sem_post(sem_data);
    sem_post(sem_inode);

    return 1;
}

int showfile_myfs(char* filename){

    int index = 0, dIndex, dIndex_1, dIndex_2;
    int fileInode = getfilename_inode(filename);


    printf("\n#####Showing File Inode %d with file Name %s#######\n", fileInode,filename);
    if(fileInode == -1)
        return -1;

    sem_wait(mut_inode);
    if((*inode_sem_no) == 0){
        (*inode_sem_no)++;
        sem_wait(sem_inode);
    }
    sem_post(mut_inode);

    sem_wait(mut_data);
    if((*data_sem_no) == 0){
        (*data_sem_no)++;
        sem_wait(sem_data);
    }
    sem_post(mut_data);

    inode* inodeTemp = &vfs.inodeList[fileInode];
    int index_count = ceil(((double)inodeTemp->file_size)/vfs.sb.sb.blocksize);
    //printf("Max inodes to get %d\n",index_count);
    char buf[vfs.sb.sb.blocksize+1];
    while(index < index_count){
        //printf("<<<<<<<<<<<<<<<Showing Index %d",index);
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex);
            bcopy(vfs.db[dIndex].data,buf,vfs.sb.sb.blocksize);
            buf[vfs.sb.sb.blocksize] = '\0';
            printf("%s",buf);
            index++;
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_1);
            bcopy(vfs.db[dIndex_1].data,buf,vfs.sb.sb.blocksize);
            buf[vfs.sb.sb.blocksize] = '\0';
            printf("%s",buf);
            index++;
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_2);
            bcopy(vfs.db[dIndex_2].data,buf,vfs.sb.sb.blocksize);
            buf[vfs.sb.sb.blocksize] = '\0';
            printf("%s",buf);
            index++;
        }
    }
    printf("\n");

    sem_wait(mut_data);
    (*data_sem_no)--;
    if((*data_sem_no) == 0){
        sem_post(sem_data);
    }
    sem_post(mut_data);

    sem_wait(mut_inode);
    (*inode_sem_no)--;
    if((*inode_sem_no) == 0){
        sem_post(sem_inode);
    }
    sem_post(mut_inode);

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
    printf("\n#####Copying file Name %s into MRFS #######\n", source);
    // Check disk space availability
    if((ceil(((double)file_size)/vfs.sb.sb.blocksize) > vfs.sb.sb.max_disk_blocks - vfs.sb.sb.used_disk_blocks) && vfs.sb.sb.max_inodes - vfs.sb.sb.used_inodes >= 1){
        fprintf(stderr,"Insufficient disk space for data.\n");
        return -1;
    }

    free_inode_index = getfreeindex(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    //printf("Free inode being used : %d\n",free_inode_index);
    tempInode = &vfs.inodeList[free_inode_index];
    initInode(tempInode);
    tempInode->filetype_permission[0] = 0x01;
    fd = fopen(source,"r");
    while(feof(fd) == 0){
        free_db_index = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
        n = fread(vfs.db[free_db_index].data,1,vfs.sb.sb.blocksize,fd);
        //printf("Free DB INDEX%d\n",free_db_index);
        //printf("%s",vfs.db[free_db_index]);
        //syncdata(&vfs.sb,&vfs.db[free_db_index],free_db_index);
        insertDataIndex(tempInode,free_db_index);
        tempInode->file_size += n;
    }
    //printf("File size : %d\n",tempInode->file_size);
    insertFileInode(pwd_inode,dest,free_inode_index);

    //syncInode(&vfs.sb,tempInode,free_inode_index);
    //syncInode(&vfs.sb,&vfs.inodeList[pwd_inode],pwd_inode);
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

    sem_wait(mut_inode);
    if((*inode_sem_no) == 0){
        (*inode_sem_no)++;
        sem_wait(sem_inode);
    }
    sem_post(mut_inode);

    sem_wait(mut_data);
    if((*data_sem_no) == 0){
        (*data_sem_no)++;
        sem_wait(sem_data);
    }
    sem_post(mut_data);

    inode* inodeTemp = &vfs.inodeList[fileInode];
    int index_count = ceil(((double)inodeTemp->file_size)/vfs.sb.sb.blocksize),size;
    //printf("Max inodes to get %d\n",index_count);

    while(index < index_count){
        //printf("<<<<<<<<<<<<<<<Showing Index %d",index);
        size = ((index == index_count - 1) ? (inodeTemp->file_size%vfs.sb.sb.blocksize) : (vfs.sb.sb.blocksize));
        if(index < 8){
            dIndex = inodeTemp->dataList[index];
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex);
            fwrite(vfs.db[dIndex].data,1,size,fd);
            index++;
        }
        else if(index < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (index-8)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_1);
            fwrite(vfs.db[dIndex_1].data,1,size,fd);
            index++;
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((index-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((index-72)%64)*4));
            //printf("<<<<<<<<<<<<<<<Showing Index %d\n",dIndex_2);
            fwrite(vfs.db[dIndex_2].data,1,size,fd);
            index++;
        }
    }
    fclose(fd);

    sem_wait(mut_data);
    (*data_sem_no)--;
    if((*data_sem_no) == 0){
        sem_post(sem_data);
    }
    sem_post(mut_data);

    sem_wait(mut_inode);
    (*inode_sem_no)--;
    if((*inode_sem_no) == 0){
        sem_post(sem_inode);
    }
    sem_post(mut_inode);

    return 1;

}

int mkdir_myfs(char* dirname){
    int free_inode_index;
    inode* tempInode;
    printf("\n#####Making directory %s#######\n", dirname);
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
    //syncInode(&vfs.sb,tempInode,free_inode_index);
    //syncInode(&vfs.sb,&vfs.inodeList[pwd_inode],pwd_inode);
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
    int max_index_1 = vft.max_index,free_inode_index,i;
    inode* tempInode;
    if(fileInode == -1 && mode == 'r')
        return -1;
    else if(fileInode == -1 && mode == 'w'){
        printf("No file with %s name exists. Creating file.\n",filename);
        free_inode_index = getfreeindex(vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
        tempInode = &vfs.inodeList[free_inode_index];
        initInode(tempInode);
        tempInode->filetype_permission[0] = 0x01;
        insertFileInode(pwd_inode,filename,free_inode_index);
        fileInode = free_inode_index;
        syncSB(&vfs.sb);
    }
    if(vft.max_index == 0){
        vft.ft = (file_table_entry*) malloc(sizeof(file_table_entry));
        vft.ft[0].offset = -1;
    }

    for( i = 0; i < max_index_1; i++){
        if(vft.ft[i].offset == -1){
            vft.ft[i].mode = (int) mode;
            vft.ft[i].offset = 0;
            vft.ft[i].fileInode = fileInode;
            vft.max_index += 1;
            return i;
        }
    }
    vft.ft = (file_table_entry*) realloc(vft.ft,sizeof(file_table_entry)*(max_index_1 + 1));
    vft.ft[max_index_1].mode = (int) mode;
    vft.ft[max_index_1].offset = 0;
    vft.ft[max_index_1].fileInode = fileInode;
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

int eof_myfs(int fd){
    int fileInode = vft.ft[fd].fileInode;
    inode* inodeTemp = &vfs.inodeList[fileInode];
    if(fileInode == -1)
        return -1;
    if(inodeTemp->file_size == vft.ft[fd].offset)
        return 1;
    return 0;
}

int read_myfs(int fd,int nbytes, char* buf){
    char* point = buf;
    int offset = vft.ft[fd].offset, dIndex, dIndex_1, dIndex_2, offset_;
    offset_ = offset;
    int fileInode = vft.ft[fd].fileInode,base, i1, i2, temp, bytes_read = 0,bytes_to_read;
    inode * inodeTemp;
    int blocksize = vfs.sb.sb.blocksize;
    int index,upper_bound = blocksize;
    if(vft.ft[fd].mode != ((int)'r'))
        return -1;

    sem_wait(mut_inode);
    if((*inode_sem_no) == 0){
        (*inode_sem_no)++;
        sem_wait(sem_inode);
    }
    sem_post(mut_inode);

    sem_wait(mut_data);
    if((*data_sem_no) == 0){
        (*data_sem_no)++;
        sem_wait(sem_data);
    }
    sem_post(mut_data);

    inodeTemp = &vfs.inodeList[fileInode];
    index = ceil(((double)inodeTemp->file_size)/blocksize);
    i1 = offset/blocksize;
    i2 = ceil(((double)offset + nbytes)/blocksize);
    //printf("Reading in file with offset %d and %d bytes and filesize%d\n",offset,nbytes,inodeTemp->file_size);
    base = i1;
    while(base < index && base < i2){
        if(base < 8){
            dIndex = inodeTemp->dataList[base];
            if(base == i2-1){
                temp = ((offset_ + nbytes < inodeTemp->file_size) ? (offset_ + nbytes) : (inodeTemp->file_size));
                bytes_to_read = (temp - offset)%blocksize;
                bcopy(vfs.db[dIndex].data+ offset%blocksize, point,  bytes_to_read);
                bytes_read += bytes_to_read;
                offset += bytes_to_read;
                base += 1;
                vft.ft[fd].offset = offset;
            }
            else{
                if(base == index-1)
                    upper_bound = inodeTemp->file_size%blocksize;
                bcopy(vfs.db[dIndex].data+ offset%blocksize, point, upper_bound - offset%blocksize );
                point += upper_bound - offset%blocksize;
                bytes_read += upper_bound - offset%blocksize;
                offset += upper_bound - offset%blocksize;
                base += 1;
            }
        }
        else if(base < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (base-8)*4));
            if(base == i2-1){
                temp = ((offset_ + nbytes < inodeTemp->file_size) ? (offset_ + nbytes) : (inodeTemp->file_size));
                bytes_to_read = (temp - offset)%blocksize;
                bcopy(vfs.db[dIndex_1].data+ offset%blocksize, point, bytes_to_read );
                bytes_read += bytes_to_read;
                offset += bytes_to_read;
                base += 1;
                vft.ft[fd].offset = offset;
            }
            else{
                if(base == index-1)
                    upper_bound = inodeTemp->file_size%blocksize;
                bcopy(vfs.db[dIndex_1].data+ offset%blocksize, point, upper_bound - offset%blocksize );
                point += upper_bound - offset%blocksize;
                bytes_read += upper_bound - offset%blocksize;
                offset += upper_bound - offset%blocksize;
                base += 1;
            }
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((base-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((base-72)%64)*4));
            if(base == i2-1){
                temp = ((offset_ + nbytes < inodeTemp->file_size) ? (offset_ + nbytes) : (inodeTemp->file_size));
                bytes_to_read = (temp - offset)%blocksize;
                bcopy(vfs.db[dIndex_2].data+ offset%blocksize, point, bytes_to_read );
                bytes_read += bytes_to_read;
                offset += bytes_to_read;
                base += 1;
                vft.ft[fd].offset = offset;
            }
            else{
                if(base == index-1)
                    upper_bound = inodeTemp->file_size%blocksize;
                bcopy(vfs.db[dIndex_2].data+ offset%blocksize, point, upper_bound - offset%blocksize );
                point += upper_bound - offset%blocksize;
                bytes_read += upper_bound - offset%blocksize;
                offset += upper_bound - offset%blocksize;
                base += 1;
            }
        }
        //printf("Base %d\n",base);
    }
    vft.ft[fd].offset = offset;

    sem_wait(mut_data);
    (*data_sem_no)--;
    if((*data_sem_no) == 0){
        sem_post(sem_data);
    }
    sem_post(mut_data);

    sem_wait(mut_inode);
    (*inode_sem_no)--;
    if((*inode_sem_no) == 0){
        sem_post(sem_inode);
    }
    sem_post(mut_inode);

    if(base == index || base == i2)
        return bytes_read;

    return -1;
}

int write_myfs(int fd,int nbytes, char* buf){
    char* point = buf;
    int offset_;
    int offset = vft.ft[fd].offset, dIndex, dIndex_1, dIndex_2;
    offset_ = offset;
    int fileInode = vft.ft[fd].fileInode,base, i1, i2, temp, bytes_read = 0,free_db_index;
    inode * inodeTemp;
    int* x;
    int blocksize = vfs.sb.sb.blocksize;
    int index,upper_bound = blocksize,bytes_to_write;
    if(vft.ft[fd].mode != ((int)'w'))
        return -1;

    inodeTemp = &vfs.inodeList[fileInode];
    index = ceil(((double)inodeTemp->file_size)/blocksize);
    i1 = offset/blocksize;
    i2 = ceil(((double)offset + nbytes)/blocksize);
    //printf("Writing in file with offset %d and %d bytes and filesize%d\n",offset,nbytes,inodeTemp->file_size);
    base = i1;
    while(i2 > index){
        //printf("Creating a datablock.\n");
        free_db_index = getfreeindex(vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
        bzero(vfs.db[free_db_index].data,blocksize);
        insertDataIndex(inodeTemp,free_db_index);
        if(index < i2 - 1)
            inodeTemp->file_size += blocksize;
        else
            inodeTemp->file_size = offset + nbytes;
        index++;
    }

    sem_wait(sem_inode);
    sem_wait(sem_data);

    if(inodeTemp->file_size < offset+nbytes)
        inodeTemp->file_size = offset + nbytes;
    syncSB(&vfs.sb);

    while(base < i2){
        if(base < 8){
            dIndex = inodeTemp->dataList[base];
            if(base == i2-1){
                temp = offset + nbytes;
                bytes_to_write = (offset_+nbytes-offset)%blocksize;
                bcopy(point, vfs.db[dIndex].data+ offset%blocksize, bytes_to_write );
                bytes_read += bytes_to_write;
                offset += bytes_to_write;
            }
            else{
                bytes_to_write = blocksize - offset%blocksize;
                bcopy(point, vfs.db[dIndex].data+ offset%blocksize, blocksize - offset%blocksize );
                point += blocksize - offset%blocksize;
                bytes_read += blocksize - offset%blocksize;
                offset += blocksize - offset%blocksize;
            }
        }
        else if(base < 72){
            dIndex = inodeTemp->dataList[8];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + (base-8)*4));
            if(base == i2-1){
                temp = (offset + nbytes);
                bytes_to_write = (offset_+nbytes-offset)%blocksize;
                bcopy(point, vfs.db[dIndex_1].data+ offset%blocksize, bytes_to_write );
                bytes_read += bytes_to_write;
                offset += bytes_to_write;
            }
            else{
                bcopy(point, vfs.db[dIndex_1].data+ offset%blocksize, blocksize - offset%blocksize );
                point += blocksize - offset%blocksize;
                bytes_read += blocksize - offset%blocksize;
                offset += blocksize - offset%blocksize;
            }
        }
        else{
            dIndex = inodeTemp->dataList[9];
            dIndex_1 = *((int*)(vfs.db[dIndex].data + ((base-72)/64)*4));
            dIndex_2 = *((int*)(vfs.db[dIndex_1].data + ((base-72)%64)*4));
            if(base == i2-1){
                temp = (offset + nbytes) ;
                bytes_to_write = (offset_+nbytes-offset)%blocksize;
                bcopy(point, vfs.db[dIndex_2].data+ offset%blocksize, bytes_to_write );
                bytes_read += bytes_to_write;
                offset += bytes_to_write;
            }
            else{
                bcopy(point, vfs.db[dIndex_2].data+ offset%blocksize, blocksize - offset%blocksize );
                point += blocksize - offset%blocksize;
                bytes_read += blocksize - offset%blocksize;
                offset += blocksize - offset%blocksize;
            }
        }
        base += 1;
    }
    vft.ft[fd].offset = offset;

    sem_post(sem_data);
    sem_post(sem_inode);

    if(base == index)
        return bytes_read;

    return -1;
}


int dump_myfs(char* dumpfile){
    FILE* fd = fopen(dumpfile,"w+");
    int n = fwrite(myfs,1,vfs.sb.sb.filesystem_size,fd);
    fclose(fd);
    printf("######## Creating backup for MRFS filesystem FILE NAME : %s\n",dumpfile);
    return n;
}

int restore_myfs(char* dumpfile){
    printf("######## Restoring backup for MRFS filesystem FILE NAME : %s\n",dumpfile);
    struct stat st;
    stat(dumpfile,&st);
    FILE* fd = fopen(dumpfile,"r");
    int n;
    char* myfs_;

    sem_inode = sem_open("sem_inode_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_inode = sem_open("mut_inode_2",O_CREAT|O_EXCL,S_IRWXU,1);
    sem_super = sem_open("sem_super_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_super = sem_open("mut_super_2",O_CREAT|O_EXCL,S_IRWXU,1);
    sem_data = sem_open("sem_data_2",O_CREAT|O_EXCL,S_IRWXU,1);
    mut_data = sem_open("mut_data_2",O_CREAT|O_EXCL,S_IRWXU,1);

    if((sem_inode == (void *)-1) || (mut_inode == (void *)-1) || (sem_inode == (void *)-1) || (sem_super == (void *)-1) || (mut_inode == (void *)-1) || (mut_super == (void *)-1)) {
        printf("sem_open() failed");
        exit(1);
    }

    shmid = shmget(IPC_PRIVATE,st.st_size,IPC_CREAT|0700);
    shmid_1 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    shmid_2 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    shmid_3 = shmget(IPC_PRIVATE,4,IPC_CREAT|0700);
    if(shmid == -1)
        perror("Unable to get shared memory.\n");

    myfs = (char*)shmat(shmid,0,0);
    inode_sem_no = (int*)shmat(shmid_1,0,0);
    data_sem_no = (int*)shmat(shmid_2,0,0);
    super_sem_no = (int*)shmat(shmid_3,0,0);

    n = fread(myfs,st.st_size,1,fd);

    *inode_sem_no = 0;
    *data_sem_no = 0;
    *super_sem_no = 0;

    //printf("Restored %d bytes from backup\n",n);
    vft.max_index = 0;

    vfs.sb.sb = *((superblock_*)(myfs));
    //printf("size %d\n",vfs.sb.sb.blocksize);
    vfs.blocks_for_datablocks = vfs.sb.sb.max_disk_blocks;
    vfs.blocks_for_inodelist = vfs.sb.sb.max_inodes;
    vfs.blocks_for_superblock = (vfs.sb.sb.filesystem_size/vfs.sb.sb.blocksize) - vfs.blocks_for_datablocks - vfs.blocks_for_inodelist;
    vfs.sb.mask.disk_bitmask_size = ceil(((double)vfs.blocks_for_datablocks)/8);
    vfs.sb.mask.inode_bitmask_size = ceil(((double)vfs.blocks_for_inodelist)/8);
    vfs.sb.mask.free_disk_bitmask = (char*)malloc(vfs.sb.mask.disk_bitmask_size);
    vfs.sb.mask.free_inode_bitmask = (char*) malloc(vfs.sb.mask.inode_bitmask_size);
    myfs_ = myfs;
    myfs_ += sizeof(superblock_);
    bcopy(myfs_, vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
    myfs_ += vfs.sb.mask.disk_bitmask_size;
    bcopy(myfs_, vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    myfs_ += vfs.sb.mask.inode_bitmask_size;

    vfs.inodeList = (inode*)(myfs + vfs.sb.sb.blocksize*vfs.blocks_for_superblock);
    vfs.db = (datablock*)(myfs + vfs.sb.sb.blocksize*(vfs.blocks_for_superblock+vfs.blocks_for_inodelist));
    fclose(fd);
    return n;
}

void sync_shared_myfs(){
    char* myfs_;
    vft.max_index = 0;

    sem_inode = sem_open("sem_inode_2",O_EXCL,S_IRWXU,1);
    mut_inode = sem_open("mut_inode_2",O_EXCL,S_IRWXU,1);
    sem_super = sem_open("sem_super_2",O_EXCL,S_IRWXU,1);
    mut_super = sem_open("mut_super_2",O_EXCL,S_IRWXU,1);
    sem_data = sem_open("sem_data_2",O_EXCL,S_IRWXU,1);
    mut_data = sem_open("mut_data_2",O_EXCL,S_IRWXU,1);

    myfs = (char*)shmat(shmid,0,0);

    inode_sem_no = (int*)shmat(shmid_1,0,0);
    data_sem_no = (int*)shmat(shmid_2,0,0);
    super_sem_no = (int*)shmat(shmid_3,0,0);

    vfs.sb.sb = *((superblock_*)(myfs));
    //printf("size %d\n",vfs.sb.sb.blocksize);
    vfs.blocks_for_datablocks = vfs.sb.sb.max_disk_blocks;
    vfs.blocks_for_inodelist = vfs.sb.sb.max_inodes;
    vfs.blocks_for_superblock = (vfs.sb.sb.filesystem_size/vfs.sb.sb.blocksize) - vfs.blocks_for_datablocks - vfs.blocks_for_inodelist;
    vfs.sb.mask.disk_bitmask_size = ceil(((double)vfs.blocks_for_datablocks)/8);
    vfs.sb.mask.inode_bitmask_size = ceil(((double)vfs.blocks_for_inodelist)/8);
    vfs.sb.mask.free_disk_bitmask = (char*)malloc(vfs.sb.mask.disk_bitmask_size);
    vfs.sb.mask.free_inode_bitmask = (char*) malloc(vfs.sb.mask.inode_bitmask_size);
    myfs_ = myfs;
    myfs_ += sizeof(superblock_);
    bcopy(myfs_, vfs.sb.mask.free_disk_bitmask,vfs.sb.mask.disk_bitmask_size);
    myfs_ += vfs.sb.mask.disk_bitmask_size;
    bcopy(myfs_, vfs.sb.mask.free_inode_bitmask,vfs.sb.mask.inode_bitmask_size);
    myfs_ += vfs.sb.mask.inode_bitmask_size;

    vfs.inodeList = (inode*)(myfs + vfs.sb.sb.blocksize*vfs.blocks_for_superblock);
    vfs.db = (datablock*)(myfs + vfs.sb.sb.blocksize*(vfs.blocks_for_superblock+vfs.blocks_for_inodelist));

}
