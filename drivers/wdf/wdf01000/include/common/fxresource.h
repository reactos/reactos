#ifndef _FXRESOURCE_H_
#define _FXRESOURCE_H_

#include "common/fxcollection.h"
#include "common/dbgtrace.h"


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

#endif //_FXRESOURCE_H_
