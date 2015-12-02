/*
 * PROJECT:         Font Embedding Dll
 * FILE:            dll\win32\t2embed\stubs.c
 * PURPOSE:         Stubs file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <t2embapi.h>
#include <debug.h>

LONG
WINAPI
TTCharToUnicode(HDC hDC,
                UCHAR* pucCharCodes,
                ULONG ulCharCodeSize,
                USHORT* pusShortCodes,
                ULONG ulShortCodeSize,
                ULONG ulFlags)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTEmbedFontFromFileA(HDC hDC,
                     LPCSTR szFontFileName,
                     USHORT usTTCIndex,
                     ULONG ulFlags,
                     ULONG ulCharSet,
                     ULONG* pulPrivStatus,
                     ULONG* pulStatus,
                     WRITEEMBEDPROC lpfnWriteToStream,
                     LPVOID lpvWriteStream,
                     USHORT* pusCharCodeSet,
                     USHORT usCharCodeCount,
                     USHORT usLanguage,
                     TTEMBEDINFO* pTTEmbedInfo)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTEnableEmbeddingForFacename(LPSTR lpszFacename,
                             BOOL bEnable)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTGetEmbeddedFontInfo(ULONG ulFlags,
                      ULONG* pulPrivStatus,
                      ULONG ulPrivs,
                      ULONG* pulStatus,
                      READEMBEDPROC lpfnReadFromStream,
                      LPVOID lpvReadStream,
                      TTLOADINFO* pTTLoadInfo)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTIsEmbeddingEnabledForFacename(LPSTR lpszFacename,
                                BOOL* pbEnabled)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTRunValidationTests(HDC hDC,
                     TTVALIDATIONTESTPARAMS* pTestParam)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTEmbedFontEx(HDC hDC,
              ULONG ulFlags,
              ULONG ulCharSet,
              ULONG* pulPrivStatus,
              ULONG* pulStatus,
              WRITEEMBEDPROC lpfnWriteToStream,
              LPVOID lpvWriteStream,
              ULONG* pulCharCodeSet,
              USHORT usCharCodeCount,
              USHORT usLanguage,
              TTEMBEDINFO* pTTEmbedInfo)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTRunValidationTestsEx(HDC hDC,
                       TTVALIDATIONTESTPARAMSEX* pTestParam)
{
    UNIMPLEMENTED;
    return E_NONE;
}

LONG
WINAPI
TTGetNewFontName(HANDLE* phFontReference,
                 LPWSTR szWinFamilyName,
                 long cchMaxWinName,
                 LPSTR szMacFamilyName,
                 long cchMaxMacName)
{
    UNIMPLEMENTED;
    return E_NONE;
}
