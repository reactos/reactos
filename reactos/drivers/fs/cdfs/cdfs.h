#ifndef CDFS_H
#define CDFS_H

#include <ddk/ntifs.h>

#define CDFS_BASIC_SECTOR 2048
#define CDFS_PRIMARY_DESCRIPTOR_LOCATION 16
#define BLOCKSIZE CDFS_BASIC_SECTOR
#define CDFS_MAX_NAME_LEN 256

struct _DIR_RECORD
{
  UCHAR  RecordLength;			// 1
  UCHAR  ExtAttrRecordLength;		// 2
  ULONG  ExtentLocationL;		// 3-6
  ULONG  ExtentLocationM;		// 7-10
  ULONG  DataLengthL;			// 11-14
  ULONG  DataLengthM;			// 15-18
  UCHAR  Year;				// 19
  UCHAR  Month;				// 20
  UCHAR  Day;				// 21
  UCHAR  Hour;				// 22
  UCHAR  Minute;			// 23
  UCHAR  Second;			// 24
  UCHAR  TimeZone;			// 25
  UCHAR  FileFlags;			// 26
  UCHAR  FileUnitSize;			// 27
  UCHAR  InterleaveGapSize;		// 28
  ULONG  VolumeSequenceNumber;		// 29-32
  UCHAR  FileIdLength;			// 33
  UCHAR  FileId[1];			// 34
} __attribute__((packed));

typedef struct _DIR_RECORD DIR_RECORD, PDIR_RECORD;


/* Primary Volume Descriptor */
struct _PVD
{
  unsigned char  VdType;		// 1
  unsigned char  StandardId[5];		// 2-6
  unsigned char  VdVersion;		// 7
  unsigned char  unused0;		// 8
  unsigned char  SystemId[32];		// 9-40
  unsigned char  VolumeId[32];		// 41-72
  unsigned char  unused1[8];		// 73-80
  unsigned long  VolumeSpaceSizeL;	// 81-84
  unsigned long  VolumeSpaceSizeM;	// 85-88
  unsigned char  unused2[32];		// 89-120
  unsigned long  VolumeSetSize;		// 121-124
  unsigned long  VolumeSequenceNumber;	// 125-128
  unsigned long  LogicalBlockSize;	// 129-132
  unsigned long  PathTableSizeL;	// 133-136
  unsigned long  PathTableSizeM;	// 137-140
  ULONG LPathTablePos;			// 141-144
  ULONG LOptPathTablePos;		// 145-148
  ULONG MPathTablePos;			// 149-152
  ULONG MOptPathTablePos;		// 153-156
  DIR_RECORD RootDirRecord;		// 157-190

  /* more data ... */

} __attribute__((packed));

typedef struct _PVD PVD, *PPVD;



typedef struct _FCB
{
  REACTOS_COMMON_FCB_HEADER RFCB;
  SECTION_OBJECT_POINTERS SectionObjectPointers;

  /* CDFS owned elements */

  struct _FCB *next;
  struct _FCB *parent;

  wchar_t name[CDFS_MAX_NAME_LEN];
  int hashval;
  unsigned int  extent_start;
  unsigned int  byte_count;
  unsigned int  file_pointer;
} FsdFcbEntry, FCB, *PFCB;


NTSTATUS
CdfsReadSectors(IN PDEVICE_OBJECT DeviceObject,
		IN ULONG DiskSector,
		IN ULONG SectorCount,
		IN OUT PUCHAR Buffer);

int CdfsStrcmpi( wchar_t *str1, wchar_t *str2 );
void CdfsWstrcpy( wchar_t *str1, wchar_t *str2, int max );


/* 
   CDFS: FCB system (Perhaps there should be a library to make this easier) 
*/

typedef struct _fcb_system
{
  int fcbs_in_use;
  int fcb_table_size;
  int fcb_table_mask;
  FsdFcbEntry **fcb_table;
  FsdFcbEntry *parent;
} fcb_system;


PFCB
FsdGetFcbEntry(fcb_system *fss,
	       PFCB ParentFcb,
	       PWSTR name);

#endif//CDFS_H
