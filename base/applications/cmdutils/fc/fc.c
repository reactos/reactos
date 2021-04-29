/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "fc.h"
#include <conutils.h>

FCRET NoDifference(VOID)
{
    ConResPuts(StdOut, IDS_NO_DIFFERENCE);
    return FCRET_IDENTICAL;
}

FCRET Different(LPCWSTR file1, LPCWSTR file2)
{
    ConResPrintf(StdOut, IDS_DIFFERENT, file1, file2);
    return FCRET_DIFFERENT;
}

FCRET LongerThan(LPCWSTR file1, LPCWSTR file2)
{
    ConResPrintf(StdOut, IDS_LONGER_THAN, file1, file2);
    return FCRET_DIFFERENT;
}

FCRET OutOfMemory(VOID)
{
    ConResPuts(StdErr, IDS_OUT_OF_MEMORY);
    return FCRET_INVALID;
}

FCRET CannotRead(LPCWSTR file)
{
    ConResPrintf(StdErr, IDS_CANNOT_READ, file);
    return FCRET_INVALID;
}

FCRET InvalidSwitch(VOID)
{
    ConResPuts(StdErr, IDS_INVALID_SWITCH);
    return FCRET_INVALID;
}

FCRET ResynchFailed(VOID)
{
    ConResPuts(StdOut, IDS_RESYNC_FAILED);
    return FCRET_DIFFERENT;
}

VOID ShowCaption(LPCWSTR file)
{
    ConPrintf(StdOut, L"***** %ls\n", file);
}

VOID PrintLine2W(const FILECOMPARE *pFC, DWORD lineno, LPCWSTR psz)
{
    if (pFC->dwFlags & FLAG_N)
        ConPrintf(StdOut, L"%5d%ls\n", lineno, psz);
    else
        ConPrintf(StdOut, L"%ls\n", psz);
}
VOID PrintLine2A(const FILECOMPARE *pFC, DWORD lineno, LPCSTR psz)
{
    if (pFC->dwFlags & FLAG_N)
        ConPrintf(StdOut, L"%5d%hs\n", lineno, psz);
    else
        ConPrintf(StdOut, L"%hs\n", psz);
}

VOID PrintLineW(const FILECOMPARE *pFC, const NODE_W *node)
{
    PrintLine2W(pFC, node->lineno, node->pszLine);
}
VOID PrintLineA(const FILECOMPARE *pFC, const NODE_A *node)
{
    PrintLine2A(pFC, node->lineno, node->pszLine);
}

VOID PrintDots(VOID)
{
    ConPrintf(StdOut, L"...\n");
}

HANDLE DoOpenFileForInput(LPCWSTR file)
{
    HANDLE hFile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ConResPrintf(StdErr, IDS_CANNOT_OPEN, file);
    }
    return hFile;
}

static FCRET BinaryFileCompare(FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2, hMapping1 = NULL, hMapping2 = NULL;
    LPBYTE pb1 = NULL, pb2 = NULL;
    LARGE_INTEGER ib, cb1, cb2, cbCommon;
    DWORD cbView, ibView;
    BOOL fDifferent = FALSE;

    hFile1 = DoOpenFileForInput(pFC->file[0]);
    if (hFile1 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile2 = DoOpenFileForInput(pFC->file[1]);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file[0], pFC->file[1]) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file[0]);
            break;
        }
        if (!GetFileSizeEx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file[1]);
            break;
        }
        cbCommon.QuadPart = min(cb1.QuadPart, cb2.QuadPart);
        if (cbCommon.QuadPart > 0)
        {
            hMapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY,
                                           cb1.HighPart, cb1.LowPart, NULL);
            if (hMapping1 == NULL)
            {
                ret = CannotRead(pFC->file[0]);
                break;
            }
            hMapping2 = CreateFileMappingW(hFile2, NULL, PAGE_READONLY,
                                           cb2.HighPart, cb2.LowPart, NULL);
            if (hMapping2 == NULL)
            {
                ret = CannotRead(pFC->file[1]);
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
                        ConPrintf(StdOut, L"%016I64X: %02X %02X\n", ib.QuadPart,
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
            ret = LongerThan(pFC->file[1], pFC->file[0]);
        else if (cb1.QuadPart > cb2.QuadPart)
            ret = LongerThan(pFC->file[0], pFC->file[1]);
        else if (fDifferent)
            ret = Different(pFC->file[0], pFC->file[1]);
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

static FCRET TextFileCompare(FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile1, hFile2, hMapping1 = NULL, hMapping2 = NULL;
    LARGE_INTEGER cb1, cb2;
    BOOL fUnicode = !!(pFC->dwFlags & FLAG_U);

    hFile1 = DoOpenFileForInput(pFC->file[0]);
    if (hFile1 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile2 = DoOpenFileForInput(pFC->file[1]);
    if (hFile2 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile1);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file[0], pFC->file[1]) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file[0]);
            break;
        }
        if (!GetFileSizeEx(hFile2, &cb2))
        {
            ret = CannotRead(pFC->file[1]);
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
            ret = CannotRead(pFC->file[0]);
            break;
        }
        hMapping2 = CreateFileMappingW(hFile2, NULL, PAGE_READONLY,
                                       cb2.HighPart, cb2.LowPart, NULL);
        if (hMapping2 == NULL)
        {
            ret = CannotRead(pFC->file[1]);
            break;
        }

        if (fUnicode)
        {
            ret = TextCompareW(pFC, &hMapping1, &cb1, &hMapping2, &cb2);
            free(pFC->last_matchW[0]);
            free(pFC->last_matchW[1]);
        }
        else
        {
            ret = TextCompareA(pFC, &hMapping1, &cb1, &hMapping2, &cb2);
            free(pFC->last_matchA[0]);
            free(pFC->last_matchA[1]);
        }
    } while (0);

    CloseHandle(hMapping1);
    CloseHandle(hMapping2);
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    return ret;
}

static BOOL IsBinaryExt(LPCWSTR filename)
{
    // Don't change this array. This is by design.
    // See also: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/fc
    static const LPCWSTR s_exts[] = { L"EXE", L"COM", L"SYS", L"OBJ", L"LIB", L"BIN" };
    size_t iext;
    LPCWSTR pch, ext, pch1 = wcsrchr(filename, L'\\'), pch2 = wcsrchr(filename, L'/');
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

    ext = wcsrchr(pch, L'.');
    if (ext)
    {
        ++ext;
        for (iext = 0; iext < _countof(s_exts); ++iext)
        {
            if (_wcsicmp(ext, s_exts[iext]) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

#define HasWildcard(filename) \
    ((wcschr((filename), L'*') != NULL) || (wcschr((filename), L'?') != NULL))

static FCRET FileCompare(FILECOMPARE *pFC)
{
    ConResPrintf(StdOut, IDS_COMPARING, pFC->file[0], pFC->file[1]);

    if (!(pFC->dwFlags & FLAG_L) &&
        ((pFC->dwFlags & FLAG_B) || IsBinaryExt(pFC->file[0]) || IsBinaryExt(pFC->file[1])))
    {
        return BinaryFileCompare(pFC);
    }
    return TextFileCompare(pFC);
}

static FCRET WildcardFileCompare(FILECOMPARE *pFC)
{
    if (pFC->dwFlags & FLAG_HELP)
    {
        ConResPuts(StdOut, IDS_USAGE);
        return FCRET_INVALID;
    }

    if (!pFC->file[0] || !pFC->file[1])
    {
        ConResPuts(StdErr, IDS_NEEDS_FILES);
        return FCRET_INVALID;
    }

    if (HasWildcard(pFC->file[0]) || HasWildcard(pFC->file[1]))
    {
        // TODO: wildcard
        ConResPuts(StdErr, IDS_CANT_USE_WILDCARD);
    }

    return FileCompare(pFC);
}

int wmain(int argc, WCHAR **argv)
{
    FILECOMPARE fc = { .dwFlags = 0, .n = 100, .nnnn = 2 };
    PWCHAR endptr;
    INT i;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != L'/')
        {
            if (!fc.file[0])
                fc.file[0] = argv[i];
            else if (!fc.file[1])
                fc.file[1] = argv[i];
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
                    if (iswdigit(argv[i][3]))
                    {
                        fc.dwFlags |= FLAG_LBn;
                        fc.n = wcstoul(&argv[i][3], &endptr, 10);
                        if (endptr == NULL || *endptr != 0)
                            return InvalidSwitch();
                    }
                    else
                    {
                        return InvalidSwitch();
                    }
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
                fc.nnnn = wcstoul(&argv[i][1], &endptr, 10);
                if (endptr == NULL || *endptr != 0)
                    return InvalidSwitch();
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
