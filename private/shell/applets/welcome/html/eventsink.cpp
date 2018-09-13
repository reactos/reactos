//////////////////////////////////////////////////////////////////////////
//
//  EventSink.cpp
//
//      This file contains the implementation of the event sink.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "webapp.h"

/**
 *  This method is the constructor for the CEventSink object. 
 */
CEventSink::CEventSink(CWebApp *pApp)
{
    m_cRefs     = 1;
    m_pApp    = pApp;
}

/**
 *  This method is called when the caller wants an interface pointer.
 *
 *  @param      riid        The interface being requested.
 *  @param      ppvObject   The resultant object pointer.
 *
 *  @return     HRESULT     S_OK, E_POINTER, E_NOINTERFACE
 */
STDMETHODIMP CEventSink::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IDispatch))
        *ppvObject = (IDispatch *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

/**
 *  This method increments the current object count.
 *
 *  @return     ULONG       The new reference count.
 */
ULONG CEventSink::AddRef(void)
{
    return ++m_cRefs;
}

/**
 *  This method decrements the object count and deletes if necessary.
 *
 *  @return     ULONG       Remaining ref count.
 */
ULONG CEventSink::Release(void)
{
    if (--m_cRefs)
        return m_cRefs;

    delete this;
    return 0;
}

// ***********************************************************************
//  IDispatch
// ***********************************************************************

HRESULT CEventSink::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)
{
    *rgdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

HRESULT CEventSink::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

HRESULT CEventSink::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    return E_NOTIMPL;
}

HRESULT CEventSink::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr)
{
    if (!m_pApp)
        return S_OK;

    switch (dispid)
    {
        // void StatusTextChange([in] BSTR Text);
        case 0x66:
            break;

        // void ProgressChange([in] long Progress, [in] long ProgressMax);
        case 0x6c:
            break;

        // void CommandStateChange([in] long Command, [in] VARIANT_BOOL Enable);
        case 0x69:
            break;

        // void DownloadBegin();
        case 0x6a:
            break;

        // void DownloadComplete();
        case 0x68:
            break;
            
        // void TitleChange([in] BSTR Text);
        case 0x071:            
            break;

        // void PropertyChange([in] BSTR szProperty);
        case 0x70:
            break;

        // void BeforeNavigate2([in] IDispatch* pDisp, [in] VARIANT* URL, [in] VARIANT* Flags, [in] VARIANT* TargetFrameName, [in] VARIANT* PostData, [in] VARIANT* Headers, [in, out] VARIANT_BOOL* Cancel);
        case 0xfa:
            m_pApp->eventBeforeNavigate2(
                pdispparams->rgvarg[5].pvarVal,     // [in] VARIANT* URL
                pdispparams->rgvarg[0].pboolVal );  // [in, out] VARIANT_BOOL* Cancel
            break;

        // void NewWindow2([in, out] IDispatch** ppDisp, [in, out] VARIANT_BOOL* Cancel);
        case 0xfb:
            break;
        
        // void NavigateComplete2([in] IDispatch* pDisp, [in] VARIANT* URL);
        case 0xfc:
            break;

        // void DocumentComplete([in] IDispatch* pDisp, [in] VARIANT* URL);
        case 0x0103:
            m_pApp->eventDocumentComplete(
                pdispparams->rgvarg[0].pvarVal );   // [in] VARIANT* URL
            break;

        // void OnQuit();
        case 0xfd:
            break;

        // void OnVisible([in] VARIANT_BOOL Visible);
        case 0xfe:
            break;

        // void OnToolBar([in] VARIANT_BOOL ToolBar);
        case 0xff:
            break;

        // void OnMenuBar([in] VARIANT_BOOL MenuBar);
        case 0x0100:
            break;

        // void OnStatusBar([in] VARIANT_BOOL StatusBar);
        case 0x0101:
            break;
            
        // void OnFullScreen([in] VARIANT_BOOL FullScreen);
        case 0x0102:
            break;
            
        // void OnTheaterMode([in] VARIANT_BOOL TheaterMode);
        case 0x0104:
            break;
    }

    return S_OK;
}
