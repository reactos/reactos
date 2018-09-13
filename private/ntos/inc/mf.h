/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mf.h

Abstract:

    This header describes the structures and interfaces required to interact
    with the multifunction enumerator.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

DEFINE_GUID( GUID_MF_ENUMERATION_INTERFACE,        
             0xaeb895f0L, 0x5586, 0x11d1, 0x8d, 0x84, 0x00, 0xa0, 0xc9, 0x06, 0xb2, 0x44 );

#if !defined(_MF_)
#define _MF_

//
// MfFlags value
//

#define MF_FLAGS_EVEN_IF_NO_RESOURCE            0x00000001
#define MF_FLAGS_NO_CREATE_IF_NO_RESOURCE       0x00000002
#define MF_FLAGS_FILL_IN_UNKNOWN_RESOURCE       0x00000004
#define MF_FLAGS_CREATE_BUT_NO_SHOW_DISABLED    0x00000008

typedef struct _MF_RESOURCE_MAP {
    
    ULONG Count;
    UCHAR Resources[ANYSIZE_ARRAY];

} MF_RESOURCE_MAP, *PMF_RESOURCE_MAP;


typedef struct _MF_VARYING_RESOURCE_ENTRY {

    UCHAR ResourceIndex;
    UCHAR Reserved[3];      // Packing
    ULONG Offset;
    ULONG Size;
    ULONG MaxCount;

} MF_VARYING_RESOURCE_ENTRY, *PMF_VARYING_RESOURCE_ENTRY;


typedef struct _MF_VARYING_RESOURCE_MAP {
    
    ULONG Count;
    MF_VARYING_RESOURCE_ENTRY Resources[ANYSIZE_ARRAY];

} MF_VARYING_RESOURCE_MAP, *PMF_VARYING_RESOURCE_MAP;


typedef struct _MF_DEVICE_INFO *PMF_DEVICE_INFO;

typedef struct _MF_DEVICE_INFO {
    
    //
    // Name for this child, unique with respect to the other children
    //
    UNICODE_STRING Name;

    //
    // A REG_MULTI_SZ style list of hardware IDs
    //
    UNICODE_STRING HardwareID;
    
    //
    // A REG_MULTI_SZ style list of compatible IDs
    //
    UNICODE_STRING CompatibleID;

    //
    // Map of resource that we totally consume
    //
    PMF_RESOURCE_MAP ResourceMap;
    
    //
    // Map of resource that we partially consume
    //
    PMF_VARYING_RESOURCE_MAP VaryingResourceMap;
    
    //
    // Flags - 
    //      MF_FLAGS_FILL_IN_UNKNOWN_RESOURCE - if the parent resource doesn't 
    //          contain a descriptor referenced in the ResourceMap use a 
    //          null (CmResourceTypeNull) descriptor instead.
    //      MF_FLAGS_EVEN_IF_NO_RESOURCE - enumerate the child even if it
    //          doesn't have any resources.
    //      MF_FLAGS_NO_CREATE_IF_NO_RESOURCE - if we can't find a resource 
    //          referenced for the child then don't enumerate the child
    //      MF_FLAGS_CREATE_BUT_NO_SHOW_DISABLED - ??
    //
    //      BUGBUG - what do these mean?
    //
    ULONG MfFlags;

} MF_DEVICE_INFO;

typedef
NTSTATUS
(*PMF_ENUMERATE_CHILD)(
    IN PVOID Context,
    IN ULONG Index,
    OUT PMF_DEVICE_INFO ChildInfo
    );

/*++


Routine Description:
    
    This returns information about children to be enumerated by a multifunction 
    driver.
    
Arguments:

    Context - Context from the MF_ENUMERATION_INTERFACE    
    
    Index - Zero based index of the children
    
    ChildInfo - Pointer to a caller allocated buffer that should be filled in
        by the callee.  This will involve allocation of extra buffers for each
        piece of information.  These will be freed by calling ExFreePool when
        they are no longer required.

Return Value:

    Status code that indicates whether or not the function was successful.
    
    STATUS_NO_MORE_ENTRIES indicates that the are no more children to enumerate

--*/

typedef struct _MF_ENUMERATION_INTERFACE {
    
    //
    // Generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    
    //
    // Multi-function enumeration data
    //
    PMF_ENUMERATE_CHILD EnumerateChild;

} MF_ENUMERATION_INTERFACE, *PMF_ENUMERATION_INTERFACE;

#endif
