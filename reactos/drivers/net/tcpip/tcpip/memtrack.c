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

VOID ShowTrackedThing( PCHAR What, PALLOCATION_TRACKER Thing ) {
    /* if( ShowTag( Thing->Tag ) ) */
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

VOID TrackWithTag( DWORD Tag, PVOID Thing, PCHAR FileName, DWORD LineNo ) {
    PALLOCATION_TRACKER TrackedThing = 
	ExAllocatePool( NonPagedPool, sizeof(*TrackedThing) );

    if( TrackedThing ) {
	TrackedThing->Tag      = Tag;
	TrackedThing->Thing    = Thing;
	TrackedThing->FileName = FileName;
	TrackedThing->LineNo   = LineNo;
	
	ExInterlockedInsertTailList( &AllocatedObjectsList, 
				     &TrackedThing->Entry,
				     &AllocatedObjectsLock );
	ShowTrackedThing( "Alloc", TrackedThing );
    }
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

    DbgPrint("Untrack: %s:%d\n", File, Line);

    KeAcquireSpinLock( &AllocatedObjectsLock, &OldIrql );
    Entry = AllocatedObjectsList.Flink;
    while( Entry != &AllocatedObjectsList ) {
	ThingInList = CONTAINING_RECORD(Entry, ALLOCATION_TRACKER, Entry);
	if( ThingInList->Thing == Thing ) {
	    RemoveEntryList(Entry);
	    
	    ShowTrackedThing( "Free ", ThingInList );
	    
	    ExFreePool( ThingInList );
	    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
	    return;
	}
	Entry = Entry->Flink;
    }
    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
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
	ShowTrackedThing( "Dump ", Thing );
	Entry = Entry->Flink;
    }
    KeReleaseSpinLock( &AllocatedObjectsLock, OldIrql );
}

#endif/*MEMTRACK*/
