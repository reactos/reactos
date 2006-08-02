#include "stdafx.h"

namespace MSTSCLib
{
#include "mstsclib_h.h"
};

#include "rdesktop/rdesktop.h"
#include "rdesktop/proto.h"

namespace
{
#ifdef _MSC_VER
	extern "C" char __ImageBase;
#endif

	HMODULE GetCurrentModule()
	{
		return reinterpret_cast<HMODULE>(&__ImageBase);
	}
}

#pragma warning(push)
#pragma warning(disable: 4584)

class RdpClient:
	/* COM basics */
	public IUnknown,
	public IDispatch,

	/* ActiveX stuff */
	public IConnectionPointContainer,
	public IDataObject,
	public IObjectSafety,
	public IOleControl,
	public IOleInPlaceActiveObject,
	public IOleInPlaceObject,
	public IOleObject,
	public IOleWindow,
	public IPersist,
	public IPersistPropertyBag,
	public IPersistStorage,
	public IPersistStreamInit,
	public IProvideClassInfo,
	public IProvideClassInfo2,
	public IQuickActivate,
	public IViewObject,
	public IViewObject2,

	// NOTE: the original has a vestigial implementation of this, which we omit
	// public ISpecifyPropertyPages,

	/* RDP client interface */
	public MSTSCLib::IMsRdpClient4,
	public MSTSCLib::IMsRdpClientNonScriptable2,

	/* RDP client context */
	public RDPCLIENT
{
private:
	/* An endless amount of COM glue */
	volatile LONG * m_moduleRefCount;
	IUnknown * m_punkOuter;
	CLSID m_classId;
	ITypeLib * m_typeLib;
	ITypeInfo * m_dispTypeInfo;
	LONG m_refCount;

	/* Inner IUnknown for aggregation support */
	class RdpClientInner: public IUnknown
	{
	public:
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			RdpClient * outerThis = InnerToOuter(this);

			using namespace MSTSCLib;

			IUnknown * pvObject = NULL;

			if(riid == IID_IUnknown)
				pvObject = static_cast<IUnknown *>(this); // use of "this" is NOT an error!
			else if(riid == IID_IConnectionPointContainer)
				pvObject = static_cast<IConnectionPointContainer *>(outerThis);
			else if(riid == IID_IDataObject)
				pvObject = static_cast<IDataObject *>(outerThis);
			else if(riid == IID_IObjectSafety)
				pvObject = static_cast<IObjectSafety *>(outerThis);
			else if(riid == IID_IOleControl)
				pvObject = static_cast<IOleControl *>(outerThis);
			else if(riid == IID_IOleInPlaceActiveObject)
				pvObject = static_cast<IOleInPlaceActiveObject *>(outerThis);
			else if(riid == IID_IOleInPlaceObject)
				pvObject = static_cast<IOleInPlaceObject *>(outerThis);
			else if(riid == IID_IOleObject)
				pvObject = static_cast<IOleObject *>(outerThis);
			else if(riid == IID_IOleWindow)
				pvObject = static_cast<IOleWindow *>(outerThis);
			else if(riid == IID_IPersist)
				pvObject = static_cast<IPersist *>(outerThis);
			else if(riid == IID_IPersistPropertyBag)
				pvObject = static_cast<IPersistPropertyBag *>(outerThis);
			else if(riid == IID_IPersistStorage)
				pvObject = static_cast<IPersistStorage *>(outerThis);
			else if(riid == IID_IPersistStreamInit)
				pvObject = static_cast<IPersistStreamInit *>(outerThis);
			else if(riid == IID_IQuickActivate)
				pvObject = static_cast<IQuickActivate *>(outerThis);
			else if(riid == IID_IViewObject)
				pvObject = static_cast<IViewObject *>(outerThis);
			else if(riid == IID_IViewObject2)
				pvObject = static_cast<IViewObject2 *>(outerThis);
			else if(riid == IID_IMsTscAx)
				pvObject = static_cast<IMsTscAx *>(outerThis);
			else if(riid == IID_IMsRdpClient)
				pvObject = static_cast<IMsRdpClient *>(outerThis);
			else if(riid == IID_IMsRdpClient2)
				pvObject = static_cast<IMsRdpClient2 *>(outerThis);
			else if(riid == IID_IMsRdpClient3)
				pvObject = static_cast<IMsRdpClient3 *>(outerThis);
			else if(riid == IID_IMsRdpClient4)
				pvObject = static_cast<IMsRdpClient4 *>(outerThis);
			else if(riid == IID_IMsTscNonScriptable)
				pvObject = static_cast<IMsTscNonScriptable *>(outerThis);
			else if(riid == IID_IMsRdpClientNonScriptable)
				pvObject = static_cast<IMsRdpClientNonScriptable *>(outerThis);
			else if(riid == IID_IMsRdpClientNonScriptable2)
				pvObject = static_cast<IMsRdpClientNonScriptable2 *>(outerThis);

			*ppvObject = pvObject;

			if(pvObject)
			{
				pvObject->AddRef();
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return InnerToOuter(this)->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return InnerToOuter(this)->release();
		}

	}
	m_inner;

	// FIXME: (re)use fixed buffers for these
	BSTR m_server;
	BSTR m_domain;
	BSTR m_userName;
	BSTR m_disconnectedText;
	BSTR m_connectingText;
	BSTR m_fullScreenTitle;

	bool m_nonIdle;
	bool m_connected;
	bool m_startConnected;

	/* Reference counting */
	ULONG addRef()
	{
		return InterlockedIncrement(&m_refCount);
	}

	ULONG release()
	{
		LONG n = InterlockedDecrement(&m_refCount);

		if(n == 0)
			delete this;

		return n;
	}

	/* Constructor */
	RdpClient(REFCLSID classId, IUnknown * punkOuter, LONG * moduleRefCount):
		m_classId(classId),
		m_punkOuter(punkOuter),
		m_moduleRefCount(moduleRefCount)
	{
		if(m_punkOuter == NULL)
			m_punkOuter = &m_inner;

		// TODO: initialize RDPCLIENT fields
	}

	/* Destructor */
	~RdpClient()
	{
		InterlockedDecrement(m_moduleRefCount);
	}

	/* Helpers */
	static HRESULT SetStringProperty(BSTR& prop, BSTR newValue)
	{
		if(newValue == NULL)
			return E_INVALIDARG;

		SysFreeString(prop);

		prop = SysAllocStringLen(newValue, SysStringLen(newValue));

		if(prop == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	static HRESULT GetStringProperty(BSTR& prop, BSTR * retVal)
	{
		*retVal = SysAllocStringLen(prop, SysStringLen(prop));

		if(*retVal == NULL)
			return E_OUTOFMEMORY;

		return S_OK;
	}

	/* Advanced settings */
	friend class AdvancedSettings;

	class AdvancedSettings: public MSTSCLib::IMsRdpClientAdvancedSettings4
	{
	private:
		/* IDispatch type information */
		ITypeInfo * m_dispTypeInfo;

		HRESULT LoadDispTypeInfo()
		{
			if(m_dispTypeInfo)
				return S_OK;

			HRESULT hr = InnerToOuter(this)->LoadTypeLibrary();

			if(FAILED(hr))
				return hr;

			hr = InnerToOuter(this)->m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClientAdvancedSettings4, &m_dispTypeInfo);

			if(FAILED(hr))
				return hr;

			assert(m_dispTypeInfo);
			return S_OK;
		}

		HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			m_dispTypeInfo->AddRef();
			*ppTI = m_dispTypeInfo;
			return S_OK;
		}

	public:
		~AdvancedSettings()
		{
			if(m_dispTypeInfo)
				m_dispTypeInfo->Release();
		}

		/* IUnknown */
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			using namespace MSTSCLib;

			if
			(
				riid == IID_IUnknown ||
				riid == IID_IDispatch ||
				riid == IID_IMsTscAdvancedSettings ||
				riid == IID_IMsRdpClientAdvancedSettings ||
				riid == IID_IMsRdpClientAdvancedSettings2 ||
				riid == IID_IMsRdpClientAdvancedSettings3 ||
				riid == IID_IMsRdpClientAdvancedSettings4
			)
			{
				*ppvObject = this;
				InnerToOuter(this)->addRef();
				return S_OK;
			}
			else
			{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return InnerToOuter(this)->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return InnerToOuter(this)->release();
		}

		/* IDispatch */
		virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
		{
			*pctinfo = 1;
			return S_OK;
		}

		virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
		{
			if(iTInfo != 0)
				return DISP_E_BADINDEX;
		
			return AcquireDispTypeInfo(ppTInfo);
		}

		virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
		}

		virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
		{
			return m_dispTypeInfo->Invoke
			(
				static_cast<MSTSCLib::IMsRdpClientAdvancedSettings4 *>(this),
				dispIdMember,
				wFlags,
				pDispParams,
				pVarResult,
				pExcepInfo,
				puArgErr
			);
		}

		/* IMsTscAdvancedSettings */
		virtual STDMETHODIMP IMsTscAdvancedSettings::put_Compress(long pcompress)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_Compress(long * pcompress)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_BitmapPeristence(long pbitmapPeristence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_BitmapPeristence(long * pbitmapPeristence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_allowBackgroundInput(long pallowBackgroundInput)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_allowBackgroundInput(long * pallowBackgroundInput)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_KeyBoardLayoutStr(BSTR rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_PluginDlls(BSTR rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_IconFile(BSTR rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_IconIndex(long rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_ContainerHandledFullScreen(long pContainerHandledFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_ContainerHandledFullScreen(long * pContainerHandledFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::put_DisableRdpdr(long pDisableRdpdr)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscAdvancedSettings::get_DisableRdpdr(long * pDisableRdpdr)
		{
			return E_NOTIMPL; // TODO
		}

		/* IMsRdpClientAdvancedSettings */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SmoothScroll(long psmoothScroll)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SmoothScroll(long * psmoothScroll)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_AcceleratorPassthrough(long pacceleratorPassthrough)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_AcceleratorPassthrough(long * pacceleratorPassthrough)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ShadowBitmap(long pshadowBitmap)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ShadowBitmap(long * pshadowBitmap)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_TransportType(long ptransportType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_TransportType(long * ptransportType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SasSequence(long psasSequence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SasSequence(long * psasSequence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EncryptionEnabled(long pencryptionEnabled)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EncryptionEnabled(long * pencryptionEnabled)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DedicatedTerminal(long pdedicatedTerminal)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DedicatedTerminal(long * pdedicatedTerminal)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RDPPort(long prdpPort)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RDPPort(long * prdpPort)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EnableMouse(long penableMouse)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EnableMouse(long * penableMouse)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DisableCtrlAltDel(long pdisableCtrlAltDel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DisableCtrlAltDel(long * pdisableCtrlAltDel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_EnableWindowsKey(long penableWindowsKey)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_EnableWindowsKey(long * penableWindowsKey)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DoubleClickDetect(long pdoubleClickDetect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DoubleClickDetect(long * pdoubleClickDetect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_MaximizeShell(long pmaximizeShell)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_MaximizeShell(long * pmaximizeShell)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyFullScreen(long photKeyFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyFullScreen(long * photKeyFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyCtrlEsc(long photKeyCtrlEsc)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyCtrlEsc(long * photKeyCtrlEsc)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltEsc(long photKeyAltEsc)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltEsc(long * photKeyAltEsc)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltTab(long photKeyAltTab)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltTab(long * photKeyAltTab)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltShiftTab(long photKeyAltShiftTab)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltShiftTab(long * photKeyAltShiftTab)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyAltSpace(long photKeyAltSpace)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyAltSpace(long * photKeyAltSpace)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_HotKeyCtrlAltDel(long photKeyCtrlAltDel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_HotKeyCtrlAltDel(long * photKeyCtrlAltDel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_orderDrawThreshold(long porderDrawThreshold)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_orderDrawThreshold(long * porderDrawThreshold)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapCacheSize(long pbitmapCacheSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapCacheSize(long * pbitmapCacheSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCacheSize(long pbitmapVirtualCacheSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCacheSize(long * pbitmapVirtualCacheSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ScaleBitmapCachesByBPP(long pbScale)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ScaleBitmapCachesByBPP(long * pbScale)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_NumBitmapCaches(long pnumBitmapCaches)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_NumBitmapCaches(long * pnumBitmapCaches)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_CachePersistenceActive(long pcachePersistenceActive)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_CachePersistenceActive(long * pcachePersistenceActive)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PersistCacheDirectory(BSTR rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_brushSupportLevel(long pbrushSupportLevel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_brushSupportLevel(long * pbrushSupportLevel)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_minInputSendInterval(long pminInputSendInterval)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_minInputSendInterval(long * pminInputSendInterval)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_InputEventsAtOnce(long pinputEventsAtOnce)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_InputEventsAtOnce(long * pinputEventsAtOnce)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_maxEventCount(long pmaxEventCount)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_maxEventCount(long * pmaxEventCount)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_keepAliveInterval(long pkeepAliveInterval)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_keepAliveInterval(long * pkeepAliveInterval)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_shutdownTimeout(long pshutdownTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_shutdownTimeout(long * pshutdownTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_overallConnectionTimeout(long poverallConnectionTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_overallConnectionTimeout(long * poverallConnectionTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_singleConnectionTimeout(long psingleConnectionTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_singleConnectionTimeout(long * psingleConnectionTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardType(long pkeyboardType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardType(long * pkeyboardType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardSubType(long pkeyboardSubType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardSubType(long * pkeyboardSubType)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_KeyboardFunctionKey(long pkeyboardFunctionKey)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_KeyboardFunctionKey(long * pkeyboardFunctionKey)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_WinceFixedPalette(long pwinceFixedPalette)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_WinceFixedPalette(long * pwinceFixedPalette)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ConnectToServerConsole(VARIANT_BOOL pConnectToConsole)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_ConnectToServerConsole(VARIANT_BOOL * pConnectToConsole)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapPersistence(long pbitmapPersistence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapPersistence(long * pbitmapPersistence)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_MinutesToIdleTimeout(long pminutesToIdleTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_MinutesToIdleTimeout(long * pminutesToIdleTimeout)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_SmartSizing(VARIANT_BOOL pfSmartSizing)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_SmartSizing(VARIANT_BOOL * pfSmartSizing)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrLocalPrintingDocName(BSTR pLocalPrintingDocName)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrLocalPrintingDocName(BSTR * pLocalPrintingDocName)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrClipCleanTempDirString(BSTR clipCleanTempDirString)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrClipCleanTempDirString(BSTR * clipCleanTempDirString)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RdpdrClipPasteInfoString(BSTR clipPasteInfoString)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RdpdrClipPasteInfoString(BSTR * clipPasteInfoString)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ClearTextPassword(BSTR rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_DisplayConnectionBar(VARIANT_BOOL pDisplayConnectionBar)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_DisplayConnectionBar(VARIANT_BOOL * pDisplayConnectionBar)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PinConnectionBar(VARIANT_BOOL pPinConnectionBar)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_PinConnectionBar(VARIANT_BOOL * pPinConnectionBar)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_GrabFocusOnConnect(VARIANT_BOOL pfGrabFocusOnConnect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_GrabFocusOnConnect(VARIANT_BOOL * pfGrabFocusOnConnect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_LoadBalanceInfo(BSTR pLBInfo)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_LoadBalanceInfo(BSTR * pLBInfo)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectDrives(VARIANT_BOOL pRedirectDrives)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectDrives(VARIANT_BOOL * pRedirectDrives)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectPrinters(VARIANT_BOOL pRedirectPrinters)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectPrinters(VARIANT_BOOL * pRedirectPrinters)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectPorts(VARIANT_BOOL pRedirectPorts)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectPorts(VARIANT_BOOL * pRedirectPorts)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_RedirectSmartCards(VARIANT_BOOL pRedirectSmartCards)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_RedirectSmartCards(VARIANT_BOOL * pRedirectSmartCards)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCache16BppSize(long pBitmapVirtualCache16BppSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCache16BppSize(long * pBitmapVirtualCache16BppSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_BitmapVirtualCache24BppSize(long pBitmapVirtualCache24BppSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_BitmapVirtualCache24BppSize(long * pBitmapVirtualCache24BppSize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_PerformanceFlags(long pDisableList)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_PerformanceFlags(long * pDisableList)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_ConnectWithEndpoint(VARIANT * rhs)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::put_NotifyTSPublicKey(VARIANT_BOOL pfNotify)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings::get_NotifyTSPublicKey(VARIANT_BOOL * pfNotify)
		{
			return E_NOTIMPL; // TODO
		}

		/* IMsRdpClientAdvancedSettings2 */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_CanAutoReconnect(VARIANT_BOOL * pfCanAutoReconnect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::put_EnableAutoReconnect(VARIANT_BOOL pfEnableAutoReconnect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_EnableAutoReconnect(VARIANT_BOOL * pfEnableAutoReconnect)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::put_MaxReconnectAttempts(long pMaxReconnectAttempts)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings2::get_MaxReconnectAttempts(long * pMaxReconnectAttempts)
		{
			return E_NOTIMPL; // TODO
		}

		/* IMsRdpClientAdvancedSettings3 */
		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::put_ConnectionBarShowMinimizeButton(VARIANT_BOOL pfShowMinimize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::get_ConnectionBarShowMinimizeButton(VARIANT_BOOL * pfShowMinimize)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::put_ConnectionBarShowRestoreButton(VARIANT_BOOL pfShowRestore)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientAdvancedSettings3::get_ConnectionBarShowRestoreButton(VARIANT_BOOL * pfShowRestore)
		{
			return E_NOTIMPL; // TODO
		}

		/* IMsRdpClientAdvancedSettings4 */
        virtual STDMETHODIMP IMsRdpClientAdvancedSettings4::put_AuthenticationLevel(unsigned int puiAuthLevel)
		{
			return E_NOTIMPL; // TODO
		}
        
        virtual STDMETHODIMP IMsRdpClientAdvancedSettings4::get_AuthenticationLevel(unsigned int * puiAuthLevel)
		{
			return E_NOTIMPL; // TODO
		}
	}
	m_advancedSettings;

	template<class Interface> HRESULT GetAdvancedSettings(Interface ** ppAdvSettings)
	{
		addRef();
		*ppAdvSettings = &m_advancedSettings;
		return S_OK;
	}

	/* Secured settings */
	friend class SecuredSettings;

	class SecuredSettings: public MSTSCLib::IMsRdpClientSecuredSettings
	{
	private:
		/* IDispatch type information */
		ITypeInfo * m_dispTypeInfo;

		HRESULT LoadDispTypeInfo()
		{
			if(m_dispTypeInfo)
				return S_OK;

			HRESULT hr = InnerToOuter(this)->LoadTypeLibrary();

			if(FAILED(hr))
				return hr;

			hr = InnerToOuter(this)->m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClientSecuredSettings, &m_dispTypeInfo);

			if(FAILED(hr))
				return hr;

			assert(m_dispTypeInfo);
			return S_OK;
		}

		HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			m_dispTypeInfo->AddRef();
			*ppTI = m_dispTypeInfo;
			return S_OK;
		}


	public:
		~SecuredSettings()
		{
			if(m_dispTypeInfo)
				m_dispTypeInfo->Release();
		}

		/* IUnknown */
		virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
		{
			using namespace MSTSCLib;

			if
			(
				riid == IID_IUnknown ||
				riid == IID_IDispatch ||
				riid == IID_IMsTscSecuredSettings ||
				riid == IID_IMsRdpClientSecuredSettings
			)
			{
				*ppvObject = this;
				InnerToOuter(this)->addRef();
				return S_OK;
			}
			else
			{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
		{
			return InnerToOuter(this)->addRef();
		}

		virtual STDMETHODIMP_(ULONG) IUnknown::Release()
		{
			return InnerToOuter(this)->release();
		}

		/* IDispatch */
		virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
		{
			*pctinfo = 1;
			return S_OK;
		}

		virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
		{
			if(iTInfo != 0)
				return DISP_E_BADINDEX;
		
			return AcquireDispTypeInfo(ppTInfo);
		}

		virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
		{
			HRESULT hr = LoadDispTypeInfo();

			if(FAILED(hr))
				return hr;

			return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
		}

		virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
		{
			return m_dispTypeInfo->Invoke
			(
				static_cast<MSTSCLib::IMsRdpClientSecuredSettings *>(this),
				dispIdMember,
				wFlags,
				pDispParams,
				pVarResult,
				pExcepInfo,
				puArgErr
			);
		}

		/* IMsTscSecuredSettings */
		virtual STDMETHODIMP IMsTscSecuredSettings::put_StartProgram(BSTR pStartProgram)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_StartProgram(BSTR * pStartProgram)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::put_WorkDir(BSTR pWorkDir)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_WorkDir(BSTR * pWorkDir)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::put_FullScreen(long pfFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsTscSecuredSettings::get_FullScreen(long * pfFullScreen)
		{
			return E_NOTIMPL; // TODO
		}

		/* IMsRdpClientSecuredSettings */
		virtual STDMETHODIMP IMsRdpClientSecuredSettings::put_KeyboardHookMode(long pkeyboardHookMode)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::get_KeyboardHookMode(long * pkeyboardHookMode)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::put_AudioRedirectionMode(long pAudioRedirectionMode)
		{
			return E_NOTIMPL; // TODO
		}

		virtual STDMETHODIMP IMsRdpClientSecuredSettings::get_AudioRedirectionMode(long * pAudioRedirectionMode)
		{
			return E_NOTIMPL; // TODO
		}
	}
	m_securedSettings;

	/* Type library loading */
	HRESULT LoadTypeLibrary()
	{
		if(m_typeLib)
			return S_OK;

		WCHAR szPath[MAX_PATH + 1];

		if(GetModuleFileNameW(GetCurrentModule(), szPath, ARRAYSIZE(szPath) - 1) == 0)
			return HRESULT_FROM_WIN32(GetLastError());

		HRESULT hr = LoadTypeLibEx(szPath, REGKIND_NONE, &m_typeLib);

		if(FAILED(hr))
			return hr;

		assert(m_typeLib);
		return S_OK;
	}

	/* IDispatch type information */
	HRESULT LoadDispTypeInfo()
	{
		if(m_dispTypeInfo)
			return S_OK;

		HRESULT hr = LoadTypeLibrary();

		if(FAILED(hr))
			return hr;

		hr = m_typeLib->GetTypeInfoOfGuid(MSTSCLib::IID_IMsRdpClient4, &m_dispTypeInfo);

		if(FAILED(hr))
			return hr;

		assert(m_dispTypeInfo);
		return S_OK;
	}

	HRESULT AcquireDispTypeInfo(ITypeInfo ** ppTI)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		m_dispTypeInfo->AddRef();
		*ppTI = m_dispTypeInfo;
		return S_OK;
	}

public:
	/* Helpers for our various embedded children */
	static RdpClient * InnerToOuter(RdpClientInner * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_inner);
	}

	static RdpClient * InnerToOuter(AdvancedSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_advancedSettings);
	}

	static RdpClient * InnerToOuter(SecuredSettings * innerThis)
	{
		return CONTAINING_RECORD(innerThis, RdpClient, m_securedSettings);
	}

public:
	/* Class factory */
	static HRESULT CreateInstance(REFCLSID classId, IUnknown * punkOuter, LONG * moduleRefCount, IUnknown ** ppObj)
	{
		RdpClient * obj = new RdpClient(classId, punkOuter, moduleRefCount);

		if(obj == NULL)
			return E_OUTOFMEMORY;

		*ppObj = &obj->m_inner;
		return S_OK;
	}

	/* IUnknown */ // DONE
	/*
		NOTE: this is the delegating implementation, to support aggregation. The actual
		implementation is RdpClientInner, above
	*/
	virtual STDMETHODIMP IUnknown::QueryInterface(REFIID riid, void ** ppvObject)
	{
		return m_punkOuter->QueryInterface(riid, ppvObject);
	}

	virtual STDMETHODIMP_(ULONG) IUnknown::AddRef()
	{
		return m_punkOuter->AddRef();
	}

	virtual STDMETHODIMP_(ULONG) IUnknown::Release()
	{
		return m_punkOuter->Release();
	}

	/* IDispatch */ // DONE
	virtual STDMETHODIMP IDispatch::GetTypeInfoCount(UINT * pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}

	virtual STDMETHODIMP IDispatch::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
	{
		if(iTInfo != 0)
			return DISP_E_BADINDEX;
	
		return AcquireDispTypeInfo(ppTInfo);
	}

	virtual STDMETHODIMP IDispatch::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		return m_dispTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
	}

	virtual STDMETHODIMP IDispatch::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
	{
		HRESULT hr = LoadDispTypeInfo();

		if(FAILED(hr))
			return hr;

		return m_dispTypeInfo->Invoke
		(
			static_cast<MSTSCLib::IMsRdpClient4 *>(this),
			dispIdMember,
			wFlags,
			pDispParams,
			pVarResult,
			pExcepInfo,
			puArgErr
		);
	}

	/* IConnectionPointContainer */ // 0/2
	virtual STDMETHODIMP IConnectionPointContainer::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IConnectionPointContainer::FindConnectionPoint(REFIID riid, IConnectionPoint ** ppCP)
	{
		return E_NOTIMPL;
	}

	/* IDataObject */ // 0/9
	virtual STDMETHODIMP IDataObject::GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::GetDataHere(FORMATETC * pformatetc, STGMEDIUM * pmedium)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::QueryGetData(FORMATETC * pformatetc)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::GetCanonicalFormatEtc(FORMATETC * pformatectIn, FORMATETC * pformatetcOut)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::SetData(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::DAdvise(FORMATETC * pformatetc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::DUnadvise(DWORD dwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IDataObject::EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
	{
		return E_NOTIMPL;
	}

	/* IObjectSafety */ // 0/2
	virtual STDMETHODIMP IObjectSafety::GetInterfaceSafetyOptions(REFIID riid, DWORD * pdwSupportedOptions, DWORD * pdwEnabledOptions)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IObjectSafety::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
	{
		return E_NOTIMPL;
	}

	/* IOleControl */ // 0/4
	virtual STDMETHODIMP IOleControl::GetControlInfo(CONTROLINFO * pCI)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleControl::OnMnemonic(MSG * pMsg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleControl::OnAmbientPropertyChange(DISPID dispID)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleControl::FreezeEvents(BOOL bFreeze)
	{
		return E_NOTIMPL;
	}

	/* IOleInPlaceActiveObject */ // 0/5
	virtual STDMETHODIMP IOleInPlaceActiveObject::TranslateAccelerator(LPMSG lpmsg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::OnFrameWindowActivate(BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::OnDocWindowActivate(BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fFrameWindow)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceActiveObject::EnableModeless(BOOL fEnable)
	{
		return E_NOTIMPL;
	}

	/* IOleInPlaceObject */ // 0/4
	virtual STDMETHODIMP IOleInPlaceObject::InPlaceDeactivate()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::UIDeactivate()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleInPlaceObject::ReactivateAndUndo()
	{
		return E_NOTIMPL;
	}

	/* IOleObject */ // 0/21
	virtual STDMETHODIMP IOleObject::SetClientSite(IOleClientSite * pClientSite)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetClientSite(IOleClientSite ** ppClientSite)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::Close(DWORD dwSaveOption)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::SetMoniker(DWORD dwWhichMoniker, IMoniker * pmk)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::InitFromData(IDataObject * pDataObject, BOOL fCreation, DWORD dwReserved)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetClipboardData(DWORD dwReserved, IDataObject ** ppDataObject)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite * pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::EnumVerbs(IEnumOLEVERB ** ppEnumOleVerb)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::Update()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::IsUpToDate()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetUserClassID(CLSID * pClsid)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetUserType(DWORD dwFormOfType, LPOLESTR * pszUserType)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::SetExtent(DWORD dwDrawAspect, SIZEL * psizel)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetExtent(DWORD dwDrawAspect, SIZEL * psizel)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::Advise(IAdviseSink * pAdvSink, DWORD * pdwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::Unadvise(DWORD dwConnection)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::EnumAdvise(IEnumSTATDATA ** ppenumAdvise)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::GetMiscStatus(DWORD dwAspect, DWORD * pdwStatus)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleObject::SetColorScheme(LOGPALETTE * pLogpal)
	{
		return E_NOTIMPL;
	}

	/* IOleWindow */ // 0/2
	virtual STDMETHODIMP IOleWindow::GetWindow(HWND * phwnd)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IOleWindow::ContextSensitiveHelp(BOOL fEnterMode)
	{
		return E_NOTIMPL;
	}

	/* IPersist */ // DONE
	virtual STDMETHODIMP IPersist::GetClassID(CLSID * pClassID)
	{
		*pClassID = m_classId;
		return S_OK;
	}

	/* IPersistPropertyBag */ // 0/3
	virtual STDMETHODIMP IPersistPropertyBag::InitNew()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistPropertyBag::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistPropertyBag::Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		return E_NOTIMPL;
	}

	/* IPersistStorage */ // 0/6
	virtual STDMETHODIMP IPersistStorage::IsDirty()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::InitNew(IStorage * pStg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::Load(IStorage * pStg)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::Save(IStorage * pStgSave, BOOL fSameAsLoad)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::SaveCompleted(IStorage * pStgNew)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStorage::HandsOffStorage()
	{
		return E_NOTIMPL;
	}

	/* IPersistStreamInit */ // 0/5
	virtual STDMETHODIMP IPersistStreamInit::IsDirty()
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::Load(LPSTREAM pStm)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::Save(LPSTREAM pStm, BOOL fClearDirty)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::GetSizeMax(ULARGE_INTEGER * pCbSize)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IPersistStreamInit::InitNew()
	{
		return E_NOTIMPL;
	}

	/* IProvideClassInfo */ // DONE
	virtual STDMETHODIMP IProvideClassInfo::GetClassInfo(ITypeInfo ** ppTI)
	{
		HRESULT hr = LoadTypeLibrary();

		if(FAILED(hr))
			return hr;

		return m_typeLib->GetTypeInfoOfGuid(m_classId, ppTI);
	}

	/* IProvideClassInfo2 */ // DONE
	virtual STDMETHODIMP IProvideClassInfo2::GetGUID(DWORD dwGuidKind, GUID * pGUID)
	{
		if(dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
			return E_INVALIDARG;

		*pGUID = MSTSCLib::DIID_IMsTscAxEvents;
		return S_OK;
	}

	/* IQuickActivate */ // 0/3
	virtual STDMETHODIMP IQuickActivate::QuickActivate(QACONTAINER * pQaContainer, QACONTROL * pQaControl)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IQuickActivate::SetContentExtent(LPSIZEL pSizel)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IQuickActivate::GetContentExtent(LPSIZEL pSizel)
	{
		return E_NOTIMPL;
	}

	/* IViewObject */ // 0/6
	virtual STDMETHODIMP IViewObject::Draw(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL (STDMETHODCALLTYPE * pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::GetColorSet(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LOGPALETTE ** ppColorSet)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DWORD * pdwFreeze)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::Unfreeze(DWORD dwFreeze)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::SetAdvise(DWORD aspects, DWORD advf, IAdviseSink * pAdvSink)
	{
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IViewObject::GetAdvise(DWORD * pAspects, DWORD * pAdvf, IAdviseSink ** ppAdvSink)
	{
		return E_NOTIMPL;
	}

	/* IViewObject2 */ // 0/1
	virtual STDMETHODIMP IViewObject2::GetExtent(DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE * ptd, LPSIZEL lpsizel)
	{
		return E_NOTIMPL;
	}

	/* IMsTscAx */ // ??/30
	virtual STDMETHODIMP IMsTscAx::put_Server(BSTR pServer)
	{
		if(m_nonIdle)
			return E_FAIL;

		return SetStringProperty(m_server, pServer);
	}

	virtual STDMETHODIMP IMsTscAx::get_Server(BSTR * pServer)
	{
		return GetStringProperty(m_server, pServer);
	}

	virtual STDMETHODIMP IMsTscAx::put_Domain(BSTR pDomain)
	{
		if(m_nonIdle)
			return E_FAIL;

		return SetStringProperty(m_domain, pDomain);
	}

	virtual STDMETHODIMP IMsTscAx::get_Domain(BSTR * pDomain)
	{
		return GetStringProperty(m_domain, pDomain);
	}

	virtual STDMETHODIMP IMsTscAx::put_UserName(BSTR pUserName)
	{
		if(m_nonIdle)
			return E_FAIL;

		return SetStringProperty(m_userName, pUserName);
	}

	virtual STDMETHODIMP IMsTscAx::get_UserName(BSTR * pUserName)
	{
		return GetStringProperty(m_userName, pUserName);
	}

	virtual STDMETHODIMP IMsTscAx::put_DisconnectedText(BSTR pDisconnectedText)
	{
		if(m_nonIdle)
			return E_FAIL;

		return SetStringProperty(m_disconnectedText, pDisconnectedText);
	}

	virtual STDMETHODIMP IMsTscAx::get_DisconnectedText(BSTR * pDisconnectedText)
	{
		return GetStringProperty(m_disconnectedText, pDisconnectedText);
	}

	virtual STDMETHODIMP IMsTscAx::put_ConnectingText(BSTR pConnectingText)
	{
		if(m_nonIdle)
			return E_FAIL;

		return SetStringProperty(m_connectingText, pConnectingText);
	}

	virtual STDMETHODIMP IMsTscAx::get_ConnectingText(BSTR * pConnectingText)
	{
		return GetStringProperty(m_connectingText, pConnectingText);
	}

	virtual STDMETHODIMP IMsTscAx::get_Connected(short * pIsConnected)
	{
		* pIsConnected = m_nonIdle;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::put_DesktopWidth(long pVal)
	{
		if(m_nonIdle)
			return E_FAIL;

		// TODO: validate

		width = pVal;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_DesktopWidth(long * pVal)
	{
		* pVal = width;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::put_DesktopHeight(long pVal)
	{
		if(m_nonIdle)
			return E_FAIL;

		// TODO: validate
		
		height = pVal;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_DesktopHeight(long * pVal)
	{
		* pVal = height;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::put_StartConnected(long pfStartConnected)
	{
		// TODO: check that the object isn't running
		m_startConnected = pfStartConnected != 0;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_StartConnected(long * pfStartConnected)
	{
		* pfStartConnected = m_startConnected;
		return S_OK;
	}

	virtual STDMETHODIMP IMsTscAx::get_HorizontalScrollBarVisible(long * pfHScrollVisible)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_VerticalScrollBarVisible(long * pfVScrollVisible)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::put_FullScreenTitle(BSTR rhs)
	{
		return SetStringProperty(m_fullScreenTitle, rhs);
	}

	virtual STDMETHODIMP IMsTscAx::get_CipherStrength(long * pCipherStrength)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_Version(BSTR * pVersion)
	{
		// EASY!!!
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_SecuredSettingsEnabled(long * pSecuredSettingsEnabled)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_SecuredSettings(MSTSCLib::IMsTscSecuredSettings ** ppSecuredSettings)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::get_AdvancedSettings(MSTSCLib::IMsTscAdvancedSettings ** ppAdvSettings)
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsTscAx::get_Debugger(MSTSCLib::IMsTscDebug ** ppDebugger)
	{
		// DO NOT IMPLEMENT
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscAx::Connect()
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::Disconnect()
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::CreateVirtualChannels(BSTR newVal)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscAx::SendOnVirtualChannel(BSTR chanName, BSTR ChanData)
	{
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClient */ // 1/10
	virtual STDMETHODIMP IMsRdpClient::put_ColorDepth(long pcolorDepth)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::get_ColorDepth(long * pcolorDepth)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::get_AdvancedSettings2(MSTSCLib::IMsRdpClientAdvancedSettings ** ppAdvSettings)
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsRdpClient::get_SecuredSettings2(MSTSCLib::IMsRdpClientSecuredSettings ** ppSecuredSettings)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::get_ExtendedDisconnectReason(MSTSCLib::ExtendedDisconnectReasonCode * pExtendedDisconnectReason)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::put_FullScreen(VARIANT_BOOL pfFullScreen)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::get_FullScreen(VARIANT_BOOL * pfFullScreen)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::SetVirtualChannelOptions(BSTR chanName, long chanOptions)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::GetVirtualChannelOptions(BSTR chanName, long * pChanOptions)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient::RequestClose(MSTSCLib::ControlCloseStatus * pCloseStatus)
	{
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClient2 */ // 1/3
	virtual STDMETHODIMP IMsRdpClient2::get_AdvancedSettings3(MSTSCLib::IMsRdpClientAdvancedSettings2 ** ppAdvSettings)
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	virtual STDMETHODIMP IMsRdpClient2::put_ConnectedStatusText(BSTR pConnectedStatusText)
	{
		// EASY!!!
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClient2::get_ConnectedStatusText(BSTR * pConnectedStatusText)
	{
		// EASY!!!
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClient3 */ // DONE
	virtual STDMETHODIMP IMsRdpClient3::get_AdvancedSettings4(MSTSCLib::IMsRdpClientAdvancedSettings3 ** ppAdvSettings)
	{
		return GetAdvancedSettings(ppAdvSettings);
	}

	/* IMsRdpClient4 */ // DONE
    virtual STDMETHODIMP IMsRdpClient4::get_AdvancedSettings5(MSTSCLib::IMsRdpClientAdvancedSettings4 ** ppAdvSettings5)
	{
		return GetAdvancedSettings(ppAdvSettings5);
	}

	/* IMsTscNonScriptable */ // 8/10
	virtual STDMETHODIMP IMsTscNonScriptable::put_ClearTextPassword(BSTR rhs)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_PortablePassword(BSTR pPortablePass)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_PortablePassword(BSTR * pPortablePass)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_PortableSalt(BSTR pPortableSalt)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_PortableSalt(BSTR * pPortableSalt)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_BinaryPassword(BSTR pBinaryPassword)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_BinaryPassword(BSTR * pBinaryPassword)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::put_BinarySalt(BSTR pSalt)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::get_BinarySalt(BSTR * pSalt)
	{
		// OBSOLETE
		return E_NOTIMPL;
	}

	virtual STDMETHODIMP IMsTscNonScriptable::ResetPassword()
	{
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClientNonScriptable */ // 0/2
	virtual STDMETHODIMP IMsRdpClientNonScriptable::NotifyRedirectDeviceChange(MSTSCLib::UINT_PTR wParam, MSTSCLib::LONG_PTR lParam)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClientNonScriptable::SendKeys(long numKeys, VARIANT_BOOL * pbArrayKeyUp, long * plKeyData)
	{
		// NOTE: the keys must be sent in a single, atomic sequence
		// TODO: acquire the write lock
		return E_NOTIMPL; // TODO
	}

	/* IMsRdpClientNonScriptable2 */ // 0/2
	virtual STDMETHODIMP IMsRdpClientNonScriptable2::put_UIParentWindowHandle(HWND phwndUIParentWindowHandle)
	{
		return E_NOTIMPL; // TODO
	}

	virtual STDMETHODIMP IMsRdpClientNonScriptable2::get_UIParentWindowHandle(HWND * phwndUIParentWindowHandle)
	{
		return E_NOTIMPL; // TODO
	}

};

#pragma warning(pop)

// EOF
