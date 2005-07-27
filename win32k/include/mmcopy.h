#ifndef NDK_MMCOPY_H
#define NDK_MMCOPY_H

#include <pseh/pseh.h>

NTSTATUS _MmCopyFromCaller( PVOID Target, PVOID Source, UINT Bytes );

#define MmCopyFromCaller(x,y,z) _MmCopyFromCaller((PCHAR)(x),(PCHAR)(y),(UINT)(z))
#define MmCopyToCaller(x,y,z) MmCopyFromCaller(x,y,z)

#endif/*NDK_MMCOPY_H*/
