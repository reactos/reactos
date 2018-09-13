/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pubquery.h

Abstract:

    This module is the include file for the published app query form.

Author:

    Dave Hastings (daveh) creation-date 08-Nov-1997

Revision History:

--*/
#ifndef _pubform_h_
#define _pubform_h_

#include <windows.h>
#include <ole2.h>
#include <commctrl.h>
#include <shlobj.h>
#include <cmnquery.h>
#include <cmnquryp.h>
#include <shsemip.h>

#include <guid.h>
#include "guidp.h"
#include "resource.h"


typedef struct
{
    const IID* piid;            // interface ID
    LPVOID  pvObject;           // pointer to the object
} INTERFACES, * LPINTERFACES;

typedef struct _QueryParameters {
    LPWSTR ApplicationName;
    LPWSTR ApplicationCategory;
} QUERYPARAMETERS, *PQUERYPARAMETERS;

class CPublishedApplicationQueryFormClassFactory : public IClassFactory
{
public:

    //
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvInterface);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    //
    // IClassFactory
    //
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, PVOID *ppvInterface);
    STDMETHOD(LockServer)(BOOL fLock);

    //
    // creator/destructor
    //
    CPublishedApplicationQueryFormClassFactory();
    ~CPublishedApplicationQueryFormClassFactory();

private:

    LONG m_RefCount;
};

class CPublishedApplicationQueryForm : public IQueryForm
{
public:

    //
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvInterface);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    //
    // IQueryForm
    //
    STDMETHOD(Initialize)(THIS_ HKEY hkForm);
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

    //
    // creator/destructor
    //
    CPublishedApplicationQueryForm(LPUNKNOWN IUnknown);
    ~CPublishedApplicationQueryForm();

private:
    LONG m_RefCount;
    LPUNKNOWN m_IUnknown;
};


class CPublishedApplicationQueryHandler : public IQueryHandler
{
public:

    //
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvInterface);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    //
    // IQueryHandler
    //
    STDMETHOD(Initialize)(THIS_ IQueryFrame *QueryFrame, DWORD Flags, LPVOID Parameters);
    STDMETHOD(GetViewInfo)(THIS_ LPCQVIEWINFO ViewInfo);
    STDMETHOD(AddScopes)(THIS);
    STDMETHOD(BrowseForScope)(THIS_ HWND Parent, LPCQSCOPE CurrentScope, LPCQSCOPE* ppScope);
    STDMETHOD(CreateResultView)(THIS_ HWND hwnDParent, HWND* phWndView);
    STDMETHOD(ActivateView)(THIS_ UINT State, WPARAM wParam, LPARAM lParam);
    STDMETHOD(InvokeCommand)(THIS_ HWND hwndParent, UINT idCmd);
    STDMETHOD(GetCommandString)(THIS_ UINT idCmd, DWORD dwFlags, LPTSTR pBuffer, INT cchBuffer);
    STDMETHOD(IssueQuery)(THIS_ LPCQPARAMS pQueryParams);
    STDMETHOD(StopQuery)(THIS);
    STDMETHOD(GetViewObject)(THIS_ UINT uScope, REFIID riid, LPVOID* ppvOut);
    STDMETHOD(LoadQuery)(THIS_ IPersistQuery* pPersistQuery);
    STDMETHOD(SaveQuery)(THIS_ IPersistQuery* pPersistQuery, LPCQSCOPE pScope);

    //
    // creator/destructor
    //
    CPublishedApplicationQueryHandler(LPUNKNOWN IUnknown);
    ~CPublishedApplicationQueryHandler();

    //
    // Internal methods
    //
    LRESULT OnSize(INT Width, INT Height);

private:
    LONG m_RefCount;
    LPUNKNOWN m_IUnknown;

    DWORD m_Flags;
    IQueryFrame *m_QueryFrame;
    HWND m_ResultView;
    HWND m_ListView;
    HWND m_Add;
    HWND m_About;
    HMENU m_FileMenu;
    HMENU m_MenuBar;
    BOOL m_ItemSelected;
};


class CPublishedApplicationQuery : public IUnknown
{
public:

    //
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvInterface);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    //
    // constructor/destructor
    //
    CPublishedApplicationQuery();
    ~CPublishedApplicationQuery();

private:
    LONG m_RefCount;
    CPublishedApplicationQueryForm *m_QueryForm;
    CPublishedApplicationQueryHandler *m_QueryHandler;
};

extern LONG g_RefCount;
extern HINSTANCE Instance;

#include "util.h"

#endif