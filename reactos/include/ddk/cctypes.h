#ifndef __INCLUDE_DDK_CCTYPES_H
#define __INCLUDE_DDK_CCTYPES_H

typedef struct _CCB
{
   ULONG BlockNr;
   PVOID Buffer;
   ULONG State;
   ULONG ActiveReaders;
   BOOLEAN WriteInProgress;
   BOOLEAN ActiveWriter;   
   ULONG References;   
   KEVENT FinishedNotify;
   KSPIN_LOCK Lock;
   BOOLEAN Modified;
   LIST_ENTRY Entry;
} CCB, *PCCB;

enum
{
   CCB_INVALID,
   CCB_NOT_CACHED,
   CCB_CACHED,
   CCB_DELETE_PENDING,
};

typedef struct _DCCB
/*
 * PURPOSE: Device cache control block
 */
{
   PCCB* HashTbl;
   ULONG HashTblSize;
   KSPIN_LOCK HashTblLock;
   PDEVICE_OBJECT DeviceObject;
   ULONG SectorSize;
   LIST_ENTRY CcbListHead;
   KSPIN_LOCK CcbListLock;
   ULONG NrCcbs;
   ULONG NrModifiedCcbs;
} DCCB, *PDCCB;

#endif /* __INCLUDE_DDK_CCTYPES_H */
