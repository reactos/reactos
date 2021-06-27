/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "fc.h"

#ifdef __REACTOS__
    #include <conutils.h>
#else
    #include <stdio.h>
    #define ConInitStdStreams() /* empty */
    #define StdOut stdout
    #define StdErr stderr
    void ConPuts(FILE *fp, LPCWSTR psz)
    {
        fputws(psz, fp);
    }
    void ConPrintf(FILE *fp, LPCWSTR psz, ...)
    {
        va_list va;
        va_start(va, psz);
        vfwprintf(fp, psz, va);
        va_end(va);
    }
    void ConResPuts(FILE *fp, UINT nID)
    {
        WCHAR sz[MAX_PATH];
        LoadStringW(NULL, nID, sz, _countof(sz));
        fputws(sz, fp);
    }
    void ConResPrintf(FILE *fp, UINT nID, ...)
    {
        va_list va;
        WCHAR sz[MAX_PATH];
        va_start(va, nID);
        LoadStringW(NULL, nID, sz, _countof(sz));
        vfwprintf(fp, sz, va);
        va_end(va);
    }
#endif
#include <strsafe.h>
#include <shlwapi.h>

FCRET NoDifference(VOID)
{
    ConResPuts(StdOut, IDS_NO_DIFFERENCE);
    return FCRET_IDENTICAL;
}

FCRET Different(LPCWSTR file0, LPCWSTR file1)
{
    ConResPrintf(StdOut, IDS_DIFFERENT, file0, file1);
    return FCRET_DIFFERENT;
}

FCRET LongerThan(LPCWSTR file0, LPCWSTR file1)
{
    ConResPrintf(StdOut, IDS_LONGER_THAN, file0, file1);
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

FCRET ResyncFailed(VOID)
{
    ConResPuts(StdOut, IDS_RESYNC_FAILED);
    return FCRET_DIFFERENT;
}

VOID PrintCaption(LPCWSTR file)
{
    ConPrintf(StdOut, L"***** %ls\n", file);
}

VOID PrintEndOfDiff(VOID)
{
    ConPuts(StdOut, L"*****\n\n");
}

VOID PrintDots(VOID)
{
    ConPuts(StdOut, L"...\n");
}

VOID PrintLineW(const FILECOMPARE *pFC, DWORD lineno, LPCWSTR psz)
{
    if (pFC->dwFlags & FLAG_N)
        ConPrintf(StdOut, L"%5d:  %ls\n", lineno, psz);
    else
        ConPrintf(StdOut, L"%ls\n", psz);
}
VOID PrintLineA(const FILECOMPARE *pFC, DWORD lineno, LPCSTR psz)
{
    if (pFC->dwFlags & FLAG_N)
        ConPrintf(StdOut, L"%5d:  %hs\n", lineno, psz);
    else
        ConPrintf(StdOut, L"%hs\n", psz);
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
    HANDLE hFile0, hFile1, hMapping0 = NULL, hMapping1 = NULL;
    LPBYTE pb0 = NULL, pb1 = NULL;
    LARGE_INTEGER ib, cb0, cb1, cbCommon;
    DWORD cbView, ibView;
    BOOL fDifferent = FALSE;

    hFile0 = DoOpenFileForInput(pFC->file[0]);
    if (hFile0 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile1 = DoOpenFileForInput(pFC->file[1]);
    if (hFile1 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile0);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file[0], pFC->file[1]) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile0, &cb0))
        {
            ret = CannotRead(pFC->file[0]);
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file[1]);
            break;
        }
        cbCommon.QuadPart = min(cb0.QuadPart, cb1.QuadPart);
        if (cbCommon.QuadPart > 0)
        {
            hMapping0 = CreateFileMappingW(hFile0, NULL, PAGE_READONLY,
                                           cb0.HighPart, cb0.LowPart, NULL);
            if (hMapping0 == NULL)
            {
                ret = CannotRead(pFC->file[0]);
                break;
            }
            hMapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY,
                                           cb1.HighPart, cb1.LowPart, NULL);
            if (hMapping1 == NULL)
            {
                ret = CannotRead(pFC->file[1]);
                break;
            }

            ret = FCRET_IDENTICAL;
            for (ib.QuadPart = 0; ib.QuadPart < cbCommon.QuadPart; )
            {
                cbView = (DWORD)min(cbCommon.QuadPart - ib.QuadPart, MAX_VIEW_SIZE);
                pb0 = MapViewOfFile(hMapping0, FILE_MAP_READ, ib.HighPart, ib.LowPart, cbView);
                pb1 = MapViewOfFile(hMapping1, FILE_MAP_READ, ib.HighPart, ib.LowPart, cbView);
                if (!pb0 || !pb1)
                {
                    ret = OutOfMemory();
                    break;
                }
                for (ibView = 0; ibView < cbView; ++ib.QuadPart, ++ibView)
                {
                    if (pb0[ibView] == pb1[ibView])
                        continue;

                    fDifferent = TRUE;
                    if (cbCommon.QuadPart > MAXDWORD)
                    {
                        ConPrintf(StdOut, L"%016I64X: %02X %02X\n", ib.QuadPart,
                                  pb0[ibView], pb1[ibView]);
                    }
                    else
                    {
                        ConPrintf(StdOut, L"%08lX: %02X %02X\n", ib.LowPart,
                                  pb0[ibView], pb1[ibView]);
                    }
                }
                UnmapViewOfFile(pb0);
                UnmapViewOfFile(pb1);
                pb0 = pb1 = NULL;
            }
            if (ret != FCRET_IDENTICAL)
                break;
        }

        if (cb0.QuadPart < cb1.QuadPart)
            ret = LongerThan(pFC->file[1], pFC->file[0]);
        else if (cb0.QuadPart > cb1.QuadPart)
            ret = LongerThan(pFC->file[0], pFC->file[1]);
        else if (fDifferent)
            ret = Different(pFC->file[0], pFC->file[1]);
        else
            ret = NoDifference();
    } while (0);

    UnmapViewOfFile(pb0);
    UnmapViewOfFile(pb1);
    CloseHandle(hMapping0);
    CloseHandle(hMapping1);
    CloseHandle(hFile0);
    CloseHandle(hFile1);
    return ret;
}

static FCRET TextFileCompare(FILECOMPARE *pFC)
{
    FCRET ret;
    HANDLE hFile0, hFile1, hMapping0 = NULL, hMapping1 = NULL;
    LARGE_INTEGER cb0, cb1;
    BOOL fUnicode = !!(pFC->dwFlags & FLAG_U);

    hFile0 = DoOpenFileForInput(pFC->file[0]);
    if (hFile0 == INVALID_HANDLE_VALUE)
        return FCRET_CANT_FIND;
    hFile1 = DoOpenFileForInput(pFC->file[1]);
    if (hFile1 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile0);
        return FCRET_CANT_FIND;
    }

    do
    {
        if (_wcsicmp(pFC->file[0], pFC->file[1]) == 0)
        {
            ret = NoDifference();
            break;
        }
        if (!GetFileSizeEx(hFile0, &cb0))
        {
            ret = CannotRead(pFC->file[0]);
            break;
        }
        if (!GetFileSizeEx(hFile1, &cb1))
        {
            ret = CannotRead(pFC->file[1]);
            break;
        }
        if (cb0.QuadPart == 0 && cb1.QuadPart == 0)
        {
            ret = NoDifference();
            break;
        }
        if (cb0.QuadPart > 0)
        {
            hMapping0 = CreateFileMappingW(hFile0, NULL, PAGE_READONLY,
                                           cb0.HighPart, cb0.LowPart, NULL);
            if (hMapping0 == NULL)
            {
                ret = CannotRead(pFC->file[0]);
                break;
            }
        }
        if (cb1.QuadPart > 0)
        {
            hMapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY,
                                           cb1.HighPart, cb1.LowPart, NULL);
            if (hMapping1 == NULL)
            {
                ret = CannotRead(pFC->file[1]);
                break;
            }
        }
        if (fUnicode)
            ret = TextCompareW(pFC, &hMapping0, &cb0, &hMapping1, &cb1);
        else
            ret = TextCompareA(pFC, &hMapping0, &cb0, &hMapping1, &cb1);
    } while (0);

    CloseHandle(hMapping0);
    CloseHandle(hMapping1);
    CloseHandle(hFile0);
    CloseHandle(hFile1);
    return ret;
}

static BOOL IsBinaryExt(LPCWSTR filename)
{
    // Don't change this array. This is by design.
    // See also: https://docs.microsoft.com/en-us/windows-server/administration/windows-commands/fc
    static const LPCWSTR s_exts[] = { L"EXE", L"COM", L"SYS", L"OBJ", L"LIB", L"BIN" };
    size_t iext;
    LPCWSTR pch, ext, pch0 = wcsrchr(filename, L'\\'), pch1 = wcsrchr(filename, L'/');
    if (!pch0 && !pch1)
        pch = filename;
    else if (!pch0 && pch1)
        pch = pch1;
    else if (pch0 && !pch1)
        pch = pch0;
    else if (pch0 < pch1)
        pch = pch1;
    else
        pch = pch0;

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

static FCRET FileCompare(FILECOMPARE *pFC)
{
    FCRET ret;
    ConResPrintf(StdOut, IDS_COMPARING, pFC->file[0], pFC->file[1]);

    if (!(pFC->dwFlags & FLAG_L) &&
        ((pFC->dwFlags & FLAG_B) || IsBinaryExt(pFC->file[0]) || IsBinaryExt(pFC->file[1])))
    {
        ret = BinaryFileCompare(pFC);
    }
    else
    {
        ret = TextFileCompare(pFC);
    }

    ConPuts(StdOut, L"\n");
    return ret;
}

/* Is it L"." or L".."? */
#define IS_DOTS(pch) \
    ((*(pch) == L'.') && (((pch)[1] == 0) || (((pch)[1] == L'.') && ((pch)[2] == 0))))
#define HasWildcard(filename) \
    ((wcschr((filename), L'*') != NULL) || (wcschr((filename), L'?') != NULL))

static inline BOOL IsTitleWild(LPCWSTR filename)
{
    LPCWSTR pch = PathFindFileNameW(filename);
    return (pch && *pch == L'*' && pch[1] == L'.' && !HasWildcard(&pch[2]));
}

static FCRET FileCompareOneSideWild(const FILECOMPARE *pFC, BOOL bWildRight)
{
    FCRET ret = FCRET_IDENTICAL;
    WIN32_FIND_DATAW find;
    HANDLE hFind;
    WCHAR szPath[MAX_PATH];
    FILECOMPARE fc;

    hFind = FindFirstFileW(pFC->file[bWildRight], &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ConResPrintf(StdErr, IDS_CANNOT_OPEN, pFC->file[bWildRight]);
        ConPuts(StdOut, L"\n");
        return FCRET_CANT_FIND;
    }
    StringCbCopyW(szPath, sizeof(szPath), pFC->file[bWildRight]);

    fc = *pFC;
    fc.file[!bWildRight] = pFC->file[!bWildRight];
    fc.file[bWildRight] = szPath;
    do
    {
        if (IS_DOTS(find.cFileName))
            continue;

        // replace file title
        PathRemoveFileSpecW(szPath);
        PathAppendW(szPath, find.cFileName);

        switch (FileCompare(&fc))
        {
            case FCRET_IDENTICAL:
                break;
            case FCRET_DIFFERENT:
                if (ret != FCRET_INVALID)
                    ret = FCRET_DIFFERENT;
                break;
            default:
                ret = FCRET_INVALID;
                break;
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
    return ret;
}

static FCRET FileCompareWildTitle(const FILECOMPARE *pFC)
{
    FCRET ret = FCRET_IDENTICAL;
    WIN32_FIND_DATAW find;
    HANDLE hFind;
    WCHAR szPath0[MAX_PATH], szPath1[MAX_PATH];
    FILECOMPARE fc;
    LPWSTR pch;

    hFind = FindFirstFileW(pFC->file[0], &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ConResPrintf(StdErr, IDS_CANNOT_OPEN, pFC->file[0]);
        ConPuts(StdOut, L"\n");
        return FCRET_CANT_FIND;
    }
    StringCbCopyW(szPath0, sizeof(szPath0), pFC->file[0]);
    StringCbCopyW(szPath1, sizeof(szPath1), pFC->file[1]);
    pch = PathFindExtensionW(pFC->file[1]);

    fc = *pFC;
    fc.file[0] = szPath0;
    fc.file[1] = szPath1;
    do
    {
        if (IS_DOTS(find.cFileName))
            continue;

        // replace file title
        PathRemoveFileSpecW(szPath0);
        PathRemoveFileSpecW(szPath1);
        PathAppendW(szPath0, find.cFileName);
        PathAppendW(szPath1, find.cFileName);

        // replace dot extension
        PathRemoveExtensionW(szPath1);
        PathAddExtensionW(szPath1, pch);

        switch (FileCompare(&fc))
        {
            case FCRET_IDENTICAL:
                break;
            case FCRET_DIFFERENT:
                if (ret != FCRET_INVALID)
                    ret = FCRET_DIFFERENT;
                break;
            default:
                ret = FCRET_INVALID;
                break;
        }
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
    return ret;
}

static FCRET FileCompareBothWild(const FILECOMPARE *pFC)
{
    FCRET ret = FCRET_IDENTICAL;
    WIN32_FIND_DATAW find0, find1;
    HANDLE hFind0, hFind1;
    WCHAR szPath0[MAX_PATH], szPath1[MAX_PATH];
    FILECOMPARE fc;

    hFind0 = FindFirstFileW(pFC->file[0], &find0);
    if (hFind0 == INVALID_HANDLE_VALUE)
    {
        ConResPrintf(StdErr, IDS_CANNOT_OPEN, pFC->file[0]);
        ConPuts(StdOut, L"\n");
        return FCRET_CANT_FIND;
    }
    hFind1 = FindFirstFileW(pFC->file[1], &find1);
    if (hFind1 == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFind0);
        ConResPrintf(StdErr, IDS_CANNOT_OPEN, pFC->file[1]);
        ConPuts(StdOut, L"\n");
        return FCRET_CANT_FIND;
    }
    StringCbCopyW(szPath0, sizeof(szPath0), pFC->file[0]);
    StringCbCopyW(szPath1, sizeof(szPath1), pFC->file[1]);

    fc = *pFC;
    fc.file[0] = szPath0;
    fc.file[1] = szPath1;
    do
    {
        while (IS_DOTS(find0.cFileName))
        {
            if (!FindNextFileW(hFind0, &find0))
                goto quit;
        }
        while (IS_DOTS(find1.cFileName))
        {
            if (!FindNextFileW(hFind1, &find1))
                goto quit;
        }

        // replace file title
        PathRemoveFileSpecW(szPath0);
        PathRemoveFileSpecW(szPath1);
        PathAppendW(szPath0, find0.cFileName);
        PathAppendW(szPath1, find1.cFileName);

        switch (FileCompare(&fc))
        {
            case FCRET_IDENTICAL:
                break;
            case FCRET_DIFFERENT:
                if (ret != FCRET_INVALID)
                    ret = FCRET_DIFFERENT;
                break;
            default:
                ret = FCRET_INVALID;
                break;
        }
    } while (FindNextFileW(hFind0, &find0) && FindNextFileW(hFind1, &find1));
quit:
    CloseHandle(hFind0);
    CloseHandle(hFind1);
    return ret;
}

static FCRET WildcardFileCompare(FILECOMPARE *pFC)
{
    BOOL fWild0, fWild1;

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

    fWild0 = HasWildcard(pFC->file[0]);
    fWild1 = HasWildcard(pFC->file[1]);
    if (fWild0 && fWild1)
    {
        if (IsTitleWild(pFC->file[0]) && IsTitleWild(pFC->file[1]))
            return FileCompareWildTitle(pFC);
        else
            return FileCompareBothWild(pFC);
    }
    else if (fWild0)
    {
        return FileCompareOneSideWild(pFC, FALSE);
    }
    else if (fWild1)
    {
        return FileCompareOneSideWild(pFC, TRUE);
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
            case L'U':
                fc.dwFlags |= FLAG_U;
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

#ifndef __REACTOS__
int main(int argc, char **argv)
{
    INT my_argc;
    LPWSTR *my_argv = CommandLineToArgvW(GetCommandLineW(), &my_argc);
    INT ret = wmain(my_argc, my_argv);
    LocalFree(my_argv);
    return ret;
}
#endif
