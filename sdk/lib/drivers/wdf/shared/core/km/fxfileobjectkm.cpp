/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxFileObjectKm.hpp

Abstract:

    This module implements a frameworks managed FileObject

Author:



Environment:

    Kernel mode only

Revision History:


--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxFileObjectKm.tmh"
}

VOID
FxFileObject::SetFileObjectContext(
    _In_ MdFileObject WdmFileObject,
    _In_ WDF_FILEOBJECT_CLASS NormalizedFileClass,
    _In_ MdIrp Irp,
    _In_ FxDevice* Device
    )
{
    MxFileObject wdmFileObject(WdmFileObject);

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Device);

    //
    // We now must be able to quickly retrieve the FxFileObject*
    // from the WDM PFILE_OBJECT when we get I/O requests.
    //
    // But different driver stacks have different rules, so
    // we must be flexible here, including not touching the
    // WDM PFILE_OBJECT at all.
    //
    if( NormalizedFileClass == WdfFileObjectWdfCanUseFsContext ) {
        wdmFileObject.SetFsContext(this);
    }
    else if( NormalizedFileClass == WdfFileObjectWdfCanUseFsContext2 ) {
        wdmFileObject.SetFsContext2(this);
    }
    else {
        //
        // WdfDeviceFileObjectNoContext will look up the FxFileObject
        // from the FxDevice->m_FileObjectListHead at runtime.
        //
    }
}

VOID
FxFileObject::Initialize(
    _In_ MdIrp CreateIrp
    )
{
    UNREFERENCED_PARAMETER(CreateIrp);

    //
    // Not needed for KMDF.
    //
    DO_NOTHING();

    return;
}

_Must_inspect_result_
NTSTATUS
FxFileObject::UpdateProcessKeepAliveCount(
    _In_ BOOLEAN Increment
    )
{
    UNREFERENCED_PARAMETER(Increment);

    //
    // Not needed for KMDF.
    //
    DO_NOTHING();

    return STATUS_NOT_IMPLEMENTED;
}
