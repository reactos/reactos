/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <stdlib.h>
#include <string.h>
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

#define LARGE_FILE_SIZE MAXLONG

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

static FCRET TooLarge(LPCWSTR file)
{
    ConResPrintf(StdErr, IDS_TOO_LARGE, file);
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

static BOOL GetFileSizeDx(HANDLE hFile, size_t *pcbFile)
{
    LARGE_INTEGER li;
    if (!GetFileSizeEx(hFile, &li))
        return FALSE;
#ifndef _WIN64
    if (li.QuadPart >= LARGE_FILE_SIZE)
        return FALSE;
#endif
    *pcbFile = (size_t)li.QuadPart;
    return TRUE;
}

static BOOL
ReadFileDx(HANDLE hFile, LPVOID lpBuffer, size_t cb, size_t *pcbDidRead)
{
#ifdef _WIN64
    BOOL ret = TRUE;
    DWORD dwDidRead, dwForRead;
    size_t ib;
    LPBYTE pb = (LPBYTE)lpBuffer;
    for (ib = 0; ib < cb; ib += dwDidRead)
    {
        dwForRead = (DWORD)min(cb - ib, LARGE_FILE_SIZE);
        ret = ReadFile(hFile, &pb[ib], dwForRead, &dwDidRead, NULL);
        if (!ret)
        {
            ib += dwDidRead;
            break;
        }
    }
    *pcbDidRead = ib;
    return ret;
#else
    return ReadFile(hFile, lpBuffer, cb, (LPDWORD)pcbDidRead, NULL);
#endif
}

static FCRET BinaryFileCompare(const FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2;
    size_t ib, cb1, cb2, cbCommon, cbRead1, cbRead2;
    LPBYTE pb1 = NULL, pb2 = NULL;
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
        if (lstrcmpiW(pFC->file1, pFC->file2) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeDx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        if (!GetFileSizeDx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file2);
            break;
        }
        cbCommon = min(cb1, cb2);
        if (cbCommon > 0)
        {
            pb1 = malloc(cbCommon);
            pb2 = malloc(cbCommon);
            if (!pb1 || !pb2)
            {
                free(pb1);
                free(pb2);
                pb1 = pb2 = NULL;
                ret = OutOfMemory();
                break;
            }
            if (!ReadFileDx(hFile1, pb1, cbCommon, &cbRead1) || cbRead1 != cbCommon)
            {
                ret = CannotRead(pFC->file1);
                break;
            }
            if (!ReadFileDx(hFile2, pb2, cbCommon, &cbRead2) || cbRead2 != cbCommon)
            {
                ret = CannotRead(pFC->file2);
                break;
            }

            for (ib = 0; ib < cbCommon; ++ib)
            {
                if (pb1[ib] == pb2[ib])
                    continue;
#ifdef _WIN64
                if (cbCommon >= LARGE_FILE_SIZE)
                {
                    ConPrintf(StdOut, L"%08lX%08lX: %02X %02X\n",
                              (DWORD)(ib >> 32), (DWORD)ib, pb1[ib], pb2[ib]);
                }
                else
#endif
                {
                    ConPrintf(StdOut, L"%08lX: %02X %02X\n", (DWORD)ib, pb1[ib], pb2[ib]);
                }
                fDifferent = TRUE;
            }
        }

        if (cb1 < cb2)
            ret = LongerThan(pFC->file2, pFC->file1);
        else if (cb1 > cb2)
            ret = LongerThan(pFC->file1, pFC->file2);
        else if (fDifferent)
            ret = Different(pFC->file1, pFC->file2);
        else
            ret = NoDifference();
    } while (0);

    CloseHandle(hFile1);
    CloseHandle(hFile2);
    free(pb1);
    free(pb2);
    return ret;
}

static FCRET
UnicodeTextCompare(const FILECOMPARE *pFC, LPWSTR psz1, size_t cch1, LPWSTR psz2, size_t cch2)
{
    BOOL fIgnoreCase = !!(pFC->dwFlags & FLAG_C);
    DWORD dwCmpFlags = (fIgnoreCase ? NORM_IGNORECASE : 0);
    if (cch1 >= LARGE_FILE_SIZE)
        return TooLarge(pFC->file1);
    if (cch2 >= LARGE_FILE_SIZE)
        return TooLarge(pFC->file2);
    if (CompareStringW(0, dwCmpFlags, psz1, (INT)cch1, psz2, (INT)cch2) == CSTR_EQUAL)
        return NoDifference();
    // TODO: compare each lines
    return Different(pFC->file1, pFC->file2);
}

static FCRET
AnsiTextCompare(const FILECOMPARE *pFC, LPSTR psz1, size_t cch1, LPSTR psz2, size_t cch2)
{
    BOOL fIgnoreCase = !!(pFC->dwFlags & FLAG_C);
    DWORD dwCmpFlags = (fIgnoreCase ? NORM_IGNORECASE : 0);
    if (cch1 >= LARGE_FILE_SIZE)
        return TooLarge(pFC->file1);
    if (cch2 >= LARGE_FILE_SIZE)
        return TooLarge(pFC->file2);
    if (CompareStringA(0, dwCmpFlags, psz1, (INT)cch1, psz2, (INT)cch2) == CSTR_EQUAL)
        return NoDifference();
    // TODO: compare each lines
    return Different(pFC->file1, pFC->file2);
}

static FCRET TextFileCompare(const FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2;
    LPBYTE pb1 = NULL, pb2 = NULL;
    size_t cb1, cb2, cbRead;
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
        if (lstrcmpiW(pFC->file1, pFC->file2) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeDx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        if (!GetFileSizeDx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file2);
            break;
        }
        if (cb1 == 0 && cb2 == 0)
        {
            ret = NoDifference();
            break;
        }

        pb1 = malloc(fUnicode ? (cb1 + 3) : (cb1 + 1));
        pb2 = malloc(fUnicode ? (cb2 + 3) : (cb2 + 1));
        if (pb1 == NULL || pb2 == NULL)
        {
            ret = OutOfMemory();
            break;
        }
        if (!ReadFileDx(hFile1, pb1, cb1, &cbRead) || cb1 != cbRead)
        {
            ret = CannotRead(pFC->file1);
            break;
        }
        if (!ReadFileDx(hFile2, pb2, cb2, &cbRead) || cb2 != cbRead)
        {
            ret = CannotRead(pFC->file2);
            break;
        }

        if (fUnicode)
        {
            pb1[cb1] = pb1[cb1 + 1] = pb1[cb1 + 2] = 0;
            pb2[cb2] = pb2[cb2 + 1] = pb2[cb2 + 2] = 0;
            ret = UnicodeTextCompare(pFC, (LPWSTR)pb1, (cb1 + 1) / sizeof(WCHAR),
                                          (LPWSTR)pb2, (cb2 + 1) / sizeof(WCHAR));
        }
        else
        {
            pb1[cb1] = pb2[cb2] = 0;
            ret = AnsiTextCompare(pFC, (LPSTR)pb1, cb1, (LPSTR)pb2, cb2);
        }
    } while (0);

    CloseHandle(hFile1);
    CloseHandle(hFile2);
    free(pb1);
    free(pb2);
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
            if (lstrcmpiW(dotext, s_dotexts[iext]) == 0)
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
        // TODO:
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
        switch (argv[i][1])
        {
            case L'A': case L'a':
                fc.dwFlags |= FLAG_A;
                break;
            case L'B': case L'b':
                fc.dwFlags |= FLAG_B;
                break;
            case L'C': case L'c':
                fc.dwFlags |= FLAG_C;
                break;
            case L'L': case L'l':
                if (lstrcmpiW(argv[i], L"/L") == 0)
                {
                    fc.dwFlags |= FLAG_L;
                }
                else if (argv[i][2] == L'B' || argv[i][2] == L'b')
                {
                    fc.dwFlags |= FLAG_LBn;
                    fc.n = wcstoul(&argv[i][3], NULL, 10);
                }
                break;
            case L'N': case L'n':
                fc.dwFlags |= FLAG_N;
                break;
            case L'O': case L'o':
                if (lstrcmpiW(argv[i], L"/OFF") == 0 || lstrcmpiW(argv[i], L"/OFFLINE") == 0)
                {
                    fc.dwFlags |= FLAG_OFFLINE;
                }
                break;
            case L'T': case L't':
                fc.dwFlags |= FLAG_T;
                break;
            case L'W': case L'w':
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
