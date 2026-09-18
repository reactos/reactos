// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//     MIL requires a number of handle tables which have different entry
//     types, but a similar structure and handling. HANDLE_TABLE class
//     provides a generic handle table implementation.
// 
//-----------------------------------------------------------------------------

MtExtern(CMILHandleTable);

//
// Make table growth much more common on debug builds so that the code
// is well exercised. On retail, use bigger numbers so we can optimize
// for performance.
//

#if DBG
#define MIL_HANDLE_TABLE_SIZE_MIN   ((UINT)0x5)
#define MIL_HANDLE_TABLE_SIZE_INC   ((UINT)0x20)
#else
#define MIL_HANDLE_TABLE_SIZE_MIN   ((UINT)0x400)
#define MIL_HANDLE_TABLE_SIZE_INC   ((UINT)0x400)
#endif

//
// Reasonably limit the maximum size (in entries) of the handle table
//

#define MIL_HANDLE_TABLE_SIZE_MAX   ((UINT)(64 * 1024 * 1024))

//
// Type field in the handle entry indicating an unused location.
//

#define EMPTY_ENTRY 0

//
// This is an accessor macro which looks up the given object index in the
// specified handle-table. It returns a VOID pointer to the handle entry
// in the table. It should be cast to the proper entry type for accessing
// data members.
//

#define ENTRY_RECORD(table, idx) \
    ((VOID*)((BYTE*)(table).m_pvTable + (idx) * (table).m_cbEntry))

/*++

struct Description:

    HANDLE_TABLE

    A table of HMIL_OBJECT/CDceObject types.

    This struct implements a low-level set of functionality common to a
    variety of handle-table and translation-table types. Features include
    caller-defined table-ENTRY type, resize/growth algorithm and the ability
    to validate that entries are of an expected type.

    Every table entry is expected to have the first field of MIL_OBJECT_TYPES
    (or a similar enum which defines 0 as invalid or free) so that we can
    validate entries. This field is also used to determine if an entry is in
    use or not.

--*/

struct HANDLE_TABLE
{

public:

    HANDLE_TABLE(UINT cbEntry);
    virtual ~HANDLE_TABLE();

    //
    // Resize the handle table to fit the requested handle. The buffer
    // might be resized a little bit more aggressively to amortize the
    // memory re-allocation. 
    //

    HRESULT ResizeToFit(
        __in HMIL_OBJECT object
        );

    //
    // Find and reserve a table entry for the provided object type. The handle
    // table is resized as necessary.
    //

    HRESULT GetNewEntry(DWORD type, __out_ecount(1) HMIL_OBJECT *phObject);

    //
    // Assign a value to the given table slot. This should be performed only
    // on an empty slot. Once the function succeeds, the table is grown to
    // include the specified entry (if necessary) and the entry is no longer
    // empty - it has the assigned type.
    //

    HRESULT AssignEntry(__in HMIL_OBJECT object, DWORD type);

    //
    // Reclaims a handle entry for reuse. Objects referenced by the HANDLE_ENTRY
    // must have been previously reclaimed.
    //

    VOID DestroyHandle(__in HMIL_OBJECT object);

    //
    // Validate that the object in the table is the expected type and
    // is actually an object assigned to this client.
    //

    BOOL ValidEntry(DWORD type, __in HMIL_OBJECT object) const;
    BOOL ValidEntry(__in HMIL_OBJECT object) const;

    //
    // Return the type of the object stored at the given location. Returns
    // zero if the entry is empty or invalid.
    //

    DWORD GetObjectType(__in HMIL_OBJECT object);

private:
    //
    // Resize the table to accommodate the given total number of handles.
    // The first time this is called, it will size the table (as opposed to
    // re-size) creating the initial allocation.
    //

    HRESULT Resize(UINT cTableSize);
    
public:

    //
    // Size of the table entries in bytes.
    //

    UINT m_cbEntry;

    //
    // Number of handle slots/entries for which we have allocated memory.
    //

    UINT m_cHandleCount;

    //
    // Free index search.
    //

    UINT m_nFreeIndex;

    //
    // Allocated handle table memory. The handle-table manages an un-typed
    // block of memory containing the handles. The caller defines the field
    // layout of the individual entries in the table - the only information
    // that the handle table relies on is the m_cbEntry (size of each table
    // entry) and the fact that the first field in any table entry is
    // MIL_RESOURCE_TYPE or a similar enum of DWORD size. EMPTY_ENTRY or zero is
    // used to indicate that the table entry is empty.
    //
    // When debugging, cast the type of the table to see the individual entries
    // for example:
    //
    //    dt -a20 CMilSlaveHandleTable::HANDLE_ENTRY <address>
    //
    //    This will dump the first 20 handle entries in the server device table
    //    provided the m_pvTable pointer is provided as <address>
    //
    //
    //    0:002> dt -a3 CMilSlaveHandleTable::HANDLE_ENTRY 0x00086410
    //    [0] @ 00086410
    //    ---------------------------------------------
    //       +0x000 Type             : 0 ( MilObjectTypeInvalid )
    //       +0x004 PObject          : (null)
    //
    //    [1] @ 00086418
    //    ---------------------------------------------
    //       +0x000 Type             : 78746343 ( MilObjectTypeContext )
    //       +0x004 PObject          : 0x000866e8
    //
    //    [2] @ 00086420
    //    ---------------------------------------------
    //       +0x000 Type             : 0 ( MilObjectTypeInvalid )
    //       +0x004 PObject          : (null)
    //
    //

    VOID *m_pvTable;
};

enum MIL_RESOURCE_TYPE;
#define HMIL_RESOURCE_NULL NULL

class CMILHandleTable : public CMILRefCountBase
{

public:

    CMILHandleTable() : CMILRefCountBase()
    {
    };

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILHandleTable));
};


