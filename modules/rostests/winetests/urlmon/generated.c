/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINE_NOWINSOCK

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "urlmon.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__) || defined(__clang__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: May not be possible without a compiler extension
 *        (if type is not just a name that is, otherwise the normal
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

#define TEST_TYPE_SIZE(type, size)              C_ASSERT(sizeof(type) == size);

#ifdef TYPE_ALIGNMENT
# define TEST_TYPE_ALIGN(type, align)           C_ASSERT(TYPE_ALIGNMENT(type) == align);
#else
# define TEST_TYPE_ALIGN(type, align)
#endif

#ifdef _TYPE_ALIGNMENT
# define TEST_TARGET_ALIGN(type, align)         C_ASSERT(_TYPE_ALIGNMENT(*(type)0) == align);
# define TEST_FIELD_ALIGN(type, field, align)   C_ASSERT(_TYPE_ALIGNMENT(((type*)0)->field) == align);
#else
# define TEST_TARGET_ALIGN(type, align)
# define TEST_FIELD_ALIGN(type, field, align)
#endif

#define TEST_FIELD_OFFSET(type, field, offset)  C_ASSERT(FIELD_OFFSET(type, field) == offset);

#define TEST_TARGET_SIZE(type, size)            TEST_TYPE_SIZE(*(type)0, size)
#define TEST_FIELD_SIZE(type, field, size)      TEST_TYPE_SIZE((((type*)0)->field), size)
#define TEST_TYPE_SIGNED(type)                  C_ASSERT((type) -1 < 0);
#define TEST_TYPE_UNSIGNED(type)                C_ASSERT((type) -1 > 0);


#ifdef _WIN64

static void test_pack_BINDINFO(void)
{
    /* BINDINFO */
    TEST_FIELD_SIZE  (BINDINFO, cbSize, 4)
    TEST_FIELD_ALIGN (BINDINFO, cbSize, 4)
    TEST_FIELD_OFFSET(BINDINFO, cbSize, 0)
    TEST_FIELD_SIZE  (BINDINFO, szExtraInfo, 8)
    TEST_FIELD_ALIGN (BINDINFO, szExtraInfo, 8)
    TEST_FIELD_OFFSET(BINDINFO, szExtraInfo, 8)
}

static void test_pack_IBindHost(void)
{
    /* IBindHost */
}

static void test_pack_IBindStatusCallback(void)
{
    /* IBindStatusCallback */
}

static void test_pack_IBinding(void)
{
    /* IBinding */
}

static void test_pack_IInternetProtocolInfo(void)
{
    /* IInternetProtocolInfo */
}

static void test_pack_IInternetSession(void)
{
    /* IInternetSession */
}

static void test_pack_IPersistMoniker(void)
{
    /* IPersistMoniker */
}

static void test_pack_IWinInetHttpInfo(void)
{
    /* IWinInetHttpInfo */
}

static void test_pack_IWinInetInfo(void)
{
    /* IWinInetInfo */
}

static void test_pack_LPBINDHOST(void)
{
    /* LPBINDHOST */
    TEST_TYPE_SIZE   (LPBINDHOST, 8)
    TEST_TYPE_ALIGN  (LPBINDHOST, 8)
}

static void test_pack_LPBINDING(void)
{
    /* LPBINDING */
    TEST_TYPE_SIZE   (LPBINDING, 8)
    TEST_TYPE_ALIGN  (LPBINDING, 8)
}

static void test_pack_LPBINDSTATUSCALLBACK(void)
{
    /* LPBINDSTATUSCALLBACK */
    TEST_TYPE_SIZE   (LPBINDSTATUSCALLBACK, 8)
    TEST_TYPE_ALIGN  (LPBINDSTATUSCALLBACK, 8)
}

static void test_pack_LPIINTERNETPROTOCOLINFO(void)
{
    /* LPIINTERNETPROTOCOLINFO */
    TEST_TYPE_SIZE   (LPIINTERNETPROTOCOLINFO, 8)
    TEST_TYPE_ALIGN  (LPIINTERNETPROTOCOLINFO, 8)
}

static void test_pack_LPIINTERNETSESSION(void)
{
    /* LPIINTERNETSESSION */
    TEST_TYPE_SIZE   (LPIINTERNETSESSION, 8)
    TEST_TYPE_ALIGN  (LPIINTERNETSESSION, 8)
}

static void test_pack_LPPERSISTMONIKER(void)
{
    /* LPPERSISTMONIKER */
    TEST_TYPE_SIZE   (LPPERSISTMONIKER, 8)
    TEST_TYPE_ALIGN  (LPPERSISTMONIKER, 8)
}

static void test_pack_LPREMFORMATETC(void)
{
    /* LPREMFORMATETC */
    TEST_TYPE_SIZE   (LPREMFORMATETC, 8)
    TEST_TYPE_ALIGN  (LPREMFORMATETC, 8)
}

static void test_pack_LPREMSECURITY_ATTRIBUTES(void)
{
    /* LPREMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (LPREMSECURITY_ATTRIBUTES, 8)
    TEST_TYPE_ALIGN  (LPREMSECURITY_ATTRIBUTES, 8)
}

static void test_pack_LPWININETHTTPINFO(void)
{
    /* LPWININETHTTPINFO */
    TEST_TYPE_SIZE   (LPWININETHTTPINFO, 8)
    TEST_TYPE_ALIGN  (LPWININETHTTPINFO, 8)
}

static void test_pack_LPWININETINFO(void)
{
    /* LPWININETINFO */
    TEST_TYPE_SIZE   (LPWININETINFO, 8)
    TEST_TYPE_ALIGN  (LPWININETINFO, 8)
}

static void test_pack_PREMSECURITY_ATTRIBUTES(void)
{
    /* PREMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (PREMSECURITY_ATTRIBUTES, 8)
    TEST_TYPE_ALIGN  (PREMSECURITY_ATTRIBUTES, 8)
}

static void test_pack_REMSECURITY_ATTRIBUTES(void)
{
    /* REMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (REMSECURITY_ATTRIBUTES, 12)
    TEST_TYPE_ALIGN  (REMSECURITY_ATTRIBUTES, 4)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, nLength, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, nLength, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, nLength, 0)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, bInheritHandle, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, bInheritHandle, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, bInheritHandle, 8)
}

static void test_pack_RemBINDINFO(void)
{
    /* RemBINDINFO */
    TEST_TYPE_SIZE   (RemBINDINFO, 96)
    TEST_TYPE_ALIGN  (RemBINDINFO, 8)
    TEST_FIELD_SIZE  (RemBINDINFO, cbSize, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, cbSize, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, cbSize, 0)
    TEST_FIELD_SIZE  (RemBINDINFO, szExtraInfo, 8)
    TEST_FIELD_ALIGN (RemBINDINFO, szExtraInfo, 8)
    TEST_FIELD_OFFSET(RemBINDINFO, szExtraInfo, 8)
    TEST_FIELD_SIZE  (RemBINDINFO, grfBindInfoF, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, grfBindInfoF, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, grfBindInfoF, 16)
    TEST_FIELD_SIZE  (RemBINDINFO, dwBindVerb, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwBindVerb, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwBindVerb, 20)
    TEST_FIELD_SIZE  (RemBINDINFO, szCustomVerb, 8)
    TEST_FIELD_ALIGN (RemBINDINFO, szCustomVerb, 8)
    TEST_FIELD_OFFSET(RemBINDINFO, szCustomVerb, 24)
    TEST_FIELD_SIZE  (RemBINDINFO, cbstgmedData, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, cbstgmedData, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, cbstgmedData, 32)
    TEST_FIELD_SIZE  (RemBINDINFO, dwOptions, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwOptions, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwOptions, 36)
    TEST_FIELD_SIZE  (RemBINDINFO, dwOptionsFlags, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwOptionsFlags, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwOptionsFlags, 40)
    TEST_FIELD_SIZE  (RemBINDINFO, dwCodePage, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwCodePage, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwCodePage, 44)
    TEST_FIELD_SIZE  (RemBINDINFO, securityAttributes, 12)
    TEST_FIELD_ALIGN (RemBINDINFO, securityAttributes, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, securityAttributes, 48)
    TEST_FIELD_SIZE  (RemBINDINFO, iid, 16)
    TEST_FIELD_ALIGN (RemBINDINFO, iid, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, iid, 60)
    TEST_FIELD_SIZE  (RemBINDINFO, pUnk, 8)
    TEST_FIELD_ALIGN (RemBINDINFO, pUnk, 8)
    TEST_FIELD_OFFSET(RemBINDINFO, pUnk, 80)
    TEST_FIELD_SIZE  (RemBINDINFO, dwReserved, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwReserved, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwReserved, 88)
}

static void test_pack_RemFORMATETC(void)
{
    /* RemFORMATETC */
    TEST_TYPE_SIZE   (RemFORMATETC, 20)
    TEST_TYPE_ALIGN  (RemFORMATETC, 4)
    TEST_FIELD_SIZE  (RemFORMATETC, cfFormat, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, cfFormat, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, cfFormat, 0)
    TEST_FIELD_SIZE  (RemFORMATETC, ptd, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, ptd, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, ptd, 4)
    TEST_FIELD_SIZE  (RemFORMATETC, dwAspect, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, dwAspect, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, dwAspect, 8)
    TEST_FIELD_SIZE  (RemFORMATETC, lindex, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, lindex, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, lindex, 12)
    TEST_FIELD_SIZE  (RemFORMATETC, tymed, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, tymed, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, tymed, 16)
}

#else /* _WIN64 */

static void test_pack_BINDINFO(void)
{
    /* BINDINFO */
    TEST_FIELD_SIZE  (BINDINFO, cbSize, 4)
    TEST_FIELD_ALIGN (BINDINFO, cbSize, 4)
    TEST_FIELD_OFFSET(BINDINFO, cbSize, 0)
    TEST_FIELD_SIZE  (BINDINFO, szExtraInfo, 4)
    TEST_FIELD_ALIGN (BINDINFO, szExtraInfo, 4)
    TEST_FIELD_OFFSET(BINDINFO, szExtraInfo, 4)
}

static void test_pack_IBindHost(void)
{
    /* IBindHost */
}

static void test_pack_IBindStatusCallback(void)
{
    /* IBindStatusCallback */
}

static void test_pack_IBinding(void)
{
    /* IBinding */
}

static void test_pack_IInternetProtocolInfo(void)
{
    /* IInternetProtocolInfo */
}

static void test_pack_IInternetSession(void)
{
    /* IInternetSession */
}

static void test_pack_IPersistMoniker(void)
{
    /* IPersistMoniker */
}

static void test_pack_IWinInetHttpInfo(void)
{
    /* IWinInetHttpInfo */
}

static void test_pack_IWinInetInfo(void)
{
    /* IWinInetInfo */
}

static void test_pack_LPBINDHOST(void)
{
    /* LPBINDHOST */
    TEST_TYPE_SIZE   (LPBINDHOST, 4)
    TEST_TYPE_ALIGN  (LPBINDHOST, 4)
}

static void test_pack_LPBINDING(void)
{
    /* LPBINDING */
    TEST_TYPE_SIZE   (LPBINDING, 4)
    TEST_TYPE_ALIGN  (LPBINDING, 4)
}

static void test_pack_LPBINDSTATUSCALLBACK(void)
{
    /* LPBINDSTATUSCALLBACK */
    TEST_TYPE_SIZE   (LPBINDSTATUSCALLBACK, 4)
    TEST_TYPE_ALIGN  (LPBINDSTATUSCALLBACK, 4)
}

static void test_pack_LPIINTERNETPROTOCOLINFO(void)
{
    /* LPIINTERNETPROTOCOLINFO */
    TEST_TYPE_SIZE   (LPIINTERNETPROTOCOLINFO, 4)
    TEST_TYPE_ALIGN  (LPIINTERNETPROTOCOLINFO, 4)
}

static void test_pack_LPIINTERNETSESSION(void)
{
    /* LPIINTERNETSESSION */
    TEST_TYPE_SIZE   (LPIINTERNETSESSION, 4)
    TEST_TYPE_ALIGN  (LPIINTERNETSESSION, 4)
}

static void test_pack_LPPERSISTMONIKER(void)
{
    /* LPPERSISTMONIKER */
    TEST_TYPE_SIZE   (LPPERSISTMONIKER, 4)
    TEST_TYPE_ALIGN  (LPPERSISTMONIKER, 4)
}

static void test_pack_LPREMFORMATETC(void)
{
    /* LPREMFORMATETC */
    TEST_TYPE_SIZE   (LPREMFORMATETC, 4)
    TEST_TYPE_ALIGN  (LPREMFORMATETC, 4)
}

static void test_pack_LPREMSECURITY_ATTRIBUTES(void)
{
    /* LPREMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (LPREMSECURITY_ATTRIBUTES, 4)
    TEST_TYPE_ALIGN  (LPREMSECURITY_ATTRIBUTES, 4)
}

static void test_pack_LPWININETHTTPINFO(void)
{
    /* LPWININETHTTPINFO */
    TEST_TYPE_SIZE   (LPWININETHTTPINFO, 4)
    TEST_TYPE_ALIGN  (LPWININETHTTPINFO, 4)
}

static void test_pack_LPWININETINFO(void)
{
    /* LPWININETINFO */
    TEST_TYPE_SIZE   (LPWININETINFO, 4)
    TEST_TYPE_ALIGN  (LPWININETINFO, 4)
}

static void test_pack_PREMSECURITY_ATTRIBUTES(void)
{
    /* PREMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (PREMSECURITY_ATTRIBUTES, 4)
    TEST_TYPE_ALIGN  (PREMSECURITY_ATTRIBUTES, 4)
}

static void test_pack_REMSECURITY_ATTRIBUTES(void)
{
    /* REMSECURITY_ATTRIBUTES */
    TEST_TYPE_SIZE   (REMSECURITY_ATTRIBUTES, 12)
    TEST_TYPE_ALIGN  (REMSECURITY_ATTRIBUTES, 4)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, nLength, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, nLength, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, nLength, 0)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, lpSecurityDescriptor, 4)
    TEST_FIELD_SIZE  (REMSECURITY_ATTRIBUTES, bInheritHandle, 4)
    TEST_FIELD_ALIGN (REMSECURITY_ATTRIBUTES, bInheritHandle, 4)
    TEST_FIELD_OFFSET(REMSECURITY_ATTRIBUTES, bInheritHandle, 8)
}

static void test_pack_RemBINDINFO(void)
{
    /* RemBINDINFO */
    TEST_TYPE_SIZE   (RemBINDINFO, 72)
    TEST_TYPE_ALIGN  (RemBINDINFO, 4)
    TEST_FIELD_SIZE  (RemBINDINFO, cbSize, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, cbSize, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, cbSize, 0)
    TEST_FIELD_SIZE  (RemBINDINFO, szExtraInfo, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, szExtraInfo, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, szExtraInfo, 4)
    TEST_FIELD_SIZE  (RemBINDINFO, grfBindInfoF, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, grfBindInfoF, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, grfBindInfoF, 8)
    TEST_FIELD_SIZE  (RemBINDINFO, dwBindVerb, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwBindVerb, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwBindVerb, 12)
    TEST_FIELD_SIZE  (RemBINDINFO, szCustomVerb, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, szCustomVerb, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, szCustomVerb, 16)
    TEST_FIELD_SIZE  (RemBINDINFO, cbstgmedData, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, cbstgmedData, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, cbstgmedData, 20)
    TEST_FIELD_SIZE  (RemBINDINFO, dwOptions, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwOptions, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwOptions, 24)
    TEST_FIELD_SIZE  (RemBINDINFO, dwOptionsFlags, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwOptionsFlags, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwOptionsFlags, 28)
    TEST_FIELD_SIZE  (RemBINDINFO, dwCodePage, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwCodePage, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwCodePage, 32)
    TEST_FIELD_SIZE  (RemBINDINFO, securityAttributes, 12)
    TEST_FIELD_ALIGN (RemBINDINFO, securityAttributes, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, securityAttributes, 36)
    TEST_FIELD_SIZE  (RemBINDINFO, iid, 16)
    TEST_FIELD_ALIGN (RemBINDINFO, iid, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, iid, 48)
    TEST_FIELD_SIZE  (RemBINDINFO, pUnk, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, pUnk, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, pUnk, 64)
    TEST_FIELD_SIZE  (RemBINDINFO, dwReserved, 4)
    TEST_FIELD_ALIGN (RemBINDINFO, dwReserved, 4)
    TEST_FIELD_OFFSET(RemBINDINFO, dwReserved, 68)
}

static void test_pack_RemFORMATETC(void)
{
    /* RemFORMATETC */
    TEST_TYPE_SIZE   (RemFORMATETC, 20)
    TEST_TYPE_ALIGN  (RemFORMATETC, 4)
    TEST_FIELD_SIZE  (RemFORMATETC, cfFormat, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, cfFormat, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, cfFormat, 0)
    TEST_FIELD_SIZE  (RemFORMATETC, ptd, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, ptd, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, ptd, 4)
    TEST_FIELD_SIZE  (RemFORMATETC, dwAspect, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, dwAspect, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, dwAspect, 8)
    TEST_FIELD_SIZE  (RemFORMATETC, lindex, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, lindex, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, lindex, 12)
    TEST_FIELD_SIZE  (RemFORMATETC, tymed, 4)
    TEST_FIELD_ALIGN (RemFORMATETC, tymed, 4)
    TEST_FIELD_OFFSET(RemFORMATETC, tymed, 16)
}

#endif /* _WIN64 */

static void test_pack(void)
{
    test_pack_BINDINFO();
    test_pack_IBindHost();
    test_pack_IBindStatusCallback();
    test_pack_IBinding();
    test_pack_IInternetProtocolInfo();
    test_pack_IInternetSession();
    test_pack_IPersistMoniker();
    test_pack_IWinInetHttpInfo();
    test_pack_IWinInetInfo();
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
