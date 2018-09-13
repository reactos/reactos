/*++

Copyright (c) 1998-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    csrlocal.c

Abstract:

    This module implements functions that are used by the functions in locale.c
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "nls.h"
#include "ntwow64n.h"

NTSTATUS
CsrBasepNlsSetUserInfo(
    IN LCTYPE   LCType,
    IN LPWSTR pData,
    IN ULONG DataLength
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepNlsSetUserInfo(LCType,
                                         pData,
                                         DataLength);
#else

    BASE_API_MSG m;
    PBASE_NLS_SET_USER_INFO_MSG a = &m.u.NlsSetUserInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    //
    //  Get the capture buffer for the strings.
    //
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              DataLength );

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR)pData,
                             DataLength,
                             (PVOID *)&a->pData );

    //
    //  Save the pointer to the cache string.
    //
    a->LCType = LCType;

    //
    //  Save the length of the data in the msg structure.
    //
    a->DataLength = DataLength;

    //
    //  Call the server to set the registry value.
    //
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                             BasepNlsSetUserInfo),
                         sizeof(*a) );

    //
    //  Free the capture buffer.
    //
    if (CaptureBuffer != NULL)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return m.ReturnValue;
#endif

}

NTSTATUS
CsrBasepNlsSetMultipleUserInfo(
    IN DWORD dwFlags,
    IN int cchData,
    IN LPCWSTR pPicture,
    IN LPCWSTR pSeparator,
    IN LPCWSTR pOrder,
    IN LPCWSTR pTLZero,
    IN LPCWSTR pTimeMarkPosn
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepNlsSetMultipleUserInfo(dwFlags,
                                                 cchData,
                                                 pPicture,
                                                 pSeparator,
                                                 pOrder,
                                                 pTLZero,
                                                 pTimeMarkPosn);
#else

    ULONG CaptureLength;          // length of capture buffer
    ULONG Length;                 // temp storage for length of string

    BASE_API_MSG m;
    PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG a = &m.u.NlsSetMultipleUserInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    //
    //  Initialize the msg structure to NULL.
    //
    RtlZeroMemory(a, sizeof(BASE_NLS_SET_MULTIPLE_USER_INFO_MSG));

    //
    //  Save the flags and the length of the data in the msg structure.
    //
    a->Flags = dwFlags;
    a->DataLength = cchData * sizeof(WCHAR);

    //
    //  Save the appropriate strings in the msg structure.
    //
    switch (dwFlags)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pSeparator) + 1;
            CaptureLength = (cchData + Length + 2 + 2 + 2) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 5,
                                                      CaptureLength );
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pPicture,
                                     cchData * sizeof(WCHAR),
                                     (PVOID *)&a->pPicture );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pSeparator,
                                     Length * sizeof(WCHAR),
                                     (PVOID *)&a->pSeparator );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pOrder,
                                     2 * sizeof(WCHAR),
                                     (PVOID *)&a->pOrder );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pTLZero,
                                     2 * sizeof(WCHAR),
                                     (PVOID *)&a->pTLZero );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pTimeMarkPosn,
                                     2 * sizeof(WCHAR),
                                     (PVOID *)&a->pTimeMarkPosn );

            break;
        }
        case ( LOCALE_STIME ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pPicture,
                                     Length * sizeof(WCHAR),
                                     (PVOID *)&a->pPicture );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pSeparator,
                                     cchData * sizeof(WCHAR),
                                     (PVOID *)&a->pSeparator );

            break;
        }
        case ( LOCALE_ITIME ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pPicture,
                                     Length * sizeof(WCHAR),
                                     (PVOID *)&a->pPicture );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pOrder,
                                     cchData * sizeof(WCHAR),
                                     (PVOID *)&a->pOrder );

            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pSeparator) + 1;
            CaptureLength = (cchData + Length + 2) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 3,
                                                      CaptureLength );
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pPicture,
                                     cchData * sizeof(WCHAR),
                                     (PVOID *)&a->pPicture );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pSeparator,
                                     Length * sizeof(WCHAR),
                                     (PVOID *)&a->pSeparator );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pOrder,
                                     2 * sizeof(WCHAR),
                                     (PVOID *)&a->pOrder );

            break;
        }
        case ( LOCALE_SDATE ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pPicture,
                                     Length * sizeof(WCHAR),
                                     (PVOID *)&a->pPicture );

            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR)pSeparator,
                                     cchData * sizeof(WCHAR),
                                     (PVOID *)&a->pSeparator );

            break;
        }
    }

    //
    //  Call the server to set the registry values.
    //
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepNlsSetMultipleUserInfo ),
                         sizeof(*a) );

    //
    //  Free the capture buffer.
    //
    if (CaptureBuffer != NULL)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return m.ReturnValue;

#endif

}


NTSTATUS
CsrBasepNlsUpdateCacheCount(
    VOID
    )
{


    BASE_API_MSG m;
    PBASE_NLS_UPDATE_CACHE_COUNT_MSG a = &m.u.NlsCacheUpdateCount;

    a->Reserved = 0L;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                             BasepNlsUpdateCacheCount),
                         sizeof(*a) );

    return (m.ReturnValue);
}
