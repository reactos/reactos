/*
 * Copyright 2004 Martin Fuchs
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // webchild.h
 //
 // Martin Fuchs, 08.02.2004
 //


#ifndef _MSC_VER
#include <exdisp.h>		// for IConnectionPointContainer
#include <exdispid.h>	// for DWebBrowserEvents IDs
#endif

#ifndef DISPID_BEFORENAVIGATE	// missing in MinGW (as of 07.02.2004)
#define DISPID_BEFORENAVIGATE			100
#define DISPID_NAVIGATECOMPLETE			101
#define DISPID_STATUSTEXTCHANGE			102
#define DISPID_QUIT						103
#define DISPID_DOWNLOADCOMPLETE			104
#define DISPID_COMMANDSTATECHANGE		105
#define DISPID_DOWNLOADBEGIN			106
#define DISPID_NEWWINDOW				107
#define DISPID_PROGRESSCHANGE			108
#define DISPID_WINDOWMOVE				109
#define DISPID_WINDOWRESIZE				110
#define DISPID_WINDOWACTIVATE			111
#define DISPID_PROPERTYCHANGE			112
#define DISPID_TITLECHANGE				113
#define DISPID_TITLEICONCHANGE			114
#define DISPID_FRAMEBEFORENAVIGATE		200
#define DISPID_FRAMENAVIGATECOMPLETE	201
#define DISPID_FRAMENEWWINDOW			204

#define DISPID_NAVIGATECOMPLETE2		252
#define DISPID_ONQUIT					253
#define DISPID_ONVISIBLE				254
#define DISPID_ONTOOLBAR				255
#define DISPID_ONMENUBAR				256
#define DISPID_ONSTATUSBAR				257
#define DISPID_ONFULLSCREEN				258
#define DISPID_DOCUMENTCOMPLETE			259
#define DISPID_ONTHEATERMODE			260
#define DISPID_ONADDRESSBAR				261
#define DISPID_WINDOWSETRESIZABLE		262
#define DISPID_WINDOWCLOSING			263
#define DISPID_WINDOWSETLEFT			264
#define DISPID_WINDOWSETTOP				265
#define DISPID_WINDOWSETWIDTH			266
#define DISPID_WINDOWSETHEIGHT			267
#define DISPID_CLIENTTOHOSTWINDOW		268
#define DISPID_SETSECURELOCKICON		269
#define DISPID_FILEDOWNLOAD				270
#define DISPID_NAVIGATEERROR			271
#define DISPID_PRIVACYIMPACTEDSTATECHANGE 272
#endif

#ifndef V_INT	// missing in MinGW (as of 07.02.2004)
#define	V_INT(x) V_UNION(x, intVal)
#endif

#ifdef _MSC_VER
#define	NOVTABLE __declspec(novtable)
#else
#define	NOVTABLE
#endif
#define	ANSUNC

#ifdef _MSC_VER
#pragma warning(disable: 4355)	// use of 'this' for initialization of _connector
#endif


struct NOVTABLE ComSrvObject	// NOVTABLE erlaubt, da protected Destruktor
{
protected:
	ComSrvObject() : _ref(1) {}
	virtual ~ComSrvObject() {}

	ULONG	_ref;
};

struct SimpleComObject : public ComSrvObject
{
	ULONG IncRef() {return ++_ref;}
	ULONG DecRef() {ULONG ref=--_ref; if (!ref) {_ref++; delete this;} return ref;}
};


 // server object interfaces

template<typename BASE> struct IComSrvQI : public BASE
{
	IComSrvQI(REFIID uuid_base)
	 :	_uuid_base(uuid_base)
	{
	}

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv) {*ppv=0;
	 if (IsEqualIID(riid, _uuid_base) ||
		 IsEqualIID(riid, IID_IUnknown)) {*ppv=static_cast<BASE*>(this); AddRef(); return S_OK;}
	 return E_NOINTERFACE;}

protected:
	IComSrvQI() {}

	REFIID	_uuid_base;
};

template<> struct IComSrvQI<IUnknown> : public IUnknown
{
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv) {*ppv=0;
	 if (IsEqualIID(riid, IID_IUnknown)) {*ppv=this; AddRef(); return S_OK;}
	 return E_NOINTERFACE;}

protected:
	IComSrvQI<IUnknown>() {}
};


template<typename BASE, typename OBJ>
	class IComSrvBase : public IComSrvQI<BASE>
{
	typedef IComSrvQI<BASE> super;

protected:
	IComSrvBase(REFIID uuid_base)
	 :	super(uuid_base)
	{
	}

public:
	STDMETHODIMP_(ULONG) AddRef() {return static_cast<OBJ*>(this)->IncRef();}
	STDMETHODIMP_(ULONG) Release() {return static_cast<OBJ*>(this)->DecRef();}
};


template<typename T> struct ConnectionPoint : public SIfacePtr<T>
{
	ConnectionPoint(IConnectionPointContainer* pCPC, REFIID riid)
	{
		CheckError(pCPC->FindConnectionPoint(riid, &_p));
	}
};

struct EventConnection
{
	EventConnection(IConnectionPoint* connectionpoint, IUnknown* sink)
	{
		CheckError(connectionpoint->Advise(sink, &_cookie));
		_connectionpoint = connectionpoint;
	}

	template<typename T> EventConnection(T& connectionpoint, IUnknown* sink)
	{
		CheckError(connectionpoint->Advise(sink, &_cookie));
		_connectionpoint = connectionpoint;
	}

/*	template<typename T> EventConnection(SIfacePtr<T>& connectionpoint, IUnknown* sink)
	{
		CheckError(connectionpoint->Advise(sink, &_cookie));
		_connectionpoint = connectionpoint.GetPtr();
	} */

/*	template<typename T> EventConnection(T& connectionpoint, IUnknown* sink)
	{
		CheckError(connectionpoint->Advise(sink, &_cookie));
		_connectionpoint = connectionpoint;
	} */

	~EventConnection()
	{
		if (_connectionpoint)
			_connectionpoint->Unadvise(_cookie);
	}

protected:
	SIfacePtr<IConnectionPoint> _connectionpoint;
	DWORD	_cookie;
};

struct EventConnector : public EventConnection
{
	EventConnector(IUnknown* unknown, REFIID riid, IUnknown* sink)
	 :	EventConnection(ConnectionPoint<IConnectionPoint>(
				SIfacePtr<IConnectionPointContainer>(unknown, IID_IConnectionPointContainer), riid), sink)
	{
	}
};


struct OleInPlaceClient : public SimpleComObject,
							public IOleClientSite,
							public IOleInPlaceSite
{
protected:
	HWND	_hwnd;

public:
	OleInPlaceClient(HWND hwnd=0)
	 :	_hwnd(hwnd)
	{
	}

	void attach(HWND hwnd)
	{
		_hwnd = hwnd;
	}

	HRESULT attach_control(IOleObject* ole_obj, LONG iVerb=OLEIVERB_INPLACEACTIVATE, HWND hwndParent=0, LPCRECT pRect=NULL)
	{
		HRESULT hr = ole_obj->SetClientSite(this);
		if (FAILED(hr))
			return hr;

//		hr = ole_obj->SetHostNames(app, doc));

		hr = ole_obj->DoVerb(iVerb, NULL, this, 0, 0/*hwnd*/, NULL/*&rcPos*/);

		return hr;
	}

	HRESULT detach(IOleObject* ole_obj, DWORD dwSaveOption=OLECLOSE_SAVEIFDIRTY)
	{
		HRESULT hr = ole_obj->Close(dwSaveOption);

		_hwnd = 0;

		return hr;
	}

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv)
	{
		if (IsEqualIID(riid, IID_IOleClientSite))
			{*ppv=static_cast<IOleClientSite*>(this); IncRef(); return S_OK;}

		if (IsEqualIID(riid, IID_IOleInPlaceSite))
			{*ppv=static_cast<IOleInPlaceSite*>(this); IncRef(); return S_OK;}

		if (IsEqualIID(riid, IID_IUnknown))
			{*ppv=static_cast<IOleClientSite/*oder auch IOleInPlaceSite*/*>(this); IncRef(); return S_OK;}

		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef() {return IncRef();}
	STDMETHODIMP_(ULONG) Release() {return DecRef();}


	 // IOleWindow:

	virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE GetWindow(/* [out] */ HWND __RPC_FAR *phwnd)
	{
		*phwnd = _hwnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(/* [in] */ BOOL fEnterMode)
	{
		return E_NOTIMPL;
	}


	 // IOleClientSite:

	virtual HRESULT STDMETHODCALLTYPE SaveObject()
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker __RPC_FAR *__RPC_FAR *ppmk)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer __RPC_FAR *__RPC_FAR *ppContainer)
	{
		ppContainer = 0;
		return E_NOINTERFACE;
	}

	virtual HRESULT STDMETHODCALLTYPE ShowObject()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout()
	{
		return S_OK;
	}


	 // IOleInPlaceSite:

	virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnUIActivate()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetWindowContext(
		/* [out] */ IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame,
		/* [out] */ IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc,
		/* [out] */ LPRECT lprcPosRect,
		/* [out] */ LPRECT lprcClipRect,
		/* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo)
	{
		ClientRect rect(_hwnd);

		ppFrame = 0;
		ppDoc = 0;
		*lprcPosRect = rect;
		*lprcClipRect = rect;

		assert(lpFrameInfo->cb>=sizeof(OLEINPLACEFRAMEINFO));
		lpFrameInfo->fMDIApp = FALSE;
		lpFrameInfo->hwndFrame = 0;
		lpFrameInfo->haccel = 0;
		lpFrameInfo->cAccelEntries = 0;

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Scroll(/* [in] */ SIZE scrollExtant)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(/* [in] */ BOOL fUndoable)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DiscardUndoState()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(/* [in] */ LPCRECT lprcPosRect)
	{
		return S_OK;
	}
};


 // window with in place activates Active-X Control

template<typename BASE, typename SMARTPTR> struct IPCtrlWindow : public BASE
{
	typedef BASE super;

	IPCtrlWindow(HWND hwnd)
	 :	super(hwnd)
	{
	}

	HRESULT create_control(HWND hwnd, REFIID clsid, REFIID riid)
	{
		 // Erzeugen einer Instanz des Controls
		HRESULT hr = _control.CreateInstance(clsid, riid);
		if (FAILED(hr))
			return hr;

		_client_side.attach(hwnd);

		hr = _client_side.attach_control(SIfacePtr<IOleObject>(_control, IID_IOleObject)/*, OLEIVERB_INPLACEACTIVATE,
											hwnd, &Rect(10, 10, 500, 500)*/);
		if (FAILED(hr))
			return hr;

		 // try to get a IOleInPlaceObject interface for window resizing
		return _control.QueryInterface(IID_IOleInPlaceObject, &_in_place_object);	// _in_place_object = _control
	}

protected:
	LRESULT WndProc(UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (message == WM_SIZE) {
			if (_in_place_object) {
				RECT rect = {0, 0, LOWORD(lparam), HIWORD(lparam)};

				_in_place_object->SetObjectRects(&rect, &rect);
			}
		} else if (message == WM_CLOSE) {
			_in_place_object = NULL;

			if (_control) {
				_client_side.detach(SIfacePtr<IOleObject>(_control, IID_IOleObject), OLECLOSE_NOSAVE);
				_control = NULL;
			}
		}

		return super::WndProc(message, wparam, lparam);
	}

	ComInit _usingCOM;
	SMARTPTR _control;
	OleInPlaceClient _client_side;
	SIfacePtr<IOleInPlaceObject> _in_place_object;
};



#include "exdispid.h"


struct DWebBrowserEventsIF
{
    virtual HRESULT BeforeNavigate(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* cancel)
		{return S_OK;}

    virtual HRESULT NavigateComplete(const String& url)
		{return S_OK;}

    virtual HRESULT StatusTextChange(const String& text)
		{return S_OK;}

    virtual HRESULT ProgressChange(long Progress, long progressMax)
		{return S_OK;}

    virtual HRESULT DownloadComplete()
		{return S_OK;}

    virtual HRESULT CommandStateChange(long command/*CSC_NAVIGATEFORWARD, CSC_NAVIGATEBACK*/, VARIANT_BOOL enable)
		{return S_OK;}

    virtual HRESULT DownloadBegin()
		{return S_OK;}

    virtual HRESULT NewWindow(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* processed)
		{return S_OK;}

    virtual HRESULT TitleChange(const String& text)
		{return S_OK;}

    virtual HRESULT TitleIconChange(const String& text)
		{return S_OK;}

    virtual HRESULT FrameBeforeNavigate(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* cancel)
		{return S_OK;}

    virtual HRESULT FrameNavigateComplete(const String& url)
		{return S_OK;}

    virtual HRESULT FrameNewWindow(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* processed)
		{return S_OK;}

    virtual HRESULT Quit(VARIANT_BOOL* cancel)
		{return S_OK;}

    virtual HRESULT WindowMove()
		{return S_OK;}

    virtual HRESULT WindowResize()
		{return S_OK;}

    virtual HRESULT WindowActivate()
		{return S_OK;}

    virtual HRESULT PropertyChange(const BStr& property)
		{return S_OK;}
};


#ifndef __DWebBrowserEvents_DISPINTERFACE_DEFINED__	// missing in MinGW (as of 07.02.2004)
interface DWebBrowserEvents : public IDispatch
{
};
#endif


struct ANSUNC DWebBrowserEventsImpl : public SimpleComObject,
						public IComSrvBase<DWebBrowserEvents, DWebBrowserEventsImpl>,
						public DWebBrowserEventsIF
{
	typedef IComSrvBase<DWebBrowserEvents, DWebBrowserEventsImpl> super;


	DWebBrowserEventsIF* _callback;


	DWebBrowserEventsImpl()
	 :	super(DIID_DWebBrowserEvents)
	{
		_callback = this;
	}

/*	 // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
	{
		*ppv = NULL;

		if (SUCCEEDED(super::QueryInterface(riid, ppv)))
			return S_OK;

		return E_NOINTERFACE;
	} */


	 // IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
		{return E_NOTIMPL;}

	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
		{return E_NOTIMPL;}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
		{return E_NOTIMPL;}

	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
						DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
	{
		switch(dispIdMember) {
		  case DISPID_BEFORENAVIGATE: {
			if (pDispParams->cArgs != 6)
				return E_INVALIDARG;

			BSTR s5 = V_BSTR(&pDispParams->rgvarg[5]);
			BSTR s3 = V_BSTR(&pDispParams->rgvarg[3]);	// Mozilla's Active X control gives us NULL in bstrVal.
			BSTR s1 = V_BSTR(&pDispParams->rgvarg[1]);

			return _callback->BeforeNavigate(s5? s5: L"", V_I4(&pDispParams->rgvarg[4]),
								  s3? s3: L"", &pDispParams->rgvarg[2],
								  s1? s1: L"", V_BOOLREF(&pDispParams->rgvarg[0]));}

		  case DISPID_NAVIGATECOMPLETE:	// in async, this is sent when we have enough to show
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			return _callback->NavigateComplete(V_BSTR(&pDispParams->rgvarg[0]));

		  case DISPID_STATUSTEXTCHANGE:
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			return _callback->StatusTextChange(V_BSTR(&pDispParams->rgvarg[0]));

		  case DISPID_QUIT:
			return _callback->Quit(NULL);

		  case DISPID_DOWNLOADCOMPLETE:
			return _callback->DownloadComplete();

		  case DISPID_COMMANDSTATECHANGE:
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			return _callback->CommandStateChange(V_I4(&pDispParams->rgvarg[1]), pDispParams->rgvarg[0].boolVal);

		  case DISPID_DOWNLOADBEGIN:
			return _callback->DownloadBegin();

		  case DISPID_NEWWINDOW:		// sent when a new window should be created
			if (pDispParams->cArgs != 6)
				return E_INVALIDARG;
			return _callback->NewWindow(V_BSTR(&pDispParams->rgvarg[5]), V_I4(&pDispParams->rgvarg[4]),
							V_BSTR(&pDispParams->rgvarg[3]), &pDispParams->rgvarg[2],
							V_BSTR(&pDispParams->rgvarg[1]), V_BOOLREF(&pDispParams->rgvarg[0]));

		  case DISPID_PROGRESSCHANGE:	// sent when download progress is updated
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			return _callback->ProgressChange(V_I4(&pDispParams->rgvarg[1]), V_I4(&pDispParams->rgvarg[0]));

		  case DISPID_WINDOWMOVE:		// sent when main window has been moved
			return _callback->WindowMove();

		  case DISPID_WINDOWRESIZE:		// sent when main window has been sized
			return _callback->WindowResize();

		  case DISPID_WINDOWACTIVATE:	// sent when main window has been activated
			return _callback->WindowActivate();

		  case DISPID_PROPERTYCHANGE:	// sent when the PutProperty method is called
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			return _callback->PropertyChange(V_BSTR(&pDispParams->rgvarg[0]));

		  case DISPID_TITLECHANGE:		// sent when the document title changes
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			return _callback->TitleChange(V_BSTR(&pDispParams->rgvarg[0]));

		  case DISPID_TITLEICONCHANGE:	// sent when the top level window icon may have changed.
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			return _callback->TitleIconChange(V_BSTR(&pDispParams->rgvarg[0]));

		  default:
			return NOERROR;
		}
	}
};


struct DWebBrowserEvents2IF
{
    virtual void StatusTextChange(const BStr& text)
		{}

    virtual void ProgressChange(long progress, long progressMax)
		{}

    virtual void WindowMove()
		{}

    virtual void WindowResize()
		{}

    virtual void WindowActivate()
		{}

    virtual void PropertyChange(const BStr& property)
		{}

    virtual void DownloadComplete()
		{}

    virtual void CommandStateChange(long command, bool enable)
		{}

    virtual void DownloadBegin()
		{}

    virtual void NewWindow2(IDispatch** ppDisp, VARIANT_BOOL& cancel)
		{}

    virtual void TitleChange(const BStr& text)
		{}

    virtual void TitleIconChange(const BStr& text)
		{}

	virtual void FrameBeforeNavigate(const BStr& url, long flags, const BStr& targetFrameName, VARIANT* postData, const BStr& headers, VARIANT_BOOL& cancel)
		{}

	virtual void FrameNavigateComplete(const BStr& url)
		{}

	virtual void FrameNewWindow(const BStr&url, long flags, const BStr& targetFrameName, VARIANT* postData, const BStr& headers, VARIANT_BOOL& processed)
		{}

    virtual void BeforeNavigate2(IDispatch* pDisp, const Variant& url, const Variant& flags,
							const Variant& targetFrameName, const Variant& postData,
							const Variant& headers, VARIANT_BOOL& cancel)
		{}

    virtual void NavigateComplete2(IDispatch* pDisp, const Variant& url)
		{}

    virtual void OnQuit()
		{}

	virtual void OnVisible(bool Visible)
		{}

	virtual void OnToolbar(bool Visible)
		{}

	virtual void OnMenubar(bool Visible)
		{}

	virtual void OnStatusbar(bool Visible)
		{}

	virtual void OnFullscreen(bool Visible)
		{}

    virtual void DocumentComplete()
		{}

	virtual void OnTheatermode(bool Visible)
		{}

	virtual void OnAddressbar(bool Visible)
		{}

	virtual void WindowSetResizable(bool Visible)
		{}

	virtual void WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL& cancel)
		{}

	virtual void WindowSetLeft(long Left)
		{}

	virtual void WindowSetTop(long Top)
		{}

	virtual void WindowSetWidth(long Width)
		{}

	virtual void WindowSetHeight(long Height)
		{}

	virtual void ClientToHostWindow(long& CX, long& CY)
		{}

	virtual void SetSecureLockIcon(long SecureLockIcon)
		{}

    virtual void FileDownload(Variant& cancel)
		{}

    virtual void NavigateError(IDispatch* pDisp, const Variant& url, const Variant& Frame, const Variant& StatusCode, VARIANT_BOOL& cancel)
		{}

    virtual void PrivacyImpactedStateChange(bool bImpacted)
		{}
};


 // Das Webbrowser-Control muﬂ zun‰chst komplett initialisiert sein, bevor eine Seite,
 // die nicht auf das Internet zugreift (z.B. addcom/./index.html) dargestellt werden kann.
struct ANSUNC BrowserNavigator
{
	BrowserNavigator(IWebBrowser* browser);

	void	goto_url(LPCTSTR url);
	void	set_html_page(const String& html_txt);
	void	navigated(LPCTSTR url);

protected:
	SIfacePtr<IWebBrowser> _browser;
	String	_new_url;
	String	_new_html_txt;
	bool	_browser_initialized;
};


 // MinGW defines a wrong FixedDWebBrowserEvents2 interface with virtual functions for DISPID calls, so we use our own, corrected version:
interface FixedDWebBrowserEvents2 : public IDispatch
{
};

struct ANSUNC DWebBrowserEvents2Impl : public SimpleComObject,
					public IComSrvBase<FixedDWebBrowserEvents2, DWebBrowserEvents2Impl>,
					public DWebBrowserEvents2IF
{
	typedef IComSrvBase<FixedDWebBrowserEvents2, DWebBrowserEvents2Impl> super;


	DWebBrowserEvents2IF* _callback;


	DWebBrowserEvents2Impl(BrowserNavigator& navigator)
	 :	super(DIID_DWebBrowserEvents2),
		_navigator(navigator)
	{
		_callback = this;
	}


/*	 // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
	{
		*ppv = NULL;

		if (SUCCEEDED(super::QueryInterface(riid, ppv)))
			return S_OK;

		return E_NOINTERFACE;
	} */


	 // IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
		{return E_NOTIMPL;}

	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
		{return E_NOTIMPL;}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
		{return E_NOTIMPL;}

	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
						DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
	{
		switch(dispIdMember) {
		  case DISPID_STATUSTEXTCHANGE:
			_callback->StatusTextChange((BStr)Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_COMMANDSTATECHANGE:
			_callback->CommandStateChange(Variant(pDispParams->rgvarg[1]), Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_DOWNLOADBEGIN:
			_callback->DownloadBegin();
			break;

		  case DISPID_PROGRESSCHANGE:	// sent when download progress is updated
			_callback->ProgressChange(Variant(pDispParams->rgvarg[1]), Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWMOVE:		// sent when main window has been moved
			_callback->WindowMove();
			break;

		  case DISPID_WINDOWRESIZE:		// sent when main window has been sized
			_callback->WindowResize();
			break;

		  case DISPID_WINDOWACTIVATE:	// sent when main window has been activated
			_callback->WindowActivate();
			break;

		  case DISPID_PROPERTYCHANGE:	// sent when the PutProperty method is called
			_callback->PropertyChange((BStr)Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_TITLECHANGE:		// sent when the document title changes
			_callback->TitleChange((BStr)Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_TITLEICONCHANGE:	// sent when the top level window icon may have changed.
			_callback->TitleIconChange((BStr)Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_FRAMEBEFORENAVIGATE:
			if (pDispParams->cArgs != 6)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			_callback->FrameBeforeNavigate(
								  (BStr)Variant(&pDispParams->rgvarg[5]), Variant(&pDispParams->rgvarg[4]),
								  (BStr)Variant(&pDispParams->rgvarg[3]), &pDispParams->rgvarg[2],
								  (BStr)Variant(&pDispParams->rgvarg[1]), *V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_FRAMENAVIGATECOMPLETE:
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->FrameNavigateComplete((BStr)Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_FRAMENEWWINDOW:
			if (pDispParams->cArgs != 6)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			_callback->FrameNewWindow((BStr)Variant(&pDispParams->rgvarg[5]), Variant(&pDispParams->rgvarg[4]),
								  (BStr)Variant(&pDispParams->rgvarg[3]), &pDispParams->rgvarg[2],
								  (BStr)Variant(&pDispParams->rgvarg[1]), *V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_BEFORENAVIGATE2:	// hyperlink clicked on
			if (pDispParams->cArgs != 7)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			_callback->BeforeNavigate2(Variant(pDispParams->rgvarg[6]),
								  pDispParams->rgvarg[5], &pDispParams->rgvarg[4],
								  pDispParams->rgvarg[3], &pDispParams->rgvarg[2],
								  pDispParams->rgvarg[1], *V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_NEWWINDOW2:		// sent when a new window should be created
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[1]) != (VT_DISPATCH|VT_BYREF))
				return E_INVALIDARG;
			_callback->NewWindow2(V_DISPATCHREF(&pDispParams->rgvarg[1]), *V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_NAVIGATECOMPLETE2:// UIActivate new document
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;

			 // notify the navigator
			NavigateComplete2(Variant(pDispParams->rgvarg[1]), Variant(pDispParams->rgvarg[0]));

			_callback->NavigateComplete2(Variant(pDispParams->rgvarg[1]), Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONQUIT:
			_callback->OnQuit();
			break;

		  case DISPID_ONVISIBLE:		// sent when the window goes visible/hidden
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnVisible(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONTOOLBAR:		// sent when the toolbar should be shown/hidden
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnToolbar(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONMENUBAR:		// sent when the menubar should be shown/hidden
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnMenubar(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONSTATUSBAR:		// sent when the statusbar should be shown/hidden
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnStatusbar(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONFULLSCREEN:		// sent when kiosk mode should be on/off
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnFullscreen(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_DOCUMENTCOMPLETE:// new document goes ReadyState_Complete
			_callback->DocumentComplete();
			break;

		  case DISPID_DOWNLOADCOMPLETE:
			_callback->DownloadComplete();
			break;

		  case DISPID_ONTHEATERMODE:	// sent when theater mode should be on/off
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnTheatermode(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_ONADDRESSBAR:		// sent when the address bar should be shown/hidden
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->OnAddressbar(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWSETRESIZABLE:// sent to set the style of the host window frame
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->WindowSetResizable(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWCLOSING:	// sent before script window.close closes the window
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			_callback->WindowClosing(Variant(pDispParams->rgvarg[1]), *V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWSETLEFT:	// sent when the put_left method is called on the WebOC
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->WindowSetLeft(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWSETTOP:		// sent when the put_top method is called on the WebOC
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->WindowSetTop(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWSETWIDTH:	// sent when the put_width method is called on the WebOC
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->WindowSetWidth(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_WINDOWSETHEIGHT:	// sent when the put_height method is called on the WebOC 
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->WindowSetHeight(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_CLIENTTOHOSTWINDOW:// sent during window.open to request conversion of dimensions
			if (pDispParams->cArgs != 2)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_I4|VT_BYREF))
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[1]) != (VT_I4|VT_BYREF))
				return E_INVALIDARG;
			_callback->ClientToHostWindow(*V_I4REF(&pDispParams->rgvarg[1]), *V_I4REF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_SETSECURELOCKICON:// sent to suggest the appropriate security icon to show
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->SetSecureLockIcon(Variant(pDispParams->rgvarg[0]));
			break;

		  case DISPID_FILEDOWNLOAD: {	// Fired to indicate the File Download dialog is opening
			if (pDispParams->cArgs != 1)	//@@ every time 2 ?!
				return E_INVALIDARG;
			Variant var(pDispParams->rgvarg[0]);
			_callback->FileDownload(var);}
			break;

		  case DISPID_NAVIGATEERROR:	// Fired to indicate the a binding error has occured
			if (pDispParams->cArgs != 5)
				return E_INVALIDARG;
			if (V_VT(&pDispParams->rgvarg[0]) != (VT_BOOL|VT_BYREF))
				return E_INVALIDARG;
			_callback->NavigateError(Variant(pDispParams->rgvarg[4]), Variant(pDispParams->rgvarg[3]),
								Variant(pDispParams->rgvarg[2]), Variant(pDispParams->rgvarg[1]),
								*V_BOOLREF(&pDispParams->rgvarg[0]));
			break;

		  case DISPID_PRIVACYIMPACTEDSTATECHANGE:// Fired when the user's browsing experience is impacted
			if (pDispParams->cArgs != 1)
				return E_INVALIDARG;
			_callback->PrivacyImpactedStateChange(Variant(pDispParams->rgvarg[0]));
			break;

		  default:
			return NOERROR;
		}

		return S_OK;
	}

protected:
	BrowserNavigator& _navigator;

	void NavigateComplete2(IDispatch* pDisp, const Variant& url)
	{
		String adr = (BStr)url;

		_navigator.navigated(adr);
	}
};


struct DWebBrowserEventsHandler : public DWebBrowserEventsImpl
{
	DWebBrowserEventsHandler(HWND hwnd, IWebBrowser* browser)
	 :	_hwnd(hwnd),
		_browser(browser, IID_IWebBrowser2),
		_connector(browser, DIID_DWebBrowserEvents, this)
	{
	}

protected:
    HRESULT BeforeNavigate(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* cancel)
	{
		return S_OK;
	}

    HRESULT NavigateComplete(const String& url)
		{return S_OK;}

    HRESULT StatusTextChange(const String& text)
	{
		_status = text;
		SetWindowText(_hwnd, FmtString(_T("%#s  -  %#s"), _title.c_str(), _status.c_str()));
		return S_OK;
	}

    HRESULT ProgressChange(long Progress, long ProgressMax)
		{return S_OK;}

    HRESULT CommandStateChange(long command, VARIANT_BOOL enable)
		{return S_OK;}

    HRESULT DownloadComplete()
		{return S_OK;}

    HRESULT DownloadBegin()
		{return S_OK;}

    HRESULT NewWindow(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* processed)
		{return S_OK;}

    HRESULT TitleChange(const String& text)
	{
		_title = text;
		SetWindowText(_hwnd, FmtString(_T("%#s  -  %#s"), _title.c_str(), _status.c_str()));
		return S_OK;
	}

    HRESULT TitleIconChange(const String& text)
		{return S_OK;}

    HRESULT FrameBeforeNavigate(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* cancel)
		{return S_OK;}

    HRESULT FrameNavigateComplete(const String& url)
		{return S_OK;}

    HRESULT FrameNewWindow(const String& url, long flags, const String& targetFrameName, VARIANT* postData, const String& headers, VARIANT_BOOL* processed)
		{return S_OK;}

    HRESULT Quit(VARIANT_BOOL* cancel)
		{return S_OK;}

    HRESULT WindowMove()
		{return S_OK;}

    HRESULT WindowResize()
		{return S_OK;}

    HRESULT WindowActivate()
		{return S_OK;}

    HRESULT PropertyChange(const BStr& property)
	{
		Variant value;
		_browser->GetProperty(property, &value);
		return S_OK;
	}

protected:
	HWND	_hwnd;
	String	_title, _status;
	SIfacePtr<IWebBrowser2> _browser;
	EventConnector _connector;
};


 // Eventhandler using DWebBrowserEvents2Impl

struct DWebBrowserEvents2Handler : public DWebBrowserEvents2Impl
{
	typedef DWebBrowserEvents2Impl super;

	DWebBrowserEvents2Handler(HWND hwnd, IWebBrowser* browser)
	 :	_hwnd(hwnd),
		_navigator(browser),
		DWebBrowserEvents2Impl(_navigator),
		_browser(browser, IID_IWebBrowser2),
		_connector(browser, DIID_DWebBrowserEvents2, this)
	{
	}

protected:
    void BeforeNavigate2(IDispatch* pDisp, const Variant& url, const Variant& flags,
							const Variant& targetFrameName, const Variant& postData,
							const Variant& headers, VARIANT_BOOL& cancel)
	{
		//String adr = (BStr)url;
	}

    void NavigateComplete2(IDispatch* pDisp, const Variant& url)
	{
		//String adr = (BStr)url;
		super::NavigateComplete2(pDisp, url);
	}

    void StatusTextChange(const BStr& text)
	{
		_status = text;
		SetWindowText(_hwnd, FmtString(_T("%#s  -  %#s"), _title.c_str(), _status.c_str()));
	}

    void ProgressChange(long Progress, long ProgressMax)
	{
	}

    void WindowMove()
	{
	}

    void WindowResize()
	{
	}

    void WindowActivate()
	{
	}

    void PropertyChange(const BStr& Property)
	{
		Variant value;
		_browser->GetProperty(Property, &value);
	}

    void CommandStateChange(long command/*CSC_NAVIGATEFORWARD, CSC_NAVIGATEBACK*/, bool enable)
	{
	}

    void DownloadBegin()
	{
	}

    void NewWindow2(IDispatch** ppDisp, VARIANT_BOOL& cancel)
	{
		//*ppDisp = ;
		//cancel = TRUE;
	}

    void TitleChange(const BStr& text)
	{
		_title = text;
		SetWindowText(_hwnd, FmtString(_T("%#s  -  %#s"), _title.c_str(), _status.c_str()));
	}

    void TitleIconChange(const BStr& text)
	{
	}

    void FrameBeforeNavigate(const BStr& url, long flags, const BStr& targetFrameName, VARIANT* postData, const BStr& headers, VARIANT_BOOL& cancel)
	{
	}

    void FrameNavigateComplete(const BStr& url)
	{
	}

    void FrameNewWindow(const BStr& url, long flags, const BStr& targetFrameName, VARIANT* postData, const BStr& headers, VARIANT_BOOL& processed)
	{
	}

    void OnQuit()
	{
	}

	void OnVisible(bool Visible)
	{
	}

	void OnToolbar(bool Visible)
	{
	}

	void OnMenubar(bool Visible)
	{
	}

	void OnStatusbar(bool Visible)
	{
	}

	void OnFullscreen(bool Visible)
	{
	}

    void DocumentComplete()
	{
	}

	void OnTheatermode(bool Visible)
	{
	}

	void OnAddressbar(bool Visible)
	{
	}

	void WindowSetResizable(bool Visible)
	{
	}

	void WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL& cancel)
	{
	}

	void WindowSetLeft(long Left)
	{
	}

	void WindowSetTop(long Top)
	{
	}

	void WindowSetWidth(long Width)
	{
	}

	void WindowSetHeight(long Height)
	{
	}

	void ClientToHostWindow(long& CX, long& CY)
	{
	}

	void SetSecureLockIcon(long SecureLockIcon)
	{
	}

    void FileDownload(Variant& cancel)
	{
	}

    void NavigateError(IDispatch* pDisp, const Variant& url, const Variant& Frame, const Variant& StatusCode, VARIANT_BOOL& cancel)
	{
	}

    void PrivacyImpactedStateChange(bool bImpacted)
	{
	}

protected:
	HWND	_hwnd;
	BrowserNavigator _navigator;
	SIfacePtr<IWebBrowser2> _browser;
	EventConnector _connector;

	String	_title, _status;
};


 /// encapsulation of Web control in MDI child windows
struct WebChildWindow : public IPCtrlWindow<ChildWindow, SIfacePtr<IWebBrowser2> >
{
	typedef IPCtrlWindow<ChildWindow, SIfacePtr<IWebBrowser2> > super;

	WebChildWindow(HWND hwnd, const WebChildWndInfo& info);
	~WebChildWindow();

	static WebChildWindow* create(HWND hmdiclient, const FileChildWndInfo& info)
	{
		ChildWindow* child = ChildWindow::create(hmdiclient, info._pos.rcNormalPosition,
			WINDOW_CREATOR_INFO(WebChildWindow,WebChildWndInfo), CLASSNAME_CHILDWND, NULL, &info);

		ShowWindow(*child, info._pos.showCmd);

		return static_cast<WebChildWindow*>(child);
	}

	IWebBrowser2* get_browser()
	{
		return _control;
	}

protected:
	DWebBrowserEventsHandler* _evt_handler1;
	DWebBrowserEvents2Handler* _evt_handler2;
};
