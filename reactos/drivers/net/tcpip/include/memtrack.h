#ifndef MEMTRACK_H
#define MEMTRACK_H

#ifdef MEMTRACK
#define MTMARK() \
      { PNDIS_BUFFER ArtyFake; \
        NDIS_STATUS ArtyStatus; \
        DbgPrint("Buffer Tracking Mark %s:%d\n", __FILE__, __LINE__); \
	NdisAllocateBuffer(&ArtyStatus, \
			   &ArtyFake, \
			   GlobalBufferPool, \
			   0, \
			   0); }

extern LIST_ENTRY AllocatedObjectsHead;
extern KSPIN_LOCK AllocatedObjectsLock;

typedef struct _ALLOCATION_TRACKER {
    LIST_ENTRY Entry;
    DWORD Tag;
    PVOID Thing;
    PCHAR FileName;
    DWORD LineNo;
} ALLOCATION_TRACKER, *PALLOCATION_TRACKER;

VOID TrackingInit();
VOID TrackWithTag( DWORD Tag, PVOID Thing, PCHAR File, DWORD Line );
#define Track(Tag,Thing) TrackWithTag(Tag,Thing,__FILE__,__LINE__)
VOID UntrackFL( PCHAR File, DWORD Line, PVOID Thing );
#define Untrack(Thing) UntrackFL(__FILE__,__LINE__,Thing)
VOID TrackDumpFL( PCHAR File, DWORD Line );
#define TrackDump() TrackDumpFL(__FILE__,__LINE__)
VOID TrackTag( DWORD Tag );

#define MEMTRACK_MAX_TAGS_TO_TRACK 64
#else
#define MTMARK()
#define Track(x,y)
#define TrackingInit()
#define TrackDump()
#define Untrack(x)
#define TrackTag(x)
#endif

#endif/*MEMMTRAC_H*/
