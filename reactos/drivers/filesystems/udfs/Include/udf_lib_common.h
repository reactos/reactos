////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __UDF_LIB_COMMON__H__
#define __UDF_LIB_COMMON__H__

#ifndef WITHOUT_FORMATTER
#include "udferr_usr.h"
#endif

typedef LONG   UDF_STATUS;

#define UDF_SUCCESS(x) ( (UDF_STATUS)(x)>=0 )

typedef UDF_STATUS (*PREAD_FUNCTION)(
    PVOID               lpParameter,
    PVOID               lpBuffer,
    ULONG               nLength,
    LONGLONG            liOffset,
    PULONG              lpNumberOfBytesRead
);

typedef UDF_STATUS (*PWRITE_FUNCTION)(
    PVOID               lpParameter,
    PVOID               lpBuffer,
    ULONG               nLength,
    LONGLONG            liOffset,
    PULONG              lpNumberOfBytesRead
);

typedef UDF_STATUS (*PIOCTL_FUNCTION)(
    PVOID               lpParameter,
    DWORD               dwIoControlCode,
    LPVOID              lpInBuffer,
    DWORD               nInBufferSize,
    LPVOID              lpOutBuffer,
    DWORD               nOutBufferSize,
    LPDWORD             lpBytesReturned
);

// For formatter

typedef UDF_STATUS (*PREOPEN_FUNCTION)(
    PVOID*              lpParameter
);

typedef UDF_STATUS (*PGETSIZE_FUNCTION)(
    PVOID               lpParameter,
    __int64*            size,
    ULONG*              block_size
);

typedef UDF_STATUS (*PFLUSH_FUNCTION)(
    PVOID               lpParameter
);

#endif //__UDF_LIB_COMMON__H__
