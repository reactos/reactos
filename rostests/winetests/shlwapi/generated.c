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
#include "winreg.h"
#include "shlwapi.h"

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

static void test_pack_ASSOCF(void)
{
    /* ASSOCF */
    TEST_TYPE(ASSOCF, 4, 4);
    TEST_TYPE_UNSIGNED(ASSOCF);
}

static void test_pack_DLLGETVERSIONPROC(void)
{
    /* DLLGETVERSIONPROC */
    TEST_TYPE(DLLGETVERSIONPROC, 4, 4);
}

static void test_pack_DLLVERSIONINFO(void)
{
    /* DLLVERSIONINFO (pack 8) */
    TEST_TYPE(DLLVERSIONINFO, 20, 4);
    TEST_FIELD(DLLVERSIONINFO, DWORD, cbSize, 0, 4, 4);
    TEST_FIELD(DLLVERSIONINFO, DWORD, dwMajorVersion, 4, 4, 4);
    TEST_FIELD(DLLVERSIONINFO, DWORD, dwMinorVersion, 8, 4, 4);
    TEST_FIELD(DLLVERSIONINFO, DWORD, dwBuildNumber, 12, 4, 4);
    TEST_FIELD(DLLVERSIONINFO, DWORD, dwPlatformID, 16, 4, 4);
}

static void test_pack_DLLVERSIONINFO2(void)
{
    /* DLLVERSIONINFO2 (pack 8) */
    TEST_TYPE(DLLVERSIONINFO2, 32, 8);
    TEST_FIELD(DLLVERSIONINFO2, DLLVERSIONINFO, info1, 0, 20, 4);
    TEST_FIELD(DLLVERSIONINFO2, DWORD, dwFlags, 20, 4, 4);
    TEST_FIELD(DLLVERSIONINFO2, ULONGLONG, ullVersion, 24, 8, 8);
}

static void test_pack_HUSKEY(void)
{
    /* HUSKEY */
    TEST_TYPE(HUSKEY, 4, 4);
}

static void test_pack_PHUSKEY(void)
{
    /* PHUSKEY */
    TEST_TYPE(PHUSKEY, 4, 4);
    TEST_TYPE_POINTER(PHUSKEY, 4, 4);
}

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
