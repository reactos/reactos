#define MEMTRACK_NO_POOL
#include "precomp.h"

#ifdef MEMTRACK

#define TRACK_TAG TAG('T','r','C','K')

static LIST_ENTRY AllocatedObjectsList;
static KSPIN_LOCK AllocatedObjectsLock;
static NPAGED_LOOKASIDE_LIST AllocatedObjectsLookasideList;
DWORD TagsToShow[MEMTRACK_MAX_TAGS_TO_TRACK] = { 0 };

VOID TrackTag( DWORD Tag ) {
    UINT i;

    for( i = 0; TagsToShow[i]; i++ );
    TagsToShow[i] = Tag;
}

VOID TrackingInit() {
    TcpipInitializeSpinLock( &AllocatedObjectsLock );
    InitializeListHead( &AllocatedObjectsList );
    ExInitializeNPagedLookasideList(&AllocatedObjectsLookasideList,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(ALLOCATION_TRACKER),
                                   TRACK_TAG,
                                   0 );
}

VOID ShowTrackedThing( PCHAR What, PALLOCATION_TRACKER Thing,
		       PCHAR File, UINT Line ) {
    /* if( ShowTag( Thing->Tag ) ) */
    if( File ) {
	TI_DbgPrint(MAX_TRACE,
		    ("[%s] Thing %08x %c%c%c%c (%s:%d) (Called from %s:%d)\n",
		     What,
		     Thing->Thing,
		     ((PCHAR)&Thing->Tag)[3],
		     ((PCHAR)&Thing->Tag)[2],
		     ((PCHAR)&Thing->Tag)[1],
		     ((PCHAR)&Thing->Tag)[0],
		     Thing->FileName,
		     Thing->LineNo,
		     File, Line));
    } else {
	TI_DbgPrint(MAX_TRACE,
		    ( "[%s] Thing %08x %c%c%c%c (%s:%d)\n",
		      What,
		      Thing->Thing,
		      ((PCHAR)&Thing->Tag)[3],
		      ((PCHAR)&Thing->Tag)[2],
		      ((PCHAR)&Thing->Tag)[1],
		      ((PCHAR)&Thing->Tag)[0],
		      Thing->FileName,
		      Thing->LineNo ));
    }
}

VOID TrackWithTag( DWORD Tag, PVOID Thing, PCHAR FileName, DWORD LineNo ) {
    PALLOCATION_TRACKER TrackedThing =
        ExAllocateFromNPagedLookasideList( &AllocatedObjectsLookasideList );

    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER ThingInList;

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);

	    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
	    ShowTrackedThing( "Alloc", ThingInList, FileName, LineNo );
	    PoolFreeBuffer( ThingInList );
	    TrackDumpFL( FileName, LineNo );
	    DbgPrint("TRACK: SPECIFIED ALREADY ALLOCATED ITEM %x\n", Thing);
	    TcpipBugCheck( 0 );
	}
	Entry = Entry->Flink;
    }

    if( TrackedThing ) {
	TrackedThing->Tag      = Tag;
	TrackedThing->Thing    = Thing;
	TrackedThing->FileName = FileName;
	TrackedThing->LineNo   = LineNo;


	InsertHeadList( &AllocatedObjectsList, &TrackedThing->Entry );
	ShowTrackedThing( "Alloc", TrackedThing, FileName, LineNo );
    }

    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );

    /*TrackDumpFL( FileName, LineNo );*/
}

BOOL ShowTag( DWORD Tag ) {
    UINT i;

    for( i = 0; TagsToShow[i] && TagsToShow[i] != Tag; i++ );

    return TagsToShow[i] ? TRUE : FALSE;
}

VOID UntrackFL( PCHAR File, DWORD Line, PVOID Thing ) {
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER ThingInList;

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);

	    ShowTrackedThing( "Free ", ThingInList, File, Line );

	    ExFreeToNPagedLookasideList( &AllocatedObjectsLookasideList,
	                                ThingInList );
 
	    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
	    /* TrackDumpFL( File, Line ); */
	    return;
	}
	Entry = Entry->Flink;
    }
    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
    TrackDumpFL( File, Line );
    DbgPrint("UNTRACK: SPECIFIED ALREADY FREE ITEM %x\n", Thing);
    TcpipBugCheck( 0 );
}

VOID TrackDumpFL( PCHAR File, DWORD Line ) {
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER Thing;

    TI_DbgPrint(MAX_TRACE,("Dump: %s:%d\n", File, Line));

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	Thing = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	ShowTrackedThing( "Dump ", Thing, 0, 0 );
	Entry = Entry->Flink;
    }
    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
}

#endif/*MEMTRACK*/
