/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CheckEscapesA/W
 * PROGRAMMER:      Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include "shelltest.h"

typedef void (WINAPI *FN_CheckEscapesA)(LPSTR string, DWORD len);
typedef void (WINAPI *FN_CheckEscapesW)(LPWSTR string, DWORD len);

static FN_CheckEscapesA s_pCheckEscapesA;
static FN_CheckEscapesW s_pCheckEscapesW;

typedef struct TESTENTRYA
{
    INT lineno;
    LPCSTR input;
    DWORD len;
    LPCSTR output;
} TESTENTRYA;

typedef struct TESTENTRYW
{
    INT lineno;
    LPCWSTR input;
    DWORD len;
    LPCWSTR output;
} TESTENTRYW;

static const TESTENTRYA s_entriesA[] =
{
    { __LINE__, "", 0, "" },
    { __LINE__, "", 1, "" },
    { __LINE__, "", 2, "" },
    { __LINE__, "", 3, "" },
    { __LINE__, "", 4, "" },
    { __LINE__, "", 5, "" },
    { __LINE__, "", 6, "" },
    { __LINE__, "ABC", 1, "" },
    { __LINE__, "ABC", 2, "A" },
    { __LINE__, "ABC", 3, "AB" },
    { __LINE__, "ABC", 4, "ABC" },
    { __LINE__, "ABC", 5, "ABC" },
    { __LINE__, "ABC", 6, "ABC" },
    { __LINE__, "AB C", 1, "" },
    { __LINE__, "AB C", 2, "A" },
    { __LINE__, "AB C", 3, "AB" },
    { __LINE__, "AB C", 4, "AB " },
    { __LINE__, "AB C", 5, "AB C" },
    { __LINE__, "AB C", 6, "\"AB C" },
    { __LINE__, "AB C ", 1, "" },
    { __LINE__, "AB C ", 2, "A" },
    { __LINE__, "AB C ", 3, "AB" },
    { __LINE__, "AB C ", 4, "AB " },
    { __LINE__, "AB C ", 5, "AB C" },
    { __LINE__, "AB C ", 6, "AB C " },
    { __LINE__, "AB,", 1, "" },
    { __LINE__, "AB,", 2, "A" },
    { __LINE__, "AB,", 3, "AB" },
    { __LINE__, "AB,", 4, "AB," },
    { __LINE__, "AB,", 5, "\"AB," },
    { __LINE__, "AB,", 6, "\"AB,\"" },
    { __LINE__, "AB\"", 1, "" },
    { __LINE__, "AB\"", 2, "A" },
    { __LINE__, "AB\"", 3, "AB" },
    { __LINE__, "AB\"", 4, "AB\"" },
    { __LINE__, "AB\"", 5, "\"AB\"" },
    { __LINE__, "AB\"", 6, "\"AB\"\"" },
    { __LINE__, "AB;", 1, "" },
    { __LINE__, "AB;", 2, "A" },
    { __LINE__, "AB;", 3, "AB" },
    { __LINE__, "AB;", 4, "AB;" },
    { __LINE__, "AB;", 5, "\"AB;" },
    { __LINE__, "AB;", 6, "\"AB;\"" },
    { __LINE__, "AB^", 1, "" },
    { __LINE__, "AB^", 2, "A" },
    { __LINE__, "AB^", 3, "AB" },
    { __LINE__, "AB^", 4, "AB^" },
    { __LINE__, "AB^", 5, "\"AB^" },
    { __LINE__, "AB^", 6, "\"AB^\"" },
};

static const TESTENTRYW s_entriesW[] =
{
    { __LINE__, L"", 0, L"" },
    { __LINE__, L"", 1, L"" },
    { __LINE__, L"", 2, L"" },
    { __LINE__, L"", 3, L"" },
    { __LINE__, L"", 4, L"" },
    { __LINE__, L"", 5, L"" },
    { __LINE__, L"", 6, L"" },
    { __LINE__, L"ABC", 1, L"ABC" },
    { __LINE__, L"ABC", 2, L"ABC" },
    { __LINE__, L"ABC", 3, L"ABC" },
    { __LINE__, L"ABC", 4, L"ABC" },
    { __LINE__, L"ABC", 5, L"ABC" },
    { __LINE__, L"ABC", 6, L"ABC" },
    { __LINE__, L"AB C", 1, L"AB C" },
    { __LINE__, L"AB C", 2, L"AB C" },
    { __LINE__, L"AB C", 3, L"AB C" },
    { __LINE__, L"AB C", 4, L"AB C" },
    { __LINE__, L"AB C", 5, L"AB C" },
    { __LINE__, L"AB C", 6, L"\"AB C\"" },
    { __LINE__, L"AB C ", 1, L"AB C " },
    { __LINE__, L"AB C ", 2, L"AB C " },
    { __LINE__, L"AB C ", 3, L"AB C " },
    { __LINE__, L"AB C ", 4, L"AB C " },
    { __LINE__, L"AB C ", 5, L"AB C " },
    { __LINE__, L"AB C ", 6, L"AB C " },
    { __LINE__, L"AB,", 1, L"AB," },
    { __LINE__, L"AB,", 2, L"AB," },
    { __LINE__, L"AB,", 3, L"AB," },
    { __LINE__, L"AB,", 4, L"AB," },
    { __LINE__, L"AB,", 5, L"\"AB,\"" },
    { __LINE__, L"AB,", 6, L"\"AB,\"" },
    { __LINE__, L"AB\"", 1, L"AB\"" },
    { __LINE__, L"AB\"", 2, L"AB\"" },
    { __LINE__, L"AB\"", 3, L"AB\"" },
    { __LINE__, L"AB\"", 4, L"AB\"" },
    { __LINE__, L"AB\"", 5, L"\"AB\"\"" },
    { __LINE__, L"AB\"", 6, L"\"AB\"\"" },
    { __LINE__, L"AB;", 1, L"AB;" },
    { __LINE__, L"AB;", 2, L"AB;" },
    { __LINE__, L"AB;", 3, L"AB;" },
    { __LINE__, L"AB;", 4, L"AB;" },
    { __LINE__, L"AB;", 5, L"\"AB;\"" },
    { __LINE__, L"AB;", 6, L"\"AB;\"" },
    { __LINE__, L"AB^", 1, L"AB^" },
    { __LINE__, L"AB^", 2, L"AB^" },
    { __LINE__, L"AB^", 3, L"AB^" },
    { __LINE__, L"AB^", 4, L"AB^" },
    { __LINE__, L"AB^", 5, L"\"AB^\"" },
    { __LINE__, L"AB^", 6, L"\"AB^\"" },
};

static void JustDoIt(void)
{
    SIZE_T i, count;
    CHAR bufA[MAX_PATH];
    WCHAR bufW[MAX_PATH];

    count = _countof(s_entriesA);
    for (i = 0; i < count; ++i)
    {
        lstrcpynA(bufA, s_entriesA[i].input, _countof(bufA));
        s_pCheckEscapesA(bufA, s_entriesA[i].len);
        ok(lstrcmpA(bufA, s_entriesA[i].output) == 0,
           "Line %d: output expected '%s' vs got '%s'\n",
           s_entriesA[i].lineno, s_entriesA[i].output, bufA);
    }

    count = _countof(s_entriesW);
    for (i = 0; i < count; ++i)
    {
        lstrcpynW(bufW, s_entriesW[i].input, _countof(bufW));
        s_pCheckEscapesW(bufW, s_entriesW[i].len);
        ok(lstrcmpW(bufW, s_entriesW[i].output) == 0,
           "Line %d: output expected '%ls' vs got '%ls'\n",
           s_entriesW[i].lineno, s_entriesW[i].output, bufW);
    }
}

START_TEST(CheckEscapes)
{
    HINSTANCE hShell32 = GetModuleHandleA("shell32");
    s_pCheckEscapesA = (FN_CheckEscapesA)GetProcAddress(hShell32, "CheckEscapesA");
    s_pCheckEscapesW = (FN_CheckEscapesW)GetProcAddress(hShell32, "CheckEscapesW");
    if (s_pCheckEscapesA && s_pCheckEscapesW)
    {
        JustDoIt();
    }
    else
    {
        skip("There is no CheckEscapesA/W\n");
    }
}
