/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SHParseDisplayName
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "shelltest.h"

/* Version masks */
#define T_ALL     0x0
#define T_WIN2K   0x1
#define T_WINXP   0x2
#define T_WIN2K3  0x4
#define T_VISTA   0x8
#define T_WIN7    0x10
#define T_WIN8    0x20
#define T_WIN10   0x40

#define T_PRE_VISTA T_WIN2K|T_WINXP|T_WIN2K3
#define T_VISTA_PLUS T_VISTA|T_WIN7|T_WIN8|T_WIN10

struct test_data
{
    int testline;
    PCWSTR wszPathToParse;
    PCWSTR wszExpectedDisplayName;
    INT nExpectedCSIDL;
    HRESULT hResult;
    UINT ValidForVersion;
};

struct test_data Tests[] =
{
    /* Tests for CDesktopFolder */
    {__LINE__, NULL, NULL, 0, E_OUTOFMEMORY, T_PRE_VISTA},
    {__LINE__, NULL, NULL, 0, E_INVALIDARG, T_VISTA_PLUS},
    {__LINE__, L"", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L" ", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), T_PRE_VISTA},
    {__LINE__, L" ", NULL, 0, E_INVALIDARG, T_VISTA_PLUS},
    {__LINE__, L":", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L": ", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L" :", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"/", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"//", NULL, 0, E_INVALIDARG, 0},
    /* This opens C:\ from Win+R and address bar */
    {__LINE__, L"\\", NULL, 0, E_INVALIDARG, 0},
    /* These two opens "C:\Program Files" from Win+R and address bar */
    {__LINE__, L"\\Program Files", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"\\Program Files\\", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"\\\\?", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"\\\\?\\", NULL, 0, E_INVALIDARG, 0},
    /* Tests for the shell: protocol */
    {__LINE__, L"shell:", NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), 0},
    {__LINE__, L"shell::", NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), 0},
    {__LINE__, L"shell:::", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0},
    {__LINE__, L"shell:::{", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0},
    {__LINE__, L"shell:fail", NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), 0},
    {__LINE__, L"shell:::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L"shell:desktop", NULL, CSIDL_DESKTOPDIRECTORY, S_OK, T_PRE_VISTA},
    {__LINE__, L"shell:windows", NULL, CSIDL_WINDOWS, S_OK, T_PRE_VISTA},
    {__LINE__, L"shell:system", NULL, CSIDL_SYSTEM, S_OK, T_PRE_VISTA},
    {__LINE__, L"shell:personal", NULL, CSIDL_MYDOCUMENTS, S_OK, T_PRE_VISTA},
    {__LINE__, L"shell:programs", NULL, CSIDL_PROGRAMS, S_OK, T_PRE_VISTA},
    {__LINE__, L"shell:programfiles", NULL, CSIDL_PROGRAM_FILES, S_OK, T_PRE_VISTA},
    /* The following tests are confusing. They don't work for SHParseDisplayName but work on psfDesktop->ParseDisplayName */
    {__LINE__, L"shell:desktop", NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    {__LINE__, L"shell:windows",  NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    {__LINE__, L"shell:system",  NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    {__LINE__, L"shell:personal",  NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    {__LINE__, L"shell:programs",  NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    {__LINE__, L"shell:programfiles",  NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_VISTA_PLUS},
    /* Tests for CInternet */
    {__LINE__, L"aa:", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"garbage:", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"ftp:", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"ftp:/", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"ftp://", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"ftp://a", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"ftp://ftp.gnu.org/gnu/octave/", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"aa:", L"aa:", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"garbage:", L"garbage:", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"ftp:", L"ftp:", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"ftp:/", L"ftp:/", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"ftp://", L"ftp:///", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"ftp://a", L"ftp://a/", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"ftp://ftp.gnu.org/gnu/octave/", L"ftp://ftp.gnu.org/gnu/octave/", 0, S_OK, T_VISTA_PLUS},
    /* Tests for CRegFolder */
    {__LINE__, L"::", NULL, 0, CO_E_CLASSSTRING, 0},
    {__LINE__, L"::{", NULL, 0, CO_E_CLASSSTRING, 0},
    {__LINE__, L"::{ ", NULL, 0, CO_E_CLASSSTRING, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D} ", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}a", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}garbage", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 0, S_OK, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D ", NULL, 0, CO_E_CLASSSTRING, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\ ", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}", 0, S_OK, 0},
     /* Tests for CDrivesFolder */
    {__LINE__, L"c:", NULL, 0, E_INVALIDARG, T_PRE_VISTA},
    {__LINE__, L"c:", L"C:\\", 0, S_OK, T_VISTA_PLUS},
    {__LINE__, L"c:\\", L"C:\\", 0, S_OK, 0},
    {__LINE__, L"C:\\", L"C:\\", 0, S_OK, 0},
    {__LINE__, L"y:\\", NULL, 0, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND), T_PRE_VISTA},
    {__LINE__, L"y:\\", NULL, 0, HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE), T_VISTA_PLUS},
    {__LINE__, L"C:\\ ", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), T_PRE_VISTA},
    {__LINE__, L"C:\\ ", NULL, 0, E_INVALIDARG, T_VISTA_PLUS},
    /* Tests for CFSFolder */
    {__LINE__, L"$", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0},
    {__LINE__, L"c:\\Program Files", L"C:\\Program Files", 0, S_OK, 0},
    {__LINE__, L"c:\\Program Files\\", L"C:\\Program Files", 0, S_OK, 0},
    /* Paths with . are valid for win+r dialog or address bar but not for ParseDisplayName */
    {__LINE__, L"c:\\Program Files\\.", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"c:\\Program Files\\..", NULL, 0, E_INVALIDARG, 0}, /* This gives C:\ when entered in address bar */
    {__LINE__, L".", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"..", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"C:\\.", NULL, 0, E_INVALIDARG, 0},
    {__LINE__, L"fonts", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0},  /* These three work for ShellExecute */
    {__LINE__, L"winsxs", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0},
    {__LINE__, L"system32", NULL, 0, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), 0}
};

UINT get_host_os_flag()
{
    switch (LOWORD(GetVersion()))
    {
    case 5: return T_WIN2K;
    case (5 | (1 << 8)): return T_WINXP;
    case (5 | (2 << 8)): return T_WIN2K3;
    case 6: return T_VISTA;
    case (6 | (1 << 8)): return T_WIN7;
    case (6 | (2 << 8)): return T_WIN8;
    case 10: return T_WIN10;
    }

    return 0;
}

START_TEST(SHParseDisplayName)
{
    HRESULT hr;
    WCHAR winDir[MAX_PATH];
    UINT os_flag = get_host_os_flag();
    ok (os_flag != 0, "Incompatible os version %d!", os_flag);
    if (os_flag == 0)
        return;

    IShellFolder *psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "hr = %lx\n", hr);

    GetWindowsDirectoryW(winDir, _countof(winDir));
    SetCurrentDirectoryW(winDir);

    for (UINT i = 0; i < _countof(Tests); i ++)
    {
        if (Tests[i].ValidForVersion && !(Tests[i].ValidForVersion & os_flag))
           continue;

        PIDLIST_ABSOLUTE pidl;
        HRESULT hr = SHParseDisplayName(Tests[i].wszPathToParse, NULL, &pidl, 0, NULL);
        ok(hr == Tests[i].hResult, "%d: Expected error 0x%lx, got 0x%lx\n", Tests[i].testline, Tests[i].hResult, hr);

        if (Tests[i].wszExpectedDisplayName == NULL && Tests[i].nExpectedCSIDL == 0)
        {
           ok(pidl == NULL, "%d: Expected no pidl\n", Tests[i].testline);
           continue;
        }

        ok(pidl != NULL, "%d: Expected pidl on success\n", Tests[i].testline);
        if(!pidl)
            continue;

        STRRET strret;
        hr = psfDesktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
        ok(hr == S_OK, "%d: hr = %lx\n", Tests[i].testline, hr);

        ok(strret.uType == STRRET_WSTR, "%d: Expected STRRET_WSTR\n", Tests[i].testline);

        if (Tests[i].wszExpectedDisplayName)
        {
            ok(!wcscmp(strret.pOleStr, Tests[i].wszExpectedDisplayName), "%d: expected %S got %S\n", Tests[i].testline, Tests[i].wszExpectedDisplayName, strret.pOleStr);
        }
        else
        {
            PIDLIST_ABSOLUTE pidlSpecial;
            hr = SHGetSpecialFolderLocation(NULL, Tests[i].nExpectedCSIDL, &pidlSpecial);
            ok(hr == S_OK, "%d: hr = %lx\n", Tests[i].testline, hr);

            STRRET strretSpecial;
            hr = psfDesktop->GetDisplayNameOf(pidlSpecial, SHGDN_FORPARSING, &strretSpecial);
            ok(hr == S_OK, "%d: hr = %lx\n", Tests[i].testline, hr);

            ok(strret.uType == STRRET_WSTR, "%d: Expected STRRET_WSTR\n", Tests[i].testline);

            ok(!wcscmp(strret.pOleStr, strretSpecial.pOleStr), "%d: expected %S got %S\n", Tests[i].testline, strretSpecial.pOleStr, strret.pOleStr);
        }
    }

    CoUninitialize();
}
