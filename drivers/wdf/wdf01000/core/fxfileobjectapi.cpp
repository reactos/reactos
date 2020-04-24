#include "wdf.h"


extern "C" {

__drv_maxIRQL(PASSIVE_LEVEL)
PUNICODE_STRING
WDFEXPORT(WdfFileObjectGetFileName)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the UNICODE_STRING for the FileName inside
    the WDM fileobject.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    PUNICODE_STRING (file name)

--*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfFileObjectGetFlags)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the flags inside the WDM fileobject.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    ULONG (flags)

--*/

{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfFileObjectGetDevice)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFFILEOBJECT FileObject
   )

/*++

Routine Description:

    This returns the Device that the fileobject is associated with.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:

    WDFDEVICE

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PFILE_OBJECT
WDFEXPORT(WdfFileObjectWdmGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFFILEOBJECT  FileObject
    )
/*++

Routine Description:

    This functions returns the corresponding WDM fileobject. If the device is opened
    by a kernel-mode componenet by sending a IRP_MJ_CREATE irp
    directly without a fileobject, this call can return a NULL pointer.

    Creating a WDFFILEOBJECT without an underlying WDM fileobject
    is done only for 'exclusive' devices.

    Serenum sends such a create-irp to the serial driver.

Arguments:

    FileObject - WDFFILEOBJECT

Return Value:


--*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"
