/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *                  2015 Robert Naumann <gonzomdx@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

static void SetBitmap(HWND hwnd, HBITMAP* hbmp, UINT uImageId)
{
    if (*hbmp)
        DeleteObject(*hbmp);

    *hbmp = (HBITMAP)LoadImageW(hExplorerInstance,
                                MAKEINTRESOURCEW(uImageId),
                                IMAGE_BITMAP,
                                0,
                                0,
                                LR_DEFAULTCOLOR);

    if (*hbmp && hwnd)
    {
        BITMAP bm;
        GetObject(*hbmp, sizeof(bm), &bm);
        ::SetWindowPos(hwnd, NULL, 0, 0, bm.bmWidth + 2, bm.bmHeight + 2,
                       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)*hbmp);
    }

}

class CTaskBarSettingsPage : public CPropertyPageImpl<CTaskBarSettingsPage>
{
private: 
    HBITMAP m_hbmpTaskbar;
    HBITMAP m_hbmpTray;
    HWND m_hwndTaskbar;

    void UpdateDialog()
    {
        BOOL bLock = IsDlgButtonChecked(IDC_TASKBARPROP_LOCK);
        BOOL bHide = IsDlgButtonChecked(IDC_TASKBARPROP_HIDE);
        BOOL bGroup = IsDlgButtonChecked(IDC_TASKBARPROP_GROUP);
        BOOL bShowQL = IsDlgButtonChecked(IDC_TASKBARPROP_SHOWQL);
        BOOL bShowClock = IsDlgButtonChecked(IDC_TASKBARPROP_CLOCK);
        BOOL bShowSeconds = IsDlgButtonChecked(IDC_TASKBARPROP_SECONDS);
        BOOL bHideInactive = IsDlgButtonChecked(IDC_TASKBARPROP_HIDEICONS);
        UINT uImageId;

        HWND hwndCustomizeNotifyButton = GetDlgItem(IDC_TASKBARPROP_ICONCUST);
        HWND hwndSeconds = GetDlgItem(IDC_TASKBARPROP_SECONDS);
        HWND hwndTaskbarBitmap = GetDlgItem(IDC_TASKBARPROP_TASKBARBITMAP);
        HWND hwndTrayBitmap = GetDlgItem(IDC_TASKBARPROP_NOTIFICATIONBITMAP);

        if (bHide)
            uImageId = IDB_TASKBARPROP_AUTOHIDE;
        else if (bLock  && bGroup  && bShowQL)
            uImageId = IDB_TASKBARPROP_LOCK_GROUP_QL;
        else if (bLock  && !bGroup && !bShowQL)
            uImageId = IDB_TASKBARPROP_LOCK_NOGROUP_NOQL;
        else if (bLock  && bGroup  && !bShowQL)
            uImageId = IDB_TASKBARPROP_LOCK_GROUP_NOQL;
        else if (bLock  && !bGroup && bShowQL)
            uImageId = IDB_TASKBARPROP_LOCK_NOGROUP_QL;
        else if (!bLock && !bGroup && !bShowQL)
            uImageId = IDB_TASKBARPROP_NOLOCK_NOGROUP_NOQL;
        else if (!bLock && bGroup  && !bShowQL)
            uImageId = IDB_TASKBARPROP_NOLOCK_GROUP_NOQL;
        else if (!bLock && !bGroup && bShowQL)
            uImageId = IDB_TASKBARPROP_NOLOCK_NOGROUP_QL;
        else if (!bLock && bGroup  && bShowQL)
            uImageId = IDB_TASKBARPROP_NOLOCK_GROUP_QL;
        else 
            ASSERT(FALSE);

        SetBitmap(hwndTaskbarBitmap, &m_hbmpTaskbar, uImageId);

        ::EnableWindow(hwndCustomizeNotifyButton, bHideInactive);
        ::EnableWindow(hwndSeconds, bShowClock);
        if (!bShowSeconds)
            CheckDlgButton(IDC_TASKBARPROP_SECONDS, BST_UNCHECKED);

        if (bHideInactive && bShowClock && bShowSeconds)
            uImageId = IDB_SYSTRAYPROP_HIDE_SECONDS;
        else if (bHideInactive && bShowClock && !bShowSeconds)
            uImageId = IDB_SYSTRAYPROP_HIDE_CLOCK;
        else if (bHideInactive && !bShowClock)
            uImageId = IDB_SYSTRAYPROP_HIDE_NOCLOCK;
        else if (!bHideInactive && bShowClock && bShowSeconds)
            uImageId = IDB_SYSTRAYPROP_SHOW_SECONDS;
        else if (!bHideInactive && bShowClock && !bShowSeconds)
            uImageId = IDB_SYSTRAYPROP_SHOW_CLOCK;
        else if (!bHideInactive && !bShowClock)
            uImageId = IDB_SYSTRAYPROP_SHOW_NOCLOCK;
        else 
            ASSERT(FALSE);

        SetBitmap(hwndTrayBitmap, &m_hbmpTray, uImageId);
    }

public:
    enum { IDD = IDD_TASKBARPROP_TASKBAR };

    BEGIN_MSG_MAP(CTaskBarSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_TASKBARPROP_ICONCUST, OnCustomizeTrayIcons)
        COMMAND_RANGE_HANDLER(IDC_TASKBARPROP_FIRST_CMD, IDC_TASKBARPROP_LAST_CMD, OnCtrlCommand)
        CHAIN_MSG_MAP(CPropertyPageImpl<CTaskBarSettingsPage>)
    END_MSG_MAP()

    CTaskBarSettingsPage(HWND hwnd):
        m_hbmpTaskbar(NULL),
        m_hbmpTray(NULL),
        m_hwndTaskbar(hwnd)
    {
    }
    
    ~CTaskBarSettingsPage()
    {
        if (m_hbmpTaskbar)
            DeleteObject(m_hbmpTaskbar);
        if (m_hbmpTray)
            DeleteObject(m_hbmpTray);
    }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        CheckDlgButton(IDC_TASKBARPROP_LOCK, g_TaskbarSettings.bLock ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_HIDE, g_TaskbarSettings.sr.AutoHide ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_ONTOP, g_TaskbarSettings.sr.AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_GROUP, g_TaskbarSettings.bGroupButtons ? BST_CHECKED : BST_UNCHECKED);
        //CheckDlgButton(IDC_TASKBARPROP_SHOWQL, g_TaskbarSettings.bShowQuickLaunch ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_CLOCK, (!g_TaskbarSettings.sr.HideClock) ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_SECONDS, g_TaskbarSettings.bShowSeconds ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_TASKBARPROP_HIDEICONS, g_TaskbarSettings.bHideInactiveIcons ? BST_CHECKED : BST_UNCHECKED);

        UpdateDialog();
        return TRUE;
    }

    LRESULT OnCustomizeTrayIcons(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
    {
        ShowCustomizeNotifyIcons(hExplorerInstance, m_hWnd);
        return 0;
    }

    LRESULT OnCtrlCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
    {
        UpdateDialog();
        SetModified(TRUE);
        return 0;
    }

    int OnApply()
    {
        TaskbarSettings newSettings;
        memcpy(&newSettings, &g_TaskbarSettings, sizeof(TaskbarSettings));

        newSettings.bLock = IsDlgButtonChecked(IDC_TASKBARPROP_LOCK);
        newSettings.sr.AutoHide = IsDlgButtonChecked(IDC_TASKBARPROP_HIDE);
        newSettings.sr.AlwaysOnTop = IsDlgButtonChecked(IDC_TASKBARPROP_ONTOP);
        newSettings.bGroupButtons = IsDlgButtonChecked(IDC_TASKBARPROP_GROUP);
        //newSettings.bShowQuickLaunch = IsDlgButtonChecked(IDC_TASKBARPROP_SHOWQL);
        newSettings.sr.HideClock = !IsDlgButtonChecked(IDC_TASKBARPROP_CLOCK);
        newSettings.bShowSeconds = IsDlgButtonChecked(IDC_TASKBARPROP_SECONDS);
        newSettings.bHideInactiveIcons = IsDlgButtonChecked(IDC_TASKBARPROP_HIDEICONS);

        SendMessage(m_hwndTaskbar, TWM_SETTINGSCHANGED, 0, (LPARAM)&newSettings);

        return PSNRET_NOERROR;
    }
};

class CStartMenuSettingsPage : public CPropertyPageImpl<CStartMenuSettingsPage>
{
private: 
    HBITMAP m_hbmpStartBitmap;

    void UpdateDialog()
    {
        HWND hwndCustomizeClassic = GetDlgItem(IDC_TASKBARPROP_STARTMENUCLASSICCUST);
        HWND hwndCustomizeModern = GetDlgItem(IDC_TASKBARPROP_STARTMENUCUST);
        HWND hwndStartBitmap = GetDlgItem(IDC_TASKBARPROP_STARTMENU_BITMAP);
        HWND hwndModernRadioBtn = GetDlgItem(IDC_TASKBARPROP_STARTMENU);
        HWND hwndModernText = GetDlgItem(IDC_TASKBARPROP_STARTMENUMODERNTEXT);
        BOOL policyNoSimpleStartMenu = SHRestricted(REST_NOSTARTPANEL) != 0;
        BOOL bModern = FALSE;

        /* If NoSimpleStartMenu, disable ability to use Modern Start Menu */
        if (policyNoSimpleStartMenu)
        {
            /* Switch to classic */
            CheckDlgButton(IDC_TASKBARPROP_STARTMENUCLASSIC, BST_CHECKED);

            /* Disable radio button */
            ::EnableWindow(hwndModernRadioBtn, FALSE);

            /* Hide controls related to modern menu */
            ::ShowWindow(hwndModernRadioBtn, SW_HIDE);
            ::ShowWindow(hwndModernText, SW_HIDE);
            ::ShowWindow(hwndCustomizeModern, SW_HIDE);
        }
        /* If no restrictions, then get bModern from dialog */
        else
        {
            bModern = IsDlgButtonChecked(IDC_TASKBARPROP_STARTMENU);
        }

        ::EnableWindow(hwndCustomizeModern, bModern);
        ::EnableWindow(hwndCustomizeClassic, !bModern);

        UINT uImageId = bModern ? IDB_STARTPREVIEW : IDB_STARTPREVIEW_CLASSIC;
        SetBitmap(hwndStartBitmap, &m_hbmpStartBitmap, uImageId);
    }

public:
    enum { IDD = IDD_TASKBARPROP_STARTMENU };

    BEGIN_MSG_MAP(CTaskBarSettingsPage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDC_TASKBARPROP_STARTMENUCLASSICCUST, OnStartMenuCustomize)
        CHAIN_MSG_MAP(CPropertyPageImpl<CStartMenuSettingsPage>)
    END_MSG_MAP()

    CStartMenuSettingsPage():
        m_hbmpStartBitmap(NULL)
    {
    }

    ~CStartMenuSettingsPage()
    {
        if (m_hbmpStartBitmap)
            DeleteObject(m_hbmpStartBitmap);
    }

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        // fix me: start menu style (classic/modern) should be read somewhere from the registry.
        CheckDlgButton(IDC_TASKBARPROP_STARTMENUCLASSIC, BST_CHECKED); // HACK: This has to be read from registry!!!!!!!
        UpdateDialog();
    
        return TRUE;
    }

    LRESULT OnStartMenuCustomize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
    {
        ShowCustomizeClassic(hExplorerInstance, m_hWnd);
        return 0;
    }

    int OnApply()
    {
        //TODO
        return PSNRET_NOERROR;
    }
};
 
 /**
*    Autor:      Jose Carlos Jesus 
*    email:      zecarlos1957@hotmail.com
*    Project:    ReactOS  
*    date:       12-08-2019   
*    input - 
*               LPARAM lParam : Pointer to Resource Dialog TEMPLATE 
*               DWORD id :       Resource ID from object to be extracted
*                                  id = 0 -> Dialog rectangle
*                                  id = # -> Object rectangle
*               LPVOID data:    Pointer to buffer that receive data
*
*    Output    return pointer to RECT object or NULL if ID not found
*                     If object not found, show a Dialog with Title, ClassName, ResourceID,
*                     and Rectangle structure from all objects inside this Resource
*
*      (NOTE) For now, just work with DLGTEMPLATEEX resource type
*/

WORD *TEMPLATE_ExtractRect(LPARAM lParam, DWORD id, LPVOID data)
{
    WCHAR buf[1024];
    WCHAR *aux = buf;
    WORD _id;
    WORD len;
    RECT rc;
    WORD *ptr = reinterpret_cast<WORD*>(lParam);
    DWORD signature = *((DWORD*)ptr);

    if(signature != 0xffff0001)
        return NULL;

    ((*ptr == 1) && (*(ptr + 1) == 0xffff)) ? ptr += 7 : ptr += 4;
    BOOL font = ((*ptr++ & DS_SETFONT) != 0) ;
    WORD nItems = *ptr++;

    rc.left = *ptr;
    rc.top = *(ptr + 1);
    rc.right = *(ptr + 2);
    rc.bottom = *(ptr + 3);

    if(id == 0)
    {
        ((RECT *)data)->left = *ptr;
        ((RECT *)data)->top = *(ptr + 1);
        ((RECT *)data)->right = *(ptr + 2);
        ((RECT *)data)->bottom = *(ptr + 3);
        return ptr;
    }

    ptr += 4;

    if (*ptr == 0)               /// No menu id
        ptr++;           
    else if (*ptr == 0xffff)     /// menu # id
        ptr += 2;        
    else 
        ptr += (lstrlenW((WCHAR*)ptr) + 1);

    if (*ptr == 0)               /// Default Dialog ClassName id
        ptr++;  
    else if(*ptr == 0xffff)      /// ClassName # id
        ptr += 2;   
    else                         /// Unicode ClassName
        ptr += (lstrlenW((WCHAR*)ptr) + 1);

    if (*ptr == 0)               /// No WindowTitle
        ptr++;
    else                         /// Unicode WindowTitle
    {
        lstrcpyW(aux, (WCHAR*)ptr);
        int n = lstrlenW((WCHAR*)ptr);
        wsprintfW(aux + n,L" Rect(%d, %d, %d, %d)\n", rc.left, rc.top, rc.right, rc.bottom);
        aux += lstrlenW(aux);
        ptr += (n + 1);
    }

    if (font)
    {
        ptr += 3;
        wsprintfW(aux, L"%s\n", (WCHAR*)ptr);
        len = lstrlenW((WCHAR*)ptr);
        aux += (len + 1);
        ptr += (len + 1);
    }

    for (WORD i = 0; i < nItems; i++)
    {
        if ((DWORD)ptr % 4) ptr++;  /// aligned DWORD

        ptr += 6;

        rc.left = *ptr;
        rc.top = *(ptr + 1);
        rc.right = *(ptr + 2);
        rc.bottom = *(ptr + 3);

        _id = *(ptr + 4);
        if (id == _id)
        {
            ((RECT*)data)->left = rc.left;
            ((RECT*)data)->top = rc.top;
            ((RECT*)data)->right = rc.right;
            ((RECT*)data)->bottom = rc.bottom;
            return ptr;
        }
        ptr += 6;
        if (*ptr == 0xffff)
        {
            wsprintfW(aux, L" Class #0x%x ", *(ptr + 1));
            aux += lstrlenW(aux);
            ptr += 2;
        }
        else
        {
            wsprintfW(aux, L" Class %s ",  ptr);
            aux += lstrlenW(aux);
            ptr += (lstrlenW((WCHAR*)ptr) + 1);
        }

        if(*ptr == 0xffff)
        {
            wsprintfW(aux, L" #0x%x _ID %d Rect(%d, %d, %d, %d)\n", *(ptr+1), _id,rc.left,rc.top, rc.right, rc.bottom);
            aux += lstrlenW(aux);
            ptr += 2;
        }
        else
        {
            wsprintfW(aux,L" %s ID. %d Rect(%d, %d, %d, %d)\n",(WCHAR*)ptr, _id,rc.left,rc.top, rc.right, rc.bottom);
            aux += lstrlenW(aux);
            ptr += (lstrlenW((WCHAR*)ptr )+ 1);
        }
        if (*ptr)
            ptr += *ptr;
        else
            ptr++;
   }
   MessageBoxW(0, buf, L"ID not found", MB_OK|MB_ICONEXCLAMATION);
   return NULL;
}

#define IDC_TABCONTROL 12320

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_PRECREATE:
        {
            RECT rc, aux;
            HRSRC hRes;
            LPVOID Template;
            APPBARDATA tbData;

            if ((hRes = FindResourceW(NULL, MAKEINTRESOURCE(IDD_TASKBARPROP_TASKBAR),
                                     (LPWSTR)RT_DIALOG)) != NULL)
                Template = LoadResource(NULL, hRes);
            else
                return 0;

            WORD *rcObject = TEMPLATE_ExtractRect(lParam, 0, &rc);
            if (rcObject == NULL) return FALSE;

            WORD oldH = rc.bottom;
            TEMPLATE_ExtractRect((LPARAM)Template, 0, &rc);
            TEMPLATE_ExtractRect(lParam, IDC_TABCONTROL, &aux);
            AdjustWindowRect(&rc, WS_POPUPWINDOW, 0); 
            
            tbData.cbSize = sizeof(APPBARDATA);
            tbData.hWnd = FindWindowW(L"Shell_TrayWnd", NULL);
            SHAppBarMessage(ABM_GETTASKBARPOS, &tbData);

            WORD tbHeight = tbData.rc.bottom - tbData.rc.top;
            tbHeight = (tbHeight * 8) / HIWORD(GetDialogBaseUnits());

            *(rcObject + 2) = rc.right;
            *(rcObject + 3) = rc.bottom + (oldH - aux.bottom) + tbHeight;

            return 0;
        }
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hExplorerInstance, MAKEINTRESOURCEW(IDI_STARTMENU));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

VOID
DisplayTrayProperties(IN HWND hwndOwner, IN HWND hwndTaskbar)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hpsp[2];
    CTaskBarSettingsPage tbSettingsPage(hwndTaskbar);
    CStartMenuSettingsPage smSettingsPage;
    CStringW caption;

    caption.LoadStringW(IDS_TASKBAR_STARTMENU_PROP_CAPTION);

    hpsp[0] = tbSettingsPage.Create();
    hpsp[1] = smSettingsPage.Create();

    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags =  PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hExplorerInstance;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_STARTMENU);
    psh.pszCaption = caption.GetString();
    psh.nPages = _countof(hpsp);
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    PropertySheet(&psh);
}
