/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"

#include "wine/test.h"

/***********************************************************************
 * Compability macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: Not sure if is possible to do without compiler extension
 *        (if type is not just a name that is, if so the normal)
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_BLOB(void)
{
    /* BLOB (pack 4) */
    TEST_TYPE(BLOB, 8, 4);
    TEST_FIELD(BLOB, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(BLOB, BYTE *, pBlobData, 4, 4, 4);
}

static void test_pack_BSTR(void)
{
    /* BSTR */
    TEST_TYPE(BSTR, 4, 4);
    TEST_TYPE_POINTER(BSTR, 2, 2);
}

static void test_pack_BSTRBLOB(void)
{
    /* BSTRBLOB (pack 4) */
    TEST_TYPE(BSTRBLOB, 8, 4);
    TEST_FIELD(BSTRBLOB, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(BSTRBLOB, BYTE *, pData, 4, 4, 4);
}

static void test_pack_BYTE_BLOB(void)
{
    /* BYTE_BLOB (pack 4) */
    TEST_TYPE(BYTE_BLOB, 8, 4);
    TEST_FIELD(BYTE_BLOB, unsigned long, clSize, 0, 4, 4);
    TEST_FIELD(BYTE_BLOB, byte[1], abData, 4, 1, 1);
}

static void test_pack_BYTE_SIZEDARR(void)
{
    /* BYTE_SIZEDARR (pack 4) */
    TEST_TYPE(BYTE_SIZEDARR, 8, 4);
    TEST_FIELD(BYTE_SIZEDARR, unsigned long, clSize, 0, 4, 4);
    TEST_FIELD(BYTE_SIZEDARR, byte *, pData, 4, 4, 4);
}

static void test_pack_CLIPDATA(void)
{
    /* CLIPDATA (pack 4) */
    TEST_TYPE(CLIPDATA, 12, 4);
    TEST_FIELD(CLIPDATA, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(CLIPDATA, long, ulClipFmt, 4, 4, 4);
    TEST_FIELD(CLIPDATA, BYTE *, pClipData, 8, 4, 4);
}

static void test_pack_CLIPFORMAT(void)
{
    /* CLIPFORMAT */
    TEST_TYPE(CLIPFORMAT, 2, 2);
    TEST_TYPE_UNSIGNED(CLIPFORMAT);
}

static void test_pack_COAUTHIDENTITY(void)
{
    /* COAUTHIDENTITY (pack 4) */
    TEST_TYPE(COAUTHIDENTITY, 28, 4);
    TEST_FIELD(COAUTHIDENTITY, USHORT *, User, 0, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, ULONG, UserLength, 4, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, USHORT *, Domain, 8, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, ULONG, DomainLength, 12, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, USHORT *, Password, 16, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, ULONG, PasswordLength, 20, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, ULONG, Flags, 24, 4, 4);
}

static void test_pack_COAUTHINFO(void)
{
    /* COAUTHINFO (pack 4) */
    TEST_TYPE(COAUTHINFO, 28, 4);
    TEST_FIELD(COAUTHINFO, DWORD, dwAuthnSvc, 0, 4, 4);
    TEST_FIELD(COAUTHINFO, DWORD, dwAuthzSvc, 4, 4, 4);
    TEST_FIELD(COAUTHINFO, LPWSTR, pwszServerPrincName, 8, 4, 4);
    TEST_FIELD(COAUTHINFO, DWORD, dwAuthnLevel, 12, 4, 4);
    TEST_FIELD(COAUTHINFO, DWORD, dwImpersonationLevel, 16, 4, 4);
    TEST_FIELD(COAUTHINFO, COAUTHIDENTITY *, pAuthIdentityData, 20, 4, 4);
    TEST_FIELD(COAUTHINFO, DWORD, dwCapabilities, 24, 4, 4);
}

static void test_pack_COSERVERINFO(void)
{
    /* COSERVERINFO (pack 4) */
    TEST_TYPE(COSERVERINFO, 16, 4);
    TEST_FIELD(COSERVERINFO, DWORD, dwReserved1, 0, 4, 4);
    TEST_FIELD(COSERVERINFO, LPWSTR, pwszName, 4, 4, 4);
    TEST_FIELD(COSERVERINFO, COAUTHINFO *, pAuthInfo, 8, 4, 4);
    TEST_FIELD(COSERVERINFO, DWORD, dwReserved2, 12, 4, 4);
}

static void test_pack_DWORD_SIZEDARR(void)
{
    /* DWORD_SIZEDARR (pack 4) */
    TEST_TYPE(DWORD_SIZEDARR, 8, 4);
    TEST_FIELD(DWORD_SIZEDARR, unsigned long, clSize, 0, 4, 4);
    TEST_FIELD(DWORD_SIZEDARR, unsigned long *, pData, 4, 4, 4);
}

static void test_pack_FLAGGED_BYTE_BLOB(void)
{
    /* FLAGGED_BYTE_BLOB (pack 4) */
    TEST_TYPE(FLAGGED_BYTE_BLOB, 12, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, unsigned long, fFlags, 0, 4, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, unsigned long, clSize, 4, 4, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, byte[1], abData, 8, 1, 1);
}

static void test_pack_FLAGGED_WORD_BLOB(void)
{
    /* FLAGGED_WORD_BLOB (pack 4) */
    TEST_TYPE(FLAGGED_WORD_BLOB, 12, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, unsigned long, fFlags, 0, 4, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, unsigned long, clSize, 4, 4, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, unsigned short[1], asData, 8, 2, 2);
}

static void test_pack_HMETAFILEPICT(void)
{
    /* HMETAFILEPICT */
    TEST_TYPE(HMETAFILEPICT, 4, 4);
}

static void test_pack_HYPER_SIZEDARR(void)
{
    /* HYPER_SIZEDARR (pack 4) */
    TEST_TYPE(HYPER_SIZEDARR, 8, 4);
    TEST_FIELD(HYPER_SIZEDARR, unsigned long, clSize, 0, 4, 4);
    TEST_FIELD(HYPER_SIZEDARR, hyper *, pData, 4, 4, 4);
}

static void test_pack_LPBLOB(void)
{
    /* LPBLOB */
    TEST_TYPE(LPBLOB, 4, 4);
    TEST_TYPE_POINTER(LPBLOB, 8, 4);
}

static void test_pack_LPBSTR(void)
{
    /* LPBSTR */
    TEST_TYPE(LPBSTR, 4, 4);
    TEST_TYPE_POINTER(LPBSTR, 4, 4);
}

static void test_pack_LPBSTRBLOB(void)
{
    /* LPBSTRBLOB */
    TEST_TYPE(LPBSTRBLOB, 4, 4);
    TEST_TYPE_POINTER(LPBSTRBLOB, 8, 4);
}

static void test_pack_LPCOLESTR(void)
{
    /* LPCOLESTR */
    TEST_TYPE(LPCOLESTR, 4, 4);
    TEST_TYPE_POINTER(LPCOLESTR, 2, 2);
}

static void test_pack_LPCY(void)
{
    /* LPCY */
    TEST_TYPE(LPCY, 4, 4);
}

static void test_pack_LPDECIMAL(void)
{
    /* LPDECIMAL */
    TEST_TYPE(LPDECIMAL, 4, 4);
}

static void test_pack_LPOLESTR(void)
{
    /* LPOLESTR */
    TEST_TYPE(LPOLESTR, 4, 4);
    TEST_TYPE_POINTER(LPOLESTR, 2, 2);
}

static void test_pack_OLECHAR(void)
{
    /* OLECHAR */
    TEST_TYPE(OLECHAR, 2, 2);
}

static void test_pack_PROPID(void)
{
    /* PROPID */
    TEST_TYPE(PROPID, 4, 4);
}

static void test_pack_RemHBITMAP(void)
{
    /* RemHBITMAP (pack 4) */
    TEST_TYPE(RemHBITMAP, 8, 4);
    TEST_FIELD(RemHBITMAP, unsigned long, cbData, 0, 4, 4);
    TEST_FIELD(RemHBITMAP, byte[1], data, 4, 1, 1);
}

static void test_pack_RemHENHMETAFILE(void)
{
    /* RemHENHMETAFILE (pack 4) */
    TEST_TYPE(RemHENHMETAFILE, 8, 4);
    TEST_FIELD(RemHENHMETAFILE, unsigned long, cbData, 0, 4, 4);
    TEST_FIELD(RemHENHMETAFILE, byte[1], data, 4, 1, 1);
}

static void test_pack_RemHGLOBAL(void)
{
    /* RemHGLOBAL (pack 4) */
    TEST_TYPE(RemHGLOBAL, 12, 4);
    TEST_FIELD(RemHGLOBAL, long, fNullHGlobal, 0, 4, 4);
    TEST_FIELD(RemHGLOBAL, unsigned long, cbData, 4, 4, 4);
    TEST_FIELD(RemHGLOBAL, byte[1], data, 8, 1, 1);
}

static void test_pack_RemHMETAFILEPICT(void)
{
    /* RemHMETAFILEPICT (pack 4) */
    TEST_TYPE(RemHMETAFILEPICT, 20, 4);
    TEST_FIELD(RemHMETAFILEPICT, long, mm, 0, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, long, xExt, 4, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, long, yExt, 8, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, unsigned long, cbData, 12, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, byte[1], data, 16, 1, 1);
}

static void test_pack_RemHPALETTE(void)
{
    /* RemHPALETTE (pack 4) */
    TEST_TYPE(RemHPALETTE, 8, 4);
    TEST_FIELD(RemHPALETTE, unsigned long, cbData, 0, 4, 4);
    TEST_FIELD(RemHPALETTE, byte[1], data, 4, 1, 1);
}

static void test_pack_SCODE(void)
{
    /* SCODE */
    TEST_TYPE(SCODE, 4, 4);
}

static void test_pack_UP_BYTE_BLOB(void)
{
    /* UP_BYTE_BLOB */
    TEST_TYPE(UP_BYTE_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_BYTE_BLOB, 8, 4);
}

static void test_pack_UP_FLAGGED_BYTE_BLOB(void)
{
    /* UP_FLAGGED_BYTE_BLOB */
    TEST_TYPE(UP_FLAGGED_BYTE_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_FLAGGED_BYTE_BLOB, 12, 4);
}

static void test_pack_UP_FLAGGED_WORD_BLOB(void)
{
    /* UP_FLAGGED_WORD_BLOB */
    TEST_TYPE(UP_FLAGGED_WORD_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_FLAGGED_WORD_BLOB, 12, 4);
}

static void test_pack_VARIANT_BOOL(void)
{
    /* VARIANT_BOOL */
    TEST_TYPE(VARIANT_BOOL, 2, 2);
    TEST_TYPE_SIGNED(VARIANT_BOOL);
}

static void test_pack_VARTYPE(void)
{
    /* VARTYPE */
    TEST_TYPE(VARTYPE, 2, 2);
    TEST_TYPE_UNSIGNED(VARTYPE);
}

static void test_pack_WORD_SIZEDARR(void)
{
    /* WORD_SIZEDARR (pack 4) */
    TEST_TYPE(WORD_SIZEDARR, 8, 4);
    TEST_FIELD(WORD_SIZEDARR, unsigned long, clSize, 0, 4, 4);
    TEST_FIELD(WORD_SIZEDARR, unsigned short *, pData, 4, 4, 4);
}

static void test_pack_remoteMETAFILEPICT(void)
{
    /* remoteMETAFILEPICT (pack 4) */
    TEST_TYPE(remoteMETAFILEPICT, 16, 4);
    TEST_FIELD(remoteMETAFILEPICT, long, mm, 0, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, long, xExt, 4, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, long, yExt, 8, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, userHMETAFILE *, hMF, 12, 4, 4);
}

static void test_pack_userBITMAP(void)
{
    /* userBITMAP (pack 4) */
    TEST_TYPE(userBITMAP, 28, 4);
    TEST_FIELD(userBITMAP, LONG, bmType, 0, 4, 4);
    TEST_FIELD(userBITMAP, LONG, bmWidth, 4, 4, 4);
    TEST_FIELD(userBITMAP, LONG, bmHeight, 8, 4, 4);
    TEST_FIELD(userBITMAP, LONG, bmWidthBytes, 12, 4, 4);
    TEST_FIELD(userBITMAP, WORD, bmPlanes, 16, 2, 2);
    TEST_FIELD(userBITMAP, WORD, bmBitsPixel, 18, 2, 2);
    TEST_FIELD(userBITMAP, ULONG, cbSize, 20, 4, 4);
    TEST_FIELD(userBITMAP, byte[1], pBuffer, 24, 1, 1);
}

static void test_pack_userCLIPFORMAT(void)
{
    /* userCLIPFORMAT (pack 4) */
    TEST_FIELD(userCLIPFORMAT, long, fContext, 0, 4, 4);
}

static void test_pack_userHBITMAP(void)
{
    /* userHBITMAP (pack 4) */
    TEST_FIELD(userHBITMAP, long, fContext, 0, 4, 4);
}

static void test_pack_userHENHMETAFILE(void)
{
    /* userHENHMETAFILE (pack 4) */
    TEST_FIELD(userHENHMETAFILE, long, fContext, 0, 4, 4);
}

static void test_pack_userHGLOBAL(void)
{
    /* userHGLOBAL (pack 4) */
    TEST_FIELD(userHGLOBAL, long, fContext, 0, 4, 4);
}

static void test_pack_userHMETAFILE(void)
{
    /* userHMETAFILE (pack 4) */
    TEST_FIELD(userHMETAFILE, long, fContext, 0, 4, 4);
}

static void test_pack_userHMETAFILEPICT(void)
{
    /* userHMETAFILEPICT (pack 4) */
    TEST_FIELD(userHMETAFILEPICT, long, fContext, 0, 4, 4);
}

static void test_pack_userHPALETTE(void)
{
    /* userHPALETTE (pack 4) */
    TEST_FIELD(userHPALETTE, long, fContext, 0, 4, 4);
}

static void test_pack_wireBSTR(void)
{
    /* wireBSTR */
    TEST_TYPE(wireBSTR, 4, 4);
    TEST_TYPE_POINTER(wireBSTR, 12, 4);
}

static void test_pack_wireCLIPFORMAT(void)
{
    /* wireCLIPFORMAT */
    TEST_TYPE(wireCLIPFORMAT, 4, 4);
}

static void test_pack_wireHBITMAP(void)
{
    /* wireHBITMAP */
    TEST_TYPE(wireHBITMAP, 4, 4);
}

static void test_pack_wireHENHMETAFILE(void)
{
    /* wireHENHMETAFILE */
    TEST_TYPE(wireHENHMETAFILE, 4, 4);
}

static void test_pack_wireHGLOBAL(void)
{
    /* wireHGLOBAL */
    TEST_TYPE(wireHGLOBAL, 4, 4);
}

static void test_pack_wireHMETAFILE(void)
{
    /* wireHMETAFILE */
    TEST_TYPE(wireHMETAFILE, 4, 4);
}

static void test_pack_wireHMETAFILEPICT(void)
{
    /* wireHMETAFILEPICT */
    TEST_TYPE(wireHMETAFILEPICT, 4, 4);
}

static void test_pack_wireHPALETTE(void)
{
    /* wireHPALETTE */
    TEST_TYPE(wireHPALETTE, 4, 4);
}

static void test_pack_CLSID(void)
{
    /* CLSID */
    TEST_TYPE(CLSID, 16, 4);
}

static void test_pack_FMTID(void)
{
    /* FMTID */
    TEST_TYPE(FMTID, 16, 4);
}

static void test_pack_GUID(void)
{
    /* GUID (pack 4) */
    TEST_TYPE(GUID, 16, 4);
    TEST_FIELD(GUID, unsigned long, Data1, 0, 4, 4);
    TEST_FIELD(GUID, unsigned short, Data2, 4, 2, 2);
    TEST_FIELD(GUID, unsigned short, Data3, 6, 2, 2);
    TEST_FIELD(GUID, unsigned char[ 8 ], Data4, 8, 8, 1);
}

static void test_pack_IID(void)
{
    /* IID */
    TEST_TYPE(IID, 16, 4);
}

static void test_pack_LPGUID(void)
{
    /* LPGUID */
    TEST_TYPE(LPGUID, 4, 4);
    TEST_TYPE_POINTER(LPGUID, 16, 4);
}

static void test_pack_APPBARDATA(void)
{
    /* APPBARDATA (pack 1) */
    TEST_TYPE(APPBARDATA, 36, 1);
    TEST_FIELD(APPBARDATA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(APPBARDATA, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(APPBARDATA, UINT, uCallbackMessage, 8, 4, 1);
    TEST_FIELD(APPBARDATA, UINT, uEdge, 12, 4, 1);
    TEST_FIELD(APPBARDATA, RECT, rc, 16, 16, 1);
    TEST_FIELD(APPBARDATA, LPARAM, lParam, 32, 4, 1);
}

static void test_pack_DRAGINFOA(void)
{
    /* DRAGINFOA (pack 1) */
    TEST_TYPE(DRAGINFOA, 24, 1);
    TEST_FIELD(DRAGINFOA, UINT, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOA, POINT, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOA, BOOL, fNC, 12, 4, 1);
    TEST_FIELD(DRAGINFOA, LPSTR, lpFileList, 16, 4, 1);
    TEST_FIELD(DRAGINFOA, DWORD, grfKeyState, 20, 4, 1);
}

static void test_pack_DRAGINFOW(void)
{
    /* DRAGINFOW (pack 1) */
    TEST_TYPE(DRAGINFOW, 24, 1);
    TEST_FIELD(DRAGINFOW, UINT, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOW, POINT, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOW, BOOL, fNC, 12, 4, 1);
    TEST_FIELD(DRAGINFOW, LPWSTR, lpFileList, 16, 4, 1);
    TEST_FIELD(DRAGINFOW, DWORD, grfKeyState, 20, 4, 1);
}

static void test_pack_FILEOP_FLAGS(void)
{
    /* FILEOP_FLAGS */
    TEST_TYPE(FILEOP_FLAGS, 2, 2);
    TEST_TYPE_UNSIGNED(FILEOP_FLAGS);
}

static void test_pack_LPDRAGINFOA(void)
{
    /* LPDRAGINFOA */
    TEST_TYPE(LPDRAGINFOA, 4, 4);
    TEST_TYPE_POINTER(LPDRAGINFOA, 24, 1);
}

static void test_pack_LPDRAGINFOW(void)
{
    /* LPDRAGINFOW */
    TEST_TYPE(LPDRAGINFOW, 4, 4);
    TEST_TYPE_POINTER(LPDRAGINFOW, 24, 1);
}

static void test_pack_LPSHELLEXECUTEINFOA(void)
{
    /* LPSHELLEXECUTEINFOA */
    TEST_TYPE(LPSHELLEXECUTEINFOA, 4, 4);
}

static void test_pack_LPSHELLEXECUTEINFOW(void)
{
    /* LPSHELLEXECUTEINFOW */
    TEST_TYPE(LPSHELLEXECUTEINFOW, 4, 4);
}

static void test_pack_LPSHFILEOPSTRUCTA(void)
{
    /* LPSHFILEOPSTRUCTA */
    TEST_TYPE(LPSHFILEOPSTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPSHFILEOPSTRUCTA, 30, 1);
}

static void test_pack_LPSHFILEOPSTRUCTW(void)
{
    /* LPSHFILEOPSTRUCTW */
    TEST_TYPE(LPSHFILEOPSTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPSHFILEOPSTRUCTW, 30, 1);
}

static void test_pack_LPSHNAMEMAPPINGA(void)
{
    /* LPSHNAMEMAPPINGA */
    TEST_TYPE(LPSHNAMEMAPPINGA, 4, 4);
    TEST_TYPE_POINTER(LPSHNAMEMAPPINGA, 16, 1);
}

static void test_pack_LPSHNAMEMAPPINGW(void)
{
    /* LPSHNAMEMAPPINGW */
    TEST_TYPE(LPSHNAMEMAPPINGW, 4, 4);
    TEST_TYPE_POINTER(LPSHNAMEMAPPINGW, 16, 1);
}

static void test_pack_NOTIFYICONDATAA(void)
{
    /* NOTIFYICONDATAA (pack 1) */
    TEST_FIELD(NOTIFYICONDATAA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, HICON, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, CHAR[128], szTip, 24, 128, 1);
    TEST_FIELD(NOTIFYICONDATAA, DWORD, dwState, 152, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, DWORD, dwStateMask, 156, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, CHAR[256], szInfo, 160, 256, 1);
}

static void test_pack_NOTIFYICONDATAW(void)
{
    /* NOTIFYICONDATAW (pack 1) */
    TEST_FIELD(NOTIFYICONDATAW, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, HICON, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, WCHAR[128], szTip, 24, 256, 1);
    TEST_FIELD(NOTIFYICONDATAW, DWORD, dwState, 280, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, DWORD, dwStateMask, 284, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, WCHAR[256], szInfo, 288, 512, 1);
}

static void test_pack_PAPPBARDATA(void)
{
    /* PAPPBARDATA */
    TEST_TYPE(PAPPBARDATA, 4, 4);
    TEST_TYPE_POINTER(PAPPBARDATA, 36, 1);
}

static void test_pack_PNOTIFYICONDATAA(void)
{
    /* PNOTIFYICONDATAA */
    TEST_TYPE(PNOTIFYICONDATAA, 4, 4);
}

static void test_pack_PNOTIFYICONDATAW(void)
{
    /* PNOTIFYICONDATAW */
    TEST_TYPE(PNOTIFYICONDATAW, 4, 4);
}

static void test_pack_PRINTEROP_FLAGS(void)
{
    /* PRINTEROP_FLAGS */
    TEST_TYPE(PRINTEROP_FLAGS, 2, 2);
    TEST_TYPE_UNSIGNED(PRINTEROP_FLAGS);
}

static void test_pack_SHELLEXECUTEINFOA(void)
{
    /* SHELLEXECUTEINFOA (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, ULONG, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, HWND, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, INT, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, HINSTANCE, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPVOID, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpClass, 40, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, HKEY, hkeyClass, 44, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, DWORD, dwHotKey, 48, 4, 1);
}

static void test_pack_SHELLEXECUTEINFOW(void)
{
    /* SHELLEXECUTEINFOW (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOW, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, ULONG, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, HWND, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, INT, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, HINSTANCE, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPVOID, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpClass, 40, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, HKEY, hkeyClass, 44, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, DWORD, dwHotKey, 48, 4, 1);
}

static void test_pack_SHFILEINFOA(void)
{
    /* SHFILEINFOA (pack 1) */
    TEST_TYPE(SHFILEINFOA, 352, 1);
    TEST_FIELD(SHFILEINFOA, HICON, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOA, int, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOA, DWORD, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOA, CHAR[MAX_PATH], szDisplayName, 12, 260, 1);
    TEST_FIELD(SHFILEINFOA, CHAR[80], szTypeName, 272, 80, 1);
}

static void test_pack_SHFILEINFOW(void)
{
    /* SHFILEINFOW (pack 1) */
    TEST_TYPE(SHFILEINFOW, 692, 1);
    TEST_FIELD(SHFILEINFOW, HICON, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOW, int, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOW, DWORD, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOW, WCHAR[MAX_PATH], szDisplayName, 12, 520, 1);
    TEST_FIELD(SHFILEINFOW, WCHAR[80], szTypeName, 532, 160, 1);
}

static void test_pack_SHFILEOPSTRUCTA(void)
{
    /* SHFILEOPSTRUCTA (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTA, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, HWND, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, UINT, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, FILEOP_FLAGS, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, BOOL, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPVOID, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_SHFILEOPSTRUCTW(void)
{
    /* SHFILEOPSTRUCTW (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTW, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, HWND, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, UINT, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, FILEOP_FLAGS, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, BOOL, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPVOID, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_SHNAMEMAPPINGA(void)
{
    /* SHNAMEMAPPINGA (pack 1) */
    TEST_TYPE(SHNAMEMAPPINGA, 16, 1);
    TEST_FIELD(SHNAMEMAPPINGA, LPSTR, pszOldPath, 0, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, LPSTR, pszNewPath, 4, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, int, cchOldPath, 8, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, int, cchNewPath, 12, 4, 1);
}

static void test_pack_SHNAMEMAPPINGW(void)
{
    /* SHNAMEMAPPINGW (pack 1) */
    TEST_TYPE(SHNAMEMAPPINGW, 16, 1);
    TEST_FIELD(SHNAMEMAPPINGW, LPWSTR, pszOldPath, 0, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, LPWSTR, pszNewPath, 4, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, int, cchOldPath, 8, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, int, cchNewPath, 12, 4, 1);
}

static void test_pack_ITEMIDLIST(void)
{
    /* ITEMIDLIST (pack 1) */
    TEST_TYPE(ITEMIDLIST, 3, 1);
    TEST_FIELD(ITEMIDLIST, SHITEMID, mkid, 0, 3, 1);
}

static void test_pack_LPCITEMIDLIST(void)
{
    /* LPCITEMIDLIST */
    TEST_TYPE(LPCITEMIDLIST, 4, 4);
    TEST_TYPE_POINTER(LPCITEMIDLIST, 3, 1);
}

static void test_pack_LPCSHITEMID(void)
{
    /* LPCSHITEMID */
    TEST_TYPE(LPCSHITEMID, 4, 4);
    TEST_TYPE_POINTER(LPCSHITEMID, 3, 1);
}

static void test_pack_LPITEMIDLIST(void)
{
    /* LPITEMIDLIST */
    TEST_TYPE(LPITEMIDLIST, 4, 4);
    TEST_TYPE_POINTER(LPITEMIDLIST, 3, 1);
}

static void test_pack_LPSHELLDETAILS(void)
{
    /* LPSHELLDETAILS */
    TEST_TYPE(LPSHELLDETAILS, 4, 4);
}

static void test_pack_LPSHITEMID(void)
{
    /* LPSHITEMID */
    TEST_TYPE(LPSHITEMID, 4, 4);
    TEST_TYPE_POINTER(LPSHITEMID, 3, 1);
}

static void test_pack_LPSTRRET(void)
{
    /* LPSTRRET */
    TEST_TYPE(LPSTRRET, 4, 4);
}

static void test_pack_SHELLDETAILS(void)
{
    /* SHELLDETAILS (pack 1) */
    TEST_FIELD(SHELLDETAILS, int, fmt, 0, 4, 1);
    TEST_FIELD(SHELLDETAILS, int, cxChar, 4, 4, 1);
}

static void test_pack_SHITEMID(void)
{
    /* SHITEMID (pack 1) */
    TEST_TYPE(SHITEMID, 3, 1);
    TEST_FIELD(SHITEMID, WORD, cb, 0, 2, 1);
    TEST_FIELD(SHITEMID, BYTE[1], abID, 2, 1, 1);
}

static void test_pack_STRRET(void)
{
    /* STRRET (pack 4) */
    TEST_FIELD(STRRET, UINT, uType, 0, 4, 4);
}

static void test_pack_AUTO_SCROLL_DATA(void)
{
    /* AUTO_SCROLL_DATA (pack 1) */
    TEST_TYPE(AUTO_SCROLL_DATA, 48, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, int, iNextSample, 0, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, DWORD, dwLastScroll, 4, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, BOOL, bFull, 8, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, POINT[NUM_POINTS], pts, 12, 24, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, DWORD[NUM_POINTS], dwTimes, 36, 12, 1);
}

static void test_pack_BFFCALLBACK(void)
{
    /* BFFCALLBACK */
    TEST_TYPE(BFFCALLBACK, 4, 4);
}

static void test_pack_BROWSEINFOA(void)
{
    /* BROWSEINFOA (pack 8) */
    TEST_TYPE(BROWSEINFOA, 32, 4);
    TEST_FIELD(BROWSEINFOA, HWND, hwndOwner, 0, 4, 4);
    TEST_FIELD(BROWSEINFOA, LPCITEMIDLIST, pidlRoot, 4, 4, 4);
    TEST_FIELD(BROWSEINFOA, LPSTR, pszDisplayName, 8, 4, 4);
    TEST_FIELD(BROWSEINFOA, LPCSTR, lpszTitle, 12, 4, 4);
    TEST_FIELD(BROWSEINFOA, UINT, ulFlags, 16, 4, 4);
    TEST_FIELD(BROWSEINFOA, BFFCALLBACK, lpfn, 20, 4, 4);
    TEST_FIELD(BROWSEINFOA, LPARAM, lParam, 24, 4, 4);
    TEST_FIELD(BROWSEINFOA, INT, iImage, 28, 4, 4);
}

static void test_pack_BROWSEINFOW(void)
{
    /* BROWSEINFOW (pack 8) */
    TEST_TYPE(BROWSEINFOW, 32, 4);
    TEST_FIELD(BROWSEINFOW, HWND, hwndOwner, 0, 4, 4);
    TEST_FIELD(BROWSEINFOW, LPCITEMIDLIST, pidlRoot, 4, 4, 4);
    TEST_FIELD(BROWSEINFOW, LPWSTR, pszDisplayName, 8, 4, 4);
    TEST_FIELD(BROWSEINFOW, LPCWSTR, lpszTitle, 12, 4, 4);
    TEST_FIELD(BROWSEINFOW, UINT, ulFlags, 16, 4, 4);
    TEST_FIELD(BROWSEINFOW, BFFCALLBACK, lpfn, 20, 4, 4);
    TEST_FIELD(BROWSEINFOW, LPARAM, lParam, 24, 4, 4);
    TEST_FIELD(BROWSEINFOW, INT, iImage, 28, 4, 4);
}

static void test_pack_CABINETSTATE(void)
{
    /* CABINETSTATE (pack 1) */
    TEST_TYPE(CABINETSTATE, 12, 1);
    TEST_FIELD(CABINETSTATE, WORD, cLength, 0, 2, 1);
    TEST_FIELD(CABINETSTATE, WORD, nVersion, 2, 2, 1);
    TEST_FIELD(CABINETSTATE, UINT, fMenuEnumFilter, 8, 4, 1);
}

static void test_pack_CIDA(void)
{
    /* CIDA (pack 1) */
    TEST_TYPE(CIDA, 8, 1);
    TEST_FIELD(CIDA, UINT, cidl, 0, 4, 1);
    TEST_FIELD(CIDA, UINT[1], aoffset, 4, 4, 1);
}

static void test_pack_CSFV(void)
{
    /* CSFV (pack 1) */
    TEST_FIELD(CSFV, UINT, cbSize, 0, 4, 1);
    TEST_FIELD(CSFV, IShellFolder*, pshf, 4, 4, 1);
    TEST_FIELD(CSFV, IShellView*, psvOuter, 8, 4, 1);
    TEST_FIELD(CSFV, LPCITEMIDLIST, pidl, 12, 4, 1);
    TEST_FIELD(CSFV, LONG, lEvents, 16, 4, 1);
    TEST_FIELD(CSFV, LPFNVIEWCALLBACK, pfnCallback, 20, 4, 1);
}

static void test_pack_DROPFILES(void)
{
    /* DROPFILES (pack 1) */
    TEST_TYPE(DROPFILES, 20, 1);
    TEST_FIELD(DROPFILES, DWORD, pFiles, 0, 4, 1);
    TEST_FIELD(DROPFILES, POINT, pt, 4, 8, 1);
    TEST_FIELD(DROPFILES, BOOL, fNC, 12, 4, 1);
    TEST_FIELD(DROPFILES, BOOL, fWide, 16, 4, 1);
}

static void test_pack_FILEDESCRIPTORA(void)
{
    /* FILEDESCRIPTORA (pack 1) */
    TEST_TYPE(FILEDESCRIPTORA, 332, 1);
    TEST_FIELD(FILEDESCRIPTORA, DWORD, dwFlags, 0, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, CLSID, clsid, 4, 16, 1);
    TEST_FIELD(FILEDESCRIPTORA, SIZEL, sizel, 20, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, POINTL, pointl, 28, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, DWORD, dwFileAttributes, 36, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, FILETIME, ftCreationTime, 40, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, FILETIME, ftLastAccessTime, 48, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, FILETIME, ftLastWriteTime, 56, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, DWORD, nFileSizeHigh, 64, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, DWORD, nFileSizeLow, 68, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, CHAR[MAX_PATH], cFileName, 72, 260, 1);
}

static void test_pack_FILEDESCRIPTORW(void)
{
    /* FILEDESCRIPTORW (pack 1) */
    TEST_TYPE(FILEDESCRIPTORW, 592, 1);
    TEST_FIELD(FILEDESCRIPTORW, DWORD, dwFlags, 0, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, CLSID, clsid, 4, 16, 1);
    TEST_FIELD(FILEDESCRIPTORW, SIZEL, sizel, 20, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, POINTL, pointl, 28, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, DWORD, dwFileAttributes, 36, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, FILETIME, ftCreationTime, 40, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, FILETIME, ftLastAccessTime, 48, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, FILETIME, ftLastWriteTime, 56, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, DWORD, nFileSizeHigh, 64, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, DWORD, nFileSizeLow, 68, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, WCHAR[MAX_PATH], cFileName, 72, 520, 1);
}

static void test_pack_FILEGROUPDESCRIPTORA(void)
{
    /* FILEGROUPDESCRIPTORA (pack 1) */
    TEST_TYPE(FILEGROUPDESCRIPTORA, 336, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORA, UINT, cItems, 0, 4, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORA, FILEDESCRIPTORA[1], fgd, 4, 332, 1);
}

static void test_pack_FILEGROUPDESCRIPTORW(void)
{
    /* FILEGROUPDESCRIPTORW (pack 1) */
    TEST_TYPE(FILEGROUPDESCRIPTORW, 596, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORW, UINT, cItems, 0, 4, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORW, FILEDESCRIPTORW[1], fgd, 4, 592, 1);
}

static void test_pack_IFileSystemBindData(void)
{
    /* IFileSystemBindData */
}

static void test_pack_IFileSystemBindDataVtbl(void)
{
    /* IFileSystemBindDataVtbl */
}

static void test_pack_IShellChangeNotify(void)
{
    /* IShellChangeNotify */
}

static void test_pack_IShellIcon(void)
{
    /* IShellIcon */
}

static void test_pack_LPBROWSEINFOA(void)
{
    /* LPBROWSEINFOA */
    TEST_TYPE(LPBROWSEINFOA, 4, 4);
    TEST_TYPE_POINTER(LPBROWSEINFOA, 32, 4);
}

static void test_pack_LPBROWSEINFOW(void)
{
    /* LPBROWSEINFOW */
    TEST_TYPE(LPBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(LPBROWSEINFOW, 32, 4);
}

static void test_pack_LPCABINETSTATE(void)
{
    /* LPCABINETSTATE */
    TEST_TYPE(LPCABINETSTATE, 4, 4);
    TEST_TYPE_POINTER(LPCABINETSTATE, 12, 1);
}

static void test_pack_LPCSFV(void)
{
    /* LPCSFV */
    TEST_TYPE(LPCSFV, 4, 4);
}

static void test_pack_LPDROPFILES(void)
{
    /* LPDROPFILES */
    TEST_TYPE(LPDROPFILES, 4, 4);
    TEST_TYPE_POINTER(LPDROPFILES, 20, 1);
}

static void test_pack_LPFILEDESCRIPTORA(void)
{
    /* LPFILEDESCRIPTORA */
    TEST_TYPE(LPFILEDESCRIPTORA, 4, 4);
    TEST_TYPE_POINTER(LPFILEDESCRIPTORA, 332, 1);
}

static void test_pack_LPFILEDESCRIPTORW(void)
{
    /* LPFILEDESCRIPTORW */
    TEST_TYPE(LPFILEDESCRIPTORW, 4, 4);
    TEST_TYPE_POINTER(LPFILEDESCRIPTORW, 592, 1);
}

static void test_pack_LPFILEGROUPDESCRIPTORA(void)
{
    /* LPFILEGROUPDESCRIPTORA */
    TEST_TYPE(LPFILEGROUPDESCRIPTORA, 4, 4);
    TEST_TYPE_POINTER(LPFILEGROUPDESCRIPTORA, 336, 1);
}

static void test_pack_LPFILEGROUPDESCRIPTORW(void)
{
    /* LPFILEGROUPDESCRIPTORW */
    TEST_TYPE(LPFILEGROUPDESCRIPTORW, 4, 4);
    TEST_TYPE_POINTER(LPFILEGROUPDESCRIPTORW, 596, 1);
}

static void test_pack_LPFNVIEWCALLBACK(void)
{
    /* LPFNVIEWCALLBACK */
    TEST_TYPE(LPFNVIEWCALLBACK, 4, 4);
}

static void test_pack_LPIDA(void)
{
    /* LPIDA */
    TEST_TYPE(LPIDA, 4, 4);
    TEST_TYPE_POINTER(LPIDA, 8, 1);
}

static void test_pack_LPQCMINFO(void)
{
    /* LPQCMINFO */
    TEST_TYPE(LPQCMINFO, 4, 4);
    TEST_TYPE_POINTER(LPQCMINFO, 20, 4);
}

static void test_pack_LPSHChangeDWORDAsIDList(void)
{
    /* LPSHChangeDWORDAsIDList */
    TEST_TYPE(LPSHChangeDWORDAsIDList, 4, 4);
    TEST_TYPE_POINTER(LPSHChangeDWORDAsIDList, 12, 1);
}

static void test_pack_LPSHChangeProductKeyAsIDList(void)
{
    /* LPSHChangeProductKeyAsIDList */
    TEST_TYPE(LPSHChangeProductKeyAsIDList, 4, 4);
    TEST_TYPE_POINTER(LPSHChangeProductKeyAsIDList, 82, 1);
}

static void test_pack_LPSHDESCRIPTIONID(void)
{
    /* LPSHDESCRIPTIONID */
    TEST_TYPE(LPSHDESCRIPTIONID, 4, 4);
    TEST_TYPE_POINTER(LPSHDESCRIPTIONID, 20, 4);
}

static void test_pack_LPSHELLFLAGSTATE(void)
{
    /* LPSHELLFLAGSTATE */
    TEST_TYPE(LPSHELLFLAGSTATE, 4, 4);
    TEST_TYPE_POINTER(LPSHELLFLAGSTATE, 4, 1);
}

static void test_pack_LPSHELLSTATE(void)
{
    /* LPSHELLSTATE */
    TEST_TYPE(LPSHELLSTATE, 4, 4);
    TEST_TYPE_POINTER(LPSHELLSTATE, 32, 1);
}

static void test_pack_LPTBINFO(void)
{
    /* LPTBINFO */
    TEST_TYPE(LPTBINFO, 4, 4);
    TEST_TYPE_POINTER(LPTBINFO, 8, 4);
}

static void test_pack_PBROWSEINFOA(void)
{
    /* PBROWSEINFOA */
    TEST_TYPE(PBROWSEINFOA, 4, 4);
    TEST_TYPE_POINTER(PBROWSEINFOA, 32, 4);
}

static void test_pack_PBROWSEINFOW(void)
{
    /* PBROWSEINFOW */
    TEST_TYPE(PBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(PBROWSEINFOW, 32, 4);
}

static void test_pack_QCMINFO(void)
{
    /* QCMINFO (pack 8) */
    TEST_TYPE(QCMINFO, 20, 4);
    TEST_FIELD(QCMINFO, HMENU, hmenu, 0, 4, 4);
    TEST_FIELD(QCMINFO, UINT, indexMenu, 4, 4, 4);
    TEST_FIELD(QCMINFO, UINT, idCmdFirst, 8, 4, 4);
    TEST_FIELD(QCMINFO, UINT, idCmdLast, 12, 4, 4);
    TEST_FIELD(QCMINFO, QCMINFO_IDMAP const*, pIdMap, 16, 4, 4);
}

static void test_pack_QCMINFO_IDMAP(void)
{
    /* QCMINFO_IDMAP (pack 8) */
    TEST_TYPE(QCMINFO_IDMAP, 12, 4);
    TEST_FIELD(QCMINFO_IDMAP, UINT, nMaxIds, 0, 4, 4);
    TEST_FIELD(QCMINFO_IDMAP, QCMINFO_IDMAP_PLACEMENT[1], pIdList, 4, 8, 4);
}

static void test_pack_QCMINFO_IDMAP_PLACEMENT(void)
{
    /* QCMINFO_IDMAP_PLACEMENT (pack 8) */
    TEST_TYPE(QCMINFO_IDMAP_PLACEMENT, 8, 4);
    TEST_FIELD(QCMINFO_IDMAP_PLACEMENT, UINT, id, 0, 4, 4);
    TEST_FIELD(QCMINFO_IDMAP_PLACEMENT, UINT, fFlags, 4, 4, 4);
}

static void test_pack_SHChangeDWORDAsIDList(void)
{
    /* SHChangeDWORDAsIDList (pack 1) */
    TEST_TYPE(SHChangeDWORDAsIDList, 12, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, USHORT, cb, 0, 2, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, DWORD, dwItem1, 2, 4, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, DWORD, dwItem2, 6, 4, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, USHORT, cbZero, 10, 2, 1);
}

static void test_pack_SHChangeNotifyEntry(void)
{
    /* SHChangeNotifyEntry (pack 1) */
    TEST_TYPE(SHChangeNotifyEntry, 8, 1);
    TEST_FIELD(SHChangeNotifyEntry, LPCITEMIDLIST, pidl, 0, 4, 1);
    TEST_FIELD(SHChangeNotifyEntry, BOOL, fRecursive, 4, 4, 1);
}

static void test_pack_SHChangeProductKeyAsIDList(void)
{
    /* SHChangeProductKeyAsIDList (pack 1) */
    TEST_TYPE(SHChangeProductKeyAsIDList, 82, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, USHORT, cb, 0, 2, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, WCHAR[39], wszProductKey, 2, 78, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, USHORT, cbZero, 80, 2, 1);
}

static void test_pack_SHDESCRIPTIONID(void)
{
    /* SHDESCRIPTIONID (pack 8) */
    TEST_TYPE(SHDESCRIPTIONID, 20, 4);
    TEST_FIELD(SHDESCRIPTIONID, DWORD, dwDescriptionId, 0, 4, 4);
    TEST_FIELD(SHDESCRIPTIONID, CLSID, clsid, 4, 16, 4);
}

static void test_pack_SHELLFLAGSTATE(void)
{
    /* SHELLFLAGSTATE (pack 1) */
    TEST_TYPE(SHELLFLAGSTATE, 4, 1);
}

static void test_pack_SHELLSTATE(void)
{
    /* SHELLSTATE (pack 1) */
    TEST_TYPE(SHELLSTATE, 32, 1);
    TEST_FIELD(SHELLSTATE, DWORD, dwWin95Unused, 4, 4, 1);
    TEST_FIELD(SHELLSTATE, UINT, uWin95Unused, 8, 4, 1);
    TEST_FIELD(SHELLSTATE, LONG, lParamSort, 12, 4, 1);
    TEST_FIELD(SHELLSTATE, int, iSortDirection, 16, 4, 1);
    TEST_FIELD(SHELLSTATE, UINT, version, 20, 4, 1);
    TEST_FIELD(SHELLSTATE, UINT, uNotUsed, 24, 4, 1);
}

static void test_pack_SHELLVIEWID(void)
{
    /* SHELLVIEWID */
    TEST_TYPE(SHELLVIEWID, 16, 4);
}

static void test_pack_TBINFO(void)
{
    /* TBINFO (pack 8) */
    TEST_TYPE(TBINFO, 8, 4);
    TEST_FIELD(TBINFO, UINT, cbuttons, 0, 4, 4);
    TEST_FIELD(TBINFO, UINT, uFlags, 4, 4, 4);
}

static void test_pack(void)
{
    test_pack_APPBARDATA();
    test_pack_AUTO_SCROLL_DATA();
    test_pack_BFFCALLBACK();
    test_pack_BLOB();
    test_pack_BROWSEINFOA();
    test_pack_BROWSEINFOW();
    test_pack_BSTR();
    test_pack_BSTRBLOB();
    test_pack_BYTE_BLOB();
    test_pack_BYTE_SIZEDARR();
    test_pack_CABINETSTATE();
    test_pack_CIDA();
    test_pack_CLIPDATA();
    test_pack_CLIPFORMAT();
    test_pack_CLSID();
    test_pack_COAUTHIDENTITY();
    test_pack_COAUTHINFO();
    test_pack_COSERVERINFO();
    test_pack_CSFV();
    test_pack_DRAGINFOA();
    test_pack_DRAGINFOW();
    test_pack_DROPFILES();
    test_pack_DWORD_SIZEDARR();
    test_pack_FILEDESCRIPTORA();
    test_pack_FILEDESCRIPTORW();
    test_pack_FILEGROUPDESCRIPTORA();
    test_pack_FILEGROUPDESCRIPTORW();
    test_pack_FILEOP_FLAGS();
    test_pack_FLAGGED_BYTE_BLOB();
    test_pack_FLAGGED_WORD_BLOB();
    test_pack_FMTID();
    test_pack_GUID();
    test_pack_HMETAFILEPICT();
    test_pack_HYPER_SIZEDARR();
    test_pack_IFileSystemBindData();
    test_pack_IFileSystemBindDataVtbl();
    test_pack_IID();
    test_pack_IShellChangeNotify();
    test_pack_IShellIcon();
    test_pack_ITEMIDLIST();
    test_pack_LPBLOB();
    test_pack_LPBROWSEINFOA();
    test_pack_LPBROWSEINFOW();
    test_pack_LPBSTR();
    test_pack_LPBSTRBLOB();
    test_pack_LPCABINETSTATE();
    test_pack_LPCITEMIDLIST();
    test_pack_LPCOLESTR();
    test_pack_LPCSFV();
    test_pack_LPCSHITEMID();
    test_pack_LPCY();
    test_pack_LPDECIMAL();
    test_pack_LPDRAGINFOA();
    test_pack_LPDRAGINFOW();
    test_pack_LPDROPFILES();
    test_pack_LPFILEDESCRIPTORA();
    test_pack_LPFILEDESCRIPTORW();
    test_pack_LPFILEGROUPDESCRIPTORA();
    test_pack_LPFILEGROUPDESCRIPTORW();
    test_pack_LPFNVIEWCALLBACK();
    test_pack_LPGUID();
    test_pack_LPIDA();
    test_pack_LPITEMIDLIST();
    test_pack_LPOLESTR();
    test_pack_LPQCMINFO();
    test_pack_LPSHChangeDWORDAsIDList();
    test_pack_LPSHChangeProductKeyAsIDList();
    test_pack_LPSHDESCRIPTIONID();
    test_pack_LPSHELLDETAILS();
    test_pack_LPSHELLEXECUTEINFOA();
    test_pack_LPSHELLEXECUTEINFOW();
    test_pack_LPSHELLFLAGSTATE();
    test_pack_LPSHELLSTATE();
    test_pack_LPSHFILEOPSTRUCTA();
    test_pack_LPSHFILEOPSTRUCTW();
    test_pack_LPSHITEMID();
    test_pack_LPSHNAMEMAPPINGA();
    test_pack_LPSHNAMEMAPPINGW();
    test_pack_LPSTRRET();
    test_pack_LPTBINFO();
    test_pack_NOTIFYICONDATAA();
    test_pack_NOTIFYICONDATAW();
    test_pack_OLECHAR();
    test_pack_PAPPBARDATA();
    test_pack_PBROWSEINFOA();
    test_pack_PBROWSEINFOW();
    test_pack_PNOTIFYICONDATAA();
    test_pack_PNOTIFYICONDATAW();
    test_pack_PRINTEROP_FLAGS();
    test_pack_PROPID();
    test_pack_QCMINFO();
    test_pack_QCMINFO_IDMAP();
    test_pack_QCMINFO_IDMAP_PLACEMENT();
    test_pack_RemHBITMAP();
    test_pack_RemHENHMETAFILE();
    test_pack_RemHGLOBAL();
    test_pack_RemHMETAFILEPICT();
    test_pack_RemHPALETTE();
    test_pack_SCODE();
    test_pack_SHChangeDWORDAsIDList();
    test_pack_SHChangeNotifyEntry();
    test_pack_SHChangeProductKeyAsIDList();
    test_pack_SHDESCRIPTIONID();
    test_pack_SHELLDETAILS();
    test_pack_SHELLEXECUTEINFOA();
    test_pack_SHELLEXECUTEINFOW();
    test_pack_SHELLFLAGSTATE();
    test_pack_SHELLSTATE();
    test_pack_SHELLVIEWID();
    test_pack_SHFILEINFOA();
    test_pack_SHFILEINFOW();
    test_pack_SHFILEOPSTRUCTA();
    test_pack_SHFILEOPSTRUCTW();
    test_pack_SHITEMID();
    test_pack_SHNAMEMAPPINGA();
    test_pack_SHNAMEMAPPINGW();
    test_pack_STRRET();
    test_pack_TBINFO();
    test_pack_UP_BYTE_BLOB();
    test_pack_UP_FLAGGED_BYTE_BLOB();
    test_pack_UP_FLAGGED_WORD_BLOB();
    test_pack_VARIANT_BOOL();
    test_pack_VARTYPE();
    test_pack_WORD_SIZEDARR();
    test_pack_remoteMETAFILEPICT();
    test_pack_userBITMAP();
    test_pack_userCLIPFORMAT();
    test_pack_userHBITMAP();
    test_pack_userHENHMETAFILE();
    test_pack_userHGLOBAL();
    test_pack_userHMETAFILE();
    test_pack_userHMETAFILEPICT();
    test_pack_userHPALETTE();
    test_pack_wireBSTR();
    test_pack_wireCLIPFORMAT();
    test_pack_wireHBITMAP();
    test_pack_wireHENHMETAFILE();
    test_pack_wireHGLOBAL();
    test_pack_wireHMETAFILE();
    test_pack_wireHMETAFILEPICT();
    test_pack_wireHPALETTE();
}

START_TEST(generated)
{
    test_pack();
}
