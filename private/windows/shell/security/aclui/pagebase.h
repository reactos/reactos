//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pagebase.h
//
//  This file contains the definition of the CSecurityPage base class
//
//--------------------------------------------------------------------------

#ifndef _PAGEBASE_H_
#define _PAGEBASE_H_

class CSecurityPage
{
protected:
    SI_PAGE_TYPE    m_siPageType;
    LPSECURITYINFO  m_psi;
    LPSECURITYINFO2 m_psi2;
    HRESULT         m_hrComInit;
    IDsObjectPicker *m_pObjectPicker;
    SI_OBJECT_INFO  m_siObjectInfo;
    DWORD           m_flLastOPOptions;
    BOOL            m_bStandalone;
    BOOL            m_bAbortPage;
    HRESULT         m_hrLastPSPCallbackResult;

public:
    CSecurityPage( LPSECURITYINFO psi, SI_PAGE_TYPE siType );
    virtual ~CSecurityPage( void );

    HPROPSHEETPAGE CreatePropSheetPage(LPCTSTR pszDlgTemplate, LPCTSTR pszDlgTitle = NULL);
    HRESULT GetObjectPicker(IDsObjectPicker **ppObjectPicker = NULL);
    HRESULT GetUserGroup(HWND hDlg, BOOL bMultiSelect, PUSER_LIST *ppUserList);

protected:
#if(_WIN32_WINNT >= 0x0500)
    HRESULT InitObjectPicker(BOOL bMultiSelect);
#endif

    virtual BOOL DlgProc(HWND, UINT, WPARAM, LPARAM) = 0;
    virtual UINT PSPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

    static INT_PTR CALLBACK _DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT CALLBACK _PSPageCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
};

typedef class CSecurityPage *LPSECURITYPAGE;

#endif  /* _PAGEBASE_H_ */
