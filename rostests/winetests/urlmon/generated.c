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
#include "urlmon.h"

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

static void test_pack_BINDINFO(void)
{
    /* BINDINFO (pack 4) */
    TEST_FIELD(BINDINFO, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(BINDINFO, LPWSTR, szExtraInfo, 4, 4, 4);
}

static void test_pack_IBindHost(void)
{
    /* IBindHost */
}

static void test_pack_IBindHostVtbl(void)
{
    /* IBindHostVtbl (pack 4) */
}

static void test_pack_IBindStatusCallback(void)
{
    /* IBindStatusCallback */
}

static void test_pack_IBindStatusCallbackVtbl(void)
{
    /* IBindStatusCallbackVtbl (pack 4) */
}

static void test_pack_IBinding(void)
{
    /* IBinding */
}

static void test_pack_IBindingVtbl(void)
{
    /* IBindingVtbl (pack 4) */
}

static void test_pack_IInternetProtocolInfo(void)
{
    /* IInternetProtocolInfo */
}

static void test_pack_IInternetProtocolInfoVtbl(void)
{
    /* IInternetProtocolInfoVtbl (pack 4) */
}

static void test_pack_IInternetSession(void)
{
    /* IInternetSession */
}

static void test_pack_IInternetSessionVtbl(void)
{
    /* IInternetSessionVtbl (pack 4) */
}

static void test_pack_IPersistMoniker(void)
{
    /* IPersistMoniker */
}

static void test_pack_IPersistMonikerVtbl(void)
{
    /* IPersistMonikerVtbl (pack 4) */
}

static void test_pack_IWinInetHttpInfo(void)
{
    /* IWinInetHttpInfo */
}

static void test_pack_IWinInetHttpInfoVtbl(void)
{
    /* IWinInetHttpInfoVtbl (pack 4) */
}

static void test_pack_IWinInetInfo(void)
{
    /* IWinInetInfo */
}

static void test_pack_IWinInetInfoVtbl(void)
{
    /* IWinInetInfoVtbl (pack 4) */
}

static void test_pack_LPBINDHOST(void)
{
    /* LPBINDHOST */
    TEST_TYPE(LPBINDHOST, 4, 4);
}

static void test_pack_LPBINDING(void)
{
    /* LPBINDING */
    TEST_TYPE(LPBINDING, 4, 4);
}

static void test_pack_LPBINDSTATUSCALLBACK(void)
{
    /* LPBINDSTATUSCALLBACK */
    TEST_TYPE(LPBINDSTATUSCALLBACK, 4, 4);
}

static void test_pack_LPIINTERNETPROTOCOLINFO(void)
{
    /* LPIINTERNETPROTOCOLINFO */
    TEST_TYPE(LPIINTERNETPROTOCOLINFO, 4, 4);
}

static void test_pack_LPIINTERNETSESSION(void)
{
    /* LPIINTERNETSESSION */
    TEST_TYPE(LPIINTERNETSESSION, 4, 4);
}

static void test_pack_LPPERSISTMONIKER(void)
{
    /* LPPERSISTMONIKER */
    TEST_TYPE(LPPERSISTMONIKER, 4, 4);
}

static void test_pack_LPREMFORMATETC(void)
{
    /* LPREMFORMATETC */
    TEST_TYPE(LPREMFORMATETC, 4, 4);
}

static void test_pack_LPREMSECURITY_ATTRIBUTES(void)
{
    /* LPREMSECURITY_ATTRIBUTES */
    TEST_TYPE(LPREMSECURITY_ATTRIBUTES, 4, 4);
}

static void test_pack_LPWININETHTTPINFO(void)
{
    /* LPWININETHTTPINFO */
    TEST_TYPE(LPWININETHTTPINFO, 4, 4);
}

static void test_pack_LPWININETINFO(void)
{
    /* LPWININETINFO */
    TEST_TYPE(LPWININETINFO, 4, 4);
}

static void test_pack_PREMSECURITY_ATTRIBUTES(void)
{
    /* PREMSECURITY_ATTRIBUTES */
    TEST_TYPE(PREMSECURITY_ATTRIBUTES, 4, 4);
}

static void test_pack_REMSECURITY_ATTRIBUTES(void)
{
    /* REMSECURITY_ATTRIBUTES (pack 4) */
    TEST_TYPE(REMSECURITY_ATTRIBUTES, 12, 4);
    TEST_FIELD(REMSECURITY_ATTRIBUTES, DWORD, nLength, 0, 4, 4);
    TEST_FIELD(REMSECURITY_ATTRIBUTES, DWORD, lpSecurityDescriptor, 4, 4, 4);
    TEST_FIELD(REMSECURITY_ATTRIBUTES, BOOL, bInheritHandle, 8, 4, 4);
}

static void test_pack_RemBINDINFO(void)
{
    /* RemBINDINFO (pack 4) */
    TEST_TYPE(RemBINDINFO, 72, 4);
    TEST_FIELD(RemBINDINFO, ULONG, cbSize, 0, 4, 4);
    TEST_FIELD(RemBINDINFO, LPWSTR, szExtraInfo, 4, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, grfBindInfoF, 8, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, dwBindVerb, 12, 4, 4);
    TEST_FIELD(RemBINDINFO, LPWSTR, szCustomVerb, 16, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, cbstgmedData, 20, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, dwOptions, 24, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, dwOptionsFlags, 28, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, dwCodePage, 32, 4, 4);
    TEST_FIELD(RemBINDINFO, REMSECURITY_ATTRIBUTES, securityAttributes, 36, 12, 4);
    TEST_FIELD(RemBINDINFO, IID, iid, 48, 16, 4);
    TEST_FIELD(RemBINDINFO, IUnknown *, pUnk, 64, 4, 4);
    TEST_FIELD(RemBINDINFO, DWORD, dwReserved, 68, 4, 4);
}

static void test_pack_RemFORMATETC(void)
{
    /* RemFORMATETC (pack 4) */
    TEST_TYPE(RemFORMATETC, 20, 4);
    TEST_FIELD(RemFORMATETC, DWORD, cfFormat, 0, 4, 4);
    TEST_FIELD(RemFORMATETC, DWORD, ptd, 4, 4, 4);
    TEST_FIELD(RemFORMATETC, DWORD, dwAspect, 8, 4, 4);
    TEST_FIELD(RemFORMATETC, LONG, lindex, 12, 4, 4);
    TEST_FIELD(RemFORMATETC, DWORD, tymed, 16, 4, 4);
}

static void test_pack(void)
{
    test_pack_BINDINFO();
    test_pack_IBindHost();
    test_pack_IBindHostVtbl();
    test_pack_IBindStatusCallback();
    test_pack_IBindStatusCallbackVtbl();
    test_pack_IBinding();
    test_pack_IBindingVtbl();
    test_pack_IInternetProtocolInfo();
    test_pack_IInternetProtocolInfoVtbl();
    test_pack_IInternetSession();
    test_pack_IInternetSessionVtbl();
    test_pack_IPersistMoniker();
    test_pack_IPersistMonikerVtbl();
    test_pack_IWinInetHttpInfo();
    test_pack_IWinInetHttpInfoVtbl();
    test_pack_IWinInetInfo();
    test_pack_IWinInetInfoVtbl();
    test_pack_LPBINDHOST();
    test_pack_LPBINDING();
    test_pack_LPBINDSTATUSCALLBACK();
    test_pack_LPIINTERNETPROTOCOLINFO();
    test_pack_LPIINTERNETSESSION();
    test_pack_LPPERSISTMONIKER();
    test_pack_LPREMFORMATETC();
    test_pack_LPREMSECURITY_ATTRIBUTES();
    test_pack_LPWININETHTTPINFO();
    test_pack_LPWININETINFO();
    test_pack_PREMSECURITY_ATTRIBUTES();
    test_pack_REMSECURITY_ATTRIBUTES();
    test_pack_RemBINDINFO();
    test_pack_RemFORMATETC();
}

START_TEST(generated)
{
    test_pack();
}
