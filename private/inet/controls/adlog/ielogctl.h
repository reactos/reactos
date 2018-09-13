// IeLogCtl.h : Declaration of the CIeLogControl

#ifndef __IELOGCONTROL_H_
#define __IELOGCONTROL_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CIeLogControl
class ATL_NO_VTABLE CIeLogControl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CIeLogControl, &CLSID_IeLogControl>,
    public CComControl<CIeLogControl>,
    public IDispatchImpl<IIeLogControl, &IID_IIeLogControl, &LIBID_IMGLOGLib>,
    public IQuickActivateImpl<CIeLogControl>,
    public IOleControlImpl<CIeLogControl>,
    public IOleObjectImpl<CIeLogControl>,
    public IOleInPlaceActiveObjectImpl<CIeLogControl>,
    public IViewObjectExImpl<CIeLogControl>,
    public IOleInPlaceObjectWindowlessImpl<CIeLogControl>,
    public ISupportErrorInfo,
    public IPersistPropertyBagImpl<CIeLogControl>,
    public IObjectSafetyImpl<CIeLogControl>
{
public:
    CIeLogControl()
    {
        m_nImages = 0;
        m_lsize = 0;

        m_pszURL = NULL;
        m_ImageList = NULL;
        GetLocalTime(&m_timeEnter);
    }

    ~CIeLogControl()
    {
        SaveLogs();
        Cleanup();

    }

    typedef enum {
        IELOG_EVENT_MOUSECLICK = 1,
        IELOG_EVENT_MOUSEOVER = 2
    } IELOG_EVENTS;

    typedef struct tagIELOGIMAGELIST {
        struct tagIELOGIMAGELIST* next;
        LPSTR    key;
        LPSTR    value;
        IELOG_EVENTS    evts;
    } IELog_ImageList;

DECLARE_REGISTRY_RESOURCEID(IDR_IELOGCONTROL)

BEGIN_COM_MAP(CIeLogControl)
    COM_INTERFACE_ENTRY(IIeLogControl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CIeLogControl)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CIeLogControl)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()

// IOleObject
    STDMETHOD(Close)(DWORD dwSaveOption);

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

// IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
    {
        ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
        if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
            return E_POINTER;
        HRESULT hr = S_OK;
        if (riid == IID_IDispatch)
        {
            *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
            *pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        }
        else if ( riid == IID_IPersistPropertyBag )
        {
            *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
            *pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_DATA;
        }
        else
        {
            *pdwSupportedOptions = 0;
            *pdwEnabledOptions = 0;
            hr = E_NOINTERFACE;
        }
        return hr;
    }
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
    {
        ATLTRACE(_T("IObjectSafetyImpl::SetInterfaceSafetyOptions\n"));
        // If we're being asked to set our safe for scripting option then oblige
        if (riid == IID_IDispatch ||
            riid == IID_IPersistPropertyBag )
        {
            // Store our current safety level to return in GetInterfaceSafetyOptions
            m_dwSafety = dwEnabledOptions & dwOptionSetMask;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

//IPersistPropertyBag
    STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

// IIeLogControl
public:
    STDMETHOD(get_images)(/*[out, retval]*/ short *pVal);
    STDMETHOD(put_images)(/*[in]*/ short newVal);
    STDMETHOD(put_msover)(/*[in]*/ BSTR bstrVal);
    STDMETHOD(put_msclick)(/*[in]*/ BSTR bstrVal);
    STDMETHOD(put_URL)(/*[in]*/ BSTR newVal);

    STDMETHOD(ImageList)(BSTR bstrKey, BSTR bstrVal);
    HRESULT OnDraw(ATL_DRAWINFO& di);

private:
    HRESULT UpdateMouseEventOnObject(LPTSTR psz, IELOG_EVENTS ievent);
    HRESULT GetURLFromMoniker(void);
    HRESULT BuildImageList(LPTSTR key, LPTSTR value);
    HRESULT SaveLogs(void);
    HRESULT put_URL(LPSTR newVal);
    void    Cleanup();

    short           m_nImages;
    long            m_lsize;
    LPTSTR            m_pszURL;
    IELog_ImageList    *m_ImageList;
    SYSTEMTIME        m_timeEnter;
};

#endif //__IELOGCONTROL_H_
