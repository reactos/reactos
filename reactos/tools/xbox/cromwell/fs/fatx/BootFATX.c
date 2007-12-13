// Functions for processing FATX partitions
// (c) 2001 Andrew de Quincey

#include "boot.h"
#include "BootFATX.h"
#include <sys/types.h>


#undef FATX_DEBUG

//#define FATX_INFO

int checkForLastDirectoryEntry(unsigned char* entry) {

	// if the filename length byte is 0 or 0xff,
	// this is the last entry
	if ((entry[0] == 0xff) || (entry[0] == 0)) {
		return 1;
	}

	// wasn't last entry
	return 0;
}

int LoadFATXFilefixed(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo,BYTE* Position) {

	if(partition == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
	} else {
		if(FATXFindFile(partition,filename,FATX_ROOT_FAT_CLUSTER,fileinfo)) {
#ifdef FATX_DEBUG
			printk("ClusterID : %d\n",fileinfo->clusterId);
			printk("fileSize  : %d\n",fileinfo->fileSize);
#endif
			fileinfo->buffer = Position;
			memset(fileinfo->buffer,0xff,fileinfo->fileSize);
			
			if(FATXLoadFromDisk(partition, fileinfo)) {
				return true;
			} else {
#ifdef FATX_INFO
				printk("LoadFATXFile : error loading %s\n",filename);
#endif
				return false;
			}
		} else {
#ifdef FATX_INFO
			printk("LoadFATXFile : file %s not found\n",filename);
#endif
			return false;
		}
	}
	return false;
}

int LoadFATXFile(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo) {

	if(partition == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("LoadFATXFile : no open FATX partition\n");
#endif
	} else {
		if(FATXFindFile(partition,filename,FATX_ROOT_FAT_CLUSTER,fileinfo)) {
#ifdef FATX_DEBUG
			printk("ClusterID : %d\n",fileinfo->clusterId);
			printk("fileSize  : %d\n",fileinfo->fileSize);
#endif
			fileinfo->buffer = malloc(fileinfo->fileSize);
			memset(fileinfo->buffer,0,fileinfo->fileSize);
			if(FATXLoadFromDisk(partition, fileinfo)) {
				return true;
			} else {
#ifdef FATX_INFO
				printk("LoadFATXFile : error loading %s\n",filename);
#endif
				return false;
			}
		} else {
#ifdef FATX_INFO
			printk("LoadFATXFile : file %s not found\n",filename);
#endif
			return false;
		}
	}
	return false;
}

void PrintFAXPartitionTable(int nDriveIndex) {

	BYTE ba[512];
	FATXPartition *partition = NULL;
	FATXFILEINFO fileinfo;

	VIDEO_ATTR=0xffe8e8e8;
	printk("FATX Partition Table:\n");
	memset(&fileinfo,0,sizeof(FATXFILEINFO));

	if(FATXSignature(nDriveIndex,SECTOR_SYSTEM,ba)) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("Partition SYSTEM\n");
		partition = OpenFATXPartition(nDriveIndex,SECTOR_SYSTEM,SYSTEM_SIZE);
		if(partition == NULL) {
			VIDEO_ATTR=0xffe8e8e8;
			printk("PrintFAXPartitionTable : error on opening STORE\n");
		} else {
			DumpFATXTree(partition);
		}
	}

	VIDEO_ATTR=0xffc8c8c8;
}

int FATXSignature(int nDriveIndex,unsigned int block,BYTE *ba) {

	if(BootIdeReadSector(0, &ba[0], block, 0, 512)) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("FATXSignature : Unable to read FATX sector\n");
#endif
		return false;
	} else {
		if( (ba[0]=='F') && (ba[1]=='A') && (ba[2]=='T') && (ba[3]=='X') ) {
			return true;
		} else {
			return false;
		}
	}
}

FATXPartition *OpenFATXPartition(int nDriveIndex,
		unsigned int partitionOffset,
		u_int64_t partitionSize) {
	unsigned char partitionInfo[FATX_PARTITION_HEADERSIZE];
	FATXPartition *partition;
	int readSize;
	unsigned int chainTableSize;

#ifdef FATX_DEBUG
	printk("OpenFATXPartition : Read partition header\n");
#endif
	// load the partition header
	readSize = FATXRawRead(nDriveIndex, partitionOffset, 0,
			FATX_PARTITION_HEADERSIZE, (char *)&partitionInfo);

  	if (readSize != FATX_PARTITION_HEADERSIZE) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("OpenFATXPartition : Out of data while reading partition header\n");
#endif
		return NULL;
	}

	// check the magic
	if (*((u_int32_t*) &partitionInfo) != FATX_PARTITION_MAGIC) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("OpenFATXPartition : No FATX partition found at requested offset\n");
#endif
		return NULL;
	}

#ifdef FATX_DEBUG
	printk("OpenFATXPartition : Allocating Partition struct\n");
#endif
	// make up new structure
	partition = (FATXPartition*) malloc(sizeof(FATXPartition));
	if (partition == NULL) {
#ifdef FATX_INFO
		printk("OpenFATXPartition : Out of memory\n");
#endif
		return NULL;
	}
#ifdef FATX_DEBUG
	printk("OpenFATXPartition : Allocating Partition struct done\n");
#endif
	memset(partition,0,sizeof(FATXPartition));

	// setup the easy bits
	partition->nDriveIndex = nDriveIndex;
	partition->partitionStart = partitionOffset;
	partition->partitionSize = partitionSize;
	partition->clusterSize = 0x4000;
	partition->clusterCount = partition->partitionSize / 0x4000;
	partition->chainMapEntrySize = (partition->clusterCount >= 0xfff4) ? 4 : 2;

	// Now, work out the size of the cluster chain map table
	chainTableSize = partition->clusterCount * partition->chainMapEntrySize;
	if (chainTableSize % FATX_CHAINTABLE_BLOCKSIZE) {
		// round up to nearest FATX_CHAINTABLE_BLOCKSIZE bytes
		chainTableSize = ((chainTableSize / FATX_CHAINTABLE_BLOCKSIZE) + 1)
				* FATX_CHAINTABLE_BLOCKSIZE;
	}

#ifdef FATX_DEBUG
	printk("OpenFATXPartition : Allocating chaintable struct\n");
#endif
  	// Load the cluster chain map table
	partition->clusterChainMap.words = (u_int16_t*) malloc(chainTableSize);
    	if (partition->clusterChainMap.words == NULL) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("OpenFATXPartition : Out of memory\n");
#endif
		return NULL;
	}

#ifdef FATX_DEBUG
	printk("Part stats : CL Count	%d \n", partition->clusterCount);
	printk("Part stats : CL Size	%d \n", partition->clusterSize);
	printk("Part stats : CM Size	%d \n", partition->chainMapEntrySize);
	printk("Part stats : Table Size	%d \n", chainTableSize);
	printk("Part stats : Part Size	%d \n", partition->partitionSize);
#endif

	readSize = FATXRawRead(nDriveIndex, partitionOffset, FATX_PARTITION_HEADERSIZE,
			chainTableSize, (char *)partition->clusterChainMap.words);

    	if (readSize != chainTableSize) {
		VIDEO_ATTR=0xffe8e8e8;
#ifdef FATX_INFO
		printk("Out of data while reading cluster chain map table\n");
#endif
	}
	partition->cluster1Address = ( ( FATX_PARTITION_HEADERSIZE + chainTableSize) );

	return partition;
}

void DumpFATXTree(FATXPartition *partition) {
	// OK, start off the recursion at the root FAT
	_DumpFATXTree(partition, FATX_ROOT_FAT_CLUSTER, 0);
}

void _DumpFATXTree(FATXPartition* partition, int clusterId, int nesting) {

	int endOfDirectory;
	unsigned char* curEntry;
	unsigned char clusterData[partition->clusterSize];
	int i,j;
	char writeBuf[512];
	char filename[50];
	u_int32_t filenameSize;
	u_int32_t fileSize;
	u_int32_t entryClusterId;
	unsigned char flags;
	char flagsStr[5];

	// OK, output all the directory entries
	endOfDirectory = 0;
	while(clusterId != -1) {
		LoadFATXCluster(partition, clusterId, clusterData);

		// loop through it, outputing entries
		for(i=0; i< partition->clusterSize / FATX_DIRECTORYENTRY_SIZE; i++) {

			// work out the currentEntry
			curEntry = clusterData + (i * FATX_DIRECTORYENTRY_SIZE);

			// first of all, check that it isn't an end of directory marker
			if (checkForLastDirectoryEntry(curEntry)) {
				endOfDirectory = 1;
				break;
			}

			// get the filename size
			filenameSize = curEntry[0];

			// check if file is deleted
			if (filenameSize == 0xE5) {
				continue;
			}

			// check size is OK
			if ((filenameSize < 1) || (filenameSize > FATX_FILENAME_MAX)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("_DumpFATXTree : Invalid filename size: %i\n", filenameSize);
			}

			// extract the filename
			memset(filename, 0, 50);
			memcpy(filename, curEntry+2, filenameSize);
			filename[filenameSize] = 0;

			// get rest of data
			flags = curEntry[1];
			entryClusterId = *((u_int32_t*) (curEntry + 0x2c));
			fileSize = *((u_int32_t*) (curEntry + 0x30));

			// wipe fileSize
			if (flags & FATX_FILEATTR_DIRECTORY) {
				fileSize = 0;
			}

			// zap flagsStr
			strcpy(flagsStr, "    ");

			// work out other flags
			if (flags & FATX_FILEATTR_READONLY) {
	              		flagsStr[0] = 'R';
			}
			if (flags & FATX_FILEATTR_HIDDEN) {
				flagsStr[1] = 'H';
			}
			if (flags & FATX_FILEATTR_SYSTEM) {
				flagsStr[2] = 'S';
			}
			if (flags & FATX_FILEATTR_ARCHIVE) {
				flagsStr[3] = 'A';
			}

			// check we don't have any unknown flags
/*
			if (flags & 0xc8) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("WARNING: file %s has unknown flags %x\n", filename, flags);
			}
*/
			
			// Output it
			for(j=0; j< nesting; j++) {
				writeBuf[j] = ' ';
			}

			VIDEO_ATTR=0xffe8e8e8;
			printk("/%s  [%s] (SZ:%i CL%x))\n",filename, flagsStr,
					fileSize, entryClusterId);

			// If it is a sub-directory, recurse
			/*
			if (flags & FATX_FILEATTR_DIRECTORY) {
				_DumpFATXTree(partition, entryClusterId, nesting+1);
			}
			*/
			// have we hit the end of the directory yet?
		}		
		if (endOfDirectory) {
			break;
		}	
		clusterId = getNextClusterInChain(partition, clusterId);
	}
}

int FATXLoadFromDisk(FATXPartition* partition, FATXFILEINFO *fileinfo) {

	unsigned char clusterData[partition->clusterSize];
	int fileSize = fileinfo->fileSize;
	int written;
	int clusterId = fileinfo->clusterId;
	BYTE *ptr;

	fileinfo->fileRead = 0;
	ptr = fileinfo->buffer;

	// loop, outputting clusters
	while(clusterId != -1) {
		// Load the cluster data
		LoadFATXCluster(partition, clusterId, clusterData);

		// Now, output it
		written = (fileSize <= partition->clusterSize) ? fileSize : partition->clusterSize;
		memcpy(ptr,clusterData,written);
		fileSize -= written;
		fileinfo->fileRead+=written;
		ptr+=written;

		// Find next cluster
		clusterId = getNextClusterInChain(partition, clusterId);
	}

	// check we actually found enough data
	if (fileSize != 0) {
#ifdef FATX_INFO
   	printk("Hit end of cluster chain before file size was zero\n");
#endif
		//free(fileinfo->buffer);
		//fileinfo->buffer = NULL;
		return false;
	}
	return true;
}

int FATXFindFile(FATXPartition* partition,
                    char* filename,
                    int clusterId, FATXFILEINFO *fileinfo) {

	int i = 0;
#ifdef FATX_DEBUG
	VIDEO_ATTR=0xffc8c8c8;
	printk("FATXFindFile : %s\n",filename);
#endif

  	// convert any '\' to '/' characters
  	while(filename[i] != 0) {
	    	if (filename[i] == '\\') {
      			filename[i] = '/';
	    	}
	    	i++;
  	}
  	
	// skip over any leading / characters
  	i=0;
  	while((filename[i] != 0) && (filename[i] == '/')) {
	    	i++;
  	}

	return _FATXFindFile(partition,&filename[i],clusterId,fileinfo);
}

int _FATXFindFile(FATXPartition* partition,
                    char* filename,
                    int clusterId, FATXFILEINFO *fileinfo) {
	unsigned char* curEntry;
	unsigned char clusterData[partition->clusterSize];
	int i = 0;
	int endOfDirectory;
	u_int32_t filenameSize;
	u_int32_t flags;
	u_int32_t entryClusterId;
	u_int32_t fileSize;
	char seekFilename[50];
	char foundFilename[50];
	char* slashPos;
	int lookForDirectory = 0;
	int lookForFile = 0;


	// work out the filename we're looking for
	slashPos = strrchr0(filename, '/');
	if (slashPos == NULL) {
	// looking for file
		lookForFile = 1;

		// check seek filename size
		if (strlen(filename) > FATX_FILENAME_MAX) {
#ifdef FATX_INFO
			printk("Bad filename supplied (one leafname is too big)\n");
#endif
			return false;
		}

		// copy the filename to look for
		strcpy(seekFilename, filename);
	} else {
		// looking for directory
		lookForDirectory = 1;

		// check seek filename size
		if ((slashPos - filename) > FATX_FILENAME_MAX) {
#ifdef FATX_INFO
			printk("Bad filename supplied (one leafname is too big)\n");
#endif
			return false;
		}

		// copy the filename to look for
		memcpy(seekFilename, filename, slashPos - filename);
		seekFilename[slashPos - filename] = 0;
	}

#ifdef FATX_DEBUG
	VIDEO_ATTR=0xffc8c8c8;
	printk("_FATXFindFile : %s\n",filename);
#endif
	// OK, search through directory entries
	endOfDirectory = 0;
	while(clusterId != -1) {
    		// load cluster data
    		LoadFATXCluster(partition, clusterId, clusterData);

		// loop through it, outputing entries
		for(i=0; i< partition->clusterSize / FATX_DIRECTORYENTRY_SIZE; i++) {
			// work out the currentEntry
			curEntry = clusterData + (i * FATX_DIRECTORYENTRY_SIZE);

			// first of all, check that it isn't an end of directory marker
			if (checkForLastDirectoryEntry(curEntry)) {
				endOfDirectory = 1;
				break;
			}

			// get the filename size
			filenameSize = curEntry[0];

			// check if file is deleted
			if (filenameSize == 0xE5) {
				continue;
			}

			// check size is OK
			if ((filenameSize < 1) || (filenameSize > FATX_FILENAME_MAX)) {
#ifdef FATX_INFO
				printk("Invalid filename size: %i\n", filenameSize);
#endif
				return false;
			}

			// extract the filename
			memset(foundFilename, 0, 50);
			memcpy(foundFilename, curEntry+2, filenameSize);
			foundFilename[filenameSize] = 0;

			// get rest of data
			flags = curEntry[1];
			entryClusterId = *((u_int32_t*) (curEntry + 0x2c));
			fileSize = *((u_int32_t*) (curEntry + 0x30));

			// is it what we're looking for...
			if (strlen(seekFilename)==strlen(foundFilename) && strncmp(foundFilename, seekFilename,strlen(seekFilename)) == 0) {
				// if we're looking for a directory and found a directory
				if (lookForDirectory) {
					if (flags & FATX_FILEATTR_DIRECTORY) {
						return _FATXFindFile(partition, slashPos+1, entryClusterId,fileinfo);
					} else {
#ifdef FATX_INFO
						printk("File not found\n");
#endif
						return false;
					}
				}

				// if we're looking for a file and found a file
				if (lookForFile) {
					if (!(flags & FATX_FILEATTR_DIRECTORY)) {
						fileinfo->clusterId = entryClusterId;
						fileinfo->fileSize = fileSize;
						memset(fileinfo->filename,0,sizeof(fileinfo->filename));
						strcpy(fileinfo->filename,filename);
						return true;
					} else {
#ifdef FATX_INFO
						printk("File not found %s\n",filename);
#endif
						return false;
					}
				}
			}
		}

		// have we hit the end of the directory yet?
		if (endOfDirectory) {
			break;
		}

		// Find next cluster
		clusterId = getNextClusterInChain(partition, clusterId);
	}

	// not found it!
#ifdef FATX_INFO
	printk("File not found\n");
#endif
	return false;
}



u_int32_t getNextClusterInChain(FATXPartition* partition, int clusterId) {
	int nextClusterId = 0;
	u_int32_t eocMarker = 0;
	u_int32_t rootFatMarker = 0;
	u_int32_t maxCluster = 0;

	// check
	if (clusterId < 1) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Attempt to access invalid cluster: %i\n", clusterId);
	}

	// get the next ID
	if (partition->chainMapEntrySize == 2) {
		nextClusterId = partition->clusterChainMap.words[clusterId];
	        eocMarker = 0xffff;
		rootFatMarker = 0xfff8;
		maxCluster = 0xfff4;
	} else if (partition->chainMapEntrySize == 4) {
		nextClusterId = partition->clusterChainMap.dwords[clusterId];
		eocMarker = 0xffffffff;
		rootFatMarker = 0xfffffff8;
		maxCluster = 0xfffffff4;
	} else {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Unknown cluster chain map entry size: %i\n", partition->chainMapEntrySize);
	}

	// is it the end of chain?
  	if ((nextClusterId == eocMarker) || (nextClusterId == rootFatMarker)) {
		return -1;
	}
	
	// is it something else unknown?
	if (nextClusterId == 0) {
		VIDEO_ATTR=0xffe8e8e8;
		printk("getNextClusterInChain : Cluster chain problem: Next cluster after %i is unallocated!\n", clusterId);
        }
	if (nextClusterId > maxCluster) {
		printk("getNextClusterInChain : Cluster chain problem: Next cluster after %i has invalid value: %i\n", clusterId, nextClusterId);
	}
	
	// OK!
	return nextClusterId;
}

void LoadFATXCluster(FATXPartition* partition, int clusterId, unsigned char* clusterData) {
	u_int64_t clusterAddress;
	u_int64_t readSize;
	
	// work out the address of the cluster
	clusterAddress = partition->cluster1Address + ((unsigned long long)(clusterId - 1) * partition->clusterSize);

	// Now, load it
	readSize = FATXRawRead(partition->nDriveIndex, partition->partitionStart,
			clusterAddress, partition->clusterSize, clusterData);

        if (readSize != partition->clusterSize) {
		printk("LoadFATXCluster : Out of data while reading cluster %i\n", clusterId);
	}
}
	    

int FATXRawRead(int drive, int sector, unsigned long long byte_offset, int byte_len, char *buf) {

	int byte_read;
	
	byte_read = 0;

//	printk("rawread: sector=0x%X, byte_offset=0x%X, len=%d\n", sector, byte_offset, byte_len);

        sector+=byte_offset/512;
        byte_offset%=512;

        while(byte_len) {
		int nThisTime=512;
		if(byte_len<512) nThisTime=byte_len;
                if(byte_offset) {
	                BYTE ba[512];
			if(BootIdeReadSector(drive, buf, sector, 0, 512)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("Unable to get first sector\n");
                                return false;
			}
			memcpy(buf, &ba[byte_offset], nThisTime-byte_offset);
			buf+=nThisTime-byte_offset;
			byte_len-=nThisTime-byte_offset;
			byte_read += nThisTime-byte_offset;
			byte_offset=0;
		} else {
			if(BootIdeReadSector(drive, buf, sector, 0, nThisTime)) {
				VIDEO_ATTR=0xffe8e8e8;
				printk("Unable to get first sector\n");
				return false;
			}
			buf+=nThisTime;
			byte_len-=nThisTime;
			byte_read += nThisTime;
		}
		sector++;
	}
	return byte_read;
}

void CloseFATXPartition(FATXPartition* partition) {
	if(partition != NULL) {
		free(partition->clusterChainMap.words);
		free(partition);
		partition = NULL;
	}
}
