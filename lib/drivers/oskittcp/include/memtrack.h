#ifndef MEMTRACK_H
#define MEMTRACK_H

#ifndef FOURCC
#define FOURCC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
#endif

#define FBSD_MALLOC FOURCC('d','s','b','f')
#define EXALLOC_TAG FOURCC('E','x','A','l')

#ifdef MEMTRACK
#define MTMARK() TrackDumpFL(__FILE__, __LINE__)
#define NdisAllocateBuffer(x,y,z,a,b) { \
    NdisAllocateBuffer(x,y,z,a,b); \
    if( *x == NDIS_STATUS_SUCCESS ) { \
        Track(NDIS_BUFFER_TAG, *y); \
    } \
}
#define NdisAllocatePacket(x,y,z) { \
    NdisAllocatePacket(x,y,z); \
    if( *x == NDIS_STATUS_SUCCESS ) { \
        Track(NDIS_PACKET_TAG, *y); \
    } \
}
#define FreeNdisPacket(x) { TI_DbgPrint(MID_TRACE,("Deleting Packet %x\n", x)); FreeNdisPacketX(x); }
#define NdisFreePacket(x) { Untrack(x); NdisFreePacket(x); }
#define NdisFreeBuffer(x) { Untrack(x); NdisFreeBuffer(x); }
#define exAllocatePool(x,y) ExAllocatePoolX(x,y,__FILE__,__LINE__)
#define exAllocatePoolWithTag(x,y,z) ExAllocatePoolX(x,y,__FILE__,__LINE__)
#define exFreePool(x) ExFreePoolX(x,__FILE__,__LINE__)

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

static __inline PVOID ExAllocatePoolX( POOL_TYPE type, SIZE_T size, PCHAR File, ULONG Line ) {
    PVOID Out = ExAllocatePool( type, size );
    if( Out ) TrackWithTag( EXALLOC_TAG, Out, File, Line );
    return Out;
}
static __inline VOID ExFreePoolX( PVOID Data, PCHAR File, ULONG Line ) {
    UntrackFL(File, Line, Data);
    ExFreePool(Data);
}

#define MEMTRACK_MAX_TAGS_TO_TRACK 64
#else
#define MTMARK()
#define Track(x,y)
#define TrackingInit()
#define TrackDump()
#define Untrack(x)
#define TrackTag(x)
#define FreeNdisPacket FreeNdisPacketX
#define exFreePool(x) ExFreePool(x)
#define exAllocatePool(x,y) ExAllocatePool(x,y)
#define exAllocatePoolWithTag(x,y,z) ExAllocatePoolWithTag(x,y,z)
#define TrackWithTag(a,b,c,d)
#define UntrackFL(a,b,c)
#endif

#endif/*MEMMTRAC_H*/
