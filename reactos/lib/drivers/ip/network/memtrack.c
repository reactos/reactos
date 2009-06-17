#define MEMTRACK_NO_POOL
#include "precomp.h"


#if DBG
#define TRACK_TAG TAG('T','r','C','K')

static LIST_ENTRY AllocatedObjectsList;
static KSPIN_LOCK AllocatedObjectsLock;
static NPAGED_LOOKASIDE_LIST AllocatedObjectsLookasideList;
ULONG TagsToShow[MEMTRACK_MAX_TAGS_TO_TRACK] = { 0 };

VOID TrackTag( ULONG Tag ) {
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

VOID ShowTrackedThing( PCHAR What, PALLOCATION_TRACKER Thing, BOOLEAN ForceShow ) {

    if (ForceShow)
    {
        DbgPrint("[%s] Thing %08x %c%c%c%c (%s:%d)\n",
		  What,
		  Thing->Thing,
		  ((PCHAR)&Thing->Tag)[3],
		  ((PCHAR)&Thing->Tag)[2],
		  ((PCHAR)&Thing->Tag)[1],
		  ((PCHAR)&Thing->Tag)[0],
		  Thing->FileName,
		  Thing->LineNo);
    }
    else
    {
	TI_DbgPrint(MAX_TRACE,
		    ("[%s] Thing %08x %c%c%c%c (%s:%d)\n",
		     What,
		     Thing->Thing,
		     ((PCHAR)&Thing->Tag)[3],
		     ((PCHAR)&Thing->Tag)[2],
		     ((PCHAR)&Thing->Tag)[1],
		     ((PCHAR)&Thing->Tag)[0],
		     Thing->FileName,
		     Thing->LineNo));
    }
}

VOID TrackWithTag( ULONG Tag, PVOID Thing, PCHAR FileName, ULONG LineNo ) {
    PALLOCATION_TRACKER TrackedThing =
        ExAllocateFromNPagedLookasideList( &AllocatedObjectsLookasideList );

    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER ThingInList;

    if (!TrackedThing) return;

    TrackedThing->Tag      = Tag;
    TrackedThing->Thing    = Thing;
    TrackedThing->FileName = FileName;
    TrackedThing->LineNo   = LineNo;

    ShowTrackedThing( "Alloc", TrackedThing, FALSE );

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);

            TI_DbgPrint(MAX_TRACE,("TRACK: SPECIFIED ALREADY ALLOCATED ITEM %x\n", Thing));
            ShowTrackedThing( "Double Alloc (Item in list)", ThingInList, FALSE );
            ShowTrackedThing( "Double Alloc (Item not in list)", TrackedThing, FALSE );

            ExFreeToNPagedLookasideList( &AllocatedObjectsLookasideList,
	                                 ThingInList );

            break;
	}
	Entry = Entry->Flink;
    }

    InsertHeadList( &AllocatedObjectsList, &TrackedThing->Entry );

    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
}

BOOLEAN ShowTag( ULONG Tag ) {
    UINT i;

    for( i = 0; TagsToShow[i] && TagsToShow[i] != Tag; i++ );

    return TagsToShow[i] ? TRUE : FALSE;
}

VOID UntrackFL( PCHAR File, ULONG Line, PVOID Thing, ULONG Tag ) {
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER ThingInList;

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);

	    ShowTrackedThing( "Free ", ThingInList, FALSE );

            if ( ThingInList->Tag != Tag ) {
                 DbgPrint("UNTRACK: TAG DOES NOT MATCH (%x)\n", Thing);
                 ShowTrackedThing("Tag Mismatch (Item in list)", ThingInList, TRUE);
                 ASSERT( FALSE );
            }

	    ExFreeToNPagedLookasideList( &AllocatedObjectsLookasideList,
	                                ThingInList );

	    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );

	    /* TrackDumpFL( File, Line ); */
	    return;
	}
	Entry = Entry->Flink;
    }
    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
    DbgPrint("UNTRACK: SPECIFIED ALREADY FREE ITEM %x\n", Thing);
    ASSERT( FALSE );
}

VOID TrackDumpFL( PCHAR File, ULONG Line ) {
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER Thing;

    TI_DbgPrint(MAX_TRACE,("Dump: %s:%d\n", File, Line));

    TcpipAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	Thing = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	ShowTrackedThing( "Dump ", Thing, FALSE );
	Entry = Entry->Flink;
    }
    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
}

#endif /* DBG */
