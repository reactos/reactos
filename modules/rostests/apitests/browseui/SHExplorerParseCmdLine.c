/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SHExplorerParseCmdLine
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org
 */

#include <apitest.h>

//#define UNICODE
#include <strsafe.h>
#include <shlobj.h>

// Macro parameters are only expanded in the second nesting...
#define _WIDEN(x) L##x
#define WIDEN(x) _WIDEN(x)

#define TEST_FILENAMEA "SHExplorerParseCmdLine.test"
#define TEST_FILENAMEW WIDEN(TEST_FILENAMEA)

#define TEST_PATHA "C:\\SHExplorerParseCmdLine.test"
#define TEST_PATHW WIDEN(TEST_PATHA)

#define PADDING_SIZE 0x100

typedef struct _EXPLORER_INFO
{
    PWSTR FileName;
    PIDLIST_ABSOLUTE pidl;
    DWORD dwFlags;
    ULONG Unknown1[5];
    PIDLIST_ABSOLUTE pidlRoot;
    ULONG Unknown3[4];
    GUID guidInproc;
    ULONG Padding[PADDING_SIZE];
} EXPLORER_INFO, *PEXPLORER_INFO;

UINT_PTR (WINAPI *SHExplorerParseCmdLine)(_Out_ PEXPLORER_INFO Info);

#define PIDL_IS_UNTOUCHED -1
#define PIDL_IS_NULL -2
#define PIDL_IS_PATH -3
#define PIDL_IS_EMPTY -4
#define PIDL_PATH_EQUALS_PATH -5

#define InvalidPointer ((PVOID)0x5555555555555555ULL)

DWORD ReplaceSubstr(
_Out_ PWCHAR OutputStr,
_In_ DWORD OutputLen,
_In_ PCWSTR InputStr,
_In_ PCWSTR ReplaceStr,
_In_ PCWSTR ReplaceWith)
{
    DWORD result = 0;
    PCWSTR pos;
    PCWSTR pwc;

    if (!OutputLen)
        return result;

    OutputStr[0] = 0;
    pos = InputStr;
    pwc = wcsstr(pos, ReplaceStr);
    while (pwc)
    {
        if (StringCchCatNW(OutputStr, OutputLen, pos, pwc - pos) == STRSAFE_E_INSUFFICIENT_BUFFER)
            break;
        if (StringCchCatW(OutputStr, OutputLen, ReplaceWith) == STRSAFE_E_INSUFFICIENT_BUFFER)
            break;
        result++;
        pos = pwc + wcslen(ReplaceStr);
        pwc = wcsstr(pos, ReplaceStr);
    }
    StringCchCatW(OutputStr, OutputLen, pos);
    return result;
}

static
VOID
TestCommandLine(
_In_ ULONG TestLine,
_In_ INT ExpectedRet,
_In_ INT ExpectedCsidl,
_In_ DWORD ExpectedFlags,
_In_ PCWSTR ExpectedFileName,
_In_ PCWSTR PidlPath,
_Out_opt_ PUINT PWriteEnd)
{
    EXPLORER_INFO Info;
    UINT_PTR Ret;
    ULONG i;
    PDWORD InfoWords = (PDWORD) &Info;

    FillMemory(&Info, sizeof(Info), 0x55);
    Info.dwFlags = 0x00000000;
    Ret = SHExplorerParseCmdLine(&Info);

    // Special case for empty cmdline: Ret is the PIDL for the selected folder.
    if (ExpectedRet == -1)
        ok((LPITEMIDLIST) Ret == Info.pidl, "Line %lu: Ret = %x, expected %p\n", TestLine, Ret, Info.pidl);
    else
        ok(Ret == ExpectedRet, "Line %lu: Ret = %x, expected %08x\n", TestLine, Ret, ExpectedRet);

    if (ExpectedFileName == NULL)
        ok(Info.FileName == InvalidPointer, "Line %lu: FileName = %p\n", TestLine, Info.FileName);
    else
    {
        ok(Info.FileName != NULL && Info.FileName != InvalidPointer, "Line %lu: FileName = %p\n", TestLine, Info.FileName);
        if (Info.FileName != NULL && Info.FileName != InvalidPointer)
        {
            ok(!wcscmp(Info.FileName, ExpectedFileName), "Line %lu: FileName = %ls, expected %ls\n", TestLine, Info.FileName, ExpectedFileName);
            LocalFree(Info.FileName);
        }
    }

    ok(Info.dwFlags == ExpectedFlags, "Line %lu: dwFlags = %08lx, expected %08lx\n", TestLine, Info.dwFlags, ExpectedFlags);

    if (ExpectedCsidl == PIDL_IS_UNTOUCHED)
        ok(Info.pidl == InvalidPointer, "Line %lu: pidl = %p\n", TestLine, Info.pidl);
    else if (ExpectedCsidl == PIDL_IS_NULL)
        ok(Info.pidl == NULL, "Line %lu: pidl = %p\n", TestLine, Info.pidl);
    else
    {
        PIDLIST_ABSOLUTE ExpectedPidl;
        HRESULT hr;

        ok(Info.pidl != NULL, "Line %lu: pidl = %p\n", TestLine, Info.pidl);
        if (Info.pidl != NULL && Info.pidl != InvalidPointer)
        {
            WCHAR pidlPathName[MAX_PATH] = L"";
            WCHAR pidlPathTest[MAX_PATH] = L"";
            WCHAR rootDir[MAX_PATH] = L"";
            WCHAR curDir[MAX_PATH] = L"";
            WCHAR replaceName[MAX_PATH];

            GetFullPathNameW(L"\\", _countof(rootDir), rootDir, NULL);

            GetCurrentDirectoryW(_countof(curDir), curDir);
            if (wcslen(curDir) != 0 && curDir[wcslen(curDir) - 1] != L'\\')
                StringCchCatW(curDir, _countof(curDir), L"\\");

            if (PidlPath)
            {
                StringCchCopyW(pidlPathTest, _countof(pidlPathTest), PidlPath);

                if (wcsstr(pidlPathTest, L"::ROOT::") != NULL && wcslen(rootDir) > 0)
                {
                    if (ReplaceSubstr(replaceName, _countof(replaceName), pidlPathTest, L"::ROOT::", rootDir))
                        StringCchCopyW(pidlPathTest, _countof(pidlPathTest), replaceName);
                }

                if (wcsstr(pidlPathTest, L"::CURDIR::") != NULL && wcslen(curDir) > 0)
                {
                    if (ReplaceSubstr(replaceName, _countof(replaceName), pidlPathTest, L"::CURDIR::", curDir))
                        StringCchCopyW(pidlPathTest, _countof(pidlPathTest), replaceName);
                }
            }

            if (Info.pidl != NULL && Info.pidl != (LPITEMIDLIST) 0x55555555)
            {
                SHGetPathFromIDListW(Info.pidl, pidlPathName);
            }

            if (ExpectedCsidl == PIDL_PATH_EQUALS_PATH)
            {
                ok(wcsicmp(pidlPathName, pidlPathTest) == 0, "Line %lu: Path from pidl does not match; pidlPathName=%S\n", TestLine, pidlPathName);
            }
            else if (ExpectedCsidl == PIDL_IS_EMPTY)
            {
                ok(wcslen(pidlPathName) == 0, "Line %lu: Unexpected non-empty path from pidl; pidlPathName=%S\n", TestLine, pidlPathName);
            }
            else
            {
                if (ExpectedCsidl == PIDL_IS_PATH)
                {
                    ExpectedPidl = SHSimpleIDListFromPath(pidlPathTest);
                    hr = ExpectedPidl == NULL ? E_FAIL : S_OK;
                    ok(ExpectedPidl != NULL, "Line %lu: SHSimpleIDListFromPath(%S) failed. pidlPathName=%S\n", TestLine, pidlPathTest, pidlPathName);
                    if (SUCCEEDED(hr))
                    {
                        ok(ILIsEqual(Info.pidl, ExpectedPidl), "Line %lu: Unexpected pidl value %p; pidlPathName=%S pidlPathTest=%S\n", TestLine, Info.pidl, pidlPathName, pidlPathTest);
                        ILFree(ExpectedPidl);
                    }
                }
                else
                {
                    hr = SHGetFolderLocation(NULL, ExpectedCsidl, NULL, 0, &ExpectedPidl);
                    ok(hr == S_OK, "Line %lu: SHGetFolderLocation returned %08lx\n", TestLine, hr);
                    if (SUCCEEDED(hr))
                    {
                        BOOL eq = ILIsEqual(Info.pidl, ExpectedPidl);
                        ILFree(ExpectedPidl);

                        ok(eq, "Line %lu: Unexpected pidl value %p; pidlPathName=%S CSIDL=%d\n", TestLine, Info.pidl, pidlPathName, ExpectedCsidl);
                    }
                }
            }

            if (Info.pidl != NULL && Info.pidl != (LPITEMIDLIST) 0x55555555)
                ILFree(Info.pidl);
        }
    }

    for (i = 0; i < sizeof(Info) / sizeof(DWORD); i++)
    {
        switch (i*4)
        {
        case 0x00: // FileName
        case 0x04: // pidl
        case 0x08: // dwFlags
        case 0x20: // pidlRoot
        case 0x34: // guidInproc (1/4)
        case 0x38: // guidInproc (2/4)
        case 0x3C: // guidInproc (3/4)
        case 0x40: // guidInproc (4/4)
            break;
        default:
            ok(InfoWords[i] == 0x55555555, "Line %lu: Word 0x%02lx has been set to 0x%08lx\n", TestLine, i * 4, InfoWords[i]);
        }
    }

    if (PWriteEnd)
    {
        PBYTE data = (PBYTE)&Info;

        *PWriteEnd = 0;

        for (i = sizeof(Info); i > 0; i--)
        {
            if (data[i - 1] != 0x55)
            {
                *PWriteEnd = i;
                break;
            }
        }
    }
}

START_TEST(SHExplorerParseCmdLine)
{
    static struct
    {
        INT TestLine;
        PCWSTR CommandLine;
        INT ExpectedRet;
        INT ExpectedCsidl;
        DWORD ExpectedFlags;
        PCWSTR ExpectedFileName;
        PCWSTR PidlPath;
    } Tests [] =
    {
        { __LINE__, L"", -1, CSIDL_MYDOCUMENTS, 0x00000009 },
        { __LINE__, L"/e", TRUE, PIDL_IS_UNTOUCHED, 0x00000008 },
        { __LINE__, L"/n", TRUE, PIDL_IS_UNTOUCHED, 0x00004001 },
        { __LINE__, L"/x", TRUE, PIDL_IS_NULL, 0x02000000, L"/x" },
        { __LINE__, L"-e", TRUE, PIDL_IS_NULL, 0x02000000, L"-e" },
        { __LINE__, L"C:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/e,C:\\", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"/select,C:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/e,::{20d04fe0-3aea-1069-a2d8-08002b30309d}", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"::{20d04fe0-3aea-1069-a2d8-08002b30309d}" },
        { __LINE__, L"::{645ff040-5081-101b-9f08-00aa002f954e}", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::{645ff040-5081-101b-9f08-00aa002f954e}" },
        { __LINE__, L"/select,::{450d8fba-ad25-11d0-98a8-0800361b1103}", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}" },
        { __LINE__, L"=", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::{20d04fe0-3aea-1069-a2d8-08002b30309d}" },
//        { __LINE__, L".", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings\\gigaherz\\Desktop" },
//        { __LINE__, L"..", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings\\gigaherz" },
        { __LINE__, L"wrongpath", TRUE, PIDL_IS_NULL, 0x02000000, L"wrongpath"},
        { __LINE__, L"%wrongdir%", TRUE, PIDL_IS_NULL, 0x02000000, L"%wrongdir%"},
        { __LINE__, L"%programfiles#", TRUE, PIDL_IS_NULL, 0x02000000, L"%programfiles#"},
        { __LINE__, L",", TRUE, PIDL_IS_EMPTY, 0x00000200},
        { __LINE__, L"\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::ROOT::" }, // disk letter depends on current directory
        { __LINE__, L"c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"c:", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"c", TRUE, PIDL_IS_NULL, 0x02000000, L"c"},
        { __LINE__, L"c:\\Program Files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\Program Files\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\Program Files/", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\Program Files/"},
        { __LINE__, L"c:/Program Files/", TRUE, PIDL_IS_NULL, 0x02000000, L"c:/Program Files/"},
        { __LINE__, L"fonts", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::fonts" }, // this would not fail if we are in Windows directory
        { __LINE__, L"winsxs", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::winsxs" },
        { __LINE__, L"system32", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::system32" },
        { __LINE__, L"drivers", TRUE, PIDL_IS_NULL, 0x02000000, L"drivers" }, // this would fail since we are not in system32 directory
        { __LINE__, L"spool", TRUE, PIDL_IS_NULL, 0x02000000, L"spool" },
 //       { __LINE__, L"wbem", TRUE, PIDL_IS_NULL, 0x02000000, L"wbem" },
        { __LINE__, TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000200, NULL, TEST_PATHW },
        { __LINE__, L"\"c:\\\"\"program files\"", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\\"program files"},
        { __LINE__, L"\"c:\\\"program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"\"c:\\ \"program files", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\ program files"},
        { __LINE__, L"\"c:\\\" program files", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\ program files"},
        { __LINE__, L"\"c:\\\", \"c:\\program files\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\,c:\\program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root", 0, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"\"/root\"", 0, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,", TRUE, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,c", TRUE, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,\"\"", TRUE, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,wrong", TRUE, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,0", TRUE, CSIDL_MYDOCUMENTS, 0x00000000},
        { __LINE__, L"/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"/root,\"c:\\\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"/root \"c:\\\"", TRUE, PIDL_IS_NULL, 0x02000000, L"/root c:\\"},
        { __LINE__, L"/root,\"c:\\\"\"program files\"", TRUE, PIDL_IS_PATH, 0x00000000},
        { __LINE__, L"/root,\"c:\\\"program files", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\Program Files" },
        { __LINE__, L"/root,c:\\,c:\\Program Files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root,c:\\,Program Files", TRUE, PIDL_IS_NULL, 0x02000000, L"Program Files"},
        { __LINE__, L"/root,\"c:\\\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"c:\\Program Files,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"c:\\" },
//        { __LINE__, L"a:\\,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"c:\\" },
//        { __LINE__, L"a:\\,/root,c", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"A:\\" },
        { __LINE__, L"c:\\,/root,c", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/select", TRUE, CSIDL_MYDOCUMENTS, 0x00000040},
        { __LINE__, L"/select,", TRUE, CSIDL_DRIVES, 0x00000240 },
        { __LINE__, L"/select,c", TRUE, PIDL_IS_NULL, 0x02000040, L"c"},
        { __LINE__, L"/select,0", TRUE, PIDL_IS_NULL, 0x02000040, L"0"},
        { __LINE__, L"/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"c:\\,/select", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"/select,c:\\Program Files,c:\\Documents and settings", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\Documents and Settings" },
        { __LINE__, L"c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000240, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
//        { __LINE__, L"a:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"z:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x02000240, L"z:\\", TEST_PATHW },
        { __LINE__, L"select,c:\\ ", TRUE, PIDL_IS_PATH, 0x02000200, L"select", L"C:\\" },
        { __LINE__, L"/select c:\\ ", TRUE, PIDL_IS_NULL, 0x02000000, L"/select c:\\"},
//        { __LINE__, L"a:\\,/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"a:\\,/select,c", TRUE, PIDL_IS_NULL, 0x02000040, L"c"},
        { __LINE__, L"c:\\,/select,c", TRUE, PIDL_IS_NULL, 0x02000240, L"c"},
        { __LINE__, L"/e", TRUE, CSIDL_MYDOCUMENTS, 0x00000008},
        { __LINE__, L"/e,", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,\"", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,\"\"", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,c:\\", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"c:\\,/e", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"/e,c", TRUE, PIDL_IS_NULL, 0x02000008, L"c"},
        { __LINE__, L"/root,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000040, TEST_FILENAMEW },
        { __LINE__, L"/select," TEST_FILENAMEW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x02000040, TEST_FILENAMEW, L"c:\\" },
        { __LINE__, L"/root,c:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"/select," TEST_PATHW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000240, NULL, L"c:\\" },
        { __LINE__, L"/e,/select," TEST_FILENAMEW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x02000048, TEST_FILENAMEW, L"c:\\" },
        { __LINE__, L"/e,/root,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000048, TEST_FILENAMEW },
        { __LINE__, L"/e,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000008, NULL, L"c:\\" },
        { __LINE__, L"/e,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/e,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/select," TEST_FILENAMEW L",/e", TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000000, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/e,http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000008, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/root,c:\\,http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000000, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/separate ", TRUE, CSIDL_MYDOCUMENTS, 0x00020000},
        { __LINE__, L"/separate,c:\\ program files", TRUE, PIDL_IS_NULL, 0x02020000, L"c:\\ program files"},
        { __LINE__, L"/separate,           c:\\program files", TRUE, PIDL_IS_PATH, 0x00020200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/separate,           c:\\program files           ,/e", TRUE, PIDL_IS_PATH, 0x00020208, NULL, L"C:\\Program Files" },
        { __LINE__, L"/separate,           c:\\program files           ,\\e", TRUE, PIDL_IS_NULL, 0x02020200, L"\\e"},
        { __LINE__, L"c:\\Documents and settings,/separate,/n,/e,/root,c:\\,/select,c:\\Program files,", TRUE, CSIDL_DRIVES, 0x00024249 },
        { __LINE__, L"c:\\Documents and settings,/separate,/n,/e,/root,{450D8FBA-AD25-11D0-98A8-0800361B1103, 0},test,/select,c:\\Program files,", TRUE, CSIDL_DRIVES, 0x02024249, L"test" },
        { __LINE__, L"c:\\Documents and settings,/inproc,/noui,/s,/separate,/n,/e,/root,{450D8FBA-AD25-11D0-98A8-0800361B1103, 0},test,/select,c:\\Program files,", FALSE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings" },
        { __LINE__, L"=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"=" TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000200, NULL, TEST_PATHW },
        { __LINE__, L"/root,=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/root=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/root=c:\\Program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root=\"c:\\Program files\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root=\"\"c:\\Program files\"\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\=/root=\"c:\\Program files\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"C:\\Program Files" },
        { __LINE__, L"/select=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/select=c:\\Program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"=,/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/select,c:\\,=", TRUE, CSIDL_DRIVES, 0x00000240 },
        { __LINE__, L"c:\\=/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"=/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"/e,=", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/e=\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/e=\"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"=\"=\"", TRUE, PIDL_IS_NULL, 0x02000000, L"="},
        { __LINE__, L"==\"=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"="},
        { __LINE__, L"===\"=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"="},
        { __LINE__, L"=\"=\"", TRUE, PIDL_IS_NULL, 0x02000000, L"="},
        { __LINE__, L"==\"==\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=="},
        { __LINE__, L"===\"===\"", TRUE, PIDL_IS_NULL, 0x02000200, L"==="},
        { __LINE__, L"=\"=a\"", TRUE, PIDL_IS_NULL, 0x02000000, L"=a"},
        { __LINE__, L"==\"=a=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=a="},
        { __LINE__, L"===\"=a=a=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=a=a="},
        { __LINE__, L"=\"=a\"==", TRUE, CSIDL_DRIVES, 0x02000200, L"=a" },
        { __LINE__, L"==\"=a=\"=", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=" },
        { __LINE__, L"===\"=a=a=\"===", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=a=" },
        { __LINE__, L"=,=,=,\"=a=a=\",=,=,=", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=a=" },
        { __LINE__, L"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"\"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\""},
        { __LINE__, L"\"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\""},
        { __LINE__, L"\"\"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\"\""},
        { __LINE__, L"/s", TRUE, CSIDL_MYDOCUMENTS, 0x00000002},
        { __LINE__, L"/noui", TRUE, CSIDL_MYDOCUMENTS, 0x00001000},
        { __LINE__, L"/idlist", TRUE, PIDL_IS_UNTOUCHED, 0x00000000},
        { __LINE__, L"-embedding", TRUE, CSIDL_MYDOCUMENTS, 0x00000080 },
        { __LINE__, L"/inproc", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,1", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,a", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,c:\\", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,\"c:\\\"", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,{20d04fe0-3aea-1069-a2d8-08002b30309d}", TRUE, PIDL_IS_UNTOUCHED, 0x00000400 },
    };
    const int TestCount = sizeof(Tests) / sizeof(Tests[0]);
    PWSTR CommandLine;
    WCHAR OriginalCommandLine[1024];
    int i;
    UINT maxWrite = 0;
    FILE * ff;
    WCHAR winDir[MAX_PATH];

    HMODULE browseui = LoadLibraryA("browseui.dll");
    SHExplorerParseCmdLine = (UINT_PTR (__stdcall *)(PEXPLORER_INFO))GetProcAddress(browseui, MAKEINTRESOURCEA(107));
    if (!SHExplorerParseCmdLine)
    {
        skip("SHExplorerParseCmdLine not found, NT 6.0?\n");
        return;
    }

    CommandLine = GetCommandLineW();
    StringCbCopyW(OriginalCommandLine, sizeof(OriginalCommandLine), CommandLine);

    ff = fopen(TEST_PATHA, "wb");
    fclose(ff);

    GetWindowsDirectoryW(winDir, _countof(winDir));
    SetCurrentDirectoryW(winDir);

    for (i = 0; i < TestCount; i++)
    {
        UINT cWrite;

        wcscpy(CommandLine, L"browseui_apitest.exe ");
        wcscat(CommandLine, Tests[i].CommandLine);
        trace("Command line (%d): %ls\n", Tests[i].TestLine, CommandLine);
        TestCommandLine(Tests[i].TestLine,
                        Tests[i].ExpectedRet,
                        Tests[i].ExpectedCsidl,
                        Tests[i].ExpectedFlags,
                        Tests[i].ExpectedFileName,
                        Tests[i].PidlPath,
                        &cWrite);

        if (cWrite > maxWrite)
            maxWrite = cWrite;
    }

    trace("Writes reached the byte right before 0x%08x\n", maxWrite);

    wcscpy(CommandLine, OriginalCommandLine);

    remove(TEST_PATHA);
}
