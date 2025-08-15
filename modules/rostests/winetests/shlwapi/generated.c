/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINE_NOWINSOCK

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winreg.h"
#include "shlwapi.h"

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

static void test_pack_ASSOCF(void)
{
    /* ASSOCF */
    TEST_TYPE_SIZE   (ASSOCF, 4)
    TEST_TYPE_ALIGN  (ASSOCF, 4)
    TEST_TYPE_UNSIGNED(ASSOCF)
}

static void test_pack_DLLGETVERSIONPROC(void)
{
    /* DLLGETVERSIONPROC */
    TEST_TYPE_SIZE   (DLLGETVERSIONPROC, 8)
    TEST_TYPE_ALIGN  (DLLGETVERSIONPROC, 8)
}

static void test_pack_DLLVERSIONINFO(void)
{
    /* DLLVERSIONINFO (pack 8) */
    TEST_TYPE_SIZE   (DLLVERSIONINFO, 20)
    TEST_TYPE_ALIGN  (DLLVERSIONINFO, 4)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, cbSize, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, cbSize, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, cbSize, 0)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwMinorVersion, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwMinorVersion, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwMinorVersion, 8)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwBuildNumber, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwBuildNumber, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwBuildNumber, 12)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwPlatformID, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwPlatformID, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwPlatformID, 16)
}

static void test_pack_DLLVERSIONINFO2(void)
{
    /* DLLVERSIONINFO2 (pack 8) */
    TEST_TYPE_SIZE   (DLLVERSIONINFO2, 32)
    TEST_TYPE_ALIGN  (DLLVERSIONINFO2, 8)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, info1, 20)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, info1, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, info1, 0)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, dwFlags, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, dwFlags, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, dwFlags, 20)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, ullVersion, 8)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, ullVersion, 8)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, ullVersion, 24)
}

static void test_pack_HUSKEY(void)
{
    /* HUSKEY */
    TEST_TYPE_SIZE   (HUSKEY, 8)
    TEST_TYPE_ALIGN  (HUSKEY, 8)
}

static void test_pack_PHUSKEY(void)
{
    /* PHUSKEY */
    TEST_TYPE_SIZE   (PHUSKEY, 8)
    TEST_TYPE_ALIGN  (PHUSKEY, 8)
    TEST_TARGET_SIZE (PHUSKEY, 8)
    TEST_TARGET_ALIGN(PHUSKEY, 8)
}

#else /* _WIN64 */

static void test_pack_ASSOCF(void)
{
    /* ASSOCF */
    TEST_TYPE_SIZE   (ASSOCF, 4)
    TEST_TYPE_ALIGN  (ASSOCF, 4)
    TEST_TYPE_UNSIGNED(ASSOCF)
}

static void test_pack_DLLGETVERSIONPROC(void)
{
    /* DLLGETVERSIONPROC */
    TEST_TYPE_SIZE   (DLLGETVERSIONPROC, 4)
    TEST_TYPE_ALIGN  (DLLGETVERSIONPROC, 4)
}

static void test_pack_DLLVERSIONINFO(void)
{
    /* DLLVERSIONINFO (pack 8) */
    TEST_TYPE_SIZE   (DLLVERSIONINFO, 20)
    TEST_TYPE_ALIGN  (DLLVERSIONINFO, 4)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, cbSize, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, cbSize, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, cbSize, 0)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwMajorVersion, 4)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwMinorVersion, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwMinorVersion, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwMinorVersion, 8)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwBuildNumber, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwBuildNumber, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwBuildNumber, 12)
    TEST_FIELD_SIZE  (DLLVERSIONINFO, dwPlatformID, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO, dwPlatformID, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO, dwPlatformID, 16)
}

static void test_pack_DLLVERSIONINFO2(void)
{
    /* DLLVERSIONINFO2 (pack 8) */
    TEST_TYPE_SIZE   (DLLVERSIONINFO2, 32)
    TEST_TYPE_ALIGN  (DLLVERSIONINFO2, 8)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, info1, 20)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, info1, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, info1, 0)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, dwFlags, 4)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, dwFlags, 4)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, dwFlags, 20)
    TEST_FIELD_SIZE  (DLLVERSIONINFO2, ullVersion, 8)
    TEST_FIELD_ALIGN (DLLVERSIONINFO2, ullVersion, 8)
    TEST_FIELD_OFFSET(DLLVERSIONINFO2, ullVersion, 24)
}

static void test_pack_HUSKEY(void)
{
    /* HUSKEY */
    TEST_TYPE_SIZE   (HUSKEY, 4)
    TEST_TYPE_ALIGN  (HUSKEY, 4)
}

static void test_pack_PHUSKEY(void)
{
    /* PHUSKEY */
    TEST_TYPE_SIZE   (PHUSKEY, 4)
    TEST_TYPE_ALIGN  (PHUSKEY, 4)
    TEST_TARGET_SIZE (PHUSKEY, 4)
    TEST_TARGET_ALIGN(PHUSKEY, 4)
}

#endif /* _WIN64 */

static void test_pack(void)
{
    test_pack_ASSOCF();
    test_pack_DLLGETVERSIONPROC();
    test_pack_DLLVERSIONINFO();
    test_pack_DLLVERSIONINFO2();
    test_pack_HUSKEY();
    test_pack_PHUSKEY();
}

START_TEST(generated)
{
    test_pack();
}
