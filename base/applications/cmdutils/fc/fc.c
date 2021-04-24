/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <conutils.h>
#include "resource.h"

// See also: https://stackoverflow.com/questions/33125766/compare-files-with-a-cmd
typedef enum FCRET // return code of FC command
{
    FCRET_INVALID = -1, FCRET_IDENTICAL, FCRET_DIFFERENT, FCRET_CANT_FIND
} FCRET;

#define MAX_VIEW_SIZE (64 * 1024 * 1024) // 64 MB

#define FLAG_A (1 << 0)
#define FLAG_B (1 << 1)
#define FLAG_C (1 << 2)
#define FLAG_L (1 << 3)
#define FLAG_LBn (1 << 4)
#define FLAG_N (1 << 5)
#define FLAG_OFFLINE (1 << 6)
#define FLAG_T (1 << 7)
#define FLAG_U (1 << 8)
#define FLAG_W (1 << 9)
#define FLAG_nnnn (1 << 10)
#define FLAG_HELP (1 << 11)

typedef struct FILECOMPARE
{
    DWORD dwFlags; // FLAG_...
    INT n, nnnn;
    LPCWSTR file1, file2;
} FILECOMPARE;

static FCRET NoDifference(VOID)
{
    ConResPuts(StdOut, IDS_NO_DIFFERENCE);
    return FCRET_IDENTICAL;
}

static FCRET Different(LPCWSTR file1, LPCWSTR file2)
{
    ConResPrintf(StdOut, IDS_DIFFERENT, file1, file2);
    return FCRET_DIFFERENT;
}

static FCRET LongerThan(LPCWSTR file1, LPCWSTR file2)
{
    ConResPrintf(StdOut, IDS_LONGER_THAN, file1, file2);
    return FCRET_DIFFERENT;
}

static FCRET OutOfMemory(VOID)
{
    ConResPuts(StdErr, IDS_OUT_OF_MEMORY);
    return FCRET_INVALID;
}

static FCRET CannotRead(LPCWSTR file)
{
    ConResPrintf(StdErr, IDS_CANNOT_READ, file);
    return FCRET_INVALID;
}

static FCRET InvalidSwitch(VOID)
{
    ConResPuts(StdErr, IDS_INVALID_SWITCH);
    return FCRET_INVALID;
}

static VOID CannotOpen(LPCWSTR file, DWORD dwError)
{
    LPVOID lpMsgBuf;
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageW(dwFlags, NULL, dwError, 0, (LPWSTR)&lpMsgBuf, 0, NULL);
    ConResPrintf(StdErr, IDS_CANNOT_OPEN, file, (LPWSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

static HANDLE DoOpenFileForInput(LPCWSTR file)
{
    HANDLE hFile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        CannotOpen(file, GetLastError());
    return hFile;
}

static FCRET BinaryFileCompare(const FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2, hMapping1 = NULL, hMapping2 = NULL;
    LPBYTE pb1 = NULL, pb2 = NULL;
    LARGE_INTEGER ib, cb1, cb2, cbCommon;
    DWORD cbView, ibView;
    BOOL fDifferent = FALSE;

    hFile1 = DoOpenFileForInput(pFC->file1);
    if (hFile1 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile2 = DoOpenFileForInput(pFC->file2);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file1, pFC->file2) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        if (!GetFileSizeEx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file2);
            break;
        }
        cbCommon.QuadPart = min(cb1.QuadPart, cb2.QuadPart);
        if (cbCommon.QuadPart > 0)
        {
            hMapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY,
                                           cb1.HighPart, cb1.LowPart, NULL);
            if (hMapping1 == NULL)
            {
                ret = CannotRead(pFC->file1);
                break;
            }
            hMapping2 = CreateFileMappingW(hFile2, NULL, PAGE_READONLY,
                                           cb2.HighPart, cb2.LowPart, NULL);
            if (hMapping2 == NULL)
            {
                ret = CannotRead(pFC->file2);
                break;
            }

            ret = FCRET_IDENTICAL;
            for (ib.QuadPart = 0; ib.QuadPart < cbCommon.QuadPart; )
            {
                cbView = (DWORD)min(cbCommon.QuadPart - ib.QuadPart, MAX_VIEW_SIZE);
                pb1 = MapViewOfFile(hMapping1, FILE_MAP_READ, ib.HighPart, ib.LowPart, cbView);
                pb2 = MapViewOfFile(hMapping2, FILE_MAP_READ, ib.HighPart, ib.LowPart, cbView);
                if (!pb1 || !pb2)
                {
                    ret = OutOfMemory();
                    break;
                }
                for (ibView = 0; ibView < cbView; ++ib.QuadPart, ++ibView)
                {
                    if (pb1[ibView] == pb2[ibView])
                        continue;

                    fDifferent = TRUE;
                    if (cbCommon.QuadPart > MAXDWORD)
                    {
                        ConPrintf(StdOut, L"%08lX%08lX: %02X %02X\n", ib.HighPart, ib.LowPart,
                                  pb1[ibView], pb2[ibView]);
                    }
                    else
                    {
                        ConPrintf(StdOut, L"%08lX: %02X %02X\n", ib.LowPart,
                                  pb1[ibView], pb2[ibView]);
                    }
                }
                UnmapViewOfFile(pb1);
                UnmapViewOfFile(pb2);
                pb1 = pb2 = NULL;
            }
            if (ret != FCRET_IDENTICAL)
                break;
        }

        if (cb1.QuadPart < cb2.QuadPart)
            ret = LongerThan(pFC->file2, pFC->file1);
        else if (cb1.QuadPart > cb2.QuadPart)
            ret = LongerThan(pFC->file1, pFC->file2);
        else if (fDifferent)
            ret = Different(pFC->file1, pFC->file2);
        else
            ret = NoDifference();
    } while (0);

    UnmapViewOfFile(pb1);
    UnmapViewOfFile(pb2);
    CloseHandle(hMapping1);
    CloseHandle(hMapping2);
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    return ret;
}

static FCRET
UnicodeTextCompare(const FILECOMPARE *pFC, HANDLE hMapping1, LARGE_INTEGER cb1,
                                           HANDLE hMapping2, LARGE_INTEGER cb2)
{
    FCRET ret;
    BOOL fIgnoreCase = !!(pFC->dwFlags & FLAG_C);
    DWORD dwCmpFlags = (fIgnoreCase ? NORM_IGNORECASE : 0);
    LPWSTR psz1, psz2;
    LARGE_INTEGER cch1, cch2;
    cch1.QuadPart = cb1.QuadPart / sizeof(WCHAR);
    cch2.QuadPart = cb2.QuadPart / sizeof(WCHAR);

    do
    {
        psz1 = MapViewOfFile(hMapping1, FILE_MAP_READ, 0, 0, cb1.LowPart);
        psz2 = MapViewOfFile(hMapping2, FILE_MAP_READ, 0, 0, cb1.LowPart);
        if (!psz1 || !psz2)
        {
            ret = OutOfMemory();
            break;
        }
        if (cch1.QuadPart < MAXLONG && cch2.QuadPart < MAXLONG)
        {
            if (CompareStringW(0, dwCmpFlags, psz1, (INT)cch1.LowPart, psz2,
                                                    (INT)cch2.LowPart) == CSTR_EQUAL)
            {
                ret = NoDifference();
                break;
            }
        }
        // TODO: compare each lines
        // TODO: large file support
        ret = Different(pFC->file1, pFC->file2);
    } while (0);

    UnmapViewOfFile(psz1);
    UnmapViewOfFile(psz2);
    return ret;
}

static FCRET
AnsiTextCompare(const FILECOMPARE *pFC, HANDLE hMapping1, LARGE_INTEGER cb1,
                                        HANDLE hMapping2, LARGE_INTEGER cb2)
{
    FCRET ret;
    BOOL fIgnoreCase = !!(pFC->dwFlags & FLAG_C);
    DWORD dwCmpFlags = (fIgnoreCase ? NORM_IGNORECASE : 0);
    LPSTR psz1, psz2;

    do
    {
        psz1 = MapViewOfFile(hMapping1, FILE_MAP_READ, 0, 0, cb1.LowPart);
        psz2 = MapViewOfFile(hMapping2, FILE_MAP_READ, 0, 0, cb2.LowPart);
        if (!psz1 || !psz2)
        {
            ret = OutOfMemory();
            break;
        }
        if (cb1.QuadPart < MAXLONG && cb2.QuadPart < MAXLONG)
        {
            if (CompareStringA(0, dwCmpFlags, psz1, (INT)cb1.LowPart,
                                              psz2, (INT)cb2.LowPart) == CSTR_EQUAL)
            {
                ret = NoDifference();
                break;
            }
        }
        // TODO: compare each lines
        // TODO: large file support
        ret = Different(pFC->file1, pFC->file2);
    } while (0);

    UnmapViewOfFile(psz1);
    UnmapViewOfFile(psz2);
    return ret;
}

static FCRET TextFileCompare(const FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2, hMapping1 = NULL, hMapping2 = NULL;
    LARGE_INTEGER cb1, cb2;
    BOOL fUnicode = !!(pFC->dwFlags & FLAG_U);

    hFile1 = DoOpenFileForInput(pFC->file1);
    if (hFile1 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile2 = DoOpenFileForInput(pFC->file2);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file1, pFC->file2) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        if (!GetFileSizeEx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file2);
            break;
        }
        if (cb1.QuadPart == 0 && cb2.QuadPart == 0)
        {
            ret = NoDifference();
            break;
        }
        hMapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY,
                                       cb1.HighPart, cb1.LowPart, NULL);
        if (hMapping1 == NULL)
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        hMapping2 = CreateFileMappingW(hFile2, NULL, PAGE_READONLY,
                                       cb2.HighPart, cb2.LowPart, NULL);
        if (hMapping2 == NULL)
        {
            ret = CannotRead(pFC->file2);
            break;
        }

        if (fUnicode)
            ret = UnicodeTextCompare(pFC, hMapping1, cb1, hMapping2, cb2);
        else
            ret = AnsiTextCompare(pFC, hMapping1, cb1, hMapping2, cb2);
    } while (0);

    CloseHandle(hMapping1);
    CloseHandle(hMapping2);
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    return ret;
}

static BOOL IsBinaryExt(LPCWSTR filename)
{
    static const LPCWSTR s_dotexts[] =
    {
        L".exe", L".com", L".sys", L".obj", L".lib", L".bin"
    };
    size_t iext;
    LPCWSTR pch, dotext, pch1 = wcsrchr(filename, L'\\'), pch2 = wcsrchr(filename, L'/');
    if (!pch1 && !pch2)
        pch = filename;
    else if (!pch1 && pch2)
        pch = pch2;
    else if (pch1 && !pch2)
        pch = pch1;
    else if (pch1 < pch2)
        pch = pch2;
    else
        pch = pch1;

    dotext = wcsrchr(pch, L'.');
    if (dotext)
    {
        for (iext = 0; iext < _countof(s_dotexts); ++iext)
        {
            if (_wcsicmp(dotext, s_dotexts[iext]) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

#define HasWildcard(filename) \
    ((wcschr((filename), L'*') != NULL) || (wcschr((filename), L'?') != NULL))

static FCRET FileCompare(const FILECOMPARE *pFC, LPCWSTR file1, LPCWSTR file2)
{
    ConResPrintf(StdOut, IDS_COMPARING, file1, file2);
    if ((pFC->dwFlags & FLAG_B) || IsBinaryExt(file1) || IsBinaryExt(file2))
        return BinaryFileCompare(pFC);
    return TextFileCompare(pFC);
}

static FCRET WildcardFileCompare(const FILECOMPARE *pFC)
{
    if (pFC->dwFlags & FLAG_HELP)
    {
        ConResPuts(StdOut, IDS_USAGE);
        return FCRET_INVALID;
    }

    if (!pFC->file1 || !pFC->file2)
    {
        ConResPuts(StdErr, IDS_NEEDS_FILES);
        return FCRET_INVALID;
    }

    if (HasWildcard(pFC->file1) || HasWildcard(pFC->file2))
    {
        // TODO: wildcard
        ConResPuts(StdErr, IDS_CANT_USE_WILDCARD);
    }

    return FileCompare(pFC, pFC->file1, pFC->file2);
}

int wmain(int argc, WCHAR **argv)
{
    FILECOMPARE fc = { .dwFlags = FLAG_L, .n = 100, .nnnn = 2 };
    INT i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != L'/')
        {
            if (!fc.file1)
                fc.file1 = argv[i];
            else if (!fc.file2)
                fc.file2 = argv[i];
            else
                return InvalidSwitch();
            continue;
        }
        switch (towupper(argv[i][1]))
        {
            case L'A':
                fc.dwFlags |= FLAG_A;
                break;
            case L'B':
                fc.dwFlags |= FLAG_B;
                break;
            case L'C':
                fc.dwFlags |= FLAG_C;
                break;
            case L'L':
                if (_wcsicmp(argv[i], L"/L") == 0)
                {
                    fc.dwFlags |= FLAG_L;
                }
                else if (towupper(argv[i][2]) == L'B')
                {
                    fc.dwFlags |= FLAG_LBn;
                    fc.n = wcstoul(&argv[i][3], NULL, 10);
                }
                break;
            case L'N':
                fc.dwFlags |= FLAG_N;
                break;
            case L'O':
                if (_wcsicmp(argv[i], L"/OFF") == 0 || _wcsicmp(argv[i], L"/OFFLINE") == 0)
                {
                    fc.dwFlags |= FLAG_OFFLINE;
                }
                break;
            case L'T':
                fc.dwFlags |= FLAG_T;
                break;
            case L'W':
                fc.dwFlags |= FLAG_W;
                break;
            case L'0': case L'1': case L'2': case L'3': case L'4':
            case L'5': case L'6': case L'7': case L'8': case L'9':
                fc.nnnn = wcstoul(&argv[i][1], NULL, 10);
                fc.dwFlags |= FLAG_nnnn;
                break;
            case L'?':
                fc.dwFlags |= FLAG_HELP;
                break;
            default:
                return InvalidSwitch();
        }
    }
    return WildcardFileCompare(&fc);
}
