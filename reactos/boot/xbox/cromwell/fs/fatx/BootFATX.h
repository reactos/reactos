#ifndef _BootFATX_H_
#define _BootFATX_H_

#include <sys/types.h>

// Definitions for FATX on-disk structures
// (c) 2001 Andrew de Quincey


#define STORE_SIZE	(0x131F00000LL)
#define SYSTEM_SIZE	(0x1f400000)
#define	CACHE1_SIZE	(0x2ee80000)
#define	CACHE2_SIZE	(0x2ee80000)
#define	CACHE3_SIZE	(0x2ee80000)

#define SECTOR_STORE	(0x0055F400L)
#define SECTOR_SYSTEM	(0x00465400L)
#define SECTOR_CACHE1	(0x00000400L)
#define SECTOR_CACHE2	(0x00177400L)
#define SECTOR_CACHE3	(0x002EE400L)

#define SECTORS_STORE	(SECTOR_EXTEND - SECTOR_STORE)
#define SECTORS_SYSTEM	(SECTOR_STORE  - SECTOR_SYSTEM)
#define SECTORS_CACHE1	(SECTOR_CACHE2 - SECTOR_CACHE1)
#define SECTORS_CACHE2	(SECTOR_CACHE3 - SECTOR_CACHE2)
#define SECTORS_CACHE3	(SECTOR_SYSTEM - SECTOR_CACHE3)


// Size of FATX partition header
#define FATX_PARTITION_HEADERSIZE 0x1000

// FATX partition magic
#define FATX_PARTITION_MAGIC 0x58544146

// FATX chain table block size
#define FATX_CHAINTABLE_BLOCKSIZE 4096

// ID of the root FAT cluster
#define FATX_ROOT_FAT_CLUSTER 1

// Size of FATX directory entries
#define FATX_DIRECTORYENTRY_SIZE 0x40

// File attribute: read only
#define FATX_FILEATTR_READONLY 0x01

// File attribute: hidden
#define FATX_FILEATTR_HIDDEN 0x02

// File attribute: system 
#define FATX_FILEATTR_SYSTEM 0x04

// File attribute: archive
#define FATX_FILEATTR_ARCHIVE 0x20

// Directory entry flag indicating entry is a sub-directory
#define FATX_FILEATTR_DIRECTORY 0x10

// max filename size
#define FATX_FILENAME_MAX 42

// This structure describes a FATX partition
typedef struct {

  int nDriveIndex;
 
  // The starting byte of the partition
  u_int64_t partitionStart;

  // The size of the partition in bytes
  u_int64_t partitionSize;

  // The cluster size of the partition
  u_int32_t clusterSize;

  // Number of clusters in the partition
  u_int32_t clusterCount;

  // Size of entries in the cluster chain map
  u_int32_t chainMapEntrySize;

  // The cluster chain map table (which may be in words OR dwords)
  union {
    u_int16_t *words;
    u_int32_t *dwords;
  } clusterChainMap;
  
  // Address of cluster 1
  u_int64_t cluster1Address;
  
} FATXPartition;

typedef struct {
	char filename[FATX_FILENAME_MAX];
	int clusterId;
	u_int32_t fileSize;
	u_int32_t fileRead;
	BYTE *buffer;
} FATXFILEINFO;

int LoadFATXFilefixed(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo,BYTE* Position);
int LoadFATXFile(FATXPartition *partition,char *filename, FATXFILEINFO *fileinfo);
void PrintFAXPartitionTable(int nDriveIndex);
int FATXSignature(int nDriveIndex,unsigned int block,BYTE *ba);
FATXPartition *OpenFATXPartition(int nDriveIndex,unsigned int partitionOffset,
		                u_int64_t partitionSize);
int FATXRawRead (int drive, int sector, unsigned long long byte_offset, int byte_len, char *buf);
void DumpFATXTree(FATXPartition *partition);
void _DumpFATXTree(FATXPartition* partition, int clusterId, int nesting);
void LoadFATXCluster(FATXPartition* partition, int clusterId, unsigned char* clusterData);
u_int32_t getNextClusterInChain(FATXPartition* partition, int clusterId);
void CloseFATXPartition(FATXPartition* partition);
int FATXFindFile(FATXPartition* partition,char* filename,int clusterId, FATXFILEINFO *fileinfo);
int _FATXFindFile(FATXPartition* partition,char* filename,int clusterId, FATXFILEINFO *fileinfo);
int FATXLoadFromDisk(FATXPartition* partition, FATXFILEINFO *fileinfo);

#endif //	_BootFATX_H_
