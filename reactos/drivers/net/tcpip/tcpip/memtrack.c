#define MEMTRACK_NO_POOL
#include <roscfg.h>
#include <tcpip.h>
#include <ntddk.h>
#include <memtrack.h>

#ifdef MEMTRACK
LIST_ENTRY AllocatedObjectsList;
KSPIN_LOCK AllocatedObjectsLock;
DWORD TagsToShow[MEMTRACK_MAX_TAGS_TO_TRACK] = { 0 };

VOID TrackTag( DWORD Tag ) {
    UINT i;

    for( i = 0; TagsToShow[i]; i++ );
    TagsToShow[i] = Tag;
}

VOID TrackingInit() {
    KeInitializeSpinLock( &AllocatedObjectsLock );
    InitializeListHead( &AllocatedObjectsList );
}

VOID ShowTrackedThing( PCHAR What, PALLOCATION_TRACKER Thing, 
		       PCHAR File, UINT Line ) {
    /* if( ShowTag( Thing->Tag ) ) */
    if( File ) {
	DbgPrint( "[%s] Thing %08x %c%c%c%c (%s:%d) (Called from %s:%d)\n", 
		  What,
		  Thing->Thing,
		  ((PCHAR)&Thing->Tag)[3],
		  ((PCHAR)&Thing->Tag)[2],
		  ((PCHAR)&Thing->Tag)[1],
		  ((PCHAR)&Thing->Tag)[0],
		  Thing->FileName,
		  Thing->LineNo,
		  File, Line );
    } else {
	DbgPrint( "[%s] Thing %08x %c%c%c%c (%s:%d)\n", 
		  What,
		  Thing->Thing,
		  ((PCHAR)&Thing->Tag)[3],
		  ((PCHAR)&Thing->Tag)[2],
		  ((PCHAR)&Thing->Tag)[1],
		  ((PCHAR)&Thing->Tag)[0],
		  Thing->FileName,
		  Thing->LineNo );
    }
}

VOID TrackWithTag( DWORD Tag, PVOID Thing, PCHAR FileName, DWORD LineNo ) {
    PALLOCATION_TRACKER TrackedThing = 
	ExAllocatePool( NonPagedPool, sizeof(*TrackedThing) );

    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER ThingInList;

    KeAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);
	    
	    ShowTrackedThing( "Alloc", ThingInList, FileName, LineNo );
	    
	    ExFreePool( ThingInList );
	    TrackDumpFL( FileName, LineNo );
	    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
	    DbgPrint("TRACK: SPECIFIED ALREADY ALLOCATED ITEM %x\n", Thing);
	    KeBugCheck( 0 );
	}
	Entry = Entry->Flink;
    }

    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );

    if( TrackedThing ) {
	TrackedThing->Tag      = Tag;
	TrackedThing->Thing    = Thing;
	TrackedThing->FileName = FileName;
	TrackedThing->LineNo   = LineNo;
	
	ExInterlockedInsertTailList( &AllocatedObjectsList, 
				     &TrackedThing->Entry,
				     &AllocatedObjectsLock );
	ShowTrackedThing( "Alloc", TrackedThing, FileName, LineNo );
    }

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

    KeAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);
	    
	    ShowTrackedThing( "Free ", ThingInList, File, Line );
	    
	    ExFreePool( ThingInList );
	    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
	    /* TrackDumpFL( File, Line ); */
	    return;
	}
	Entry = Entry->Flink;
    }
    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
    TrackDumpFL( File, Line );
    DbgPrint("UNTRACK: SPECIFIED ALREADY FREE ITEM %x\n", Thing);
    KeBugCheck( 0 );
}

VOID TrackDumpFL( PCHAR File, DWORD Line ) {
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PALLOCATION_TRACKER Thing;

    DbgPrint("Dump: %s:%d\n", File, Line);

    KeAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	Thing = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	ShowTrackedThing( "Dump ", Thing, 0, 0 );
	Entry = Entry->Flink;
    }
    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
}

#endif/*MEMTRACK*/
