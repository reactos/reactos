/////////////////////////////////////////////////////////////////////////////
// CFGDLG.H
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __CFGDLG_H__
#define __CFGDLG_H__

#include "pidllist.h"

/////////////////////////////////////////////////////////////////////////////
// Typedefs
/////////////////////////////////////////////////////////////////////////////
typedef int (STDAPICALLTYPE * FONTSPROC)(HWND hDlg, LPCSTR lpszKeyPath);

/////////////////////////////////////////////////////////////////////////////
// Structs
/////////////////////////////////////////////////////////////////////////////
struct TVCHANNELDATA
{
    ISubscriptionItem * pSubsItem;
    CString             strURL;
    CString             strTitle;
    TASK_TRIGGER        tt;
};

/////////////////////////////////////////////////////////////////////////////
// Macros
/////////////////////////////////////////////////////////////////////////////
#define UpDown_GetRange(hwndCtl)                    ((DWORD)SendMessage((hwndCtl), UDM_GETRANGE, 0, 0))
#define UpDown_SetRange(hwndCtl, posMin, posMax)    ((int)(DWORD)SendMessage((hwndCtl), UDM_SETRANGE, 0, MAKELPARAM((posMax), (posMin))))
#define UpDown_GetBuddy(hwndCtl)                    ((DWORD)SendMessage((hwndCtl), UDM_GETBUDDY, 0, 0))
#define UpDown_SetBuddy(hwndCtl, hwndBuddy)         ((DWORD)SendMessage((hwndCtl), UDM_SETBUDDY, 0, (LPARAM) (hwndBuddy) ))

/////////////////////////////////////////////////////////////////////////////
// Functions
/////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK PSGeneralPageDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL PSGeneralPage_OnInitDialog(HWND hWnd, HWND hwndFocus, LPARAM lParam);
BOOL PSGeneralPage_OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);
void PSGeneralPage_OnApply(HWND hDlg, LPARAM lParam);

#ifdef FEATURE_CUSTOMDRAWIMAGES
LRESULT CALLBACK PSGeneralPageWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif  // FEATURE_CUSTOMDRAWIMAGES

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ChannelAddToTreeProc(ISubscriptionMgr2 * pSubscriptionMgr2, CHANNELENUMINFO * pci, int nItemNum, BOOL bDefaultTopLevelURL, LPARAM lParam);
INT_PTR CALLBACK CreateSubscriptionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HIMAGELIST  LoadImageList();
void        ToggleItemCheck(HWND hWnd, HWND hwndTree, HTREEITEM hti);
HRESULT     SubscribeToChannelForScreenSaver(HWND hwnd, TVCHANNELDATA * pChannelData);

#endif  // __CFGDLG_H__
