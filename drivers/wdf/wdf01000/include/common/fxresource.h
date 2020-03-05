#ifndef _FXRESOURCE_H_
#define _FXRESOURCE_H_

#include "common/fxcollection.h"
#include "common/dbgtrace.h"

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

    ~FxCmResList(){}

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
        if (!NT_SUCCESS(ntStatus))
        {
            if (NULL != resList)
            {
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
        if (resReqList == NULL)
        {
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
            if (!NT_SUCCESS(ntStatus))
            {
                if (NULL != resReqList)
                {
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

#endif //_FXRESOURCE_H_
