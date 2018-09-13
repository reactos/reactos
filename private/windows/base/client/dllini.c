/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllprof.c

Abstract:

    This module contains the client side of the Win32 Initialization
    File APIs

Author:

    Steve Wood (stevewo) 24-Sep-1990

Revision History:

--*/

#include "basedll.h"

#if DBG
BOOLEAN BaseDllDumpIniCalls;
#endif

PINIFILE_MAPPING BaseDllIniFileMapping;
PINIFILE_CACHE BaseDllIniFileCache;
UNICODE_STRING BaseDllIniUserKeyPath;
UNICODE_STRING BaseDllIniSoftwareKeyPath;
ULONG LockFileKey = 1;

struct {
    PINIFILE_MAPPING_TARGET MappingTarget;
    ULONG MappingFlags;
    BOOLEAN WriteAccess;
    UNICODE_STRING RegistryPath;
    HANDLE RegistryKey;
    CRITICAL_SECTION Lock;
} BaseDllRegistryCache;


NTSTATUS
BaseDllInitializeIniFileMappings(
                                PBASE_STATIC_SERVER_DATA StaticServerData
                                )
{

    BaseDllIniFileMapping = (PINIFILE_MAPPING)StaticServerData->IniFileMapping;
    BaseDllIniFileCache = NULL;

#if DBG

    BaseDllDumpIniCalls = FALSE;

#endif

    // BaseDllDumpIniCalls = TRUE;
    RtlInitUnicodeString( &BaseDllIniUserKeyPath, NULL );
    RtlInitUnicodeString( &BaseDllIniSoftwareKeyPath, L"\\Registry\\Machine\\Software" );

    RtlZeroMemory( &BaseDllRegistryCache, sizeof( BaseDllRegistryCache ) );
    BaseDllRegistryCache.RegistryKey = INVALID_HANDLE_VALUE;
    InitializeCriticalSection(&BaseDllRegistryCache.Lock);
    return STATUS_SUCCESS;
}

NTSTATUS
BaseDllReadWriteIniFile(
                       IN BOOLEAN Unicode,
                       IN BOOLEAN WriteOperation,
                       IN BOOLEAN SectionOperation,
                       IN PVOID FileName OPTIONAL,
                       IN PVOID ApplicationName OPTIONAL,
                       IN PVOID VariableName OPTIONAL,
                       IN OUT PVOID VariableValue OPTIONAL,
                       IN OUT PULONG VariableValueLength OPTIONAL
                       );

DWORD
WINAPI
GetPrivateProfileSectionNamesA(
                              LPSTR lpszReturnBuffer,
                              DWORD nSize,
                              LPCSTR lpFileName
                              )
{
    return GetPrivateProfileStringA( NULL,
                                     NULL,
                                     NULL,
                                     lpszReturnBuffer,
                                     nSize,
                                     lpFileName
                                   );
}

DWORD
WINAPI
GetPrivateProfileSectionNamesW(
                              LPWSTR lpszReturnBuffer,
                              DWORD nSize,
                              LPCWSTR lpFileName
                              )
{
    return GetPrivateProfileStringW( NULL,
                                     NULL,
                                     NULL,
                                     lpszReturnBuffer,
                                     nSize,
                                     lpFileName
                                   );
}

#define NibbleToChar(x) (N2C[x])
#define CharToNibble(x) ((x)>='0'&&(x)<='9' ? (x)-'0' : ((10+(x)-'A')&0x000f))
char N2C[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

BOOL
WINAPI
GetPrivateProfileStructA(
                        LPCSTR lpszSection,
                        LPCSTR lpszKey,
                        LPVOID lpStruct,
                        UINT uSizeStruct,
                        LPCSTR szFile
                        )
{
    UCHAR szBuf[256];
    LPSTR lpBuf, lpBufTemp, lpFreeBuffer;
    UINT nLen;
    BYTE checksum;
    BOOL Result;

    nLen = uSizeStruct*2 + 10;
    if (nLen < uSizeStruct) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (nLen > sizeof( szBuf )) {
        lpFreeBuffer = (LPSTR)RtlAllocateHeap( RtlProcessHeap(),
                                               0,
                                               nLen
                                             );
        if (!lpFreeBuffer) {
            return FALSE;
        }
        lpBuf = lpFreeBuffer;
    } else {
        lpFreeBuffer = NULL;
        lpBuf = (LPSTR)szBuf;
    }

    Result = FALSE;
    nLen = GetPrivateProfileStringA( lpszSection,
                                     lpszKey,
                                     NULL,
                                     lpBuf,
                                     nLen,
                                     szFile
                                   );

    if (nLen == uSizeStruct*2+2) {
        /* Room for the one byte check sum */
        uSizeStruct+=1;
        checksum = 0;
        for (lpBufTemp=lpBuf; uSizeStruct!=0; --uSizeStruct) {
            BYTE bStruct;
            BYTE cTemp;

            cTemp = *lpBufTemp++;
            bStruct = (BYTE)CharToNibble(cTemp);
            cTemp = *lpBufTemp++;
            bStruct = (BYTE)((bStruct<<4) | CharToNibble(cTemp));

            if (uSizeStruct == 1) {
                if (checksum == bStruct) {
                    Result = TRUE;
                } else {
                    SetLastError( ERROR_INVALID_DATA );
                }

                break;
            }

            checksum += bStruct;
            *((LPBYTE)lpStruct)++ = bStruct;
        }
    } else {
        SetLastError( ERROR_BAD_LENGTH );
    }

    if (lpFreeBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, lpFreeBuffer );
    }

    return Result;
}

BOOL
WINAPI
GetPrivateProfileStructW(
                        LPCWSTR lpszSection,
                        LPCWSTR lpszKey,
                        LPVOID   lpStruct,
                        UINT     uSizeStruct,
                        LPCWSTR szFile
                        )
{
    WCHAR szBuf[256];
    PWSTR lpBuf, lpBufTemp, lpFreeBuffer;
    UINT nLen;
    BYTE checksum;
    BOOL Result;

    nLen = uSizeStruct*2 + 10;
    if (nLen < uSizeStruct) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ((nLen * sizeof( WCHAR )) > sizeof( szBuf )) {
        lpFreeBuffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(),
                                               0,
                                               nLen * sizeof( WCHAR )
                                             );
        if (!lpFreeBuffer) {
            return FALSE;
        }
        lpBuf = lpFreeBuffer;
    } else {
        lpFreeBuffer = NULL;
        lpBuf = (PWSTR)szBuf;
    }

    Result = FALSE;
    nLen = GetPrivateProfileStringW( lpszSection,
                                     lpszKey,
                                     NULL,
                                     lpBuf,
                                     nLen,
                                     szFile
                                   );

    if (nLen == uSizeStruct*2+2) {
        /* Room for the one byte check sum */
        uSizeStruct+=1;
        checksum = 0;
        for (lpBufTemp=lpBuf; uSizeStruct!=0; --uSizeStruct) {
            BYTE bStruct;
            WCHAR cTemp;

            cTemp = *lpBufTemp++;
            bStruct = (BYTE)CharToNibble(cTemp);
            cTemp = *lpBufTemp++;
            bStruct = (BYTE)((bStruct<<4) | CharToNibble(cTemp));

            if (uSizeStruct == 1) {
                if (checksum == bStruct) {
                    Result = TRUE;
                } else {
                    SetLastError( ERROR_INVALID_DATA );
                }

                break;
            }

            checksum += bStruct;
            *((LPBYTE)lpStruct)++ = bStruct;
        }
    } else {
        SetLastError( ERROR_BAD_LENGTH );
    }

    if (lpFreeBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, lpFreeBuffer );
    }

    return Result;
}

BOOL
WINAPI
WritePrivateProfileStructA(
                          LPCSTR lpszSection,
                          LPCSTR lpszKey,
                          LPVOID   lpStruct,
                          UINT     uSizeStruct,
                          LPCSTR szFile
                          )
{
    UCHAR szBuf[256];
    LPSTR lpBuf, lpBufTemp, lpFreeBuffer;
    UINT nLen;
    BOOL Result;
    BYTE checksum;

    if (lpStruct == NULL) {
        return WritePrivateProfileStringA( lpszSection,
                                           lpszKey,
                                           NULL,
                                           szFile
                                         );
    }


    nLen = uSizeStruct*2 + 3;
    if (nLen < uSizeStruct) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (nLen > sizeof( szBuf )) {
        lpFreeBuffer = (LPSTR)RtlAllocateHeap( RtlProcessHeap(),
                                               0,
                                               nLen
                                             );
        if (!lpFreeBuffer) {
            return FALSE;
        }
        lpBuf = lpFreeBuffer;
    } else {
        lpFreeBuffer = NULL;
        lpBuf = (LPSTR)szBuf;
    }

    checksum = 0;
    for (lpBufTemp=lpBuf; uSizeStruct != 0; --uSizeStruct) {
        BYTE bStruct;

        bStruct = *((LPBYTE)lpStruct)++;
        checksum = checksum + bStruct;

        *lpBufTemp++ = NibbleToChar((bStruct>>4)&0x000f);
        *lpBufTemp++ = NibbleToChar(bStruct&0x000f);
    }
    *lpBufTemp++ = NibbleToChar((checksum>>4)&0x000f);
    *lpBufTemp++ = NibbleToChar(checksum&0x000f);
    *lpBufTemp = '\0';

    Result = WritePrivateProfileStringA( lpszSection,
                                         lpszKey,
                                         lpBuf,
                                         szFile
                                       );

    if (lpFreeBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, lpFreeBuffer );
    }

    return Result;
}

BOOL
WINAPI
WritePrivateProfileStructW(
                          LPCWSTR lpszSection,
                          LPCWSTR lpszKey,
                          LPVOID   lpStruct,
                          UINT     uSizeStruct,
                          LPCWSTR szFile
                          )
{
    WCHAR szBuf[256];
    PWSTR lpBuf, lpBufTemp, lpFreeBuffer;
    UINT nLen;
    BOOL Result;
    BYTE checksum;

    if (lpStruct == NULL) {
        return WritePrivateProfileStringW( lpszSection,
                                           lpszKey,
                                           NULL,
                                           szFile
                                         );
    }


    nLen = uSizeStruct*2 + 3;
    if (nLen < uSizeStruct) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ((nLen * sizeof( WCHAR )) > sizeof( szBuf )) {
        lpFreeBuffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(),
                                               0,
                                               nLen * sizeof( WCHAR )
                                             );
        if (!lpFreeBuffer) {
            return FALSE;
        }
        lpBuf = lpFreeBuffer;
    } else {
        lpFreeBuffer = NULL;
        lpBuf = (PWSTR)szBuf;
    }

    checksum = 0;
    for (lpBufTemp=lpBuf; uSizeStruct != 0; --uSizeStruct) {
        BYTE bStruct;

        bStruct = *((LPBYTE)lpStruct)++;
        checksum = checksum + bStruct;

        *lpBufTemp++ = (WCHAR)NibbleToChar((bStruct>>4)&0x000f);
        *lpBufTemp++ = (WCHAR)NibbleToChar(bStruct&0x000f);
    }
    *lpBufTemp++ = (WCHAR)NibbleToChar((checksum>>4)&0x000f);
    *lpBufTemp++ = (WCHAR)NibbleToChar(checksum&0x000f);
    *lpBufTemp = L'\0';

    Result = WritePrivateProfileStringW( lpszSection,
                                         lpszKey,
                                         lpBuf,
                                         szFile
                                       );

    if (lpFreeBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, lpFreeBuffer );
    }

    return Result;
}

UINT
GetProfileIntA(
              LPCSTR lpAppName,
              LPCSTR lpKeyName,
              INT nDefault
              )
{
    return( GetPrivateProfileIntA( lpAppName,
                                   lpKeyName,
                                   nDefault,
                                   NULL
                                 )
          );
}

DWORD
GetProfileStringA(
                 LPCSTR lpAppName,
                 LPCSTR lpKeyName,
                 LPCSTR lpDefault,
                 LPSTR lpReturnedString,
                 DWORD nSize
                 )
{
    return( GetPrivateProfileStringA( lpAppName,
                                      lpKeyName,
                                      lpDefault,
                                      lpReturnedString,
                                      nSize,
                                      NULL
                                    )
          );
}

BOOL
WriteProfileStringA(
                   LPCSTR lpAppName,
                   LPCSTR lpKeyName,
                   LPCSTR lpString
                   )
{
    return( WritePrivateProfileStringA( lpAppName,
                                        lpKeyName,
                                        lpString,
                                        NULL
                                      )
          );
}

DWORD
GetProfileSectionA(
                  LPCSTR lpAppName,
                  LPSTR lpReturnedString,
                  DWORD nSize
                  )
{
    return( GetPrivateProfileSectionA( lpAppName,
                                       lpReturnedString,
                                       nSize,
                                       NULL
                                     )
          );
}

BOOL
WriteProfileSectionA(
                    LPCSTR lpAppName,
                    LPCSTR lpString
                    )
{
    return( WritePrivateProfileSectionA( lpAppName,
                                         lpString,
                                         NULL
                                       )
          );
}

UINT
APIENTRY
GetProfileIntW(
              LPCWSTR lpAppName,
              LPCWSTR lpKeyName,
              INT nDefault
              )
{
    return( GetPrivateProfileIntW( lpAppName,
                                   lpKeyName,
                                   nDefault,
                                   NULL
                                 )
          );
}

DWORD
APIENTRY
GetProfileStringW(
                 LPCWSTR lpAppName,
                 LPCWSTR lpKeyName,
                 LPCWSTR lpDefault,
                 LPWSTR lpReturnedString,
                 DWORD nSize
                 )
{
    return( GetPrivateProfileStringW( lpAppName,
                                      lpKeyName,
                                      lpDefault,
                                      lpReturnedString,
                                      nSize,
                                      NULL
                                    )
          );
}

BOOL
APIENTRY
WriteProfileStringW(
                   LPCWSTR lpAppName,
                   LPCWSTR lpKeyName,
                   LPCWSTR lpString
                   )
{
    return( WritePrivateProfileStringW( lpAppName,
                                        lpKeyName,
                                        lpString,
                                        NULL
                                      )
          );
}

DWORD
APIENTRY
GetProfileSectionW(
                  LPCWSTR lpAppName,
                  LPWSTR lpReturnedString,
                  DWORD nSize
                  )
{
    return( GetPrivateProfileSectionW( lpAppName,
                                       lpReturnedString,
                                       nSize,
                                       NULL
                                     )
          );
}

BOOL
APIENTRY
WriteProfileSectionW(
                    LPCWSTR lpAppName,
                    LPCWSTR lpString
                    )
{
    return( WritePrivateProfileSectionW( lpAppName,
                                         lpString,
                                         NULL
                                       )
          );
}


UINT
GetPrivateProfileIntA(
                     LPCSTR lpAppName,
                     LPCSTR lpKeyName,
                     INT nDefault,
                     LPCSTR lpFileName
                     )
{
    NTSTATUS Status;
    ULONG ReturnValue;
    UCHAR ValueBuffer[ 256 ];
    ULONG cb;

    ReturnValue = 0;
    cb = GetPrivateProfileStringA( lpAppName,
                                   lpKeyName,
                                   NULL,
                                   ValueBuffer,
                                   sizeof( ValueBuffer ),
                                   lpFileName
                                 );
    if (cb == 0) {
        ReturnValue = nDefault;
    } else {
        Status = RtlCharToInteger( ValueBuffer, 0, &ReturnValue );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
        } else {
            SetLastError( NO_ERROR );
        }
    }

    return ReturnValue;
}

DWORD
GetPrivateProfileStringA(
                        LPCSTR lpAppName,
                        LPCSTR lpKeyName,
                        LPCSTR lpDefault,
                        LPSTR lpReturnedString,
                        DWORD nSize,
                        LPCSTR lpFileName
                        )
{
    NTSTATUS Status;
    ULONG n;

    if (lpDefault == NULL) {
        lpDefault = "";
    }

    n = nSize;
    Status = BaseDllReadWriteIniFile( FALSE,    // Unicode,
                                      FALSE,    // WriteOperation
                                      FALSE,    // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      (PVOID)lpKeyName,
                                      (PVOID)lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (!lpAppName || !lpKeyName) {
            if (nSize >= 2) {
                n = nSize - 2;
                lpReturnedString[ n+1 ] = '\0';
                //
                // GetPrivateProfileString(): don't leave 1st byte of double byte char alone
                //
                lpReturnedString[ n ] = '\0';
                if ( n > 0 ) {
                    LPSTR pc = lpReturnedString;
                    LPSTR pcEnd = lpReturnedString + n - 1;
                    //
                    // if the last character is the 1st byte of
                    // double byte character, erase it.
                    //
                    while ( pc <= pcEnd ) {
                        pc += IsDBCSLeadByte( *pc ) ? 2 : 1;
                    }
                    if ( (pc - pcEnd ) == 2 ) {
                        *pcEnd = '\0';
                    }
                }

                return ( n );
            } else {
                n = 0;
            }
        } else {
            if (nSize >= 1) {
                n = nSize - 1;
            } else {
                n = 0;
            }
        }
    } else {
        n = strlen( lpDefault );
        while (n > 0 && lpDefault[n-1] == ' ') {
            n -= 1;
        }

        if (n >= nSize) {
            n = nSize;
        }

        strncpy( lpReturnedString, lpDefault, n );
    }

    if (n < nSize) {
        lpReturnedString[ n ] = '\0';
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = '\0';
    }

    return( n );
}


BOOL
WritePrivateProfileStringA(
                          LPCSTR lpAppName,
                          LPCSTR lpKeyName,
                          LPCSTR lpString,
                          LPCSTR lpFileName
                          )
{
    NTSTATUS Status;

    Status = BaseDllReadWriteIniFile( FALSE,    // Unicode,
                                      TRUE,     // WriteOperation
                                      FALSE,    // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      (PVOID)lpKeyName,
                                      (PVOID)(lpKeyName == NULL ? NULL : lpString),
                                      NULL
                                    );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        return( FALSE );
    }
}

DWORD
GetPrivateProfileSectionA(
                         LPCSTR lpAppName,
                         LPSTR lpReturnedString,
                         DWORD nSize,
                         LPCSTR lpFileName
                         )
{
    NTSTATUS Status;
    ULONG n;

    n = nSize;
    Status = BaseDllReadWriteIniFile( FALSE,    // Unicode,
                                      FALSE,    // WriteOperation
                                      TRUE,     // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      NULL,
                                      (PVOID)lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (nSize >= 2) {
            n = nSize - 2;
            lpReturnedString[ n+1 ] = '\0';
        } else {
            n = 0;
        }
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        n = 0;
    }

    if (n < nSize) {
        lpReturnedString[ n ] = '\0';
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = '\0';
    }

    return( n );
}

BOOL
WritePrivateProfileSectionA(
                           LPCSTR lpAppName,
                           LPCSTR lpString,
                           LPCSTR lpFileName
                           )
{
    NTSTATUS Status;

    Status = BaseDllReadWriteIniFile( FALSE,    // Unicode,
                                      TRUE,     // WriteOperation
                                      TRUE,     // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      NULL,
                                      (PVOID)lpString,
                                      NULL
                                    );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        return( FALSE );
    }
}


UINT
APIENTRY
GetPrivateProfileIntW(
                     LPCWSTR lpAppName,
                     LPCWSTR lpKeyName,
                     INT nDefault,
                     LPCWSTR lpFileName
                     )
{
    NTSTATUS Status;
    ULONG ReturnValue;
    WCHAR ValueBuffer[ 256 ];
    UNICODE_STRING Value;
    ANSI_STRING AnsiString;
    ULONG cb;

    ReturnValue = 0;
    cb = GetPrivateProfileStringW( lpAppName,
                                   lpKeyName,
                                   NULL,
                                   ValueBuffer,
                                   sizeof( ValueBuffer ) / sizeof( WCHAR ),
                                   lpFileName
                                 );
    if (cb == 0) {
        ReturnValue = nDefault;
    } else {
        Value.Buffer = ValueBuffer;
        Value.Length = (USHORT)(cb * sizeof( WCHAR ));
        Value.MaximumLength = (USHORT)((cb + 1) * sizeof( WCHAR ));
        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &Value,
                                               TRUE
                                             );
        if (NT_SUCCESS( Status )) {
            Status = RtlCharToInteger( AnsiString.Buffer, 0, &ReturnValue );
            RtlFreeAnsiString( &AnsiString );
        }

        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
        } else {
            SetLastError( NO_ERROR );
        }
    }

    return ReturnValue;
}

DWORD
APIENTRY
GetPrivateProfileStringW(
                        LPCWSTR lpAppName,
                        LPCWSTR lpKeyName,
                        LPCWSTR lpDefault,
                        LPWSTR lpReturnedString,
                        DWORD nSize,
                        LPCWSTR lpFileName
                        )
{
    NTSTATUS Status;
    ULONG n;

    if (lpDefault == NULL) {
        lpDefault = L"";
    }

    n = nSize;
    Status = BaseDllReadWriteIniFile( TRUE,     // Unicode,
                                      FALSE,    // WriteOperation
                                      FALSE,    // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      (PVOID)lpKeyName,
                                      (PVOID)lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (!lpAppName || !lpKeyName) {
            if (nSize >= 2) {
                n = nSize - 2;
                lpReturnedString[ n+1 ] = UNICODE_NULL;
            } else {
                n = 0;
            }
        } else {
            if (nSize >= 1) {
                n = nSize - 1;
            } else {
                n = 0;
            }
        }
    } else {
        n = wcslen( lpDefault );
        while (n > 0 && lpDefault[n-1] == L' ') {
            n -= 1;
        }

        if (n >= nSize) {
            n = nSize;
        }

        wcsncpy( lpReturnedString, lpDefault, n );
    }

    if (n < nSize) {
        lpReturnedString[ n ] = UNICODE_NULL;
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = UNICODE_NULL;
    }

    return( n );
}

BOOL
APIENTRY
WritePrivateProfileStringW(
                          LPCWSTR lpAppName,
                          LPCWSTR lpKeyName,
                          LPCWSTR lpString,
                          LPCWSTR lpFileName
                          )
{
    NTSTATUS Status;

    Status = BaseDllReadWriteIniFile( TRUE,     // Unicode,
                                      TRUE,     // WriteOperation
                                      FALSE,    // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      (PVOID)lpKeyName,
                                      (PVOID)(lpKeyName == NULL ? NULL : lpString),
                                      NULL
                                    );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        return( FALSE );
    }
}

DWORD
APIENTRY
GetPrivateProfileSectionW(
                         LPCWSTR lpAppName,
                         LPWSTR lpReturnedString,
                         DWORD nSize,
                         LPCWSTR lpFileName
                         )
{
    NTSTATUS Status;
    ULONG n;

    n = nSize;
    Status = BaseDllReadWriteIniFile( TRUE,     // Unicode,
                                      FALSE,    // WriteOperation
                                      TRUE,     // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      NULL,
                                      (PVOID)lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (nSize >= 2) {
            n = nSize - 2;
            lpReturnedString[ n+1 ] = UNICODE_NULL;
        } else {
            n = 0;
        }
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        n = 0;
    }

    if (n < nSize) {
        lpReturnedString[ n ] = UNICODE_NULL;
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = UNICODE_NULL;
    }

    return( n );
}


BOOL
APIENTRY
WritePrivateProfileSectionW(
                           LPCWSTR lpAppName,
                           LPCWSTR lpString,
                           LPCWSTR lpFileName
                           )
{
    NTSTATUS Status;

    Status = BaseDllReadWriteIniFile( TRUE,     // Unicode,
                                      TRUE,     // WriteOperation
                                      TRUE,     // SectionOperation
                                      (PVOID)lpFileName,
                                      (PVOID)lpAppName,
                                      NULL,
                                      (PVOID)lpString,
                                      NULL
                                    );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            BaseSetLastNTError( Status );
        }
        return( FALSE );
    }
}


VOID
BaseDllFlushRegistryCache( VOID );

BOOL
CloseProfileUserMapping( VOID )
{
    BaseDllFlushRegistryCache();
    if (BaseDllIniUserKeyPath.Buffer != NULL) {
        RtlFreeUnicodeString( &BaseDllIniUserKeyPath );
        RtlInitUnicodeString( &BaseDllIniUserKeyPath, NULL );
        return TRUE;
    } else {
        return TRUE;
    }
}


BOOL
OpenProfileUserMapping( VOID )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;

    BaseDllFlushRegistryCache();
    if (BaseDllIniUserKeyPath.Length == 0) {
        Status = RtlFormatCurrentUserKeyPath( &BaseDllIniUserKeyPath );
        if (NT_SUCCESS( Status )) {
            InitializeObjectAttributes( &ObjectAttributes,
                                        &BaseDllIniUserKeyPath,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL
                                      );
            Status = NtOpenKey( &Key,
                                GENERIC_READ,
                                &ObjectAttributes
                              );

            if (NT_SUCCESS( Status )) {
                NtClose( Key );
            } else {
                RtlFreeUnicodeString( &BaseDllIniUserKeyPath );
                RtlInitUnicodeString( &BaseDllIniUserKeyPath, NULL );
            }
        }

        if (!NT_SUCCESS( Status )) {
            if (!RtlCreateUnicodeString( &BaseDllIniUserKeyPath, L"\\REGISTRY\\USER\\.DEFAULT" )) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

char *xOperationNames[] = {
    "FlushProfiles",
    "ReadKeyValue",
    "WriteKeyValue",
    "DeleteKey",
    "ReadKeyNames",
    "ReadSectionNames",
    "ReadSection",
    "WriteSection",
    "DeleteSection",
    "RefreshIniFileMapping"
};


NTSTATUS
BaseDllCaptureIniFileParameters(
                               BOOLEAN UnicodeParameters,
                               INIFILE_OPERATION Operation,
                               BOOLEAN WriteOperation,
                               BOOLEAN MultiValueStrings,
                               PVOID FileName,
                               PVOID ApplicationName,
                               PVOID VariableName,
                               PVOID VariableValue,
                               PULONG ResultMaxChars OPTIONAL,
                               PINIFILE_PARAMETERS *ReturnedParameterBlock
                               );


NTSTATUS
BaseDllReadWriteIniFileViaMapping(
                                 IN PINIFILE_PARAMETERS a
                                 );

NTSTATUS
BaseDllReadWriteIniFileOnDisk(
                             IN PINIFILE_PARAMETERS a
                             );

NTSTATUS
BaseDllOpenIniFileOnDisk(
                        IN PINIFILE_PARAMETERS a
                        );

NTSTATUS
BaseDllCloseIniFileOnDisk(
                         IN PINIFILE_PARAMETERS a
                         );

NTSTATUS
BaseDllAppendNullToResultBuffer(
                               IN PINIFILE_PARAMETERS a
                               );

NTSTATUS
BaseDllAppendStringToResultBuffer(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PANSI_STRING String OPTIONAL,
                                 IN PUNICODE_STRING StringU OPTIONAL,
                                 IN BOOLEAN IncludeNull
                                 );

NTSTATUS
BaseDllAppendBufferToResultBuffer(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PBYTE Buffer OPTIONAL,
                                 IN PWSTR BufferU OPTIONAL,
                                 IN ULONG Chars,
                                 IN BOOLEAN IncludeNull
                                 );

NTSTATUS
BaseDllReadWriteIniFile(
                       IN BOOLEAN Unicode,
                       IN BOOLEAN WriteOperation,
                       IN BOOLEAN SectionOperation,
                       IN PVOID FileName OPTIONAL,
                       IN PVOID ApplicationName OPTIONAL,
                       IN PVOID VariableName OPTIONAL,
                       IN OUT PVOID VariableValue OPTIONAL,
                       IN OUT PULONG VariableValueLength OPTIONAL
                       )
{
    BOOLEAN MultiValueStrings;
    INIFILE_OPERATION Operation;
    PINIFILE_PARAMETERS a;
    NTSTATUS Status;

    if (SectionOperation) {
        VariableName = NULL;
    }

    MultiValueStrings = FALSE;
    if (WriteOperation) {
        if (ARGUMENT_PRESENT( ApplicationName )) {
            if (ARGUMENT_PRESENT( VariableName )) {
                if (ARGUMENT_PRESENT( VariableValue )) {
                    Operation = WriteKeyValue;
                } else {
                    Operation = DeleteKey;
                }
            } else {
                if (ARGUMENT_PRESENT( VariableValue )) {
                    Operation = WriteSection;
                    MultiValueStrings = TRUE;
                } else {
                    Operation = DeleteSection;
                }
            }
        } else {
#if DBG
            if (ARGUMENT_PRESENT( VariableName ) ||
                ARGUMENT_PRESENT( VariableValue )
               ) {
                return STATUS_INVALID_PARAMETER;
            } else
#endif
                if (ARGUMENT_PRESENT( FileName )) {
                Operation = RefreshIniFileMapping;
            } else {
                Operation = FlushProfiles;
            }
        }
    } else {
        if (ARGUMENT_PRESENT( ApplicationName )) {
            if (!ARGUMENT_PRESENT( VariableValue )) {
                return STATUS_INVALID_PARAMETER;
            } else
                if (ARGUMENT_PRESENT( VariableName )) {
                Operation = ReadKeyValue;
            } else
                if (SectionOperation) {
                Operation = ReadSection;
                MultiValueStrings = TRUE;
            } else {
                Operation = ReadKeyNames;
                MultiValueStrings = TRUE;
            }
        } else
            if (SectionOperation || !ARGUMENT_PRESENT( VariableValue )) {
            return STATUS_INVALID_PARAMETER;
        } else {
            VariableName = NULL;
            Operation = ReadSectionNames;
            MultiValueStrings = TRUE;
        }
    }

#if DBG
    if (WriteOperation) {
        ASSERT( Operation == WriteKeyValue || Operation == WriteSection || Operation == DeleteKey || Operation == DeleteSection || Operation == FlushProfiles || Operation == RefreshIniFileMapping );
    } else {
        ASSERT( Operation == ReadKeyValue || Operation == ReadKeyNames || Operation == ReadSectionNames || Operation == ReadSection );
    }
#endif

    Status = BaseDllCaptureIniFileParameters( Unicode,
                                              Operation,
                                              WriteOperation,
                                              MultiValueStrings,
                                              FileName,
                                              ApplicationName,
                                              VariableName,
                                              VariableValue,
                                              VariableValueLength,
                                              &a
                                            );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

#if DBG
    if (BaseDllDumpIniCalls) {
        DbgPrint( "BASEDLL: called with profile operation\n" );
        DbgPrint( "    Operation: %s  Write: %u\n", xOperationNames[ a->Operation ], a->WriteOperation );
        DbgPrint( "    BaseFileName: %wZ\n", &a->BaseFileName );
        DbgPrint( "    IniFileNameMapping: %08x\n", a->IniFileNameMapping );
        DbgPrint( "    FileName: %wZ\n", &a->FileName );
        DbgPrint( "    NtFileName: %wZ\n", &a->NtFileName );
        DbgPrint( "    ApplicationName: %wZ (%Z)\n", &a->ApplicationNameU, &a->ApplicationName );
        DbgPrint( "    VariableName: %wZ (%Z)\n", &a->VariableNameU, &a->VariableName );
        if (a->WriteOperation) {
            DbgPrint( "    VariableValue: %ws (%s)\n", a->ValueBufferU, a->ValueBuffer );
        }
    }
#endif // DBG

    if (a->Operation == RefreshIniFileMapping) {

#if defined(BUILD_WOW6432)
        Status = CsrBasepRefreshIniFileMapping(&a->BaseFileName);
#else
        BASE_API_MSG m;
        PBASE_REFRESHINIFILEMAPPING_MSG ap = &m.u.RefreshIniFileMapping;
        PCSR_CAPTURE_HEADER CaptureBuffer;

        CaptureBuffer = NULL;
        if (a->BaseFileName.Length > (MAX_PATH * sizeof( WCHAR ))) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      a->BaseFileName.MaximumLength
                                                    );
            if (CaptureBuffer == NULL) {
                Status = STATUS_NO_MEMORY;
            } else {
                CsrCaptureMessageString( CaptureBuffer,
                                         (PCHAR)a->BaseFileName.Buffer,
                                         a->BaseFileName.Length,
                                         a->BaseFileName.MaximumLength,
                                         (PSTRING)&ap->IniFileName
                                       );
                Status = STATUS_SUCCESS;
            }
        }

        if (NT_SUCCESS( Status )) {
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 CaptureBuffer,
                                 CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                      BasepRefreshIniFileMapping
                                                    ),
                                 sizeof( *ap )
                               );

            Status = (NTSTATUS)m.ReturnValue;
        }

        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        }
#endif
    } else
        if (a->IniFileNameMapping != NULL) {
        Status = BaseDllReadWriteIniFileViaMapping( a );
#if DBG
        if (BaseDllDumpIniCalls) {
            if (NT_SUCCESS( Status ) ||
                Status == STATUS_BUFFER_OVERFLOW ||
                Status == STATUS_MORE_PROCESSING_REQUIRED
               ) {
                if (!a->WriteOperation) {
                    if (a->Unicode) {
                        if (a->Operation == ReadKeyValue) {
                            DbgPrint( "BASEDLL: Returning value from registry - '%.*ws' (%u)\n", a->ResultChars, a->ResultBufferU, a->ResultChars );
                        } else {
                            PWSTR s;

                            DbgPrint( "BASEDLL: Return multi-value from registry: (%u)\n", a->ResultChars );
                            s = a->ResultBufferU;
                            s[ a->ResultChars ] = UNICODE_NULL;
                            while (*s) {
                                DbgPrint( "    %ws\n", s );
                                while (*s++) {
                                }
                            }
                        }
                    } else {
                        if (a->Operation == ReadKeyValue) {
                            DbgPrint( "BASEDLL: Returning value from registry - '%.*s' (%u)\n", a->ResultChars, a->ResultBuffer, a->ResultChars );
                        } else {
                            PBYTE s;

                            DbgPrint( "BASEDLL: Return multi-value from registry: (%u)\n", a->ResultChars );
                            s = a->ResultBuffer;
                            s[ a->ResultChars ] = '\0';
                            while (*s) {
                                DbgPrint( "    (%s)\n", s );
                                while (*s++) {
                                }
                            }
                        }
                    }
                } else {
                    DbgPrint( "BASEDLL: Returning success for above write operation\n" );
                }

                if (Status == STATUS_BUFFER_OVERFLOW) {
                    DbgPrint( "    *** above result partial as buffer too small.\n" );
                } else
                    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                    DbgPrint( "    *** above result partial and will now look on disk.\n" );
                }
            } else {
                DbgPrint( "BASEDLL: Profile operation %s failed: Status == %x\n", xOperationNames[ a->Operation ], Status );
            }

            if (a->ValueBufferAllocated) {
                if (a->Unicode) {
                    PWSTR s;

                    DbgPrint( "BASEDLL: Remaining Variables to write to disk:\n" );
                    s = a->ValueBufferU;
                    while (*s) {
                        DbgPrint( "    %ws\n", s );
                        while (*s++) {
                        }
                    }
                } else {
                    PBYTE s;

                    DbgPrint( "BASEDLL: Remaining Variables to write to disk:\n" );
                    s = a->ValueBuffer;
                    while (*s) {
                        DbgPrint( "    (%s)\n", s );
                        while (*s++) {
                        }
                    }

                }
            }
        }
#endif // DBG
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        Status = BaseDllReadWriteIniFileOnDisk( a );
#if DBG
        if (BaseDllDumpIniCalls) {
            if (NT_SUCCESS( Status ) ||
                Status == STATUS_BUFFER_OVERFLOW
               ) {
                if (!a->WriteOperation) {
                    if (a->Unicode) {
                        if (a->Operation == ReadKeyValue) {
                            DbgPrint( "BASEDLL: Returning value from disk - '%.*ws' (%u)\n", a->ResultChars, a->ResultBufferU, a->ResultChars );
                        } else {
                            PWSTR s;

                            DbgPrint( "BASEDLL: Return multi-value from disk: (%u)\n", a->ResultChars );
                            s = a->ResultBufferU;
                            s[ a->ResultChars ] = UNICODE_NULL;
                            while (*s) {
                                DbgPrint( "    %ws\n", s );
                                while (*s++) {
                                }
                            }
                        }
                    } else {
                        if (a->Operation == ReadKeyValue) {
                            DbgPrint( "BASEDLL: Returning value from disk - '%.*s' (%u)\n", a->ResultChars, a->ResultBuffer, a->ResultChars );
                        } else {
                            PBYTE s;

                            DbgPrint( "BASEDLL: Return multi-value from disk: (%u)\n", a->ResultChars );
                            s = a->ResultBuffer;
                            s[ a->ResultChars ] = '\0';
                            while (*s) {
                                DbgPrint( "    (%s)\n", s );
                                while (*s++) {
                                }
                            }
                        }
                    }

                    if (Status == STATUS_BUFFER_OVERFLOW) {
                        DbgPrint( "    *** above result partial as buffer too small.\n" );
                    }
                } else {
                    DbgPrint( "BASEDLL: Returning success for above write operation.\n" );
                }
            } else {
                DbgPrint( "BASEDLL: Profile operation %s failed: Status == %x\n", xOperationNames[ a->Operation ], Status );
            }
        }
#endif // DBG
    }

    if (BaseRunningInServerProcess || a->Operation == FlushProfiles) {
        BaseDllFlushRegistryCache();
    }

    if (NT_SUCCESS( Status )) {
        if (a->Operation == ReadSectionNames ||
            a->Operation == ReadKeyNames ||
            a->Operation == ReadSection
           ) {
            BaseDllAppendNullToResultBuffer( a );
        }
    }

    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (a->WriteOperation) {
            BaseIniFileUpdateCount++;
        } else
            if (ARGUMENT_PRESENT( VariableValueLength )) {
            *VariableValueLength = a->ResultChars;
        }
    }

    if (a->ValueBufferAllocated) {
        if (a->Unicode) {
            RtlFreeHeap( RtlProcessHeap(), 0, a->ValueBufferU );
        } else {
            RtlFreeHeap( RtlProcessHeap(), 0, a->ValueBuffer );
        }
    }

    RtlFreeHeap( RtlProcessHeap(), 0, a );

    return Status;
}


ULONG
BaseDllIniFileNameLength(
                        IN BOOLEAN Unicode,
                        IN PVOID *Name
                        );

NTSTATUS
BaseDllFindIniFileNameMapping(
                             IN PUNICODE_STRING FileName,
                             IN PUNICODE_STRING BaseFileName,
                             OUT PINIFILE_MAPPING_FILENAME *ReturnedFileNameMapping
                             );

BOOLEAN
BaseDllGetApplicationName(
                         IN PINIFILE_PARAMETERS a,
                         OUT PANSI_STRING *ApplicationName OPTIONAL,
                         OUT PUNICODE_STRING *ApplicationNameU OPTIONAL
                         );

BOOLEAN
BaseDllGetVariableName(
                      IN PINIFILE_PARAMETERS a,
                      OUT PANSI_STRING *VariableName OPTIONAL,
                      OUT PUNICODE_STRING *VariableNameU OPTIONAL
                      );

BOOLEAN
BaseDllGetVariableValue(
                       IN PINIFILE_PARAMETERS a,
                       OUT PBYTE *VariableValue OPTIONAL,
                       OUT PWSTR *VariableValueU OPTIONAL,
                       OUT PULONG VariableValueLength
                       );

NTSTATUS
BaseDllCaptureIniFileParameters(
                               BOOLEAN Unicode,
                               INIFILE_OPERATION Operation,
                               BOOLEAN WriteOperation,
                               BOOLEAN MultiValueStrings,
                               PVOID FileName OPTIONAL,
                               PVOID ApplicationName OPTIONAL,
                               PVOID VariableName OPTIONAL,
                               PVOID VariableValue OPTIONAL,
                               PULONG ResultMaxChars OPTIONAL,
                               PINIFILE_PARAMETERS *ReturnedParameterBlock
                               )
{
    NTSTATUS Status;
    PBYTE s;
    PWSTR p, p1;
    ULONG TotalSize,
    CharSize,
    NtFileNameLength,
    FileNameLength,
    ApplicationNameLength,
    VariableNameLength,
    VariableValueLength;
    ANSI_STRING AnsiString;
    PINIFILE_PARAMETERS a;

    if (ARGUMENT_PRESENT( FileName )) {
        if (Unicode) {
            FileNameLength = wcslen( FileName );
        } else {
            FileNameLength = strlen( FileName );
        }
    } else {
        FileNameLength = 0;
    }

    if (ARGUMENT_PRESENT( ApplicationName )) {
        ApplicationNameLength = BaseDllIniFileNameLength( Unicode, &ApplicationName );
    } else {
        ApplicationNameLength = 0;
        VariableName = NULL;
    }

    if (ARGUMENT_PRESENT( VariableName )) {
        VariableNameLength = BaseDllIniFileNameLength( Unicode, &VariableName );
    } else {
        VariableNameLength = 0;
    }

    if (ARGUMENT_PRESENT( VariableValue )) {
        if (ARGUMENT_PRESENT( ResultMaxChars )) {
            VariableValueLength = 0;
        } else {
            if (!MultiValueStrings) {
                if (Unicode) {
                    VariableValueLength = wcslen( VariableValue );
                } else {
                    VariableValueLength = strlen( VariableValue );
                }
            } else {
                if (Unicode) {
                    p = (PWSTR)VariableValue;
                    while (*p) {
                        while (*p++) {
                            ;
                        }
                    }

                    VariableValueLength = (ULONG)(p - (PWSTR)VariableValue);
                } else {
                    s = (PBYTE)VariableValue;
                    while (*s) {
                        while (*s++) {
                            ;
                        }
                    }

                    VariableValueLength = (ULONG)(s - (PBYTE)VariableValue);
                }
            }
        }
    } else {
        VariableValueLength = 0;
    }

    NtFileNameLength = RtlGetLongestNtPathLength() * sizeof( WCHAR );
    TotalSize = sizeof( *a ) + NtFileNameLength;
    if (!Unicode) {
        TotalSize += (FileNameLength + 1 ) * sizeof( WCHAR );
    }

    // We have to allocate enough buffer for DBCS string.
    CharSize = (Unicode ? sizeof(WORD) : sizeof( WCHAR ));
    TotalSize += (ApplicationNameLength + 1 +
                  VariableNameLength + 1 +
                  VariableValueLength + 1
                 ) * CharSize;
    a = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), TotalSize );
    if (a == NULL) {
        KdPrint(( "BASE: Unable to allocate IniFile parameter buffer of %u bytes\n", TotalSize ));
        return STATUS_NO_MEMORY;
    }

    a->Operation = Operation;
    a->WriteOperation = WriteOperation;
    a->Unicode = Unicode;
    a->IniFile = NULL;
    a->ValueBufferAllocated = FALSE;
    a->MultiValueStrings = MultiValueStrings;

    p = (PWSTR)(a + 1);
    a->NtFileName.Buffer = p;
    a->NtFileName.Length = 0;
    a->NtFileName.MaximumLength = (USHORT)NtFileNameLength;
    p = (PWSTR)((PCHAR)p + NtFileNameLength);

    if (ARGUMENT_PRESENT( FileName )) {
        a->FileName.MaximumLength = (USHORT)((FileNameLength + 1) * sizeof( UNICODE_NULL ));
        if (Unicode) {
            a->FileName.Length = (USHORT)(FileNameLength * sizeof( WCHAR ));
            a->FileName.Buffer = FileName;
        } else {
            AnsiString.Buffer = FileName;
            AnsiString.Length = (USHORT)FileNameLength;
            AnsiString.MaximumLength = (USHORT)(AnsiString.Length + 1);
            a->FileName.Buffer = p;
            a->FileName.Length = 0;
            p += FileNameLength + 1;
            Status = Basep8BitStringToUnicodeString( &a->FileName, &AnsiString, FALSE );
            if (!NT_SUCCESS( Status )) {
                RtlFreeHeap( RtlProcessHeap(), 0, a );
                return Status;
            }
        }

        a->BaseFileName.Length = 0;
        p1 = a->FileName.Buffer + FileNameLength;
        while (--p1 > a->FileName.Buffer) {
            if (*p1 == OBJ_NAME_PATH_SEPARATOR ||
                *p1 == L'/' ||
                *p1 == L':'
               ) {
                p1++;
                break;
            }
        }

        a->BaseFileName.Buffer = p1;
        a->BaseFileName.Length = (USHORT)((FileNameLength - (p1 - a->FileName.Buffer)) * sizeof( WCHAR ));
        a->BaseFileName.MaximumLength = (USHORT)(a->BaseFileName.Length + sizeof( UNICODE_NULL ));
        BaseDllFindIniFileNameMapping( &a->FileName,
                                       &a->BaseFileName,
                                       &a->IniFileNameMapping
                                     );
    } else {
        RtlInitUnicodeString( &a->FileName, L"win.ini" );
        a->BaseFileName = a->FileName;
        a->IniFileNameMapping = (PINIFILE_MAPPING_FILENAME)BaseDllIniFileMapping->WinIniFileMapping;
    }

    if (ARGUMENT_PRESENT( ApplicationName )) {
        // We have to keep enough buffer for DBCS string.
        a->ApplicationName.MaximumLength = (USHORT)((ApplicationNameLength * sizeof(WORD)) + 1);
        a->ApplicationNameU.MaximumLength = (USHORT)(a->ApplicationName.MaximumLength * sizeof( UNICODE_NULL ));
        if (Unicode) {
            a->ApplicationNameU.Buffer = ApplicationName;
            a->ApplicationNameU.Length = (USHORT)(ApplicationNameLength * sizeof( UNICODE_NULL ));
            a->ApplicationName.Buffer = (PBYTE)p;
            a->ApplicationName.Length = 0;
            p = (PWSTR)((PCHAR)p + (ApplicationNameLength * sizeof(WORD)) + 1);
        } else {
            a->ApplicationName.Buffer = ApplicationName;
            a->ApplicationName.Length = (USHORT)ApplicationNameLength;
            a->ApplicationNameU.Buffer = p;
            a->ApplicationNameU.Length = 0;
            p += ApplicationNameLength + 1;
        }
    } else {
        RtlInitAnsiString( &a->ApplicationName, NULL );
        RtlInitUnicodeString( &a->ApplicationNameU, NULL );
    }

    if (ARGUMENT_PRESENT( VariableName )) {
        // We have to keep enough buffer for DBCS string.
        a->VariableName.MaximumLength = (USHORT)((VariableNameLength *sizeof(WORD)) + 1);
        a->VariableNameU.MaximumLength = (USHORT)(a->VariableName.MaximumLength * sizeof( UNICODE_NULL ));
        if (Unicode) {
            a->VariableNameU.Buffer = VariableName;
            a->VariableNameU.Length = (USHORT)(VariableNameLength * sizeof( UNICODE_NULL ));
            a->VariableName.Buffer = (PBYTE)p;
            a->VariableName.Length = 0;
            p = (PWSTR)((PCHAR)p + (VariableNameLength * sizeof(WORD)) + 1);
        } else {
            a->VariableName.Buffer = VariableName;
            a->VariableName.Length = (USHORT)VariableNameLength;
            a->VariableNameU.Buffer = p;
            a->VariableNameU.Length = 0;
            p += VariableNameLength + 1;
        }
    } else {
        RtlInitAnsiString( &a->VariableName, NULL );
        RtlInitUnicodeString( &a->VariableNameU, NULL );
    }

    if (ARGUMENT_PRESENT( VariableValue )) {
        if (a->WriteOperation) {
            if (Unicode) {
                a->ValueBufferU = VariableValue;
                a->ValueLengthU = VariableValueLength * sizeof( WCHAR );
                *(PBYTE)p = '\0';
                a->ValueBuffer = (PBYTE)p;
                a->ValueLength = 0;
            } else {
                a->ValueBuffer = VariableValue;
                a->ValueLength = VariableValueLength;
                *p = UNICODE_NULL;
                a->ValueBufferU = p;
                a->ValueLengthU = 0;
            }
        } else {
            if (ARGUMENT_PRESENT( ResultMaxChars )) {
                a->ResultMaxChars = *ResultMaxChars;
            } else {
                a->ResultMaxChars = 0;
            }
            a->ResultChars = 0;
            if (Unicode) {
                a->ResultBufferU = VariableValue;
                a->ResultBuffer = NULL;
            } else {
                a->ResultBuffer = VariableValue;
                a->ResultBufferU = NULL;
            }
        }
    } else {
        if (a->WriteOperation) {
            a->ValueBuffer = NULL;
            a->ValueLength = 0;
            a->ValueBufferU = NULL;
            a->ValueLengthU = 0;
        } else {
            a->ResultMaxChars = 0;
            a->ResultChars = 0;
            a->ResultBufferU = NULL;
            a->ResultBuffer = NULL;
        }
    }

    *ReturnedParameterBlock = a;
    return STATUS_SUCCESS;
}


BOOLEAN
BaseDllGetApplicationName(
                         IN PINIFILE_PARAMETERS a,
                         OUT PANSI_STRING *ApplicationName OPTIONAL,
                         OUT PUNICODE_STRING *ApplicationNameU OPTIONAL
                         )
{
    NTSTATUS Status;

    if (ARGUMENT_PRESENT( ApplicationName )) {
        if (a->ApplicationName.Length == 0) {
            Status = RtlUnicodeStringToAnsiString( &a->ApplicationName, &a->ApplicationNameU, FALSE );
            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: UnicodeToAnsi of %wZ failed (%08x)\n", &a->ApplicationNameU, Status ));
                return FALSE;
            }
        }

        *ApplicationName = &a->ApplicationName;
        return TRUE;
    }

    if (ARGUMENT_PRESENT( ApplicationNameU )) {
        if (a->ApplicationNameU.Length == 0) {
            Status = RtlAnsiStringToUnicodeString( &a->ApplicationNameU, &a->ApplicationName, FALSE );
            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: AnsiToUnicode of %Z failed (%08x)\n", &a->ApplicationName, Status ));
                return FALSE;

            }
        }
        *ApplicationNameU = &a->ApplicationNameU;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
BaseDllGetVariableName(
                      IN PINIFILE_PARAMETERS a,
                      OUT PANSI_STRING *VariableName OPTIONAL,
                      OUT PUNICODE_STRING *VariableNameU OPTIONAL
                      )
{
    NTSTATUS Status;

    if (ARGUMENT_PRESENT( VariableName )) {
        if (a->VariableName.Length == 0) {
            Status = RtlUnicodeStringToAnsiString( &a->VariableName, &a->VariableNameU, FALSE );
            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: UnicodeToAnsi of %wZ failed (%08x)\n", &a->VariableNameU, Status ));
                return FALSE;
            }
        }

        *VariableName = &a->VariableName;
        return TRUE;
    }

    if (ARGUMENT_PRESENT( VariableNameU )) {
        if (a->VariableNameU.Length == 0) {
            Status = RtlAnsiStringToUnicodeString( &a->VariableNameU, &a->VariableName, FALSE );
            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: AnsiToUnicode of %Z failed (%08x)\n", &a->VariableName, Status ));
                return FALSE;

            }
        }
        *VariableNameU = &a->VariableNameU;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
BaseDllGetVariableValue(
                       IN PINIFILE_PARAMETERS a,
                       OUT PBYTE *VariableValue OPTIONAL,
                       OUT PWSTR *VariableValueU OPTIONAL,
                       OUT PULONG VariableValueLength
                       )
{
    NTSTATUS Status;
    ULONG Index;

    if (ARGUMENT_PRESENT( VariableValue )) {
        if (a->ValueLength == 0) {
            if (a->ValueBufferU == NULL || a->ValueLengthU == 0) {
                *VariableValue = "";
                *VariableValueLength = 1;
                return TRUE;
            }

            a->ValueLength = a->ValueLengthU;
            Status = RtlUnicodeToMultiByteN( a->ValueBuffer,
                                             a->ValueLength,
                                             &Index,
                                             a->ValueBufferU,
                                             a->ValueLengthU
                                           );

            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: UnicodeToAnsi of %.*ws failed (%08x)\n",
                          a->ValueLengthU / sizeof( WCHAR ), a->ValueBufferU, Status
                        ));
                return FALSE;
            }

            // Set real converted size
            a->ValueLength = Index;
            a->ValueBuffer[ Index ] = '\0';       // Null terminate converted value
        } else {
            Index = a->ValueLength;
        }

        *VariableValue = a->ValueBuffer;
        *VariableValueLength = Index + 1;
        return TRUE;
    }

    if (ARGUMENT_PRESENT( VariableValueU )) {
        if (a->ValueLengthU == 0) {
            if (a->ValueBuffer == NULL || a->ValueLength == 0) {
                *VariableValueU = L"";
                *VariableValueLength = sizeof( UNICODE_NULL );
                return TRUE;
            }

            a->ValueLengthU = a->ValueLength * sizeof( WCHAR );
            Status = RtlMultiByteToUnicodeN( a->ValueBufferU,
                                             a->ValueLengthU,
                                             &Index,
                                             a->ValueBuffer,
                                             a->ValueLength
                                           );


            if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: AnsiToUnicode of %.*s failed (%08x)\n",
                          a->ValueLength, a->ValueBuffer, Status
                        ));
                return FALSE;
            }

            Index = Index / sizeof( WCHAR );
            a->ValueBufferU[ Index ] = UNICODE_NULL;    // Null terminate converted value
        } else {
            Index = a->ValueLengthU / sizeof( WCHAR );
        }

        *VariableValueU = a->ValueBufferU;
        *VariableValueLength = (Index + 1) * sizeof( WCHAR );
        return TRUE;
    }

    return FALSE;
}


NTSTATUS
BaseDllAppendNullToResultBuffer(
                               IN PINIFILE_PARAMETERS a
                               )
{
    return BaseDllAppendBufferToResultBuffer( a,
                                              NULL,
                                              NULL,
                                              0,
                                              TRUE
                                            );
}

NTSTATUS
BaseDllAppendStringToResultBuffer(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PANSI_STRING String OPTIONAL,
                                 IN PUNICODE_STRING StringU OPTIONAL,
                                 IN BOOLEAN IncludeNull
                                 )
{
    if (ARGUMENT_PRESENT( String )) {
        if (ARGUMENT_PRESENT( StringU )) {
            return STATUS_INVALID_PARAMETER;
        } else {
            return BaseDllAppendBufferToResultBuffer( a,
                                                      String->Buffer,
                                                      NULL,
                                                      String->Length,
                                                      IncludeNull
                                                    );
        }
    } else
        if (ARGUMENT_PRESENT( StringU )) {
        if (ARGUMENT_PRESENT( String )) {
            return STATUS_INVALID_PARAMETER;
        } else {
            return BaseDllAppendBufferToResultBuffer( a,
                                                      NULL,
                                                      StringU->Buffer,
                                                      StringU->Length / sizeof( WCHAR ),
                                                      IncludeNull
                                                    );
        }
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
BaseDllAppendBufferToResultBuffer(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PBYTE Buffer OPTIONAL,
                                 IN PWSTR BufferU OPTIONAL,
                                 IN ULONG Chars,
                                 IN BOOLEAN IncludeNull
                                 )
{
    NTSTATUS Status, OverflowStatus;
    ULONG Index;

    OverflowStatus = STATUS_SUCCESS;
    if (ARGUMENT_PRESENT( Buffer )) {
        if (ARGUMENT_PRESENT( BufferU )) {
            return STATUS_INVALID_PARAMETER;
        } else {
            ULONG CharsMbcs = Chars;
            //
            // In this point, Chars does not contains proper value for Unicode.
            // because. Chars was computed based on DBCS string length,
            // This is correct, sources string is DBCS, then
            // if the source is not DBCS. we just adjust it here.
            //
            if (a->Unicode) {
                Status = RtlMultiByteToUnicodeSize(&Chars,Buffer,Chars);
                if (!NT_SUCCESS( Status )) {
                    KdPrint(( "BASEDLL: AnsiToUnicodeSize of %.*s failed (%08x)\n", Chars, Buffer, Status ));
                    return Status;
                }
                Chars /= sizeof(WCHAR);
            }
            if (a->ResultChars + Chars >= a->ResultMaxChars) {
                OverflowStatus = STATUS_BUFFER_OVERFLOW;
                Chars = a->ResultMaxChars - a->ResultChars;
                if (Chars) {
                    Chars -= 1;
                }
            }

            if (Chars) {
                if (a->Unicode) {
                    Status = RtlMultiByteToUnicodeN( a->ResultBufferU + a->ResultChars,
                                                     Chars * sizeof( WCHAR ),
                                                     &Index,
                                                     Buffer,
                                                     CharsMbcs
                                                   );
                    if (!NT_SUCCESS( Status )) {
                        KdPrint(( "BASEDLL: AnsiToUnicode of %.*s failed (%08x)\n", Chars, Buffer, Status ));
                        return Status;
                    }
                } else {
                    memcpy( a->ResultBuffer + a->ResultChars, Buffer, Chars );
                }

                a->ResultChars += Chars;
            }
        }
    } else
        if (ARGUMENT_PRESENT( BufferU )) {
        if (ARGUMENT_PRESENT( Buffer )) {
            return STATUS_INVALID_PARAMETER;
        } else {
            ULONG CharsUnicode = Chars;
            //
            // In this point, Chars does not contains proper value for DBCS.
            // because. Chars was computed by just devide Unicode string length
            // by two. This is correct, sources string is Unicode, then
            // if the source is not Unicode. we just adjust it here.
            //
            if (!(a->Unicode)) {
                Status = RtlUnicodeToMultiByteSize(&Chars,BufferU,Chars * sizeof(WCHAR));
                if (!NT_SUCCESS( Status )) {
                    KdPrint(( "BASEDLL: UnicodeToAnsiSize of %.*ws failed (%08x)\n", Chars, BufferU, Status ));
                    return Status;
                }
            }
            if (a->ResultChars + Chars >= a->ResultMaxChars) {
                OverflowStatus = STATUS_BUFFER_OVERFLOW;
                Chars = a->ResultMaxChars - a->ResultChars;
                if (Chars) {
                    Chars -= 1;
                }
            }

            if (Chars) {
                if (a->Unicode) {
                    memcpy( a->ResultBufferU + a->ResultChars, BufferU, Chars * sizeof( WCHAR ) );
                } else {
                    Status = RtlUnicodeToMultiByteN( a->ResultBuffer + a->ResultChars,
                                                     Chars,
                                                     &Index,
                                                     BufferU,
                                                     CharsUnicode * sizeof( WCHAR )
                                                   );
                    if (!NT_SUCCESS( Status )) {
                        KdPrint(( "BASEDLL: UnicodeToAnsi of %.*ws failed (%08x)\n", Chars, BufferU, Status ));
                        return Status;
                    }
                }

                a->ResultChars += Chars;
            }
        }
    }

    if (IncludeNull) {
        if (a->ResultChars + 1 >= a->ResultMaxChars) {
            return STATUS_BUFFER_OVERFLOW;
        }

        if (a->Unicode) {
            a->ResultBufferU[ a->ResultChars ] = UNICODE_NULL;
        } else {
            a->ResultBuffer[ a->ResultChars ] = '\0';
        }

        a->ResultChars += 1;
    }

    return OverflowStatus;
}


ULONG
BaseDllIniFileNameLength(
                        IN BOOLEAN Unicode,
                        IN PVOID *Name
                        )
{
    if (Unicode) {
        PWSTR p;

        p = *Name;
        while (*p == L' ') {
            p++;
        }
        *Name = p;
        while (*p != UNICODE_NULL) {
            p++;
        }

        if (p > (PWSTR)*Name) {
            while (*--p == L' ') {
            }
            p++;
        }

        return (ULONG)(p - (PWSTR)*Name);
    } else {
        PCH p;

        p = *Name;
        while (*p == ' ') {
            p++;
        }
        *Name = p;
        while (*p != '\0') {
            p++;
        }

        if (p > (PCH)*Name) {
            while (*--p == ' ') {
            }
            p++;
        }

        return (ULONG)(p - (PCH)*Name);
    }
}



NTSTATUS
BaseDllFindIniFileNameMapping(
                             IN PUNICODE_STRING FileName,
                             IN PUNICODE_STRING BaseFileName,
                             OUT PINIFILE_MAPPING_FILENAME *ReturnedFileNameMapping
                             )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_FILENAME FileNameMapping;
    UNICODE_STRING WinIniString;
    WCHAR TermSrvWindowsPath[MAX_PATH+1];
    UNICODE_STRING TermsrvWindowsDir;


    Status = STATUS_OBJECT_NAME_NOT_FOUND;
    RtlInitUnicodeString(&WinIniString, L"win.ini");

    //
    // Only look in mapping if
    //   Unqualified name was specified OR
    //   Path specified exactly matches the name of the Windows directory OR
    //   Filename is not win.ini (special hack for Windows Sound System, which
    //      expects GetPrivateProfileString on C:\SNDSYS\WIN.INI to return the
    //      data from the file, not the registry)
    //

    if (gpTermsrvGetWindowsDirectoryW) {

        if (gpTermsrvGetWindowsDirectoryW (TermSrvWindowsPath,MAX_PATH)) {

            RtlInitUnicodeString(&TermsrvWindowsDir,TermSrvWindowsPath);

        } else {

            RtlInitUnicodeString(&TermsrvWindowsDir,L"");

        }
    }


    if ((FileName->Buffer == BaseFileName->Buffer) ||
        RtlPrefixUnicodeString( &BaseWindowsDirectory, FileName, TRUE ) ||
        (!RtlEqualUnicodeString( BaseFileName, &WinIniString, TRUE )) ||
        // Also check for in per user's windows directory
        (IsTerminalServer() && RtlPrefixUnicodeString( &TermsrvWindowsDir, FileName, TRUE ))) {

        FileNameMapping = (PINIFILE_MAPPING_FILENAME)BaseDllIniFileMapping->FileNames;
        while (FileNameMapping != NULL) {

            BASE_READ_REMOTE_STR_TEMP(TempStr);

            if (RtlEqualUnicodeString( BaseFileName,
                                       BASE_READ_REMOTE_STR(FileNameMapping->Name, TempStr),
                                       TRUE )) {
                Status = STATUS_SUCCESS;
                break;
            }

            FileNameMapping = (PINIFILE_MAPPING_FILENAME)FileNameMapping->Next;
        }

        if (FileNameMapping == NULL) {
            FileNameMapping = (PINIFILE_MAPPING_FILENAME)BaseDllIniFileMapping->DefaultFileNameMapping;
        }

        *ReturnedFileNameMapping = FileNameMapping;
    } else {
        *ReturnedFileNameMapping = NULL;
    }

    return Status;
}


NTSTATUS
BaseDllOpenMappingTarget(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_VARNAME VarNameMapping,
                        IN PUNICODE_STRING ApplicationName OPTIONAL,
                        IN BOOLEAN WriteAccess,
                        OUT PHANDLE Key
                        );

PINIFILE_MAPPING_APPNAME
BaseDllFindAppNameMapping(
                         IN PINIFILE_MAPPING_FILENAME FileNameMapping,
                         IN PUNICODE_STRING ApplicationName
                         );

PINIFILE_MAPPING_VARNAME
BaseDllFindVarNameMapping(
                         IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                         IN PUNICODE_STRING VariableName
                         );

NTSTATUS
BaseDllReadApplicationNames(
                           IN PINIFILE_PARAMETERS a
                           );

NTSTATUS
BaseDllCheckKeyNotEmpty(
                       IN HANDLE Key,
                       IN PUNICODE_STRING SubKeyName
                       );

NTSTATUS
BaseDllReadVariableNames(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_APPNAME AppNameMapping
                        );

NTSTATUS
BaseDllReadVariableValue(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                        IN PINIFILE_MAPPING_VARNAME VarNameMapping OPTIONAL,
                        IN PUNICODE_STRING VariableName OPTIONAL
                        );

NTSTATUS
BaseDllReadApplicationVariables(
                               IN PINIFILE_PARAMETERS a,
                               IN PINIFILE_MAPPING_APPNAME AppNameMapping
                               );

NTSTATUS
BaseDllDeleteApplicationVariables(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PINIFILE_MAPPING_APPNAME AppNameMapping
                                 );

NTSTATUS
BaseDllWriteApplicationVariables(
                                IN PINIFILE_PARAMETERS a,
                                IN PINIFILE_MAPPING_APPNAME AppNameMapping
                                );

NTSTATUS
BaseDllWriteVariableValue(
                         IN PINIFILE_PARAMETERS a,
                         IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                         IN PINIFILE_MAPPING_VARNAME VarNameMapping OPTIONAL,
                         IN PUNICODE_STRING VariableName OPTIONAL
                         );

NTSTATUS
BaseDllReadWriteIniFileViaMapping(
                                 IN PINIFILE_PARAMETERS a
                                 )
{
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    PUNICODE_STRING ApplicationNameU;

    if (a->Operation == FlushProfiles) {
        return STATUS_SUCCESS;
    } else
        if (a->Operation == ReadSectionNames) {
        return BaseDllReadApplicationNames( a );
    } else
        if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    AppNameMapping = BaseDllFindAppNameMapping( a->IniFileNameMapping, ApplicationNameU );
    if (AppNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    } else
        if (a->Operation == ReadKeyValue) {
        return BaseDllReadVariableValue( a, AppNameMapping, NULL, NULL );
    } else
        if (a->Operation == ReadKeyNames) {
        return BaseDllReadVariableNames( a, AppNameMapping );
    } else
        if (a->Operation == ReadSection) {
        return BaseDllReadApplicationVariables( a, AppNameMapping );
    } else
        if (a->Operation == WriteKeyValue || a->Operation == DeleteKey) {
        return BaseDllWriteVariableValue( a, AppNameMapping, NULL, NULL );
    } else
        if (a->Operation == WriteSection || a->Operation == DeleteSection) {
        return BaseDllWriteApplicationVariables( a, AppNameMapping );
    }

    return STATUS_INVALID_PARAMETER;
}

VOID
BaseDllFlushRegistryCache( VOID )
{
    RtlEnterCriticalSection(&BaseDllRegistryCache.Lock);

    BaseDllRegistryCache.MappingTarget = NULL;
    BaseDllRegistryCache.MappingFlags = 0;

    if (BaseDllRegistryCache.RegistryPath.Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, BaseDllRegistryCache.RegistryPath.Buffer );
        RtlZeroMemory( &BaseDllRegistryCache.RegistryPath,
                       sizeof( BaseDllRegistryCache.RegistryPath )
                     );
    }

    if (BaseDllRegistryCache.RegistryKey != INVALID_HANDLE_VALUE) {
        NtClose( BaseDllRegistryCache.RegistryKey );
        BaseDllRegistryCache.RegistryKey = INVALID_HANDLE_VALUE;
    }

    RtlLeaveCriticalSection(&BaseDllRegistryCache.Lock);

    return;
}

NTSTATUS
BaseDllOpenMappingTarget(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_VARNAME VarNameMapping,
                        IN PUNICODE_STRING ApplicationName OPTIONAL,
                        IN BOOLEAN WriteAccess,
                        OUT PHANDLE Key
                        )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_TARGET MappingTarget;
    ULONG MappingFlags;
    BOOLEAN AppendApplicationName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG CreateDisposition;
    PUNICODE_STRING RegistryPathPrefix;
    UNICODE_STRING RegistryPath;
    KEY_BASIC_INFORMATION KeyBasicInfo;
    ULONG ResultLength;
    ULONG n;
    BOOLEAN OperationRetried = FALSE;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    *Key = INVALID_HANDLE_VALUE;
    MappingTarget = (PINIFILE_MAPPING_TARGET)VarNameMapping->MappingTarget;
    MappingFlags = VarNameMapping->MappingFlags & (INIFILE_MAPPING_APPEND_BASE_NAME |
                                                   INIFILE_MAPPING_APPEND_APPLICATION_NAME |
                                                   INIFILE_MAPPING_SOFTWARE_RELATIVE |
                                                   INIFILE_MAPPING_USER_RELATIVE
                                                  );
    if (MappingTarget == NULL || MappingTarget->RegistryPath.Length == 0) {
        return STATUS_SUCCESS;
    }

    if (ARGUMENT_PRESENT( ApplicationName ) &&
        (MappingFlags & INIFILE_MAPPING_APPEND_APPLICATION_NAME)
       ) {
        AppendApplicationName = TRUE;
    } else {
        AppendApplicationName = FALSE;
    }

    if (MappingFlags & INIFILE_MAPPING_USER_RELATIVE) {
        if (!BaseRunningInServerProcess && BaseDllIniUserKeyPath.Length == 0) {
            OpenProfileUserMapping();
        }

        if (BaseDllIniUserKeyPath.Length == 0) {
            KdPrint(( "BASE: Attempt to access user profile specific portion of .INI file.\n" ));
            KdPrint(( "      when there is no current user defined.\n" ));
            KdPrint(( "      Path: %wZ\n",
                      &MappingTarget->RegistryPath
                    ));
            return STATUS_ACCESS_DENIED;
        }

        RegistryPathPrefix = &BaseDllIniUserKeyPath;
    } else
        if (MappingFlags & INIFILE_MAPPING_SOFTWARE_RELATIVE) {
        RegistryPathPrefix = &BaseDllIniSoftwareKeyPath;
    } else {
        RegistryPathPrefix = NULL;
    }

    if (RegistryPathPrefix != NULL) {
        n = RegistryPathPrefix->Length + sizeof( WCHAR );
    } else {
        n = 0;
    }

    n += sizeof( WCHAR ) + MappingTarget->RegistryPath.Length;
    if (MappingFlags & INIFILE_MAPPING_APPEND_BASE_NAME) {
        n += sizeof( WCHAR ) + a->BaseFileName.Length;
    }

    if (AppendApplicationName) {
        n += sizeof( WCHAR ) + ApplicationName->Length;
    }
    n += sizeof( UNICODE_NULL );

    RegistryPath.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, n );
    if (RegistryPath.Buffer == NULL) {

        KdPrint(( "BASE: Unable to allocate registry path buffer of %u bytes\n", n ));
        return STATUS_NO_MEMORY;
    }
    RegistryPath.Length = 0;
    RegistryPath.MaximumLength = (USHORT)n;

    if (RegistryPathPrefix != NULL) {
        RtlAppendUnicodeStringToString( &RegistryPath, RegistryPathPrefix );
        RtlAppendUnicodeToString( &RegistryPath, L"\\" );
    }

    RtlAppendUnicodeStringToString( &RegistryPath,
                                    BASE_READ_REMOTE_STR(MappingTarget->RegistryPath, TempStr)
                                  );

    if (MappingFlags & INIFILE_MAPPING_APPEND_BASE_NAME) {
        RtlAppendUnicodeToString( &RegistryPath, L"\\" );
        RtlAppendUnicodeStringToString( &RegistryPath, &a->BaseFileName );
    }
    if (AppendApplicationName) {
        RtlAppendUnicodeToString( &RegistryPath, L"\\" );
        RtlAppendUnicodeStringToString( &RegistryPath, ApplicationName );
    }

    RtlEnterCriticalSection(&BaseDllRegistryCache.Lock);
    if (BaseDllRegistryCache.RegistryKey != INVALID_HANDLE_VALUE &&
        BaseDllRegistryCache.MappingTarget == MappingTarget &&
        BaseDllRegistryCache.MappingFlags == MappingFlags &&
        BaseDllRegistryCache.WriteAccess == WriteAccess &&
        RtlEqualUnicodeString( &BaseDllRegistryCache.RegistryPath, &RegistryPath, TRUE )
       ) {
        Status = NtQueryKey( BaseDllRegistryCache.RegistryKey,
                             KeyBasicInformation,
                             &KeyBasicInfo,
                             sizeof( KeyBasicInfo ),
                             &ResultLength
                           );
        if (Status != STATUS_KEY_DELETED) {
            RtlFreeHeap( RtlProcessHeap(), 0, RegistryPath.Buffer );
            *Key = BaseDllRegistryCache.RegistryKey;
            RtlLeaveCriticalSection(&BaseDllRegistryCache.Lock);
            return STATUS_SUCCESS;
        }
    }
    RtlLeaveCriticalSection(&BaseDllRegistryCache.Lock);
    BaseDllFlushRegistryCache();

    InitializeObjectAttributes( &ObjectAttributes,
                                &RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                NULL,
                                NULL
                              );

    if (WriteAccess) {
        repeatoperation:

        Status = NtCreateKey( Key,
                              STANDARD_RIGHTS_WRITE |
                              KEY_QUERY_VALUE |
                              KEY_ENUMERATE_SUB_KEYS |
                              KEY_SET_VALUE |
                              KEY_CREATE_SUB_KEY,
                              &ObjectAttributes,
                              0,
                              NULL,
                              0,
                              &CreateDisposition
                            );
        //
        // There are cases where dorks delete the virtual ini file in the
        // registry. To handle this, if we get object path not found, attempt
        // to create the containing key and then repeat the operation
        //

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND && OperationRetried == FALSE ) {

            NTSTATUS RetryStatus;
            OBJECT_ATTRIBUTES RetryObjectAttributes;
            ULONG RetryCreateDisposition;
            UNICODE_STRING RetryRegistryPath;
            HANDLE RetryKey;

            RetryRegistryPath = RegistryPath;
            while ( RetryRegistryPath.Buffer[RetryRegistryPath.Length>>1] != (WCHAR)'\\' ) {
                RetryRegistryPath.Length -= sizeof(WCHAR);
            }

            InitializeObjectAttributes( &RetryObjectAttributes,
                                        &RetryRegistryPath,
                                        OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                        NULL,
                                        NULL
                                      );
            RetryStatus = NtCreateKey( &RetryKey,
                                       STANDARD_RIGHTS_WRITE |
                                       KEY_QUERY_VALUE |
                                       KEY_ENUMERATE_SUB_KEYS |
                                       KEY_SET_VALUE |
                                       KEY_CREATE_SUB_KEY,
                                       &RetryObjectAttributes,
                                       0,
                                       NULL,
                                       0,
                                       &RetryCreateDisposition
                                     );
            if ( NT_SUCCESS(RetryStatus) ) {
                NtClose(RetryKey);
                OperationRetried = TRUE;
                goto repeatoperation;
            }
        }
    } else {
        Status = NtOpenKey( Key,
                            GENERIC_READ,
                            &ObjectAttributes
                          );
    }


    if (NT_SUCCESS( Status )) {
#if DBG
        if (BaseDllDumpIniCalls) {
            KdPrint(( "BASEDLL: Opened %wZ\n", &RegistryPath ));
        }
#endif

        RtlEnterCriticalSection(&BaseDllRegistryCache.Lock);
        BaseDllRegistryCache.MappingTarget = MappingTarget;
        BaseDllRegistryCache.MappingFlags = MappingFlags;
        BaseDllRegistryCache.WriteAccess = WriteAccess;
        BaseDllRegistryCache.RegistryPath = RegistryPath;
        BaseDllRegistryCache.RegistryKey = *Key;
        RtlLeaveCriticalSection(&BaseDllRegistryCache.Lock);
    } else {
#if DBG
        if (BaseDllDumpIniCalls || WriteAccess || Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            DbgPrint( "BASEDLL: Failed to open %wZ for %s - Status == %lx\n",
                      &RegistryPath,
                      WriteAccess ? "write" : "read",
                      Status
                    );
        }
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, RegistryPath.Buffer );
    }

    return Status;
}


PINIFILE_MAPPING_APPNAME
BaseDllFindAppNameMapping(
                         IN PINIFILE_MAPPING_FILENAME FileNameMapping,
                         IN PUNICODE_STRING ApplicationName
                         )
{
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->ApplicationNames;
    while (AppNameMapping != NULL) {

        if (RtlEqualUnicodeString( BASE_READ_REMOTE_STR(AppNameMapping->Name, TempStr),
                                   ApplicationName,
                                   TRUE )) {
            return AppNameMapping;
        }

        AppNameMapping = (PINIFILE_MAPPING_APPNAME)AppNameMapping->Next;
    }

    return (PINIFILE_MAPPING_APPNAME)FileNameMapping->DefaultAppNameMapping;
}


PINIFILE_MAPPING_VARNAME
BaseDllFindVarNameMapping(
                         IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                         IN PUNICODE_STRING VariableName
                         )
{

    PINIFILE_MAPPING_VARNAME VarNameMapping;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
    while (VarNameMapping != NULL) {

        if (RtlEqualUnicodeString( BASE_READ_REMOTE_STR(VarNameMapping->Name,TempStr),
                                   VariableName,
                                   TRUE )) {
            return VarNameMapping;
        }

        VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
    }

    return (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
}


NTSTATUS
BaseDllReadApplicationNames(
                           IN PINIFILE_PARAMETERS a
                           )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    HANDLE Key;
    WCHAR Buffer[ 256 ];
    PKEY_BASIC_INFORMATION KeyInformation;
    ULONG SubKeyIndex;
    ULONG ResultLength;
    UNICODE_STRING SubKeyName;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    AppNameMapping = (PINIFILE_MAPPING_APPNAME)a->IniFileNameMapping->ApplicationNames;
    while (AppNameMapping != NULL) {

        Status = BaseDllAppendStringToResultBuffer( a,
                                                    NULL,
                                                    BASE_READ_REMOTE_STR(AppNameMapping->Name,TempStr),
                                                    TRUE
                                                  );

        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        AppNameMapping = (PINIFILE_MAPPING_APPNAME)AppNameMapping->Next;
    }

    AppNameMapping = (PINIFILE_MAPPING_APPNAME)a->IniFileNameMapping->DefaultAppNameMapping;
    if (AppNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping,
                                       NULL,
                                       FALSE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;
    for (SubKeyIndex = 0; TRUE; SubKeyIndex++) {
        Status = NtEnumerateKey( Key,
                                 SubKeyIndex,
                                 KeyBasicInformation,
                                 KeyInformation,
                                 sizeof( Buffer ),
                                 &ResultLength
                               );

        if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
        }

        if (NT_SUCCESS( Status )) {
            SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
            SubKeyName.Length = (USHORT)KeyInformation->NameLength;
            SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;
            Status = BaseDllCheckKeyNotEmpty( Key,
                                              &SubKeyName
                                            );

            if (NT_SUCCESS( Status ) ) {
                Status = BaseDllAppendStringToResultBuffer( a, NULL, &SubKeyName, TRUE );
            } else
                if (Status != STATUS_NO_MORE_ENTRIES) {
                break;
            } else {
                Status = STATUS_SUCCESS;
            }
        }

        if (!NT_SUCCESS( Status )) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
BaseDllCheckKeyNotEmpty(
                       IN HANDLE Key,
                       IN PUNICODE_STRING SubKeyName
                       )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SubKey;
    KEY_VALUE_BASIC_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    InitializeObjectAttributes( &ObjectAttributes,
                                SubKeyName,
                                OBJ_CASE_INSENSITIVE,
                                Key,
                                NULL
                              );
    Status = NtOpenKey( &SubKey,
                        GENERIC_READ,
                        &ObjectAttributes
                      );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = NtEnumerateValueKey( SubKey,
                                  0,
                                  KeyValueBasicInformation,
                                  &KeyValueInformation,
                                  sizeof( KeyValueInformation ),
                                  &ResultLength
                                );


    if (Status == STATUS_BUFFER_OVERFLOW) {
        Status = STATUS_SUCCESS;
    }

    NtClose( SubKey );

    return Status;
}

NTSTATUS
BaseDllReadVariableNames(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_APPNAME AppNameMapping
                        )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    PUNICODE_STRING ApplicationNameU;
    WCHAR Buffer[ 256 ];
    PKEY_VALUE_BASIC_INFORMATION KeyValueInformation;
    ULONG ValueIndex;
    ULONG ResultLength;
    UNICODE_STRING VariableName;
    HANDLE Key;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
    while (VarNameMapping != NULL) {

        Status = BaseDllAppendStringToResultBuffer( a,
                                                    NULL,
                                                    BASE_READ_REMOTE_STR(VarNameMapping->Name,TempStr),
                                                    TRUE
                                                  );

        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
    }

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
    if (VarNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       VarNameMapping,
                                       ApplicationNameU,
                                       FALSE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    KeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = NtEnumerateValueKey( Key,
                                      ValueIndex,
                                      KeyValueBasicInformation,
                                      KeyValueInformation,
                                      sizeof( Buffer ),
                                      &ResultLength
                                    );
        if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
        } else
            if (!NT_SUCCESS( Status )) {
            return Status;
        }

        VariableName.Buffer = KeyValueInformation->Name;
        VariableName.Length = (USHORT)KeyValueInformation->NameLength;
        VariableName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
        Status = BaseDllAppendStringToResultBuffer( a, NULL, &VariableName, TRUE );
        if (!NT_SUCCESS( Status )) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
BaseDllReadVariableValue(
                        IN PINIFILE_PARAMETERS a,
                        IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                        IN PINIFILE_MAPPING_VARNAME VarNameMapping OPTIONAL,
                        IN PUNICODE_STRING VariableName OPTIONAL
                        )
{
    NTSTATUS Status;
    PUNICODE_STRING ApplicationNameU;
    KEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    PKEY_VALUE_PARTIAL_INFORMATION p;
    ULONG ResultLength;
    UNICODE_STRING Value;
    BOOLEAN OutputVariableName;
    UNICODE_STRING EqualSign;
    PWSTR s;
    HANDLE Key;

    if (ARGUMENT_PRESENT( VariableName )) {
        RtlInitUnicodeString( &EqualSign, L"=" );
        OutputVariableName = TRUE;
    } else {
        if (!BaseDllGetVariableName( a, NULL, &VariableName )) {
            return STATUS_INVALID_PARAMETER;
        }

        OutputVariableName = FALSE;
    }

    if (!ARGUMENT_PRESENT( VarNameMapping )) {
        VarNameMapping = BaseDllFindVarNameMapping( AppNameMapping, VariableName );
    }

    if (VarNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       VarNameMapping,
                                       ApplicationNameU,
                                       FALSE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    Status = NtQueryValueKey( Key,
                              VariableName,
                              KeyValuePartialInformation,
                              &KeyValueInformation,
                              sizeof( KeyValueInformation ),
                              &ResultLength
                            );
    if (!NT_SUCCESS( Status )) {
        if (Status != STATUS_BUFFER_OVERFLOW) {
            return Status;
        }

        p = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), ResultLength );
        if (p == NULL) {
            return STATUS_NO_MEMORY;
        }

        Status = NtQueryValueKey( Key,
                                  VariableName,
                                  KeyValuePartialInformation,
                                  p,
                                  ResultLength,
                                  &ResultLength
                                );
    } else {
        p = &KeyValueInformation;
    }

    if (NT_SUCCESS( Status )) {
        if (p->Type == REG_SZ) {
            if (OutputVariableName) {
                Status = BaseDllAppendStringToResultBuffer( a, NULL, VariableName, FALSE );
                if (NT_SUCCESS( Status )) {
                    Status = BaseDllAppendStringToResultBuffer( a, NULL, &EqualSign, FALSE );
                }
            }

            if (NT_SUCCESS( Status )) {
                Value.Buffer = (PWSTR)&p->Data[ 0 ];
                if (p->DataLength < sizeof( UNICODE_NULL )) {
                    Value.Length = 0;
                } else {
                    Value.Length = (USHORT)(p->DataLength - sizeof( UNICODE_NULL ));
                }
                Value.MaximumLength = (USHORT)(p->DataLength);
                s = (PWSTR)Value.Buffer;
                if (a->Operation == ReadKeyValue &&
                    Value.Length >= (2 * sizeof( WCHAR )) &&
                    (s[ 0 ] == s[ (Value.Length - sizeof( WCHAR )) / sizeof( WCHAR ) ]) &&
                    (s[ 0 ] == L'"' || s[ 0 ] == L'\'')
                   ) {
                    Value.Buffer += 1;
                    Value.Length -= (2 * sizeof( WCHAR ));
                    Value.MaximumLength -= (2 * sizeof( WCHAR ));
                }

                Status = BaseDllAppendStringToResultBuffer( a, NULL, &Value, TRUE );
            }
        } else {
            KdPrint(( "BASE: Registry value %wZ not REG_SZ type (%u)\n", VariableName, p->Type ));
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
    }


    if (p != &KeyValueInformation) {
        RtlFreeHeap( RtlProcessHeap(), 0, p );
    }

    return Status;
}


NTSTATUS
BaseDllReadApplicationVariables(
                               IN PINIFILE_PARAMETERS a,
                               IN PINIFILE_MAPPING_APPNAME AppNameMapping
                               )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    PUNICODE_STRING ApplicationNameU;
    WCHAR Buffer[ 256 ];
    PKEY_VALUE_BASIC_INFORMATION KeyValueInformation;
    ULONG ValueIndex;
    ULONG ResultLength;
    UNICODE_STRING VariableName;
    HANDLE Key;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
    while (VarNameMapping != NULL) {
        if (VarNameMapping->Name.Length != 0) {

            Status = BaseDllReadVariableValue( a,
                                               AppNameMapping,
                                               VarNameMapping,
                                               BASE_READ_REMOTE_STR(VarNameMapping->Name,TempStr)
                                             );

            if (!NT_SUCCESS( Status )) {
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                    Status == STATUS_OBJECT_TYPE_MISMATCH
                   ) {
                    Status = STATUS_SUCCESS;
                } else {
                    return Status;
                }
            }
        }

        VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
    }

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
    if (VarNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       VarNameMapping,
                                       ApplicationNameU,
                                       FALSE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    KeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
    for (ValueIndex = 0; TRUE; ValueIndex++) {
        Status = NtEnumerateValueKey( Key,
                                      ValueIndex,
                                      KeyValueBasicInformation,
                                      KeyValueInformation,
                                      sizeof( Buffer ),
                                      &ResultLength
                                    );
        if (Status == STATUS_NO_MORE_ENTRIES) {
            break;
        } else
            if (!NT_SUCCESS( Status )) {
            return Status;
        }

        VariableName.Buffer = KeyValueInformation->Name;
        VariableName.Length = (USHORT)KeyValueInformation->NameLength;
        VariableName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
        Status = BaseDllReadVariableValue( a, AppNameMapping, NULL, &VariableName );

        if (!NT_SUCCESS( Status ) &&
            Status != STATUS_OBJECT_NAME_NOT_FOUND &&
            Status != STATUS_OBJECT_TYPE_MISMATCH
           ) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
BaseDllDeleteApplicationVariables(
                                 IN PINIFILE_PARAMETERS a,
                                 IN PINIFILE_MAPPING_APPNAME AppNameMapping
                                 )
{
    NTSTATUS Status;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    WCHAR Buffer[ 256 ];
    HANDLE Key;
    PKEY_VALUE_BASIC_INFORMATION KeyValueInformation;
    ULONG ResultLength;
    PUNICODE_STRING ApplicationNameU;
    UNICODE_STRING VariableName;
    BASE_READ_REMOTE_STR_TEMP(TempStr);

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
    while (VarNameMapping != NULL) {
        if (VarNameMapping->Name.Length != 0) {

            Status = BaseDllWriteVariableValue( a,
                                                AppNameMapping,
                                                VarNameMapping,
                                                BASE_READ_REMOTE_STR(VarNameMapping->Name,TempStr)
                                              );

            if (!NT_SUCCESS( Status )) {
                if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                    return Status;
                }
            }
        }

        VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
    }

    VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
    if (VarNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       VarNameMapping,
                                       ApplicationNameU,
                                       TRUE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    KeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
    do {
        //
        // Enumerate the 0th key.  Since we are deleting as we go
        // this will always be a new key until we are out of entries.
        //
        Status = NtEnumerateValueKey( Key,
                                      0,
                                      KeyValueBasicInformation,
                                      KeyValueInformation,
                                      sizeof( Buffer ),
                                      &ResultLength
                                    );

        if (NT_SUCCESS( Status )) {
    
            VariableName.Buffer = KeyValueInformation->Name;
            VariableName.Length = (USHORT)KeyValueInformation->NameLength;
            VariableName.MaximumLength = (USHORT)KeyValueInformation->NameLength;
            Status = NtDeleteValueKey( Key,
                                       &VariableName
                                     );
            //
            // If we couldn't find VariableName, then somebody must be deleting
            // at the same time we are and beat us to it, so we just ignore the error.
            //
    
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                Status = STATUS_SUCCESS;
            }
        }

    } while (NT_SUCCESS( Status ));

    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    //
    // We can't delete key, as if there are handles open to it,
    // future attempts to recreate it will fail.
    //
    // Status = NtDeleteKey( Key );
    //

    BaseDllFlushRegistryCache();

    return Status;
}


NTSTATUS
BaseDllWriteApplicationVariables(
                                IN PINIFILE_PARAMETERS a,
                                IN PINIFILE_MAPPING_APPNAME AppNameMapping
                                )
{
    NTSTATUS Status;
    ULONG n;
    PVOID SaveValueBuffer, NewValueBuffer, CurrentValueBuffer, CurrentVariableStart, FreeBuffer;
    ULONG SaveValueLength, NewValueLength, CurrentValueLength, CurrentVariableLength;

    if (a->Operation == DeleteSection) {
        return BaseDllDeleteApplicationVariables( a, AppNameMapping );
    }

    if (a->ValueBuffer != NULL && a->ValueLength != 0) {
        SaveValueBuffer = a->ValueBuffer;
        SaveValueLength = a->ValueLength;
    } else
        if (a->ValueBufferU != NULL && a->ValueLengthU != 0) {
        SaveValueBuffer = a->ValueBufferU;
        SaveValueLength = a->ValueLengthU;
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    NewValueBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), SaveValueLength );
    if (NewValueBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    FreeBuffer = NULL;
    try {
        RtlMoveMemory( NewValueBuffer, SaveValueBuffer, NewValueLength = SaveValueLength );
        CurrentValueBuffer = NewValueBuffer;
        CurrentValueLength = NewValueLength;
        NewValueLength = 0;
        while (CurrentValueLength) {
            if (a->Unicode) {
                PWSTR s, s1;

                s = CurrentValueBuffer;
                n = CurrentValueLength / sizeof( WCHAR );
                while (n && *s != UNICODE_NULL && *s <= L' ') {
                    n--;
                    s++;
                }
                if (!n || *s == UNICODE_NULL) {

                    break;
                }

                CurrentVariableStart = s;
                a->VariableNameU.Buffer = s;
                while (n && *s != L'=') {
                    n--;
                    s++;
                }
                if (!n) {
                    break;
                }

                s1 = s++;
                n--;
                while (s1 > a->VariableNameU.Buffer) {
                    if (s1[-1] > L' ') {
                        break;
                    }
                    s1 -= 1;
                }
                a->VariableNameU.Length = (USHORT)((PCHAR)s1 - (PCHAR)a->VariableNameU.Buffer);
                if (a->VariableNameU.Length == 0) {
                    break;
                }
                a->VariableNameU.MaximumLength = a->VariableNameU.Length + sizeof( UNICODE_NULL );
                while (n && *s == L' ') {
                    n--;
                    s++;
                }

                a->ValueBufferU = s;
                while (n && *s != UNICODE_NULL) {
                    n--;
                    s++;
                }
                if (!n) {
                    break;
                }

                a->ValueLengthU = (USHORT)((PCHAR)s - (PCHAR)a->ValueBufferU);
                n--;
                s++;
                CurrentVariableLength = (ULONG)((PCHAR)s - (PCHAR)CurrentVariableStart);
                CurrentValueBuffer = s;
                CurrentValueLength = n * sizeof( WCHAR );
            } else {
                PBYTE s, s1;

                s = CurrentValueBuffer;
                n = CurrentValueLength;
                while (n && *s != '\0' && *s <= ' ') {
                    n--;
                    s++;
                }
                if (!n || *s == '\0') {

                    break;
                }

                CurrentVariableStart = s;
                a->VariableName.Buffer = s;
                while (n && *s != '=') {
                    n--;
                    s++;
                }
                if (!n) {
                    break;
                }

                s1 = s++;
                n--;
                while (s1 > a->VariableName.Buffer) {
                    if (s1[-1] > ' ') {
                        break;
                    }
                    s1 -= 1;
                }
                a->VariableName.Length = (USHORT)(s1 - a->VariableName.Buffer);
                if (a->VariableName.Length == 0) {
                    break;
                }
                a->VariableName.MaximumLength = a->VariableName.Length + 1;
                while (n && *s == ' ') {
                    n--;
                    s++;
                }

                a->ValueBuffer = s;
                while (n && *s != '\0') {
                    n--;
                    s++;
                }
                if (!n) {
                    break;
                }

                a->ValueLength = (USHORT)(s - a->ValueBuffer);
                n--;
                s++;
                CurrentVariableLength = (ULONG)(s - (PCHAR)CurrentVariableStart);
                CurrentValueBuffer = s;
                CurrentValueLength = n;

                a->VariableNameU.MaximumLength = a->VariableName.MaximumLength * sizeof( WCHAR );
                a->VariableNameU.Length = 0;
                FreeBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                              MAKE_TAG( TMP_TAG ),
                                              a->VariableNameU.MaximumLength +
                                              ((a->ValueLength+1) * sizeof( WCHAR ))
                                            );
                if (FreeBuffer == NULL) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                a->VariableNameU.Buffer = FreeBuffer;
                a->ValueBufferU = (PWSTR)((PCHAR)FreeBuffer + a->VariableNameU.MaximumLength );
            }

            Status = BaseDllWriteVariableValue( a, AppNameMapping, NULL, NULL );
            if (FreeBuffer != NULL) {
                RtlFreeHeap( RtlProcessHeap(), 0, FreeBuffer );
                FreeBuffer = NULL;
                RtlInitUnicodeString( &a->VariableNameU, NULL );
                a->ValueBufferU = NULL;
                a->ValueLengthU = 0;
            }

            if (!NT_SUCCESS( Status )) {
                if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
                    break;
                } else {
                    RtlMoveMemory( (PCHAR)NewValueBuffer + NewValueLength,
                                   CurrentVariableStart,
                                   CurrentVariableLength
                                 );
                    NewValueLength += CurrentVariableLength;
                    Status = STATUS_SUCCESS;
                }
            }
        }
    }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
    }

    if (NewValueLength) {
        a->ValueBufferAllocated = TRUE;
        if (a->Unicode) {
            a->ValueBufferU = NewValueBuffer;
            a->ValueLengthU = NewValueLength;
        } else {
            a->ValueBuffer = NewValueBuffer;
            a->ValueLength = NewValueLength;
        }
    } else {
        RtlFreeHeap( RtlProcessHeap(), 0, NewValueBuffer );
    }

    return Status;
}


NTSTATUS
BaseDllWriteVariableValue(
                         IN PINIFILE_PARAMETERS a,
                         IN PINIFILE_MAPPING_APPNAME AppNameMapping,
                         IN PINIFILE_MAPPING_VARNAME VarNameMapping OPTIONAL,
                         IN PUNICODE_STRING VariableName OPTIONAL
                         )
{
    NTSTATUS Status;
    PUNICODE_STRING ApplicationNameU;
    PWSTR VariableValueU;
    ULONG VariableValueLength;
    HANDLE Key;
    KEY_VALUE_BASIC_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    if (!ARGUMENT_PRESENT( VariableName )) {
        if (!BaseDllGetVariableName( a, NULL, &VariableName )) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (!ARGUMENT_PRESENT( VarNameMapping )) {
        VarNameMapping = BaseDllFindVarNameMapping( AppNameMapping, VariableName );
    }

    if (VarNameMapping == NULL) {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!BaseDllGetApplicationName( a, NULL, &ApplicationNameU )) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BaseDllOpenMappingTarget( a,
                                       VarNameMapping,
                                       ApplicationNameU,
                                       TRUE,
                                       &Key
                                     );
    if (!NT_SUCCESS( Status ) || Key == INVALID_HANDLE_VALUE) {
        return Status;
    }

    Status = NtQueryValueKey( Key,
                              VariableName,
                              KeyValueBasicInformation,
                              &KeyValueInformation,
                              sizeof( KeyValueInformation ),
                              &ResultLength
                            );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (KeyValueInformation.Type != REG_SZ) {
            return STATUS_OBJECT_TYPE_MISMATCH;
        }
    }

    if (a->Operation == WriteKeyValue || a->Operation == WriteSection) {
        if (!BaseDllGetVariableValue( a, NULL, &VariableValueU, &VariableValueLength )) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = NtSetValueKey( Key,
                                    VariableName,
                                    0,
                                    REG_SZ,
                                    VariableValueU,
                                    VariableValueLength
                                  );
        }
    } else {
        Status = NtDeleteValueKey( Key,
                                   VariableName
                                 );
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            Status = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS( Status ) && (VarNameMapping->MappingFlags & INIFILE_MAPPING_WRITE_TO_INIFILE_TOO)) {
#if 0
        DbgPrint( "BASEDLL: WriteToProfileToo for [%wZ] %wZ . %wZ\n",
                  &a->FileName,
                  ApplicationNameU,
                  VariableName
                );
#endif
        return STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return Status;
    }
}



NTSTATUS
BaseDllReadSectionNames(
                       IN PINIFILE_PARAMETERS a
                       );

NTSTATUS
BaseDllReadKeywordNames(
                       IN PINIFILE_PARAMETERS a
                       );

NTSTATUS
BaseDllReadKeywordValue(
                       IN PINIFILE_PARAMETERS a
                       );

NTSTATUS
BaseDllReadSection(
                  IN PINIFILE_PARAMETERS a
                  );

NTSTATUS
BaseDllWriteSection(
                   IN PINIFILE_PARAMETERS a
                   );

NTSTATUS
BaseDllWriteKeywordValue(
                        IN PINIFILE_PARAMETERS a,
                        IN PUNICODE_STRING VariableName OPTIONAL
                        );


#define BYTE_ORDER_MARK           0xFEFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE

NTSTATUS
BaseDllReadWriteIniFileOnDisk(
                             IN PINIFILE_PARAMETERS a
                             )
{
    NTSTATUS Status;
    ULONG PartialResultChars;

    if (!a->WriteOperation) {
        PartialResultChars = a->ResultChars;
    }

    Status = BaseDllOpenIniFileOnDisk( a );
    if (NT_SUCCESS( Status )) {
        try {
            a->TextEnd = (PCHAR)a->IniFile->BaseAddress + a->IniFile->EndOfFile;
            a->TextCurrent = a->IniFile->BaseAddress;
            if (a->IniFile->UnicodeFile &&
                ((*(PWCHAR)a->TextCurrent == BYTE_ORDER_MARK) ||
                 (*(PWCHAR)a->TextCurrent == REVERSE_BYTE_ORDER_MARK)))
            {
                // Skip past the BOM.
                ((PWCHAR)a->TextCurrent)++;
            }

            if (a->Operation == ReadSectionNames) {
                Status = BaseDllReadSectionNames( a );
            } else
                if (a->Operation == ReadKeyValue) {
                Status = BaseDllReadKeywordValue( a );
            } else
                if (a->Operation == ReadKeyNames) {
                Status = BaseDllReadKeywordNames( a );
            } else
                if (a->Operation == ReadSection) {
                Status = BaseDllReadSection( a );
            } else
                if (a->Operation == WriteKeyValue || a->Operation == DeleteKey) {
                Status = BaseDllWriteKeywordValue( a, NULL );
            } else
                if (a->Operation == WriteSection || a->Operation == DeleteSection) {
                Status = BaseDllWriteSection( a );
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        finally {
            BaseDllCloseIniFileOnDisk( a );
        }
    }

    if (IsTerminalServer()) {
        // The entry they were looking for wasn't found, see if we need to sync
        // up the ini file
        if (!NT_SUCCESS(Status) && ((a->Operation == ReadSectionNames) ||
                                    (a->Operation == ReadKeyValue) ||
                                    (a->Operation == ReadKeyNames) ||
                                    (a->Operation == ReadSection))) {

            // Sync up the ini file (if necessary), if we updated it, retry the
            // original request
            if (TermsrvSyncUserIniFile(a)) {
                BaseDllReadWriteIniFileOnDisk(a);
            }
        } else if (NT_SUCCESS(Status) && ((a->Operation == WriteKeyValue) ||
                                          (a->Operation == DeleteKey) ||
                                          (a->Operation == WriteSection) ||
                                          (a->Operation == DeleteSection)) &&
                                          TermsrvAppInstallMode()) {
            // Update log of installed files
            if (gpTermsrvLogInstallIniFile) {
                gpTermsrvLogInstallIniFile(&a->NtFileName);
            }
        }
    }


    if (Status == STATUS_OBJECT_NAME_NOT_FOUND &&
        !a->WriteOperation &&
        PartialResultChars != 0
       ) {
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS
BaseDllOpenIniFileOnDisk(
                        IN PINIFILE_PARAMETERS a
                        )
{
    NTSTATUS Status;
    UNICODE_STRING FullFileName;
    ULONG n;
    PWSTR FilePart;
    PINIFILE_CACHE IniFile;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset, Length;

    a->NtFileName.Length = 0;
    if ((a->FileName.Length > sizeof( WCHAR ) &&
         a->FileName.Buffer[ 1 ] == L':'
        ) ||
        (a->FileName.Length != 0 &&
         wcscspn( a->FileName.Buffer, L"\\/" ) != (a->FileName.Length / sizeof( WCHAR ))
        )
       ) {
        n = GetFullPathNameW( a->FileName.Buffer,
                              a->NtFileName.MaximumLength / sizeof( WCHAR ),
                              a->NtFileName.Buffer,
                              &FilePart
                            );
        if (n > a->NtFileName.MaximumLength) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            a->NtFileName.Length = (USHORT)(n * sizeof( WCHAR ));
            Status = STATUS_SUCCESS;
        }

        /*
         * If the base windows directory was specified,
         * redirect to user's directory
         */
        if (gpTermsrvConvertSysRootToUserDir) {
            gpTermsrvConvertSysRootToUserDir( &a->NtFileName, &BaseWindowsDirectory );
        }

    } else {
        //
        // get user based ini file
        //
        if (!gpTermsrvBuildIniFileName ||
             !(NT_SUCCESS(Status = gpTermsrvBuildIniFileName( &a->NtFileName, &a->BaseFileName )))) {

            RtlCopyUnicodeString( &a->NtFileName,
                                  &BaseWindowsDirectory
                                );
            Status = RtlAppendUnicodeToString( &a->NtFileName,
                                               L"\\"
                                             );
            if (NT_SUCCESS( Status )) {
                Status = RtlAppendUnicodeStringToString( &a->NtFileName,
                                                         &a->BaseFileName
                                                       );
            }
        }
    }

    IniFile = NULL;
    if (NT_SUCCESS( Status )) {
        if (RtlDosPathNameToNtPathName_U( a->NtFileName.Buffer,
                                          &FullFileName,
                                          &FilePart,
                                          NULL
                                        )
           ) {
            RtlCopyUnicodeString( &a->NtFileName, &FullFileName );
            RtlFreeUnicodeString( &FullFileName );

            IniFile = RtlAllocateHeap( RtlProcessHeap(),
                                       MAKE_TAG( INI_TAG ) | HEAP_ZERO_MEMORY,
                                       sizeof( *IniFile ) + a->NtFileName.MaximumLength
                                     );
            if (IniFile == NULL) {
                return STATUS_NO_MEMORY;
            }
            IniFile->NtFileName.Buffer = (PWSTR)(IniFile + 1);
            IniFile->NtFileName.MaximumLength = a->NtFileName.MaximumLength;
            RtlCopyUnicodeString( &IniFile->NtFileName, &a->NtFileName );
            IniFile->FileMapping = a->IniFileNameMapping;
            IniFile->WriteAccess = a->WriteOperation;

            if (gpTermsrvCORIniFile) {
                /*
                 * We call a function who handles copy on reference INI files
                 * before attempting the open.
                 */


                gpTermsrvCORIniFile( &IniFile->NtFileName );

            }


            InitializeObjectAttributes( &ObjectAttributes,
                                        &IniFile->NtFileName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL
                                      );
            if (IniFile->WriteAccess) {
                Status = NtCreateFile( &IniFile->FileHandle,
                                       SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                       &ObjectAttributes,
                                       &IoStatusBlock,
                                       0,
                                       FILE_ATTRIBUTE_NORMAL,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       FILE_OPEN_IF,
                                       FILE_SYNCHRONOUS_IO_NONALERT |
                                       FILE_NON_DIRECTORY_FILE,
                                       NULL,
                                       0
                                     );
            } else {
                Status = NtOpenFile( &IniFile->FileHandle,
                                     SYNCHRONIZE | GENERIC_READ,
                                     &ObjectAttributes,
                                     &IoStatusBlock,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     FILE_SYNCHRONOUS_IO_NONALERT |
                                     FILE_NON_DIRECTORY_FILE
                                   );
            }

#if DBG
            if (!NT_SUCCESS( Status )) {
                if (BaseDllDumpIniCalls) {
                    KdPrint(( "BASEDLL: Unable to open %wZ - Status == %x\n", &a->NtFileName, Status ));
                }
            }
#endif // DBG
        } else {
            Status = STATUS_OBJECT_PATH_NOT_FOUND;
        }
    }

    if (NT_SUCCESS( Status )) {
        IniFile->LockedFile = FALSE;
        ByteOffset.QuadPart = 0;
        Length.QuadPart = -1;
        Status = NtLockFile( IniFile->FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &ByteOffset,
                             &Length,
                             LockFileKey,
                             FALSE,
                             IniFile->WriteAccess
                           );
        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_NOT_SUPPORTED) {
                //
                // Go naked on downlevel servers since they can't do anything useful
                // to help.
                //

                Status = STATUS_SUCCESS;
            }
        } else {
            IniFile->LockedFile = TRUE;
        }

        if (NT_SUCCESS( Status )) {
            Status = NtQueryInformationFile( IniFile->FileHandle,
                                             &IoStatusBlock,
                                             &IniFile->StandardInformation,
                                             sizeof( IniFile->StandardInformation ),
                                             FileStandardInformation
                                           );
            if (Status == STATUS_BUFFER_OVERFLOW) {
                Status = STATUS_SUCCESS;
            } else
                if (!NT_SUCCESS( Status )) {
                KdPrint(( "BASEDLL: Unable to QueryInformation for %wZ - Status == %x\n", &a->NtFileName, Status ));
            }
        }
    }

    if (!NT_SUCCESS( Status )) {
#if DBG
        if (BaseDllDumpIniCalls) {
            KdPrint(( "BASEDLL: Open of %wZ failed - Status == %x\n",
                      &IniFile->NtFileName,
                      Status
                    ));
        }
#endif // DBG

        if (IniFile != NULL) {
            if (IniFile->LockedFile) {
                ByteOffset.QuadPart = 0;
                Length.QuadPart = -1;
                NtUnlockFile( IniFile->FileHandle,
                              &IoStatusBlock,
                              &ByteOffset,
                              &Length,
                              LockFileKey
                            );
            }

            NtClose( IniFile->FileHandle );
            RtlFreeHeap( RtlProcessHeap(), 0, IniFile );
        }

        return Status;
    }

    IniFile->EndOfFile = IniFile->StandardInformation.EndOfFile.LowPart;
    IniFile->CommitSize = IniFile->EndOfFile + (4 * (IniFile->UnicodeFile ? sizeof( WCHAR ) : 1));
    IniFile->RegionSize = IniFile->CommitSize + 0x100000; // Room for 256KB of growth
    Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                      &IniFile->BaseAddress,
                                      0,
                                      &IniFile->RegionSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
    if (NT_SUCCESS( Status )) {
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &IniFile->BaseAddress,
                                          0,
                                          &IniFile->CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (NT_SUCCESS( Status )) {
            Status = NtReadFile( IniFile->FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 IniFile->BaseAddress,
                                 IniFile->EndOfFile,
                                 NULL,
                                 &LockFileKey
                               );
            if (NT_SUCCESS( Status ) && IoStatusBlock.Information != IniFile->EndOfFile) {
                Status = STATUS_END_OF_FILE;
            }
        }
    }

    if (NT_SUCCESS( Status )) {
        // We would like to check the possibility of IS_TEXT_UNICODE_DBCS_LEADBYTE.
        INT iResult = ~0x0;
        IniFile->UpdateOffset = 0xFFFFFFFF;
        IniFile->UpdateEndOffset = 0;
        IniFile->UnicodeFile = RtlIsTextUnicode( IniFile->BaseAddress, IniFile->EndOfFile, &iResult );
        if (IniFile->UnicodeFile) {
            PWSTR Src;

            Src = (PWSTR)((PCHAR)IniFile->BaseAddress + IniFile->EndOfFile);
            while (Src > (PWSTR)IniFile->BaseAddress && Src[ -1 ] <= L' ') {
                if (Src[-1] == L'\r' || Src[-1] == L'\n') {
                    break;
                }

                IniFile->EndOfFile -= sizeof( WCHAR );
                Src -= 1;
            }

            Src = (PWSTR)((PCHAR)IniFile->BaseAddress + IniFile->EndOfFile);
            if (Src > (PWSTR)IniFile->BaseAddress) {
                if (Src[-1] != L'\n') {
                    *Src++ = L'\r';
                    *Src++ = L'\n';
                    IniFile->UpdateOffset = IniFile->EndOfFile;
                    IniFile->UpdateEndOffset = IniFile->UpdateOffset + 2 * sizeof( WCHAR );
                    IniFile->EndOfFile = IniFile->UpdateEndOffset;
                }
            }
        } else {
            PBYTE Src;

            Src = (PBYTE)((PCHAR)IniFile->BaseAddress + IniFile->EndOfFile);
            while (Src > (PBYTE)IniFile->BaseAddress && Src[ -1 ] <= ' ') {
                if (Src[-1] == '\r' || Src[-1] == '\n') {
                    break;
                }

                IniFile->EndOfFile -= 1;
                Src -= 1;
            }

            Src = (PBYTE)((PCHAR)IniFile->BaseAddress + IniFile->EndOfFile);
            if (Src > (PBYTE)IniFile->BaseAddress) {
                if (Src[-1] != '\n') {
                    *Src++ = '\r';
                    *Src++ = '\n';
                    IniFile->UpdateOffset = IniFile->EndOfFile;
                    IniFile->UpdateEndOffset = IniFile->UpdateOffset + 2;
                    IniFile->EndOfFile = IniFile->UpdateEndOffset;
                }
            }
        }

        a->IniFile = IniFile;
    } else {
        KdPrint(( "BASEDLL: Read of %wZ failed - Status == %x\n",
                  &IniFile->NtFileName,
                  Status
                ));

        if (IniFile->LockedFile) {
            ByteOffset.QuadPart = 0;
            Length.QuadPart = -1;
            NtUnlockFile( IniFile->FileHandle,
                          &IoStatusBlock,
                          &ByteOffset,
                          &Length,
                          LockFileKey
                        );
        }

        NtClose( IniFile->FileHandle );

        RtlFreeHeap( RtlProcessHeap(), 0, IniFile );
    }

    return Status;
}


NTSTATUS
BaseDllCloseIniFileOnDisk(
                         IN PINIFILE_PARAMETERS a
                         )
{
    PINIFILE_CACHE IniFile;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG UpdateLength;
    LARGE_INTEGER ByteOffset, Length;

    Status = STATUS_SUCCESS;
    IniFile = a->IniFile;
    if (IniFile != NULL) {
        if (IniFile->BaseAddress != NULL) {
            if (IniFile->UpdateOffset != 0xFFFFFFFF && IniFile->WriteAccess) {
                ByteOffset.HighPart = 0;
                ByteOffset.LowPart = IniFile->UpdateOffset;
                UpdateLength = IniFile->UpdateEndOffset - IniFile->UpdateOffset;
                Status = NtWriteFile( IniFile->FileHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      (PCHAR)(IniFile->BaseAddress) + IniFile->UpdateOffset,
                                      UpdateLength,
                                      &ByteOffset,
                                      &LockFileKey
                                    );
                if (NT_SUCCESS( Status )) {
                    if (IoStatusBlock.Information != UpdateLength) {
                        Status = STATUS_DISK_FULL;
                    } else {
                        Length.QuadPart = IniFile->EndOfFile;
                        Status = NtSetInformationFile( IniFile->FileHandle,
                                                       &IoStatusBlock,
                                                       &Length,
                                                       sizeof( Length ),
                                                       FileEndOfFileInformation
                                                     );
                    }
                }

                if (!NT_SUCCESS( Status )) {
                    KdPrint(( "BASEDLL: Unable to write changes for %wZ to disk - Status == %x\n",
                              &IniFile->NtFileName,
                              Status
                            ));
                }
            }

            NtFreeVirtualMemory( NtCurrentProcess(),
                                 &IniFile->BaseAddress,
                                 &IniFile->RegionSize,
                                 MEM_RELEASE
                               );
            IniFile->BaseAddress = NULL;
            IniFile->CommitSize = 0;
            IniFile->RegionSize = 0;
        }

        if (IniFile->FileHandle != NULL) {
            if (IniFile->LockedFile) {
                ByteOffset.QuadPart = 0;
                Length.QuadPart = -1;
                NtUnlockFile( IniFile->FileHandle,
                              &IoStatusBlock,
                              &ByteOffset,
                              &Length,
                              HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)
                            );
            }

            Status = NtClose( IniFile->FileHandle );
            IniFile->FileHandle = NULL;
        }

        RtlFreeHeap( RtlProcessHeap(), 0, IniFile );
    }

    return Status;
}


#define STOP_AT_SECTION 1
#define STOP_AT_KEYWORD 2
#define STOP_AT_NONSECTION 3

NTSTATUS
BaseDllFindSection(
                  IN PINIFILE_PARAMETERS a
                  );

NTSTATUS
BaseDllFindKeyword(
                  IN PINIFILE_PARAMETERS a
                  );

NTSTATUS
BaseDllAdvanceTextPointer(
                         IN PINIFILE_PARAMETERS a,
                         IN ULONG StopAt
                         );

NTSTATUS
BaseDllReadSectionNames(
                       IN PINIFILE_PARAMETERS a
                       )
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;
    while (NT_SUCCESS( Status )) {
        Status = BaseDllAdvanceTextPointer( a, STOP_AT_SECTION );
        if (Status == STATUS_MORE_ENTRIES) {
            Status = BaseDllAppendStringToResultBuffer( a,
                                                        a->AnsiSectionName,
                                                        a->UnicodeSectionName,
                                                        TRUE
                                                      );
        } else {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }
    }

    return Status;
}


NTSTATUS
BaseDllReadKeywordNames(
                       IN PINIFILE_PARAMETERS a
                       )
{
    NTSTATUS Status;

    Status = BaseDllFindSection( a );
    while (NT_SUCCESS( Status )) {
        Status = BaseDllAdvanceTextPointer( a, STOP_AT_KEYWORD );
        if (Status == STATUS_MORE_ENTRIES) {
            Status = BaseDllAppendStringToResultBuffer( a,
                                                        a->AnsiKeywordName,
                                                        a->UnicodeKeywordName,
                                                        TRUE
                                                      );
        } else {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }
    }

    return Status;
}


NTSTATUS
BaseDllReadKeywordValue(
                       IN PINIFILE_PARAMETERS a
                       )
{
    NTSTATUS Status;

    Status = BaseDllFindSection( a );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = BaseDllFindKeyword( a );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    if (a->IniFile->UnicodeFile) {
        PWSTR Src;

        Src = (PWSTR)a->UnicodeKeywordValue->Buffer;
        while (*Src <= L' ' && a->UnicodeKeywordValue->Length) {
            Src += 1;
            a->UnicodeKeywordValue->Buffer = Src;
            a->UnicodeKeywordValue->Length -= sizeof( WCHAR );
            a->UnicodeKeywordValue->MaximumLength -= sizeof( WCHAR );
        }

        if (a->UnicodeKeywordValue->Length >= (2 * sizeof( WCHAR )) &&
            (Src[ 0 ] == Src[ (a->UnicodeKeywordValue->Length - sizeof( WCHAR )) / sizeof( WCHAR ) ]) &&
            (Src[ 0 ] == L'"' || Src[ 0 ] == L'\'')
           ) {
            a->UnicodeKeywordValue->Buffer += 1;
            a->UnicodeKeywordValue->Length -= (2 * sizeof( WCHAR ));
            a->UnicodeKeywordValue->MaximumLength -= (2 * sizeof( WCHAR ));
        }
    } else {
        PBYTE Src;

        Src = (PBYTE)a->AnsiKeywordValue->Buffer;
        while (*Src <= ' ' && a->AnsiKeywordValue->Length) {
            Src += 1;
            a->AnsiKeywordValue->Buffer = Src;
            a->AnsiKeywordValue->Length -= sizeof( UCHAR );
            a->AnsiKeywordValue->MaximumLength -= sizeof( UCHAR );
        }

        if (a->AnsiKeywordValue->Length >= (2 * sizeof( UCHAR )) &&
            (Src[ 0 ] == Src[ (a->AnsiKeywordValue->Length - sizeof( UCHAR )) / sizeof( UCHAR ) ]) &&
            (Src[ 0 ] == '"' || Src[ 0 ] == '\'')
           ) {
            a->AnsiKeywordValue->Buffer += 1;
            a->AnsiKeywordValue->Length -= (2 * sizeof( UCHAR ));
            a->AnsiKeywordValue->MaximumLength -= (2 * sizeof( UCHAR ));
        }
    }

    return BaseDllAppendStringToResultBuffer( a,
                                              a->AnsiKeywordValue,
                                              a->UnicodeKeywordValue,
                                              TRUE
                                            );
}


NTSTATUS
BaseDllReadSection(
                  IN PINIFILE_PARAMETERS a
                  )
{
    NTSTATUS Status;

    Status = BaseDllFindSection( a );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    while (TRUE) {
        Status = BaseDllAdvanceTextPointer( a, STOP_AT_NONSECTION );
        if (Status == STATUS_MORE_ENTRIES) {
            if (a->AnsiKeywordName || a->UnicodeKeywordName) {
                Status = BaseDllAppendStringToResultBuffer( a,
                                                            a->AnsiKeywordName,
                                                            a->UnicodeKeywordName,
                                                            FALSE
                                                          );
                if (!NT_SUCCESS( Status )) {
                    return Status;
                }

                Status = BaseDllAppendBufferToResultBuffer( a,
                                                            a->Unicode ? NULL : "=",
                                                            a->Unicode ? L"=" : NULL,
                                                            1,
                                                            FALSE
                                                          );
                if (!NT_SUCCESS( Status )) {
                    return Status;
                }
            }

            if (a->IniFile->UnicodeFile) {
                PWSTR Src;

                Src = (PWSTR)a->UnicodeKeywordValue->Buffer;
                while (*Src <= L' ' && a->UnicodeKeywordValue->Length) {
                    Src += 1;
                    a->UnicodeKeywordValue->Buffer = Src;
                    a->UnicodeKeywordValue->Length -= sizeof( WCHAR );
                    a->UnicodeKeywordValue->MaximumLength -= sizeof( WCHAR );
                }
            } else {
                PBYTE Src;

                Src = (PBYTE)a->AnsiKeywordValue->Buffer;
                while (*Src <= ' ' && a->AnsiKeywordValue->Length) {
                    Src += 1;
                    a->AnsiKeywordValue->Buffer = Src;
                    a->AnsiKeywordValue->Length -= sizeof( UCHAR );
                    a->AnsiKeywordValue->MaximumLength -= sizeof( UCHAR );
                }
            }

            Status = BaseDllAppendStringToResultBuffer( a,
                                                        a->AnsiKeywordValue,
                                                        a->UnicodeKeywordValue,
                                                        TRUE
                                                      );
            if (!NT_SUCCESS( Status )) {
                return Status;
            }
        } else {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }

            break;
        }
    }

    return Status;
}


NTSTATUS
BaseDllModifyMappedFile(
                       IN PINIFILE_PARAMETERS a,
                       IN PVOID AddressInFile,
                       IN ULONG SizeToRemove,
                       IN PVOID InsertBuffer,
                       IN ULONG InsertAmount
                       );


NTSTATUS
BaseDllWriteSection(
                   IN PINIFILE_PARAMETERS a
                   )
{
    NTSTATUS Status;
    BOOLEAN InsertSectionName;
    PVOID InsertBuffer;
    ULONG InsertAmount, n;
    PANSI_STRING AnsiSectionName;
    PUNICODE_STRING UnicodeSectionName;
    PBYTE AnsiKeywordValue, s;
    PWSTR UnicodeKeywordValue, w;
    ULONG ValueLength, SizeToRemove;
    PVOID AddressInFile;

    InsertAmount = 0;
    Status = BaseDllFindSection( a );
    if (!NT_SUCCESS( Status )) {
        if (a->Operation == DeleteSection) {
            return STATUS_SUCCESS;
        }

        AddressInFile = a->TextEnd;
        if (a->IniFile->UnicodeFile) {
            if (!BaseDllGetApplicationName( a, NULL, &UnicodeSectionName )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeSectionName->Length;
        } else {
            if (!BaseDllGetApplicationName( a, &AnsiSectionName, NULL )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiSectionName->Length;
        }

        InsertSectionName = TRUE;
        SizeToRemove = 0;
    } else {
        if (a->Operation == DeleteSection) {
            AddressInFile = a->TextStart;
        } else {
            AddressInFile = a->TextCurrent;
        }
        while (TRUE) {
            //
            // For delete operations need to iterate all lines in section,
            // not just those that have an = on them. Otherwise sections like
            // [foo]
            // a
            // b = c
            // d
            //
            // don't get deleted properly.
            //
            Status = BaseDllAdvanceTextPointer(
                                              a,
                                              (a->Operation == DeleteSection) ? STOP_AT_NONSECTION : STOP_AT_KEYWORD
                                              );

            if (Status == STATUS_MORE_ENTRIES) {
            } else
                if (Status == STATUS_NO_MORE_ENTRIES) {
                SizeToRemove = (ULONG)((PCHAR)a->TextCurrent - (PCHAR)AddressInFile);
                break;
            } else {
                return Status;
            }
        }

        InsertSectionName = FALSE;
    }

    if (a->Operation == DeleteSection) {
        InsertBuffer = NULL;
    } else {
        if (a->IniFile->UnicodeFile) {
            if (!BaseDllGetVariableValue( a, NULL, &UnicodeKeywordValue, &ValueLength )) {
                return STATUS_INVALID_PARAMETER;
            }
            ValueLength -= sizeof( WCHAR );

            //
            // Add in size of value, + \r\n for each line
            //

            w = UnicodeKeywordValue;
            InsertAmount += ValueLength;
            while (*w) {
                while (*w++) {
                }
                InsertAmount += (2-1) * sizeof( WCHAR );    // Subtract out NULL byte already in ValueLength
            }
        } else {
            if (!BaseDllGetVariableValue( a, &AnsiKeywordValue, NULL, &ValueLength )) {
                return STATUS_INVALID_PARAMETER;
            }
            ValueLength -= sizeof( UCHAR );

            //
            // Add in size of value, + \r\n for each line
            //

            s = AnsiKeywordValue;
            InsertAmount += ValueLength;
            while (*s) {
                while (*s++) {
                }
                InsertAmount += 2 - 1;      // Subtract out NULL byte already in ValueLength
            }
        }

        InsertBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), InsertAmount + sizeof( UNICODE_NULL ) );
        if (InsertBuffer == NULL) {
            return STATUS_NO_MEMORY;
        }

        if (a->IniFile->UnicodeFile) {
            PWSTR Src, Dst;

            Dst = InsertBuffer;
            if (InsertSectionName) {
                *Dst++ = L'[';
                Src = UnicodeSectionName->Buffer;
                n = UnicodeSectionName->Length / sizeof( WCHAR );
                while (n--) {
                    *Dst++ = *Src++;
                }
                *Dst++ = L']';
                *Dst++ = L'\r';
                *Dst++ = L'\n';
            }

            Src = UnicodeKeywordValue;
            while (*Src) {
                while (*Dst = *Src++) {
                    Dst += 1;
                }

                *Dst++ = L'\r';
                *Dst++ = L'\n';
            }
        } else {
            PBYTE Src, Dst;

            Dst = InsertBuffer;
            if (InsertSectionName) {
                *Dst++ = '[';
                Src = AnsiSectionName->Buffer;
                n = AnsiSectionName->Length;
                while (n--) {
                    *Dst++ = *Src++;
                }
                *Dst++ = ']';
                *Dst++ = '\r';
                *Dst++ = '\n';
            }

            Src = AnsiKeywordValue;
            while (*Src) {
                while (*Dst = *Src++) {
                    Dst += 1;
                }

                *Dst++ = '\r';
                *Dst++ = '\n';
            }
        }
    }

    Status = BaseDllModifyMappedFile( a,
                                      AddressInFile,
                                      SizeToRemove,
                                      InsertBuffer,
                                      InsertAmount
                                    );
    RtlFreeHeap( RtlProcessHeap(), 0, InsertBuffer );
    return Status;
}

NTSTATUS
BaseDllCalculateDeleteLength(
                            IN PINIFILE_PARAMETERS a
                            )
{
    ULONG DeleteLength;

    if (a->IniFile->UnicodeFile) {
        DeleteLength = (ULONG)((PCHAR)a->TextCurrent -
                               (PCHAR)a->UnicodeKeywordName->Buffer);
    } else {
        DeleteLength = (ULONG)((PCHAR)a->TextCurrent -
                               a->AnsiKeywordName->Buffer);
    }

    return DeleteLength;
}

NTSTATUS
BaseDllWriteKeywordValue(
                        IN PINIFILE_PARAMETERS a,
                        IN PUNICODE_STRING VariableName OPTIONAL
                        )
{
    NTSTATUS Status;
    BOOLEAN InsertSectionName;
    BOOLEAN InsertKeywordName;
    PVOID InsertBuffer;
    ULONG InsertAmount, n;
    PANSI_STRING AnsiSectionName;
    PANSI_STRING AnsiKeywordName;
    PUNICODE_STRING UnicodeSectionName;
    PUNICODE_STRING UnicodeKeywordName;
    PBYTE AnsiKeywordValue;
    PWSTR UnicodeKeywordValue;
    ULONG ValueLength;
    ULONG DeleteLength;
    PVOID AddressInFile;

    InsertAmount = 0;
    Status = BaseDllFindSection( a );
    if (!NT_SUCCESS( Status )) {
        if (a->Operation == DeleteKey) {
            return STATUS_SUCCESS;
        }

        AddressInFile = a->TextEnd;
        if (a->IniFile->UnicodeFile) {
            if (!BaseDllGetApplicationName( a, NULL, &UnicodeSectionName )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeSectionName->Length;
        } else {
            if (!BaseDllGetApplicationName( a, &AnsiSectionName, NULL )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiSectionName->Length;
        }

        InsertSectionName = TRUE;
    } else {
        InsertSectionName = FALSE;
        Status = BaseDllFindKeyword( a );
    }

    if (!NT_SUCCESS( Status )) {
        if (a->Operation == DeleteKey) {
            return STATUS_SUCCESS;
        }

        if (!InsertSectionName) {
            AddressInFile = a->TextCurrent;
        }

        if (a->IniFile->UnicodeFile) {
            if (!BaseDllGetVariableName( a, NULL, &UnicodeKeywordName )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of Keyword=\r\n
            //

            InsertAmount += (1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeKeywordName->Length;
        } else {
            if (!BaseDllGetVariableName( a, &AnsiKeywordName, NULL )) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of Keyword=\r\n
            //

            InsertAmount += (1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiKeywordName->Length;
        }

        InsertKeywordName = TRUE;
    } else {
        if (a->IniFile->UnicodeFile) {
            if (a->Operation == DeleteKey) {
                DeleteLength = BaseDllCalculateDeleteLength( a );
                return BaseDllModifyMappedFile( a,
                                                a->UnicodeKeywordName->Buffer,
                                                DeleteLength,
                                                NULL,
                                                0
                                              );
            } else {
                AddressInFile = a->UnicodeKeywordValue->Buffer;
            }
        } else {
            if (a->Operation == DeleteKey) {
                DeleteLength = BaseDllCalculateDeleteLength( a );
                return BaseDllModifyMappedFile( a,
                                                a->AnsiKeywordName->Buffer,
                                                DeleteLength,
                                                NULL,
                                                0
                                              );
            } else {
                AddressInFile = a->AnsiKeywordValue->Buffer;
            }
        }
        InsertKeywordName = FALSE;
    }

    if (a->IniFile->UnicodeFile) {
        if (!BaseDllGetVariableValue( a, NULL, &UnicodeKeywordValue, &ValueLength )) {
            return STATUS_INVALID_PARAMETER;
        }
        ValueLength -= sizeof( WCHAR );

        if (InsertAmount == 0) {
            return BaseDllModifyMappedFile( a,
                                            a->UnicodeKeywordValue->Buffer,
                                            a->UnicodeKeywordValue->Length,
                                            UnicodeKeywordValue,
                                            ValueLength
                                          );
        }

        //
        // Add in size of value
        //

        InsertAmount += ValueLength;
    } else {
        if (!BaseDllGetVariableValue( a, &AnsiKeywordValue, NULL, &ValueLength )) {
            return STATUS_INVALID_PARAMETER;
        }
        ValueLength -= sizeof( UCHAR );

        if (InsertAmount == 0) {
            return BaseDllModifyMappedFile( a,
                                            a->AnsiKeywordValue->Buffer,
                                            a->AnsiKeywordValue->Length,
                                            AnsiKeywordValue,
                                            ValueLength
                                          );
        }

        //
        // Add in size of value
        //

        InsertAmount += ValueLength;
    }

    InsertBuffer = RtlAllocateHeap( RtlProcessHeap(), MAKE_TAG( TMP_TAG ), InsertAmount  + sizeof( UNICODE_NULL ) );
    if (InsertBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    if (a->IniFile->UnicodeFile) {
        PWSTR Src, Dst;

        Dst = InsertBuffer;
        if (InsertSectionName) {
            *Dst++ = L'[';
            Src = UnicodeSectionName->Buffer;
            n = UnicodeSectionName->Length / sizeof( WCHAR );
            while (n--) {
                *Dst++ = *Src++;
            }
            *Dst++ = L']';
            *Dst++ = L'\r';
            *Dst++ = L'\n';
        }

        if (InsertKeywordName) {
            Src = UnicodeKeywordName->Buffer;
            n = UnicodeKeywordName->Length / sizeof( WCHAR );
            while (n--) {
                *Dst++ = *Src++;
            }
            *Dst++ = L'=';
        }

        Src = UnicodeKeywordValue;
        n = ValueLength / sizeof( WCHAR );
        while (n--) {
            *Dst++ = *Src++;
        }

        if (InsertKeywordName) {
            *Dst++ = L'\r';
            *Dst++ = L'\n';
        }
    } else {
        PBYTE Src, Dst;

        Dst = InsertBuffer;
        if (InsertSectionName) {
            *Dst++ = '[';
            Src = AnsiSectionName->Buffer;
            n = AnsiSectionName->Length;
            while (n--) {
                *Dst++ = *Src++;
            }
            *Dst++ = ']';
            *Dst++ = '\r';
            *Dst++ = '\n';
        }

        if (InsertKeywordName) {
            Src = AnsiKeywordName->Buffer;
            n = AnsiKeywordName->Length;
            while (n--) {
                *Dst++ = *Src++;
            }
            *Dst++ = '=';
        }

        Src = AnsiKeywordValue;
        n = ValueLength;
        while (n--) {
            *Dst++ = *Src++;
        }

        if (InsertKeywordName) {
            *Dst++ = '\r';
            *Dst++ = '\n';
        }
    }

    Status = BaseDllModifyMappedFile( a,
                                      AddressInFile,
                                      0,
                                      InsertBuffer,
                                      InsertAmount
                                    );
    RtlFreeHeap( RtlProcessHeap(), 0, InsertBuffer );
    return Status;
}


NTSTATUS
BaseDllFindSection(
                  IN PINIFILE_PARAMETERS a
                  )
{
    NTSTATUS Status;
    PANSI_STRING AnsiSectionName;
    PUNICODE_STRING UnicodeSectionName;
    BOOL FreeUnicodeBuffer;

    while (TRUE) {
        Status = BaseDllAdvanceTextPointer( a, STOP_AT_SECTION );
        if (Status == STATUS_MORE_ENTRIES) {
            FreeUnicodeBuffer = FALSE;
            if (a->AnsiSectionName) {
                // Ansi ini file
                if (a->Unicode) {
                    // Unicode parm - convert the ansi sectio name to unicode
                    if (!BaseDllGetApplicationName( a, NULL, &UnicodeSectionName )) {
                        return STATUS_INVALID_PARAMETER;
                    }

                    a->UnicodeSectionName = &a->SectionNameU;
                    Status = RtlAnsiStringToUnicodeString( a->UnicodeSectionName,
                                                           a->AnsiSectionName,
                                                           TRUE
                                                         );
                    if (!NT_SUCCESS( Status )) {
                        KdPrint(( "BASEDLL: AnsiToUnicode of %Z failed (%08x)\n", a->AnsiSectionName, Status ));
                        return Status;
                    }

                    FreeUnicodeBuffer = TRUE;
                } else {
                    // Ansi parm
                    if (!BaseDllGetApplicationName( a, &AnsiSectionName, NULL )) {
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            } else {
                // Doesn't matter - Unicode ini, get the Unicode section name.
                if (!BaseDllGetApplicationName( a, NULL, &UnicodeSectionName )) {
                    return STATUS_INVALID_PARAMETER;
                }
            }

            if (a->AnsiSectionName == NULL || a->Unicode) {
                if (RtlEqualUnicodeString( UnicodeSectionName,
                                           a->UnicodeSectionName,
                                           TRUE
                                         )
                   ) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            } else {
                if (RtlEqualString( AnsiSectionName, a->AnsiSectionName, TRUE )) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            }

            if (FreeUnicodeBuffer) {
                RtlFreeUnicodeString( a->UnicodeSectionName );
            }

            if (Status != STATUS_MORE_ENTRIES) {
                return Status;
            }
        } else {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
}

NTSTATUS
BaseDllFindKeyword(
                  IN PINIFILE_PARAMETERS a
                  )
{
    NTSTATUS Status;
    PANSI_STRING AnsiKeywordName;
    PUNICODE_STRING UnicodeKeywordName;
    BOOL FreeUnicodeBuffer;

    while (TRUE) {
        Status = BaseDllAdvanceTextPointer( a, STOP_AT_KEYWORD );
        if (Status == STATUS_MORE_ENTRIES) {
            FreeUnicodeBuffer = FALSE;

            // Here's the deal.  We want to compare Unicode if possible.  If the
            // The ini is Ansi and the input parm is ansi, use ansi.  Otherwise
            // use Unicode for everything.

            if (a->AnsiKeywordName) {
                // Ansi ini file.
                if (a->Unicode) {
                    // Unicode parm - convert the ansi ini keyword to unicode.
                    if (!BaseDllGetVariableName( a, NULL, &UnicodeKeywordName )) {
                        return STATUS_INVALID_PARAMETER;
                    }

                    a->UnicodeKeywordName = &a->KeywordNameU;
                    Status = RtlAnsiStringToUnicodeString( a->UnicodeKeywordName,
                                                           a->AnsiKeywordName,
                                                           TRUE
                                                         );
                    if (!NT_SUCCESS( Status )) {
                        KdPrint(( "BASEDLL: AnsiToUnicode of %Z failed (%08x)\n", a->AnsiKeywordName, Status ));
                        return Status;
                    }

                    FreeUnicodeBuffer = TRUE;
                } else {
                    // Ansi param
                    if (!BaseDllGetVariableName( a, &AnsiKeywordName, NULL )) {
                        return STATUS_INVALID_PARAMETER;
                    }
                }
            } else {
                // Doesn't matter - Unicode ini, get the Unicode parm.
                if (!BaseDllGetVariableName( a, NULL, &UnicodeKeywordName )) {
                    return STATUS_INVALID_PARAMETER;
                }
            }

            if (a->AnsiKeywordName == NULL || a->Unicode) {
                if (RtlEqualUnicodeString( UnicodeKeywordName,
                                           a->UnicodeKeywordName,
                                           TRUE
                                         )
                   ) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            } else {
                if (RtlEqualString( AnsiKeywordName, a->AnsiKeywordName, TRUE )) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            }

            if (FreeUnicodeBuffer) {
                RtlFreeUnicodeString( a->UnicodeKeywordName );
            }

            if (Status != STATUS_MORE_ENTRIES) {
                return Status;
            }
        } else {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
}

NTSTATUS
BaseDllAdvanceTextPointer(
                         IN PINIFILE_PARAMETERS a,
                         IN ULONG StopAt
                         )
{
    BOOLEAN AllowNoEquals;

    if (StopAt == STOP_AT_NONSECTION) {
        StopAt = STOP_AT_KEYWORD;
        AllowNoEquals = TRUE;
    } else {
        AllowNoEquals = FALSE;
    }

    if (a->IniFile->UnicodeFile) {
        PWSTR Src, EndOfLine, EqualSign, EndOfFile;
        PWSTR Name, EndOfName, Value, EndOfValue;

#define INI_TEXT(quote) L##quote

        Src = a->TextCurrent;
        EndOfFile = a->TextEnd;
        while (Src < EndOfFile) {
            //
            // Find first non-blank character on a line.  Skip blank lines
            //

            while (Src < EndOfFile && *Src <= INI_TEXT(' ')) {
                Src++;
            }

            if (Src >= EndOfFile) {
                a->TextCurrent = Src;
                break;
            }

            EndOfLine = Src;
            EqualSign = NULL;
            a->TextStart = Src;
            while (EndOfLine < EndOfFile) {
                if (EqualSign == NULL && *EndOfLine == INI_TEXT('=')) {
                    EqualSign = ++EndOfLine;
                } else
                    if (*EndOfLine == INI_TEXT('\r') || *EndOfLine == INI_TEXT('\n')) {
                    if (*EndOfLine == INI_TEXT('\r')) {
                        EndOfLine++;
                    }

                    if (*EndOfLine == INI_TEXT('\n')) {
                        EndOfLine++;
                    }

                    break;
                } else {
                    EndOfLine++;
                }
            }

            if (*Src != INI_TEXT(';')) {
                if (*Src == INI_TEXT('[')) {
                    Name = Src + 1;
                    while (Name < EndOfLine && *Name <= INI_TEXT(' ')) {
                        Name++;
                    }
                    EndOfName = Name;
                    while (EndOfName < EndOfLine && *EndOfName != INI_TEXT(']')) {
                        EndOfName++;
                    }

                    while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' ')) {
                        EndOfName--;
                    }
                    a->SectionNameU.Buffer = Name;
                    a->SectionNameU.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                    a->SectionNameU.MaximumLength = a->SectionNameU.Length;
                    a->AnsiSectionName = NULL;
                    a->UnicodeSectionName = &a->SectionNameU;
                    if (StopAt == STOP_AT_SECTION) {
                        a->TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    } else
                        if (StopAt == STOP_AT_KEYWORD) {
                        return STATUS_NO_MORE_ENTRIES;
                    }
                } else
                    if (AllowNoEquals || (EqualSign != NULL) ) {

                    if (EqualSign != NULL) {
                        Name = Src;
                        EndOfName = EqualSign - 1;
                        while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' ')) {
                            EndOfName--;
                        }

                        a->KeywordNameU.Buffer = Name;
                        a->KeywordNameU.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                        a->KeywordNameU.MaximumLength = a->KeywordNameU.Length;
                        a->AnsiKeywordName = NULL;
                        a->UnicodeKeywordName = &a->KeywordNameU;

                        Value = EqualSign;
                    } else {
                        Value = Src;
                        a->AnsiKeywordName = NULL;
                        a->UnicodeKeywordName = NULL;
                    }

                    EndOfValue = EndOfLine;
                    while (EndOfValue > Value && EndOfValue[ -1 ] <= INI_TEXT(' ')) {
                        EndOfValue--;
                    }
                    a->KeywordValueU.Buffer = Value;
                    a->KeywordValueU.Length = (USHORT)((PCHAR)EndOfValue - (PCHAR)Value);
                    a->KeywordValueU.MaximumLength = a->KeywordValueU.Length;
                    a->AnsiKeywordValue = NULL;
                    a->UnicodeKeywordValue = &a->KeywordValueU;
                    if (StopAt == STOP_AT_KEYWORD) {
                        a->TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
                }
            }

            Src = EndOfLine;
        }
    } else {
        PBYTE Src, EndOfLine, EqualSign, EndOfFile;
        PBYTE Name, EndOfName, Value, EndOfValue;

#undef INI_TEXT
#define INI_TEXT(quote) quote

        Src = a->TextCurrent;
        EndOfFile = a->TextEnd;
        while (Src < EndOfFile) {
            //
            // Find first non-blank character on a line.  Skip blank lines
            //

            while (Src < EndOfFile && *Src <= INI_TEXT(' ')) {
                Src++;
            }

            if (Src >= EndOfFile) {
                a->TextCurrent = Src;
                break;
            }

            EndOfLine = Src;
            EqualSign = NULL;
            a->TextStart = Src;
            while (EndOfLine < EndOfFile) {
                if (EqualSign == NULL && *EndOfLine == INI_TEXT('=')) {
                    EqualSign = ++EndOfLine;
                } else
                    if (*EndOfLine == INI_TEXT('\r') || *EndOfLine == INI_TEXT('\n')) {
                    if (*EndOfLine == INI_TEXT('\r')) {
                        EndOfLine++;
                    }

                    if (*EndOfLine == INI_TEXT('\n')) {
                        EndOfLine++;
                    }

                    break;
                } else {
                    EndOfLine++;
                }
            }

            if (*Src != INI_TEXT(';')) {
                if (*Src == INI_TEXT('[')) {
                    Name = Src + 1;
                    while (Name < EndOfLine && *Name <= INI_TEXT(' ')) {
                        Name++;
                    }
                    EndOfName = Name;
                    while (EndOfName < EndOfLine) {
                        if (*EndOfName == INI_TEXT(']')) {
                            break;
                        }
                        if (IsDBCSLeadByte(*EndOfName)) {
                            EndOfName++;
                        }
                        EndOfName++;
                    }
                    while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' ')) {
                        EndOfName--;
                    }
                    a->SectionName.Buffer = Name;
                    a->SectionName.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                    a->SectionName.MaximumLength = a->SectionName.Length;
                    a->AnsiSectionName = &a->SectionName;
                    a->UnicodeSectionName = NULL;
                    if (StopAt == STOP_AT_SECTION) {
                        a->TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    } else
                        if (StopAt == STOP_AT_KEYWORD) {
                        return STATUS_NO_MORE_ENTRIES;
                    }
                } else
                    if (AllowNoEquals || (EqualSign != NULL)) {

                    if (EqualSign != NULL) {
                        Name = Src;
                        EndOfName = EqualSign - 1;
                        while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' ')) {
                            EndOfName--;
                        }

                        a->KeywordName.Buffer = Name;
                        a->KeywordName.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                        a->KeywordName.MaximumLength = a->KeywordName.Length;
                        a->AnsiKeywordName = &a->KeywordName;
                        a->UnicodeKeywordName = NULL;

                        Value = EqualSign;
                    } else {
                        Value = Src;
                        a->AnsiKeywordName = NULL;
                        a->UnicodeKeywordName = NULL;
                    }

                    EndOfValue = EndOfLine;
                    while (EndOfValue > Value && EndOfValue[ -1 ] <= INI_TEXT(' ')) {
                        EndOfValue--;
                    }
                    a->KeywordValue.Buffer = Value;
                    a->KeywordValue.Length = (USHORT)((PCHAR)EndOfValue - (PCHAR)Value);
                    a->KeywordValue.MaximumLength = a->KeywordValue.Length;
                    a->AnsiKeywordValue = &a->KeywordValue;
                    a->UnicodeKeywordValue = NULL;
                    if (StopAt == STOP_AT_KEYWORD) {
                        a->TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
                }
            }

            Src = EndOfLine;
        }
    }

    return STATUS_NO_MORE_ENTRIES;
}


NTSTATUS
BaseDllModifyMappedFile(
                       IN PINIFILE_PARAMETERS a,
                       IN PVOID AddressInFile,
                       IN ULONG SizeToRemove,
                       IN PVOID InsertBuffer,
                       IN ULONG InsertAmount
                       )
{
    NTSTATUS Status;
    ULONG NewEndOfFile, UpdateOffset, UpdateLength;

    NewEndOfFile = a->IniFile->EndOfFile - SizeToRemove + InsertAmount;
    if (NewEndOfFile > a->IniFile->CommitSize) {
        if (NewEndOfFile > a->IniFile->RegionSize) {
            return STATUS_BUFFER_OVERFLOW;
        }

        a->IniFile->CommitSize = NewEndOfFile;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &a->IniFile->BaseAddress,
                                          0,
                                          &a->IniFile->CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "BASEDLL: Unable to allocate memory to grow %wZ - Status == %x\n",
                      &a->IniFile->NtFileName,
                      Status
                    ));

            return Status;
        }

        a->IniFile->EndOfFile = NewEndOfFile;
    }

    UpdateOffset = (ULONG)((PCHAR)AddressInFile - (PCHAR)(a->IniFile->BaseAddress)),
                   UpdateLength = (ULONG)((PCHAR)a->TextEnd - (PCHAR)AddressInFile) + InsertAmount - SizeToRemove;
    //
    // Are we deleting more than we are inserting?
    //
    if (SizeToRemove > InsertAmount) {
        //
        // Yes copy over insert string.
        //
        RtlMoveMemory( AddressInFile, InsertBuffer, InsertAmount );

        //
        // Delete remaining text after insertion string by moving it
        // up
        //

        RtlMoveMemory( (PCHAR)AddressInFile + InsertAmount,
                       (PCHAR)AddressInFile + SizeToRemove,
                       UpdateLength - InsertAmount
                     );
    } else
        if (InsertAmount > 0) {
        //
        // Are we deleting less than we are inserting?
        //
        if (SizeToRemove < InsertAmount) {
            //
            // Move text down to make room for insertion
            //

            RtlMoveMemory( (PCHAR)AddressInFile + InsertAmount - SizeToRemove,
                           (PCHAR)AddressInFile,
                           UpdateLength - InsertAmount + SizeToRemove
                         );
        } else {
            //
            // Deleting and inserting same amount, update just that text as
            // no shifting was done.
            //

            UpdateLength = InsertAmount;
        }

        //
        // Copy over insert string
        //

        RtlMoveMemory( AddressInFile, InsertBuffer, InsertAmount );
    } else {
        //
        // Nothing to change, as InsertAmount and SizeToRemove are zero
        //
        return STATUS_SUCCESS;
    }

    if (a->IniFile->EndOfFile != NewEndOfFile) {
        a->IniFile->EndOfFile = NewEndOfFile;
    }

    if (UpdateOffset < a->IniFile->UpdateOffset) {
        a->IniFile->UpdateOffset = UpdateOffset;
    }

    if ((UpdateOffset + UpdateLength) > a->IniFile->UpdateEndOffset) {
        a->IniFile->UpdateEndOffset = UpdateOffset + UpdateLength;
    }

    return STATUS_SUCCESS;
}


VOID
BaseDllCheckInitFromIniFile(
                           IN DWORD Flags,
                           IN ULONG MappingFlags,
                           IN PUNICODE_STRING FileName,
                           IN PUNICODE_STRING ApplicationName,
                           IN PUNICODE_STRING KeyName,
                           IN OUT PWSTR *Buffer,
                           IN OUT PULONG Size
                           );

BOOL
QueryWin31IniFilesMappedToRegistry(
                                  IN DWORD Flags,
                                  OUT PWSTR Buffer,
                                  IN DWORD cchBuffer,
                                  OUT LPDWORD cchUsed
                                  )
{
    BOOL Result;
    PINIFILE_MAPPING_FILENAME FileNameMapping;
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    PINIFILE_MAPPING_VARNAME VarNameMapping;
    UNICODE_STRING NullString;

    ULONG Size;

    *cchUsed = 0;
    RtlInitUnicodeString( &NullString, NULL );
    Result = TRUE;
    Size = cchBuffer * sizeof( WCHAR );

    FileNameMapping = (PINIFILE_MAPPING_FILENAME)BaseDllIniFileMapping->DefaultFileNameMapping;
    if (FileNameMapping != NULL) {
        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->DefaultAppNameMapping;
        VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
        BaseDllCheckInitFromIniFile( Flags,
                                     VarNameMapping->MappingFlags,
                                     &NullString,
                                     &NullString,
                                     &NullString,
                                     &Buffer,
                                     &Size
                                   );

    }

    FileNameMapping = (PINIFILE_MAPPING_FILENAME)BaseDllIniFileMapping->FileNames;
    while (FileNameMapping) {
        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->DefaultAppNameMapping;

        if (AppNameMapping != NULL) {

            BASE_READ_REMOTE_STR_TEMP(TempStr);

            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;

            BaseDllCheckInitFromIniFile( Flags,
                                         VarNameMapping->MappingFlags,
                                         BASE_READ_REMOTE_STR(FileNameMapping->Name,TempStr),
                                         &NullString,
                                         &NullString,
                                         &Buffer,
                                         &Size
                                       );

        }

        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->ApplicationNames;
        while (AppNameMapping) {
            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;

            if (VarNameMapping != NULL) {

                BASE_READ_REMOTE_STR_TEMP(TempStr1);
                BASE_READ_REMOTE_STR_TEMP(TempStr2);

                BaseDllCheckInitFromIniFile( Flags,
                                             VarNameMapping->MappingFlags,
                                             BASE_READ_REMOTE_STR(FileNameMapping->Name, TempStr1),
                                             BASE_READ_REMOTE_STR(AppNameMapping->Name, TempStr2),
                                             &NullString,
                                             &Buffer,
                                             &Size
                                           );

            }

            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
            while (VarNameMapping) {

                BASE_READ_REMOTE_STR_TEMP(TempStr1);
                BASE_READ_REMOTE_STR_TEMP(TempStr2);
                BASE_READ_REMOTE_STR_TEMP(TempStr3);

                BaseDllCheckInitFromIniFile( Flags,
                                             VarNameMapping->MappingFlags,
                                             BASE_READ_REMOTE_STR(FileNameMapping->Name,TempStr1),
                                             BASE_READ_REMOTE_STR(AppNameMapping->Name,TempStr2),
                                             BASE_READ_REMOTE_STR(VarNameMapping->Name,TempStr3),
                                             &Buffer,
                                             &Size
                                           );

                VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
            }

            AppNameMapping = (PINIFILE_MAPPING_APPNAME)AppNameMapping->Next;
        }

        FileNameMapping = (PINIFILE_MAPPING_FILENAME)FileNameMapping->Next;
    }

    if (Result) {
        *cchUsed = (((cchBuffer * sizeof( WCHAR )) - Size) / sizeof( WCHAR )) + 1;
    }

    return Result;
}

VOID
BaseDllCheckInitFromIniFile(
                           IN DWORD Flags,
                           IN ULONG MappingFlags,
                           IN PUNICODE_STRING FileName,
                           IN PUNICODE_STRING ApplicationName,
                           IN PUNICODE_STRING KeyName,
                           IN OUT PWSTR *Buffer,
                           IN OUT PULONG Size
                           )
{
    PWSTR s;
    ULONG cb;

    if (MappingFlags & INIFILE_MAPPING_INIT_FROM_INIFILE) {
        cb = FileName->Length + sizeof( UNICODE_NULL ) +
             ApplicationName->Length + sizeof( UNICODE_NULL ) +
             KeyName->Length + sizeof( UNICODE_NULL );

        if (MappingFlags & INIFILE_MAPPING_USER_RELATIVE) {
            if (!(Flags & WIN31_INIFILES_MAPPED_TO_USER)) {
                return;
            }
        } else
            if (!(Flags & WIN31_INIFILES_MAPPED_TO_SYSTEM)) {
            return;
        }

        if ((cb+sizeof( UNICODE_NULL )) < *Size) {
            *Size = *Size - cb;
            s = *Buffer;

            if (FileName->Length) {
                RtlMoveMemory( s, FileName->Buffer, FileName->Length );
                s = (PWSTR)((PCHAR)s + FileName->Length);
            }
            *s++ = UNICODE_NULL;

            if (ApplicationName->Length) {
                RtlMoveMemory( s, ApplicationName->Buffer, ApplicationName->Length );
                s = (PWSTR)((PCHAR)s + ApplicationName->Length);
            }
            *s++ = UNICODE_NULL;

            if (KeyName->Length) {
                RtlMoveMemory( s, KeyName->Buffer, KeyName->Length );
                s = (PWSTR)((PCHAR)s + KeyName->Length);
            }
            *s++ = UNICODE_NULL;

            *s = UNICODE_NULL;
            *Buffer = s;
        }
    }

    return;
}


#if DBG

VOID
BaseDllDumpIniFileMappings(
                          PINIFILE_MAPPING IniFileMapping
                          );

VOID
BaseDllPrintMappingTarget(
                         IN PINIFILE_MAPPING_VARNAME VarNameMapping
                         );

VOID
BaseDllDumpIniFileMappings(
                          PINIFILE_MAPPING IniFileMapping
                          )
{
    PINIFILE_MAPPING_FILENAME FileNameMapping;
    PINIFILE_MAPPING_APPNAME AppNameMapping;
    PINIFILE_MAPPING_VARNAME VarNameMapping;

    DbgPrint( "IniFileMapping\n" );
    FileNameMapping = (PINIFILE_MAPPING_FILENAME)IniFileMapping->DefaultFileNameMapping;
    if (FileNameMapping != NULL) {
        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->DefaultAppNameMapping;
        VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
        DbgPrint( "    " );
        BaseDllPrintMappingTarget( VarNameMapping );
    }

    FileNameMapping = (PINIFILE_MAPPING_FILENAME)IniFileMapping->FileNames;
    while (FileNameMapping) {
        BASE_READ_REMOTE_STR_TEMP(TempStr);

        DbgPrint( "    %wZ\n",
                  BASE_READ_REMOTE_STR(FileNameMapping->Name,TempStr)
                );

        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->DefaultAppNameMapping;
        if (AppNameMapping != NULL) {
            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
            DbgPrint( "        " );
            BaseDllPrintMappingTarget( VarNameMapping );
        }

        AppNameMapping = (PINIFILE_MAPPING_APPNAME)FileNameMapping->ApplicationNames;
        while (AppNameMapping) {
            BASE_READ_REMOTE_STR_TEMP(TempStr);

            DbgPrint( "        %wZ\n",
                      BASE_READ_REMOTE_STR(AppNameMapping->Name,TempStr)
                    );

            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
            if (VarNameMapping != NULL) {
                DbgPrint( "            " );
                BaseDllPrintMappingTarget( VarNameMapping );
            }

            VarNameMapping = (PINIFILE_MAPPING_VARNAME)AppNameMapping->VariableNames;
            while (VarNameMapping) {
                BASE_READ_REMOTE_STR_TEMP(TempStr);

                DbgPrint( "            %wZ ",
                          BASE_READ_REMOTE_STR(VarNameMapping->Name, TempStr) );

                BaseDllPrintMappingTarget( VarNameMapping );
                VarNameMapping = (PINIFILE_MAPPING_VARNAME)VarNameMapping->Next;
            }

            AppNameMapping = (PINIFILE_MAPPING_APPNAME)AppNameMapping->Next;
        }

        FileNameMapping = (PINIFILE_MAPPING_FILENAME)FileNameMapping->Next;
    }

    return;
}


VOID
BaseDllPrintMappingTarget(
                         IN PINIFILE_MAPPING_VARNAME VarNameMapping
                         )
{
    DbgPrint( "= " );

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_WRITE_TO_INIFILE_TOO) {
        DbgPrint( "!" );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_INIT_FROM_INIFILE) {
        DbgPrint( "#" );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_READ_FROM_REGISTRY_ONLY) {
        DbgPrint( "@" );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_USER_RELATIVE) {
        DbgPrint( "USR:" );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_SOFTWARE_RELATIVE) {
        DbgPrint( "SYS:" );
    }

    if (VarNameMapping->MappingTarget) {
        BASE_READ_REMOTE_STR_TEMP(TempStr);

        DbgPrint( "%wZ",
                  BASE_READ_REMOTE_STR((((PINIFILE_MAPPING_TARGET)(VarNameMapping->MappingTarget))->RegistryPath), TempStr)
                );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_APPEND_BASE_NAME) {
        DbgPrint( " (Append Base Name)" );
    }

    if (VarNameMapping->MappingFlags &INIFILE_MAPPING_APPEND_APPLICATION_NAME) {
        DbgPrint( " (Append App Name)" );
    }

    DbgPrint( "\n" );
}

#endif // DBG
