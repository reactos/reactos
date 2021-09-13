/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for Control_RunDLLW
 * PROGRAMMER:      Giannis Adamopoulos
 */

#include "shelltest.h"

#include <cpl.h>
#include <stdio.h>

#define NDEBUG
#include <debug.h>

extern "C"
void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

int g_iParams;
int g_iClk;
WCHAR g_wstrParams[MAX_PATH];

extern "C"
LONG CALLBACK
CPlApplet(HWND hwndCPl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    INT i = (INT)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 2;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = CPL_DYNAMIC_RES ;
                CPlInfo->idName = CPL_DYNAMIC_RES ;
                CPlInfo->idInfo = CPL_DYNAMIC_RES ;
            }
            break;
        case CPL_NEWINQUIRE:
        {
            LPNEWCPLINFOW pinfo = (LPNEWCPLINFOW)lParam2;

            memset(pinfo, 0, sizeof(NEWCPLINFOW));
            pinfo->dwSize = sizeof(NEWCPLINFOW);
            pinfo->hIcon = LoadIcon(NULL, IDI_APPLICATION);
            if (i == 0)
            {
                wcscpy(pinfo->szName, L"name0");
                wcscpy(pinfo->szInfo, L"info0");
                wcscpy(pinfo->szHelpFile, L"help0");
            }
            else
            {
                wcscpy(pinfo->szName, L"name1");
                wcscpy(pinfo->szInfo, L"info1");
                wcscpy(pinfo->szHelpFile, L"help1");
            }
            break;
        }
        case CPL_DBLCLK:
            g_iClk = i;
            break;
        case CPL_STARTWPARMSW:
            g_iParams = i;
            ok(lParam2 != NULL, "Got NULL lParam2!\n");
            if (lParam2)
                wcscpy(g_wstrParams, (LPCWSTR)lParam2);
            break;
    }

    return FALSE;
}


#define MSG_NOT_CALLED -1

struct param_test
{
    int srcLine;
    LPCWSTR cmd;
    INT iStartParams; /* MSG_NOT_CALLED when CPL_STARTWPARMS is not sent */
    LPCWSTR params;   /* second param of CPL_STARTWPARMS */
    INT iClick;       /* MSG_NOT_CALLED when CPL_DBLCLK is not sent */
};

struct param_test tests[] =
{
    {__LINE__, L"", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L",name0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L",name1", MSG_NOT_CALLED, L"", 1},
    {__LINE__, L",@0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L",@1", MSG_NOT_CALLED, L"", 1},
    {__LINE__, L",0", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L",1", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L",@name0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L",@name1", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L" name0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L" name1", MSG_NOT_CALLED, L"", 1},
    {__LINE__, L" @0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L" @1", MSG_NOT_CALLED, L"", 1},
    {__LINE__, L" 0", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L" 1", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L" @name0", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L" @name1", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L"\"name0\"", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L"\"name1\"", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L",\"name0\"", MSG_NOT_CALLED, L"", 0},
    {__LINE__, L",\"name1\"", MSG_NOT_CALLED, L"", 1},
    {__LINE__, L"\",name0\"", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L"\",name1\"", MSG_NOT_CALLED, L"", MSG_NOT_CALLED},
    {__LINE__, L",name0,@1", 0, L"@1", 0},
    {__LINE__, L",name1,@0", 1, L"@0", 1},
    {__LINE__, L",name0, ", 0, L" ", 0},
    {__LINE__, L",name1, ", 1, L" ", 1},
    {__LINE__, L",@0,@1", 0, L"@1", 0},
    {__LINE__, L",@1,@0", 1, L"@0", 1},
    {__LINE__, L",\"@0\",@1", 0, L"@1", 0},
    {__LINE__, L",\"@1\",@0", 1, L"@0", 1},
    {__LINE__, L",\"@0\",\"@1\"", 0, L"\"@1\"", 0},
    {__LINE__, L",\"@1\",\"@0\"", 1, L"\"@0\"", 1},
    {__LINE__, L",\"@0\",@1,2,3,4,5", 0, L"@1,2,3,4,5", 0},
    {__LINE__, L",\"@1\",@0,2,3,4,5", 1, L"@0,2,3,4,5", 1},
    {__LINE__, L",\"@0\",@1,2,\"3\",4,5", 0, L"@1,2,\"3\",4,5", 0},
    {__LINE__, L",\"@1\",@0,2,\"3\",4,5", 1, L"@0,2,\"3\",4,5", 1},
    {__LINE__, L",\"@0\", @1 , 2 , 3 , 4 , 5", 0, L" @1 , 2 , 3 , 4 , 5", 0},
    {__LINE__, L",\"@1\", @0 , 2 , 3 , 4 , 5", 1, L" @0 , 2 , 3 , 4 , 5", 1},
    {__LINE__, L",\"@0\", @1 , 2 , /3 , 4 , 5", 0, L" @1 , 2 , /3 , 4 , 5", 0},
    {__LINE__, L",\"@1\", @0 , 2 , /3 , 4 , 5", 1, L" @0 , 2 , /3 , 4 , 5", 1},
    {__LINE__, L",\"@0\", @1 , 2 , /3 , 4 , 5", 0, L" @1 , 2 , /3 , 4 , 5", 0},
    {__LINE__, L",\"@1\", @0 , 2 , /3 , 4 , 5", 1, L" @0 , 2 , /3 , 4 , 5", 1},

};

START_TEST(Control_RunDLLW)
{
    WCHAR finename[MAX_PATH];
    WCHAR buffer[MAX_PATH];

    GetModuleFileNameW(NULL, finename, MAX_PATH);

    for (UINT i = 0; i < _countof(tests); i++)
    {
        swprintf(buffer, L"%s%s", finename, tests[i].cmd);

        g_iClk = MSG_NOT_CALLED;
        g_iParams = MSG_NOT_CALLED;
        g_wstrParams[0] = 0;
        Control_RunDLLW( GetDesktopWindow (), 0, buffer, 0);
        ok (tests[i].iClick == g_iClk, "%d, CPL_DBLCLK: expected %d got %d\n", tests[i].srcLine, tests[i].iClick, g_iClk);
        ok (tests[i].iStartParams == g_iParams, "%d, CPL_STARTWPARMSW: expected %d got %d\n", tests[i].srcLine, tests[i].iStartParams, g_iParams);
        ok (wcscmp(tests[i].params, g_wstrParams) == 0, "%d, CPL_STARTWPARMSW: expected %S got %S\n", tests[i].srcLine, tests[i].params, g_wstrParams);
    }
}