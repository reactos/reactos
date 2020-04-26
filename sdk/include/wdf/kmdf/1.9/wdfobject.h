/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfobject.h

Abstract:

    This is the C header for driver frameworks objects

Revision History:


--*/

#ifndef _WDFOBJECT_H_
#define _WDFOBJECT_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

// 
// Specifies the highest IRQL level allowed on callbacks
// to the device driver.
// 
typedef enum _WDF_EXECUTION_LEVEL {
    WdfExecutionLevelInvalid = 0x00,
    WdfExecutionLevelInheritFromParent,
    WdfExecutionLevelPassive,
    WdfExecutionLevelDispatch,
} WDF_EXECUTION_LEVEL;

// 
// Specifies the concurrency of callbacks to the device driver
// 
typedef enum _WDF_SYNCHRONIZATION_SCOPE {
    WdfSynchronizationScopeInvalid = 0x00,
    WdfSynchronizationScopeInheritFromParent,
    WdfSynchronizationScopeDevice,
    WdfSynchronizationScopeQueue,
    WdfSynchronizationScopeNone,
} WDF_SYNCHRONIZATION_SCOPE;



typedef
__drv_functionClass(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_OBJECT_CONTEXT_CLEANUP(
    __in
    WDFOBJECT Object
    );

typedef EVT_WDF_OBJECT_CONTEXT_CLEANUP *PFN_WDF_OBJECT_CONTEXT_CLEANUP;

typedef
__drv_functionClass(EVT_WDF_OBJECT_CONTEXT_DESTROY)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_OBJECT_CONTEXT_DESTROY(
    __in
    WDFOBJECT Object
    );

typedef EVT_WDF_OBJECT_CONTEXT_DESTROY *PFN_WDF_OBJECT_CONTEXT_DESTROY;


typedef const struct _WDF_OBJECT_CONTEXT_TYPE_INFO *PCWDF_OBJECT_CONTEXT_TYPE_INFO;

typedef struct _WDF_OBJECT_ATTRIBUTES {
    //
    // Size in bytes of this structure
    //
    ULONG Size;

    //
    // Function to call when the object is deleted
    //
    PFN_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanupCallback;

    //
    // Function to call when the objects memory is destroyed when the
    // the last reference count goes to zero
    //
    PFN_WDF_OBJECT_CONTEXT_DESTROY EvtDestroyCallback;

    //
    // Execution level constraints for Object
    //
    WDF_EXECUTION_LEVEL ExecutionLevel;

    //
    // Synchronization level constraint for Object
    //
    WDF_SYNCHRONIZATION_SCOPE SynchronizationScope;

    //
    // Optional Parent Object
    //
    WDFOBJECT ParentObject;

    //
    // Overrides the size of the context allocated as specified by
    // ContextTypeInfo->ContextSize
    //
    size_t ContextSizeOverride;

    //
    // Pointer to the type information to be associated with the object
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO ContextTypeInfo;

} WDF_OBJECT_ATTRIBUTES, *PWDF_OBJECT_ATTRIBUTES;

VOID
FORCEINLINE
WDF_OBJECT_ATTRIBUTES_INIT(
    __out PWDF_OBJECT_ATTRIBUTES Attributes
    )
{
    RtlZeroMemory(Attributes, sizeof(WDF_OBJECT_ATTRIBUTES));
    Attributes->Size = sizeof(WDF_OBJECT_ATTRIBUTES);
    Attributes->ExecutionLevel = WdfExecutionLevelInheritFromParent;
    Attributes->SynchronizationScope = WdfSynchronizationScopeInheritFromParent;
}

#define WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(_attributes, _contexttype) \
    (_attributes)->ContextTypeInfo = WDF_GET_CONTEXT_TYPE_INFO(_contexttype)->UniqueType
//
// VOID
// FORCEINLINE
// WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
//     PWDF_OBJECT_ATTRIBUTES Attributes,
//     <typename>
//     )
//
// NOTE:  Do not put a ; at the end of the last line.  This will require the
// caller to specify a ; after the call.
//
#define WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(_attributes,  _contexttype) \
    WDF_OBJECT_ATTRIBUTES_INIT(_attributes);                                \
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(_attributes, _contexttype)

typedef
PCWDF_OBJECT_CONTEXT_TYPE_INFO
(__cdecl *PFN_GET_UNIQUE_CONTEXT_TYPE)(
    VOID
    );

//
// Since C does not have strong type checking we must invent our own
//
typedef struct _WDF_OBJECT_CONTEXT_TYPE_INFO {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // String representation of the context's type name, i.e. "DEVICE_CONTEXT"
    //
    PCHAR ContextName;

    //
    // The size of the context in bytes.  This will be the size of the context
    // associated with the handle unless
    // WDF_OBJECT_ATTRIBUTES::ContextSizeOverride is specified.
    //
    size_t ContextSize;

    //
    // If NULL, this structure is the unique type identifier for the context
    // type.  If != NULL, the UniqueType pointer value is the unique type id
    // for the context type.
    //
    PCWDF_OBJECT_CONTEXT_TYPE_INFO UniqueType;

    //
    // Function pointer to retrieve the context type information structure
    // pointer from the provider of the context type.  This function is invoked
    // by the client driver's entry point by the KMDF stub after all class
    // drivers are loaded and before DriverEntry is invoked.
    //
    PFN_GET_UNIQUE_CONTEXT_TYPE EvtDriverGetUniqueContextType;

} WDF_OBJECT_CONTEXT_TYPE_INFO, *PWDF_OBJECT_CONTEXT_TYPE_INFO;

//
// Converts a type name a unique name in which we can retrieve type specific
// information.
//
#define WDF_TYPE_NAME_TO_TYPE_INFO(_contexttype) \
      _WDF_ ## _contexttype ## _TYPE_INFO

//
// Converts a type name a unique name to the structure which will initialize
// it through an external component.
//
#define WDF_TYPE_NAME_TO_EXTERNAL_INIT(_contexttype) \
      _WDF_ ## _contexttype ## _EXTERNAL_INIT

#define WDF_TYPE_NAME_TO_EXTERNAL_INIT_FUNCTION(_contexttype) \
      _contexttype ## _EXTERNAL_INIT_FUNCTION

//
// Returns an address to the type information representing this typename
//
#define WDF_GET_CONTEXT_TYPE_INFO(_contexttype) \
    (&WDF_TYPE_NAME_TO_TYPE_INFO(_contexttype))

//
// Used to help generate our own usable pointer to the type typedef.  For instance,
// a call as WDF_TYPE_NAME_POINTER_TYPE(DEVICE_CONTEXT) would generate:
//
// WDF_POINTER_TYPE_DEVICE_CONTEXT
//
// which would be the equivalent of DEVICE_CONTEXT*
//
#define WDF_TYPE_NAME_POINTER_TYPE(_contexttype) \
    WDF_POINTER_TYPE_ ## _contexttype

//
// Declares a typename so that in can be associated with a handle.  This will
// use the type's name with a _ prepended as the "friendly name" (which results
// in the autogenerated casting function being named WdfObjectGet_<typename>, ie
// WdfObjectGet_DEVICE_CONTEXT.  See WDF_DECLARE_CONTEXT_TYPE_WITH_NAME for
// more details on what is generated.
//
#define WDF_DECLARE_CONTEXT_TYPE(_contexttype)  \
        WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(_contexttype, WdfObjectGet_ ## _contexttype)

//
// WDF_DECLARE_CONTEXT_TYPE_WITH_NAME performs the following 3 tasks
//
// 1)  declare a typedef for the context type so that its pointer type can be
//     referred to later
// 2)  declare and initialize global structure that represents the type
//     information for this
//     context type
// 3)  declare and implement a function named _castingfunction
//     which does the proper type conversion.
//
// WDF_DECLARE_TYPE_AND_GLOBALS implements 1 & 2
// WDF_DECLARE_CASTING_FUNCTION implements 3
//
// For instance, the invocation of
// WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, WdfDeviceGetContext)
// would result in the following being generated:
//
// typedef DEVICE_CONTEXT* WDF_POINTER_TYPE_DEVICE_CONTEXT;
//
// extern const __declspec(selectany) WDF_OBJECT_CONTEXT_TYPE_INFO  _WDF_DEVICE_CONTEXT_TYPE_INFO =
// {
//     sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO),
//     "DEVICE_CONTEXT",
//     sizeof(DEVICE_CONTEXT),
// };
//
// WDF_POINTER_TYPE_DEVICE_CONTEXT
// WdfDeviceGetContext(
//    WDFOBJECT Handle
//    )
// {
//     return (WDF_POINTER_TYPE_DEVICE_CONTEXT)
//         WdfObjectGetTypedContextWorker(
//              Handle,
//              (&_WDF_DEVICE_CONTEXT_TYPE_INFO)->UniqueType
//              );
// }
//
#define WDF_TYPE_INIT_BASE_SECTION_NAME ".kmdftypeinit"
#define WDF_TYPE_INIT_SECTION_NAME      ".kmdftypeinit$b"

//
// .data is the default section that global data would be placed into.  We
// cannot just use ".data" in __declspec(allocate()) without first declaring
// it in a #pragma section() even though it is a default section name.
//
#ifndef WDF_TYPE_DEFAULT_SECTION_NAME
#define WDF_TYPE_DEFAULT_SECTION_NAME ".data"
#endif // WDF_TYPE_DEFAULT_SECTION_NAME

#pragma section(WDF_TYPE_INIT_SECTION_NAME, read, write)
#pragma section(WDF_TYPE_DEFAULT_SECTION_NAME)

#define WDF_DECLARE_TYPE_AND_GLOBALS(_contexttype, _UniqueType, _GetUniqueType, _section)\
                                                                        \
typedef _contexttype* WDF_TYPE_NAME_POINTER_TYPE(_contexttype);         \
                                                                        \
WDF_EXTERN_C __declspec(allocate( _section )) __declspec(selectany) extern const WDF_OBJECT_CONTEXT_TYPE_INFO WDF_TYPE_NAME_TO_TYPE_INFO(_contexttype) =  \
{                                                                       \
    sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO),                               \
    #_contexttype,                                                      \
    sizeof(_contexttype),                                               \
    _UniqueType,                                                        \
    _GetUniqueType,                                                     \
};                                                                      \

#define WDF_DECLARE_CASTING_FUNCTION(_contexttype, _castingfunction)    \
                                                                        \
__drv_aliasesMem                                                        \
WDF_EXTERN_C                                                            \
WDF_TYPE_NAME_POINTER_TYPE(_contexttype)                                \
FORCEINLINE                                                             \
_castingfunction(                                                       \
   __in WDFOBJECT Handle                                                     \
   )                                                                    \
{                                                                       \
    return (WDF_TYPE_NAME_POINTER_TYPE(_contexttype))                   \
        WdfObjectGetTypedContextWorker(                                 \
            Handle,                                                     \
            WDF_GET_CONTEXT_TYPE_INFO(_contexttype)->UniqueType         \
            );                                                          \
}

#define WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(_contexttype, _castingfunction) \
                                                                        \
WDF_DECLARE_TYPE_AND_GLOBALS(                                           \
    _contexttype,                                                       \
    WDF_GET_CONTEXT_TYPE_INFO(_contexttype),                            \
    NULL,                                                               \
    WDF_TYPE_DEFAULT_SECTION_NAME)                                      \
                                                                        \
WDF_DECLARE_CASTING_FUNCTION(_contexttype, _castingfunction)

//
// WDF_DECLARE_SHARED_CONTEXT_TYPE_WITH_NAME is the same as
// WDF_DECLARE_CONTEXT_TYPE_WITH_NAME with respect to the types and structures
// that are created and initialized.  The casting function is different in that
// it passes the UniqueType to WdfObjectGetTypedContextWorker() instead of the
// global type structure created.  It also creates a structure which will contain
// an initialization function which will be invoked before DriverEntry() is
// called.
//
// It is the responsibilty of the component exporting the unique type to define
// and implement the function which will return the unique type.   The format of
// the define is:
//
// #define _contexttype ## _EXTERNAL_INIT_FUNCTION
//
// (e.g. #define DEVICE_CONTEXT_EXTERNALINIT_FUNCTION DeviceContextInit()
//  for a type of DEVICE_CONTEXT)
//
#define WDF_DECLARE_SHARED_CONTEXT_TYPE_WITH_NAME(_contexttype, _castingfunction) \
                                                                        \
WDF_DECLARE_TYPE_AND_GLOBALS(                                           \
    _contexttype,                                                       \
    NULL,                                                               \
    WDF_TYPE_NAME_TO_EXTERNAL_INIT_FUNCTION(_contexttype),              \
    WDF_TYPE_INIT_SECTION_NAME)                                         \
                                                                        \
WDF_DECLARE_CASTING_FUNCTION(_contexttype, _castingfunction)

//
// Generic conversion macro from handle to type.  This should be used if the
// autogenerated conversion function does not suite the programmers calling style.
//
// The type parameter should be name of the type (e.g. DEVICE_CONTEXT), not the
// name of the pointer to the type (PDEVICE_CONTEXT).
//
// Example call:
//
// WDFDEVICE device;
// PDEVICE_CONTEXT pContext;
//
// pContext = WdfObjectGetTypedContext(device, DEVICE_CONTEXT);
//
//
#define WdfObjectGetTypedContext(handle, type)  \
(type*)                                         \
WdfObjectGetTypedContextWorker(                 \
    (WDFOBJECT) handle,                         \
    WDF_GET_CONTEXT_TYPE_INFO(type)->UniqueType \
    )

//
// WDF Function: WdfObjectGetTypedContextWorker
//
typedef
WDFAPI
PVOID
(FASTCALL *PFN_WDFOBJECTGETTYPEDCONTEXTWORKER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    );

PVOID
FORCEINLINE
WdfObjectGetTypedContextWorker(
    __in
    WDFOBJECT Handle,
    __in
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    )
{
    return ((PFN_WDFOBJECTGETTYPEDCONTEXTWORKER) WdfFunctions[WdfObjectGetTypedContextWorkerTableIndex])(WdfDriverGlobals, Handle, TypeInfo);
}

//
// WDF Function: WdfObjectAllocateContext
//
typedef
WDFAPI
NTSTATUS
(*PFN_WDFOBJECTALLOCATECONTEXT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in
    PWDF_OBJECT_ATTRIBUTES ContextAttributes,
    __deref_opt_out
    PVOID* Context
    );

NTSTATUS
FORCEINLINE
WdfObjectAllocateContext(
    __in
    WDFOBJECT Handle,
    __in
    PWDF_OBJECT_ATTRIBUTES ContextAttributes,
    __deref_opt_out
    PVOID* Context
    )
{
    return ((PFN_WDFOBJECTALLOCATECONTEXT) WdfFunctions[WdfObjectAllocateContextTableIndex])(WdfDriverGlobals, Handle, ContextAttributes, Context);
}

//
// WDF Function: WdfObjectContextGetObject
//
typedef
WDFAPI
WDFOBJECT
(FASTCALL *PFN_WDFOBJECTCONTEXTGETOBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PVOID ContextPointer
    );

WDFOBJECT
FORCEINLINE
WdfObjectContextGetObject(
    __in
    PVOID ContextPointer
    )
{
    return ((PFN_WDFOBJECTCONTEXTGETOBJECT) WdfFunctions[WdfObjectContextGetObjectTableIndex])(WdfDriverGlobals, ContextPointer);
}

//
// WDF Function: WdfObjectReferenceActual
//
typedef
WDFAPI
VOID
(*PFN_WDFOBJECTREFERENCEACTUAL)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in __nullterminated
    PCHAR File
    );

VOID
FORCEINLINE
WdfObjectReferenceActual(
    __in
    WDFOBJECT Handle,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in __nullterminated
    PCHAR File
    )
{
    ((PFN_WDFOBJECTREFERENCEACTUAL) WdfFunctions[WdfObjectReferenceActualTableIndex])(WdfDriverGlobals, Handle, Tag, Line, File);
}

//
// WDF Function: WdfObjectDereferenceActual
//
typedef
WDFAPI
VOID
(*PFN_WDFOBJECTDEREFERENCEACTUAL)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Handle,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in __nullterminated
    PCHAR File
    );

VOID
FORCEINLINE
WdfObjectDereferenceActual(
    __in
    WDFOBJECT Handle,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in __nullterminated
    PCHAR File
    )
{
    ((PFN_WDFOBJECTDEREFERENCEACTUAL) WdfFunctions[WdfObjectDereferenceActualTableIndex])(WdfDriverGlobals, Handle, Tag, Line, File);
}

//
// WDF Function: WdfObjectCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFOBJECTCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFOBJECT* Object
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfObjectCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFOBJECT* Object
    )
{
    return ((PFN_WDFOBJECTCREATE) WdfFunctions[WdfObjectCreateTableIndex])(WdfDriverGlobals, Attributes, Object);
}

//
// WDF Function: WdfObjectDelete
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFOBJECTDELETE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfObjectDelete(
    __in
    WDFOBJECT Object
    )
{
    ((PFN_WDFOBJECTDELETE) WdfFunctions[WdfObjectDeleteTableIndex])(WdfDriverGlobals, Object);
}

//
// WDF Function: WdfObjectQuery
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFOBJECTQUERY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object,
    __in
    CONST GUID* Guid,
    __in
    ULONG QueryBufferLength,
    __out_bcount(QueryBufferLength)
    PVOID QueryBuffer
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfObjectQuery(
    __in
    WDFOBJECT Object,
    __in
    CONST GUID* Guid,
    __in
    ULONG QueryBufferLength,
    __out_bcount(QueryBufferLength)
    PVOID QueryBuffer
    )
{
    return ((PFN_WDFOBJECTQUERY) WdfFunctions[WdfObjectQueryTableIndex])(WdfDriverGlobals, Object, Guid, QueryBufferLength, QueryBuffer);
}



//
// Reference an object
//
// VOID
// WdfObjectReference(
//    __in WDFOBJECT Handle
//    );
//
// VOID
// WdfObjectReferenceWithTag(
//     __in WDFOBJECT Handle,
//     __in PVOID Tag
//     );
//
#define WdfObjectReference(Handle) \
        WdfObjectReferenceWithTag(Handle, NULL)

#define WdfObjectReferenceWithTag(Handle, Tag) \
        WdfObjectReferenceActual(Handle, Tag, __LINE__, __FILE__)


//
// Dereference an object.  If an object allows for a client explicitly deleting
// it, call WdfObjectDelete.  Do not use WdfObjectDereferenceXxx to delete an
// object.
//
// VOID
// WdfObjectDereference(
//   __in WDFOBJECT Handle
//   );
//
// VOID
// WdfObjectDereferenceWithTag(
//     __in WDFOBJECT Handle
//     __in PVOID Tag
//     );
//
#define WdfObjectDereference(Handle)   \
        WdfObjectDereferenceWithTag(Handle, NULL)

#define WdfObjectDereferenceWithTag(Handle, Tag) \
        WdfObjectDereferenceActual(Handle, Tag, __LINE__, __FILE__)

#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFOBJECT_H_

