//******************************************************************
// layout.h
//
// This file contains the declarations for the code hat lays out the 
// CTrapEventDialog. This is neccessary when the edit/view button changes the 
// dialog form its small (main) view to the extended view.
//
// Author: Larry A. French
//
// History:
//      20-Febuary-96  Wrote it
//
//
// Copyright (C) 1996 Microsoft Corporation.  All rights reserved.
//******************************************************************

#ifndef _layout_h
#define _layout_h

class CEventTrapDlg;
class CMainLayout;
class CExtendedLayout;

class CLayout
{
public:
    CLayout();
    void Initialize(CEventTrapDlg* pdlg);
	void LayoutAndRedraw(BOOL bExtendedView, int cx, int cy);
	void ShowExtendedView(BOOL bShow);
    void LayoutView(BOOL bExtendedView);

private:
    // Private member functions
	void ResizeMainLayout(CMainLayout& layoutMain);
	void ResizeExtendedLayout(CExtendedLayout& layoutExtended);

    // Private member data
    CEventTrapDlg* m_pdlg;
	CSize m_sizeMainViewInitial;
	CSize m_sizeExtendedViewInitial;
	int m_cyMainView;
	int m_cyExtendedView;
};

#endif //_layout_h
