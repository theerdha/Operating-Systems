char* myfs;
int blocks_for_superblock = 0;
int block_size = 256;

// Assuming the system to be byte addressable
int create_myfs(int size,int max_inodes){
	// Size is measured in Mbytes
	// max_inodes gives no of inode blocks 
	try{
		myfs = new char[size*1024*1024];
		char* myfs_ = myfs;
		int no_of_data_blocks;
		/* Initialize super block */
		
		// Initialize file system size - 4bytes
		*myfs_ = size;
		myfs_ += 4;
		
		// Initialize max no of indoes - 4bytes
		*myfs_ = int(max_inodes);
		myfs_ += 4;
		
		// Initialize no of inodes used - 4bytes
		*myfs_ = int(0);
		myfs_ += 4;
		
		// Initialize max no of diskblock - 4bytes
		// computing size of super block 
		no_of_data_blocks = (size*1024*4 - max_inodes);
		no_of_data_blocks = no_of_data_blocks - ceil(no_of_data_blocks/(8*block_size) + max_inodes/8 + double(20)/block_size);
		blocks_for_superblock = size*1024*4 - no_of_data_blocks - max_inodes;
		*myfs_ = no_of_data_blocks; // Total block minus blocks for super block minus block for inode list
		myfs_ += 4;
		
		// Initialize no of diskblock used - 4bytes
		*myfs_ = int(0);
		myfs_ += 4;
		
		// Initialize list of free disks blocks
		for(int i = 0; i < ceil(no_of_data_blocks/8); i++){
			*myfs_ = 0x00;
			myfs_ += 1;
		}
		//cout << "Created Free disk blocks bitmask with " << ceil(no_of_data_blocks/8) << " bytes\n";

		// Initilize list of free inode blocks
		// First Inode contains the root directory
		*myfs_ = 0x80;
		myfs_ += 1;
		for(int i = 1; i < ceil(max_inodes/8); i++){
			*myfs_ = 0x00;
			myfs_ += 1;
		}
		cout << "Created Super Block spannig upto " << blocks_for_superblock << " block\n";

		/* Initializing Root Inode */
		myfs_ = myfs;
		myfs_ += block_size*blocks_for_superblock;
		// 2 bytes for access permission and file type d-rwx-rwx-rwx 9 bits so 2 bytes
		*myfs_ = 0x03;
		myfs_ +=1 ;
		*myfs_ = 0xFF;
		myfs_ +=1 ;
		// Setting time last modified and time last read
		
		return 0;
	}	
	catch{
		cerr << "Cannot allocate memory for super block\n";
		return -1;
	}
}

