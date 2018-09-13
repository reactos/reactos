/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    range.h

Abstract:

    Kernel-mode range list support for arbiters

Author:

    Andy Thornton (andrewth) 02/17/97

Revision History:

--*/

#ifndef _RANGE_
#define _RANGE_
            
//
// Debugging options
//

#if DBG && !defined(NTOS_KERNEL_RUNTIME)
    #include <stdio.h>
#endif

#undef MAX_ULONGLONG
#define MAX_ULONGLONG   ((ULONGLONG)-1)

#define RTL_RANGE_LIST_ENTRY_TAG    'elRR'
#define RTL_RANGE_LIST_MISC_TAG     'mlRR'

#if DBG
    #define DEBUG_PRINT(Level, Message) \
        if (Level <= RtlRangeDebugLevel) DbgPrint Message
#else
    #define DEBUG_PRINT(Level, Message) 
#endif // DBG
    
//
// Range list structures
//

#define RTLP_RANGE_LIST_ENTRY_MERGED         0x0001

typedef struct _RTLP_RANGE_LIST_ENTRY {

    //
    // Common data
    //
    ULONGLONG Start;
    ULONGLONG End;
  
    union {
        
        //
        // An Allocated range
        //
        struct {
            
            //
            // Data from the user given in AddRange
            //
            PVOID UserData;
            
            //
            // The owner of the range
            //
            PVOID Owner;
        
        } Allocated;

        //
        // A Merged range
        //
        struct {
            
            //
            // List of ranges that overlap between Start and End
            //
            LIST_ENTRY ListHead;
        
        } Merged;
   
    };

    //
    // User defined flags given in AddRange
    //
    UCHAR Attributes;
    
    //
    // Range descriptors
    //
    UCHAR PublicFlags;          // use RANGE_*
    
    //
    // Control information
    //
    USHORT PrivateFlags;        // use RANGE_LIST_ENTRY_*

    //
    // Main linked list entry
    //
    LIST_ENTRY ListEntry;    

} RTLP_RANGE_LIST_ENTRY, *PRTLP_RANGE_LIST_ENTRY;


//
// Useful macros for dealing with range list entries
//

#define MERGED(Entry)   (BOOLEAN)((Entry)->PrivateFlags & RTLP_RANGE_LIST_ENTRY_MERGED)
#define SHARED(Entry)   (BOOLEAN)((Entry)->PublicFlags & RTL_RANGE_SHARED)
#define CONFLICT(Entry) (BOOLEAN)((Entry)->PublicFlags & RTL_RANGE_CONFLICT)

//
// List Traversing Macros
//

#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

#define FOR_ALL_IN_LIST_SAFE(Type, Head, Current, Next)                 \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry),  \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,      \
                                       Type, ListEntry);                \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,      \
                                     Type, ListEntry)                   \
       )


#define FOR_REST_IN_LIST(Type, Head, Current)                           \
    for(;                                                               \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

#define FOR_REST_IN_LIST_SAFE(Type, Head, Current, Next)                \
    for((Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,          \
                                       Type, ListEntry);                \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Flink,      \
                                     Type, ListEntry)                   \
       )

//
// Backwards List Traversing Macros
//

#define FOR_ALL_IN_LIST_BACKWARDS(Type, Head, Current)                  \
    for((Current) = CONTAINING_RECORD((Head)->Blink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Blink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

#define FOR_ALL_IN_LIST_SAFE_BACKWARDS(Type, Head, Current, Next)       \
    for((Current) = CONTAINING_RECORD((Head)->Blink, Type, ListEntry),  \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Blink,      \
                                       Type, ListEntry);                \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Blink,      \
                                     Type, ListEntry)                   \
       )


#define FOR_REST_IN_LIST_BACKWARDS(Type, Head, Current)                 \
    for(;                                                               \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Blink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )

#define FOR_REST_IN_LIST_SAFE_BACKWARDS(Type, Head, Current, Next)      \
    for((Next) = CONTAINING_RECORD((Current)->ListEntry.Blink,          \
                                       Type, ListEntry);                \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = (Next),                                              \
            (Next) = CONTAINING_RECORD((Current)->ListEntry.Blink,      \
                                     Type, ListEntry)                   \
       )

//
// Misc Macros
//

#define LAST_IN_LIST(ListHead, Entry)                                   \
    ( (Entry)->ListEntry.Flink == ListHead )

#define FIRST_IN_LIST(ListHead, Entry)                                  \
    ( (Entry)->ListEntry.Blink == ListHead )


#define RANGE_DISJOINT(a,b)                                             \
    ( ((a)->Start < (b)->Start && (a)->End < (b)->Start)                \
    ||((b)->Start < (a)->Start && (b)->End < (a)->Start) )

#define RANGE_INTERSECT(a,b)                                            \
    !RANGE_DISJOINT((a),(b))

#define RANGE_LIMITS_DISJOINT(s1,e1,s2,e2)                              \
    ( ((s1) < (s2) && (e1) < (s2))                                      \
    ||((s2) < (s1) && (e2) < (s1)) )

#define RANGE_LIMITS_INTERSECT(s1,e1,s2,e2)                             \
    !RANGE_LIMITS_DISJOINT((s1),(e1),(s2),(e2))

#define RANGE_LIST_ENTRY_FROM_LIST_ENTRY(Entry)                         \
    CONTAINING_RECORD((Entry), RTLP_RANGE_LIST_ENTRY, ListEntry)    

#define RANGE_LIST_FROM_LIST_HEAD(Head)                                 \
    CONTAINING_RECORD((Head), RTL_RANGE_LIST, ListHead)    

#define FOR_REST_OF_RANGES(_Iterator, _Current, _Forward)               \
    for ((_Current) = (PRTL_RANGE)(_Iterator)->Current;                 \
         (_Current) != NULL;                                            \
         RtlGetNextRange((_Iterator), &(_Current), (_Forward))          \
         )

//
//  VOID
//  InsertEntryList(
//      PLIST_ENTRY Previous,
//      PLIST_ENTRY Entry
//      );
//

#define InsertEntryList(Previous, Entry) {                              \
    PLIST_ENTRY _EX_Next = (Previous)->Flink;                           \
    PLIST_ENTRY _EX_Previous = (Previous);                              \
    (Entry)->Flink = _EX_Next;                                          \
    (Entry)->Blink = _EX_Previous;                                      \
    _EX_Next->Blink = (Entry);                                          \
    _EX_Previous->Flink = (Entry);                                      \
    }
    



#endif            
