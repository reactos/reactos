/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

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

static void test_pack_ABC(void)
{
    /* ABC (pack 4) */
    TEST_TYPE(ABC, 12, 4);
    TEST_FIELD(ABC, INT, abcA, 0, 4, 4);
    TEST_FIELD(ABC, UINT, abcB, 4, 4, 4);
    TEST_FIELD(ABC, INT, abcC, 8, 4, 4);
}

static void test_pack_ABCFLOAT(void)
{
    /* ABCFLOAT (pack 4) */
    TEST_TYPE(ABCFLOAT, 12, 4);
    TEST_FIELD(ABCFLOAT, FLOAT, abcfA, 0, 4, 4);
    TEST_FIELD(ABCFLOAT, FLOAT, abcfB, 4, 4, 4);
    TEST_FIELD(ABCFLOAT, FLOAT, abcfC, 8, 4, 4);
}

static void test_pack_ABORTPROC(void)
{
    /* ABORTPROC */
    TEST_TYPE(ABORTPROC, 4, 4);
}

static void test_pack_BITMAP(void)
{
    /* BITMAP (pack 4) */
    TEST_TYPE(BITMAP, 24, 4);
    TEST_FIELD(BITMAP, INT, bmType, 0, 4, 4);
    TEST_FIELD(BITMAP, INT, bmWidth, 4, 4, 4);
    TEST_FIELD(BITMAP, INT, bmHeight, 8, 4, 4);
    TEST_FIELD(BITMAP, INT, bmWidthBytes, 12, 4, 4);
    TEST_FIELD(BITMAP, WORD, bmPlanes, 16, 2, 2);
    TEST_FIELD(BITMAP, WORD, bmBitsPixel, 18, 2, 2);
    TEST_FIELD(BITMAP, LPVOID, bmBits, 20, 4, 4);
}

static void test_pack_BITMAPCOREHEADER(void)
{
    /* BITMAPCOREHEADER (pack 4) */
    TEST_TYPE(BITMAPCOREHEADER, 12, 4);
    TEST_FIELD(BITMAPCOREHEADER, DWORD, bcSize, 0, 4, 4);
    TEST_FIELD(BITMAPCOREHEADER, WORD, bcWidth, 4, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, WORD, bcHeight, 6, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, WORD, bcPlanes, 8, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, WORD, bcBitCount, 10, 2, 2);
}

static void test_pack_BITMAPCOREINFO(void)
{
    /* BITMAPCOREINFO (pack 4) */
    TEST_TYPE(BITMAPCOREINFO, 16, 4);
    TEST_FIELD(BITMAPCOREINFO, BITMAPCOREHEADER, bmciHeader, 0, 12, 4);
    TEST_FIELD(BITMAPCOREINFO, RGBTRIPLE[1], bmciColors, 12, 3, 1);
}

static void test_pack_BITMAPFILEHEADER(void)
{
    /* BITMAPFILEHEADER (pack 2) */
    TEST_TYPE(BITMAPFILEHEADER, 14, 2);
    TEST_FIELD(BITMAPFILEHEADER, WORD, bfType, 0, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, DWORD, bfSize, 2, 4, 2);
    TEST_FIELD(BITMAPFILEHEADER, WORD, bfReserved1, 6, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, WORD, bfReserved2, 8, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, DWORD, bfOffBits, 10, 4, 2);
}

static void test_pack_BITMAPINFO(void)
{
    /* BITMAPINFO (pack 4) */
    TEST_TYPE(BITMAPINFO, 44, 4);
    TEST_FIELD(BITMAPINFO, BITMAPINFOHEADER, bmiHeader, 0, 40, 4);
    TEST_FIELD(BITMAPINFO, RGBQUAD[1], bmiColors, 40, 4, 1);
}

static void test_pack_BITMAPINFOHEADER(void)
{
    /* BITMAPINFOHEADER (pack 4) */
    TEST_TYPE(BITMAPINFOHEADER, 40, 4);
    TEST_FIELD(BITMAPINFOHEADER, DWORD, biSize, 0, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, LONG, biWidth, 4, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, LONG, biHeight, 8, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, WORD, biPlanes, 12, 2, 2);
    TEST_FIELD(BITMAPINFOHEADER, WORD, biBitCount, 14, 2, 2);
    TEST_FIELD(BITMAPINFOHEADER, DWORD, biCompression, 16, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, DWORD, biSizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, LONG, biXPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, LONG, biYPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, DWORD, biClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, DWORD, biClrImportant, 36, 4, 4);
}

static void test_pack_BITMAPV4HEADER(void)
{
    /* BITMAPV4HEADER (pack 4) */
    TEST_TYPE(BITMAPV4HEADER, 108, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4Size, 0, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, LONG, bV4Width, 4, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, LONG, bV4Height, 8, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, WORD, bV4Planes, 12, 2, 2);
    TEST_FIELD(BITMAPV4HEADER, WORD, bV4BitCount, 14, 2, 2);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4V4Compression, 16, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4SizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, LONG, bV4XPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, LONG, bV4YPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4ClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4ClrImportant, 36, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4RedMask, 40, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4GreenMask, 44, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4BlueMask, 48, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4AlphaMask, 52, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4CSType, 56, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, CIEXYZTRIPLE, bV4Endpoints, 60, 36, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4GammaRed, 96, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4GammaGreen, 100, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, DWORD, bV4GammaBlue, 104, 4, 4);
}

static void test_pack_BITMAPV5HEADER(void)
{
    /* BITMAPV5HEADER (pack 4) */
    TEST_TYPE(BITMAPV5HEADER, 124, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5Size, 0, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, LONG, bV5Width, 4, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, LONG, bV5Height, 8, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, WORD, bV5Planes, 12, 2, 2);
    TEST_FIELD(BITMAPV5HEADER, WORD, bV5BitCount, 14, 2, 2);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5Compression, 16, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5SizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, LONG, bV5XPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, LONG, bV5YPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5ClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5ClrImportant, 36, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5RedMask, 40, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5GreenMask, 44, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5BlueMask, 48, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5AlphaMask, 52, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5CSType, 56, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, CIEXYZTRIPLE, bV5Endpoints, 60, 36, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5GammaRed, 96, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5GammaGreen, 100, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5GammaBlue, 104, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5Intent, 108, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5ProfileData, 112, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5ProfileSize, 116, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, DWORD, bV5Reserved, 120, 4, 4);
}

static void test_pack_BLENDFUNCTION(void)
{
    /* BLENDFUNCTION (pack 4) */
    TEST_TYPE(BLENDFUNCTION, 4, 1);
    TEST_FIELD(BLENDFUNCTION, BYTE, BlendOp, 0, 1, 1);
    TEST_FIELD(BLENDFUNCTION, BYTE, BlendFlags, 1, 1, 1);
    TEST_FIELD(BLENDFUNCTION, BYTE, SourceConstantAlpha, 2, 1, 1);
    TEST_FIELD(BLENDFUNCTION, BYTE, AlphaFormat, 3, 1, 1);
}

static void test_pack_CHARSETINFO(void)
{
    /* CHARSETINFO (pack 4) */
    TEST_TYPE(CHARSETINFO, 32, 4);
    TEST_FIELD(CHARSETINFO, UINT, ciCharset, 0, 4, 4);
    TEST_FIELD(CHARSETINFO, UINT, ciACP, 4, 4, 4);
    TEST_FIELD(CHARSETINFO, FONTSIGNATURE, fs, 8, 24, 4);
}

static void test_pack_CIEXYZ(void)
{
    /* CIEXYZ (pack 4) */
    TEST_TYPE(CIEXYZ, 12, 4);
    TEST_FIELD(CIEXYZ, FXPT2DOT30, ciexyzX, 0, 4, 4);
    TEST_FIELD(CIEXYZ, FXPT2DOT30, ciexyzY, 4, 4, 4);
    TEST_FIELD(CIEXYZ, FXPT2DOT30, ciexyzZ, 8, 4, 4);
}

static void test_pack_CIEXYZTRIPLE(void)
{
    /* CIEXYZTRIPLE (pack 4) */
    TEST_TYPE(CIEXYZTRIPLE, 36, 4);
    TEST_FIELD(CIEXYZTRIPLE, CIEXYZ, ciexyzRed, 0, 12, 4);
    TEST_FIELD(CIEXYZTRIPLE, CIEXYZ, ciexyzGreen, 12, 12, 4);
    TEST_FIELD(CIEXYZTRIPLE, CIEXYZ, ciexyzBlue, 24, 12, 4);
}

static void test_pack_COLOR16(void)
{
    /* COLOR16 */
    TEST_TYPE(COLOR16, 2, 2);
}

static void test_pack_COLORADJUSTMENT(void)
{
    /* COLORADJUSTMENT (pack 4) */
    TEST_TYPE(COLORADJUSTMENT, 24, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caSize, 0, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caFlags, 2, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caIlluminantIndex, 4, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caRedGamma, 6, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caGreenGamma, 8, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caBlueGamma, 10, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caReferenceBlack, 12, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, WORD, caReferenceWhite, 14, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, SHORT, caContrast, 16, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, SHORT, caBrightness, 18, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, SHORT, caColorfulness, 20, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, SHORT, caRedGreenTint, 22, 2, 2);
}

static void test_pack_DEVMODEA(void)
{
    /* DEVMODEA (pack 4) */
    TEST_FIELD(DEVMODEA, BYTE[CCHDEVICENAME], dmDeviceName, 0, 32, 1);
    TEST_FIELD(DEVMODEA, WORD, dmSpecVersion, 32, 2, 2);
    TEST_FIELD(DEVMODEA, WORD, dmDriverVersion, 34, 2, 2);
    TEST_FIELD(DEVMODEA, WORD, dmSize, 36, 2, 2);
    TEST_FIELD(DEVMODEA, WORD, dmDriverExtra, 38, 2, 2);
    TEST_FIELD(DEVMODEA, DWORD, dmFields, 40, 4, 4);
}

static void test_pack_DEVMODEW(void)
{
    /* DEVMODEW (pack 4) */
    TEST_FIELD(DEVMODEW, WCHAR[CCHDEVICENAME], dmDeviceName, 0, 64, 2);
    TEST_FIELD(DEVMODEW, WORD, dmSpecVersion, 64, 2, 2);
    TEST_FIELD(DEVMODEW, WORD, dmDriverVersion, 66, 2, 2);
    TEST_FIELD(DEVMODEW, WORD, dmSize, 68, 2, 2);
    TEST_FIELD(DEVMODEW, WORD, dmDriverExtra, 70, 2, 2);
    TEST_FIELD(DEVMODEW, DWORD, dmFields, 72, 4, 4);
}

static void test_pack_DIBSECTION(void)
{
    /* DIBSECTION (pack 4) */
    TEST_TYPE(DIBSECTION, 84, 4);
    TEST_FIELD(DIBSECTION, BITMAP, dsBm, 0, 24, 4);
    TEST_FIELD(DIBSECTION, BITMAPINFOHEADER, dsBmih, 24, 40, 4);
    TEST_FIELD(DIBSECTION, DWORD[3], dsBitfields, 64, 12, 4);
    TEST_FIELD(DIBSECTION, HANDLE, dshSection, 76, 4, 4);
    TEST_FIELD(DIBSECTION, DWORD, dsOffset, 80, 4, 4);
}

static void test_pack_DISPLAY_DEVICEA(void)
{
    /* DISPLAY_DEVICEA (pack 4) */
    TEST_TYPE(DISPLAY_DEVICEA, 424, 4);
    TEST_FIELD(DISPLAY_DEVICEA, DWORD, cb, 0, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEA, CHAR[32], DeviceName, 4, 32, 1);
    TEST_FIELD(DISPLAY_DEVICEA, CHAR[128], DeviceString, 36, 128, 1);
    TEST_FIELD(DISPLAY_DEVICEA, DWORD, StateFlags, 164, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEA, CHAR[128], DeviceID, 168, 128, 1);
    TEST_FIELD(DISPLAY_DEVICEA, CHAR[128], DeviceKey, 296, 128, 1);
}

static void test_pack_DISPLAY_DEVICEW(void)
{
    /* DISPLAY_DEVICEW (pack 4) */
    TEST_TYPE(DISPLAY_DEVICEW, 840, 4);
    TEST_FIELD(DISPLAY_DEVICEW, DWORD, cb, 0, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEW, WCHAR[32], DeviceName, 4, 64, 2);
    TEST_FIELD(DISPLAY_DEVICEW, WCHAR[128], DeviceString, 68, 256, 2);
    TEST_FIELD(DISPLAY_DEVICEW, DWORD, StateFlags, 324, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEW, WCHAR[128], DeviceID, 328, 256, 2);
    TEST_FIELD(DISPLAY_DEVICEW, WCHAR[128], DeviceKey, 584, 256, 2);
}

static void test_pack_DOCINFOA(void)
{
    /* DOCINFOA (pack 4) */
    TEST_TYPE(DOCINFOA, 20, 4);
    TEST_FIELD(DOCINFOA, INT, cbSize, 0, 4, 4);
    TEST_FIELD(DOCINFOA, LPCSTR, lpszDocName, 4, 4, 4);
    TEST_FIELD(DOCINFOA, LPCSTR, lpszOutput, 8, 4, 4);
    TEST_FIELD(DOCINFOA, LPCSTR, lpszDatatype, 12, 4, 4);
    TEST_FIELD(DOCINFOA, DWORD, fwType, 16, 4, 4);
}

static void test_pack_DOCINFOW(void)
{
    /* DOCINFOW (pack 4) */
    TEST_TYPE(DOCINFOW, 20, 4);
    TEST_FIELD(DOCINFOW, INT, cbSize, 0, 4, 4);
    TEST_FIELD(DOCINFOW, LPCWSTR, lpszDocName, 4, 4, 4);
    TEST_FIELD(DOCINFOW, LPCWSTR, lpszOutput, 8, 4, 4);
    TEST_FIELD(DOCINFOW, LPCWSTR, lpszDatatype, 12, 4, 4);
    TEST_FIELD(DOCINFOW, DWORD, fwType, 16, 4, 4);
}

static void test_pack_EMR(void)
{
    /* EMR (pack 4) */
    TEST_TYPE(EMR, 8, 4);
    TEST_FIELD(EMR, DWORD, iType, 0, 4, 4);
    TEST_FIELD(EMR, DWORD, nSize, 4, 4, 4);
}

static void test_pack_EMRABORTPATH(void)
{
    /* EMRABORTPATH (pack 4) */
    TEST_TYPE(EMRABORTPATH, 8, 4);
    TEST_FIELD(EMRABORTPATH, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRANGLEARC(void)
{
    /* EMRANGLEARC (pack 4) */
    TEST_TYPE(EMRANGLEARC, 28, 4);
    TEST_FIELD(EMRANGLEARC, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRANGLEARC, POINTL, ptlCenter, 8, 8, 4);
    TEST_FIELD(EMRANGLEARC, DWORD, nRadius, 16, 4, 4);
    TEST_FIELD(EMRANGLEARC, FLOAT, eStartAngle, 20, 4, 4);
    TEST_FIELD(EMRANGLEARC, FLOAT, eSweepAngle, 24, 4, 4);
}

static void test_pack_EMRARC(void)
{
    /* EMRARC (pack 4) */
    TEST_TYPE(EMRARC, 40, 4);
    TEST_FIELD(EMRARC, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRARC, RECTL, rclBox, 8, 16, 4);
    TEST_FIELD(EMRARC, POINTL, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRARC, POINTL, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRARCTO(void)
{
    /* EMRARCTO (pack 4) */
    TEST_TYPE(EMRARCTO, 40, 4);
    TEST_FIELD(EMRARCTO, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRARCTO, RECTL, rclBox, 8, 16, 4);
    TEST_FIELD(EMRARCTO, POINTL, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRARCTO, POINTL, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRBEGINPATH(void)
{
    /* EMRBEGINPATH (pack 4) */
    TEST_TYPE(EMRBEGINPATH, 8, 4);
    TEST_FIELD(EMRBEGINPATH, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRBITBLT(void)
{
    /* EMRBITBLT (pack 4) */
    TEST_TYPE(EMRBITBLT, 100, 4);
    TEST_FIELD(EMRBITBLT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRBITBLT, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRBITBLT, LONG, xDest, 24, 4, 4);
    TEST_FIELD(EMRBITBLT, LONG, yDest, 28, 4, 4);
    TEST_FIELD(EMRBITBLT, LONG, cxDest, 32, 4, 4);
    TEST_FIELD(EMRBITBLT, LONG, cyDest, 36, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, dwRop, 40, 4, 4);
    TEST_FIELD(EMRBITBLT, LONG, xSrc, 44, 4, 4);
    TEST_FIELD(EMRBITBLT, LONG, ySrc, 48, 4, 4);
    TEST_FIELD(EMRBITBLT, XFORM, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRBITBLT, COLORREF, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRBITBLT, DWORD, cbBitsSrc, 96, 4, 4);
}

static void test_pack_EMRCHORD(void)
{
    /* EMRCHORD (pack 4) */
    TEST_TYPE(EMRCHORD, 40, 4);
    TEST_FIELD(EMRCHORD, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCHORD, RECTL, rclBox, 8, 16, 4);
    TEST_FIELD(EMRCHORD, POINTL, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRCHORD, POINTL, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRCLOSEFIGURE(void)
{
    /* EMRCLOSEFIGURE (pack 4) */
    TEST_TYPE(EMRCLOSEFIGURE, 8, 4);
    TEST_FIELD(EMRCLOSEFIGURE, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRCREATEBRUSHINDIRECT(void)
{
    /* EMRCREATEBRUSHINDIRECT (pack 4) */
    TEST_TYPE(EMRCREATEBRUSHINDIRECT, 24, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, DWORD, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, LOGBRUSH32, lb, 12, 12, 4);
}

static void test_pack_EMRCREATECOLORSPACE(void)
{
    /* EMRCREATECOLORSPACE (pack 4) */
    TEST_TYPE(EMRCREATECOLORSPACE, 340, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, DWORD, ihCS, 8, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, LOGCOLORSPACEA, lcs, 12, 328, 4);
}

static void test_pack_EMRCREATECOLORSPACEW(void)
{
    /* EMRCREATECOLORSPACEW (pack 4) */
    TEST_TYPE(EMRCREATECOLORSPACEW, 612, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, DWORD, ihCS, 8, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, LOGCOLORSPACEW, lcs, 12, 588, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, DWORD, dwFlags, 600, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, DWORD, cbData, 604, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, BYTE[1], Data, 608, 1, 1);
}

static void test_pack_EMRCREATEDIBPATTERNBRUSHPT(void)
{
    /* EMRCREATEDIBPATTERNBRUSHPT (pack 4) */
    TEST_TYPE(EMRCREATEDIBPATTERNBRUSHPT, 32, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, iUsage, 12, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, offBmi, 16, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, cbBmi, 20, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, offBits, 24, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, DWORD, cbBits, 28, 4, 4);
}

static void test_pack_EMRCREATEMONOBRUSH(void)
{
    /* EMRCREATEMONOBRUSH (pack 4) */
    TEST_TYPE(EMRCREATEMONOBRUSH, 32, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, iUsage, 12, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, offBmi, 16, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, cbBmi, 20, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, offBits, 24, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, DWORD, cbBits, 28, 4, 4);
}

static void test_pack_EMRCREATEPEN(void)
{
    /* EMRCREATEPEN (pack 4) */
    TEST_TYPE(EMRCREATEPEN, 28, 4);
    TEST_FIELD(EMRCREATEPEN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEPEN, DWORD, ihPen, 8, 4, 4);
    TEST_FIELD(EMRCREATEPEN, LOGPEN, lopn, 12, 16, 4);
}

static void test_pack_EMRDELETECOLORSPACE(void)
{
    /* EMRDELETECOLORSPACE (pack 4) */
    TEST_TYPE(EMRDELETECOLORSPACE, 12, 4);
    TEST_FIELD(EMRDELETECOLORSPACE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRDELETECOLORSPACE, DWORD, ihCS, 8, 4, 4);
}

static void test_pack_EMRDELETEOBJECT(void)
{
    /* EMRDELETEOBJECT (pack 4) */
    TEST_TYPE(EMRDELETEOBJECT, 12, 4);
    TEST_FIELD(EMRDELETEOBJECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRDELETEOBJECT, DWORD, ihObject, 8, 4, 4);
}

static void test_pack_EMRELLIPSE(void)
{
    /* EMRELLIPSE (pack 4) */
    TEST_TYPE(EMRELLIPSE, 24, 4);
    TEST_FIELD(EMRELLIPSE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRELLIPSE, RECTL, rclBox, 8, 16, 4);
}

static void test_pack_EMRENDPATH(void)
{
    /* EMRENDPATH (pack 4) */
    TEST_TYPE(EMRENDPATH, 8, 4);
    TEST_FIELD(EMRENDPATH, EMR, emr, 0, 8, 4);
}

static void test_pack_EMREOF(void)
{
    /* EMREOF (pack 4) */
    TEST_TYPE(EMREOF, 20, 4);
    TEST_FIELD(EMREOF, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREOF, DWORD, nPalEntries, 8, 4, 4);
    TEST_FIELD(EMREOF, DWORD, offPalEntries, 12, 4, 4);
    TEST_FIELD(EMREOF, DWORD, nSizeLast, 16, 4, 4);
}

static void test_pack_EMREXCLUDECLIPRECT(void)
{
    /* EMREXCLUDECLIPRECT (pack 4) */
    TEST_TYPE(EMREXCLUDECLIPRECT, 24, 4);
    TEST_FIELD(EMREXCLUDECLIPRECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXCLUDECLIPRECT, RECTL, rclClip, 8, 16, 4);
}

static void test_pack_EMREXTCREATEFONTINDIRECTW(void)
{
    /* EMREXTCREATEFONTINDIRECTW (pack 4) */
    TEST_TYPE(EMREXTCREATEFONTINDIRECTW, 332, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, DWORD, ihFont, 8, 4, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, EXTLOGFONTW, elfw, 12, 320, 4);
}

static void test_pack_EMREXTCREATEPEN(void)
{
    /* EMREXTCREATEPEN (pack 4) */
    TEST_TYPE(EMREXTCREATEPEN, 56, 4);
    TEST_FIELD(EMREXTCREATEPEN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTCREATEPEN, DWORD, ihPen, 8, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, DWORD, offBmi, 12, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, DWORD, cbBmi, 16, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, DWORD, offBits, 20, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, DWORD, cbBits, 24, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, EXTLOGPEN, elp, 28, 28, 4);
}

static void test_pack_EMREXTFLOODFILL(void)
{
    /* EMREXTFLOODFILL (pack 4) */
    TEST_TYPE(EMREXTFLOODFILL, 24, 4);
    TEST_FIELD(EMREXTFLOODFILL, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTFLOODFILL, POINTL, ptlStart, 8, 8, 4);
    TEST_FIELD(EMREXTFLOODFILL, COLORREF, crColor, 16, 4, 4);
    TEST_FIELD(EMREXTFLOODFILL, DWORD, iMode, 20, 4, 4);
}

static void test_pack_EMREXTSELECTCLIPRGN(void)
{
    /* EMREXTSELECTCLIPRGN (pack 4) */
    TEST_TYPE(EMREXTSELECTCLIPRGN, 20, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, DWORD, cbRgnData, 8, 4, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, DWORD, iMode, 12, 4, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, BYTE[1], RgnData, 16, 1, 1);
}

static void test_pack_EMREXTTEXTOUTA(void)
{
    /* EMREXTTEXTOUTA (pack 4) */
    TEST_TYPE(EMREXTTEXTOUTA, 76, 4);
    TEST_FIELD(EMREXTTEXTOUTA, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTTEXTOUTA, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMREXTTEXTOUTA, DWORD, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, FLOAT, exScale, 28, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, FLOAT, eyScale, 32, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, EMRTEXT, emrtext, 36, 40, 4);
}

static void test_pack_EMREXTTEXTOUTW(void)
{
    /* EMREXTTEXTOUTW (pack 4) */
    TEST_TYPE(EMREXTTEXTOUTW, 76, 4);
    TEST_FIELD(EMREXTTEXTOUTW, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMREXTTEXTOUTW, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMREXTTEXTOUTW, DWORD, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, FLOAT, exScale, 28, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, FLOAT, eyScale, 32, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, EMRTEXT, emrtext, 36, 40, 4);
}

static void test_pack_EMRFILLPATH(void)
{
    /* EMRFILLPATH (pack 4) */
    TEST_TYPE(EMRFILLPATH, 24, 4);
    TEST_FIELD(EMRFILLPATH, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRFILLPATH, RECTL, rclBounds, 8, 16, 4);
}

static void test_pack_EMRFILLRGN(void)
{
    /* EMRFILLRGN (pack 4) */
    TEST_TYPE(EMRFILLRGN, 36, 4);
    TEST_FIELD(EMRFILLRGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRFILLRGN, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRFILLRGN, DWORD, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRFILLRGN, DWORD, ihBrush, 28, 4, 4);
    TEST_FIELD(EMRFILLRGN, BYTE[1], RgnData, 32, 1, 1);
}

static void test_pack_EMRFLATTENPATH(void)
{
    /* EMRFLATTENPATH (pack 4) */
    TEST_TYPE(EMRFLATTENPATH, 8, 4);
    TEST_FIELD(EMRFLATTENPATH, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRFORMAT(void)
{
    /* EMRFORMAT (pack 4) */
    TEST_TYPE(EMRFORMAT, 16, 4);
    TEST_FIELD(EMRFORMAT, DWORD, dSignature, 0, 4, 4);
    TEST_FIELD(EMRFORMAT, DWORD, nVersion, 4, 4, 4);
    TEST_FIELD(EMRFORMAT, DWORD, cbData, 8, 4, 4);
    TEST_FIELD(EMRFORMAT, DWORD, offData, 12, 4, 4);
}

static void test_pack_EMRFRAMERGN(void)
{
    /* EMRFRAMERGN (pack 4) */
    TEST_TYPE(EMRFRAMERGN, 44, 4);
    TEST_FIELD(EMRFRAMERGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRFRAMERGN, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRFRAMERGN, DWORD, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRFRAMERGN, DWORD, ihBrush, 28, 4, 4);
    TEST_FIELD(EMRFRAMERGN, SIZEL, szlStroke, 32, 8, 4);
    TEST_FIELD(EMRFRAMERGN, BYTE[1], RgnData, 40, 1, 1);
}

static void test_pack_EMRGDICOMMENT(void)
{
    /* EMRGDICOMMENT (pack 4) */
    TEST_TYPE(EMRGDICOMMENT, 16, 4);
    TEST_FIELD(EMRGDICOMMENT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRGDICOMMENT, DWORD, cbData, 8, 4, 4);
    TEST_FIELD(EMRGDICOMMENT, BYTE[1], Data, 12, 1, 1);
}

static void test_pack_EMRGLSBOUNDEDRECORD(void)
{
    /* EMRGLSBOUNDEDRECORD (pack 4) */
    TEST_TYPE(EMRGLSBOUNDEDRECORD, 32, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, DWORD, cbData, 24, 4, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, BYTE[1], Data, 28, 1, 1);
}

static void test_pack_EMRGLSRECORD(void)
{
    /* EMRGLSRECORD (pack 4) */
    TEST_TYPE(EMRGLSRECORD, 16, 4);
    TEST_FIELD(EMRGLSRECORD, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRGLSRECORD, DWORD, cbData, 8, 4, 4);
    TEST_FIELD(EMRGLSRECORD, BYTE[1], Data, 12, 1, 1);
}

static void test_pack_EMRINTERSECTCLIPRECT(void)
{
    /* EMRINTERSECTCLIPRECT (pack 4) */
    TEST_TYPE(EMRINTERSECTCLIPRECT, 24, 4);
    TEST_FIELD(EMRINTERSECTCLIPRECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRINTERSECTCLIPRECT, RECTL, rclClip, 8, 16, 4);
}

static void test_pack_EMRINVERTRGN(void)
{
    /* EMRINVERTRGN (pack 4) */
    TEST_TYPE(EMRINVERTRGN, 32, 4);
    TEST_FIELD(EMRINVERTRGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRINVERTRGN, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRINVERTRGN, DWORD, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRINVERTRGN, BYTE[1], RgnData, 28, 1, 1);
}

static void test_pack_EMRLINETO(void)
{
    /* EMRLINETO (pack 4) */
    TEST_TYPE(EMRLINETO, 16, 4);
    TEST_FIELD(EMRLINETO, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRLINETO, POINTL, ptl, 8, 8, 4);
}

static void test_pack_EMRMASKBLT(void)
{
    /* EMRMASKBLT (pack 4) */
    TEST_TYPE(EMRMASKBLT, 128, 4);
    TEST_FIELD(EMRMASKBLT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRMASKBLT, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRMASKBLT, LONG, xDest, 24, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, yDest, 28, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, cxDest, 32, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, cyDest, 36, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, dwRop, 40, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, xSrc, 44, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, ySrc, 48, 4, 4);
    TEST_FIELD(EMRMASKBLT, XFORM, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRMASKBLT, COLORREF, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, cbBitsSrc, 96, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, xMask, 100, 4, 4);
    TEST_FIELD(EMRMASKBLT, LONG, yMask, 104, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, iUsageMask, 108, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, offBmiMask, 112, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, cbBmiMask, 116, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, offBitsMask, 120, 4, 4);
    TEST_FIELD(EMRMASKBLT, DWORD, cbBitsMask, 124, 4, 4);
}

static void test_pack_EMRMODIFYWORLDTRANSFORM(void)
{
    /* EMRMODIFYWORLDTRANSFORM (pack 4) */
    TEST_TYPE(EMRMODIFYWORLDTRANSFORM, 36, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, XFORM, xform, 8, 24, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, DWORD, iMode, 32, 4, 4);
}

static void test_pack_EMRMOVETOEX(void)
{
    /* EMRMOVETOEX (pack 4) */
    TEST_TYPE(EMRMOVETOEX, 16, 4);
    TEST_FIELD(EMRMOVETOEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRMOVETOEX, POINTL, ptl, 8, 8, 4);
}

static void test_pack_EMROFFSETCLIPRGN(void)
{
    /* EMROFFSETCLIPRGN (pack 4) */
    TEST_TYPE(EMROFFSETCLIPRGN, 16, 4);
    TEST_FIELD(EMROFFSETCLIPRGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMROFFSETCLIPRGN, POINTL, ptlOffset, 8, 8, 4);
}

static void test_pack_EMRPAINTRGN(void)
{
    /* EMRPAINTRGN (pack 4) */
    TEST_TYPE(EMRPAINTRGN, 32, 4);
    TEST_FIELD(EMRPAINTRGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPAINTRGN, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPAINTRGN, DWORD, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRPAINTRGN, BYTE[1], RgnData, 28, 1, 1);
}

static void test_pack_EMRPIE(void)
{
    /* EMRPIE (pack 4) */
    TEST_TYPE(EMRPIE, 40, 4);
    TEST_FIELD(EMRPIE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPIE, RECTL, rclBox, 8, 16, 4);
    TEST_FIELD(EMRPIE, POINTL, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRPIE, POINTL, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRPIXELFORMAT(void)
{
    /* EMRPIXELFORMAT (pack 4) */
    TEST_TYPE(EMRPIXELFORMAT, 48, 4);
    TEST_FIELD(EMRPIXELFORMAT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPIXELFORMAT, PIXELFORMATDESCRIPTOR, pfd, 8, 40, 4);
}

static void test_pack_EMRPLGBLT(void)
{
    /* EMRPLGBLT (pack 4) */
    TEST_TYPE(EMRPLGBLT, 140, 4);
    TEST_FIELD(EMRPLGBLT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPLGBLT, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPLGBLT, POINTL[3], aptlDest, 24, 24, 4);
    TEST_FIELD(EMRPLGBLT, LONG, xSrc, 48, 4, 4);
    TEST_FIELD(EMRPLGBLT, LONG, ySrc, 52, 4, 4);
    TEST_FIELD(EMRPLGBLT, LONG, cxSrc, 56, 4, 4);
    TEST_FIELD(EMRPLGBLT, LONG, cySrc, 60, 4, 4);
    TEST_FIELD(EMRPLGBLT, XFORM, xformSrc, 64, 24, 4);
    TEST_FIELD(EMRPLGBLT, COLORREF, crBkColorSrc, 88, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, iUsageSrc, 92, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, offBmiSrc, 96, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, cbBmiSrc, 100, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, offBitsSrc, 104, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, cbBitsSrc, 108, 4, 4);
    TEST_FIELD(EMRPLGBLT, LONG, xMask, 112, 4, 4);
    TEST_FIELD(EMRPLGBLT, LONG, yMask, 116, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, iUsageMask, 120, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, offBmiMask, 124, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, cbBmiMask, 128, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, offBitsMask, 132, 4, 4);
    TEST_FIELD(EMRPLGBLT, DWORD, cbBitsMask, 136, 4, 4);
}

static void test_pack_EMRPOLYBEZIER(void)
{
    /* EMRPOLYBEZIER (pack 4) */
    TEST_TYPE(EMRPOLYBEZIER, 36, 4);
    TEST_FIELD(EMRPOLYBEZIER, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIER, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIER, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIER, POINTL[1], aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYBEZIER16(void)
{
    /* EMRPOLYBEZIER16 (pack 4) */
    TEST_TYPE(EMRPOLYBEZIER16, 32, 4);
    TEST_FIELD(EMRPOLYBEZIER16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIER16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIER16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIER16, POINTS[1], apts, 28, 4, 2);
}

static void test_pack_EMRPOLYBEZIERTO(void)
{
    /* EMRPOLYBEZIERTO (pack 4) */
    TEST_TYPE(EMRPOLYBEZIERTO, 36, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, POINTL[1], aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYBEZIERTO16(void)
{
    /* EMRPOLYBEZIERTO16 (pack 4) */
    TEST_TYPE(EMRPOLYBEZIERTO16, 32, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, POINTS[1], apts, 28, 4, 2);
}

static void test_pack_EMRPOLYDRAW(void)
{
    /* EMRPOLYDRAW (pack 4) */
    TEST_TYPE(EMRPOLYDRAW, 40, 4);
    TEST_FIELD(EMRPOLYDRAW, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYDRAW, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYDRAW, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYDRAW, POINTL[1], aptl, 28, 8, 4);
    TEST_FIELD(EMRPOLYDRAW, BYTE[1], abTypes, 36, 1, 1);
}

static void test_pack_EMRPOLYDRAW16(void)
{
    /* EMRPOLYDRAW16 (pack 4) */
    TEST_TYPE(EMRPOLYDRAW16, 36, 4);
    TEST_FIELD(EMRPOLYDRAW16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYDRAW16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYDRAW16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYDRAW16, POINTS[1], apts, 28, 4, 2);
    TEST_FIELD(EMRPOLYDRAW16, BYTE[1], abTypes, 32, 1, 1);
}

static void test_pack_EMRPOLYGON(void)
{
    /* EMRPOLYGON (pack 4) */
    TEST_TYPE(EMRPOLYGON, 36, 4);
    TEST_FIELD(EMRPOLYGON, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYGON, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYGON, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYGON, POINTL[1], aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYGON16(void)
{
    /* EMRPOLYGON16 (pack 4) */
    TEST_TYPE(EMRPOLYGON16, 32, 4);
    TEST_FIELD(EMRPOLYGON16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYGON16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYGON16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYGON16, POINTS[1], apts, 28, 4, 2);
}

static void test_pack_EMRPOLYLINE(void)
{
    /* EMRPOLYLINE (pack 4) */
    TEST_TYPE(EMRPOLYLINE, 36, 4);
    TEST_FIELD(EMRPOLYLINE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINE, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINE, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINE, POINTL[1], aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYLINE16(void)
{
    /* EMRPOLYLINE16 (pack 4) */
    TEST_TYPE(EMRPOLYLINE16, 32, 4);
    TEST_FIELD(EMRPOLYLINE16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINE16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINE16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINE16, POINTS[1], apts, 28, 4, 2);
}

static void test_pack_EMRPOLYLINETO(void)
{
    /* EMRPOLYLINETO (pack 4) */
    TEST_TYPE(EMRPOLYLINETO, 36, 4);
    TEST_FIELD(EMRPOLYLINETO, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINETO, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINETO, DWORD, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINETO, POINTL[1], aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYLINETO16(void)
{
    /* EMRPOLYLINETO16 (pack 4) */
    TEST_TYPE(EMRPOLYLINETO16, 32, 4);
    TEST_FIELD(EMRPOLYLINETO16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINETO16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINETO16, DWORD, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINETO16, POINTS[1], apts, 28, 4, 2);
}

static void test_pack_EMRPOLYPOLYGON(void)
{
    /* EMRPOLYPOLYGON (pack 4) */
    TEST_TYPE(EMRPOLYPOLYGON, 44, 4);
    TEST_FIELD(EMRPOLYPOLYGON, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYGON, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYGON, DWORD, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, DWORD, cptl, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, DWORD[1], aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, POINTL[1], aptl, 36, 8, 4);
}

static void test_pack_EMRPOLYPOLYGON16(void)
{
    /* EMRPOLYPOLYGON16 (pack 4) */
    TEST_TYPE(EMRPOLYPOLYGON16, 40, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, DWORD, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, DWORD, cpts, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, DWORD[1], aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, POINTS[1], apts, 36, 4, 2);
}

static void test_pack_EMRPOLYPOLYLINE(void)
{
    /* EMRPOLYPOLYLINE (pack 4) */
    TEST_TYPE(EMRPOLYPOLYLINE, 44, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, DWORD, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, DWORD, cptl, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, DWORD[1], aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, POINTL[1], aptl, 36, 8, 4);
}

static void test_pack_EMRPOLYPOLYLINE16(void)
{
    /* EMRPOLYPOLYLINE16 (pack 4) */
    TEST_TYPE(EMRPOLYPOLYLINE16, 40, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, DWORD, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, DWORD, cpts, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, DWORD[1], aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, POINTS[1], apts, 36, 4, 2);
}

static void test_pack_EMRPOLYTEXTOUTA(void)
{
    /* EMRPOLYTEXTOUTA (pack 4) */
    TEST_TYPE(EMRPOLYTEXTOUTA, 80, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, DWORD, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, FLOAT, exScale, 28, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, FLOAT, eyScale, 32, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, LONG, cStrings, 36, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, EMRTEXT[1], aemrtext, 40, 40, 4);
}

static void test_pack_EMRPOLYTEXTOUTW(void)
{
    /* EMRPOLYTEXTOUTW (pack 4) */
    TEST_TYPE(EMRPOLYTEXTOUTW, 80, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, DWORD, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, FLOAT, exScale, 28, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, FLOAT, eyScale, 32, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, LONG, cStrings, 36, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, EMRTEXT[1], aemrtext, 40, 40, 4);
}

static void test_pack_EMRREALIZEPALETTE(void)
{
    /* EMRREALIZEPALETTE (pack 4) */
    TEST_TYPE(EMRREALIZEPALETTE, 8, 4);
    TEST_FIELD(EMRREALIZEPALETTE, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRRECTANGLE(void)
{
    /* EMRRECTANGLE (pack 4) */
    TEST_TYPE(EMRRECTANGLE, 24, 4);
    TEST_FIELD(EMRRECTANGLE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRRECTANGLE, RECTL, rclBox, 8, 16, 4);
}

static void test_pack_EMRRESIZEPALETTE(void)
{
    /* EMRRESIZEPALETTE (pack 4) */
    TEST_TYPE(EMRRESIZEPALETTE, 16, 4);
    TEST_FIELD(EMRRESIZEPALETTE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRRESIZEPALETTE, DWORD, ihPal, 8, 4, 4);
    TEST_FIELD(EMRRESIZEPALETTE, DWORD, cEntries, 12, 4, 4);
}

static void test_pack_EMRRESTOREDC(void)
{
    /* EMRRESTOREDC (pack 4) */
    TEST_TYPE(EMRRESTOREDC, 12, 4);
    TEST_FIELD(EMRRESTOREDC, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRRESTOREDC, LONG, iRelative, 8, 4, 4);
}

static void test_pack_EMRROUNDRECT(void)
{
    /* EMRROUNDRECT (pack 4) */
    TEST_TYPE(EMRROUNDRECT, 32, 4);
    TEST_FIELD(EMRROUNDRECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRROUNDRECT, RECTL, rclBox, 8, 16, 4);
    TEST_FIELD(EMRROUNDRECT, SIZEL, szlCorner, 24, 8, 4);
}

static void test_pack_EMRSAVEDC(void)
{
    /* EMRSAVEDC (pack 4) */
    TEST_TYPE(EMRSAVEDC, 8, 4);
    TEST_FIELD(EMRSAVEDC, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRSCALEVIEWPORTEXTEX(void)
{
    /* EMRSCALEVIEWPORTEXTEX (pack 4) */
    TEST_TYPE(EMRSCALEVIEWPORTEXTEX, 24, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, LONG, xNum, 8, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, LONG, xDenom, 12, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, LONG, yNum, 16, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, LONG, yDenom, 20, 4, 4);
}

static void test_pack_EMRSCALEWINDOWEXTEX(void)
{
    /* EMRSCALEWINDOWEXTEX (pack 4) */
    TEST_TYPE(EMRSCALEWINDOWEXTEX, 24, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, LONG, xNum, 8, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, LONG, xDenom, 12, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, LONG, yNum, 16, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, LONG, yDenom, 20, 4, 4);
}

static void test_pack_EMRSELECTCLIPPATH(void)
{
    /* EMRSELECTCLIPPATH (pack 4) */
    TEST_TYPE(EMRSELECTCLIPPATH, 12, 4);
    TEST_FIELD(EMRSELECTCLIPPATH, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTCLIPPATH, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSELECTCOLORSPACE(void)
{
    /* EMRSELECTCOLORSPACE (pack 4) */
    TEST_TYPE(EMRSELECTCOLORSPACE, 12, 4);
    TEST_FIELD(EMRSELECTCOLORSPACE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTCOLORSPACE, DWORD, ihCS, 8, 4, 4);
}

static void test_pack_EMRSELECTOBJECT(void)
{
    /* EMRSELECTOBJECT (pack 4) */
    TEST_TYPE(EMRSELECTOBJECT, 12, 4);
    TEST_FIELD(EMRSELECTOBJECT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTOBJECT, DWORD, ihObject, 8, 4, 4);
}

static void test_pack_EMRSELECTPALETTE(void)
{
    /* EMRSELECTPALETTE (pack 4) */
    TEST_TYPE(EMRSELECTPALETTE, 12, 4);
    TEST_FIELD(EMRSELECTPALETTE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTPALETTE, DWORD, ihPal, 8, 4, 4);
}

static void test_pack_EMRSETARCDIRECTION(void)
{
    /* EMRSETARCDIRECTION (pack 4) */
    TEST_TYPE(EMRSETARCDIRECTION, 12, 4);
    TEST_FIELD(EMRSETARCDIRECTION, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETARCDIRECTION, DWORD, iArcDirection, 8, 4, 4);
}

static void test_pack_EMRSETBKCOLOR(void)
{
    /* EMRSETBKCOLOR (pack 4) */
    TEST_TYPE(EMRSETBKCOLOR, 12, 4);
    TEST_FIELD(EMRSETBKCOLOR, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBKCOLOR, COLORREF, crColor, 8, 4, 4);
}

static void test_pack_EMRSETBKMODE(void)
{
    /* EMRSETBKMODE (pack 4) */
    TEST_TYPE(EMRSETBKMODE, 12, 4);
    TEST_FIELD(EMRSETBKMODE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBKMODE, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETBRUSHORGEX(void)
{
    /* EMRSETBRUSHORGEX (pack 4) */
    TEST_TYPE(EMRSETBRUSHORGEX, 16, 4);
    TEST_FIELD(EMRSETBRUSHORGEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBRUSHORGEX, POINTL, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETCOLORADJUSTMENT(void)
{
    /* EMRSETCOLORADJUSTMENT (pack 4) */
    TEST_TYPE(EMRSETCOLORADJUSTMENT, 32, 4);
    TEST_FIELD(EMRSETCOLORADJUSTMENT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETCOLORADJUSTMENT, COLORADJUSTMENT, ColorAdjustment, 8, 24, 2);
}

static void test_pack_EMRSETCOLORSPACE(void)
{
    /* EMRSETCOLORSPACE (pack 4) */
    TEST_TYPE(EMRSETCOLORSPACE, 12, 4);
    TEST_FIELD(EMRSETCOLORSPACE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETCOLORSPACE, DWORD, ihCS, 8, 4, 4);
}

static void test_pack_EMRSETDIBITSTODEVICE(void)
{
    /* EMRSETDIBITSTODEVICE (pack 4) */
    TEST_TYPE(EMRSETDIBITSTODEVICE, 76, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, xDest, 24, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, yDest, 28, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, xSrc, 32, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, ySrc, 36, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, cxSrc, 40, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, LONG, cySrc, 44, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, offBmiSrc, 48, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, cbBmiSrc, 52, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, offBitsSrc, 56, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, cbBitsSrc, 60, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, iUsageSrc, 64, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, iStartScan, 68, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, DWORD, cScans, 72, 4, 4);
}

static void test_pack_EMRSETICMMODE(void)
{
    /* EMRSETICMMODE (pack 4) */
    TEST_TYPE(EMRSETICMMODE, 12, 4);
    TEST_FIELD(EMRSETICMMODE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETICMMODE, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETLAYOUT(void)
{
    /* EMRSETLAYOUT (pack 4) */
    TEST_TYPE(EMRSETLAYOUT, 12, 4);
    TEST_FIELD(EMRSETLAYOUT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETLAYOUT, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETMAPMODE(void)
{
    /* EMRSETMAPMODE (pack 4) */
    TEST_TYPE(EMRSETMAPMODE, 12, 4);
    TEST_FIELD(EMRSETMAPMODE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMAPMODE, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETMAPPERFLAGS(void)
{
    /* EMRSETMAPPERFLAGS (pack 4) */
    TEST_TYPE(EMRSETMAPPERFLAGS, 12, 4);
    TEST_FIELD(EMRSETMAPPERFLAGS, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMAPPERFLAGS, DWORD, dwFlags, 8, 4, 4);
}

static void test_pack_EMRSETMETARGN(void)
{
    /* EMRSETMETARGN (pack 4) */
    TEST_TYPE(EMRSETMETARGN, 8, 4);
    TEST_FIELD(EMRSETMETARGN, EMR, emr, 0, 8, 4);
}

static void test_pack_EMRSETMITERLIMIT(void)
{
    /* EMRSETMITERLIMIT (pack 4) */
    TEST_TYPE(EMRSETMITERLIMIT, 12, 4);
    TEST_FIELD(EMRSETMITERLIMIT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMITERLIMIT, FLOAT, eMiterLimit, 8, 4, 4);
}

static void test_pack_EMRSETPIXELV(void)
{
    /* EMRSETPIXELV (pack 4) */
    TEST_TYPE(EMRSETPIXELV, 20, 4);
    TEST_FIELD(EMRSETPIXELV, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETPIXELV, POINTL, ptlPixel, 8, 8, 4);
    TEST_FIELD(EMRSETPIXELV, COLORREF, crColor, 16, 4, 4);
}

static void test_pack_EMRSETPOLYFILLMODE(void)
{
    /* EMRSETPOLYFILLMODE (pack 4) */
    TEST_TYPE(EMRSETPOLYFILLMODE, 12, 4);
    TEST_FIELD(EMRSETPOLYFILLMODE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETPOLYFILLMODE, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETROP2(void)
{
    /* EMRSETROP2 (pack 4) */
    TEST_TYPE(EMRSETROP2, 12, 4);
    TEST_FIELD(EMRSETROP2, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETROP2, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETSTRETCHBLTMODE(void)
{
    /* EMRSETSTRETCHBLTMODE (pack 4) */
    TEST_TYPE(EMRSETSTRETCHBLTMODE, 12, 4);
    TEST_FIELD(EMRSETSTRETCHBLTMODE, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETSTRETCHBLTMODE, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETTEXTALIGN(void)
{
    /* EMRSETTEXTALIGN (pack 4) */
    TEST_TYPE(EMRSETTEXTALIGN, 12, 4);
    TEST_FIELD(EMRSETTEXTALIGN, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETTEXTALIGN, DWORD, iMode, 8, 4, 4);
}

static void test_pack_EMRSETTEXTCOLOR(void)
{
    /* EMRSETTEXTCOLOR (pack 4) */
    TEST_TYPE(EMRSETTEXTCOLOR, 12, 4);
    TEST_FIELD(EMRSETTEXTCOLOR, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETTEXTCOLOR, COLORREF, crColor, 8, 4, 4);
}

static void test_pack_EMRSETVIEWPORTEXTEX(void)
{
    /* EMRSETVIEWPORTEXTEX (pack 4) */
    TEST_TYPE(EMRSETVIEWPORTEXTEX, 16, 4);
    TEST_FIELD(EMRSETVIEWPORTEXTEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETVIEWPORTEXTEX, SIZEL, szlExtent, 8, 8, 4);
}

static void test_pack_EMRSETVIEWPORTORGEX(void)
{
    /* EMRSETVIEWPORTORGEX (pack 4) */
    TEST_TYPE(EMRSETVIEWPORTORGEX, 16, 4);
    TEST_FIELD(EMRSETVIEWPORTORGEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETVIEWPORTORGEX, POINTL, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETWINDOWEXTEX(void)
{
    /* EMRSETWINDOWEXTEX (pack 4) */
    TEST_TYPE(EMRSETWINDOWEXTEX, 16, 4);
    TEST_FIELD(EMRSETWINDOWEXTEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWINDOWEXTEX, SIZEL, szlExtent, 8, 8, 4);
}

static void test_pack_EMRSETWINDOWORGEX(void)
{
    /* EMRSETWINDOWORGEX (pack 4) */
    TEST_TYPE(EMRSETWINDOWORGEX, 16, 4);
    TEST_FIELD(EMRSETWINDOWORGEX, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWINDOWORGEX, POINTL, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETWORLDTRANSFORM(void)
{
    /* EMRSETWORLDTRANSFORM (pack 4) */
    TEST_TYPE(EMRSETWORLDTRANSFORM, 32, 4);
    TEST_FIELD(EMRSETWORLDTRANSFORM, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWORLDTRANSFORM, XFORM, xform, 8, 24, 4);
}

static void test_pack_EMRSTRETCHBLT(void)
{
    /* EMRSTRETCHBLT (pack 4) */
    TEST_TYPE(EMRSTRETCHBLT, 108, 4);
    TEST_FIELD(EMRSTRETCHBLT, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSTRETCHBLT, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, xDest, 24, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, yDest, 28, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, cxDest, 32, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, cyDest, 36, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, dwRop, 40, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, xSrc, 44, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, ySrc, 48, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, XFORM, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRSTRETCHBLT, COLORREF, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, DWORD, cbBitsSrc, 96, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, cxSrc, 100, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, LONG, cySrc, 104, 4, 4);
}

static void test_pack_EMRSTRETCHDIBITS(void)
{
    /* EMRSTRETCHDIBITS (pack 4) */
    TEST_TYPE(EMRSTRETCHDIBITS, 80, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, xDest, 24, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, yDest, 28, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, xSrc, 32, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, ySrc, 36, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, cxSrc, 40, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, cySrc, 44, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, offBmiSrc, 48, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, cbBmiSrc, 52, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, offBitsSrc, 56, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, cbBitsSrc, 60, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, iUsageSrc, 64, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, DWORD, dwRop, 68, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, cxDest, 72, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, LONG, cyDest, 76, 4, 4);
}

static void test_pack_EMRSTROKEANDFILLPATH(void)
{
    /* EMRSTROKEANDFILLPATH (pack 4) */
    TEST_TYPE(EMRSTROKEANDFILLPATH, 24, 4);
    TEST_FIELD(EMRSTROKEANDFILLPATH, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSTROKEANDFILLPATH, RECTL, rclBounds, 8, 16, 4);
}

static void test_pack_EMRSTROKEPATH(void)
{
    /* EMRSTROKEPATH (pack 4) */
    TEST_TYPE(EMRSTROKEPATH, 24, 4);
    TEST_FIELD(EMRSTROKEPATH, EMR, emr, 0, 8, 4);
    TEST_FIELD(EMRSTROKEPATH, RECTL, rclBounds, 8, 16, 4);
}

static void test_pack_EMRTEXT(void)
{
    /* EMRTEXT (pack 4) */
    TEST_TYPE(EMRTEXT, 40, 4);
    TEST_FIELD(EMRTEXT, POINTL, ptlReference, 0, 8, 4);
    TEST_FIELD(EMRTEXT, DWORD, nChars, 8, 4, 4);
    TEST_FIELD(EMRTEXT, DWORD, offString, 12, 4, 4);
    TEST_FIELD(EMRTEXT, DWORD, fOptions, 16, 4, 4);
    TEST_FIELD(EMRTEXT, RECTL, rcl, 20, 16, 4);
    TEST_FIELD(EMRTEXT, DWORD, offDx, 36, 4, 4);
}

static void test_pack_EMRWIDENPATH(void)
{
    /* EMRWIDENPATH (pack 4) */
    TEST_TYPE(EMRWIDENPATH, 8, 4);
    TEST_FIELD(EMRWIDENPATH, EMR, emr, 0, 8, 4);
}

static void test_pack_ENHMETAHEADER(void)
{
    /* ENHMETAHEADER (pack 4) */
    TEST_TYPE(ENHMETAHEADER, 108, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, iType, 0, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, nSize, 4, 4, 4);
    TEST_FIELD(ENHMETAHEADER, RECTL, rclBounds, 8, 16, 4);
    TEST_FIELD(ENHMETAHEADER, RECTL, rclFrame, 24, 16, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, dSignature, 40, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, nVersion, 44, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, nBytes, 48, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, nRecords, 52, 4, 4);
    TEST_FIELD(ENHMETAHEADER, WORD, nHandles, 56, 2, 2);
    TEST_FIELD(ENHMETAHEADER, WORD, sReserved, 58, 2, 2);
    TEST_FIELD(ENHMETAHEADER, DWORD, nDescription, 60, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, offDescription, 64, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, nPalEntries, 68, 4, 4);
    TEST_FIELD(ENHMETAHEADER, SIZEL, szlDevice, 72, 8, 4);
    TEST_FIELD(ENHMETAHEADER, SIZEL, szlMillimeters, 80, 8, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, cbPixelFormat, 88, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, offPixelFormat, 92, 4, 4);
    TEST_FIELD(ENHMETAHEADER, DWORD, bOpenGL, 96, 4, 4);
    TEST_FIELD(ENHMETAHEADER, SIZEL, szlMicrometers, 100, 8, 4);
}

static void test_pack_ENHMETARECORD(void)
{
    /* ENHMETARECORD (pack 4) */
    TEST_TYPE(ENHMETARECORD, 12, 4);
    TEST_FIELD(ENHMETARECORD, DWORD, iType, 0, 4, 4);
    TEST_FIELD(ENHMETARECORD, DWORD, nSize, 4, 4, 4);
    TEST_FIELD(ENHMETARECORD, DWORD[1], dParm, 8, 4, 4);
}

static void test_pack_ENHMFENUMPROC(void)
{
    /* ENHMFENUMPROC */
    TEST_TYPE(ENHMFENUMPROC, 4, 4);
}

static void test_pack_ENUMLOGFONTA(void)
{
    /* ENUMLOGFONTA (pack 4) */
    TEST_TYPE(ENUMLOGFONTA, 156, 4);
    TEST_FIELD(ENUMLOGFONTA, LOGFONTA, elfLogFont, 0, 60, 4);
    TEST_FIELD(ENUMLOGFONTA, BYTE[LF_FULLFACESIZE], elfFullName, 60, 64, 1);
    TEST_FIELD(ENUMLOGFONTA, BYTE[LF_FACESIZE], elfStyle, 124, 32, 1);
}

static void test_pack_ENUMLOGFONTEXA(void)
{
    /* ENUMLOGFONTEXA (pack 4) */
    TEST_TYPE(ENUMLOGFONTEXA, 188, 4);
    TEST_FIELD(ENUMLOGFONTEXA, LOGFONTA, elfLogFont, 0, 60, 4);
    TEST_FIELD(ENUMLOGFONTEXA, BYTE[LF_FULLFACESIZE], elfFullName, 60, 64, 1);
    TEST_FIELD(ENUMLOGFONTEXA, BYTE[LF_FACESIZE], elfStyle, 124, 32, 1);
    TEST_FIELD(ENUMLOGFONTEXA, BYTE[LF_FACESIZE], elfScript, 156, 32, 1);
}

static void test_pack_ENUMLOGFONTEXW(void)
{
    /* ENUMLOGFONTEXW (pack 4) */
    TEST_TYPE(ENUMLOGFONTEXW, 348, 4);
    TEST_FIELD(ENUMLOGFONTEXW, LOGFONTW, elfLogFont, 0, 92, 4);
    TEST_FIELD(ENUMLOGFONTEXW, WCHAR[LF_FULLFACESIZE], elfFullName, 92, 128, 2);
    TEST_FIELD(ENUMLOGFONTEXW, WCHAR[LF_FACESIZE], elfStyle, 220, 64, 2);
    TEST_FIELD(ENUMLOGFONTEXW, WCHAR[LF_FACESIZE], elfScript, 284, 64, 2);
}

static void test_pack_ENUMLOGFONTW(void)
{
    /* ENUMLOGFONTW (pack 4) */
    TEST_TYPE(ENUMLOGFONTW, 284, 4);
    TEST_FIELD(ENUMLOGFONTW, LOGFONTW, elfLogFont, 0, 92, 4);
    TEST_FIELD(ENUMLOGFONTW, WCHAR[LF_FULLFACESIZE], elfFullName, 92, 128, 2);
    TEST_FIELD(ENUMLOGFONTW, WCHAR[LF_FACESIZE], elfStyle, 220, 64, 2);
}

static void test_pack_EXTLOGFONTA(void)
{
    /* EXTLOGFONTA (pack 4) */
    TEST_TYPE(EXTLOGFONTA, 192, 4);
    TEST_FIELD(EXTLOGFONTA, LOGFONTA, elfLogFont, 0, 60, 4);
    TEST_FIELD(EXTLOGFONTA, BYTE[LF_FULLFACESIZE], elfFullName, 60, 64, 1);
    TEST_FIELD(EXTLOGFONTA, BYTE[LF_FACESIZE], elfStyle, 124, 32, 1);
    TEST_FIELD(EXTLOGFONTA, DWORD, elfVersion, 156, 4, 4);
    TEST_FIELD(EXTLOGFONTA, DWORD, elfStyleSize, 160, 4, 4);
    TEST_FIELD(EXTLOGFONTA, DWORD, elfMatch, 164, 4, 4);
    TEST_FIELD(EXTLOGFONTA, DWORD, elfReserved, 168, 4, 4);
    TEST_FIELD(EXTLOGFONTA, BYTE[ELF_VENDOR_SIZE], elfVendorId, 172, 4, 1);
    TEST_FIELD(EXTLOGFONTA, DWORD, elfCulture, 176, 4, 4);
    TEST_FIELD(EXTLOGFONTA, PANOSE, elfPanose, 180, 10, 1);
}

static void test_pack_EXTLOGFONTW(void)
{
    /* EXTLOGFONTW (pack 4) */
    TEST_TYPE(EXTLOGFONTW, 320, 4);
    TEST_FIELD(EXTLOGFONTW, LOGFONTW, elfLogFont, 0, 92, 4);
    TEST_FIELD(EXTLOGFONTW, WCHAR[LF_FULLFACESIZE], elfFullName, 92, 128, 2);
    TEST_FIELD(EXTLOGFONTW, WCHAR[LF_FACESIZE], elfStyle, 220, 64, 2);
    TEST_FIELD(EXTLOGFONTW, DWORD, elfVersion, 284, 4, 4);
    TEST_FIELD(EXTLOGFONTW, DWORD, elfStyleSize, 288, 4, 4);
    TEST_FIELD(EXTLOGFONTW, DWORD, elfMatch, 292, 4, 4);
    TEST_FIELD(EXTLOGFONTW, DWORD, elfReserved, 296, 4, 4);
    TEST_FIELD(EXTLOGFONTW, BYTE[ELF_VENDOR_SIZE], elfVendorId, 300, 4, 1);
    TEST_FIELD(EXTLOGFONTW, DWORD, elfCulture, 304, 4, 4);
    TEST_FIELD(EXTLOGFONTW, PANOSE, elfPanose, 308, 10, 1);
}

static void test_pack_EXTLOGPEN(void)
{
    /* EXTLOGPEN (pack 4) */
    TEST_TYPE(EXTLOGPEN, 28, 4);
    TEST_FIELD(EXTLOGPEN, DWORD, elpPenStyle, 0, 4, 4);
    TEST_FIELD(EXTLOGPEN, DWORD, elpWidth, 4, 4, 4);
    TEST_FIELD(EXTLOGPEN, UINT, elpBrushStyle, 8, 4, 4);
    TEST_FIELD(EXTLOGPEN, COLORREF, elpColor, 12, 4, 4);
    TEST_FIELD(EXTLOGPEN, ULONG_PTR, elpHatch, 16, 4, 4);
    TEST_FIELD(EXTLOGPEN, DWORD, elpNumEntries, 20, 4, 4);
    TEST_FIELD(EXTLOGPEN, DWORD[1], elpStyleEntry, 24, 4, 4);
}

static void test_pack_FIXED(void)
{
    /* FIXED (pack 4) */
    TEST_TYPE(FIXED, 4, 2);
    TEST_FIELD(FIXED, WORD, fract, 0, 2, 2);
    TEST_FIELD(FIXED, SHORT, value, 2, 2, 2);
}

static void test_pack_FONTENUMPROCA(void)
{
    /* FONTENUMPROCA */
    TEST_TYPE(FONTENUMPROCA, 4, 4);
}

static void test_pack_FONTENUMPROCW(void)
{
    /* FONTENUMPROCW */
    TEST_TYPE(FONTENUMPROCW, 4, 4);
}

static void test_pack_FONTSIGNATURE(void)
{
    /* FONTSIGNATURE (pack 4) */
    TEST_TYPE(FONTSIGNATURE, 24, 4);
    TEST_FIELD(FONTSIGNATURE, DWORD[4], fsUsb, 0, 16, 4);
    TEST_FIELD(FONTSIGNATURE, DWORD[2], fsCsb, 16, 8, 4);
}

static void test_pack_FXPT16DOT16(void)
{
    /* FXPT16DOT16 */
    TEST_TYPE(FXPT16DOT16, 4, 4);
}

static void test_pack_FXPT2DOT30(void)
{
    /* FXPT2DOT30 */
    TEST_TYPE(FXPT2DOT30, 4, 4);
}

static void test_pack_GCP_RESULTSA(void)
{
    /* GCP_RESULTSA (pack 4) */
    TEST_TYPE(GCP_RESULTSA, 36, 4);
    TEST_FIELD(GCP_RESULTSA, DWORD, lStructSize, 0, 4, 4);
    TEST_FIELD(GCP_RESULTSA, LPSTR, lpOutString, 4, 4, 4);
    TEST_FIELD(GCP_RESULTSA, UINT *, lpOrder, 8, 4, 4);
    TEST_FIELD(GCP_RESULTSA, INT *, lpDx, 12, 4, 4);
    TEST_FIELD(GCP_RESULTSA, INT *, lpCaretPos, 16, 4, 4);
    TEST_FIELD(GCP_RESULTSA, LPSTR, lpClass, 20, 4, 4);
    TEST_FIELD(GCP_RESULTSA, LPWSTR, lpGlyphs, 24, 4, 4);
    TEST_FIELD(GCP_RESULTSA, UINT, nGlyphs, 28, 4, 4);
    TEST_FIELD(GCP_RESULTSA, UINT, nMaxFit, 32, 4, 4);
}

static void test_pack_GCP_RESULTSW(void)
{
    /* GCP_RESULTSW (pack 4) */
    TEST_TYPE(GCP_RESULTSW, 36, 4);
    TEST_FIELD(GCP_RESULTSW, DWORD, lStructSize, 0, 4, 4);
    TEST_FIELD(GCP_RESULTSW, LPWSTR, lpOutString, 4, 4, 4);
    TEST_FIELD(GCP_RESULTSW, UINT *, lpOrder, 8, 4, 4);
    TEST_FIELD(GCP_RESULTSW, INT *, lpDx, 12, 4, 4);
    TEST_FIELD(GCP_RESULTSW, INT *, lpCaretPos, 16, 4, 4);
    TEST_FIELD(GCP_RESULTSW, LPSTR, lpClass, 20, 4, 4);
    TEST_FIELD(GCP_RESULTSW, LPWSTR, lpGlyphs, 24, 4, 4);
    TEST_FIELD(GCP_RESULTSW, UINT, nGlyphs, 28, 4, 4);
    TEST_FIELD(GCP_RESULTSW, UINT, nMaxFit, 32, 4, 4);
}

static void test_pack_GLYPHMETRICS(void)
{
    /* GLYPHMETRICS (pack 4) */
    TEST_TYPE(GLYPHMETRICS, 20, 4);
    TEST_FIELD(GLYPHMETRICS, UINT, gmBlackBoxX, 0, 4, 4);
    TEST_FIELD(GLYPHMETRICS, UINT, gmBlackBoxY, 4, 4, 4);
    TEST_FIELD(GLYPHMETRICS, POINT, gmptGlyphOrigin, 8, 8, 4);
    TEST_FIELD(GLYPHMETRICS, SHORT, gmCellIncX, 16, 2, 2);
    TEST_FIELD(GLYPHMETRICS, SHORT, gmCellIncY, 18, 2, 2);
}

static void test_pack_GLYPHMETRICSFLOAT(void)
{
    /* GLYPHMETRICSFLOAT (pack 4) */
    TEST_TYPE(GLYPHMETRICSFLOAT, 24, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, FLOAT, gmfBlackBoxX, 0, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, FLOAT, gmfBlackBoxY, 4, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, POINTFLOAT, gmfptGlyphOrigin, 8, 8, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, FLOAT, gmfCellIncX, 16, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, FLOAT, gmfCellIncY, 20, 4, 4);
}

static void test_pack_GOBJENUMPROC(void)
{
    /* GOBJENUMPROC */
    TEST_TYPE(GOBJENUMPROC, 4, 4);
}

static void test_pack_GRADIENT_RECT(void)
{
    /* GRADIENT_RECT (pack 4) */
    TEST_TYPE(GRADIENT_RECT, 8, 4);
    TEST_FIELD(GRADIENT_RECT, ULONG, UpperLeft, 0, 4, 4);
    TEST_FIELD(GRADIENT_RECT, ULONG, LowerRight, 4, 4, 4);
}

static void test_pack_GRADIENT_TRIANGLE(void)
{
    /* GRADIENT_TRIANGLE (pack 4) */
    TEST_TYPE(GRADIENT_TRIANGLE, 12, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, ULONG, Vertex1, 0, 4, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, ULONG, Vertex2, 4, 4, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, ULONG, Vertex3, 8, 4, 4);
}

static void test_pack_HANDLETABLE(void)
{
    /* HANDLETABLE (pack 4) */
    TEST_TYPE(HANDLETABLE, 4, 4);
    TEST_FIELD(HANDLETABLE, HGDIOBJ[1], objectHandle, 0, 4, 4);
}

static void test_pack_ICMENUMPROCA(void)
{
    /* ICMENUMPROCA */
    TEST_TYPE(ICMENUMPROCA, 4, 4);
}

static void test_pack_ICMENUMPROCW(void)
{
    /* ICMENUMPROCW */
    TEST_TYPE(ICMENUMPROCW, 4, 4);
}

static void test_pack_KERNINGPAIR(void)
{
    /* KERNINGPAIR (pack 4) */
    TEST_TYPE(KERNINGPAIR, 8, 4);
    TEST_FIELD(KERNINGPAIR, WORD, wFirst, 0, 2, 2);
    TEST_FIELD(KERNINGPAIR, WORD, wSecond, 2, 2, 2);
    TEST_FIELD(KERNINGPAIR, INT, iKernAmount, 4, 4, 4);
}

static void test_pack_LAYERPLANEDESCRIPTOR(void)
{
    /* LAYERPLANEDESCRIPTOR (pack 4) */
    TEST_TYPE(LAYERPLANEDESCRIPTOR, 32, 4);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, WORD, nSize, 0, 2, 2);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, WORD, nVersion, 2, 2, 2);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, iPixelType, 8, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cColorBits, 9, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cRedBits, 10, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cRedShift, 11, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cGreenBits, 12, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cGreenShift, 13, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cBlueBits, 14, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cBlueShift, 15, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAlphaBits, 16, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAlphaShift, 17, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAccumBits, 18, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAccumRedBits, 19, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAccumGreenBits, 20, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAccumBlueBits, 21, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAccumAlphaBits, 22, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cDepthBits, 23, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cStencilBits, 24, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, cAuxBuffers, 25, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, iLayerPlane, 26, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, BYTE, bReserved, 27, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, COLORREF, crTransparent, 28, 4, 4);
}

static void test_pack_LCSCSTYPE(void)
{
    /* LCSCSTYPE */
    TEST_TYPE(LCSCSTYPE, 4, 4);
}

static void test_pack_LCSGAMUTMATCH(void)
{
    /* LCSGAMUTMATCH */
    TEST_TYPE(LCSGAMUTMATCH, 4, 4);
}

static void test_pack_LINEDDAPROC(void)
{
    /* LINEDDAPROC */
    TEST_TYPE(LINEDDAPROC, 4, 4);
}

static void test_pack_LOCALESIGNATURE(void)
{
    /* LOCALESIGNATURE (pack 4) */
    TEST_TYPE(LOCALESIGNATURE, 32, 4);
    TEST_FIELD(LOCALESIGNATURE, DWORD[4], lsUsb, 0, 16, 4);
    TEST_FIELD(LOCALESIGNATURE, DWORD[2], lsCsbDefault, 16, 8, 4);
    TEST_FIELD(LOCALESIGNATURE, DWORD[2], lsCsbSupported, 24, 8, 4);
}

static void test_pack_LOGBRUSH(void)
{
    /* LOGBRUSH (pack 4) */
    TEST_TYPE(LOGBRUSH, 12, 4);
    TEST_FIELD(LOGBRUSH, UINT, lbStyle, 0, 4, 4);
    TEST_FIELD(LOGBRUSH, COLORREF, lbColor, 4, 4, 4);
    TEST_FIELD(LOGBRUSH, ULONG_PTR, lbHatch, 8, 4, 4);
}

static void test_pack_LOGCOLORSPACEA(void)
{
    /* LOGCOLORSPACEA (pack 4) */
    TEST_TYPE(LOGCOLORSPACEA, 328, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsSignature, 0, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsVersion, 4, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsSize, 8, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, LCSCSTYPE, lcsCSType, 12, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, LCSGAMUTMATCH, lcsIntent, 16, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, CIEXYZTRIPLE, lcsEndpoints, 20, 36, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsGammaRed, 56, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsGammaGreen, 60, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, DWORD, lcsGammaBlue, 64, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, CHAR[MAX_PATH], lcsFilename, 68, 260, 1);
}

static void test_pack_LOGCOLORSPACEW(void)
{
    /* LOGCOLORSPACEW (pack 4) */
    TEST_TYPE(LOGCOLORSPACEW, 588, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsSignature, 0, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsVersion, 4, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsSize, 8, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, LCSCSTYPE, lcsCSType, 12, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, LCSGAMUTMATCH, lcsIntent, 16, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, CIEXYZTRIPLE, lcsEndpoints, 20, 36, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsGammaRed, 56, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsGammaGreen, 60, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, DWORD, lcsGammaBlue, 64, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, WCHAR[MAX_PATH], lcsFilename, 68, 520, 2);
}

static void test_pack_LOGFONTA(void)
{
    /* LOGFONTA (pack 4) */
    TEST_TYPE(LOGFONTA, 60, 4);
    TEST_FIELD(LOGFONTA, LONG, lfHeight, 0, 4, 4);
    TEST_FIELD(LOGFONTA, LONG, lfWidth, 4, 4, 4);
    TEST_FIELD(LOGFONTA, LONG, lfEscapement, 8, 4, 4);
    TEST_FIELD(LOGFONTA, LONG, lfOrientation, 12, 4, 4);
    TEST_FIELD(LOGFONTA, LONG, lfWeight, 16, 4, 4);
    TEST_FIELD(LOGFONTA, BYTE, lfItalic, 20, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfUnderline, 21, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfStrikeOut, 22, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfCharSet, 23, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfOutPrecision, 24, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfClipPrecision, 25, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfQuality, 26, 1, 1);
    TEST_FIELD(LOGFONTA, BYTE, lfPitchAndFamily, 27, 1, 1);
    TEST_FIELD(LOGFONTA, CHAR[LF_FACESIZE], lfFaceName, 28, 32, 1);
}

static void test_pack_LOGFONTW(void)
{
    /* LOGFONTW (pack 4) */
    TEST_TYPE(LOGFONTW, 92, 4);
    TEST_FIELD(LOGFONTW, LONG, lfHeight, 0, 4, 4);
    TEST_FIELD(LOGFONTW, LONG, lfWidth, 4, 4, 4);
    TEST_FIELD(LOGFONTW, LONG, lfEscapement, 8, 4, 4);
    TEST_FIELD(LOGFONTW, LONG, lfOrientation, 12, 4, 4);
    TEST_FIELD(LOGFONTW, LONG, lfWeight, 16, 4, 4);
    TEST_FIELD(LOGFONTW, BYTE, lfItalic, 20, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfUnderline, 21, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfStrikeOut, 22, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfCharSet, 23, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfOutPrecision, 24, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfClipPrecision, 25, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfQuality, 26, 1, 1);
    TEST_FIELD(LOGFONTW, BYTE, lfPitchAndFamily, 27, 1, 1);
    TEST_FIELD(LOGFONTW, WCHAR[LF_FACESIZE], lfFaceName, 28, 64, 2);
}

static void test_pack_LOGPEN(void)
{
    /* LOGPEN (pack 4) */
    TEST_TYPE(LOGPEN, 16, 4);
    TEST_FIELD(LOGPEN, UINT, lopnStyle, 0, 4, 4);
    TEST_FIELD(LOGPEN, POINT, lopnWidth, 4, 8, 4);
    TEST_FIELD(LOGPEN, COLORREF, lopnColor, 12, 4, 4);
}

static void test_pack_LPABC(void)
{
    /* LPABC */
    TEST_TYPE(LPABC, 4, 4);
    TEST_TYPE_POINTER(LPABC, 12, 4);
}

static void test_pack_LPABCFLOAT(void)
{
    /* LPABCFLOAT */
    TEST_TYPE(LPABCFLOAT, 4, 4);
    TEST_TYPE_POINTER(LPABCFLOAT, 12, 4);
}

static void test_pack_LPBITMAP(void)
{
    /* LPBITMAP */
    TEST_TYPE(LPBITMAP, 4, 4);
    TEST_TYPE_POINTER(LPBITMAP, 24, 4);
}

static void test_pack_LPBITMAPCOREHEADER(void)
{
    /* LPBITMAPCOREHEADER */
    TEST_TYPE(LPBITMAPCOREHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPCOREHEADER, 12, 4);
}

static void test_pack_LPBITMAPCOREINFO(void)
{
    /* LPBITMAPCOREINFO */
    TEST_TYPE(LPBITMAPCOREINFO, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPCOREINFO, 16, 4);
}

static void test_pack_LPBITMAPFILEHEADER(void)
{
    /* LPBITMAPFILEHEADER */
    TEST_TYPE(LPBITMAPFILEHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPFILEHEADER, 14, 2);
}

static void test_pack_LPBITMAPINFO(void)
{
    /* LPBITMAPINFO */
    TEST_TYPE(LPBITMAPINFO, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPINFO, 44, 4);
}

static void test_pack_LPBITMAPINFOHEADER(void)
{
    /* LPBITMAPINFOHEADER */
    TEST_TYPE(LPBITMAPINFOHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPINFOHEADER, 40, 4);
}

static void test_pack_LPBITMAPV5HEADER(void)
{
    /* LPBITMAPV5HEADER */
    TEST_TYPE(LPBITMAPV5HEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPV5HEADER, 124, 4);
}

static void test_pack_LPCHARSETINFO(void)
{
    /* LPCHARSETINFO */
    TEST_TYPE(LPCHARSETINFO, 4, 4);
    TEST_TYPE_POINTER(LPCHARSETINFO, 32, 4);
}

static void test_pack_LPCIEXYZ(void)
{
    /* LPCIEXYZ */
    TEST_TYPE(LPCIEXYZ, 4, 4);
    TEST_TYPE_POINTER(LPCIEXYZ, 12, 4);
}

static void test_pack_LPCIEXYZTRIPLE(void)
{
    /* LPCIEXYZTRIPLE */
    TEST_TYPE(LPCIEXYZTRIPLE, 4, 4);
    TEST_TYPE_POINTER(LPCIEXYZTRIPLE, 36, 4);
}

static void test_pack_LPCOLORADJUSTMENT(void)
{
    /* LPCOLORADJUSTMENT */
    TEST_TYPE(LPCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(LPCOLORADJUSTMENT, 24, 2);
}

static void test_pack_LPDEVMODEA(void)
{
    /* LPDEVMODEA */
    TEST_TYPE(LPDEVMODEA, 4, 4);
}

static void test_pack_LPDEVMODEW(void)
{
    /* LPDEVMODEW */
    TEST_TYPE(LPDEVMODEW, 4, 4);
}

static void test_pack_LPDIBSECTION(void)
{
    /* LPDIBSECTION */
    TEST_TYPE(LPDIBSECTION, 4, 4);
    TEST_TYPE_POINTER(LPDIBSECTION, 84, 4);
}

static void test_pack_LPDISPLAY_DEVICEA(void)
{
    /* LPDISPLAY_DEVICEA */
    TEST_TYPE(LPDISPLAY_DEVICEA, 4, 4);
    TEST_TYPE_POINTER(LPDISPLAY_DEVICEA, 424, 4);
}

static void test_pack_LPDISPLAY_DEVICEW(void)
{
    /* LPDISPLAY_DEVICEW */
    TEST_TYPE(LPDISPLAY_DEVICEW, 4, 4);
    TEST_TYPE_POINTER(LPDISPLAY_DEVICEW, 840, 4);
}

static void test_pack_LPDOCINFOA(void)
{
    /* LPDOCINFOA */
    TEST_TYPE(LPDOCINFOA, 4, 4);
    TEST_TYPE_POINTER(LPDOCINFOA, 20, 4);
}

static void test_pack_LPDOCINFOW(void)
{
    /* LPDOCINFOW */
    TEST_TYPE(LPDOCINFOW, 4, 4);
    TEST_TYPE_POINTER(LPDOCINFOW, 20, 4);
}

static void test_pack_LPENHMETAHEADER(void)
{
    /* LPENHMETAHEADER */
    TEST_TYPE(LPENHMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(LPENHMETAHEADER, 108, 4);
}

static void test_pack_LPENHMETARECORD(void)
{
    /* LPENHMETARECORD */
    TEST_TYPE(LPENHMETARECORD, 4, 4);
    TEST_TYPE_POINTER(LPENHMETARECORD, 12, 4);
}

static void test_pack_LPENUMLOGFONTA(void)
{
    /* LPENUMLOGFONTA */
    TEST_TYPE(LPENUMLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTA, 156, 4);
}

static void test_pack_LPENUMLOGFONTEXA(void)
{
    /* LPENUMLOGFONTEXA */
    TEST_TYPE(LPENUMLOGFONTEXA, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTEXA, 188, 4);
}

static void test_pack_LPENUMLOGFONTEXW(void)
{
    /* LPENUMLOGFONTEXW */
    TEST_TYPE(LPENUMLOGFONTEXW, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTEXW, 348, 4);
}

static void test_pack_LPENUMLOGFONTW(void)
{
    /* LPENUMLOGFONTW */
    TEST_TYPE(LPENUMLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTW, 284, 4);
}

static void test_pack_LPEXTLOGFONTA(void)
{
    /* LPEXTLOGFONTA */
    TEST_TYPE(LPEXTLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGFONTA, 192, 4);
}

static void test_pack_LPEXTLOGFONTW(void)
{
    /* LPEXTLOGFONTW */
    TEST_TYPE(LPEXTLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGFONTW, 320, 4);
}

static void test_pack_LPEXTLOGPEN(void)
{
    /* LPEXTLOGPEN */
    TEST_TYPE(LPEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGPEN, 28, 4);
}

static void test_pack_LPFONTSIGNATURE(void)
{
    /* LPFONTSIGNATURE */
    TEST_TYPE(LPFONTSIGNATURE, 4, 4);
    TEST_TYPE_POINTER(LPFONTSIGNATURE, 24, 4);
}

static void test_pack_LPGCP_RESULTSA(void)
{
    /* LPGCP_RESULTSA */
    TEST_TYPE(LPGCP_RESULTSA, 4, 4);
    TEST_TYPE_POINTER(LPGCP_RESULTSA, 36, 4);
}

static void test_pack_LPGCP_RESULTSW(void)
{
    /* LPGCP_RESULTSW */
    TEST_TYPE(LPGCP_RESULTSW, 4, 4);
    TEST_TYPE_POINTER(LPGCP_RESULTSW, 36, 4);
}

static void test_pack_LPGLYPHMETRICS(void)
{
    /* LPGLYPHMETRICS */
    TEST_TYPE(LPGLYPHMETRICS, 4, 4);
    TEST_TYPE_POINTER(LPGLYPHMETRICS, 20, 4);
}

static void test_pack_LPGLYPHMETRICSFLOAT(void)
{
    /* LPGLYPHMETRICSFLOAT */
    TEST_TYPE(LPGLYPHMETRICSFLOAT, 4, 4);
    TEST_TYPE_POINTER(LPGLYPHMETRICSFLOAT, 24, 4);
}

static void test_pack_LPGRADIENT_RECT(void)
{
    /* LPGRADIENT_RECT */
    TEST_TYPE(LPGRADIENT_RECT, 4, 4);
    TEST_TYPE_POINTER(LPGRADIENT_RECT, 8, 4);
}

static void test_pack_LPGRADIENT_TRIANGLE(void)
{
    /* LPGRADIENT_TRIANGLE */
    TEST_TYPE(LPGRADIENT_TRIANGLE, 4, 4);
    TEST_TYPE_POINTER(LPGRADIENT_TRIANGLE, 12, 4);
}

static void test_pack_LPHANDLETABLE(void)
{
    /* LPHANDLETABLE */
    TEST_TYPE(LPHANDLETABLE, 4, 4);
    TEST_TYPE_POINTER(LPHANDLETABLE, 4, 4);
}

static void test_pack_LPKERNINGPAIR(void)
{
    /* LPKERNINGPAIR */
    TEST_TYPE(LPKERNINGPAIR, 4, 4);
    TEST_TYPE_POINTER(LPKERNINGPAIR, 8, 4);
}

static void test_pack_LPLAYERPLANEDESCRIPTOR(void)
{
    /* LPLAYERPLANEDESCRIPTOR */
    TEST_TYPE(LPLAYERPLANEDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(LPLAYERPLANEDESCRIPTOR, 32, 4);
}

static void test_pack_LPLOCALESIGNATURE(void)
{
    /* LPLOCALESIGNATURE */
    TEST_TYPE(LPLOCALESIGNATURE, 4, 4);
    TEST_TYPE_POINTER(LPLOCALESIGNATURE, 32, 4);
}

static void test_pack_LPLOGBRUSH(void)
{
    /* LPLOGBRUSH */
    TEST_TYPE(LPLOGBRUSH, 4, 4);
    TEST_TYPE_POINTER(LPLOGBRUSH, 12, 4);
}

static void test_pack_LPLOGCOLORSPACEA(void)
{
    /* LPLOGCOLORSPACEA */
    TEST_TYPE(LPLOGCOLORSPACEA, 4, 4);
    TEST_TYPE_POINTER(LPLOGCOLORSPACEA, 328, 4);
}

static void test_pack_LPLOGCOLORSPACEW(void)
{
    /* LPLOGCOLORSPACEW */
    TEST_TYPE(LPLOGCOLORSPACEW, 4, 4);
    TEST_TYPE_POINTER(LPLOGCOLORSPACEW, 588, 4);
}

static void test_pack_LPLOGFONTA(void)
{
    /* LPLOGFONTA */
    TEST_TYPE(LPLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPLOGFONTA, 60, 4);
}

static void test_pack_LPLOGFONTW(void)
{
    /* LPLOGFONTW */
    TEST_TYPE(LPLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPLOGFONTW, 92, 4);
}

static void test_pack_LPLOGPEN(void)
{
    /* LPLOGPEN */
    TEST_TYPE(LPLOGPEN, 4, 4);
    TEST_TYPE_POINTER(LPLOGPEN, 16, 4);
}

static void test_pack_LPMAT2(void)
{
    /* LPMAT2 */
    TEST_TYPE(LPMAT2, 4, 4);
    TEST_TYPE_POINTER(LPMAT2, 16, 2);
}

static void test_pack_LPMETAFILEPICT(void)
{
    /* LPMETAFILEPICT */
    TEST_TYPE(LPMETAFILEPICT, 4, 4);
    TEST_TYPE_POINTER(LPMETAFILEPICT, 16, 4);
}

static void test_pack_LPMETAHEADER(void)
{
    /* LPMETAHEADER */
    TEST_TYPE(LPMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(LPMETAHEADER, 18, 2);
}

static void test_pack_LPMETARECORD(void)
{
    /* LPMETARECORD */
    TEST_TYPE(LPMETARECORD, 4, 4);
    TEST_TYPE_POINTER(LPMETARECORD, 8, 4);
}

static void test_pack_LPNEWTEXTMETRICA(void)
{
    /* LPNEWTEXTMETRICA */
    TEST_TYPE(LPNEWTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPNEWTEXTMETRICA, 72, 4);
}

static void test_pack_LPNEWTEXTMETRICW(void)
{
    /* LPNEWTEXTMETRICW */
    TEST_TYPE(LPNEWTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPNEWTEXTMETRICW, 76, 4);
}

static void test_pack_LPOUTLINETEXTMETRICA(void)
{
    /* LPOUTLINETEXTMETRICA */
    TEST_TYPE(LPOUTLINETEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPOUTLINETEXTMETRICA, 212, 4);
}

static void test_pack_LPOUTLINETEXTMETRICW(void)
{
    /* LPOUTLINETEXTMETRICW */
    TEST_TYPE(LPOUTLINETEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPOUTLINETEXTMETRICW, 216, 4);
}

static void test_pack_LPPANOSE(void)
{
    /* LPPANOSE */
    TEST_TYPE(LPPANOSE, 4, 4);
    TEST_TYPE_POINTER(LPPANOSE, 10, 1);
}

static void test_pack_LPPELARRAY(void)
{
    /* LPPELARRAY */
    TEST_TYPE(LPPELARRAY, 4, 4);
    TEST_TYPE_POINTER(LPPELARRAY, 20, 4);
}

static void test_pack_LPPIXELFORMATDESCRIPTOR(void)
{
    /* LPPIXELFORMATDESCRIPTOR */
    TEST_TYPE(LPPIXELFORMATDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(LPPIXELFORMATDESCRIPTOR, 40, 4);
}

static void test_pack_LPPOINTFX(void)
{
    /* LPPOINTFX */
    TEST_TYPE(LPPOINTFX, 4, 4);
    TEST_TYPE_POINTER(LPPOINTFX, 8, 2);
}

static void test_pack_LPPOLYTEXTA(void)
{
    /* LPPOLYTEXTA */
    TEST_TYPE(LPPOLYTEXTA, 4, 4);
    TEST_TYPE_POINTER(LPPOLYTEXTA, 40, 4);
}

static void test_pack_LPPOLYTEXTW(void)
{
    /* LPPOLYTEXTW */
    TEST_TYPE(LPPOLYTEXTW, 4, 4);
    TEST_TYPE_POINTER(LPPOLYTEXTW, 40, 4);
}

static void test_pack_LPRASTERIZER_STATUS(void)
{
    /* LPRASTERIZER_STATUS */
    TEST_TYPE(LPRASTERIZER_STATUS, 4, 4);
    TEST_TYPE_POINTER(LPRASTERIZER_STATUS, 6, 2);
}

static void test_pack_LPRGBQUAD(void)
{
    /* LPRGBQUAD */
    TEST_TYPE(LPRGBQUAD, 4, 4);
    TEST_TYPE_POINTER(LPRGBQUAD, 4, 1);
}

static void test_pack_LPRGNDATA(void)
{
    /* LPRGNDATA */
    TEST_TYPE(LPRGNDATA, 4, 4);
    TEST_TYPE_POINTER(LPRGNDATA, 36, 4);
}

static void test_pack_LPTEXTMETRICA(void)
{
    /* LPTEXTMETRICA */
    TEST_TYPE(LPTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPTEXTMETRICA, 56, 4);
}

static void test_pack_LPTEXTMETRICW(void)
{
    /* LPTEXTMETRICW */
    TEST_TYPE(LPTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPTEXTMETRICW, 60, 4);
}

static void test_pack_LPTRIVERTEX(void)
{
    /* LPTRIVERTEX */
    TEST_TYPE(LPTRIVERTEX, 4, 4);
    TEST_TYPE_POINTER(LPTRIVERTEX, 16, 4);
}

static void test_pack_LPTTPOLYCURVE(void)
{
    /* LPTTPOLYCURVE */
    TEST_TYPE(LPTTPOLYCURVE, 4, 4);
    TEST_TYPE_POINTER(LPTTPOLYCURVE, 12, 2);
}

static void test_pack_LPTTPOLYGONHEADER(void)
{
    /* LPTTPOLYGONHEADER */
    TEST_TYPE(LPTTPOLYGONHEADER, 4, 4);
    TEST_TYPE_POINTER(LPTTPOLYGONHEADER, 16, 4);
}

static void test_pack_LPXFORM(void)
{
    /* LPXFORM */
    TEST_TYPE(LPXFORM, 4, 4);
    TEST_TYPE_POINTER(LPXFORM, 24, 4);
}

static void test_pack_MAT2(void)
{
    /* MAT2 (pack 4) */
    TEST_TYPE(MAT2, 16, 2);
    TEST_FIELD(MAT2, FIXED, eM11, 0, 4, 2);
    TEST_FIELD(MAT2, FIXED, eM12, 4, 4, 2);
    TEST_FIELD(MAT2, FIXED, eM21, 8, 4, 2);
    TEST_FIELD(MAT2, FIXED, eM22, 12, 4, 2);
}

static void test_pack_METAFILEPICT(void)
{
    /* METAFILEPICT (pack 4) */
    TEST_TYPE(METAFILEPICT, 16, 4);
    TEST_FIELD(METAFILEPICT, LONG, mm, 0, 4, 4);
    TEST_FIELD(METAFILEPICT, LONG, xExt, 4, 4, 4);
    TEST_FIELD(METAFILEPICT, LONG, yExt, 8, 4, 4);
    TEST_FIELD(METAFILEPICT, HMETAFILE, hMF, 12, 4, 4);
}

static void test_pack_METAHEADER(void)
{
    /* METAHEADER (pack 2) */
    TEST_TYPE(METAHEADER, 18, 2);
    TEST_FIELD(METAHEADER, WORD, mtType, 0, 2, 2);
    TEST_FIELD(METAHEADER, WORD, mtHeaderSize, 2, 2, 2);
    TEST_FIELD(METAHEADER, WORD, mtVersion, 4, 2, 2);
    TEST_FIELD(METAHEADER, DWORD, mtSize, 6, 4, 2);
    TEST_FIELD(METAHEADER, WORD, mtNoObjects, 10, 2, 2);
    TEST_FIELD(METAHEADER, DWORD, mtMaxRecord, 12, 4, 2);
    TEST_FIELD(METAHEADER, WORD, mtNoParameters, 16, 2, 2);
}

static void test_pack_METARECORD(void)
{
    /* METARECORD (pack 4) */
    TEST_TYPE(METARECORD, 8, 4);
    TEST_FIELD(METARECORD, DWORD, rdSize, 0, 4, 4);
    TEST_FIELD(METARECORD, WORD, rdFunction, 4, 2, 2);
    TEST_FIELD(METARECORD, WORD[1], rdParm, 6, 2, 2);
}

static void test_pack_MFENUMPROC(void)
{
    /* MFENUMPROC */
    TEST_TYPE(MFENUMPROC, 4, 4);
}

static void test_pack_NEWTEXTMETRICA(void)
{
    /* NEWTEXTMETRICA (pack 4) */
    TEST_TYPE(NEWTEXTMETRICA, 72, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmHeight, 0, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmAscent, 4, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmDescent, 8, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmWeight, 28, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmOverhang, 32, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, LONG, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmFirstChar, 44, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmLastChar, 45, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmDefaultChar, 46, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmBreakChar, 47, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmItalic, 48, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmUnderlined, 49, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmStruckOut, 50, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmPitchAndFamily, 51, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, BYTE, tmCharSet, 52, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, DWORD, ntmFlags, 56, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, UINT, ntmSizeEM, 60, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, UINT, ntmCellHeight, 64, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, UINT, ntmAvgWidth, 68, 4, 4);
}

static void test_pack_NEWTEXTMETRICEXA(void)
{
    /* NEWTEXTMETRICEXA (pack 4) */
    TEST_TYPE(NEWTEXTMETRICEXA, 96, 4);
    TEST_FIELD(NEWTEXTMETRICEXA, NEWTEXTMETRICA, ntmTm, 0, 72, 4);
    TEST_FIELD(NEWTEXTMETRICEXA, FONTSIGNATURE, ntmFontSig, 72, 24, 4);
}

static void test_pack_NEWTEXTMETRICEXW(void)
{
    /* NEWTEXTMETRICEXW (pack 4) */
    TEST_TYPE(NEWTEXTMETRICEXW, 100, 4);
    TEST_FIELD(NEWTEXTMETRICEXW, NEWTEXTMETRICW, ntmTm, 0, 76, 4);
    TEST_FIELD(NEWTEXTMETRICEXW, FONTSIGNATURE, ntmFontSig, 76, 24, 4);
}

static void test_pack_NEWTEXTMETRICW(void)
{
    /* NEWTEXTMETRICW (pack 4) */
    TEST_TYPE(NEWTEXTMETRICW, 76, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmHeight, 0, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmAscent, 4, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmDescent, 8, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmWeight, 28, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmOverhang, 32, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, LONG, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, WCHAR, tmFirstChar, 44, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, WCHAR, tmLastChar, 46, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, WCHAR, tmDefaultChar, 48, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, WCHAR, tmBreakChar, 50, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, BYTE, tmItalic, 52, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, BYTE, tmUnderlined, 53, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, BYTE, tmStruckOut, 54, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, BYTE, tmPitchAndFamily, 55, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, BYTE, tmCharSet, 56, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, DWORD, ntmFlags, 60, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, UINT, ntmSizeEM, 64, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, UINT, ntmCellHeight, 68, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, UINT, ntmAvgWidth, 72, 4, 4);
}

static void test_pack_NPEXTLOGPEN(void)
{
    /* NPEXTLOGPEN */
    TEST_TYPE(NPEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(NPEXTLOGPEN, 28, 4);
}

static void test_pack_OLDFONTENUMPROC(void)
{
    /* OLDFONTENUMPROC */
    TEST_TYPE(OLDFONTENUMPROC, 4, 4);
}

static void test_pack_OLDFONTENUMPROCA(void)
{
    /* OLDFONTENUMPROCA */
    TEST_TYPE(OLDFONTENUMPROCA, 4, 4);
}

static void test_pack_OLDFONTENUMPROCW(void)
{
    /* OLDFONTENUMPROCW */
    TEST_TYPE(OLDFONTENUMPROCW, 4, 4);
}

static void test_pack_OUTLINETEXTMETRICA(void)
{
    /* OUTLINETEXTMETRICA (pack 4) */
    TEST_TYPE(OUTLINETEXTMETRICA, 212, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmSize, 0, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, TEXTMETRICA, otmTextMetrics, 4, 56, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, BYTE, otmFiller, 60, 1, 1);
    TEST_FIELD(OUTLINETEXTMETRICA, PANOSE, otmPanoseNumber, 61, 10, 1);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmfsSelection, 72, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmfsType, 76, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmsCharSlopeRise, 80, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmsCharSlopeRun, 84, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmItalicAngle, 88, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmEMSquare, 92, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmAscent, 96, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmDescent, 100, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmLineGap, 104, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmsCapEmHeight, 108, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmsXHeight, 112, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, RECT, otmrcFontBox, 116, 16, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmMacAscent, 132, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmMacDescent, 136, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmMacLineGap, 140, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmusMinimumPPEM, 144, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, POINT, otmptSubscriptSize, 148, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, POINT, otmptSubscriptOffset, 156, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, POINT, otmptSuperscriptSize, 164, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, POINT, otmptSuperscriptOffset, 172, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, UINT, otmsStrikeoutSize, 180, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmsStrikeoutPosition, 184, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmsUnderscoreSize, 188, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, INT, otmsUnderscorePosition, 192, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, LPSTR, otmpFamilyName, 196, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, LPSTR, otmpFaceName, 200, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, LPSTR, otmpStyleName, 204, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, LPSTR, otmpFullName, 208, 4, 4);
}

static void test_pack_OUTLINETEXTMETRICW(void)
{
    /* OUTLINETEXTMETRICW (pack 4) */
    TEST_TYPE(OUTLINETEXTMETRICW, 216, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmSize, 0, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, TEXTMETRICW, otmTextMetrics, 4, 60, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, BYTE, otmFiller, 64, 1, 1);
    TEST_FIELD(OUTLINETEXTMETRICW, PANOSE, otmPanoseNumber, 65, 10, 1);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmfsSelection, 76, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmfsType, 80, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmsCharSlopeRise, 84, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmsCharSlopeRun, 88, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmItalicAngle, 92, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmEMSquare, 96, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmAscent, 100, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmDescent, 104, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmLineGap, 108, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmsCapEmHeight, 112, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmsXHeight, 116, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, RECT, otmrcFontBox, 120, 16, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmMacAscent, 136, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmMacDescent, 140, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmMacLineGap, 144, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmusMinimumPPEM, 148, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, POINT, otmptSubscriptSize, 152, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, POINT, otmptSubscriptOffset, 160, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, POINT, otmptSuperscriptSize, 168, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, POINT, otmptSuperscriptOffset, 176, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, UINT, otmsStrikeoutSize, 184, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmsStrikeoutPosition, 188, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmsUnderscoreSize, 192, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, INT, otmsUnderscorePosition, 196, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, LPSTR, otmpFamilyName, 200, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, LPSTR, otmpFaceName, 204, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, LPSTR, otmpStyleName, 208, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, LPSTR, otmpFullName, 212, 4, 4);
}

static void test_pack_PABC(void)
{
    /* PABC */
    TEST_TYPE(PABC, 4, 4);
    TEST_TYPE_POINTER(PABC, 12, 4);
}

static void test_pack_PABCFLOAT(void)
{
    /* PABCFLOAT */
    TEST_TYPE(PABCFLOAT, 4, 4);
    TEST_TYPE_POINTER(PABCFLOAT, 12, 4);
}

static void test_pack_PANOSE(void)
{
    /* PANOSE (pack 4) */
    TEST_TYPE(PANOSE, 10, 1);
    TEST_FIELD(PANOSE, BYTE, bFamilyType, 0, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bSerifStyle, 1, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bWeight, 2, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bProportion, 3, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bContrast, 4, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bStrokeVariation, 5, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bArmStyle, 6, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bLetterform, 7, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bMidline, 8, 1, 1);
    TEST_FIELD(PANOSE, BYTE, bXHeight, 9, 1, 1);
}

static void test_pack_PATTERN(void)
{
    /* PATTERN */
    TEST_TYPE(PATTERN, 12, 4);
}

static void test_pack_PBITMAP(void)
{
    /* PBITMAP */
    TEST_TYPE(PBITMAP, 4, 4);
    TEST_TYPE_POINTER(PBITMAP, 24, 4);
}

static void test_pack_PBITMAPCOREHEADER(void)
{
    /* PBITMAPCOREHEADER */
    TEST_TYPE(PBITMAPCOREHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPCOREHEADER, 12, 4);
}

static void test_pack_PBITMAPCOREINFO(void)
{
    /* PBITMAPCOREINFO */
    TEST_TYPE(PBITMAPCOREINFO, 4, 4);
    TEST_TYPE_POINTER(PBITMAPCOREINFO, 16, 4);
}

static void test_pack_PBITMAPFILEHEADER(void)
{
    /* PBITMAPFILEHEADER */
    TEST_TYPE(PBITMAPFILEHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPFILEHEADER, 14, 2);
}

static void test_pack_PBITMAPINFO(void)
{
    /* PBITMAPINFO */
    TEST_TYPE(PBITMAPINFO, 4, 4);
    TEST_TYPE_POINTER(PBITMAPINFO, 44, 4);
}

static void test_pack_PBITMAPINFOHEADER(void)
{
    /* PBITMAPINFOHEADER */
    TEST_TYPE(PBITMAPINFOHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPINFOHEADER, 40, 4);
}

static void test_pack_PBITMAPV4HEADER(void)
{
    /* PBITMAPV4HEADER */
    TEST_TYPE(PBITMAPV4HEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPV4HEADER, 108, 4);
}

static void test_pack_PBITMAPV5HEADER(void)
{
    /* PBITMAPV5HEADER */
    TEST_TYPE(PBITMAPV5HEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPV5HEADER, 124, 4);
}

static void test_pack_PBLENDFUNCTION(void)
{
    /* PBLENDFUNCTION */
    TEST_TYPE(PBLENDFUNCTION, 4, 4);
    TEST_TYPE_POINTER(PBLENDFUNCTION, 4, 1);
}

static void test_pack_PCHARSETINFO(void)
{
    /* PCHARSETINFO */
    TEST_TYPE(PCHARSETINFO, 4, 4);
    TEST_TYPE_POINTER(PCHARSETINFO, 32, 4);
}

static void test_pack_PCOLORADJUSTMENT(void)
{
    /* PCOLORADJUSTMENT */
    TEST_TYPE(PCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(PCOLORADJUSTMENT, 24, 2);
}

static void test_pack_PDEVMODEA(void)
{
    /* PDEVMODEA */
    TEST_TYPE(PDEVMODEA, 4, 4);
}

static void test_pack_PDEVMODEW(void)
{
    /* PDEVMODEW */
    TEST_TYPE(PDEVMODEW, 4, 4);
}

static void test_pack_PDIBSECTION(void)
{
    /* PDIBSECTION */
    TEST_TYPE(PDIBSECTION, 4, 4);
    TEST_TYPE_POINTER(PDIBSECTION, 84, 4);
}

static void test_pack_PDISPLAY_DEVICEA(void)
{
    /* PDISPLAY_DEVICEA */
    TEST_TYPE(PDISPLAY_DEVICEA, 4, 4);
    TEST_TYPE_POINTER(PDISPLAY_DEVICEA, 424, 4);
}

static void test_pack_PDISPLAY_DEVICEW(void)
{
    /* PDISPLAY_DEVICEW */
    TEST_TYPE(PDISPLAY_DEVICEW, 4, 4);
    TEST_TYPE_POINTER(PDISPLAY_DEVICEW, 840, 4);
}

static void test_pack_PELARRAY(void)
{
    /* PELARRAY (pack 4) */
    TEST_TYPE(PELARRAY, 20, 4);
    TEST_FIELD(PELARRAY, LONG, paXCount, 0, 4, 4);
    TEST_FIELD(PELARRAY, LONG, paYCount, 4, 4, 4);
    TEST_FIELD(PELARRAY, LONG, paXExt, 8, 4, 4);
    TEST_FIELD(PELARRAY, LONG, paYExt, 12, 4, 4);
    TEST_FIELD(PELARRAY, BYTE, paRGBs, 16, 1, 1);
}

static void test_pack_PEMR(void)
{
    /* PEMR */
    TEST_TYPE(PEMR, 4, 4);
    TEST_TYPE_POINTER(PEMR, 8, 4);
}

static void test_pack_PEMRABORTPATH(void)
{
    /* PEMRABORTPATH */
    TEST_TYPE(PEMRABORTPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRABORTPATH, 8, 4);
}

static void test_pack_PEMRANGLEARC(void)
{
    /* PEMRANGLEARC */
    TEST_TYPE(PEMRANGLEARC, 4, 4);
    TEST_TYPE_POINTER(PEMRANGLEARC, 28, 4);
}

static void test_pack_PEMRARC(void)
{
    /* PEMRARC */
    TEST_TYPE(PEMRARC, 4, 4);
    TEST_TYPE_POINTER(PEMRARC, 40, 4);
}

static void test_pack_PEMRARCTO(void)
{
    /* PEMRARCTO */
    TEST_TYPE(PEMRARCTO, 4, 4);
    TEST_TYPE_POINTER(PEMRARCTO, 40, 4);
}

static void test_pack_PEMRBEGINPATH(void)
{
    /* PEMRBEGINPATH */
    TEST_TYPE(PEMRBEGINPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRBEGINPATH, 8, 4);
}

static void test_pack_PEMRBITBLT(void)
{
    /* PEMRBITBLT */
    TEST_TYPE(PEMRBITBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRBITBLT, 100, 4);
}

static void test_pack_PEMRCHORD(void)
{
    /* PEMRCHORD */
    TEST_TYPE(PEMRCHORD, 4, 4);
    TEST_TYPE_POINTER(PEMRCHORD, 40, 4);
}

static void test_pack_PEMRCLOSEFIGURE(void)
{
    /* PEMRCLOSEFIGURE */
    TEST_TYPE(PEMRCLOSEFIGURE, 4, 4);
    TEST_TYPE_POINTER(PEMRCLOSEFIGURE, 8, 4);
}

static void test_pack_PEMRCREATEBRUSHINDIRECT(void)
{
    /* PEMRCREATEBRUSHINDIRECT */
    TEST_TYPE(PEMRCREATEBRUSHINDIRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEBRUSHINDIRECT, 24, 4);
}

static void test_pack_PEMRCREATECOLORSPACE(void)
{
    /* PEMRCREATECOLORSPACE */
    TEST_TYPE(PEMRCREATECOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATECOLORSPACE, 340, 4);
}

static void test_pack_PEMRCREATECOLORSPACEW(void)
{
    /* PEMRCREATECOLORSPACEW */
    TEST_TYPE(PEMRCREATECOLORSPACEW, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATECOLORSPACEW, 612, 4);
}

static void test_pack_PEMRCREATEDIBPATTERNBRUSHPT(void)
{
    /* PEMRCREATEDIBPATTERNBRUSHPT */
    TEST_TYPE(PEMRCREATEDIBPATTERNBRUSHPT, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEDIBPATTERNBRUSHPT, 32, 4);
}

static void test_pack_PEMRCREATEMONOBRUSH(void)
{
    /* PEMRCREATEMONOBRUSH */
    TEST_TYPE(PEMRCREATEMONOBRUSH, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEMONOBRUSH, 32, 4);
}

static void test_pack_PEMRCREATEPALETTE(void)
{
    /* PEMRCREATEPALETTE */
    TEST_TYPE(PEMRCREATEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEPALETTE, 20, 4);
}

static void test_pack_PEMRCREATEPEN(void)
{
    /* PEMRCREATEPEN */
    TEST_TYPE(PEMRCREATEPEN, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEPEN, 28, 4);
}

static void test_pack_PEMRDELETECOLORSPACE(void)
{
    /* PEMRDELETECOLORSPACE */
    TEST_TYPE(PEMRDELETECOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRDELETECOLORSPACE, 12, 4);
}

static void test_pack_PEMRDELETEOBJECT(void)
{
    /* PEMRDELETEOBJECT */
    TEST_TYPE(PEMRDELETEOBJECT, 4, 4);
    TEST_TYPE_POINTER(PEMRDELETEOBJECT, 12, 4);
}

static void test_pack_PEMRELLIPSE(void)
{
    /* PEMRELLIPSE */
    TEST_TYPE(PEMRELLIPSE, 4, 4);
    TEST_TYPE_POINTER(PEMRELLIPSE, 24, 4);
}

static void test_pack_PEMRENDPATH(void)
{
    /* PEMRENDPATH */
    TEST_TYPE(PEMRENDPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRENDPATH, 8, 4);
}

static void test_pack_PEMREOF(void)
{
    /* PEMREOF */
    TEST_TYPE(PEMREOF, 4, 4);
    TEST_TYPE_POINTER(PEMREOF, 20, 4);
}

static void test_pack_PEMREXCLUDECLIPRECT(void)
{
    /* PEMREXCLUDECLIPRECT */
    TEST_TYPE(PEMREXCLUDECLIPRECT, 4, 4);
    TEST_TYPE_POINTER(PEMREXCLUDECLIPRECT, 24, 4);
}

static void test_pack_PEMREXTCREATEFONTINDIRECTW(void)
{
    /* PEMREXTCREATEFONTINDIRECTW */
    TEST_TYPE(PEMREXTCREATEFONTINDIRECTW, 4, 4);
    TEST_TYPE_POINTER(PEMREXTCREATEFONTINDIRECTW, 332, 4);
}

static void test_pack_PEMREXTCREATEPEN(void)
{
    /* PEMREXTCREATEPEN */
    TEST_TYPE(PEMREXTCREATEPEN, 4, 4);
    TEST_TYPE_POINTER(PEMREXTCREATEPEN, 56, 4);
}

static void test_pack_PEMREXTFLOODFILL(void)
{
    /* PEMREXTFLOODFILL */
    TEST_TYPE(PEMREXTFLOODFILL, 4, 4);
    TEST_TYPE_POINTER(PEMREXTFLOODFILL, 24, 4);
}

static void test_pack_PEMREXTSELECTCLIPRGN(void)
{
    /* PEMREXTSELECTCLIPRGN */
    TEST_TYPE(PEMREXTSELECTCLIPRGN, 4, 4);
    TEST_TYPE_POINTER(PEMREXTSELECTCLIPRGN, 20, 4);
}

static void test_pack_PEMREXTTEXTOUTA(void)
{
    /* PEMREXTTEXTOUTA */
    TEST_TYPE(PEMREXTTEXTOUTA, 4, 4);
    TEST_TYPE_POINTER(PEMREXTTEXTOUTA, 76, 4);
}

static void test_pack_PEMREXTTEXTOUTW(void)
{
    /* PEMREXTTEXTOUTW */
    TEST_TYPE(PEMREXTTEXTOUTW, 4, 4);
    TEST_TYPE_POINTER(PEMREXTTEXTOUTW, 76, 4);
}

static void test_pack_PEMRFILLPATH(void)
{
    /* PEMRFILLPATH */
    TEST_TYPE(PEMRFILLPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRFILLPATH, 24, 4);
}

static void test_pack_PEMRFILLRGN(void)
{
    /* PEMRFILLRGN */
    TEST_TYPE(PEMRFILLRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRFILLRGN, 36, 4);
}

static void test_pack_PEMRFLATTENPATH(void)
{
    /* PEMRFLATTENPATH */
    TEST_TYPE(PEMRFLATTENPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRFLATTENPATH, 8, 4);
}

static void test_pack_PEMRFORMAT(void)
{
    /* PEMRFORMAT */
    TEST_TYPE(PEMRFORMAT, 4, 4);
    TEST_TYPE_POINTER(PEMRFORMAT, 16, 4);
}

static void test_pack_PEMRFRAMERGN(void)
{
    /* PEMRFRAMERGN */
    TEST_TYPE(PEMRFRAMERGN, 4, 4);
    TEST_TYPE_POINTER(PEMRFRAMERGN, 44, 4);
}

static void test_pack_PEMRGDICOMMENT(void)
{
    /* PEMRGDICOMMENT */
    TEST_TYPE(PEMRGDICOMMENT, 4, 4);
    TEST_TYPE_POINTER(PEMRGDICOMMENT, 16, 4);
}

static void test_pack_PEMRGLSBOUNDEDRECORD(void)
{
    /* PEMRGLSBOUNDEDRECORD */
    TEST_TYPE(PEMRGLSBOUNDEDRECORD, 4, 4);
    TEST_TYPE_POINTER(PEMRGLSBOUNDEDRECORD, 32, 4);
}

static void test_pack_PEMRGLSRECORD(void)
{
    /* PEMRGLSRECORD */
    TEST_TYPE(PEMRGLSRECORD, 4, 4);
    TEST_TYPE_POINTER(PEMRGLSRECORD, 16, 4);
}

static void test_pack_PEMRINTERSECTCLIPRECT(void)
{
    /* PEMRINTERSECTCLIPRECT */
    TEST_TYPE(PEMRINTERSECTCLIPRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRINTERSECTCLIPRECT, 24, 4);
}

static void test_pack_PEMRINVERTRGN(void)
{
    /* PEMRINVERTRGN */
    TEST_TYPE(PEMRINVERTRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRINVERTRGN, 32, 4);
}

static void test_pack_PEMRLINETO(void)
{
    /* PEMRLINETO */
    TEST_TYPE(PEMRLINETO, 4, 4);
    TEST_TYPE_POINTER(PEMRLINETO, 16, 4);
}

static void test_pack_PEMRMASKBLT(void)
{
    /* PEMRMASKBLT */
    TEST_TYPE(PEMRMASKBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRMASKBLT, 128, 4);
}

static void test_pack_PEMRMODIFYWORLDTRANSFORM(void)
{
    /* PEMRMODIFYWORLDTRANSFORM */
    TEST_TYPE(PEMRMODIFYWORLDTRANSFORM, 4, 4);
    TEST_TYPE_POINTER(PEMRMODIFYWORLDTRANSFORM, 36, 4);
}

static void test_pack_PEMRMOVETOEX(void)
{
    /* PEMRMOVETOEX */
    TEST_TYPE(PEMRMOVETOEX, 4, 4);
    TEST_TYPE_POINTER(PEMRMOVETOEX, 16, 4);
}

static void test_pack_PEMROFFSETCLIPRGN(void)
{
    /* PEMROFFSETCLIPRGN */
    TEST_TYPE(PEMROFFSETCLIPRGN, 4, 4);
    TEST_TYPE_POINTER(PEMROFFSETCLIPRGN, 16, 4);
}

static void test_pack_PEMRPAINTRGN(void)
{
    /* PEMRPAINTRGN */
    TEST_TYPE(PEMRPAINTRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRPAINTRGN, 32, 4);
}

static void test_pack_PEMRPIE(void)
{
    /* PEMRPIE */
    TEST_TYPE(PEMRPIE, 4, 4);
    TEST_TYPE_POINTER(PEMRPIE, 40, 4);
}

static void test_pack_PEMRPIXELFORMAT(void)
{
    /* PEMRPIXELFORMAT */
    TEST_TYPE(PEMRPIXELFORMAT, 4, 4);
    TEST_TYPE_POINTER(PEMRPIXELFORMAT, 48, 4);
}

static void test_pack_PEMRPLGBLT(void)
{
    /* PEMRPLGBLT */
    TEST_TYPE(PEMRPLGBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRPLGBLT, 140, 4);
}

static void test_pack_PEMRPOLYBEZIER(void)
{
    /* PEMRPOLYBEZIER */
    TEST_TYPE(PEMRPOLYBEZIER, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIER, 36, 4);
}

static void test_pack_PEMRPOLYBEZIER16(void)
{
    /* PEMRPOLYBEZIER16 */
    TEST_TYPE(PEMRPOLYBEZIER16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIER16, 32, 4);
}

static void test_pack_PEMRPOLYBEZIERTO(void)
{
    /* PEMRPOLYBEZIERTO */
    TEST_TYPE(PEMRPOLYBEZIERTO, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIERTO, 36, 4);
}

static void test_pack_PEMRPOLYBEZIERTO16(void)
{
    /* PEMRPOLYBEZIERTO16 */
    TEST_TYPE(PEMRPOLYBEZIERTO16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIERTO16, 32, 4);
}

static void test_pack_PEMRPOLYDRAW(void)
{
    /* PEMRPOLYDRAW */
    TEST_TYPE(PEMRPOLYDRAW, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYDRAW, 40, 4);
}

static void test_pack_PEMRPOLYDRAW16(void)
{
    /* PEMRPOLYDRAW16 */
    TEST_TYPE(PEMRPOLYDRAW16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYDRAW16, 36, 4);
}

static void test_pack_PEMRPOLYGON(void)
{
    /* PEMRPOLYGON */
    TEST_TYPE(PEMRPOLYGON, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYGON, 36, 4);
}

static void test_pack_PEMRPOLYGON16(void)
{
    /* PEMRPOLYGON16 */
    TEST_TYPE(PEMRPOLYGON16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYGON16, 32, 4);
}

static void test_pack_PEMRPOLYLINE(void)
{
    /* PEMRPOLYLINE */
    TEST_TYPE(PEMRPOLYLINE, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINE, 36, 4);
}

static void test_pack_PEMRPOLYLINE16(void)
{
    /* PEMRPOLYLINE16 */
    TEST_TYPE(PEMRPOLYLINE16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINE16, 32, 4);
}

static void test_pack_PEMRPOLYLINETO(void)
{
    /* PEMRPOLYLINETO */
    TEST_TYPE(PEMRPOLYLINETO, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINETO, 36, 4);
}

static void test_pack_PEMRPOLYLINETO16(void)
{
    /* PEMRPOLYLINETO16 */
    TEST_TYPE(PEMRPOLYLINETO16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINETO16, 32, 4);
}

static void test_pack_PEMRPOLYPOLYGON(void)
{
    /* PEMRPOLYPOLYGON */
    TEST_TYPE(PEMRPOLYPOLYGON, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYGON, 44, 4);
}

static void test_pack_PEMRPOLYPOLYGON16(void)
{
    /* PEMRPOLYPOLYGON16 */
    TEST_TYPE(PEMRPOLYPOLYGON16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYGON16, 40, 4);
}

static void test_pack_PEMRPOLYPOLYLINE(void)
{
    /* PEMRPOLYPOLYLINE */
    TEST_TYPE(PEMRPOLYPOLYLINE, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYLINE, 44, 4);
}

static void test_pack_PEMRPOLYPOLYLINE16(void)
{
    /* PEMRPOLYPOLYLINE16 */
    TEST_TYPE(PEMRPOLYPOLYLINE16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYLINE16, 40, 4);
}

static void test_pack_PEMRPOLYTEXTOUTA(void)
{
    /* PEMRPOLYTEXTOUTA */
    TEST_TYPE(PEMRPOLYTEXTOUTA, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYTEXTOUTA, 80, 4);
}

static void test_pack_PEMRPOLYTEXTOUTW(void)
{
    /* PEMRPOLYTEXTOUTW */
    TEST_TYPE(PEMRPOLYTEXTOUTW, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYTEXTOUTW, 80, 4);
}

static void test_pack_PEMRREALIZEPALETTE(void)
{
    /* PEMRREALIZEPALETTE */
    TEST_TYPE(PEMRREALIZEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRREALIZEPALETTE, 8, 4);
}

static void test_pack_PEMRRECTANGLE(void)
{
    /* PEMRRECTANGLE */
    TEST_TYPE(PEMRRECTANGLE, 4, 4);
    TEST_TYPE_POINTER(PEMRRECTANGLE, 24, 4);
}

static void test_pack_PEMRRESIZEPALETTE(void)
{
    /* PEMRRESIZEPALETTE */
    TEST_TYPE(PEMRRESIZEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRRESIZEPALETTE, 16, 4);
}

static void test_pack_PEMRRESTOREDC(void)
{
    /* PEMRRESTOREDC */
    TEST_TYPE(PEMRRESTOREDC, 4, 4);
    TEST_TYPE_POINTER(PEMRRESTOREDC, 12, 4);
}

static void test_pack_PEMRROUNDRECT(void)
{
    /* PEMRROUNDRECT */
    TEST_TYPE(PEMRROUNDRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRROUNDRECT, 32, 4);
}

static void test_pack_PEMRSAVEDC(void)
{
    /* PEMRSAVEDC */
    TEST_TYPE(PEMRSAVEDC, 4, 4);
    TEST_TYPE_POINTER(PEMRSAVEDC, 8, 4);
}

static void test_pack_PEMRSCALEVIEWPORTEXTEX(void)
{
    /* PEMRSCALEVIEWPORTEXTEX */
    TEST_TYPE(PEMRSCALEVIEWPORTEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSCALEVIEWPORTEXTEX, 24, 4);
}

static void test_pack_PEMRSCALEWINDOWEXTEX(void)
{
    /* PEMRSCALEWINDOWEXTEX */
    TEST_TYPE(PEMRSCALEWINDOWEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSCALEWINDOWEXTEX, 24, 4);
}

static void test_pack_PEMRSELECTCLIPPATH(void)
{
    /* PEMRSELECTCLIPPATH */
    TEST_TYPE(PEMRSELECTCLIPPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTCLIPPATH, 12, 4);
}

static void test_pack_PEMRSELECTCOLORSPACE(void)
{
    /* PEMRSELECTCOLORSPACE */
    TEST_TYPE(PEMRSELECTCOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTCOLORSPACE, 12, 4);
}

static void test_pack_PEMRSELECTOBJECT(void)
{
    /* PEMRSELECTOBJECT */
    TEST_TYPE(PEMRSELECTOBJECT, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTOBJECT, 12, 4);
}

static void test_pack_PEMRSELECTPALETTE(void)
{
    /* PEMRSELECTPALETTE */
    TEST_TYPE(PEMRSELECTPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTPALETTE, 12, 4);
}

static void test_pack_PEMRSETARCDIRECTION(void)
{
    /* PEMRSETARCDIRECTION */
    TEST_TYPE(PEMRSETARCDIRECTION, 4, 4);
    TEST_TYPE_POINTER(PEMRSETARCDIRECTION, 12, 4);
}

static void test_pack_PEMRSETBKCOLOR(void)
{
    /* PEMRSETBKCOLOR */
    TEST_TYPE(PEMRSETBKCOLOR, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBKCOLOR, 12, 4);
}

static void test_pack_PEMRSETBKMODE(void)
{
    /* PEMRSETBKMODE */
    TEST_TYPE(PEMRSETBKMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBKMODE, 12, 4);
}

static void test_pack_PEMRSETBRUSHORGEX(void)
{
    /* PEMRSETBRUSHORGEX */
    TEST_TYPE(PEMRSETBRUSHORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBRUSHORGEX, 16, 4);
}

static void test_pack_PEMRSETCOLORADJUSTMENT(void)
{
    /* PEMRSETCOLORADJUSTMENT */
    TEST_TYPE(PEMRSETCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETCOLORADJUSTMENT, 32, 4);
}

static void test_pack_PEMRSETCOLORSPACE(void)
{
    /* PEMRSETCOLORSPACE */
    TEST_TYPE(PEMRSETCOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETCOLORSPACE, 12, 4);
}

static void test_pack_PEMRSETDIBITSTODEVICE(void)
{
    /* PEMRSETDIBITSTODEVICE */
    TEST_TYPE(PEMRSETDIBITSTODEVICE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETDIBITSTODEVICE, 76, 4);
}

static void test_pack_PEMRSETICMMODE(void)
{
    /* PEMRSETICMMODE */
    TEST_TYPE(PEMRSETICMMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETICMMODE, 12, 4);
}

static void test_pack_PEMRSETLAYOUT(void)
{
    /* PEMRSETLAYOUT */
    TEST_TYPE(PEMRSETLAYOUT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETLAYOUT, 12, 4);
}

static void test_pack_PEMRSETMAPMODE(void)
{
    /* PEMRSETMAPMODE */
    TEST_TYPE(PEMRSETMAPMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMAPMODE, 12, 4);
}

static void test_pack_PEMRSETMAPPERFLAGS(void)
{
    /* PEMRSETMAPPERFLAGS */
    TEST_TYPE(PEMRSETMAPPERFLAGS, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMAPPERFLAGS, 12, 4);
}

static void test_pack_PEMRSETMETARGN(void)
{
    /* PEMRSETMETARGN */
    TEST_TYPE(PEMRSETMETARGN, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMETARGN, 8, 4);
}

static void test_pack_PEMRSETMITERLIMIT(void)
{
    /* PEMRSETMITERLIMIT */
    TEST_TYPE(PEMRSETMITERLIMIT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMITERLIMIT, 12, 4);
}

static void test_pack_PEMRSETPALETTEENTRIES(void)
{
    /* PEMRSETPALETTEENTRIES */
    TEST_TYPE(PEMRSETPALETTEENTRIES, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPALETTEENTRIES, 24, 4);
}

static void test_pack_PEMRSETPIXELV(void)
{
    /* PEMRSETPIXELV */
    TEST_TYPE(PEMRSETPIXELV, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPIXELV, 20, 4);
}

static void test_pack_PEMRSETPOLYFILLMODE(void)
{
    /* PEMRSETPOLYFILLMODE */
    TEST_TYPE(PEMRSETPOLYFILLMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPOLYFILLMODE, 12, 4);
}

static void test_pack_PEMRSETROP2(void)
{
    /* PEMRSETROP2 */
    TEST_TYPE(PEMRSETROP2, 4, 4);
    TEST_TYPE_POINTER(PEMRSETROP2, 12, 4);
}

static void test_pack_PEMRSETSTRETCHBLTMODE(void)
{
    /* PEMRSETSTRETCHBLTMODE */
    TEST_TYPE(PEMRSETSTRETCHBLTMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETSTRETCHBLTMODE, 12, 4);
}

static void test_pack_PEMRSETTEXTALIGN(void)
{
    /* PEMRSETTEXTALIGN */
    TEST_TYPE(PEMRSETTEXTALIGN, 4, 4);
    TEST_TYPE_POINTER(PEMRSETTEXTALIGN, 12, 4);
}

static void test_pack_PEMRSETTEXTCOLOR(void)
{
    /* PEMRSETTEXTCOLOR */
    TEST_TYPE(PEMRSETTEXTCOLOR, 4, 4);
    TEST_TYPE_POINTER(PEMRSETTEXTCOLOR, 12, 4);
}

static void test_pack_PEMRSETVIEWPORTEXTEX(void)
{
    /* PEMRSETVIEWPORTEXTEX */
    TEST_TYPE(PEMRSETVIEWPORTEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETVIEWPORTEXTEX, 16, 4);
}

static void test_pack_PEMRSETVIEWPORTORGEX(void)
{
    /* PEMRSETVIEWPORTORGEX */
    TEST_TYPE(PEMRSETVIEWPORTORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETVIEWPORTORGEX, 16, 4);
}

static void test_pack_PEMRSETWINDOWEXTEX(void)
{
    /* PEMRSETWINDOWEXTEX */
    TEST_TYPE(PEMRSETWINDOWEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWINDOWEXTEX, 16, 4);
}

static void test_pack_PEMRSETWINDOWORGEX(void)
{
    /* PEMRSETWINDOWORGEX */
    TEST_TYPE(PEMRSETWINDOWORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWINDOWORGEX, 16, 4);
}

static void test_pack_PEMRSETWORLDTRANSFORM(void)
{
    /* PEMRSETWORLDTRANSFORM */
    TEST_TYPE(PEMRSETWORLDTRANSFORM, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWORLDTRANSFORM, 32, 4);
}

static void test_pack_PEMRSTRETCHBLT(void)
{
    /* PEMRSTRETCHBLT */
    TEST_TYPE(PEMRSTRETCHBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRSTRETCHBLT, 108, 4);
}

static void test_pack_PEMRSTRETCHDIBITS(void)
{
    /* PEMRSTRETCHDIBITS */
    TEST_TYPE(PEMRSTRETCHDIBITS, 4, 4);
    TEST_TYPE_POINTER(PEMRSTRETCHDIBITS, 80, 4);
}

static void test_pack_PEMRSTROKEANDFILLPATH(void)
{
    /* PEMRSTROKEANDFILLPATH */
    TEST_TYPE(PEMRSTROKEANDFILLPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSTROKEANDFILLPATH, 24, 4);
}

static void test_pack_PEMRSTROKEPATH(void)
{
    /* PEMRSTROKEPATH */
    TEST_TYPE(PEMRSTROKEPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSTROKEPATH, 24, 4);
}

static void test_pack_PEMRTEXT(void)
{
    /* PEMRTEXT */
    TEST_TYPE(PEMRTEXT, 4, 4);
    TEST_TYPE_POINTER(PEMRTEXT, 40, 4);
}

static void test_pack_PEMRWIDENPATH(void)
{
    /* PEMRWIDENPATH */
    TEST_TYPE(PEMRWIDENPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRWIDENPATH, 8, 4);
}

static void test_pack_PENHMETAHEADER(void)
{
    /* PENHMETAHEADER */
    TEST_TYPE(PENHMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(PENHMETAHEADER, 108, 4);
}

static void test_pack_PEXTLOGFONTA(void)
{
    /* PEXTLOGFONTA */
    TEST_TYPE(PEXTLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGFONTA, 192, 4);
}

static void test_pack_PEXTLOGFONTW(void)
{
    /* PEXTLOGFONTW */
    TEST_TYPE(PEXTLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGFONTW, 320, 4);
}

static void test_pack_PEXTLOGPEN(void)
{
    /* PEXTLOGPEN */
    TEST_TYPE(PEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGPEN, 28, 4);
}

static void test_pack_PFONTSIGNATURE(void)
{
    /* PFONTSIGNATURE */
    TEST_TYPE(PFONTSIGNATURE, 4, 4);
    TEST_TYPE_POINTER(PFONTSIGNATURE, 24, 4);
}

static void test_pack_PGLYPHMETRICSFLOAT(void)
{
    /* PGLYPHMETRICSFLOAT */
    TEST_TYPE(PGLYPHMETRICSFLOAT, 4, 4);
    TEST_TYPE_POINTER(PGLYPHMETRICSFLOAT, 24, 4);
}

static void test_pack_PGRADIENT_RECT(void)
{
    /* PGRADIENT_RECT */
    TEST_TYPE(PGRADIENT_RECT, 4, 4);
    TEST_TYPE_POINTER(PGRADIENT_RECT, 8, 4);
}

static void test_pack_PGRADIENT_TRIANGLE(void)
{
    /* PGRADIENT_TRIANGLE */
    TEST_TYPE(PGRADIENT_TRIANGLE, 4, 4);
    TEST_TYPE_POINTER(PGRADIENT_TRIANGLE, 12, 4);
}

static void test_pack_PHANDLETABLE(void)
{
    /* PHANDLETABLE */
    TEST_TYPE(PHANDLETABLE, 4, 4);
    TEST_TYPE_POINTER(PHANDLETABLE, 4, 4);
}

static void test_pack_PIXELFORMATDESCRIPTOR(void)
{
    /* PIXELFORMATDESCRIPTOR (pack 4) */
    TEST_TYPE(PIXELFORMATDESCRIPTOR, 40, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, WORD, nSize, 0, 2, 2);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, WORD, nVersion, 2, 2, 2);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, DWORD, dwFlags, 4, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, iPixelType, 8, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cColorBits, 9, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cRedBits, 10, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cRedShift, 11, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cGreenBits, 12, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cGreenShift, 13, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cBlueBits, 14, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cBlueShift, 15, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAlphaBits, 16, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAlphaShift, 17, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAccumBits, 18, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAccumRedBits, 19, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAccumGreenBits, 20, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAccumBlueBits, 21, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAccumAlphaBits, 22, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cDepthBits, 23, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cStencilBits, 24, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, cAuxBuffers, 25, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, iLayerType, 26, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, BYTE, bReserved, 27, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, DWORD, dwLayerMask, 28, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, DWORD, dwVisibleMask, 32, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, DWORD, dwDamageMask, 36, 4, 4);
}

static void test_pack_PLAYERPLANEDESCRIPTOR(void)
{
    /* PLAYERPLANEDESCRIPTOR */
    TEST_TYPE(PLAYERPLANEDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PLAYERPLANEDESCRIPTOR, 32, 4);
}

static void test_pack_PLOCALESIGNATURE(void)
{
    /* PLOCALESIGNATURE */
    TEST_TYPE(PLOCALESIGNATURE, 4, 4);
    TEST_TYPE_POINTER(PLOCALESIGNATURE, 32, 4);
}

static void test_pack_PLOGBRUSH(void)
{
    /* PLOGBRUSH */
    TEST_TYPE(PLOGBRUSH, 4, 4);
    TEST_TYPE_POINTER(PLOGBRUSH, 12, 4);
}

static void test_pack_PLOGFONTA(void)
{
    /* PLOGFONTA */
    TEST_TYPE(PLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(PLOGFONTA, 60, 4);
}

static void test_pack_PLOGFONTW(void)
{
    /* PLOGFONTW */
    TEST_TYPE(PLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(PLOGFONTW, 92, 4);
}

static void test_pack_PMETAHEADER(void)
{
    /* PMETAHEADER */
    TEST_TYPE(PMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(PMETAHEADER, 18, 2);
}

static void test_pack_PMETARECORD(void)
{
    /* PMETARECORD */
    TEST_TYPE(PMETARECORD, 4, 4);
    TEST_TYPE_POINTER(PMETARECORD, 8, 4);
}

static void test_pack_PNEWTEXTMETRICA(void)
{
    /* PNEWTEXTMETRICA */
    TEST_TYPE(PNEWTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(PNEWTEXTMETRICA, 72, 4);
}

static void test_pack_PNEWTEXTMETRICW(void)
{
    /* PNEWTEXTMETRICW */
    TEST_TYPE(PNEWTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(PNEWTEXTMETRICW, 76, 4);
}

static void test_pack_POINTFLOAT(void)
{
    /* POINTFLOAT (pack 4) */
    TEST_TYPE(POINTFLOAT, 8, 4);
    TEST_FIELD(POINTFLOAT, FLOAT, x, 0, 4, 4);
    TEST_FIELD(POINTFLOAT, FLOAT, y, 4, 4, 4);
}

static void test_pack_POINTFX(void)
{
    /* POINTFX (pack 4) */
    TEST_TYPE(POINTFX, 8, 2);
    TEST_FIELD(POINTFX, FIXED, x, 0, 4, 2);
    TEST_FIELD(POINTFX, FIXED, y, 4, 4, 2);
}

static void test_pack_POLYTEXTA(void)
{
    /* POLYTEXTA (pack 4) */
    TEST_TYPE(POLYTEXTA, 40, 4);
    TEST_FIELD(POLYTEXTA, INT, x, 0, 4, 4);
    TEST_FIELD(POLYTEXTA, INT, y, 4, 4, 4);
    TEST_FIELD(POLYTEXTA, UINT, n, 8, 4, 4);
    TEST_FIELD(POLYTEXTA, LPCSTR, lpstr, 12, 4, 4);
    TEST_FIELD(POLYTEXTA, UINT, uiFlags, 16, 4, 4);
    TEST_FIELD(POLYTEXTA, RECT, rcl, 20, 16, 4);
    TEST_FIELD(POLYTEXTA, INT *, pdx, 36, 4, 4);
}

static void test_pack_POLYTEXTW(void)
{
    /* POLYTEXTW (pack 4) */
    TEST_TYPE(POLYTEXTW, 40, 4);
    TEST_FIELD(POLYTEXTW, INT, x, 0, 4, 4);
    TEST_FIELD(POLYTEXTW, INT, y, 4, 4, 4);
    TEST_FIELD(POLYTEXTW, UINT, n, 8, 4, 4);
    TEST_FIELD(POLYTEXTW, LPCWSTR, lpstr, 12, 4, 4);
    TEST_FIELD(POLYTEXTW, UINT, uiFlags, 16, 4, 4);
    TEST_FIELD(POLYTEXTW, RECT, rcl, 20, 16, 4);
    TEST_FIELD(POLYTEXTW, INT *, pdx, 36, 4, 4);
}

static void test_pack_POUTLINETEXTMETRICA(void)
{
    /* POUTLINETEXTMETRICA */
    TEST_TYPE(POUTLINETEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(POUTLINETEXTMETRICA, 212, 4);
}

static void test_pack_POUTLINETEXTMETRICW(void)
{
    /* POUTLINETEXTMETRICW */
    TEST_TYPE(POUTLINETEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(POUTLINETEXTMETRICW, 216, 4);
}

static void test_pack_PPELARRAY(void)
{
    /* PPELARRAY */
    TEST_TYPE(PPELARRAY, 4, 4);
    TEST_TYPE_POINTER(PPELARRAY, 20, 4);
}

static void test_pack_PPIXELFORMATDESCRIPTOR(void)
{
    /* PPIXELFORMATDESCRIPTOR */
    TEST_TYPE(PPIXELFORMATDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PPIXELFORMATDESCRIPTOR, 40, 4);
}

static void test_pack_PPOINTFLOAT(void)
{
    /* PPOINTFLOAT */
    TEST_TYPE(PPOINTFLOAT, 4, 4);
    TEST_TYPE_POINTER(PPOINTFLOAT, 8, 4);
}

static void test_pack_PPOLYTEXTA(void)
{
    /* PPOLYTEXTA */
    TEST_TYPE(PPOLYTEXTA, 4, 4);
    TEST_TYPE_POINTER(PPOLYTEXTA, 40, 4);
}

static void test_pack_PPOLYTEXTW(void)
{
    /* PPOLYTEXTW */
    TEST_TYPE(PPOLYTEXTW, 4, 4);
    TEST_TYPE_POINTER(PPOLYTEXTW, 40, 4);
}

static void test_pack_PRGNDATA(void)
{
    /* PRGNDATA */
    TEST_TYPE(PRGNDATA, 4, 4);
    TEST_TYPE_POINTER(PRGNDATA, 36, 4);
}

static void test_pack_PRGNDATAHEADER(void)
{
    /* PRGNDATAHEADER */
    TEST_TYPE(PRGNDATAHEADER, 4, 4);
    TEST_TYPE_POINTER(PRGNDATAHEADER, 32, 4);
}

static void test_pack_PTEXTMETRICA(void)
{
    /* PTEXTMETRICA */
    TEST_TYPE(PTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(PTEXTMETRICA, 56, 4);
}

static void test_pack_PTEXTMETRICW(void)
{
    /* PTEXTMETRICW */
    TEST_TYPE(PTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(PTEXTMETRICW, 60, 4);
}

static void test_pack_PTRIVERTEX(void)
{
    /* PTRIVERTEX */
    TEST_TYPE(PTRIVERTEX, 4, 4);
    TEST_TYPE_POINTER(PTRIVERTEX, 16, 4);
}

static void test_pack_PXFORM(void)
{
    /* PXFORM */
    TEST_TYPE(PXFORM, 4, 4);
    TEST_TYPE_POINTER(PXFORM, 24, 4);
}

static void test_pack_RASTERIZER_STATUS(void)
{
    /* RASTERIZER_STATUS (pack 4) */
    TEST_TYPE(RASTERIZER_STATUS, 6, 2);
    TEST_FIELD(RASTERIZER_STATUS, SHORT, nSize, 0, 2, 2);
    TEST_FIELD(RASTERIZER_STATUS, SHORT, wFlags, 2, 2, 2);
    TEST_FIELD(RASTERIZER_STATUS, SHORT, nLanguageID, 4, 2, 2);
}

static void test_pack_RGBQUAD(void)
{
    /* RGBQUAD (pack 4) */
    TEST_TYPE(RGBQUAD, 4, 1);
    TEST_FIELD(RGBQUAD, BYTE, rgbBlue, 0, 1, 1);
    TEST_FIELD(RGBQUAD, BYTE, rgbGreen, 1, 1, 1);
    TEST_FIELD(RGBQUAD, BYTE, rgbRed, 2, 1, 1);
    TEST_FIELD(RGBQUAD, BYTE, rgbReserved, 3, 1, 1);
}

static void test_pack_RGBTRIPLE(void)
{
    /* RGBTRIPLE (pack 4) */
    TEST_TYPE(RGBTRIPLE, 3, 1);
    TEST_FIELD(RGBTRIPLE, BYTE, rgbtBlue, 0, 1, 1);
    TEST_FIELD(RGBTRIPLE, BYTE, rgbtGreen, 1, 1, 1);
    TEST_FIELD(RGBTRIPLE, BYTE, rgbtRed, 2, 1, 1);
}

static void test_pack_RGNDATA(void)
{
    /* RGNDATA (pack 4) */
    TEST_TYPE(RGNDATA, 36, 4);
    TEST_FIELD(RGNDATA, RGNDATAHEADER, rdh, 0, 32, 4);
    TEST_FIELD(RGNDATA, char[1], Buffer, 32, 1, 1);
}

static void test_pack_RGNDATAHEADER(void)
{
    /* RGNDATAHEADER (pack 4) */
    TEST_TYPE(RGNDATAHEADER, 32, 4);
    TEST_FIELD(RGNDATAHEADER, DWORD, dwSize, 0, 4, 4);
    TEST_FIELD(RGNDATAHEADER, DWORD, iType, 4, 4, 4);
    TEST_FIELD(RGNDATAHEADER, DWORD, nCount, 8, 4, 4);
    TEST_FIELD(RGNDATAHEADER, DWORD, nRgnSize, 12, 4, 4);
    TEST_FIELD(RGNDATAHEADER, RECT, rcBound, 16, 16, 4);
}

static void test_pack_TEXTMETRICA(void)
{
    /* TEXTMETRICA (pack 4) */
    TEST_TYPE(TEXTMETRICA, 56, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmHeight, 0, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmAscent, 4, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmDescent, 8, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmWeight, 28, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmOverhang, 32, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(TEXTMETRICA, LONG, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(TEXTMETRICA, BYTE, tmFirstChar, 44, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmLastChar, 45, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmDefaultChar, 46, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmBreakChar, 47, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmItalic, 48, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmUnderlined, 49, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmStruckOut, 50, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmPitchAndFamily, 51, 1, 1);
    TEST_FIELD(TEXTMETRICA, BYTE, tmCharSet, 52, 1, 1);
}

static void test_pack_TEXTMETRICW(void)
{
    /* TEXTMETRICW (pack 4) */
    TEST_TYPE(TEXTMETRICW, 60, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmHeight, 0, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmAscent, 4, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmDescent, 8, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmWeight, 28, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmOverhang, 32, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(TEXTMETRICW, LONG, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(TEXTMETRICW, WCHAR, tmFirstChar, 44, 2, 2);
    TEST_FIELD(TEXTMETRICW, WCHAR, tmLastChar, 46, 2, 2);
    TEST_FIELD(TEXTMETRICW, WCHAR, tmDefaultChar, 48, 2, 2);
    TEST_FIELD(TEXTMETRICW, WCHAR, tmBreakChar, 50, 2, 2);
    TEST_FIELD(TEXTMETRICW, BYTE, tmItalic, 52, 1, 1);
    TEST_FIELD(TEXTMETRICW, BYTE, tmUnderlined, 53, 1, 1);
    TEST_FIELD(TEXTMETRICW, BYTE, tmStruckOut, 54, 1, 1);
    TEST_FIELD(TEXTMETRICW, BYTE, tmPitchAndFamily, 55, 1, 1);
    TEST_FIELD(TEXTMETRICW, BYTE, tmCharSet, 56, 1, 1);
}

static void test_pack_TRIVERTEX(void)
{
    /* TRIVERTEX (pack 4) */
    TEST_TYPE(TRIVERTEX, 16, 4);
    TEST_FIELD(TRIVERTEX, LONG, x, 0, 4, 4);
    TEST_FIELD(TRIVERTEX, LONG, y, 4, 4, 4);
    TEST_FIELD(TRIVERTEX, COLOR16, Red, 8, 2, 2);
    TEST_FIELD(TRIVERTEX, COLOR16, Green, 10, 2, 2);
    TEST_FIELD(TRIVERTEX, COLOR16, Blue, 12, 2, 2);
    TEST_FIELD(TRIVERTEX, COLOR16, Alpha, 14, 2, 2);
}

static void test_pack_TTPOLYCURVE(void)
{
    /* TTPOLYCURVE (pack 4) */
    TEST_TYPE(TTPOLYCURVE, 12, 2);
    TEST_FIELD(TTPOLYCURVE, WORD, wType, 0, 2, 2);
    TEST_FIELD(TTPOLYCURVE, WORD, cpfx, 2, 2, 2);
    TEST_FIELD(TTPOLYCURVE, POINTFX[1], apfx, 4, 8, 2);
}

static void test_pack_TTPOLYGONHEADER(void)
{
    /* TTPOLYGONHEADER (pack 4) */
    TEST_TYPE(TTPOLYGONHEADER, 16, 4);
    TEST_FIELD(TTPOLYGONHEADER, DWORD, cb, 0, 4, 4);
    TEST_FIELD(TTPOLYGONHEADER, DWORD, dwType, 4, 4, 4);
    TEST_FIELD(TTPOLYGONHEADER, POINTFX, pfxStart, 8, 8, 2);
}

static void test_pack_XFORM(void)
{
    /* XFORM (pack 4) */
    TEST_TYPE(XFORM, 24, 4);
    TEST_FIELD(XFORM, FLOAT, eM11, 0, 4, 4);
    TEST_FIELD(XFORM, FLOAT, eM12, 4, 4, 4);
    TEST_FIELD(XFORM, FLOAT, eM21, 8, 4, 4);
    TEST_FIELD(XFORM, FLOAT, eM22, 12, 4, 4);
    TEST_FIELD(XFORM, FLOAT, eDx, 16, 4, 4);
    TEST_FIELD(XFORM, FLOAT, eDy, 20, 4, 4);
}

static void test_pack(void)
{
    test_pack_ABC();
    test_pack_ABCFLOAT();
    test_pack_ABORTPROC();
    test_pack_BITMAP();
    test_pack_BITMAPCOREHEADER();
    test_pack_BITMAPCOREINFO();
    test_pack_BITMAPFILEHEADER();
    test_pack_BITMAPINFO();
    test_pack_BITMAPINFOHEADER();
    test_pack_BITMAPV4HEADER();
    test_pack_BITMAPV5HEADER();
    test_pack_BLENDFUNCTION();
    test_pack_CHARSETINFO();
    test_pack_CIEXYZ();
    test_pack_CIEXYZTRIPLE();
    test_pack_COLOR16();
    test_pack_COLORADJUSTMENT();
    test_pack_DEVMODEA();
    test_pack_DEVMODEW();
    test_pack_DIBSECTION();
    test_pack_DISPLAY_DEVICEA();
    test_pack_DISPLAY_DEVICEW();
    test_pack_DOCINFOA();
    test_pack_DOCINFOW();
    test_pack_EMR();
    test_pack_EMRABORTPATH();
    test_pack_EMRANGLEARC();
    test_pack_EMRARC();
    test_pack_EMRARCTO();
    test_pack_EMRBEGINPATH();
    test_pack_EMRBITBLT();
    test_pack_EMRCHORD();
    test_pack_EMRCLOSEFIGURE();
    test_pack_EMRCREATEBRUSHINDIRECT();
    test_pack_EMRCREATECOLORSPACE();
    test_pack_EMRCREATECOLORSPACEW();
    test_pack_EMRCREATEDIBPATTERNBRUSHPT();
    test_pack_EMRCREATEMONOBRUSH();
    test_pack_EMRCREATEPEN();
    test_pack_EMRDELETECOLORSPACE();
    test_pack_EMRDELETEOBJECT();
    test_pack_EMRELLIPSE();
    test_pack_EMRENDPATH();
    test_pack_EMREOF();
    test_pack_EMREXCLUDECLIPRECT();
    test_pack_EMREXTCREATEFONTINDIRECTW();
    test_pack_EMREXTCREATEPEN();
    test_pack_EMREXTFLOODFILL();
    test_pack_EMREXTSELECTCLIPRGN();
    test_pack_EMREXTTEXTOUTA();
    test_pack_EMREXTTEXTOUTW();
    test_pack_EMRFILLPATH();
    test_pack_EMRFILLRGN();
    test_pack_EMRFLATTENPATH();
    test_pack_EMRFORMAT();
    test_pack_EMRFRAMERGN();
    test_pack_EMRGDICOMMENT();
    test_pack_EMRGLSBOUNDEDRECORD();
    test_pack_EMRGLSRECORD();
    test_pack_EMRINTERSECTCLIPRECT();
    test_pack_EMRINVERTRGN();
    test_pack_EMRLINETO();
    test_pack_EMRMASKBLT();
    test_pack_EMRMODIFYWORLDTRANSFORM();
    test_pack_EMRMOVETOEX();
    test_pack_EMROFFSETCLIPRGN();
    test_pack_EMRPAINTRGN();
    test_pack_EMRPIE();
    test_pack_EMRPIXELFORMAT();
    test_pack_EMRPLGBLT();
    test_pack_EMRPOLYBEZIER();
    test_pack_EMRPOLYBEZIER16();
    test_pack_EMRPOLYBEZIERTO();
    test_pack_EMRPOLYBEZIERTO16();
    test_pack_EMRPOLYDRAW();
    test_pack_EMRPOLYDRAW16();
    test_pack_EMRPOLYGON();
    test_pack_EMRPOLYGON16();
    test_pack_EMRPOLYLINE();
    test_pack_EMRPOLYLINE16();
    test_pack_EMRPOLYLINETO();
    test_pack_EMRPOLYLINETO16();
    test_pack_EMRPOLYPOLYGON();
    test_pack_EMRPOLYPOLYGON16();
    test_pack_EMRPOLYPOLYLINE();
    test_pack_EMRPOLYPOLYLINE16();
    test_pack_EMRPOLYTEXTOUTA();
    test_pack_EMRPOLYTEXTOUTW();
    test_pack_EMRREALIZEPALETTE();
    test_pack_EMRRECTANGLE();
    test_pack_EMRRESIZEPALETTE();
    test_pack_EMRRESTOREDC();
    test_pack_EMRROUNDRECT();
    test_pack_EMRSAVEDC();
    test_pack_EMRSCALEVIEWPORTEXTEX();
    test_pack_EMRSCALEWINDOWEXTEX();
    test_pack_EMRSELECTCLIPPATH();
    test_pack_EMRSELECTCOLORSPACE();
    test_pack_EMRSELECTOBJECT();
    test_pack_EMRSELECTPALETTE();
    test_pack_EMRSETARCDIRECTION();
    test_pack_EMRSETBKCOLOR();
    test_pack_EMRSETBKMODE();
    test_pack_EMRSETBRUSHORGEX();
    test_pack_EMRSETCOLORADJUSTMENT();
    test_pack_EMRSETCOLORSPACE();
    test_pack_EMRSETDIBITSTODEVICE();
    test_pack_EMRSETICMMODE();
    test_pack_EMRSETLAYOUT();
    test_pack_EMRSETMAPMODE();
    test_pack_EMRSETMAPPERFLAGS();
    test_pack_EMRSETMETARGN();
    test_pack_EMRSETMITERLIMIT();
    test_pack_EMRSETPIXELV();
    test_pack_EMRSETPOLYFILLMODE();
    test_pack_EMRSETROP2();
    test_pack_EMRSETSTRETCHBLTMODE();
    test_pack_EMRSETTEXTALIGN();
    test_pack_EMRSETTEXTCOLOR();
    test_pack_EMRSETVIEWPORTEXTEX();
    test_pack_EMRSETVIEWPORTORGEX();
    test_pack_EMRSETWINDOWEXTEX();
    test_pack_EMRSETWINDOWORGEX();
    test_pack_EMRSETWORLDTRANSFORM();
    test_pack_EMRSTRETCHBLT();
    test_pack_EMRSTRETCHDIBITS();
    test_pack_EMRSTROKEANDFILLPATH();
    test_pack_EMRSTROKEPATH();
    test_pack_EMRTEXT();
    test_pack_EMRWIDENPATH();
    test_pack_ENHMETAHEADER();
    test_pack_ENHMETARECORD();
    test_pack_ENHMFENUMPROC();
    test_pack_ENUMLOGFONTA();
    test_pack_ENUMLOGFONTEXA();
    test_pack_ENUMLOGFONTEXW();
    test_pack_ENUMLOGFONTW();
    test_pack_EXTLOGFONTA();
    test_pack_EXTLOGFONTW();
    test_pack_EXTLOGPEN();
    test_pack_FIXED();
    test_pack_FONTENUMPROCA();
    test_pack_FONTENUMPROCW();
    test_pack_FONTSIGNATURE();
    test_pack_FXPT16DOT16();
    test_pack_FXPT2DOT30();
    test_pack_GCP_RESULTSA();
    test_pack_GCP_RESULTSW();
    test_pack_GLYPHMETRICS();
    test_pack_GLYPHMETRICSFLOAT();
    test_pack_GOBJENUMPROC();
    test_pack_GRADIENT_RECT();
    test_pack_GRADIENT_TRIANGLE();
    test_pack_HANDLETABLE();
    test_pack_ICMENUMPROCA();
    test_pack_ICMENUMPROCW();
    test_pack_KERNINGPAIR();
    test_pack_LAYERPLANEDESCRIPTOR();
    test_pack_LCSCSTYPE();
    test_pack_LCSGAMUTMATCH();
    test_pack_LINEDDAPROC();
    test_pack_LOCALESIGNATURE();
    test_pack_LOGBRUSH();
    test_pack_LOGCOLORSPACEA();
    test_pack_LOGCOLORSPACEW();
    test_pack_LOGFONTA();
    test_pack_LOGFONTW();
    test_pack_LOGPEN();
    test_pack_LPABC();
    test_pack_LPABCFLOAT();
    test_pack_LPBITMAP();
    test_pack_LPBITMAPCOREHEADER();
    test_pack_LPBITMAPCOREINFO();
    test_pack_LPBITMAPFILEHEADER();
    test_pack_LPBITMAPINFO();
    test_pack_LPBITMAPINFOHEADER();
    test_pack_LPBITMAPV5HEADER();
    test_pack_LPCHARSETINFO();
    test_pack_LPCIEXYZ();
    test_pack_LPCIEXYZTRIPLE();
    test_pack_LPCOLORADJUSTMENT();
    test_pack_LPDEVMODEA();
    test_pack_LPDEVMODEW();
    test_pack_LPDIBSECTION();
    test_pack_LPDISPLAY_DEVICEA();
    test_pack_LPDISPLAY_DEVICEW();
    test_pack_LPDOCINFOA();
    test_pack_LPDOCINFOW();
    test_pack_LPENHMETAHEADER();
    test_pack_LPENHMETARECORD();
    test_pack_LPENUMLOGFONTA();
    test_pack_LPENUMLOGFONTEXA();
    test_pack_LPENUMLOGFONTEXW();
    test_pack_LPENUMLOGFONTW();
    test_pack_LPEXTLOGFONTA();
    test_pack_LPEXTLOGFONTW();
    test_pack_LPEXTLOGPEN();
    test_pack_LPFONTSIGNATURE();
    test_pack_LPGCP_RESULTSA();
    test_pack_LPGCP_RESULTSW();
    test_pack_LPGLYPHMETRICS();
    test_pack_LPGLYPHMETRICSFLOAT();
    test_pack_LPGRADIENT_RECT();
    test_pack_LPGRADIENT_TRIANGLE();
    test_pack_LPHANDLETABLE();
    test_pack_LPKERNINGPAIR();
    test_pack_LPLAYERPLANEDESCRIPTOR();
    test_pack_LPLOCALESIGNATURE();
    test_pack_LPLOGBRUSH();
    test_pack_LPLOGCOLORSPACEA();
    test_pack_LPLOGCOLORSPACEW();
    test_pack_LPLOGFONTA();
    test_pack_LPLOGFONTW();
    test_pack_LPLOGPEN();
    test_pack_LPMAT2();
    test_pack_LPMETAFILEPICT();
    test_pack_LPMETAHEADER();
    test_pack_LPMETARECORD();
    test_pack_LPNEWTEXTMETRICA();
    test_pack_LPNEWTEXTMETRICW();
    test_pack_LPOUTLINETEXTMETRICA();
    test_pack_LPOUTLINETEXTMETRICW();
    test_pack_LPPANOSE();
    test_pack_LPPELARRAY();
    test_pack_LPPIXELFORMATDESCRIPTOR();
    test_pack_LPPOINTFX();
    test_pack_LPPOLYTEXTA();
    test_pack_LPPOLYTEXTW();
    test_pack_LPRASTERIZER_STATUS();
    test_pack_LPRGBQUAD();
    test_pack_LPRGNDATA();
    test_pack_LPTEXTMETRICA();
    test_pack_LPTEXTMETRICW();
    test_pack_LPTRIVERTEX();
    test_pack_LPTTPOLYCURVE();
    test_pack_LPTTPOLYGONHEADER();
    test_pack_LPXFORM();
    test_pack_MAT2();
    test_pack_METAFILEPICT();
    test_pack_METAHEADER();
    test_pack_METARECORD();
    test_pack_MFENUMPROC();
    test_pack_NEWTEXTMETRICA();
    test_pack_NEWTEXTMETRICEXA();
    test_pack_NEWTEXTMETRICEXW();
    test_pack_NEWTEXTMETRICW();
    test_pack_NPEXTLOGPEN();
    test_pack_OLDFONTENUMPROC();
    test_pack_OLDFONTENUMPROCA();
    test_pack_OLDFONTENUMPROCW();
    test_pack_OUTLINETEXTMETRICA();
    test_pack_OUTLINETEXTMETRICW();
    test_pack_PABC();
    test_pack_PABCFLOAT();
    test_pack_PANOSE();
    test_pack_PATTERN();
    test_pack_PBITMAP();
    test_pack_PBITMAPCOREHEADER();
    test_pack_PBITMAPCOREINFO();
    test_pack_PBITMAPFILEHEADER();
    test_pack_PBITMAPINFO();
    test_pack_PBITMAPINFOHEADER();
    test_pack_PBITMAPV4HEADER();
    test_pack_PBITMAPV5HEADER();
    test_pack_PBLENDFUNCTION();
    test_pack_PCHARSETINFO();
    test_pack_PCOLORADJUSTMENT();
    test_pack_PDEVMODEA();
    test_pack_PDEVMODEW();
    test_pack_PDIBSECTION();
    test_pack_PDISPLAY_DEVICEA();
    test_pack_PDISPLAY_DEVICEW();
    test_pack_PELARRAY();
    test_pack_PEMR();
    test_pack_PEMRABORTPATH();
    test_pack_PEMRANGLEARC();
    test_pack_PEMRARC();
    test_pack_PEMRARCTO();
    test_pack_PEMRBEGINPATH();
    test_pack_PEMRBITBLT();
    test_pack_PEMRCHORD();
    test_pack_PEMRCLOSEFIGURE();
    test_pack_PEMRCREATEBRUSHINDIRECT();
    test_pack_PEMRCREATECOLORSPACE();
    test_pack_PEMRCREATECOLORSPACEW();
    test_pack_PEMRCREATEDIBPATTERNBRUSHPT();
    test_pack_PEMRCREATEMONOBRUSH();
    test_pack_PEMRCREATEPALETTE();
    test_pack_PEMRCREATEPEN();
    test_pack_PEMRDELETECOLORSPACE();
    test_pack_PEMRDELETEOBJECT();
    test_pack_PEMRELLIPSE();
    test_pack_PEMRENDPATH();
    test_pack_PEMREOF();
    test_pack_PEMREXCLUDECLIPRECT();
    test_pack_PEMREXTCREATEFONTINDIRECTW();
    test_pack_PEMREXTCREATEPEN();
    test_pack_PEMREXTFLOODFILL();
    test_pack_PEMREXTSELECTCLIPRGN();
    test_pack_PEMREXTTEXTOUTA();
    test_pack_PEMREXTTEXTOUTW();
    test_pack_PEMRFILLPATH();
    test_pack_PEMRFILLRGN();
    test_pack_PEMRFLATTENPATH();
    test_pack_PEMRFORMAT();
    test_pack_PEMRFRAMERGN();
    test_pack_PEMRGDICOMMENT();
    test_pack_PEMRGLSBOUNDEDRECORD();
    test_pack_PEMRGLSRECORD();
    test_pack_PEMRINTERSECTCLIPRECT();
    test_pack_PEMRINVERTRGN();
    test_pack_PEMRLINETO();
    test_pack_PEMRMASKBLT();
    test_pack_PEMRMODIFYWORLDTRANSFORM();
    test_pack_PEMRMOVETOEX();
    test_pack_PEMROFFSETCLIPRGN();
    test_pack_PEMRPAINTRGN();
    test_pack_PEMRPIE();
    test_pack_PEMRPIXELFORMAT();
    test_pack_PEMRPLGBLT();
    test_pack_PEMRPOLYBEZIER();
    test_pack_PEMRPOLYBEZIER16();
    test_pack_PEMRPOLYBEZIERTO();
    test_pack_PEMRPOLYBEZIERTO16();
    test_pack_PEMRPOLYDRAW();
    test_pack_PEMRPOLYDRAW16();
    test_pack_PEMRPOLYGON();
    test_pack_PEMRPOLYGON16();
    test_pack_PEMRPOLYLINE();
    test_pack_PEMRPOLYLINE16();
    test_pack_PEMRPOLYLINETO();
    test_pack_PEMRPOLYLINETO16();
    test_pack_PEMRPOLYPOLYGON();
    test_pack_PEMRPOLYPOLYGON16();
    test_pack_PEMRPOLYPOLYLINE();
    test_pack_PEMRPOLYPOLYLINE16();
    test_pack_PEMRPOLYTEXTOUTA();
    test_pack_PEMRPOLYTEXTOUTW();
    test_pack_PEMRREALIZEPALETTE();
    test_pack_PEMRRECTANGLE();
    test_pack_PEMRRESIZEPALETTE();
    test_pack_PEMRRESTOREDC();
    test_pack_PEMRROUNDRECT();
    test_pack_PEMRSAVEDC();
    test_pack_PEMRSCALEVIEWPORTEXTEX();
    test_pack_PEMRSCALEWINDOWEXTEX();
    test_pack_PEMRSELECTCLIPPATH();
    test_pack_PEMRSELECTCOLORSPACE();
    test_pack_PEMRSELECTOBJECT();
    test_pack_PEMRSELECTPALETTE();
    test_pack_PEMRSETARCDIRECTION();
    test_pack_PEMRSETBKCOLOR();
    test_pack_PEMRSETBKMODE();
    test_pack_PEMRSETBRUSHORGEX();
    test_pack_PEMRSETCOLORADJUSTMENT();
    test_pack_PEMRSETCOLORSPACE();
    test_pack_PEMRSETDIBITSTODEVICE();
    test_pack_PEMRSETICMMODE();
    test_pack_PEMRSETLAYOUT();
    test_pack_PEMRSETMAPMODE();
    test_pack_PEMRSETMAPPERFLAGS();
    test_pack_PEMRSETMETARGN();
    test_pack_PEMRSETMITERLIMIT();
    test_pack_PEMRSETPALETTEENTRIES();
    test_pack_PEMRSETPIXELV();
    test_pack_PEMRSETPOLYFILLMODE();
    test_pack_PEMRSETROP2();
    test_pack_PEMRSETSTRETCHBLTMODE();
    test_pack_PEMRSETTEXTALIGN();
    test_pack_PEMRSETTEXTCOLOR();
    test_pack_PEMRSETVIEWPORTEXTEX();
    test_pack_PEMRSETVIEWPORTORGEX();
    test_pack_PEMRSETWINDOWEXTEX();
    test_pack_PEMRSETWINDOWORGEX();
    test_pack_PEMRSETWORLDTRANSFORM();
    test_pack_PEMRSTRETCHBLT();
    test_pack_PEMRSTRETCHDIBITS();
    test_pack_PEMRSTROKEANDFILLPATH();
    test_pack_PEMRSTROKEPATH();
    test_pack_PEMRTEXT();
    test_pack_PEMRWIDENPATH();
    test_pack_PENHMETAHEADER();
    test_pack_PEXTLOGFONTA();
    test_pack_PEXTLOGFONTW();
    test_pack_PEXTLOGPEN();
    test_pack_PFONTSIGNATURE();
    test_pack_PGLYPHMETRICSFLOAT();
    test_pack_PGRADIENT_RECT();
    test_pack_PGRADIENT_TRIANGLE();
    test_pack_PHANDLETABLE();
    test_pack_PIXELFORMATDESCRIPTOR();
    test_pack_PLAYERPLANEDESCRIPTOR();
    test_pack_PLOCALESIGNATURE();
    test_pack_PLOGBRUSH();
    test_pack_PLOGFONTA();
    test_pack_PLOGFONTW();
    test_pack_PMETAHEADER();
    test_pack_PMETARECORD();
    test_pack_PNEWTEXTMETRICA();
    test_pack_PNEWTEXTMETRICW();
    test_pack_POINTFLOAT();
    test_pack_POINTFX();
    test_pack_POLYTEXTA();
    test_pack_POLYTEXTW();
    test_pack_POUTLINETEXTMETRICA();
    test_pack_POUTLINETEXTMETRICW();
    test_pack_PPELARRAY();
    test_pack_PPIXELFORMATDESCRIPTOR();
    test_pack_PPOINTFLOAT();
    test_pack_PPOLYTEXTA();
    test_pack_PPOLYTEXTW();
    test_pack_PRGNDATA();
    test_pack_PRGNDATAHEADER();
    test_pack_PTEXTMETRICA();
    test_pack_PTEXTMETRICW();
    test_pack_PTRIVERTEX();
    test_pack_PXFORM();
    test_pack_RASTERIZER_STATUS();
    test_pack_RGBQUAD();
    test_pack_RGBTRIPLE();
    test_pack_RGNDATA();
    test_pack_RGNDATAHEADER();
    test_pack_TEXTMETRICA();
    test_pack_TEXTMETRICW();
    test_pack_TRIVERTEX();
    test_pack_TTPOLYCURVE();
    test_pack_TTPOLYGONHEADER();
    test_pack_XFORM();
}

START_TEST(generated)
{
    test_pack();
}
