/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pnprlist.h

Abstract:

    This file declares the routines and data structures used to manipulate
    relations list.  Relation lists are used by Plug and Play during the
    processing of device removal and ejection.

Author:

    Robert Nelson (robertn) Apr, 1998.

Revision History:

--*/

//
// A RELATION_LIST_ENTRY is an element of a relation list.
//
// It contains all the PDEVICE_OBJECTS which exist at the same level in the
// DEVICE_NODE tree.
//
// Individual PDEVICE_OBJECT entries are tagged by setting their lowest bit.
//
// MaxCount indicates the size of the Devices array.  Count indicates the number
// of elements which are currently being used.  When a relation list is
// compressed Count will equal MaxCount.
//
typedef struct _RELATION_LIST_ENTRY {
    ULONG                   Count;          // Number of current entries
    ULONG                   MaxCount;       // Size of Entries list
    PDEVICE_OBJECT          Devices[1];     // Variable length list of device objects
}   RELATION_LIST_ENTRY, *PRELATION_LIST_ENTRY;

//
// A RELATION_LIST contains a number of RELATION_LIST_ENTRY structures.
//
// Each entry in Entries describes all the devices of a given level in the
// DEVICE_NODE tree.  In order to conserve memory, space is only allocated for
// the entries between the lowest and highest levels inclusive.  The member
// FirstLevel indicates which level is at index 0 of Entries.  MaxLevel
// indicates the last level represented in Entries.  The number of entries is
// determined by the formula MaxLevel - FirstLevel + 1.  The Entries array can
// be sparse.  Each element of Entries will either be a PRELATION_LIST_ENTRY or
// NULL.
//
// The total number of PDEVICE_OBJECTs in all PRELATION_LIST_ENTRYs is kept in
// Count.  Individual PDEVICE_OBJECTS may be tagged.  The tag is maintained in
// Bit 0 of the PDEVICE_OBJECT.  The total number of PDEVICE_OBJECTs tagged is
// kept in TagCount.  This is used to rapidly determine whether or not all
// objects have been tagged.
//
typedef struct _RELATION_LIST {
    ULONG                   Count;          // Count of Devices in all Entries
    ULONG                   TagCount;       // Count of Tagged Devices
    ULONG                   FirstLevel;     // Level Number of Entries[0]
    ULONG                   MaxLevel;       // - FirstLevel + 1 = Number of Entries
    PRELATION_LIST_ENTRY    Entries[1];     // Variable length list of entries
}   RELATION_LIST, *PRELATION_LIST;

//
// A PENDING_RELATIONS_LIST_ENTRY is used to track relation lists for operations
// which may pend.  This includes removal when open handles exist and device
// ejection.
//
// The Link field is used to link the PENDING_RELATIONS_LIST_ENTRYs together.
//
// The DeviceObject field is the DEVICE_OBJECT to which the operation was
// originally targetted.  It will also exist as a member of the relations list.
//
// The RelationsList is a list of BusRelations, RemovalRelations, (and
// EjectionRelations in the case of eject) which are related to DeviceObject and
// its relations.
//
// The EjectIrp is pointer to the Eject IRP which has been sent to the PDO.  If
// this is a pending surprise removal then EjectIrp is not used.
//
typedef struct _PENDING_RELATIONS_LIST_ENTRY {
    LIST_ENTRY              Link;
    WORK_QUEUE_ITEM         WorkItem;
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent;
    PDEVICE_OBJECT          DeviceObject;
    PRELATION_LIST          RelationsList;
    PIRP                    EjectIrp;
    ULONG                   Problem;
    BOOLEAN                 ProfileChangingEject;
    BOOLEAN                 DisplaySafeRemovalDialog;
    SYSTEM_POWER_STATE      LightestSleepState;
    PDOCK_INTERFACE         DockInterface;
}   PENDING_RELATIONS_LIST_ENTRY, *PPENDING_RELATIONS_LIST_ENTRY;

//
// Functions exported to other kernel modules.
//
NTSTATUS
IopAddRelationToList(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DirectDescendant,
    IN BOOLEAN Tagged
    );

PRELATION_LIST
IopAllocateRelationList(
    VOID
    );

NTSTATUS
IopCompressRelationList(
    IN OUT PRELATION_LIST *List
    );

BOOLEAN
IopEnumerateRelations(
    IN PRELATION_LIST List,
    IN OUT PULONG Marker,
    OUT PDEVICE_OBJECT *PhysicalDevice,
    OUT BOOLEAN *DirectDescendant, OPTIONAL
    OUT BOOLEAN *Tagged, OPTIONAL
    BOOLEAN Reverse
    );

VOID
IopFreeRelationList(
    IN PRELATION_LIST List
    );

ULONG
IopGetRelationsCount(
    IN PRELATION_LIST List
    );

ULONG
IopGetRelationsTaggedCount(
    IN PRELATION_LIST List
    );

BOOLEAN
IopIsRelationInList(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IopMergeRelationLists(
    IN OUT PRELATION_LIST TargetList,
    IN PRELATION_LIST SourceList,
    IN BOOLEAN Tagged
    );

NTSTATUS
IopRemoveIndirectRelationsFromList(
    IN PRELATION_LIST List
    );

NTSTATUS
IopRemoveRelationFromList(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IopSetAllRelationsTags(
    IN PRELATION_LIST List,
    IN BOOLEAN Tagged
    );

NTSTATUS
IopSetRelationsTag(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Tagged
    );

