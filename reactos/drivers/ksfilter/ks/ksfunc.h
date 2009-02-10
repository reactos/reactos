#ifndef KSFUNC_H__
#define KSFUNC_H__

#include "ksiface.h"

NTSTATUS
NTAPI
NewIKsDevice(IKsDevice** OutDevice);

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_KSDEVICE TAG('K', 'S', 'E', 'D')
#define TAG_KSOBJECT_TAG TAG('K', 'S', 'O', 'H')





#endif
