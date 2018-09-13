// genpage.h : header file
//

#ifndef __GENPAGE_H__
#define __GENPAGE_H__
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devgenpg.h

Abstract:

    header file for genpage.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "proppage.h"

//
// context help id
//
#define IDH_DISABLEHELP (DWORD(-1))
#define idh_devmgr_manage_command_line  102170  // Device Manager: "Allo&w the selected computer to be changed when launching from the command line.  This only applies if you save the console." (Button)
#define idh_devmgr_view_devicetree  102110  // Device Manager: "&Device tree" (Button)
#define idh_devmgr_manage_local 102130  // Device Manager: "&Local computer:  (the computer this console is running on)" (Button)
#define idh_devmgr_manage_remote    102140  // Device Manager: "&Another computer:" (Button)
#define idh_devmgr_manage_remote_name   102150  // Device Manager: "" (Edit)
#define idh_devmgr_view_all 102100  // Device Manager: "&All" (Button)
#define idh_devmgr_manage_remote_browse 102160  // Device Manager: "B&rowse..." (Button)
#define idh_devmgr_view_resources   102120  // Device Manager: "&Resources" (Button)


class CGeneralPage : public CPropSheetPage
{
public:
    CGeneralPage();
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnReset();
    virtual BOOL OnWizFinish();
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
    HPROPSHEETPAGE Create(LONG_PTR lConsoleHandle);
    void SetOutputBuffer(String* pstrMachineName, COOKIE_TYPE* pct)
    {
        m_pstrMachineName = pstrMachineName;
        m_pct = pct;
    }
    void DoBrowse();
private:
    LONG_PTR m_lConsoleHandle;
    TCHAR   m_MachineName[MAX_COMPUTERNAME_LENGTH + 3];
    COOKIE_TYPE m_ct;
    String* m_pstrMachineName;
    COOKIE_TYPE* m_pct;
    BOOL    m_IsLocalMachine;
};

#endif  // __GENPAGE_H__
