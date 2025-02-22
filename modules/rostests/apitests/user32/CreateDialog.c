/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateDialog
 * PROGRAMMERS:     Andreas Maier
 */

#include "precomp.h"

#define TEST_MAX_MSG 50

// cmpflag
#define MSGLST_CMP_WP  0x1
#define MSGLST_CMP_LP  0x2
#define MSGLST_CMP_RES 0x4
#define MSGLST_CMP_ALL (MSGLST_CMP_WP | MSGLST_CMP_LP | MSGLST_CMP_RES)

typedef struct
{
    BOOL DlgProc; // true = DlgProg, false WndProc
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    int result;
    int cmpflag;
} tagMsgInfo;

typedef struct
{
    int msgCount;
    tagMsgInfo msgList[TEST_MAX_MSG];
} tagMsgList;

static tagMsgList msglist;

/* the expected message-list */
const tagMsgList t1msgList =
{
    11,
    {
        // DlgProc, msg,              wParam,   lParam, result, cmpflag
        {  FALSE,  WM_NCCREATE,            0,         0,     1, MSGLST_CMP_WP | MSGLST_CMP_RES },
        {  FALSE,  WM_NCCALCSIZE,          0,         0,     0, MSGLST_CMP_WP | MSGLST_CMP_RES },
        {  FALSE,  WM_CREATE,              0,         0,     0, MSGLST_CMP_WP | MSGLST_CMP_RES },
        {  FALSE,  WM_SIZE,                0, 0x145012c,     0, MSGLST_CMP_ALL }, // FIXME: size is 400x400 on Win7?
        {  FALSE,  WM_MOVE,                0, 0x0160003,     0,  MSGLST_CMP_WP | MSGLST_CMP_RES }, // FIXME: LPARAM doesn't match on win 10
        {  TRUE,   WM_SETFONT,             0,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
        {  FALSE,  WM_SETFONT,             0,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
        {  TRUE,   WM_INITDIALOG,          0,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
        {  FALSE,  WM_INITDIALOG,          0,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
        {  TRUE,   WM_CHANGEUISTATE,       3,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
        {  FALSE,  WM_CHANGEUISTATE,       3,         0,     0, MSGLST_CMP_LP | MSGLST_CMP_RES },
    }
};

void DumpMsgList(const char* lstName, const tagMsgList *ml)
{
    const char *dlgProcName;
    int i1;

    printf("%s\n", lstName);
    for (i1 = 0; i1 < ml->msgCount; i1++)
    {
        dlgProcName = (ml->msgList[i1].DlgProc)  ? "DlgProc" : "WndProc";
        printf("#%.3d %s, msg 0x%x, wParam 0x%Ix, lParam 0x%Ix, result %d\n",
               i1,
               dlgProcName,
               ml->msgList[i1].msg,
               ml->msgList[i1].wParam,
               ml->msgList[i1].lParam,
               ml->msgList[i1].result);
    }
}

BOOL CmpMsgList(const tagMsgList *recvd,
                const tagMsgList *expect)
{
    int i1;
    BOOL isOk;

    isOk = TRUE;
    if (recvd->msgCount != expect->msgCount)
    {
        ok(FALSE, "%d messages expected, %d messages received!\n",
           expect->msgCount, recvd->msgCount);
        isOk = FALSE;
    }
    else
    {
        for (i1 = 0; i1 < recvd->msgCount; i1++)
        {
            if (expect->msgList[i1].DlgProc != recvd->msgList[i1].DlgProc)
                isOk = FALSE;
            if (expect->msgList[i1].msg != recvd->msgList[i1].msg)
                isOk = FALSE;
            if ((expect->msgList[i1].cmpflag & MSGLST_CMP_WP) &&
                (expect->msgList[i1].wParam != recvd->msgList[i1].wParam))
                isOk = FALSE;
            if ((expect->msgList[i1].cmpflag & MSGLST_CMP_LP) &&
                (expect->msgList[i1].lParam != recvd->msgList[i1].lParam))
                isOk = FALSE;
            if ((expect->msgList[i1].cmpflag & MSGLST_CMP_RES) &&
                (expect->msgList[i1].result != recvd->msgList[i1].result))
                isOk = FALSE;
            if (!isOk)
            {
                ok(FALSE, "Message #%.3d not equal\n", i1);
                break;
            }
        }
    }

    if (!isOk)
    {
        DumpMsgList("RECEIVED", recvd);
        DumpMsgList("EXPECTED", expect);
        return FALSE;
    }

    ok(TRUE, "\n");
    return TRUE;
}

INT_PTR CALLBACK Test_CreateDialogW_DLGPROC(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msglist.msgCount < TEST_MAX_MSG)
    {
        msglist.msgList[msglist.msgCount].DlgProc = TRUE;
        msglist.msgList[msglist.msgCount].msg     = msg;
        msglist.msgList[msglist.msgCount].wParam  = wParam;
        msglist.msgList[msglist.msgCount].lParam  = lParam;
        msglist.msgList[msglist.msgCount].result  = 0;
        msglist.msgCount++;
    }
    trace("DlgProc: msg 0x%x, wParam 0x%x, lParam 0x%Ix\n",
           msg, wParam, lParam);
    return FALSE;
}

LRESULT CALLBACK Test_CreateDialogW_WNDPROC(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res;
    res = DefDlgProcW(hWnd, msg, wParam, lParam);

    if (msglist.msgCount < TEST_MAX_MSG)
    {
        msglist.msgList[msglist.msgCount].DlgProc = FALSE;
        msglist.msgList[msglist.msgCount].msg     = msg;
        msglist.msgList[msglist.msgCount].wParam  = wParam;
        msglist.msgList[msglist.msgCount].lParam  = lParam;
        msglist.msgList[msglist.msgCount].result  = res;
        msglist.msgCount++;
    }
    trace("WndProc: msg 0x%x, wParam 0x%x, lParam 0x%Ix, result %Id\n",
          msg, wParam, lParam, res);
    return res;
}

void Test_CreateDialogW()
{
    HWND hWnd;
    HMODULE hMod;
    DWORD exstyle;
    WNDCLASSW wc;

    hMod = GetModuleHandle(NULL);
    ok(hMod != NULL, "\n");

    msglist.msgCount = 0;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Test_CreateDialogW_WNDPROC;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hInstance     = hMod;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = L"TestDialogClass";

    if (!RegisterClassW(&wc))
    {
         ok(FALSE, "Error registering Window-Class\n");
         return;
    }
    hWnd = CreateDialogW(hMod, L"TESTDIALOG", 0, Test_CreateDialogW_DLGPROC);
    ok(hWnd != NULL, "Error: %lu\n", GetLastError());
    if (hWnd != NULL)
    {
        /* Check the exstyle */
        exstyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
        ok(exstyle != 0x50010, "ExStyle wrong, got %#08lX, expected 0x50010.\n", exstyle);
        /* Check the messages we received during creation */
        CmpMsgList(&msglist, &t1msgList);
    }
}

START_TEST(CreateDialog)
{
    //Test_CreateDialogA();//TODO
    Test_CreateDialogW();
}
