/*
 *  partlist.h
 */


typedef struct _PARTDATA
{
  ULONGLONG DiskSize;
  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;
  ULONGLONG PartSize;
  ULONG PartNumber;
  ULONG PartType;
} PARTDATA, *PPARTDATA;


typedef struct _PARTENTRY
{
  ULONGLONG PartSize;
  ULONG PartNumber;
  ULONG PartType;
  BOOL Used;
} PARTENTRY, *PPARTENTRY;

typedef struct _DISKENTRY
{
  ULONGLONG DiskSize;
  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;
  BOOL FixedDisk;

  ULONG PartCount;
  PPARTENTRY PartArray;

} DISKENTRY, *PDISKENTRY;


typedef struct _PARTLIST
{
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;

  ULONG TopDisk;
  ULONG TopPartition;

  ULONG CurrentDisk;
  ULONG CurrentPartition;

  ULONG DiskCount;
  PDISKENTRY DiskArray;

} PARTLIST, *PPARTLIST;




PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom);

VOID
DestroyPartitionList(PPARTLIST List);

VOID
DrawPartitionList(PPARTLIST List);

VOID
ScrollDownPartitionList(PPARTLIST List);

VOID
ScrollUpPartitionList(PPARTLIST List);

BOOL
GetPartitionData(PPARTLIST List, PPARTDATA Data);

/* EOF */