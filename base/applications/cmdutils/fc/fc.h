/*
 * PROJECT:     ReactOS FC Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Comparing files
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#pragma once
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef __REACTOS__
    #include <windef.h>
    #include <winbase.h>
    #include <winuser.h>
    #include <winnls.h>
#else
    #include <windows.h>
#endif
#include <wine/list.h>
#include "resource.h"

// See also: https://stackoverflow.com/questions/33125766/compare-files-with-a-cmd
typedef enum FCRET // return code of FC command
{
    FCRET_INVALID = -1,
    FCRET_IDENTICAL = 0,
    FCRET_DIFFERENT = 1,
    FCRET_CANT_FIND = 2,
    FCRET_NO_MORE_DATA = 3 // (extension)
} FCRET;

typedef struct NODE_W
{
    struct list entry;
    LPWSTR pszLine;
    LPWSTR pszComp; // compressed
    DWORD lineno;
    DWORD hash;
} NODE_W;
typedef struct NODE_A
{
    struct list entry;
    LPSTR pszLine;
    LPSTR pszComp; // compressed
    DWORD lineno;
    DWORD hash;
} NODE_A;

#define FLAG_A (1 << 0) // abbreviation
#define FLAG_B (1 << 1) // binary
#define FLAG_C (1 << 2) // ignore cases
#define FLAG_L (1 << 3) // ASCII mode
#define FLAG_LBn (1 << 4) // line buffers
#define FLAG_N (1 << 5) // show line numbers
#define FLAG_OFFLINE (1 << 6) // ???
#define FLAG_T (1 << 7) // prevent fc from converting tabs to spaces
#define FLAG_U (1 << 8) // Unicode
#define FLAG_W (1 << 9) // compress white space
#define FLAG_nnnn (1 << 10) // ???
#define FLAG_HELP (1 << 11) // show usage

typedef struct FILECOMPARE
{
    DWORD dwFlags; // FLAG_...
    INT n; // # of line buffers
    INT nnnn; // retry count before resynch
    LPCWSTR file[2];
    struct list list[2];
} FILECOMPARE;

// text.h
FCRET TextCompareW(FILECOMPARE *pFC,
                   HANDLE *phMapping0, const LARGE_INTEGER *pcb0,
                   HANDLE *phMapping1, const LARGE_INTEGER *pcb1);
FCRET TextCompareA(FILECOMPARE *pFC,
                   HANDLE *phMapping0, const LARGE_INTEGER *pcb0,
                   HANDLE *phMapping1, const LARGE_INTEGER *pcb1);
// fc.c
VOID PrintLineW(const FILECOMPARE *pFC, DWORD lineno, LPCWSTR psz);
VOID PrintLineA(const FILECOMPARE *pFC, DWORD lineno, LPCSTR psz);
VOID PrintCaption(LPCWSTR file);
VOID PrintEndOfDiff(VOID);
VOID PrintDots(VOID);
FCRET NoDifference(VOID);
FCRET Different(LPCWSTR file0, LPCWSTR file1);
FCRET LongerThan(LPCWSTR file0, LPCWSTR file1);
FCRET OutOfMemory(VOID);
FCRET CannotRead(LPCWSTR file);
FCRET InvalidSwitch(VOID);
FCRET ResyncFailed(VOID);
HANDLE DoOpenFileForInput(LPCWSTR file);

#ifdef _WIN64
    #define MAX_VIEW_SIZE (256 * 1024 * 1024) // 256 MB
#else
    #define MAX_VIEW_SIZE (64 * 1024 * 1024) // 64 MB
#endif
