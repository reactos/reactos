#ifndef NDK_MMCOPY_H
#define NDK_MMCOPY_H

#include <pseh/pseh2.h>

NTSTATUS _MmCopyFromCaller( PVOID Target, PVOID Source, UINT Bytes );
NTSTATUS _MmCopyToCaller( PVOID Target, PVOID Source, UINT Bytes );

#define MmCopyFromCaller(x,y,z) _MmCopyFromCaller((PCHAR)(x),(PCHAR)(y),(UINT)(z))
#define MmCopyToCaller(x,y,z) _MmCopyToCaller((PCHAR)(x),(PCHAR)(y),(UINT)(z))

#endif/*NDK_MMCOPY_H*/
