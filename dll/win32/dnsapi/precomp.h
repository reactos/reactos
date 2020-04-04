/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/dnsapi/precomp.h
 * PURPOSE:         Win32 DNS API Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _DNSAPI_H
#define _DNSAPI_H

/* INCLUDES ******************************************************************/

#include <stdarg.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <windns.h>
#include <reactos/windns_undoc.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <dnsrslvr_c.h>

#include <adns.h>

typedef struct
{
    adns_state State;
} WINDNS_CONTEXT, *PWINDNS_CONTEXT;

static inline LPWSTR dns_strdup_uw( const char *str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc(GetProcessHeap(),0,( len * sizeof(WCHAR) ))))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPWSTR dns_strdup_aw( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc(GetProcessHeap(), 0, ( len * sizeof(WCHAR) ))))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPSTR dns_strdup_a( LPCSTR src )
{
    LPSTR dst;

    if (!src) return NULL;
    dst = HeapAlloc(GetProcessHeap(), 0, (lstrlenA( src ) + 1) * sizeof(char) );
    if (dst) lstrcpyA( dst, src );
    return dst;
}

static inline char *dns_strdup_u( const char *src )
{
    char *dst;

    if (!src) return NULL;
    dst = HeapAlloc(GetProcessHeap(), 0, (strlen( src ) + 1) * sizeof(char) );
    if (dst) strcpy( dst, src );
    return dst;
}

static inline LPWSTR dns_strdup_w( LPCWSTR src )
{
    LPWSTR dst;

    if (!src) return NULL;
    dst = HeapAlloc(GetProcessHeap(), 0, (lstrlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) lstrcpyW( dst, src );
    return dst;
}

static inline LPSTR dns_strdup_wa( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc(GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *dns_strdup_wu( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc(GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *dns_strdup_au( LPCSTR src )
{
    char *dst = NULL;
    LPWSTR ret = dns_strdup_aw( src );

    if (ret)
    {
        dst = dns_strdup_wu( ret );
        HeapFree( GetProcessHeap(), 0, ret );
    }
    return dst;
}

static inline LPSTR dns_strdup_ua( const char *src )
{
    LPSTR dst = NULL;
    LPWSTR ret = dns_strdup_uw( src );

    if (ret)
    {
        dst = dns_strdup_wa( ret );
        HeapFree( GetProcessHeap(), 0, ret );
    }
    return dst;
}

DNS_STATUS DnsIntTranslateAdnsToDNS_STATUS(int Status);
void DnsIntFreeRecordList(PDNS_RECORD ToFree);

#endif /* _DNSAPI_H */
