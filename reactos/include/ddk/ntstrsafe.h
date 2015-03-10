/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifndef _NTSTRSAFE_H_INCLUDED_
#define _NTSTRSAFE_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28719) /* disable banned api usage warning */
#endif /* _MSC_VER */

#ifndef C_ASSERT
#ifdef _MSC_VER
# define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else
# define C_ASSERT(e) extern void __C_ASSERT__(int [(e)?1:-1])
#endif
#endif /* C_ASSERT */

#ifdef __cplusplus
#define _STRSAFE_EXTERN_C extern "C"
#else
#define _STRSAFE_EXTERN_C extern
#endif

#define NTSTRSAFEAPI static __inline NTSTATUS NTAPI
#define NTSTRSAFE_INLINE_API static __inline NTSTATUS NTAPI

#ifndef NTSTRSAFE_MAX_CCH
#define NTSTRSAFE_MAX_CCH 2147483647
#endif

#ifndef _STRSAFE_H_INCLUDED_
#define STRSAFE_IGNORE_NULLS 0x00000100
#define STRSAFE_FILL_BEHIND_NULL 0x00000200
#define STRSAFE_FILL_ON_FAILURE 0x00000400
#define STRSAFE_NULL_ON_FAILURE 0x00000800
#define STRSAFE_NO_TRUNCATION 0x00001000
#define STRSAFE_IGNORE_NULL_UNICODE_STRINGS 0x00010000
#define STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED 0x00020000

#define STRSAFE_VALID_FLAGS (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS (STRSAFE_VALID_FLAGS | STRSAFE_IGNORE_NULL_UNICODE_STRINGS | STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED)

#define STRSAFE_FILL_BYTE(x) ((STRSAFE_DWORD)(((x) & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x) ((STRSAFE_DWORD)(((x) & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))

#define STRSAFE_GET_FILL_PATTERN(dwFlags) ((int)((dwFlags) & 0x000000FF))
#endif

typedef char *STRSAFE_LPSTR;
typedef const char *STRSAFE_LPCSTR;
typedef wchar_t *STRSAFE_LPWSTR;
typedef const wchar_t *STRSAFE_LPCWSTR;

typedef _Null_terminated_ char *NTSTRSAFE_PSTR;
typedef _Null_terminated_ const char *NTSTRSAFE_PCSTR;
typedef _Null_terminated_ wchar_t *NTSTRSAFE_PWSTR;
typedef _Null_terminated_ const wchar_t *NTSTRSAFE_PCWSTR;

typedef ULONG STRSAFE_DWORD;

NTSTRSAFEAPI RtlStringCopyWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc);
NTSTRSAFEAPI RtlStringCopyWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc);
NTSTRSAFEAPI RtlStringCopyExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCopyExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCopyNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy);
NTSTRSAFEAPI RtlStringCopyNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy);
NTSTRSAFEAPI RtlStringCopyNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCopyNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCatWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc);
NTSTRSAFEAPI RtlStringCatWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc);
NTSTRSAFEAPI RtlStringCatExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCatExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCatNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend);
NTSTRSAFEAPI RtlStringCatNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend);
NTSTRSAFEAPI RtlStringCatNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringCatNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags);
NTSTRSAFEAPI RtlStringVPrintfWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,va_list argList);
NTSTRSAFEAPI RtlStringVPrintfWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,va_list argList);
NTSTRSAFEAPI RtlStringVPrintfExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList);
NTSTRSAFEAPI RtlStringVPrintfExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList);

NTSTRSAFEAPI
RtlStringLengthWorkerA(
  _In_reads_or_z_(cchMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) size_t *pcchLength);

NTSTRSAFEAPI
RtlStringLengthWorkerW(
  _In_reads_or_z_(cchMax) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) size_t *pcchLength);

NTSTRSAFEAPI
RtlStringCchCopyA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc);

NTSTRSAFEAPI
RtlStringCchCopyW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc);

NTSTRSAFEAPI
RtlStringCchCopyA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyWorkerA(pszDest, cchDest, pszSrc);
}

NTSTRSAFEAPI
RtlStringCchCopyW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyWorkerW(pszDest,cchDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCbCopyA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc);

NTSTRSAFEAPI
RtlStringCbCopyW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc);

NTSTRSAFEAPI
RtlStringCbCopyA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc)
{
    size_t cchDest = cbDest / sizeof(char);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringCopyWorkerA(pszDest,cbDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCbCopyW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    size_t cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringCopyWorkerW(pszDest,cchDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCchCopyExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCopyExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCopyExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringCopyExWorkerA(pszDest,cchDest,cchDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCchCopyExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    size_t cbDest = cchDest * sizeof(wchar_t);

    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCbCopyExA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) STRSAFE_LPSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCopyExW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCopyExA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) STRSAFE_LPSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchRemaining = 0;

    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlStringCopyExWorkerA(pszDest,cbDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCbCopyExW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchRemaining = 0;

    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlStringCopyExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
            *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCchCopyNA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToCopy);

NTSTRSAFEAPI
RtlStringCchCopyNW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToCopy);


NTSTRSAFEAPI
RtlStringCchCopyNA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToCopy)
{
    if (cchDest > NTSTRSAFE_MAX_CCH || cchToCopy > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNWorkerA(pszDest,cchDest,pszSrc,cchToCopy);
}

NTSTRSAFEAPI
RtlStringCchCopyNW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToCopy)
{
    if (cchDest > NTSTRSAFE_MAX_CCH || cchToCopy > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNWorkerW(pszDest,cchDest,pszSrc,cchToCopy);
}

NTSTRSAFEAPI
RtlStringCbCopyNA(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToCopy);

NTSTRSAFEAPI
RtlStringCbCopyNW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToCopy);

NTSTRSAFEAPI
RtlStringCbCopyNA(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToCopy)
{
    size_t cchDest  = cbDest / sizeof(char);
    size_t cchToCopy = cbToCopy / sizeof(char);

    if (cchDest > NTSTRSAFE_MAX_CCH || cchToCopy > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNWorkerA(pszDest, cchDest, pszSrc, cchToCopy);
}

NTSTRSAFEAPI
RtlStringCbCopyNW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToCopy)
{
    size_t cchDest  = cbDest / sizeof(wchar_t);
    size_t cchToCopy = cbToCopy / sizeof(wchar_t);

    if (cchDest > NTSTRSAFE_MAX_CCH || cchToCopy > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNWorkerW(pszDest, cchDest, pszSrc, cchToCopy);
}

NTSTRSAFEAPI
RtlStringCchCopyNExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToCopy,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCopyNExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToCopy,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCopyNExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToCopy,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNExWorkerA(pszDest, cchDest, cchDest, pszSrc, cchToCopy, ppszDestEnd, pcchRemaining, dwFlags);
}

NTSTRSAFEAPI
RtlStringCchCopyNExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToCopy,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringCopyNExWorkerW(pszDest,cchDest,cchDest * sizeof(wchar_t), pszSrc, cchToCopy, ppszDestEnd, pcchRemaining, dwFlags);
}

NTSTRSAFEAPI
RtlStringCbCopyNExA(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToCopy,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCopyNExW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToCopy,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCopyNExA(
  _Out_writes_bytes_(cbDest) STRSAFE_LPSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToCopy,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) STRSAFE_LPSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchRemaining = 0;

    if (cbDest > NTSTRSAFE_MAX_CCH)
    {
        if ((pszDest != NULL) && (cbDest > 0))
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlStringCopyNExWorkerA(pszDest,cbDest,cbDest,pszSrc,cbToCopy,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = cchRemaining;
    return Status;
}

NTSTRSAFEAPI
RtlStringCbCopyNExW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToCopy) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToCopy,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchDest;
    size_t cchToCopy;
    size_t cchRemaining = 0;
    cchDest = cbDest / sizeof(wchar_t);
    cchToCopy = cbToCopy / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if ((pszDest != NULL) && (cbDest > 0))
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlStringCopyNExWorkerW(pszDest,cchDest,cbDest,pszSrc,cchToCopy,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    return Status;
}

NTSTRSAFEAPI
RtlStringCchCatA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc);

NTSTRSAFEAPI
RtlStringCchCatW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc);

NTSTRSAFEAPI
RtlStringCchCatA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatWorkerA(pszDest,cchDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCchCatW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatWorkerW(pszDest,cchDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCbCatA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc);

NTSTRSAFEAPI
RtlStringCbCatW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc);

NTSTRSAFEAPI
RtlStringCbCatA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc)
{
    if (cbDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatWorkerA(pszDest,cbDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCbCatW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    size_t cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatWorkerW(pszDest,cchDest,pszSrc);
}

NTSTRSAFEAPI
RtlStringCchCatExA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCatExW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCatExA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatExWorkerA(pszDest,cchDest,cchDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCchCatExW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    size_t cbDest = cchDest*sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCbCatExA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCatExW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCatExA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchRemaining = 0;
    if (cbDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringCatExWorkerA(pszDest,cbDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
    return Status;
}

NTSTRSAFEAPI
RtlStringCbCatExW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ NTSTRSAFE_PCWSTR pszSrc,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchRemaining = 0;

    if (cchDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringCatExWorkerW(pszDest,cchDest,cbDest,pszSrc,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    return Status;
}

NTSTRSAFEAPI
RtlStringCchCatNA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToAppend);

NTSTRSAFEAPI
RtlStringCchCatNW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToAppend);

NTSTRSAFEAPI
RtlStringCchCatNA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToAppend)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNWorkerA(pszDest,cchDest,pszSrc,cchToAppend);
}

NTSTRSAFEAPI
RtlStringCchCatNW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToAppend)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNWorkerW(pszDest,cchDest,pszSrc,cchToAppend);
}

NTSTRSAFEAPI
RtlStringCbCatNA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToAppend);

NTSTRSAFEAPI
RtlStringCbCatNW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToAppend);

NTSTRSAFEAPI
RtlStringCbCatNA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToAppend)
{
    if (cbDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNWorkerA(pszDest,cbDest,pszSrc,cbToAppend);
}

NTSTRSAFEAPI
RtlStringCbCatNW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToAppend)
{
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchToAppend = cbToAppend / sizeof(wchar_t);

    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNWorkerW(pszDest,cchDest,pszSrc,cchToAppend);
}

NTSTRSAFEAPI
RtlStringCchCatNExA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToAppend,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCatNExW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToAppend,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCchCatNExA(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cchToAppend,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNExWorkerA(pszDest,cchDest,cchDest,pszSrc,cchToAppend,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCchCatNExW(
  _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_reads_or_z_(cchToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cchToAppend,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
        return STATUS_INVALID_PARAMETER;
    return RtlStringCatNExWorkerW(pszDest,cchDest,(cchDest*sizeof(wchar_t)),pszSrc,cchToAppend,ppszDestEnd,pcchRemaining,dwFlags);
}

NTSTRSAFEAPI
RtlStringCbCatNExA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToAppend,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCatNExW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToAppend,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags);

NTSTRSAFEAPI
RtlStringCbCatNExA(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCSTR pszSrc,
  _In_ size_t cbToAppend,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchRemaining = 0;
    if (cbDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringCatNExWorkerA(pszDest,cbDest,cbDest,pszSrc,cbToAppend,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
    return Status;
}

NTSTRSAFEAPI
RtlStringCbCatNExW(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_reads_bytes_(cbToAppend) STRSAFE_LPCWSTR pszSrc,
  _In_ size_t cbToAppend,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchToAppend = cbToAppend / sizeof(wchar_t);
    size_t cchRemaining = 0;
    if (cchDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringCatNExWorkerW(pszDest,cchDest,cbDest,pszSrc,cchToAppend,ppszDestEnd,&cchRemaining,dwFlags);
    if ((NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) && pcbRemaining)
        *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
    return Status;
}

NTSTRSAFEAPI
RtlStringCchVPrintfA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCchVPrintfW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCchVPrintfA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    return RtlStringVPrintfWorkerA(pszDest,cchDest,pszFormat,argList);
}

NTSTRSAFEAPI
RtlStringCchVPrintfW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList)
{
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
}

NTSTRSAFEAPI
RtlStringCbVPrintfA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCbVPrintfW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCbVPrintfA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList)
{
    if (cbDest > NTSTRSAFE_MAX_CCH)
    {
        if (cbDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringVPrintfWorkerA(pszDest,cbDest,pszFormat,argList);
}

NTSTRSAFEAPI
RtlStringCbVPrintfW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList)
{
    size_t cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }
    return RtlStringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
}

static __inline NTSTATUS
RtlStringCchPrintfA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCchPrintfW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCchPrintfA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    va_list argList;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfWorkerA(pszDest,cchDest,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCchPrintfW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    va_list argList;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCbPrintfA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCbPrintfW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCbPrintfA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    va_list argList;
    if (cbDest > NTSTRSAFE_MAX_CCH)
    {
        if (cbDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfWorkerA(pszDest,cbDest,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCbPrintfW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    va_list argList;
    size_t cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfWorkerW(pszDest,cchDest,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCchPrintfExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCchPrintfExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCchPrintfExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    va_list argList;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfExWorkerA(pszDest,cchDest,cchDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCchPrintfExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    size_t cbDest = cchDest * sizeof(wchar_t);
    va_list argList;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }
    va_start(argList,pszFormat);
    Status = RtlStringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
    va_end(argList);
    return Status;
}

static __inline NTSTATUS
RtlStringCbPrintfExA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCbPrintfExW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...);

static __inline NTSTATUS
RtlStringCbPrintfExA(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    size_t cchDest;
    size_t cchRemaining = 0;

    cchDest = cbDest / sizeof(char);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    {
        va_list argList;
        va_start(argList,pszFormat);
        Status = RtlStringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
        va_end(argList);
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
        }
    }
    return Status;
}

static __inline NTSTATUS
RtlStringCbPrintfExW(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  ...)
{
    NTSTATUS Status;
    size_t cchDest;
    size_t cchRemaining = 0;
    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    {
        va_list argList;
        va_start(argList,pszFormat);
        Status = RtlStringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
        va_end(argList);
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCchVPrintfExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCchVPrintfExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCchVPrintfExA(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList)
{
    NTSTATUS Status;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = '\0';
        return STATUS_INVALID_PARAMETER;
    }

    {
        size_t cbDest;
        cbDest = cchDest*sizeof(char);
        Status = RtlStringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCchVPrintfExW(
  _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcchRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList)
{
    NTSTATUS Status;
    if (cchDest > NTSTRSAFE_MAX_CCH)
    {
        if (cchDest > 0)
            *pszDest = L'\0';
        return STATUS_INVALID_PARAMETER;
    }

    {
        size_t cbDest;
        cbDest = cchDest*sizeof(wchar_t);
        Status = RtlStringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,pcchRemaining,dwFlags,pszFormat,argList);
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCbVPrintfExA(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCbVPrintfExW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList);

NTSTRSAFEAPI
RtlStringCbVPrintfExA(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
  _In_ va_list argList)
{
    NTSTATUS Status;
    size_t cchDest;
    size_t cchRemaining = 0;
    cchDest = cbDest / sizeof(char);
    if (cchDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringVPrintfExWorkerA(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            *pcbRemaining = (cchRemaining*sizeof(char)) + (cbDest % sizeof(char));
        }
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringCbVPrintfExW(
  _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
  _In_ size_t cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR *ppszDestEnd,
  _Out_opt_ size_t *pcbRemaining,
  _In_ STRSAFE_DWORD dwFlags,
  _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
  _In_ va_list argList)
{
    NTSTATUS Status;
    size_t cchDest;
    size_t cchRemaining = 0;
    cchDest = cbDest / sizeof(wchar_t);
    if (cchDest > NTSTRSAFE_MAX_CCH)
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringVPrintfExWorkerW(pszDest,cchDest,cbDest,ppszDestEnd,&cchRemaining,dwFlags,pszFormat,argList);
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (pcbRemaining)
        {
            *pcbRemaining = (cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
        }
    }
    return Status;
}


_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCchLengthA(
  _In_reads_or_z_(cchMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t *pcchLength);

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCchLengthW(
  _In_reads_or_z_(cchMax) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t *pcchLength);

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCchLengthA(
  _In_reads_or_z_(cchMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t *pcchLength)
{
    NTSTATUS Status;
    if (!psz || (cchMax > NTSTRSAFE_MAX_CCH))
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringLengthWorkerA(psz,cchMax,pcchLength);
    if (!NT_SUCCESS(Status) && pcchLength)
    {
        *pcchLength = 0;
    }
    return Status;
}

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCchLengthW(
  _In_reads_or_z_(cchMax) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t *pcchLength)
{
    NTSTATUS Status;
    if (!psz || (cchMax > NTSTRSAFE_MAX_CCH))
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringLengthWorkerW(psz,cchMax,pcchLength);
    if (!NT_SUCCESS(Status) && pcchLength)
    {
        *pcchLength = 0;
    }
    return Status;
}

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCbLengthA(
  _In_reads_or_z_(cbMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax) size_t *pcbLength);

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCbLengthW(
  _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t *pcbLength);

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCbLengthA(
  _In_reads_or_z_(cbMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax) size_t *pcbLength)
{
    NTSTATUS Status;
    size_t cchMax;
    size_t cchLength = 0;
    cchMax = cbMax / sizeof(char);
    if (!psz || (cchMax > NTSTRSAFE_MAX_CCH))
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringLengthWorkerA(psz,cchMax,&cchLength);
    if (pcbLength)
    {
        if (NT_SUCCESS(Status))
        {
            *pcbLength = cchLength*sizeof(char);
        }
        else
        {
            *pcbLength = 0;
        }
    }
    return Status;
}

_Must_inspect_result_
NTSTRSAFEAPI
RtlStringCbLengthW(
  _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t *pcbLength)
{
    NTSTATUS Status;
    size_t cchMax;
    size_t cchLength = 0;
    cchMax = cbMax / sizeof(wchar_t);
    if (!psz || (cchMax > NTSTRSAFE_MAX_CCH))
        Status = STATUS_INVALID_PARAMETER;
    else
        Status = RtlStringLengthWorkerW(psz,cchMax,&cchLength);
    if (pcbLength)
    {
        if (NT_SUCCESS(Status))
        {
            *pcbLength = cchLength*sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        while(cchDest && (*pszSrc!='\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }
        if (cchDest==0)
        {
            pszDest--;
            Status = STATUS_BUFFER_OVERFLOW;
        }
        *pszDest= '\0';
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        while(cchDest && (*pszSrc!=L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
        }
        if (cchDest==0)
        {
            pszDest--;
            Status = STATUS_BUFFER_OVERFLOW;
        }
        *pszDest= L'\0';
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest!=0) || (cbDest!=0))
                    Status = STATUS_INVALID_PARAMETER;
            }
            if (!pszSrc)
                pszSrc = "";
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc!='\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while(cchRemaining && (*pszSrc!='\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
                    }
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                *pszDestEnd = '\0';
            }
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPWSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest!=0) || (cbDest!=0))
                    Status = STATUS_INVALID_PARAMETER;
            }
            if (!pszSrc)
                pszSrc = L"";
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszSrc!=L'\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                pszDestEnd = pszDest;
                cchRemaining = cchDest;
                while(cchRemaining && (*pszSrc!=L'\0'))
                {
                    *pszDestEnd++ = *pszSrc++;
                    cchRemaining--;
                }
                if (cchRemaining > 0)
                {
                    if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                    {
                        memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                    }
                }
                else
                {
                    pszDestEnd--;
                    cchRemaining++;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                *pszDestEnd = L'\0';
            }
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchSrc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        while(cchDest && cchSrc && (*pszSrc!='\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchSrc--;
        }
        if (cchDest==0)
        {
            pszDest--;
            Status = STATUS_BUFFER_OVERFLOW;
        }
        *pszDest= '\0';
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        while(cchDest && cchToCopy && (*pszSrc!=L'\0'))
        {
            *pszDest++ = *pszSrc++;
            cchDest--;
            cchToCopy--;
        }
        if (cchDest==0)
        {
            pszDest--;
            Status = STATUS_BUFFER_OVERFLOW;
        }
        *pszDest= L'\0';
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToCopy,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
        if (cchToCopy > NTSTRSAFE_MAX_CCH)
            Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (dwFlags & STRSAFE_IGNORE_NULLS)
            {
                if (!pszDest)
                {
                    if ((cchDest!=0) || (cbDest!=0))
                        Status = STATUS_INVALID_PARAMETER;
                }
                if (!pszSrc)
                    pszSrc = "";
            }
            if (NT_SUCCESS(Status))
            {
                if (cchDest==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = 0;
                    if ((cchToCopy!=0) && (*pszSrc!='\0'))
                    {
                        if (!pszDest)
                            Status = STATUS_INVALID_PARAMETER;
                        else
                            Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    while(cchRemaining && cchToCopy && (*pszSrc!='\0'))
                    {
                        *pszDestEnd++ = *pszSrc++;
                        cchRemaining--;
                        cchToCopy--;
                    }
                    if (cchRemaining > 0)
                    {
                        if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        {
                            memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
                        }
                    }
                    else
                    {
                        pszDestEnd--;
                        cchRemaining++;
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                    *pszDestEnd = '\0';
                }
            }
        }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCopyNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToCopy,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPWSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
        if (cchToCopy > NTSTRSAFE_MAX_CCH)
            Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (dwFlags & STRSAFE_IGNORE_NULLS)
            {
                if (!pszDest)
                {
                    if ((cchDest!=0) || (cbDest!=0))
                        Status = STATUS_INVALID_PARAMETER;
                }
                if (!pszSrc)
                    pszSrc = L"";
            }
            if (NT_SUCCESS(Status))
            {
                if (cchDest==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = 0;
                    if ((cchToCopy!=0) && (*pszSrc!=L'\0'))
                    {
                        if (!pszDest)
                            Status = STATUS_INVALID_PARAMETER;
                        else
                            Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    while(cchRemaining && cchToCopy && (*pszSrc!=L'\0'))
                    {
                        *pszDestEnd++ = *pszSrc++;
                        cchRemaining--;
                        cchToCopy--;
                    }
                    if (cchRemaining > 0)
                    {
                        if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                        {
                            memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                        }
                    }
                    else
                    {
                        pszDestEnd--;
                        cchRemaining++;
                        Status = STATUS_BUFFER_OVERFLOW;
                    }
                    *pszDestEnd = L'\0';
                }
            }
        }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCatWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc)
{
    NTSTATUS Status;
    size_t cchDestLength;
    Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
    if (NT_SUCCESS(Status))
        Status = RtlStringCopyWorkerA(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc);
    return Status;
}

NTSTRSAFEAPI RtlStringCatWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc)
{
    NTSTATUS Status;
    size_t cchDestLength;
    Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
    if (NT_SUCCESS(Status))
        Status = RtlStringCopyWorkerW(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc);
    return Status;
}

NTSTRSAFEAPI RtlStringCatExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        size_t cchDestLength;
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest==0) && (cbDest==0))
                    cchDestLength = 0;
                else
                    Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
                if (NT_SUCCESS(Status))
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }
            if (!pszSrc)
                pszSrc = "";
        }
        else
        {
            Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
            if (NT_SUCCESS(Status))
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                if (*pszSrc!='\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
                Status = RtlStringCopyExWorkerA(pszDestEnd,cchRemaining,(cchRemaining*sizeof(char)) + (cbDest % sizeof(char)),pszSrc,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
            }
            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCatExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPWSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        size_t cchDestLength;
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest==0) && (cbDest==0))
                    cchDestLength = 0;
                else
                    Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
                if (NT_SUCCESS(Status))
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }
            if (!pszSrc)
                pszSrc = L"";
        }
        else
        {
            Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
            if (NT_SUCCESS(Status))
            {
                pszDestEnd = pszDest + cchDestLength;
                cchRemaining = cchDest - cchDestLength;
            }
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                if (*pszSrc!=L'\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
                Status = RtlStringCopyExWorkerW(pszDestEnd,cchRemaining,(cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),pszSrc,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
            }
            if (dwFlags & STRSAFE_NULL_ON_FAILURE)
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCatNWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend)
{
    NTSTATUS Status;
    size_t cchDestLength;
    Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
    if (NT_SUCCESS(Status))
        Status = RtlStringCopyNWorkerA(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc,cchToAppend);
    return Status;
}

NTSTRSAFEAPI RtlStringCatNWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend)
{
    NTSTATUS Status;
    size_t cchDestLength;
    Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
    if (NT_SUCCESS(Status))
        Status = RtlStringCopyNWorkerW(pszDest + cchDestLength,cchDest - cchDestLength,pszSrc,cchToAppend);
    return Status;
}

NTSTRSAFEAPI RtlStringCatNExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCSTR pszSrc,size_t cchToAppend,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestLength = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
        if (cchToAppend > NTSTRSAFE_MAX_CCH)
            Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (dwFlags & STRSAFE_IGNORE_NULLS)
            {
                if (!pszDest)
                {
                    if ((cchDest==0) && (cbDest==0))
                        cchDestLength = 0;
                    else
                        Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
                    if (NT_SUCCESS(Status))
                    {
                        pszDestEnd = pszDest + cchDestLength;
                        cchRemaining = cchDest - cchDestLength;
                    }
                }
                if (!pszSrc)
                    pszSrc = "";
            }
            else
            {
                Status = RtlStringLengthWorkerA(pszDest,cchDest,&cchDestLength);
                if (NT_SUCCESS(Status))
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }
            if (NT_SUCCESS(Status))
            {
                if (cchDest==0)
                {
                    if ((cchToAppend!=0) && (*pszSrc!='\0'))
                    {
                        if (!pszDest)
                            Status = STATUS_INVALID_PARAMETER;
                        else
                            Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                    Status = RtlStringCopyNExWorkerA(pszDestEnd,cchRemaining,(cchRemaining*sizeof(char)) + (cbDest % sizeof(char)),pszSrc,cchToAppend,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringCatNExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPCWSTR pszSrc,size_t cchToAppend,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPWSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    size_t cchDestLength = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
        if (cchToAppend > NTSTRSAFE_MAX_CCH)
            Status = STATUS_INVALID_PARAMETER;
        else
        {
            if (dwFlags & STRSAFE_IGNORE_NULLS)
            {
                if (!pszDest)
                {
                    if ((cchDest==0) && (cbDest==0))
                        cchDestLength = 0;
                    else
                        Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
                    if (NT_SUCCESS(Status))
                    {
                        pszDestEnd = pszDest + cchDestLength;
                        cchRemaining = cchDest - cchDestLength;
                    }
                }
                if (!pszSrc)
                    pszSrc = L"";
            }
            else
            {
                Status = RtlStringLengthWorkerW(pszDest,cchDest,&cchDestLength);
                if (NT_SUCCESS(Status))
                {
                    pszDestEnd = pszDest + cchDestLength;
                    cchRemaining = cchDest - cchDestLength;
                }
            }
            if (NT_SUCCESS(Status))
            {
                if (cchDest==0)
                {
                    if ((cchToAppend!=0) && (*pszSrc!=L'\0'))
                    {
                        if (!pszDest)
                            Status = STATUS_INVALID_PARAMETER;
                        else
                            Status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                    Status = RtlStringCopyNExWorkerW(pszDestEnd,cchRemaining,(cchRemaining*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)),pszSrc,cchToAppend,&pszDestEnd,&cchRemaining,dwFlags & (~(STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)));
            }
        }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringVPrintfWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,STRSAFE_LPCSTR pszFormat,va_list argList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        int iRet;
        size_t cchMax;
        cchMax = cchDest - 1;
        iRet = _vsnprintf(pszDest,cchMax,pszFormat,argList);
        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            pszDest += cchMax;
            *pszDest = '\0';
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
            if (((size_t)iRet)==cchMax)
            {
                pszDest += cchMax;
                *pszDest = '\0';
            }
    }
    return Status;
}

NTSTRSAFEAPI RtlStringVPrintfWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,STRSAFE_LPCWSTR pszFormat,va_list argList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (cchDest==0)
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        int iRet;
        size_t cchMax;
        cchMax = cchDest - 1;
        iRet = _vsnwprintf(pszDest,cchMax,pszFormat,argList);
        if ((iRet < 0) || (((size_t)iRet) > cchMax))
        {
            pszDest += cchMax;
            *pszDest = L'\0';
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
            if (((size_t)iRet)==cchMax)
            {
                pszDest += cchMax;
                *pszDest = L'\0';
            }
    }
    return Status;
}

NTSTRSAFEAPI RtlStringVPrintfExWorkerA(STRSAFE_LPSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags,STRSAFE_LPCSTR pszFormat,va_list argList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest!=0) || (cbDest!=0))
                    Status = STATUS_INVALID_PARAMETER;
            }
            if (!pszFormat)
                pszFormat = "";
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszFormat!='\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                int iRet;
                size_t cchMax;
                cchMax = cchDest - 1;
                iRet = _vsnprintf(pszDest,cchMax,pszFormat,argList);
                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = '\0';
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                    if (((size_t)iRet)==cchMax)
                    {
                        pszDestEnd = pszDest + cchMax;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
                    else
                        if (((size_t)iRet) < cchMax)
                        {
                            pszDestEnd = pszDest + iRet;
                            cchRemaining = cchDest - iRet;
                            if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                            {
                                memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(char)) + (cbDest % sizeof(char)));
                            }
                        }
            }
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = '\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = '\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI RtlStringVPrintfExWorkerW(STRSAFE_LPWSTR pszDest,size_t cchDest,size_t cbDest,STRSAFE_LPWSTR *ppszDestEnd,size_t *pcchRemaining,STRSAFE_DWORD dwFlags,STRSAFE_LPCWSTR pszFormat,va_list argList)
{
    NTSTATUS Status = STATUS_SUCCESS;
    STRSAFE_LPWSTR pszDestEnd = pszDest;
    size_t cchRemaining = 0;
    if (dwFlags & (~STRSAFE_VALID_FLAGS))
        Status = STATUS_INVALID_PARAMETER;
    else
    {
        if (dwFlags & STRSAFE_IGNORE_NULLS)
        {
            if (!pszDest)
            {
                if ((cchDest!=0) || (cbDest!=0))
                    Status = STATUS_INVALID_PARAMETER;
            }
            if (!pszFormat)
                pszFormat = L"";
        }
        if (NT_SUCCESS(Status))
        {
            if (cchDest==0)
            {
                pszDestEnd = pszDest;
                cchRemaining = 0;
                if (*pszFormat!=L'\0')
                {
                    if (!pszDest)
                        Status = STATUS_INVALID_PARAMETER;
                    else
                        Status = STATUS_BUFFER_OVERFLOW;
                }
            }
            else
            {
                int iRet;
                size_t cchMax;
                cchMax = cchDest - 1;
                iRet = _vsnwprintf(pszDest,cchMax,pszFormat,argList);
                if ((iRet < 0) || (((size_t)iRet) > cchMax))
                {
                    pszDestEnd = pszDest + cchMax;
                    cchRemaining = 1;
                    *pszDestEnd = L'\0';
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                    if (((size_t)iRet)==cchMax)
                    {
                        pszDestEnd = pszDest + cchMax;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
                    else
                        if (((size_t)iRet) < cchMax)
                        {
                            pszDestEnd = pszDest + iRet;
                            cchRemaining = cchDest - iRet;
                            if (dwFlags & STRSAFE_FILL_BEHIND_NULL)
                            {
                                memset(pszDestEnd + 1,STRSAFE_GET_FILL_PATTERN(dwFlags),((cchRemaining - 1)*sizeof(wchar_t)) + (cbDest % sizeof(wchar_t)));
                            }
                        }
            }
        }
    }
    if (!NT_SUCCESS(Status))
    {
        if (pszDest)
        {
            if (dwFlags & STRSAFE_FILL_ON_FAILURE)
            {
                memset(pszDest,STRSAFE_GET_FILL_PATTERN(dwFlags),cbDest);
                if (STRSAFE_GET_FILL_PATTERN(dwFlags)==0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                }
                else
                    if (cchDest > 0)
                    {
                        pszDestEnd = pszDest + cchDest - 1;
                        cchRemaining = 1;
                        *pszDestEnd = L'\0';
                    }
            }
            if (dwFlags & (STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION))
            {
                if (cchDest > 0)
                {
                    pszDestEnd = pszDest;
                    cchRemaining = cchDest;
                    *pszDestEnd = L'\0';
                }
            }
        }
    }
    if (NT_SUCCESS(Status) || (Status==STATUS_BUFFER_OVERFLOW))
    {
        if (ppszDestEnd)
            *ppszDestEnd = pszDestEnd;
        if (pcchRemaining)
            *pcchRemaining = cchRemaining;
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringLengthWorkerA(
  _In_reads_or_z_(cchMax) STRSAFE_LPCSTR psz,
  _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) size_t *pcchLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    size_t cchMaxPrev = cchMax;
    while(cchMax && (*psz!='\0'))
    {
        psz++;
        cchMax--;
    }
    if (cchMax==0)
        Status = STATUS_INVALID_PARAMETER;
    if (pcchLength)
    {
        if (NT_SUCCESS(Status))
            *pcchLength = cchMaxPrev - cchMax;
        else
            *pcchLength = 0;
    }
    return Status;
}

NTSTRSAFEAPI
RtlStringLengthWorkerW(
  _In_reads_or_z_(cchMax) STRSAFE_LPCWSTR psz,
  _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) size_t *pcchLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    size_t cchMaxPrev = cchMax;
    while(cchMax && (*psz!=L'\0'))
    {
        psz++;
        cchMax--;
    }
    if (cchMax==0)
        Status = STATUS_INVALID_PARAMETER;
    if (pcchLength)
    {
        if (NT_SUCCESS(Status))
            *pcchLength = cchMaxPrev - cchMax;
        else
            *pcchLength = 0;
    }
    return Status;
}

#define RtlStringCopyWorkerA RtlStringCopyWorkerA_instead_use_StringCchCopyA_or_StringCchCopyExA;
#define RtlStringCopyWorkerW RtlStringCopyWorkerW_instead_use_StringCchCopyW_or_StringCchCopyExW;
#define RtlStringCopyExWorkerA RtlStringCopyExWorkerA_instead_use_StringCchCopyA_or_StringCchCopyExA;
#define RtlStringCopyExWorkerW RtlStringCopyExWorkerW_instead_use_StringCchCopyW_or_StringCchCopyExW;
#define RtlStringCatWorkerA RtlStringCatWorkerA_instead_use_StringCchCatA_or_StringCchCatExA;
#define RtlStringCatWorkerW RtlStringCatWorkerW_instead_use_StringCchCatW_or_StringCchCatExW;
#define RtlStringCatExWorkerA RtlStringCatExWorkerA_instead_use_StringCchCatA_or_StringCchCatExA;
#define RtlStringCatExWorkerW RtlStringCatExWorkerW_instead_use_StringCchCatW_or_StringCchCatExW;
#define RtlStringCatNWorkerA RtlStringCatNWorkerA_instead_use_StringCchCatNA_or_StrincCbCatNA;
#define RtlStringCatNWorkerW RtlStringCatNWorkerW_instead_use_StringCchCatNW_or_StringCbCatNW;
#define RtlStringCatNExWorkerA RtlStringCatNExWorkerA_instead_use_StringCchCatNExA_or_StringCbCatNExA;
#define RtlStringCatNExWorkerW RtlStringCatNExWorkerW_instead_use_StringCchCatNExW_or_StringCbCatNExW;
#define RtlStringVPrintfWorkerA RtlStringVPrintfWorkerA_instead_use_StringCchVPrintfA_or_StringCchVPrintfExA;
#define RtlStringVPrintfWorkerW RtlStringVPrintfWorkerW_instead_use_StringCchVPrintfW_or_StringCchVPrintfExW;
#define RtlStringVPrintfExWorkerA RtlStringVPrintfExWorkerA_instead_use_StringCchVPrintfA_or_StringCchVPrintfExA;
#define RtlStringVPrintfExWorkerW RtlStringVPrintfExWorkerW_instead_use_StringCchVPrintfW_or_StringCchVPrintfExW;
#define RtlStringLengthWorkerA RtlStringLengthWorkerA_instead_use_StringCchLengthA_or_StringCbLengthA;
#define RtlStringLengthWorkerW RtlStringLengthWorkerW_instead_use_StringCchLengthW_or_StringCbLengthW;

#ifdef _MSC_VER
#pragma warning(pop)
#endif /* _MSC_VER */

#endif /* _NTSTRSAFE_H_INCLUDED_ */
