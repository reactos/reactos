// DHTMLEdit.h : Declaration of the CDHTMLEdit
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __DHTMLEDIT_H_
#define __DHTMLEDIT_H_

#include "resource.h"       // main symbols
#include "CPDHTMLEd.h"
#include "triedtctlid.h"
#include "private.h"
#include "wininet.h"
#include "trixacc.h"


class CProxyFrame;
class CEventXferSink;

typedef /* [helpstring][uuid] */ 
enum TriEditFilterConstants
    {	filterNone	= 0x1,
		filterDTCs	= 0x2,
		filterASP	= 0x8,
		preserveSourceCode	= 0x10,
		filterSourceCode	= 0x20,
		filterAll	= filterDTCs | filterASP | preserveSourceCode
    }	TriEditCtlFilterConstants;

#define DOCUMENT_COMPETE_MESSAGE	(WM_USER+18)
#define DOCUMENT_COMPETE_SIGNATURE	0xADFE


//	ATL 3.0 macro for overriding the window class settings.
#ifndef DECLARE_WND_CLASS_EX
#define DECLARE_WND_CLASS_EX(WndClassName, style, bkgnd) \
static CWndClassInfo& GetWndClassInfo() \
{ \
	static CWndClassInfo wc = \
	{ \
		{ sizeof(WNDCLASSEX), style, StartWindowProc, \
		  0, 0, NULL, NULL, NULL, (HBRUSH)(bkgnd + 1), NULL, WndClassName, NULL }, \
		NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
	}; \
	return wc; \
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDHTMLSafe
// This is the Safe for Scripting control, which is complete and stand-alone,
// but aggregated by DHTMLEdit to provide Load and Save capabilities.
//
class ATL_NO_VTABLE CDHTMLSafe : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDHTMLSafe, &CLSID_DHTMLSafe>,
	public CComControl<CDHTMLSafe>,
	public IDispatchImpl<IDHTMLSafe, &IID_IDHTMLSafe, &LIBID_DHTMLEDLib>,
	public IProvideClassInfo2Impl<&CLSID_DHTMLSafe, &DIID__DHTMLSafeEvents, &LIBID_DHTMLEDLib>,
	public IPersistStreamInitImpl<CDHTMLSafe>,
	public IPersistStorageImpl<CDHTMLSafe>,
	public IPersistPropertyBagImpl<CDHTMLSafe>,
	public IOleControlImpl<CDHTMLSafe>,
	public IOleObjectImpl<CDHTMLSafe>,
	public IOleInPlaceActiveObjectImpl<CDHTMLSafe>,
	public IViewObjectExImpl<CDHTMLSafe>,
	public IOleInPlaceObjectWindowlessImpl<CDHTMLSafe>,
	public IDataObjectImpl<CDHTMLSafe>,
	public ISpecifyPropertyPagesImpl<CDHTMLSafe>,
	public CProxy_DHTMLSafeEvents<CDHTMLSafe>,
	public IPropertyNotifySinkCP<CDHTMLSafe>,
	public IConnectionPointContainerImpl<CDHTMLSafe>,
	public IInterconnector
{
public:

	DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS, (HBRUSH)(COLOR_WINDOW+1))

	CDHTMLSafe();
	~CDHTMLSafe();

	BOOL IsUserMode() {
		HRESULT hr = S_OK;		
		BOOL bVal = FALSE;

		if (FAILED(hr = GetAmbientUserMode(bVal)))
			bVal = FALSE;
		
		return (bVal) ? TRUE : FALSE;
	};


DECLARE_REGISTRY_RESOURCEID(IDR_DHTMLSafe)
DECLARE_AGGREGATABLE(CDHTMLSafe)

BEGIN_COM_MAP(CDHTMLSafe)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IDHTMLSafe)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
	COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
	COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY_IMPL(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IInterconnector)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDHTMLSafe)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	PROP_ENTRY("ActivateApplets",				DISPID_ACTIVATEAPPLETS,			CLSID_NULL)
	PROP_ENTRY("ActivateActiveXControls",		DISPID_ACTIVATEACTIVEXCONTROLS, CLSID_NULL)
	PROP_ENTRY("ActivateDTCs",					DISPID_ACTIVATEDTCS,			CLSID_NULL)
	PROP_ENTRY("ShowDetails",					DISPID_SHOWDETAILS,				CLSID_NULL)
	PROP_ENTRY("ShowBorders",					DISPID_SHOWBORDERS,				CLSID_NULL)
	PROP_ENTRY("Appearance",					DISPID_DHTMLEDITAPPEARANCE,		CLSID_NULL)
	PROP_ENTRY("Scrollbars",					DISPID_DHTMLEDITSCROLLBARS,		CLSID_NULL)
	PROP_ENTRY("ScrollbarAppearance",			DISPID_SCROLLBARAPPEARANCE,		CLSID_NULL)
	PROP_ENTRY("SourceCodePreservation",		DISPID_SOURCECODEPRESERVATION,	CLSID_NULL)
	PROP_ENTRY("AbsoluteDropMode",				DISPID_ABSOLUTEDROPMODE,		CLSID_NULL)
	PROP_ENTRY("SnapToGrid",					DISPID_SNAPTOGRID,				CLSID_NULL)
	PROP_ENTRY("SnapToGridX",					DISPID_SNAPTOGRIDX,				CLSID_NULL)
	PROP_ENTRY("SnapToGridY",					DISPID_SNAPTOGRIDY,				CLSID_NULL)
	PROP_ENTRY("UseDivOnCarriageReturn",		DISPID_USEDIVONCR,				CLSID_NULL)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CDHTMLSafe)
	CONNECTION_POINT_ENTRY(DIID__DHTMLSafeEvents)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CDHTMLSafe)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SHOWWINDOW, OnShow)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(DOCUMENT_COMPETE_MESSAGE, OnDocumentComplete)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
		return S_OK;
	}

// IDHTMLSafe
public:
	STDMETHOD(get_IsDirty)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_SourceCodePreservation)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_SourceCodePreservation)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_ScrollbarAppearance)(/*[out, retval]*/ DHTMLEDITAPPEARANCE *pVal);
	STDMETHOD(put_ScrollbarAppearance)(/*[in]*/ DHTMLEDITAPPEARANCE newVal);

	STDMETHOD(get_Scrollbars)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Scrollbars)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_Appearance)(/*[out, retval]*/ DHTMLEDITAPPEARANCE *pVal);
	STDMETHOD(put_Appearance)(/*[in]*/ DHTMLEDITAPPEARANCE newVal);

	STDMETHOD(get_ShowBorders)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowBorders)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ShowDetails)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowDetails)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_ActivateDTCs)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateDTCs)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ActivateActiveXControls)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateActiveXControls)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ActivateApplets)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateApplets)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_DOM)(/*[out, retval]*/ IHTMLDocument2 **pVal);
	STDMETHOD(get_DocumentHTML)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_DocumentHTML)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_AbsoluteDropMode)(/* [retval][out] */ VARIANT_BOOL *pVal);
    STDMETHOD(put_AbsoluteDropMode)(/* [in] */ VARIANT_BOOL newVal);

    STDMETHOD(get_SnapToGridX)(/* [retval][out] */ LONG  *pVal);        
    STDMETHOD(put_SnapToGridX)(/* [in] */ LONG newVal);

    STDMETHOD(get_SnapToGridY)(/* [retval][out] */ LONG  *pVal);
    STDMETHOD(put_SnapToGridY)(/* [in] */ LONG newVal);

    STDMETHOD(get_SnapToGrid)(/* [retval][out] */ VARIANT_BOOL  *pVal);
    STDMETHOD(put_SnapToGrid)(/* [in] */ VARIANT_BOOL newVal);

    STDMETHOD(get_CurrentDocumentPath)(/* [retval][out] */ BSTR  *pVal);

    STDMETHOD(get_BaseURL)(/* [retval][out] */ BSTR  *baseURL);
    STDMETHOD(put_BaseURL)(/* [in] */ BSTR baseURL);
    STDMETHOD(get_DocumentTitle)(/* [retval][out] */ BSTR  *docTitle);

	STDMETHOD(get_UseDivOnCarriageReturn)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UseDivOnCarriageReturn)(/*[in]*/ VARIANT_BOOL newVal);

    STDMETHOD(get_Busy)(/* [retval][out] */ VARIANT_BOOL *pVal);

	STDMETHOD(ExecCommand)(DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt, LPVARIANT pInVar, LPVARIANT pOutVar);
	STDMETHOD(QueryStatus)(DHTMLEDITCMDID cmdID, /*[out, retval]*/ DHTMLEDITCMDF* retval);
	STDMETHOD(SetContextMenu)(/*[in]*/ LPVARIANT menuStrings, /*[in]*/ LPVARIANT menuStates);
	STDMETHOD(NewDocument)(void);
	STDMETHOD(LoadURL)(BSTR url);
	STDMETHOD(FilterSourceCode)(/*[in]*/ BSTR sourceCodeIn, /*[out, retval]*/ BSTR* sourceCodeOut);
	STDMETHOD(Refresh)(void);

	HRESULT OnDraw(ATL_DRAWINFO& di);
	LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	LRESULT OnShow(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	LRESULT OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	LRESULT OnDocumentComplete(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);
	STDMETHOD(TranslateAccelerator)(LPMSG lpmsg);
	STDMETHOD(OnAmbientPropertyChange)(DISPID dispID);
	STDMETHOD(OnMnemonic)(LPMSG pMsg);
	HRESULT IOleInPlaceObject_UIDeactivate ( void );

	// IOleObject
	STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);

	// IOleObjectImpl
	HRESULT IOleObject_SetExtent (DWORD dwDrawAspect, SIZEL *psizel);

	// IPersist overrides
	STDMETHOD(GetClassID)(CLSID *pClassID);

	// IPersistStreamInit override
	HRESULT IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap);
	HRESULT IPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY* pMap);
	
	BOOL IsPropertyBagLoading ( void ) { return m_bfPropertyBagLoading; }

	// IPersistPropertyBag override
	HRESULT IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap);
	HRESULT IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap);

	// IInterconnector
	STDMETHOD(GetInterconnector)( SIZE_T* vp ) { *vp = (SIZE_T)m_pFrame; return S_OK; }
	STDMETHOD(GetCtlWnd)( SIZE_T* vw ) { *vw = (SIZE_T)m_hWndCD; return S_OK; }
	STDMETHOD(MakeDirty)( DISPID dispid ) { SetDirty ( TRUE ); FireOnChanged ( dispid ); return S_OK; }

	DECLARE_GET_CONTROLLING_UNKNOWN();

	// ATL helper functions override

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	HRESULT FinalConstruct();
	void FinalRelease();

	void FocusSite ( BOOL bfGetFocus );

	IOleControlSite* GetControlSite ( void ) { return m_piControlSite; }

	HRESULT ControlQueryInterface(const IID& iid, void** ppv);
	BOOL	IsSafeForScripting ( void ) { return ( NULL == GetOuterEditControl () ); }

protected:
	HRESULT SpecialEdit ( DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt );

private:
	ATL_PROPMAP_ENTRY* ProperPropMap ( void );
	IDHTMLEdit* GetOuterEditControl ( void );

protected:

	class CProxyFrame*	m_pFrame;
	IOleControlSite*	m_piControlSite;
	BOOL				m_fJustCreated;
	IDHTMLEdit*			m_piOuterEditCtl;
	BOOL				m_bfPropertyBagLoading;
	BOOL				m_bfOuterEditUnknownTested;
};


/////////////////////////////////////////////////////////////////////////////
// CDHTMLEdit
//
class ATL_NO_VTABLE CDHTMLEdit : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDHTMLEdit, &CLSID_DHTMLEdit>,
	public IDispatchImpl<IDHTMLEdit, &IID_IDHTMLEdit, &LIBID_DHTMLEDLib>,
	public CProxy_DHTMLEditEvents<CDHTMLEdit>,
	public IProvideClassInfo2Impl<&CLSID_DHTMLEdit, &DIID__DHTMLEditEvents, &LIBID_DHTMLEDLib>,
	public IConnectionPointContainerImpl<CDHTMLEdit>,
	public IOleObject,
	public IServiceProvider,
	public IInternetSecurityManager
{
public:

	CDHTMLEdit();
	~CDHTMLEdit();

DECLARE_REGISTRY_RESOURCEID(IDR_DHTMLEDIT)
DECLARE_AGGREGATABLE(CDHTMLEdit)

BEGIN_COM_MAP(CDHTMLEdit)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IDHTMLEdit)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IServiceProvider)
	COM_INTERFACE_ENTRY(IInternetSecurityManager)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_punkInnerCtl)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDHTMLEdit)
	PROP_ENTRY("ActivateApplets",				DISPID_ACTIVATEAPPLETS,			CLSID_NULL)
	PROP_ENTRY("ActivateActiveXControls",		DISPID_ACTIVATEACTIVEXCONTROLS, CLSID_NULL)
	PROP_ENTRY("ActivateDTCs",					DISPID_ACTIVATEDTCS,			CLSID_NULL)
	PROP_ENTRY("ShowDetails",					DISPID_SHOWDETAILS,				CLSID_NULL)
	PROP_ENTRY("ShowBorders",					DISPID_SHOWBORDERS,				CLSID_NULL)
	PROP_ENTRY("Appearance",					DISPID_DHTMLEDITAPPEARANCE,		CLSID_NULL)
	PROP_ENTRY("Scrollbars",					DISPID_DHTMLEDITSCROLLBARS,		CLSID_NULL)
	PROP_ENTRY("ScrollbarAppearance",			DISPID_SCROLLBARAPPEARANCE,		CLSID_NULL)
	PROP_ENTRY("SourceCodePreservation",		DISPID_SOURCECODEPRESERVATION,	CLSID_NULL)
	PROP_ENTRY("AbsoluteDropMode",				DISPID_ABSOLUTEDROPMODE,		CLSID_NULL)
	PROP_ENTRY("SnapToGrid",					DISPID_SNAPTOGRID,				CLSID_NULL)
	PROP_ENTRY("SnapToGridX",					DISPID_SNAPTOGRIDX,				CLSID_NULL)
	PROP_ENTRY("SnapToGridY",					DISPID_SNAPTOGRIDY,				CLSID_NULL)
	PROP_ENTRY("BrowseMode",					DISPID_BROWSEMODE,				CLSID_NULL)
	PROP_ENTRY("UseDivOnCarriageReturn",		DISPID_USEDIVONCR,				CLSID_NULL)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CDHTMLEdit)
	CONNECTION_POINT_ENTRY(DIID__DHTMLEditEvents)
END_CONNECTION_POINT_MAP()


// IDHTMLEdit
public:
	STDMETHOD(get_IsDirty)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_SourceCodePreservation)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_SourceCodePreservation)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_ScrollbarAppearance)(/*[out, retval]*/ DHTMLEDITAPPEARANCE *pVal);
	STDMETHOD(put_ScrollbarAppearance)(/*[in]*/ DHTMLEDITAPPEARANCE newVal);

	STDMETHOD(get_Scrollbars)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Scrollbars)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_Appearance)(/*[out, retval]*/ DHTMLEDITAPPEARANCE *pVal);
	STDMETHOD(put_Appearance)(/*[in]*/ DHTMLEDITAPPEARANCE newVal);

	STDMETHOD(get_ShowBorders)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowBorders)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ShowDetails)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ShowDetails)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_ActivateDTCs)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateDTCs)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ActivateActiveXControls)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateActiveXControls)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ActivateApplets)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ActivateApplets)(/*[in]*/ VARIANT_BOOL newVal);

	STDMETHOD(get_DOM)(/*[out, retval]*/ IHTMLDocument2 **pVal);
	STDMETHOD(get_DocumentHTML)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_DocumentHTML)(/*[in]*/ BSTR newVal);

    STDMETHOD(get_AbsoluteDropMode)(/* [retval][out] */ VARIANT_BOOL *pVal);
    STDMETHOD(put_AbsoluteDropMode)(/* [in] */ VARIANT_BOOL newVal);

    STDMETHOD(get_SnapToGridX)(/* [retval][out] */ LONG  *pVal);        
    STDMETHOD(put_SnapToGridX)(/* [in] */ LONG newVal);

    STDMETHOD(get_SnapToGridY)(/* [retval][out] */ LONG  *pVal);
    STDMETHOD(put_SnapToGridY)(/* [in] */ LONG newVal);

    STDMETHOD(get_SnapToGrid)(/* [retval][out] */ VARIANT_BOOL  *pVal);
    STDMETHOD(put_SnapToGrid)(/* [in] */ VARIANT_BOOL newVal);

    STDMETHOD(get_CurrentDocumentPath)(/* [retval][out] */ BSTR  *pVal);

    STDMETHOD(get_BaseURL)(/* [retval][out] */ BSTR  *baseURL);
    STDMETHOD(put_BaseURL)(/* [in] */ BSTR baseURL);
    STDMETHOD(get_DocumentTitle)(/* [retval][out] */ BSTR  *docTitle);
    STDMETHOD(get_BrowseMode)(/* [retval][out] */ VARIANT_BOOL  *pVal);
    STDMETHOD(put_BrowseMode)(/* [in] */ VARIANT_BOOL newVal);

	STDMETHOD(get_UseDivOnCarriageReturn)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_UseDivOnCarriageReturn)(/*[in]*/ VARIANT_BOOL newVal);

    STDMETHOD(get_Busy)(/* [retval][out] */ VARIANT_BOOL *pVal);

	STDMETHOD(LoadDocument)(LPVARIANT path, LPVARIANT promptUser);
	STDMETHOD(SaveDocument)(LPVARIANT path, LPVARIANT promptUser);
	STDMETHOD(ExecCommand)(DHTMLEDITCMDID cmdID, OLECMDEXECOPT cmdexecopt, LPVARIANT pInVar, LPVARIANT pOutVar);
	STDMETHOD(QueryStatus)(DHTMLEDITCMDID cmdID, /*[out, retval]*/ DHTMLEDITCMDF* retval);
	STDMETHOD(SetContextMenu)(/*[in]*/ LPVARIANT menuStrings, /*[in]*/ LPVARIANT menuStates);
	STDMETHOD(NewDocument)(void);
	STDMETHOD(PrintDocument)(VARIANT* pvarWithUI);
	STDMETHOD(LoadURL)(BSTR url);
	STDMETHOD(FilterSourceCode)(/*[in]*/ BSTR sourceCodeIn, /*[out, retval]*/ BSTR* sourceCodeOut);
	STDMETHOD(Refresh)(void);

	// IOleObject
	STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);
	STDMETHOD(GetClientSite)(IOleClientSite **ppClientSite);
	STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
	STDMETHOD(Close)(DWORD dwSaveOption);
	STDMETHOD(SetMoniker)(DWORD dwWhichMoniker, IMoniker* pmk);
	STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk );
	STDMETHOD(InitFromData)(IDataObject*  pDataObject, BOOL fCreation, DWORD dwReserved);
	STDMETHOD(GetClipboardData)(DWORD dwReserved, IDataObject** ppDataObject);
	STDMETHOD(DoVerb)(LONG iVerb, LPMSG lpmsg, IOleClientSite* pActiveSite , LONG lindex , HWND hwndParent, LPCRECT lprcPosRect);
	STDMETHOD(EnumVerbs)(IEnumOLEVERB **ppEnumOleVerb);
	STDMETHOD(Update)(void);
	STDMETHOD(IsUpToDate)(void);
	STDMETHOD(GetUserClassID)(CLSID *pClsid);
	STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR *pszUserType);
	STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL *psizel);
	STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL *psizel);
	STDMETHOD(Advise)(IAdviseSink *pAdvSink, DWORD *pdwConnection);
	STDMETHOD(Unadvise)(DWORD dwConnection);
	STDMETHOD(EnumAdvise)(IEnumSTATDATA **ppenumAdvise);
	STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus);
	STDMETHOD(SetColorScheme)(LOGPALETTE* pLogpal);

	// IServiceProvider
	STDMETHODIMP QueryService( REFGUID guidService, REFIID riid, void** ppv );

	// IInternetSecurityManager
	STDMETHOD(GetSecurityId)(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved);
	STDMETHOD(GetSecuritySite)(IInternetSecurityMgrSite **ppSite);
	STDMETHOD(GetZoneMappings)(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags);
	STDMETHOD(MapUrlToZone)(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags);
	STDMETHOD(ProcessUrlAction)(LPCWSTR pwszUrl, DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy, BYTE* pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
	STDMETHOD(QueryCustomPolicy)(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved);
	STDMETHOD(SetSecuritySite)(IInternetSecurityMgrSite *pSite);
	STDMETHOD(SetZoneMapping)(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);

	DECLARE_GET_CONTROLLING_UNKNOWN()

	// ATL helper functions override

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease();

protected:

	HRESULT PromptOpenFile(LPTSTR pPath, ULONG ulPathLen);
	HRESULT PromptSaveAsFile(LPTSTR pPath, ULONG ulPathLen);

protected:

	IUnknown*			m_punkInnerCtl;
	IDHTMLSafe*			m_pInnerCtl;
	IOleObject*			m_pInnerIOleObj;
	IInterconnector*	m_pInterconnect;
	CEventXferSink*		m_pXferSink;
	IConnectionPoint*	m_piInnerCtlConPt;
	DWORD				m_dwXferCookie;
};


#endif //__DHTMLEDIT_H_
