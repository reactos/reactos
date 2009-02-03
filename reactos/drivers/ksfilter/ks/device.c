#include <ntddk.h>
#include <debug.h>
#include <ks.h>

#include "ksiface.h"
#include "ksfunc.h"

typedef struct
{
    IKsDeviceVtbl *lpVtbl;

    LONG ref;


}IKsDeviceImpl;


NTSTATUS
NTAPI
NewIKsDevice(IKsDevice** OutDevice)
{
    IKsDeviceImpl * This;

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IKsDeviceImpl), 0x12345678); //FIX TAG
    if (!This)
       return STATUS_INSUFFICIENT_RESOURCES;

    This->ref = 1;
    //This->lpVtbl = &vt_IKsDevice;

    *OutDevice = (IKsDevice*)This;
    return STATUS_SUCCESS;
}

