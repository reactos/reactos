#ifndef _FXHANDLE_H_
#define _FXHANDLE_H_

#include "common/fxverifier.h"
#include "common/fxtypes.h"
#include "wdf.h"


class FxObject;
struct FxContextHeader;

struct FxContextHeader {
    //
    // Backpointer to the object that this is a context for
    //
    FxObject* Object;

    //
    // Next context in the chain
    //
    FxContextHeader* NextHeader;

    //
    // Function to call when object is deleted
    //
    PFN_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanupCallback;

    //
    // Function to call when the object's memory is destroyed
    // when the last reference count goes to zero
    //
    PFN_WDF_OBJECT_CONTEXT_DESTROY EvtDestroyCallback;

    //
    // Type associated with this context
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO ContextTypeInfo;

    //
    // Start of client's context
    //
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONG_PTR Context[1];
};

//
// We want all the way up to the aligned field, but not the field itself
//
#define FX_CONTEXT_HEADER_SIZE FIELD_OFFSET(FxContextHeader, Context)

#define COMPUTE_RAW_OBJECT_SIZE(_rawObjectSize) \
    ((USHORT) WDF_ALIGN_SIZE_UP(_rawObjectSize, MEMORY_ALLOCATION_ALIGNMENT))

//
// Computes the size required for a fixed size object plus any extra buffer space
// it requires.  The extra buffer space is aligned on a process natural boundary.
//
#define COMPUTE_OBJECT_SIZE(_rawObjectSize, _extraSize) \
   (COMPUTE_RAW_OBJECT_SIZE(_rawObjectSize) + (USHORT) WDF_ALIGN_SIZE_UP(_extraSize, MEMORY_ALLOCATION_ALIGNMENT))

// forward definitions
typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;
enum FxObjectType : int;

PVOID
FxObjectHandleAlloc(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        POOL_TYPE PoolType,
    __in        size_t Size,
    __in        ULONG Tag,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        USHORT ExtraSize,
    __in        FxObjectType ObjectType
    );

//
// Gets size of the context associated with the specified attributes structure.
//
size_t
FxGetContextSize(
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    );

//
// Computes the total size of an object taking into account its fixed size,
// any additional size after the fixed, and an object header, so the memory looks
// like:
//
// Object
// additional optional memory
// WDF_HANDLE_HEADER
//
_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize2(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in        size_t ContextSize,
    __out       size_t* Total
    );

_Must_inspect_result_
NTSTATUS
FxCalculateObjectTotalSize(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        USHORT RawObjectSize,
    __in        USHORT ExtraSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out       size_t* Total
    );

PVOID
FxObjectAndHandleHeaderInit(
    __in        PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        PVOID AllocationStart,
    __in        USHORT ObjectSize,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in        FxObjectType ObjectType
    );

VOID
FxContextHeaderInit(
    __in        FxContextHeader* Header,
    __in        FxObject* Object,
    __in_opt    PWDF_OBJECT_ATTRIBUTES Attributes
    );

//__inline
VOID
FxObjectHandleGetPtr(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in WDFOBJECT Handle,
    __in WDFTYPE Type,
    __out PVOID* PPObject
    );

VOID
FxObjectHandleGetPtrQI(
    __in FxObject* Object,
    __out PVOID* PPObject,
    __in WDFOBJECT Handle,
    __in WDFTYPE Type,
    __in WDFOBJECT_OFFSET Offset
    );

//__inline
VOID
FxObjectHandleCreate(
    __in  FxObject* Object,
    __out PWDFOBJECT Handle
    );

VOID
FxObjectHandleGetPtrAndGlobals(
    __in  PFX_DRIVER_GLOBALS CallersGlobals,
    __in  WDFOBJECT Handle,
    __in  WDFTYPE   Type,
    __out PVOID*    PPObject,
    __out PFX_DRIVER_GLOBALS* ObjectGlobals
    );

_Must_inspect_result_
NTSTATUS
FxObjectAllocateContext(
    __in        FxObject*               Object,
    __in        PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in        BOOLEAN                 AllowCallbacksOnly,
    __deref_opt_out PVOID*              Context
    );

#endif //_FXHANDLE_H_