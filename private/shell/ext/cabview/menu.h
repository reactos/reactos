//*******************************************************************************************
//
// Filename : Menu.h
//	
//				CCabItemMenu
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _MENU_H_
#define _MENU_H_

#include "folder.h"

//*****************************************************************************
//
// CCabItemMenu
//
// Purpose:
//
//        IContextMenu for the Shellfolder
//
//*****************************************************************************
class CCabItemMenu : public IContextMenu
{
public:
	CCabItemMenu(HWND hwndOwner, CCabFolder*pcf, LPCABITEM *apit, UINT cpit);
	~CCabItemMenu();

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHODIMP InvokeCommand(
                             LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(
                                UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax);

private:
	static HMENU LoadPopupMenu(UINT id, UINT uSubMenu);

	static HGLOBAL * CALLBACK CCabItemMenu::ShouldExtract(LPCTSTR pszFile, DWORD dwSize,
		UINT date, UINT time, UINT attribs, LPARAM lParam);

private:
	CRefCount m_cRef;

	CRefDll m_cRefDll;

	HWND m_hwndOwner;
	CCabFolder *m_pcfHere;

	CCabItemList m_lSel;
} ;

#endif // _MENU_H_
