// clsgenpg.h : header file
//

#ifndef __CLSGENPG_H__
#define __CLSGENPG_H__

/*++

Copyright (C) 1997-  Microsoft Corporation

Module Name:

    clsgenpg.h

Abstract:

    header file for clsgenpg.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "proppage.h"

#define IDH_DISABLEHELP	(DWORD(-1))

class CClassGeneralPage : public CPropSheetPage
{
public:
    CClassGeneralPage() : m_pClass(NULL),
			  CPropSheetPage(g_hInstance, IDD_CLSGEN_PAGE)
	{}
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
    virtual void UpdateControls(LPARAM lParam = 0);
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
    virtual UINT DestroyCallback();

    HPROPSHEETPAGE Create(CClass* pClass);

private:
    CClass* m_pClass;
};

#endif // __CLSGENPG_H__
