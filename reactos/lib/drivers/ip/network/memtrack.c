#define MEMTRACK_NO_POOL
#include "precomp.h"


#if DBG
#define TRACK_TAG TAG('T','r','C','K')

static LIST_ENTRY AllocatedObjectsList;
static KSPIN_LOCK AllocatedObjectsLock;
static NPAGED_LOOKASIDE_LIST AllocatedObjectsLookasideList;

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

            ShowTrackedThing( "Double Alloc (Item in list)", ThingInList, TRUE );
            ShowTrackedThing( "Double Alloc (Item not in list)", TrackedThing, TRUE );

            ExFreeToNPagedLookasideList( &AllocatedObjectsLookasideList,
	                                 ThingInList );

            break;
	}
	Entry = Entry->Flink;
    }

    InsertHeadList( &AllocatedObjectsList, &TrackedThing->Entry );

    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
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

	    return;
	}
	Entry = Entry->Flink;
    }
    TcpipReleaseSpinLock( &AllocatedObjectsLock, OldIrql );

    DbgPrint("[Double Free] Thing %08x %c%c%c%c (%s:%d)\n",
             Thing,
             ((PCHAR)&Tag)[3],
             ((PCHAR)&Tag)[2],
             ((PCHAR)&Tag)[1],
             ((PCHAR)&Tag)[0],
             File,
             Line);

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
