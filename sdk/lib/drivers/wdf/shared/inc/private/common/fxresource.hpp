/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxResource.hpp

Abstract:

    This module implements the resource object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXRESOURCE_H_
#define _FXRESOURCE_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxResource.hpp.tmh"
#endif

}

#if (FX_CORE_MODE==FX_CORE_USER_MODE)

struct FxRegisterResourceInfo : FxStump {
    //
    // start physical address of resource range assigned by pnp
    //
    PHYSICAL_ADDRESS m_StartPa;

    //
    // start of mapped system base address
    //
    PVOID m_StartSystemVa;

    //
    // end of mapped system base address
    //
    PVOID m_EndSystemVa;

    //
    // user-mode mapped address
    //
    PVOID m_StartUsermodeVa;

    //
    // Length of resource range assigned by pnp
    //
    SIZE_T m_Length;

    //
    // Length of mapped resource range
    //
    SIZE_T m_MappedLength;

    static
    NTSTATUS
    _CreateAndInit(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _In_ ULONG Count,
        _Out_ FxRegisterResourceInfo** RegisterTable
        )
    {
        NTSTATUS status;
        FxRegisterResourceInfo* table;

        ASSERT(RegisterTable != NULL);
        *RegisterTable = NULL;

        table = new (DriverGlobals) FxRegisterResourceInfo[Count];
        if (table == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failed to allocate Resource table %!STATUS!", status);
            return status;
        }

        *RegisterTable = table;
        status = STATUS_SUCCESS;

        return status;
    }

    FxRegisterResourceInfo(
        VOID
        ):
        m_StartSystemVa(NULL),
        m_EndSystemVa(NULL),
        m_StartUsermodeVa(NULL),
        m_Length(0),
        m_MappedLength(0)
    {
        m_StartPa.QuadPart = 0;
    };

    ~FxRegisterResourceInfo(){;};

    VOID
    SetPhysicalAddress(
        __in PHYSICAL_ADDRESS StartPa,
        __in SIZE_T Length
        )
    {
        m_StartPa = StartPa;
        m_Length = Length;
    }

    VOID
    SetMappedAddress(
        __in PVOID SystemBaseAddress,
        __in SIZE_T MappedLength,
        __in PVOID UsermodeBaseAddress
        )
    {
        m_StartSystemVa = SystemBaseAddress;
        m_MappedLength = MappedLength;
        m_EndSystemVa = ((PUCHAR) m_StartSystemVa) + MappedLength - 1;
        m_StartUsermodeVa = UsermodeBaseAddress;
    };

    VOID
    ClearMappedAddress(
        VOID
        )
    {
        m_StartSystemVa = NULL;
        m_EndSystemVa = NULL;
        m_StartUsermodeVa = NULL;
        m_MappedLength = 0;
    };

};

struct FxPortResourceInfo : FxStump {
    //
    // start physical address
    //
    PHYSICAL_ADDRESS m_StartPa;

    //
    // end physical address
    //
    PHYSICAL_ADDRESS m_EndPa;

    //
    // Length of resource
    //
    SIZE_T m_Length;

    static
    NTSTATUS
    _CreateAndInit(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _In_ ULONG Count,
        _Out_ FxPortResourceInfo** PortTable
        )
    {
        NTSTATUS status;
        FxPortResourceInfo* table;

        ASSERT(PortTable != NULL);
        *PortTable = NULL;

        table = new (DriverGlobals) FxPortResourceInfo[Count];
        if (table == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Failed to allocate Resource table %!STATUS!", status);
            return status;
        }

        *PortTable = table;
        status = STATUS_SUCCESS;

        return status;
    }

    FxPortResourceInfo(
        VOID
        ):
        m_Length(0)
    {
        m_StartPa.QuadPart = 0;
        m_EndPa.QuadPart = 0;
    };

    ~FxPortResourceInfo(){;};

    VOID
    SetPhysicalAddress(
        __in PHYSICAL_ADDRESS StartPa,
        __in SIZE_T Length
        )
    {
        m_StartPa = StartPa;
        m_EndPa.QuadPart = StartPa.QuadPart + Length - 1;
        m_Length = Length;
    }
};

#endif

//
// Used in FilterResourceRequirements, and QueryResourceRequirements
//

class FxResourceIo : public FxObject {
public:
    IO_RESOURCE_DESCRIPTOR m_Descriptor;

    //
    // Clone of m_Descriptor which is returned to the driver writer when it
    // requests a descriptor pointer.  We clone the descriptor so that if the
    // driver writer attempts to modify the pointer in place, they modify the
    // clone and not the real descriptor.
    //
    IO_RESOURCE_DESCRIPTOR m_DescriptorClone;

public:
    FxResourceIo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PIO_RESOURCE_DESCRIPTOR Resource
        ) : FxObject(FX_TYPE_RESOURCE_IO, 0, FxDriverGlobals)
    {
        RtlCopyMemory(&m_Descriptor, Resource, sizeof(m_Descriptor));
    }

    DECLARE_INTERNAL_NEW_OPERATOR();
};

//
// Used in StartDevice
//
class FxResourceCm : public FxObject {
public:
    CM_PARTIAL_RESOURCE_DESCRIPTOR m_Descriptor;

    //
    // Clone of m_Descriptor which is returned to the driver writer when it
    // requests a descriptor pointer.  We clone the descriptor so that if the
    // driver writer attempts to modify the pointer in place, they modify the
    // clone and not the real descriptor.
    //
    CM_PARTIAL_RESOURCE_DESCRIPTOR m_DescriptorClone;

public:
    FxResourceCm(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource
        ) : FxObject(FX_TYPE_RESOURCE_CM, 0, FxDriverGlobals)
    {
        RtlCopyMemory(&m_Descriptor, Resource, sizeof(m_Descriptor));
    }

    DECLARE_INTERNAL_NEW_OPERATOR();
};

enum FxResourceAccessFlags {
    FxResourceNoAccess      = 0x0000,
    FxResourceAddAllowed    = 0x0001,
    FxResourceRemoveAllowed = 0x0002,
    FxResourceAllAccessAllowed = FxResourceAddAllowed | FxResourceRemoveAllowed,
};

class FxResourceCollection : public FxCollection {
protected:
    FxResourceCollection(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFTYPE Type,
        __in USHORT Size,
        __in UCHAR AccessFlags = FxResourceNoAccess
        ) : FxCollection(FxDriverGlobals, Type, Size),
            m_AccessFlags(AccessFlags), m_Changed(FALSE)
    {
        //
        // Driver cannot delete this or any of its derivations
        //
        MarkNoDeleteDDI();
    }

public:

    BOOLEAN
    RemoveAndDelete(
        __in ULONG Index
        );

    _Must_inspect_result_
    NTSTATUS
    AddAt(
        __in ULONG Index,
        __in FxObject* Object
        );

    BOOLEAN
    IsRemoveAllowed(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_AccessFlags, FxResourceRemoveAllowed);
    }

    BOOLEAN
    IsAddAllowed(
        VOID
        )
    {
        return FLAG_TO_BOOL(m_AccessFlags, FxResourceAddAllowed);
    }

    VOID
    MarkChanged(
        VOID
        )
    {
        m_Changed = TRUE;
    }

    BOOLEAN
    IsChanged(
        VOID
        )
    {
        return m_Changed;
    }

public:
    UCHAR m_AccessFlags;

    BOOLEAN m_Changed;
};

class FxCmResList : public FxResourceCollection {

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
protected:
    //
    // Table of mapped register resources
    //
    FxRegisterResourceInfo* m_RegisterResourceTable;
    ULONG m_RegisterResourceTableSizeCe;

    //
    // Table of port resources
    //
    FxPortResourceInfo* m_PortResourceTable;
    ULONG m_PortResourceTableSizeCe;

    //
    // TRUE if we have at least one CmResourceTypeConnection resource.
    //
    BOOLEAN m_HasConnectionResources;

    //
    // Lock to serialize access to port/register resource table
    //
    MxLock m_ResourceTableLock;

#endif // FX_CORE_USER_MODE

protected:
    FxCmResList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in UCHAR AccessFlags
        ) : FxResourceCollection(FxDriverGlobals,
                                 FX_TYPE_CM_RES_LIST,
                                 sizeof(FxCmResList),
                                 AccessFlags)
    {
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
        m_RegisterResourceTable = NULL;
        m_PortResourceTable = NULL;
        m_RegisterResourceTableSizeCe = 0;
        m_PortResourceTableSizeCe = 0;
        m_HasConnectionResources = FALSE;
#endif // FX_CORE_USER_MODE
    }

    ~FxCmResList();

public:
    static
    _Must_inspect_result_
    NTSTATUS
    _CreateAndInit(
        __in FxCmResList** ResourceList,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice * Device,
        __in_opt PWDF_OBJECT_ATTRIBUTES ListAttributes,
        __in UCHAR AccessFlags
        )
    {
        NTSTATUS ntStatus;
        FxCmResList *resList = NULL;

        UNREFERENCED_PARAMETER(Device);

        //
        // Initialize
        //
        *ResourceList = NULL;

        //
        // Allocate a new resource list object
        //
        resList = new(FxDriverGlobals, ListAttributes)
            FxCmResList(FxDriverGlobals, AccessFlags);

        if (resList == NULL) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR,
                                TRACINGDEVICE,
                                "Failed to allocate FxCmResList, "
                                "returning %!STATUS!",
                                ntStatus);
            goto exit;
        }

        *ResourceList = resList;
        ntStatus = STATUS_SUCCESS;

    exit:
        if (!NT_SUCCESS(ntStatus)) {
            if (NULL != resList) {
                resList->DeleteFromFailedCreate();
            }
        }
        return ntStatus;
    }

    WDFCMRESLIST
    GetHandle(
        VOID
        )
    {
        return (WDFCMRESLIST) GetObjectHandle();
    }

    _Must_inspect_result_
    NTSTATUS
    BuildFromWdmList(
        __in PCM_RESOURCE_LIST ResourceList,
        __in UCHAR AccessFlags
        );

    _Must_inspect_result_
    PCM_RESOURCE_LIST
    CreateWdmList(
        __in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType = PagedPool
        );

    ULONG
    GetCount(
        VOID
        );

    PCM_PARTIAL_RESOURCE_DESCRIPTOR
    GetDescriptor(
        __in ULONG Index
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    //
    // Lock functions used internally
    //
    __inline
    void
#pragma prefast(suppress:__WARNING_UNEXPECTED_IRQL_CHANGE, "UM has no IRQL")
    LockResourceTable(
        )
    {
        KIRQL oldIrql;

        m_ResourceTableLock.Acquire(&oldIrql);

        UNREFERENCED_PARAMETER(oldIrql);
    }

    __inline
    void
#pragma prefast(suppress:__WARNING_UNEXPECTED_IRQL_CHANGE, "UM has no IRQL")
    UnlockResourceTable(
        )
    {
        m_ResourceTableLock.Release(PASSIVE_LEVEL);
    }

    NTSTATUS
    BuildRegisterResourceTable(
        VOID
        );

    NTSTATUS
    BuildPortResourceTable(
        VOID
        );

    VOID
    UpdateRegisterResourceEntryLocked(
        __in FxRegisterResourceInfo* Entry,
        __in PVOID SystemMappedAddress,
        __in SIZE_T NumberOfBytes,
        __in PVOID UsermodeMappedAddress
        );

    VOID
    ClearRegisterResourceEntryLocked(
        __in FxRegisterResourceInfo* Entry
        );

    HRESULT
    ValidateRegisterPhysicalAddressRange (
        __in PHYSICAL_ADDRESS PhysicalAddress,
        __in SIZE_T Size,
        __out FxRegisterResourceInfo** TableEntry
        );

    HRESULT
    ValidateRegisterSystemBaseAddress (
        __in PVOID Address,
        __out PVOID* UsermodeBaseAddress
        );

    HRESULT
    ValidateRegisterSystemAddressRange (
        __in PVOID SystemAddress,
        __in SIZE_T Length,
        __out_opt PVOID* UsermodeAddress
        );

    HRESULT
    ValidateAndClearMapping(
        __in PVOID Address,
        __in SIZE_T Length
        );

    HRESULT
    ValidatePortAddressRange(
        __in PVOID Address,
        __in SIZE_T Length
        );

    SIZE_T
    GetResourceLength(
        __in PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
        __out_opt PHYSICAL_ADDRESS* Start
        );

    HRESULT
    MapIoSpaceWorker(
        __in PHYSICAL_ADDRESS PhysicalAddress,
        __in SIZE_T NumberOfBytes,
        __in MEMORY_CACHING_TYPE  CacheType,
        __deref_out VOID** PseudoBaseAddress
        );

    VOID
    ValidateResourceUnmap(
        VOID
        );

    VOID
    DeleteRegisterResourceTable(
        VOID
        )
    {
        LockResourceTable();
        if (m_RegisterResourceTable != NULL) {
            delete [] m_RegisterResourceTable;
            m_RegisterResourceTable = NULL;
            m_RegisterResourceTableSizeCe = 0;
        }
        UnlockResourceTable();
    }

    VOID
    DeletePortResourceTable(
        VOID
        )
    {
        LockResourceTable();
        if (m_PortResourceTable != NULL) {
            delete [] m_PortResourceTable;
            m_PortResourceTable = NULL;
            m_PortResourceTableSizeCe = 0;
        }
        UnlockResourceTable();
    }

    _Must_inspect_result_
    NTSTATUS
    CheckForConnectionResources(
        VOID
        );

    BOOLEAN
    HasConnectionResources(
        VOID
        )
    {
        return m_HasConnectionResources;
    }

#endif // FX_CORE_USER_MODE

};

class FxIoResReqList : public FxResourceCollection {
protected:
    FxIoResReqList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in UCHAR AccessFlags = FxResourceNoAccess
        ) :
        FxResourceCollection(FxDriverGlobals,
                             FX_TYPE_IO_RES_REQ_LIST,
                             sizeof(FxIoResReqList),
                             AccessFlags),
        m_SlotNumber(0), m_InterfaceType(Internal)
    {
        m_AccessFlags = AccessFlags;
    }

public:

    static
    _Must_inspect_result_
    NTSTATUS
    _CreateAndInit(
        __in FxIoResReqList** ResourceReqList,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES ListAttributes,
        __in UCHAR AccessFlags
        )
    {
        NTSTATUS ntStatus;
        FxIoResReqList *resReqList = NULL;

        //
        // Initialize
        //
        *ResourceReqList = NULL;

        //
        // Allocate a new resource list object
        //
        resReqList = new(FxDriverGlobals, ListAttributes)
            FxIoResReqList(FxDriverGlobals, AccessFlags);
        if (resReqList == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR,
                                TRACINGDEVICE,
                                "Failed to allocate FxIoResReqList, "
                                "returning %!STATUS!",
                                ntStatus);

            goto exit;
        }

        *ResourceReqList = resReqList;
        ntStatus = STATUS_SUCCESS;

        exit:
            if (!NT_SUCCESS(ntStatus)) {
                if (NULL != resReqList) {
                    resReqList->DeleteFromFailedCreate();
                }
            }
            return ntStatus;
    }

    WDFIORESREQLIST
    GetHandle(
        VOID
        )
    {
        return (WDFIORESREQLIST) GetObjectHandle();
    }

    static
    _Must_inspect_result_
    FxIoResReqList*
    _CreateFromWdmList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PIO_RESOURCE_REQUIREMENTS_LIST WdmRequirementsList,
        __in UCHAR AccessFlags
        );

    _Must_inspect_result_
    PIO_RESOURCE_REQUIREMENTS_LIST
    CreateWdmList(
        VOID
        );

public:
    ULONG m_SlotNumber;

    INTERFACE_TYPE m_InterfaceType;
};

class FxIoResList : public FxResourceCollection {
public:
    FxIoResList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxIoResReqList* RequirementsList
        ) :
        FxResourceCollection(FxDriverGlobals, FX_TYPE_IO_RES_LIST, sizeof(FxIoResList)),
        m_OwningList(RequirementsList)
    {
        m_AccessFlags = RequirementsList->m_AccessFlags;
    }

    WDFIORESLIST
    GetHandle(
        VOID
        )
    {
        return (WDFIORESLIST) GetObjectHandle();
    }

    _Must_inspect_result_
    NTSTATUS
    BuildFromWdmList(
        __deref_in PIO_RESOURCE_LIST* WdmResourceList
        );

public:
    FxIoResReqList* m_OwningList;
};

#endif // _FXRESOURCE_H_
