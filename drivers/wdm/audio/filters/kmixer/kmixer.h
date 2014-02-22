#ifndef _KMIXER_PCH_
#define _KMIXER_PCH_

#include <portcls.h>
#include <float_cast.h>

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;


}KMIXER_DEVICE_EXT, *PKMIXER_DEVICE_EXT;

typedef struct
{
    KSPIN_LOCK Lock;


}SUM_NODE_CONTEXT, *PSUM_NODE_CONTEXT;


NTSTATUS
NTAPI
KMixAllocateDeviceHeader(
    IN PKMIXER_DEVICE_EXT DeviceExtension);

NTSTATUS
CreatePin(
    IN PIRP Irp);

#ifndef _M_IX86
#define KeSaveFloatingPointState(x) ((void)(x), STATUS_SUCCESS)
#define KeRestoreFloatingPointState(x) ((void)0)
#endif

#endif /* _KMIXER_PCH_ */
