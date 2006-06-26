#include "stdafx.h"

namespace
{
	using namespace MSTSCLib;

	typedef HRESULT (STDAPICALLTYPE * PFNDLLGETCLASSOBJECT)(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR * ppv);
	typedef HRESULT (STDAPICALLTYPE * PFNDLLCANUNLOADNOW)(void);
	typedef ULONG (STDAPICALLTYPE * PFNDLLGETTSCCTLVER)(void);

	PFNDLLGETCLASSOBJECT pfnDllGetClassObject = NULL;
	PFNDLLCANUNLOADNOW pfnDllCanUnloadNow = NULL;
	PFNDLLGETTSCCTLVER pfnDllGetTscCtlVer = NULL;

	HMODULE hmMstscax = NULL;

	void init()
	{
		if(hmMstscax)
			return;

		hmMstscax = LoadLibrary(TEXT("mstscax_.dll"));
		pfnDllGetClassObject = (PFNDLLGETCLASSOBJECT)GetProcAddress(hmMstscax, "DllGetClassObject");
		pfnDllCanUnloadNow = (PFNDLLCANUNLOADNOW)GetProcAddress(hmMstscax, "DllCanUnloadNow");
		pfnDllGetTscCtlVer = (PFNDLLGETTSCCTLVER)GetProcAddress(hmMstscax, "DllGetTscCtlVer");
	}

	void dbgprintf(LPCTSTR fmt, ...)
	{
		TCHAR buf[0x1000];
		
		va_list args;
		va_start(args, fmt);
		StringCbVPrintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		OutputDebugString(buf);
	}

#if 0
	const IID MsTscAxIIDs[] =
	{
		IID_IMsRdpClient,
		IID_IMsTscAx,
		//IID_IMsTscAxEvents,
		IID_IMsTscNonScriptable,
		IID_IMsRdpClientNonScriptable,
	};

	const IID MsRdpClient[] =
	{
		IID_IMsRdpClient,
		IID_IMsTscAx,
		//IID_IMsTscAxEvents,
		IID_IMsTscNonScriptable,
		IID_IMsRdpClientNonScriptable,
	};

	const IID MsRdpClient2[] =
	{
		IID_IMsRdpClient2,
		IID_IMsRdpClient,
		IID_IMsTscAx,
		//IID_IMsTscAxEvents,
		IID_IMsTscNonScriptable,
		IID_IMsRdpClientNonScriptable,
	};

	const IID MsRdpClient3[] =
	{
		IID_IMsRdpClient3,
		IID_IMsRdpClient2,
		IID_IMsRdpClient,
		IID_IMsTscAx,
		//IID_IMsTscAxEvents,
		IID_IMsTscNonScriptable,
		IID_IMsRdpClientNonScriptable,
	};

	const IID MsRdpClient4[] =
	{
		IID_IMsRdpClient4,
		IID_IMsRdpClient3,
		IID_IMsRdpClient2,
		IID_IMsRdpClient,
		IID_IMsTscAx,
		//IID_IMsTscAxEvents,
		IID_IMsTscNonScriptable,
		IID_IMsRdpClientNonScriptable,
		IID_IMsRdpClientNonScriptable2,
	};
#endif

	std::wstring UUIDToString(const UUID& uuid)
	{
		std::wstring s;
		LPOLESTR str;
		StringFromCLSID(uuid, &str);
		s += str;
		CoTaskMemFree(str);
		return s;
	}

	std::wstring MonikerToString(IMoniker * pmk)
	{
		LPOLESTR pszName = NULL;

		if(SUCCEEDED(pmk->GetDisplayName(NULL, NULL, &pszName)))
		{
			std::wstring s(pszName);
			CoTaskMemFree(pszName);
			return s;
		}
		else
			return std::wstring(L"<error>");
	}

	std::basic_string<TCHAR> RectToString(const RECT& rc)
	{
		std::basic_ostringstream<TCHAR> o;
		o << "{" << " left:" << rc.left << " top:" << rc.top << " right:" << rc.right << " bottom:" << rc.bottom << " }";
		return o.str();
	}

	std::basic_string<TCHAR> RectToString(const RECTL& rc)
	{
		std::basic_ostringstream<TCHAR> o;
		o << "{" << " left:" << rc.left << " top:" << rc.top << " right:" << rc.right << " bottom:" << rc.bottom << " }";
		return o.str();
	}

	std::basic_string<TCHAR> SizeToString(const SIZE& sz)
	{
		std::basic_ostringstream<TCHAR> o;
		o << "{ " << " cx:" << sz.cx << " cy:" << sz.cy << " }";
		return o.str();
	}

	template<class T> LPCTSTR BooleanToString(const T& X)
	{
		return X ? TEXT("true") : TEXT("false");
	}

#pragma warning(disable:4584)

	class CoClass:
		/* Standard interfaces */
		public IUnknown,
		public IDispatch,
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
		public ISpecifyPropertyPages,
		public IViewObject,
		public IViewObject2,

		/* RDP client interfaces */
		public IMsRdpClient4,
		public IMsRdpClientNonScriptable2
	{
	private:
		LONG m_refCount;

		IUnknown * m_IUnknown;

		IDispatch * m_IDispatch;
		IConnectionPointContainer * m_IConnectionPointContainer;
		IDataObject * m_IDataObject;
		IObjectSafety * m_IObjectSafety;
		IOleControl * m_IOleControl;
		IOleInPlaceActiveObject * m_IOleInPlaceActiveObject;
		IOleInPlaceObject * m_IOleInPlaceObject;
		IOleObject * m_IOleObject;
		IOleWindow * m_IOleWindow;
		IPersist * m_IPersist;
		IPersistPropertyBag * m_IPersistPropertyBag;
		IPersistStorage * m_IPersistStorage;
		IPersistStreamInit * m_IPersistStreamInit;
		IProvideClassInfo * m_IProvideClassInfo;
		IProvideClassInfo2 * m_IProvideClassInfo2;
		IQuickActivate * m_IQuickActivate;
		ISpecifyPropertyPages * m_ISpecifyPropertyPages;
		IViewObject * m_IViewObject;
		IViewObject2 * m_IViewObject2;

		IMsRdpClient * m_IMsRdpClient;
		IMsRdpClient2 * m_IMsRdpClient2;
		IMsRdpClient3 * m_IMsRdpClient3;
		IMsRdpClient4 * m_IMsRdpClient4;
		IMsTscAx * m_IMsTscAx;
		IMsTscNonScriptable * m_IMsTscNonScriptable;
		IMsRdpClientNonScriptable * m_IMsRdpClientNonScriptable;
		IMsRdpClientNonScriptable2 * m_IMsRdpClientNonScriptable2;

		IDispatch * getIDispatch()
		{
			if(m_IDispatch)
				return m_IDispatch;

			if(m_IMsRdpClient)
				m_IDispatch = m_IMsRdpClient;
			else if(m_IMsRdpClient2)
				m_IDispatch = m_IMsRdpClient2;
			else if(m_IMsRdpClient3)
				m_IDispatch = m_IMsRdpClient3;
			else if(m_IMsRdpClient4)
				m_IDispatch = m_IMsRdpClient4;
			else if(m_IMsTscAx)
				m_IDispatch = m_IMsTscAx;

			if(m_IDispatch)
			{
				m_IDispatch->AddRef();
				return m_IDispatch;
			}

			if(SUCCEEDED(m_IUnknown->QueryInterface(&m_IDispatch)))
				return m_IDispatch;

			return NULL;
		}

		IConnectionPointContainer * getIConnectionPointContainer()
		{
			if(m_IConnectionPointContainer)
				return m_IConnectionPointContainer;

			m_IUnknown->QueryInterface(&m_IConnectionPointContainer);
			return m_IConnectionPointContainer;

		}

		IDataObject * getIDataObject()
		{
			if(m_IDataObject)
				return m_IDataObject;

			m_IUnknown->QueryInterface(&m_IDataObject);
			return m_IDataObject;
		}

		IObjectSafety * getIObjectSafety()
		{
			if(m_IObjectSafety)
				return m_IObjectSafety;

			m_IUnknown->QueryInterface(&m_IObjectSafety);
			return m_IObjectSafety;
		}

		IOleControl * getIOleControl()
		{
			if(m_IOleControl)
				return m_IOleControl;

			m_IUnknown->QueryInterface(&m_IOleControl);
			return m_IOleControl;
		}

		IOleInPlaceActiveObject * getIOleInPlaceActiveObject()
		{
			if(m_IOleInPlaceActiveObject)
				return m_IOleInPlaceActiveObject;

			m_IUnknown->QueryInterface(&m_IOleInPlaceActiveObject);
			return m_IOleInPlaceActiveObject;
		}

		IOleInPlaceObject * getIOleInPlaceObject()
		{
			if(m_IOleInPlaceObject)
				return m_IOleInPlaceObject;

			m_IUnknown->QueryInterface(&m_IOleInPlaceObject);
			return m_IOleInPlaceObject;
		}

		IOleObject * getIOleObject()
		{
			if(m_IOleObject)
				return m_IOleObject;

			m_IUnknown->QueryInterface(&m_IOleObject);
			return m_IOleObject;
		}

		IOleWindow * getIOleWindow()
		{
			if(m_IOleWindow)
				return m_IOleWindow;

			if(m_IOleInPlaceActiveObject)
				m_IOleWindow = m_IOleInPlaceActiveObject;

			if(m_IOleWindow)
			{
				m_IOleWindow->AddRef();
				return m_IOleWindow;
			}

			m_IUnknown->QueryInterface(&m_IOleWindow);
			return m_IOleWindow;
		}

		IPersist * getIPersist()
		{
			if(m_IPersist)
				return m_IPersist;

			if(m_IPersistPropertyBag)
				m_IPersist = m_IPersistPropertyBag;
			else if(m_IPersistStorage)
				m_IPersist = m_IPersistStorage;
			else if(m_IPersistStreamInit)
				m_IPersist = m_IPersistStreamInit;

			if(m_IPersist)
			{
				m_IPersist->AddRef();
				return m_IPersist;
			}

			m_IUnknown->QueryInterface(&m_IPersist);
			return m_IPersist;
		}

		IPersistPropertyBag * getIPersistPropertyBag()
		{
			if(m_IPersistPropertyBag)
				return m_IPersistPropertyBag;

			m_IUnknown->QueryInterface(&m_IPersistPropertyBag);
			return m_IPersistPropertyBag;
		}

		IPersistStorage * getIPersistStorage()
		{
			if(m_IPersistStorage)
				return m_IPersistStorage;

			m_IUnknown->QueryInterface(&m_IPersistStorage);
			return m_IPersistStorage;
		}

		IPersistStreamInit * getIPersistStreamInit()
		{
			if(m_IPersistStreamInit)
				return m_IPersistStreamInit;

			m_IUnknown->QueryInterface(&m_IPersistStreamInit);
			return m_IPersistStreamInit;
		}

		IProvideClassInfo * getIProvideClassInfo()
		{
			if(m_IProvideClassInfo)
				return m_IProvideClassInfo;

			if(m_IProvideClassInfo2)
				m_IProvideClassInfo = m_IProvideClassInfo2;

			if(m_IProvideClassInfo)
			{
				m_IProvideClassInfo->AddRef();
				return m_IProvideClassInfo;
			}

			m_IUnknown->QueryInterface(&m_IProvideClassInfo);
			return m_IProvideClassInfo;
		}

		IProvideClassInfo2 * getIProvideClassInfo2()
		{
			if(m_IProvideClassInfo2)
				return m_IProvideClassInfo2;

			m_IUnknown->QueryInterface(&m_IProvideClassInfo2);
			return m_IProvideClassInfo2;
		}

		IQuickActivate * getIQuickActivate()
		{
			if(m_IQuickActivate)
				return m_IQuickActivate;

			m_IUnknown->QueryInterface(&m_IQuickActivate);
			return m_IQuickActivate;
		}

		ISpecifyPropertyPages * getISpecifyPropertyPages()
		{
			if(m_ISpecifyPropertyPages)
				return m_ISpecifyPropertyPages;

			m_IUnknown->QueryInterface(&m_ISpecifyPropertyPages);
			return m_ISpecifyPropertyPages;
		}

		IViewObject * getIViewObject()
		{
			if(m_IViewObject)
				return m_IViewObject;

			if(m_IViewObject2)
				m_IViewObject = m_IViewObject2;

			if(m_IViewObject)
			{
				m_IViewObject->AddRef();
				return m_IViewObject;
			}

			m_IUnknown->QueryInterface(&m_IViewObject);
			return m_IViewObject;
		}

		IViewObject2 * getIViewObject2()
		{
			if(m_IViewObject2)
				return m_IViewObject2;

			m_IUnknown->QueryInterface(&m_IViewObject2);
			return m_IViewObject2;
		}

		IMsRdpClient * getIMsRdpClient()
		{
			if(m_IMsRdpClient)
				return m_IMsRdpClient;

			if(m_IMsRdpClient2)
				m_IMsRdpClient = m_IMsRdpClient2;
			else if(m_IMsRdpClient3)
				m_IMsRdpClient = m_IMsRdpClient3;
			else if(m_IMsRdpClient4)
				m_IMsRdpClient = m_IMsRdpClient4;

			if(m_IMsRdpClient)
			{
				m_IMsRdpClient->AddRef();
				return m_IMsRdpClient;
			}

			m_IUnknown->QueryInterface(&m_IMsRdpClient);
			return m_IMsRdpClient;
		}

		IMsRdpClient2 * getIMsRdpClient2()
		{
			if(m_IMsRdpClient2)
				return m_IMsRdpClient2;

			if(m_IMsRdpClient3)
				m_IMsRdpClient2 = m_IMsRdpClient3;
			else if(m_IMsRdpClient4)
				m_IMsRdpClient2 = m_IMsRdpClient4;

			if(m_IMsRdpClient2)
			{
				m_IMsRdpClient2->AddRef();
				return m_IMsRdpClient2;
			}

			m_IUnknown->QueryInterface(&m_IMsRdpClient2);
			return m_IMsRdpClient2;
		}

		IMsRdpClient3 * getIMsRdpClient3()
		{
			if(m_IMsRdpClient3)
				return m_IMsRdpClient3;

			if(m_IMsRdpClient4)
				m_IMsRdpClient3 = m_IMsRdpClient4;

			if(m_IMsRdpClient3)
			{
				m_IMsRdpClient3->AddRef();
				return m_IMsRdpClient3;
			}

			m_IUnknown->QueryInterface(&m_IMsRdpClient3);
			return m_IMsRdpClient3;
		}

		IMsRdpClient4 * getIMsRdpClient4()
		{
			if(m_IMsRdpClient4)
				return m_IMsRdpClient4;

			m_IUnknown->QueryInterface(&m_IMsRdpClient4);
			return m_IMsRdpClient4;
		}

		IMsTscAx * getIMsTscAx()
		{
			if(m_IMsTscAx)
				return m_IMsTscAx;

			if(m_IMsRdpClient)
				m_IMsTscAx = m_IMsRdpClient;
			else if(m_IMsRdpClient2)
				m_IMsTscAx = m_IMsRdpClient2;
			else if(m_IMsRdpClient3)
				m_IMsTscAx = m_IMsRdpClient3;
			else if(m_IMsRdpClient4)
				m_IMsTscAx = m_IMsRdpClient4;

			if(m_IMsTscAx)
			{
				m_IMsTscAx->AddRef();
				return m_IMsTscAx;
			}

			m_IUnknown->QueryInterface(&m_IMsTscAx);
			return m_IMsTscAx;
		}

		IMsTscNonScriptable * getIMsTscNonScriptable()
		{
			if(m_IMsTscNonScriptable)
				return m_IMsTscNonScriptable;

			if(m_IMsRdpClientNonScriptable)
				m_IMsTscNonScriptable = m_IMsRdpClientNonScriptable;
			else if(m_IMsRdpClientNonScriptable2)
				m_IMsTscNonScriptable = m_IMsRdpClientNonScriptable2;

			if(m_IMsTscNonScriptable)
			{
				m_IMsTscNonScriptable->AddRef();
				return m_IMsTscNonScriptable;
			}

			m_IUnknown->QueryInterface(&m_IMsTscNonScriptable);
			return m_IMsTscNonScriptable;
		}

		IMsRdpClientNonScriptable * getIMsRdpClientNonScriptable()
		{
			if(m_IMsRdpClientNonScriptable)
				return m_IMsRdpClientNonScriptable;

			if(m_IMsRdpClientNonScriptable2)
				m_IMsRdpClientNonScriptable = m_IMsRdpClientNonScriptable2;

			if(m_IMsRdpClientNonScriptable)
			{
				m_IMsRdpClientNonScriptable->AddRef();
				return m_IMsRdpClientNonScriptable;
			}

			m_IUnknown->QueryInterface(&m_IMsRdpClientNonScriptable);
			return m_IMsRdpClientNonScriptable;
		}

		IMsRdpClientNonScriptable2 * getIMsRdpClientNonScriptable2()
		{
			if(m_IMsRdpClientNonScriptable2)
				return m_IMsRdpClientNonScriptable2;

			m_IUnknown->QueryInterface(&m_IMsRdpClientNonScriptable2);
			return m_IMsRdpClientNonScriptable2;
		}

	public:
		CoClass(IUnknown * pUnknw):
			m_refCount(1),
			m_IUnknown(pUnknw),
			m_IDispatch(NULL),
			m_IConnectionPointContainer(NULL),
			m_IDataObject(NULL),
			m_IObjectSafety(NULL),
			m_IOleControl(NULL),
			m_IOleInPlaceActiveObject(NULL),
			m_IOleInPlaceObject(NULL),
			m_IOleObject(NULL),
			m_IOleWindow(NULL),
			m_IPersist(NULL),
			m_IPersistPropertyBag(NULL),
			m_IPersistStorage(NULL),
			m_IPersistStreamInit(NULL),
			m_IProvideClassInfo(NULL),
			m_IProvideClassInfo2(NULL),
			m_IQuickActivate(NULL),
			m_ISpecifyPropertyPages(NULL),
			m_IViewObject(NULL),
			m_IViewObject2(NULL),
			m_IMsRdpClient(NULL),
			m_IMsRdpClient2(NULL),
			m_IMsRdpClient3(NULL),
			m_IMsRdpClient4(NULL),
			m_IMsTscAx(NULL),
			m_IMsTscNonScriptable(NULL),
			m_IMsRdpClientNonScriptable(NULL),
			m_IMsRdpClientNonScriptable2(NULL)
		{
			m_IUnknown->AddRef();
		}

		~CoClass()
		{
			if(m_IUnknown)
				m_IUnknown->Release();

			if(m_IDispatch)
				m_IDispatch->Release();

			if(m_IConnectionPointContainer)
				m_IConnectionPointContainer->Release();

			if(m_IDataObject)
				m_IDataObject->Release();

			if(m_IObjectSafety)
				m_IObjectSafety->Release();

			if(m_IOleControl)
				m_IOleControl->Release();

			if(m_IOleInPlaceActiveObject)
				m_IOleInPlaceActiveObject->Release();

			if(m_IOleInPlaceObject)
				m_IOleInPlaceObject->Release();

			if(m_IOleObject)
				m_IOleObject->Release();

			if(m_IOleWindow)
				m_IOleWindow->Release();

			if(m_IPersist)
				m_IPersist->Release();

			if(m_IPersistPropertyBag)
				m_IPersistPropertyBag->Release();

			if(m_IPersistStorage)
				m_IPersistStorage->Release();

			if(m_IPersistStreamInit)
				m_IPersistStreamInit->Release();

			if(m_IProvideClassInfo)
				m_IProvideClassInfo->Release();

			if(m_IProvideClassInfo2)
				m_IProvideClassInfo2->Release();

			if(m_IQuickActivate)
				m_IQuickActivate->Release();

			if(m_ISpecifyPropertyPages)
				m_ISpecifyPropertyPages->Release();

			if(m_IViewObject)
				m_IViewObject->Release();

			if(m_IViewObject2)
				m_IViewObject2->Release();

			if(m_IMsRdpClient)
				m_IMsRdpClient->Release();

			if(m_IMsRdpClient2)
				m_IMsRdpClient2->Release();

			if(m_IMsRdpClient3)
				m_IMsRdpClient3->Release();

			if(m_IMsRdpClient4)
				m_IMsRdpClient4->Release();

			if(m_IMsTscAx)
				m_IMsTscAx->Release();

			if(m_IMsTscNonScriptable)
				m_IMsTscNonScriptable->Release();

			if(m_IMsRdpClientNonScriptable)
				m_IMsRdpClientNonScriptable->Release();

			if(m_IMsRdpClientNonScriptable2)
				m_IMsRdpClientNonScriptable2->Release();
		}

		/* IUnknown */
	public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
		{
			HRESULT hr;
			IUnknown * pvObject = NULL;
			
			dbgprintf(TEXT("IUnknown::QueryInterface(%ls, %p)"), UUIDToString(riid).c_str(), ppvObject);

#define QIBEGIN() \
	if(riid == IID_IUnknown) \
	{ \
		hr = S_OK; \
		pvObject = (IUnknown *)(this); \
	}

#define QI(I) \
	else if(riid == IID_ ## I) \
	{ \
		if(m_ ## I) \
		{ \
			m_ ## I->AddRef(); \
			hr = S_OK; \
		} \
		else \
		{ \
			hr = m_IUnknown->QueryInterface(&m_ ## I); \
		} \
 \
		if(SUCCEEDED(hr)) \
			pvObject = static_cast<I *>(this); \
	}

#define QIEND() \
	else \
	{ \
		hr = E_NOINTERFACE; \
		pvObject = NULL; \
	}

			QIBEGIN()

			/* Standard interfaces */
			QI(IDispatch)
			QI(IConnectionPointContainer)
			QI(IDataObject)
			QI(IObjectSafety)
			QI(IOleControl)
			QI(IOleInPlaceActiveObject)
			QI(IOleInPlaceObject)
			QI(IOleObject)
			QI(IOleWindow)
			QI(IPersist)
			QI(IPersistPropertyBag)
			QI(IPersistStorage)
			QI(IPersistStreamInit)
			QI(IProvideClassInfo)
			QI(IProvideClassInfo2)
			QI(IQuickActivate)
			QI(ISpecifyPropertyPages)
			QI(IViewObject)
			QI(IViewObject2)

			/* Terminal services client */
			QI(IMsRdpClient)
			QI(IMsRdpClient2)
			QI(IMsRdpClient3)
			QI(IMsRdpClient4)
			QI(IMsTscAx)
			QI(IMsTscNonScriptable)
			QI(IMsRdpClientNonScriptable)
			QI(IMsRdpClientNonScriptable2)
			QIEND()

#undef QIBEGIN
#undef QIEND
#undef QI

			if(SUCCEEDED(hr))
			{
				assert(pvObject);
				pvObject->AddRef();
			}
			else
			{
				assert(pvObject == NULL);
			}

			*ppvObject = pvObject;

			dbgprintf(TEXT("IUnknown::QueryInterface -> %08X, ppvObject = %p"), hr, *ppvObject);
			return hr;
		}

        virtual ULONG STDMETHODCALLTYPE AddRef(void)
		{
			return InterlockedIncrement(&m_refCount);
		}

        virtual ULONG STDMETHODCALLTYPE Release(void)
		{
			LONG n = InterlockedDecrement(&m_refCount);

			if(n == 0)
				delete this;

			return n;
		}

		/* IDispatch */
	private:
		static std::basic_string<TCHAR> VariantToString(const VARIANT& var)
		{
			std::basic_ostringstream<TCHAR> o;

			switch(var.vt & VT_TYPEMASK)
			{
			case VT_EMPTY:           o << "<empty>"; break;
			case VT_NULL:            o << "<null>"; break;
			case VT_I2:              o << "short"; break;
			case VT_I4:              o << "long"; break;
			case VT_R4:              o << "float"; break;
			case VT_R8:              o << "double"; break;
			case VT_CY:              o << "CURRENCY"; break;
			case VT_DATE:            o << "DATE"; break;
			case VT_BSTR:            o << "string"; break;
			case VT_DISPATCH:        o << "IDispatch *"; break;
			case VT_ERROR:           o << "SCODE"; break;
			case VT_BOOL:            o << "bool"; break;
			case VT_VARIANT:         o << "VARIANT *"; break;
			case VT_UNKNOWN:         o << "IUnknown *"; break;
			case VT_DECIMAL:         o << "DECIMAL"; break;
			case VT_I1:              o << "char"; break;
			case VT_UI1:             o << "unsigned char"; break;
			case VT_UI2:             o << "unsigned short"; break;
			case VT_UI4:             o << "unsigned long"; break;
			case VT_I8:              o << "long long"; break;
			case VT_UI8:             o << "unsigned long long"; break;
			case VT_INT:             o << "int"; break;
			case VT_UINT:            o << "unsigned int"; break;
			case VT_VOID:            o << "void"; break;
			case VT_HRESULT:         o << "HRESULT"; break;
			case VT_PTR:             o << "void *"; break;
			case VT_SAFEARRAY:       o << "SAFEARRAY *"; break;
			case VT_LPSTR:           o << "LPSTR"; break;
			case VT_LPWSTR:          o << "LPWSTR"; break;
			case VT_RECORD:          o << "struct { }"; break;
			case VT_INT_PTR:         o << "intptr_t"; break;
			case VT_UINT_PTR:        o << "uintptr_t"; break;
			case VT_FILETIME:        o << "FILETIME"; break;
			default:                 o << "???"; break;
			}

			if(var.vt & VT_ARRAY)
				o << "[]";
			else if(var.vt & VT_BYREF)
				o << " *";
			else
			{
				switch(var.vt & VT_TYPEMASK)
				{
				case VT_EMPTY:
				case VT_NULL:
				case VT_RECORD:
				case VT_VOID:

					// TODO
				case VT_CY:
				case VT_DATE:
				case VT_DECIMAL:
				case VT_FILETIME:
					break;

				default:
					o << " = ";
				}

				switch(var.vt & VT_TYPEMASK)
				{
				case VT_I2:       o << var.iVal; break;
				case VT_I4:       o << var.lVal; break;
				case VT_R4:       o << var.fltVal; break;
				case VT_R8:       o << var.dblVal; break;
				case VT_BSTR:     o << std::wstring(var.bstrVal, var.bstrVal + SysStringLen(var.bstrVal)); break;
				case VT_BOOL:     o << var.boolVal ? "true" : "false"; break;
				case VT_I1:       o << int(var.cVal); break;
				case VT_UI1:      o << unsigned int(var.bVal); break;
				case VT_UI2:      o << var.uiVal; break;
				case VT_UI4:      o << var.ulVal; break;
				case VT_I8:       o << var.llVal; break;
				case VT_UI8:      o << var.ullVal; break;
				case VT_INT:      o << var.intVal; break;
				case VT_UINT:     o << var.uintVal; break;
				case VT_LPSTR:    o << LPSTR(var.byref); break;
				case VT_LPWSTR:   o << LPWSTR(var.byref); break;
				case VT_INT_PTR:  o << var.intVal; break; // BUGBUG
				case VT_UINT_PTR: o << var.uintVal; break; // BUGBUG

				case VT_DISPATCH:
				case VT_VARIANT:
				case VT_UNKNOWN:
				case VT_PTR:
				case VT_SAFEARRAY:
				case VT_RECORD:
					o << var.byref; break;

				case VT_ERROR:
				case VT_HRESULT:
					o << std::hex << var.ulVal; break;

				default:
					assert(0);
				}
			}

			return o.str();
		}

		static void FreeExcepInfo(const EXCEPINFO& excepInfo)
		{
			if(excepInfo.bstrSource)
				SysFreeString(excepInfo.bstrSource);

			if(excepInfo.bstrDescription)
				SysFreeString(excepInfo.bstrDescription);

			if(excepInfo.bstrHelpFile)
				SysFreeString(excepInfo.bstrHelpFile);
		}

		static std::basic_string<TCHAR> ExcepInfoToString(const EXCEPINFO& excepInfo)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "{";
			o << " code: " << excepInfo.wCode << " from: " << std::basic_string<OLECHAR>(excepInfo.bstrSource, excepInfo.bstrSource + SysStringLen(excepInfo.bstrSource));

			BSTR bstrDescription = NULL;

			if(excepInfo.bstrDescription)
				bstrDescription = excepInfo.bstrDescription;
			else if(excepInfo.pfnDeferredFillIn)
			{
				EXCEPINFO excepInfoCopy = excepInfo;

				if(SUCCEEDED(excepInfoCopy.pfnDeferredFillIn(&excepInfoCopy)) && excepInfoCopy.bstrDescription)
				{
					bstrDescription = excepInfoCopy.bstrDescription;
					excepInfoCopy.bstrDescription = NULL;
				}

				if(excepInfoCopy.bstrSource == excepInfo.bstrSource)
					excepInfoCopy.bstrSource = NULL;

				if(excepInfoCopy.bstrHelpFile == excepInfo.bstrDescription)
					excepInfoCopy.bstrDescription = NULL;

				FreeExcepInfo(excepInfoCopy);
			}

			if(bstrDescription)
			{
				o << " msg: " << std::basic_string<OLECHAR>(bstrDescription, bstrDescription + SysStringLen(bstrDescription));

				if(excepInfo.bstrDescription == NULL)
					SysFreeString(bstrDescription);
			}

			o << " }";

			return o.str().c_str();
		}

	public:
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT * pctinfo)
		{
			IDispatch * pIDispatch = getIDispatch();
			dbgprintf(TEXT("IDispatch::GetTypeInfoCount(%p)"), pctinfo);
			HRESULT hr = pIDispatch->GetTypeInfoCount(pctinfo);
			dbgprintf(TEXT("IDispatch::GetTypeInfoCount -> %08X, ctinfo = %lu"), hr, *pctinfo);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
		{
			IDispatch * pIDispatch = getIDispatch();
			dbgprintf(TEXT("IDispatch::GetTypeInfo(%lu, %08X, %p)"), iTInfo, lcid, ppTInfo);
			HRESULT hr = pIDispatch->GetTypeInfo(iTInfo, lcid, ppTInfo);
			dbgprintf(TEXT("IDispatch::GetTypeInfo -> %08X, pTInfo = %p"), hr, *ppTInfo);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgDispId)
		{
			IDispatch * pIDispatch = getIDispatch();
			std::wstring strtemp;

			std::wostringstream strtempo;

			strtemp.resize(0);
			strtemp += L"[ ";

			for(UINT i = 0; i < cNames; ++ i)
			{
				if(i)
					strtemp += L", ";

				strtemp += rgszNames[i];
			}

			strtemp += L" ]";

			dbgprintf(TEXT("IDispatch::GetIDsOfNames(%ls, %ls, %lu, %08X, %p)"), UUIDToString(riid).c_str(), strtemp.c_str(), cNames, lcid, rgDispId);
			HRESULT hr = pIDispatch->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);

			strtemp.resize(0);
			strtempo.str(strtemp);

			strtempo << L"[ ";

			for(UINT i = 0; i < cNames; ++ i)
			{
				if(i)
					strtempo << L", ";

				strtempo << rgDispId[i];
			}

			strtempo << L" ]";

			dbgprintf(TEXT("IDispatch::GetIDsOfNames -> %08X, rgDispId = [%ls]"), hr, strtemp.c_str());

			return hr;
		}
        virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr)
		{
			IDispatch * pIDispatch = getIDispatch();

			std::basic_ostringstream<TCHAR> strtempo;

			strtempo << L"{\n";

			for(unsigned int i = pDispParams->cArgs, j = pDispParams->cNamedArgs; j < pDispParams->cArgs; -- i, ++ j)
			{
				strtempo << L"\t";
				strtempo << VariantToString(pDispParams->rgvarg[i - 1]);
				strtempo << L"\n";
			}

			for(unsigned int i = pDispParams->cArgs - pDispParams->cNamedArgs; i > 0; -- i)
			{
				strtempo << L"\t";
				strtempo << L"["; strtempo << pDispParams->rgdispidNamedArgs[i - 1]; strtempo << L"] => ";
				strtempo << VariantToString(pDispParams->rgvarg[i - 1]);
				strtempo << L"\n";
			}

			strtempo << L"}";

			dbgprintf(TEXT("IDispatch::Invoke(%ld, %ls, %08X, %04X, %s, %p, %p, %p)"), dispIdMember, UUIDToString(riid), lcid, wFlags, strtempo.str().c_str(), pVarResult, pExcepInfo, puArgErr);

			VARIANT VarResult = { };
			EXCEPINFO ExcepInfo = { };

			if(pVarResult == NULL)
				pVarResult = &VarResult;

			if(pExcepInfo == NULL)
				pExcepInfo = &ExcepInfo;

			HRESULT hr = pIDispatch->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

			dbgprintf(TEXT("IDispatch::Invoke -> %08X, returns %s, throws %s"), hr, VariantToString(*pVarResult).c_str(), ExcepInfoToString(*pExcepInfo).c_str());

			FreeExcepInfo(ExcepInfo);

			return hr;
		}

		/* IConnectionPointContainer */
	public:
		virtual HRESULT STDMETHODCALLTYPE EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
		{
			IConnectionPointContainer * pIConnectionPointContainer = getIConnectionPointContainer();
			dbgprintf(TEXT("IConnectionPointContainer::EnumConnectionPoints(%p)"), ppEnum);
			// TODO: hook enumerator
			HRESULT hr = pIConnectionPointContainer->EnumConnectionPoints(ppEnum);
			dbgprintf(TEXT("IConnectionPointContainer::EnumConnectionPoints -> %08X, pEnum = %p"), hr, *ppEnum);
			return hr;
		}

		virtual HRESULT STDMETHODCALLTYPE FindConnectionPoint(REFIID riid, IConnectionPoint ** ppCP)
		{
			IConnectionPointContainer * pIConnectionPointContainer = getIConnectionPointContainer();
			dbgprintf(TEXT("IConnectionPointContainer::FindConnectionPoint(%ls, %p)"), UUIDToString(riid).c_str(), ppCP);
			// TODO: hook connection point
			HRESULT hr = pIConnectionPointContainer->FindConnectionPoint(riid, ppCP);
			dbgprintf(TEXT("IConnectionPointContainer::FindConnectionPoint -> %08X, pCP = %p"), hr, *ppCP);
			return hr;
		}

		/* IDataObject */
	private:
		static std::basic_string<TCHAR> TargetDeviceToString(const DVTARGETDEVICE& targetdev)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "{";
			o << LPCWSTR(targetdev.tdData[targetdev.tdDriverNameOffset]);

			if(targetdev.tdDeviceNameOffset)
			{
				o << ",";
				o << LPCWSTR(targetdev.tdData[targetdev.tdDeviceNameOffset]);
			}

			if(targetdev.tdPortNameOffset)
			{
				o << ",";
				o << LPCWSTR(targetdev.tdData[targetdev.tdPortNameOffset]);
			}

			o << " }";

			return o.str();
		}

		static LPCTSTR AspectToString(DWORD aspect)
		{
			switch(aspect)
			{
			case DVASPECT_CONTENT: return TEXT("content");
			case DVASPECT_THUMBNAIL: return TEXT("thumbnail");
			case DVASPECT_ICON: return TEXT("icon");
			case DVASPECT_DOCPRINT: return TEXT("printable");
			default: return TEXT("<unknown>");
			}
		}

		static LPCTSTR TymedToString(DWORD tymed)
		{
			switch(tymed)
			{
			case TYMED_HGLOBAL: return TEXT("memory");
			case TYMED_FILE: return TEXT("file");
			case TYMED_ISTREAM: return TEXT("IStream");
			case TYMED_ISTORAGE: return TEXT("IStorage");
			case TYMED_GDI:  return TEXT("bitmap");
			case TYMED_MFPICT: return TEXT("metafile");
			case TYMED_ENHMF: return TEXT("enhanced metafile");
			case TYMED_NULL: return TEXT("<no data>");
			default: return TEXT("<unknown>");
			}
		}

		static std::basic_string<TCHAR> FormatEtcToString(const FORMATETC& formatetc)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "{";

			// cfFormat
			o << " format: ";

			switch(formatetc.cfFormat)
			{
			case CF_TEXT:            o << "ANSI text"; break;
			case CF_BITMAP:          o << "bitmap"; break;
			case CF_METAFILEPICT:    o << "metafile"; break;
			case CF_SYLK:            o << "symlink"; break;
			case CF_DIF:             o << "DIF"; break;
			case CF_TIFF:            o << "TIFF"; break;
			case CF_OEMTEXT:         o << "OEM text"; break;
			case CF_DIB:             o << "DIBv4"; break;
			case CF_PALETTE:         o << "palette"; break;
			case CF_PENDATA:         o << "pen data"; break;
			case CF_RIFF:            o << "RIFF"; break;
			case CF_WAVE:            o << "WAV"; break;
			case CF_UNICODETEXT:     o << "Unicode text"; break;
			case CF_ENHMETAFILE:     o << "enhanced metafile"; break;
			case CF_HDROP:           o << "list of files"; break;
			case CF_LOCALE:          o << "LCID"; break;
			case CF_DIBV5:           o << "DIBv5"; break;
			case CF_OWNERDISPLAY:    o << "<owner displayed>"; break;
			case CF_DSPTEXT:         o << "<display text>"; break;
			case CF_DSPBITMAP:       o << "<display bitmap>"; break;
			case CF_DSPMETAFILEPICT: o << "<display metafile>"; break;
			case CF_DSPENHMETAFILE:  o << "<display enhanced metafile>"; break;

			default:
				o << "<";

				if(formatetc.cfFormat >= CF_PRIVATEFIRST && formatetc.cfFormat <= CF_PRIVATELAST)
					o << "private";
				else if(formatetc.cfFormat >= CF_GDIOBJFIRST && formatetc.cfFormat <= CF_GDIOBJLAST)
					o << "GDI object";
				else
					o << "unknown";

				o << " " << std::hex << formatetc.cfFormat << std::dec << ">";
			}

			// ptd
			if(formatetc.ptd)
				o << " device: " << TargetDeviceToString(*formatetc.ptd);

			// dwAspect
			o << " aspect: " << AspectToString(formatetc.dwAspect);

			// lindex
			if(formatetc.dwAspect == DVASPECT_CONTENT || formatetc.dwAspect == DVASPECT_DOCPRINT)
				o << " page split: " << formatetc.lindex;

			// tymed
			o << " medium: " << TymedToString(formatetc.tymed);

			return o.str();
		}

		static std::basic_string<TCHAR> MediumToString(const STGMEDIUM& medium)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "{ ";
			o << TymedToString(medium.tymed);
			o << " }";

			return o.str();
		}

	public:
        virtual HRESULT STDMETHODCALLTYPE GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::GetData(%s, %p)"), FormatEtcToString(*pformatetcIn).c_str(), pmedium);
			HRESULT hr = pIDataObject->GetData(pformatetcIn, pmedium);
			dbgprintf(TEXT("IDataObject::GetData -> %08X, %s"), hr, MediumToString(*pmedium).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC * pformatetc, STGMEDIUM * pmedium)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::GetDataHere(%s, %p)"), FormatEtcToString(*pformatetc).c_str(), pmedium);
			HRESULT hr = pIDataObject->GetDataHere(pformatetc, pmedium);
			dbgprintf(TEXT("IDataObject::GetDataHere -> %08X, medium = %s"), hr, MediumToString(*pmedium).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC * pformatetc)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::QueryGetData(%s)"), FormatEtcToString(*pformatetc).c_str());
			HRESULT hr = pIDataObject->QueryGetData(pformatetc);
			dbgprintf(TEXT("IDataObject::QueryGetData -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC * pformatectIn, FORMATETC * pformatetcOut)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::GetCanonicalFormatEtc(%s, %p)"), FormatEtcToString(*pformatectIn).c_str(), pformatetcOut);
			HRESULT hr = pIDataObject->GetCanonicalFormatEtc(pformatectIn, pformatetcOut);
			dbgprintf(TEXT("IDataObject::GetCanonicalFormatEtc -> %08X, formatetcOut = %s"), hr, FormatEtcToString(*pformatetcOut).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SetData(FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::SetData(%s, %p, %s)"), FormatEtcToString(*pformatetc).c_str(), MediumToString(*pmedium).c_str(), fRelease ? TEXT("true") : TEXT("false"));
			HRESULT hr = pIDataObject->SetData(pformatetc, pmedium, fRelease);
			dbgprintf(TEXT("IDataObject::SetData -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::EnumFormatEtc(%lu, %p)"), dwDirection, ppenumFormatEtc);
			HRESULT hr = pIDataObject->EnumFormatEtc(dwDirection, ppenumFormatEtc);
			dbgprintf(TEXT("IDataObject::EnumFormatEtc -> %08X, penumFormatEtc = %p"), hr, *ppenumFormatEtc);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC * pformatetc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::DAdvise(%s, %lu, %p, %p)"), FormatEtcToString(*pformatetc).c_str(), advf, pAdvSink, pdwConnection);
			HRESULT hr = pIDataObject->DAdvise(pformatetc, advf, pAdvSink, pdwConnection);
			dbgprintf(TEXT("IDataObject::DAdvise -> %08X, dwConnection = %lu"), hr, *pdwConnection);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::DUnadvise(%lu)"), dwConnection);
			HRESULT hr = pIDataObject->DUnadvise(dwConnection);
			dbgprintf(TEXT("IDataObject::DUnadvise -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
		{
			IDataObject * pIDataObject = getIDataObject();
			dbgprintf(TEXT("IDataObject::EnumDAdvise(%p)"), ppenumAdvise);
			HRESULT hr = pIDataObject->EnumDAdvise(ppenumAdvise);
			dbgprintf(TEXT("IDataObject::EnumDAdvise -> %08X, penumAdvise = %p"), hr, *ppenumAdvise);
			return hr;
		}

		/* IObjectSafety */
	public:
		virtual HRESULT STDMETHODCALLTYPE IObjectSafety::GetInterfaceSafetyOptions(REFIID riid, DWORD * pdwSupportedOptions, DWORD * pdwEnabledOptions)
		{
			IObjectSafety * pIObjectSafety = getIObjectSafety();
			dbgprintf(TEXT("IObjectSafety::GetInterfaceSafetyOptions(%ls, %p, %p)"), UUIDToString(riid).c_str(), pdwSupportedOptions, pdwEnabledOptions);
			HRESULT hr = pIObjectSafety->GetInterfaceSafetyOptions(riid, pdwSupportedOptions, pdwEnabledOptions);
			dbgprintf(TEXT("IObjectSafety::GetInterfaceSafetyOptions -> %08X, dwSupportedOptions = %08X, dwEnabledOptions = %08X"), hr, *pdwSupportedOptions, *pdwEnabledOptions);
			return hr;
		}
        virtual HRESULT STDMETHODCALLTYPE IObjectSafety::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
		{
			IObjectSafety * pIObjectSafety = getIObjectSafety();
			dbgprintf(TEXT("IObjectSafety::SetInterfaceSafetyOptions(%ls, %08X, %08X)"), UUIDToString(riid).c_str(), dwOptionSetMask, dwEnabledOptions);
			HRESULT hr = pIObjectSafety->SetInterfaceSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);
			dbgprintf(TEXT("IObjectSafety::SetInterfaceSafetyOptions -> %08X"), hr);
			return hr;
		}

		/* IOleControl */
	private:
		std::basic_string<TCHAR> ControlInfoToString(const CONTROLINFO& ci)
		{
			std::basic_ostringstream<TCHAR> o;

			int firstdone = 0;

			o << "{ ";

			if(ci.cAccel && ci.hAccel)
			{
				LPACCEL pAccel = static_cast<LPACCEL>(GlobalLock(ci.hAccel));

				if(pAccel)
				{
					for(USHORT i = 0; i < ci.cAccel; ++ i)
					{
						if(i)
							o << ", ";

						if(pAccel[i].fVirt & FSHIFT)
							o << "SHIFT + ";

						if(pAccel[i].fVirt & FCONTROL)
							o << "CONTROL + ";

						if(pAccel[i].fVirt & FALT)
							o << "ALT + ";

						if(pAccel[i].fVirt & FVIRTKEY)
							o << "<vkey:" << std::hex << pAccel[i].key << std::dec << ">";
						else
							o << wchar_t(pAccel[i].key);

						o << " " << std::hex << pAccel[i].cmd << std::dec;
					}

					firstdone = ci.cAccel;

					GlobalUnlock(pAccel);
				}
			}

			if(ci.dwFlags & CTRLINFO_EATS_RETURN)
			{
				if(!firstdone)
				{
					o << ", ";
					++ firstdone;
				}

				o << "ENTER";
			}

			if(ci.dwFlags & CTRLINFO_EATS_ESCAPE)
			{
				if(!firstdone)
				{
					o << ", ";
					++ firstdone;
				}

				o << "ESC";
			}

			if(firstdone)
				o << " ";

			o << "}";

			return o.str();
		}

		std::basic_string<TCHAR> MnemonicToString(const MSG& msg)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "[";

			switch(msg.message)
			{
			case WM_SYSKEYDOWN:
				o << "ALT + ";

			case WM_KEYDOWN:
				TCHAR sz[1024];
				GetKeyNameText(LONG(msg.lParam), sz, ARRAYSIZE(sz));
				o << sz;

			default:
				o << "<unknown message " << std::hex << msg.message << std::dec << ">";
			}

			o << "]";

			return o.str();
		}

	public:
        virtual HRESULT STDMETHODCALLTYPE GetControlInfo(CONTROLINFO * pCI)
		{
			IOleControl * pIOleControl = getIOleControl();
			dbgprintf(TEXT("IOleControl::GetControlInfo(%p)"), pCI);
			HRESULT hr = pIOleControl->GetControlInfo(pCI);
			dbgprintf(TEXT("IOleControl::GetControlInfo -> %08X, %s"), hr, ControlInfoToString(*pCI).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE OnMnemonic(MSG * pMsg)
		{
			IOleControl * pIOleControl = getIOleControl();
			dbgprintf(TEXT("IOleControl::OnMnemonic(%s)"), MnemonicToString(*pMsg).c_str());
			HRESULT hr = pIOleControl->OnMnemonic(pMsg);
			dbgprintf(TEXT("IOleControl::OnMnemonic -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE OnAmbientPropertyChange(DISPID dispID)
		{
			IOleControl * pIOleControl = getIOleControl();
			dbgprintf(TEXT("IOleControl::OnAmbientPropertyChange(%08X)"), dispID);
			HRESULT hr = pIOleControl->OnAmbientPropertyChange(dispID);
			dbgprintf(TEXT("IOleControl::OnAmbientPropertyChange -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE FreezeEvents(BOOL bFreeze)
		{
			IOleControl * pIOleControl = getIOleControl();
			dbgprintf(TEXT("IOleControl::FreezeEvents(%s)"), BooleanToString(bFreeze));
			HRESULT hr = pIOleControl->FreezeEvents(bFreeze);
			dbgprintf(TEXT("IOleControl::FreezeEvents -> %08X"), hr);
			return hr;
		}

		/* IOleInPlaceActiveObject */
	public:
        virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg)
		{
			IOleInPlaceActiveObject * pIOleInPlaceActiveObject = getIOleInPlaceActiveObject();
			dbgprintf(TEXT("IOleInPlaceActiveObject::TranslateAccelerator(%s)"), MnemonicToString(*lpmsg).c_str());
			HRESULT hr = pIOleInPlaceActiveObject->TranslateAccelerator(lpmsg);
			dbgprintf(TEXT("IOleInPlaceActiveObject::TranslateAccelerator -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate)
		{
			IOleInPlaceActiveObject * pIOleInPlaceActiveObject = getIOleInPlaceActiveObject();
			dbgprintf(TEXT("IOleInPlaceActiveObject::OnFrameWindowActivate(%s)"), BooleanToString(fActivate));
			HRESULT hr = pIOleInPlaceActiveObject->OnFrameWindowActivate(fActivate);
			dbgprintf(TEXT("IOleInPlaceActiveObject::OnFrameWindowActivate -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate)
		{
			IOleInPlaceActiveObject * pIOleInPlaceActiveObject = getIOleInPlaceActiveObject();
			dbgprintf(TEXT("IOleInPlaceActiveObject::OnDocWindowActivate(%s)"), BooleanToString(fActivate));
			HRESULT hr = pIOleInPlaceActiveObject->OnDocWindowActivate(fActivate);
			dbgprintf(TEXT("IOleInPlaceActiveObject::OnDocWindowActivate -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fFrameWindow)
		{
			IOleInPlaceActiveObject * pIOleInPlaceActiveObject = getIOleInPlaceActiveObject();
			dbgprintf(TEXT("IOleInPlaceActiveObject::ResizeBorder(%s)"), RectToString(*prcBorder).c_str(), pUIWindow, BooleanToString(fFrameWindow));
			HRESULT hr = pIOleInPlaceActiveObject->ResizeBorder(prcBorder, pUIWindow, fFrameWindow);
			dbgprintf(TEXT("IOleInPlaceActiveObject::ResizeBorder -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable)
		{
			IOleInPlaceActiveObject * pIOleInPlaceActiveObject = getIOleInPlaceActiveObject();
			dbgprintf(TEXT("IOleInPlaceActiveObject::EnableModeless(%s)"), BooleanToString(fEnable));
			HRESULT hr = pIOleInPlaceActiveObject->EnableModeless(fEnable);
			dbgprintf(TEXT("IOleInPlaceActiveObject::EnableModeless -> %08X"), hr);
			return hr;
		}

		/* IOleInPlaceObject */
	public:
        virtual HRESULT STDMETHODCALLTYPE InPlaceDeactivate(void)
		{
			IOleInPlaceObject * pIOleInPlaceObject = getIOleInPlaceObject();
			dbgprintf(TEXT("IOleInPlaceObject::InPlaceDeactivate()"));
			HRESULT hr = pIOleInPlaceObject->InPlaceDeactivate();
			dbgprintf(TEXT("IOleInPlaceObject::InPlaceDeactivate -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE UIDeactivate(void)
		{
			IOleInPlaceObject * pIOleInPlaceObject = getIOleInPlaceObject();
			dbgprintf(TEXT("IOleInPlaceObject::UIDeactivate()"));
			HRESULT hr = pIOleInPlaceObject->UIDeactivate();
			dbgprintf(TEXT("IOleInPlaceObject::UIDeactivate -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
		{
			IOleInPlaceObject * pIOleInPlaceObject = getIOleInPlaceObject();
			dbgprintf(TEXT("IOleInPlaceObject::SetObjectRects(%s, %s)"), RectToString(*lprcPosRect).c_str(), RectToString(*lprcClipRect).c_str());
			HRESULT hr = pIOleInPlaceObject->SetObjectRects(lprcPosRect, lprcClipRect);
			dbgprintf(TEXT("IOleInPlaceObject::SetObjectRects -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE ReactivateAndUndo(void)
		{
			IOleInPlaceObject * pIOleInPlaceObject = getIOleInPlaceObject();
			dbgprintf(TEXT("IOleInPlaceObject::ReactivateAndUndo()"));
			HRESULT hr = pIOleInPlaceObject->ReactivateAndUndo();
			dbgprintf(TEXT("IOleInPlaceObject::ReactivateAndUndo -> %08X"), hr);
			return hr;
		}

		/* IOleWindow */
	public:
        virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND * phwnd)
		{
			IOleWindow * pIOleWindow = getIOleWindow();
			dbgprintf(TEXT("IOleWindow::GetWindow(%p)"), phwnd);
			HRESULT hr = pIOleWindow->GetWindow(phwnd);
			dbgprintf(TEXT("IOleWindow::GetWindow -> %08X, hwnd = %X"), hr, *phwnd);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode)
		{
			IOleWindow * pIOleWindow = getIOleWindow();
			dbgprintf(TEXT("IOleWindow::ContextSensitiveHelp(%s)"), BooleanToString(fEnterMode));
			HRESULT hr = pIOleWindow->ContextSensitiveHelp(fEnterMode);
			dbgprintf(TEXT("IOleWindow::ContextSensitiveHelp -> %08X"), hr);
			return hr;
		}

		/* IOleObject */
	public:
        virtual HRESULT STDMETHODCALLTYPE SetClientSite(IOleClientSite * pClientSite)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::SetClientSite(%p)"), pClientSite);
			HRESULT hr = pIOleObject->SetClientSite(pClientSite);
			dbgprintf(TEXT("IOleObject::SetClientSite -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetClientSite(IOleClientSite ** ppClientSite)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetClientSite(%s)"), ppClientSite);
			HRESULT hr = pIOleObject->GetClientSite(ppClientSite);
			dbgprintf(TEXT("IOleObject::GetClientSite -> %08X"), hr, ppClientSite);
			return hr;
		}

		virtual HRESULT STDMETHODCALLTYPE SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::SetHostNames(%ls, %ls)"), szContainerApp, szContainerObj);
			HRESULT hr = pIOleObject->SetHostNames(szContainerApp, szContainerObj);
			dbgprintf(TEXT("IOleObject::SetHostNames -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Close(DWORD dwSaveOption)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::Close(%lu)"), dwSaveOption);
			HRESULT hr = pIOleObject->Close(dwSaveOption);
			dbgprintf(TEXT("IOleObject::Close -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SetMoniker(DWORD dwWhichMoniker, IMoniker * pmk)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::SetMoniker(%lu, %p)"), dwWhichMoniker, MonikerToString(pmk).c_str());
			HRESULT hr = pIOleObject->SetMoniker(dwWhichMoniker, pmk);
			dbgprintf(TEXT("IOleObject::SetMoniker -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetMoniker(%lu, %lu, %p)"), dwAssign, dwWhichMoniker, ppmk);
			HRESULT hr = pIOleObject->GetMoniker(dwAssign, dwWhichMoniker, ppmk);
			dbgprintf(TEXT("IOleObject::GetMoniker -> %08X, pmk = %s"), hr, SUCCEEDED(hr) ? MonikerToString(*ppmk).c_str() : TEXT("<null>"));
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE InitFromData(IDataObject * pDataObject, BOOL fCreation, DWORD dwReserved)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::InitFromData(%p, %s, %lu)"), pDataObject, BooleanToString(fCreation), dwReserved);
			HRESULT hr = pIOleObject->InitFromData(pDataObject, fCreation, dwReserved);
			dbgprintf(TEXT("IOleObject::InitFromData -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetClipboardData(DWORD dwReserved, IDataObject ** ppDataObject)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetClipboardData(%lu, %p)"), dwReserved, ppDataObject);
			HRESULT hr = pIOleObject->GetClipboardData(dwReserved, ppDataObject);
			dbgprintf(TEXT("IOleObject::GetClipboardData -> %08X, pDataObject = %p"), hr, *ppDataObject);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite * pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::DoVerb(%ld, %p, %p, %ld, %p, %s)"), iVerb, lpmsg, pActiveSite, lindex, hwndParent, RectToString(*lprcPosRect).c_str());
			HRESULT hr = pIOleObject->DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
			dbgprintf(TEXT("IOleObject::DoVerb -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE EnumVerbs(IEnumOLEVERB ** ppEnumOleVerb)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::EnumVerbs(%p)"), ppEnumOleVerb);
			HRESULT hr = pIOleObject->EnumVerbs(ppEnumOleVerb);
			dbgprintf(TEXT("IOleObject::EnumVerbs -> %08X, pEnumOleVerb = %p"), hr, *ppEnumOleVerb);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Update(void)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::Update(%s)"));
			HRESULT hr = pIOleObject->Update();
			dbgprintf(TEXT("IOleObject::Update -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IsUpToDate(void)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::IsUpToDate(%s)"));
			HRESULT hr = pIOleObject->IsUpToDate();
			dbgprintf(TEXT("IOleObject::IsUpToDate -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetUserClassID(CLSID * pClsid)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetUserClassID(%p)"), pClsid);
			HRESULT hr = pIOleObject->GetUserClassID(pClsid);
			dbgprintf(TEXT("IOleObject::GetUserClassID -> %08X, Clsid = %ls"), hr, UUIDToString(*pClsid).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetUserType(DWORD dwFormOfType, LPOLESTR * pszUserType)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetUserType(%lu, %p)"), dwFormOfType, pszUserType);
			HRESULT hr = pIOleObject->GetUserType(dwFormOfType, pszUserType);
			dbgprintf(TEXT("IOleObject::GetUserType -> %08X, szUserType = %s"), hr, *pszUserType);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SetExtent(DWORD dwDrawAspect, SIZEL * psizel)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::SetExtent(%lu, %s)"), dwDrawAspect, SizeToString(*psizel).c_str());
			HRESULT hr = pIOleObject->SetExtent(dwDrawAspect, psizel);
			dbgprintf(TEXT("IOleObject::SetExtent -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetExtent(DWORD dwDrawAspect, SIZEL * psizel)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetExtent(%lu, %p)"), dwDrawAspect, psizel);
			HRESULT hr = pIOleObject->GetExtent(dwDrawAspect, psizel);
			dbgprintf(TEXT("IOleObject::GetExtent -> %08X"), hr, SizeToString(*psizel).c_str());
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Advise(IAdviseSink * pAdvSink, DWORD * pdwConnection)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::Advise(%p, %p)"), pAdvSink, pdwConnection);
			HRESULT hr = pIOleObject->Advise(pAdvSink, pdwConnection);
			dbgprintf(TEXT("IOleObject::Advise -> %08X, dwConnection = %lu"), hr, *pdwConnection);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwConnection)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::Unadvise(%lu)"), dwConnection);
			HRESULT hr = pIOleObject->Unadvise(dwConnection);
			dbgprintf(TEXT("IOleObject::Unadvise -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE EnumAdvise(IEnumSTATDATA ** ppenumAdvise)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::EnumAdvise(%p)"), ppenumAdvise);
			HRESULT hr = pIOleObject->EnumAdvise(ppenumAdvise);
			dbgprintf(TEXT("IOleObject::EnumAdvise -> %08X, penumAdvise = %p"), hr, *ppenumAdvise);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE GetMiscStatus(DWORD dwAspect, DWORD * pdwStatus)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::GetMiscStatus(%lu, %p)"), dwAspect, pdwStatus);
			HRESULT hr = pIOleObject->GetMiscStatus(dwAspect, pdwStatus);
			dbgprintf(TEXT("IOleObject::GetMiscStatus -> %08X, dwStatus = %08X"), hr, *pdwStatus);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SetColorScheme(LOGPALETTE * pLogpal)
		{
			IOleObject * pIOleObject = getIOleObject();
			dbgprintf(TEXT("IOleObject::SetColorScheme(%p)"), pLogpal);
			HRESULT hr = pIOleObject->SetColorScheme(pLogpal);
			dbgprintf(TEXT("IOleObject::SetColorScheme -> %08X"), hr);
			return hr;
		}

		/* IPersist */
	public:
        virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * pClassID)
		{
			IPersist * pIPersist = getIPersist();
			dbgprintf(TEXT("IPersist::GetClassID(%p)"), pClassID);
			HRESULT hr = pIPersist->GetClassID(pClassID);
			dbgprintf(TEXT("IPersist::GetClassID -> %08X, ClassId = %ls"), hr, UUIDToString(*pClassID).c_str());
			return hr;
		}

		/* IPersistPropertyBag */
	public:
        virtual HRESULT STDMETHODCALLTYPE InitNew(void)
		{
			IPersistPropertyBag * pIPersistPropertyBag = getIPersistPropertyBag();
			dbgprintf(TEXT("IPersistPropertyBag::InitNew()"));
			HRESULT hr = pIPersistPropertyBag->InitNew();
			dbgprintf(TEXT("IPersistPropertyBag::InitNew -> %08X, "), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
		{
			IPersistPropertyBag * pIPersistPropertyBag = getIPersistPropertyBag();
			dbgprintf(TEXT("IPersistPropertyBag::Load(%p, %p)"), pPropBag, pErrorLog);
			HRESULT hr = pIPersistPropertyBag->Load(pPropBag, pErrorLog);
			dbgprintf(TEXT("IPersistPropertyBag::Load -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
		{
			IPersistPropertyBag * pIPersistPropertyBag = getIPersistPropertyBag();
			dbgprintf(TEXT("IPersistPropertyBag::Save(%p, %s, %s)"), pPropBag, BooleanToString(fClearDirty), BooleanToString(fSaveAllProperties));
			HRESULT hr = pIPersistPropertyBag->Save(pPropBag, fClearDirty, fSaveAllProperties);
			dbgprintf(TEXT("IPersistPropertyBag::Save -> %08X"), hr);
			return hr;
		}

		/* IPersistStorage */
	public:
		virtual HRESULT STDMETHODCALLTYPE IPersistStorage::IsDirty(void)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::IsDirty()"));
			HRESULT hr = pIPersistStorage->IsDirty();
			dbgprintf(TEXT("IPersistStorage::IsDirty -> %08X, "), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStorage::InitNew(IStorage * pStg)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::InitNew(%p)"), pStg);
			HRESULT hr = pIPersistStorage->InitNew(pStg);
			dbgprintf(TEXT("IPersistStorage::InitNew -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStorage::Load(IStorage * pStg)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::Load(%p)"), pStg);
			HRESULT hr = pIPersistStorage->Load(pStg);
			dbgprintf(TEXT("IPersistStorage::Load -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStorage::Save(IStorage * pStgSave, BOOL fSameAsLoad)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::Save(%p, %s)"), pStgSave, BooleanToString(fSameAsLoad));
			HRESULT hr = pIPersistStorage->Save(pStgSave, fSameAsLoad);
			dbgprintf(TEXT("IPersistStorage::Save -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE SaveCompleted(IStorage * pStgNew)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::SaveCompleted(%p)"), pStgNew);
			HRESULT hr = pIPersistStorage->SaveCompleted(pStgNew);
			dbgprintf(TEXT("IPersistStorage::SaveCompleted -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE HandsOffStorage(void)
		{
			IPersistStorage * pIPersistStorage = getIPersistStorage();
			dbgprintf(TEXT("IPersistStorage::HandsOffStorage()"));
			HRESULT hr = pIPersistStorage->HandsOffStorage();
			dbgprintf(TEXT("IPersistStorage::HandsOffStorage -> %08X"), hr);
			return hr;
		}

		/* IPersistStreamInit */
	public:
		virtual HRESULT STDMETHODCALLTYPE IPersistStreamInit::IsDirty(void)
		{
			IPersistStreamInit * pIPersistStreamInit = getIPersistStreamInit();
			dbgprintf(TEXT("IPersistStreamInit::IsDirty()"));
			HRESULT hr = pIPersistStreamInit->IsDirty();
			dbgprintf(TEXT("IPersistStreamInit::IsDirty -> %08X"), hr);
			return hr;
		}

		virtual HRESULT STDMETHODCALLTYPE IPersistStreamInit::Load(LPSTREAM pStm)
		{
			IPersistStreamInit * pIPersistStreamInit = getIPersistStreamInit();
			dbgprintf(TEXT("IPersistStreamInit::Load(%p)"), pStm);
			HRESULT hr = pIPersistStreamInit->Load(pStm);
			dbgprintf(TEXT("IPersistStreamInit::Load -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStreamInit::Save(LPSTREAM pStm, BOOL fClearDirty)
		{
			IPersistStreamInit * pIPersistStreamInit = getIPersistStreamInit();
			dbgprintf(TEXT("IPersistStreamInit::Save(%p, %s)"), pStm, BooleanToString(fClearDirty));
			HRESULT hr = pIPersistStreamInit->Save(pStm, fClearDirty);
			dbgprintf(TEXT("IPersistStreamInit::Save -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStreamInit::GetSizeMax(ULARGE_INTEGER * pCbSize)
		{
			IPersistStreamInit * pIPersistStreamInit = getIPersistStreamInit();
			dbgprintf(TEXT("IPersistStreamInit::GetSizeMax(%p)"), pCbSize);
			HRESULT hr = pIPersistStreamInit->GetSizeMax(pCbSize);
			dbgprintf(TEXT("IPersistStreamInit::GetSizeMax -> %08X, CbSize = %llu"), hr, pCbSize->QuadPart);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IPersistStreamInit::InitNew(void)
		{
			IPersistStreamInit * pIPersistStreamInit = getIPersistStreamInit();
			dbgprintf(TEXT("IPersistStreamInit::InitNew()"));
			HRESULT hr = pIPersistStreamInit->InitNew();
			dbgprintf(TEXT("IPersistStreamInit::InitNew -> %08X, "), hr);
			return hr;
		}

		/* IProvideClassInfo */
	public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo ** ppTI)
		{
			IProvideClassInfo * pIProvideClassInfo = getIProvideClassInfo();
			dbgprintf(TEXT("IProvideClassInfo::GetClassInfo(%p)"), ppTI);
			HRESULT hr = pIProvideClassInfo->GetClassInfo(ppTI);
			dbgprintf(TEXT("IProvideClassInfo::GetClassInfo -> %08X, pTI = %p"), hr, *ppTI);
			return hr;
		}

		/* IProvideClassInfo2 */
	public:
		virtual HRESULT STDMETHODCALLTYPE IProvideClassInfo2::GetGUID(DWORD dwGuidKind, GUID * pGUID)
		{
			IProvideClassInfo2 * pIProvideClassInfo2 = getIProvideClassInfo2();
			dbgprintf(TEXT("IProvideClassInfo2::GetGUID(%lu, %p)"), dwGuidKind, pGUID);
			HRESULT hr = pIProvideClassInfo2->GetGUID(dwGuidKind, pGUID);
			dbgprintf(TEXT("IProvideClassInfo2::GetGUID -> %08X, GUID = %ls"), hr, UUIDToString(*pGUID).c_str());
			return hr;
		}

		/* IQuickActivate */
	public:
		virtual HRESULT STDMETHODCALLTYPE IQuickActivate::QuickActivate(QACONTAINER * pQaContainer, QACONTROL * pQaControl) // TODO
		{
			IQuickActivate * pIQuickActivate = getIQuickActivate();
			dbgprintf(TEXT("IQuickActivate::QuickActivate(%p, %p)"), pQaContainer, pQaControl);
			HRESULT hr = pIQuickActivate->QuickActivate(pQaContainer, pQaControl);
			dbgprintf(TEXT("IQuickActivate::QuickActivate -> %08X, pQaControl = %p"), hr, pQaControl);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IQuickActivate::SetContentExtent(LPSIZEL pSizel)
		{
			IQuickActivate * pIQuickActivate = getIQuickActivate();
			dbgprintf(TEXT("IQuickActivate::SetContentExtent(%s)"), SizeToString(*pSizel).c_str());
			HRESULT hr = pIQuickActivate->SetContentExtent(pSizel);
			dbgprintf(TEXT("IQuickActivate::SetContentExtent -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IQuickActivate::GetContentExtent(LPSIZEL pSizel)
		{
			IQuickActivate * pIQuickActivate = getIQuickActivate();
			dbgprintf(TEXT("IQuickActivate::GetContentExtent(%p)"), pSizel);
			HRESULT hr = pIQuickActivate->GetContentExtent(pSizel);
			dbgprintf(TEXT("IQuickActivate::GetContentExtent -> %08X, Sizel = %s"), hr, SizeToString(*pSizel).c_str());
			return hr;
		}

		/* ISpecifyPropertyPages */
	private:
		std::basic_string<TCHAR> CauuidToString(const CAUUID& cauuid)
		{
			std::basic_ostringstream<TCHAR> o;

			o << "{";

			for(ULONG i = 0; i < cauuid.cElems; ++ i)
			{
				if(i)
					o << ", ";
				else
					o << " ";

				o << UUIDToString(cauuid.pElems[i]);
			}

			o << " }";

			return o.str();
		}

	public:
		virtual HRESULT STDMETHODCALLTYPE ISpecifyPropertyPages::GetPages(CAUUID * pPages)
		{
			ISpecifyPropertyPages * pISpecifyPropertyPages = getISpecifyPropertyPages();
			dbgprintf(TEXT("ISpecifyPropertyPages::GetPages(%p)"), pPages);
			HRESULT hr = pISpecifyPropertyPages->GetPages(pPages);
			dbgprintf(TEXT("ISpecifyPropertyPages::GetPages -> %08X, Pages = %s"), hr, CauuidToString(*pPages).c_str());
			return hr;
		}

		/* IViewObject */
	public:
		virtual HRESULT STDMETHODCALLTYPE IViewObject::Draw(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds, LPCRECTL lprcWBounds, BOOL (STDMETHODCALLTYPE * pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::Draw(%s, %ld, %p, %s, %p, %p, %s, %s, %p, %p)"), AspectToString(dwDrawAspect), lindex, pvAspect, TargetDeviceToString(*ptd).c_str(), hdcTargetDev, hdcDraw, RectToString(*lprcBounds).c_str(), RectToString(*lprcWBounds).c_str(), pfnContinue, dwContinue);
			HRESULT hr = pIViewObject->Draw(dwDrawAspect, lindex, pvAspect, ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
			dbgprintf(TEXT("IViewObject::Draw -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IViewObject::GetColorSet(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DVTARGETDEVICE * ptd, HDC hicTargetDev, LOGPALETTE ** ppColorSet)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::GetColorSet(%s, %ld, %p, %s, %p, %p)"), AspectToString(dwDrawAspect), lindex, pvAspect, TargetDeviceToString(*ptd).c_str(), hicTargetDev, ppColorSet);
			HRESULT hr = pIViewObject->GetColorSet(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, ppColorSet);
			dbgprintf(TEXT("IViewObject::GetColorSet -> %08X, pColorSet = %p"), hr, *ppColorSet);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IViewObject::Freeze(DWORD dwDrawAspect, LONG lindex, void * pvAspect, DWORD * pdwFreeze)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::Freeze(%s, %ld, %p, %p)"), AspectToString(dwDrawAspect), lindex, pvAspect, pdwFreeze);
			HRESULT hr = pIViewObject->Freeze(dwDrawAspect, lindex, pvAspect, pdwFreeze);
			dbgprintf(TEXT("IViewObject::Freeze -> %08X, dwFreeze = %08X"), hr, *pdwFreeze);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IViewObject::Unfreeze(DWORD dwFreeze)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::Unfreeze(%08X)"), dwFreeze);
			HRESULT hr = pIViewObject->Unfreeze(dwFreeze);
			dbgprintf(TEXT("IViewObject::Unfreeze -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IViewObject::SetAdvise(DWORD aspects, DWORD advf, IAdviseSink * pAdvSink)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::SetAdvise(%s, %08X, %p)"), AspectToString(aspects), advf, pAdvSink);
			HRESULT hr = pIViewObject->SetAdvise(aspects, advf, pAdvSink);
			dbgprintf(TEXT("IViewObject::SetAdvise -> %08X"), hr);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE IViewObject::GetAdvise(DWORD * pAspects, DWORD * pAdvf, IAdviseSink ** ppAdvSink)
		{
			IViewObject * pIViewObject = getIViewObject();
			dbgprintf(TEXT("IViewObject::GetAdvise(%p, %p, %p)"), pAspects, pAdvf, ppAdvSink);
			HRESULT hr = pIViewObject->GetAdvise(pAspects, pAdvf, ppAdvSink);
			dbgprintf(TEXT("IViewObject::GetAdvise -> %08X, aspects = %s, advf = %08X, pAdvSink %p"), hr, AspectToString(*pAspects), *pAdvf, *ppAdvSink);
			return hr;
		}

		/* IViewObject2 */
	public:
		virtual HRESULT STDMETHODCALLTYPE IViewObject2::GetExtent(DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE * ptd, LPSIZEL lpsizel)
		{
			IViewObject2 * pIViewObject2 = getIViewObject2();
			dbgprintf(TEXT("IViewObject2::GetExtent(%s, %ld, %s, %p)"), AspectToString(dwDrawAspect), lindex, TargetDeviceToString(*ptd).c_str(), lpsizel);
			HRESULT hr = pIViewObject2->GetExtent(dwDrawAspect, lindex, ptd, lpsizel);
			dbgprintf(TEXT("IViewObject2::GetExtent -> %08X, sizel = %s"), hr, SizeToString(*lpsizel).c_str());
			return hr;
		}

		/* IMsTscAx */
	public:
		virtual HRESULT __stdcall put_Server(BSTR pServer)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_Server(%ls)"), pServer);
			HRESULT hr = pIMsTscAx->put_Server(pServer);
			dbgprintf(TEXT("IMsTscAx::put_Server -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_Server(BSTR * pServer)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_Server()"), pServer);
			HRESULT hr = pIMsTscAx->get_Server(pServer);
			dbgprintf(TEXT("IMsTscAx::get_Server -> %08X, server = %ls"), hr, *pServer);
			return hr;
		}

		virtual HRESULT __stdcall put_Domain(BSTR pDomain)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_Domain(%ls)"), pDomain);
			HRESULT hr = pIMsTscAx->put_Domain(pDomain);
			dbgprintf(TEXT("IMsTscAx::put_Domain -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_Domain(BSTR * pDomain)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_Domain(%p)"), pDomain);
			HRESULT hr = pIMsTscAx->get_Domain(pDomain);
			dbgprintf(TEXT("IMsTscAx::get_Domain -> %08X, Domain = %ls"), hr, *pDomain);
			return hr;
		}

		virtual HRESULT __stdcall put_UserName(BSTR pUserName)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_UserName(%ls)"), pUserName);
			HRESULT hr = pIMsTscAx->put_UserName(pUserName);
			dbgprintf(TEXT("IMsTscAx::put_UserName -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_UserName(BSTR * pUserName)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_UserName(%p)"), pUserName);
			HRESULT hr = pIMsTscAx->get_UserName(pUserName);
			dbgprintf(TEXT("IMsTscAx::get_UserName -> %08X, UserName = %ls"), hr, *pUserName);
			return hr;
		}

		virtual HRESULT __stdcall put_DisconnectedText(BSTR pDisconnectedText)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_DisconnectedText(%ls)"), pDisconnectedText);
			HRESULT hr = pIMsTscAx->put_DisconnectedText(pDisconnectedText);
			dbgprintf(TEXT("IMsTscAx::put_DisconnectedText -> %08X, "), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_DisconnectedText(BSTR * pDisconnectedText)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_DisconnectedText(%p)"), pDisconnectedText);
			HRESULT hr = pIMsTscAx->get_DisconnectedText(pDisconnectedText);
			dbgprintf(TEXT("IMsTscAx::get_DisconnectedText -> %08X, DisconnectedText = %ls"), hr, *pDisconnectedText);
			return hr;
		}

		virtual HRESULT __stdcall put_ConnectingText(BSTR pConnectingText)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_ConnectingText(%ls)"), pConnectingText);
			HRESULT hr = pIMsTscAx->put_ConnectingText(pConnectingText);
			dbgprintf(TEXT("IMsTscAx::put_ConnectingText -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_ConnectingText(BSTR * pConnectingText)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_ConnectingText(%p)"), pConnectingText);
			HRESULT hr = pIMsTscAx->get_ConnectingText(pConnectingText);
			dbgprintf(TEXT("IMsTscAx::get_ConnectingText -> %08X, ConnectingText = %ls"), hr, *pConnectingText);
			return hr;
		}

		virtual HRESULT __stdcall get_Connected(short * pIsConnected)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_Connected(%p)"), pIsConnected);
			HRESULT hr = pIMsTscAx->get_Connected(pIsConnected);
			dbgprintf(TEXT("IMsTscAx::get_Connected -> %08X, IsConnected = %s"), hr, BooleanToString(*pIsConnected));
			return hr;
		}

		virtual HRESULT __stdcall put_DesktopWidth(long pVal)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_DesktopWidth(%ld)"), pVal);
			HRESULT hr = pIMsTscAx->put_DesktopWidth(pVal);
			dbgprintf(TEXT("IMsTscAx::put_DesktopWidth -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_DesktopWidth(long * pVal)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_DesktopWidth(%p)"), pVal);
			HRESULT hr = pIMsTscAx->get_DesktopWidth(pVal);
			dbgprintf(TEXT("IMsTscAx::get_DesktopWidth -> %08X, Val = %lu"), hr, *pVal);
			return hr;
		}

		virtual HRESULT __stdcall put_DesktopHeight(long pVal)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_DesktopHeight(%ld)"), pVal);
			HRESULT hr = pIMsTscAx->put_DesktopHeight(pVal);
			dbgprintf(TEXT("IMsTscAx::put_DesktopHeight -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_DesktopHeight(long * pVal)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_DesktopHeight(%p)"), pVal);
			HRESULT hr = pIMsTscAx->get_DesktopHeight(pVal);
			dbgprintf(TEXT("IMsTscAx::get_DesktopHeight -> %08X, Val = %lu"), hr, *pVal);
			return hr;
		}

		virtual HRESULT __stdcall put_StartConnected(long pfStartConnected)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_StartConnected(%s)"), BooleanToString(pfStartConnected));
			HRESULT hr = pIMsTscAx->put_StartConnected(pfStartConnected);
			dbgprintf(TEXT("IMsTscAx::put_StartConnected -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_StartConnected(long * pfStartConnected)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_StartConnected(%p)"), pfStartConnected);
			HRESULT hr = pIMsTscAx->get_StartConnected(pfStartConnected);
			dbgprintf(TEXT("IMsTscAx::get_StartConnected -> %08X, fStartConnected = %s"), hr, BooleanToString(*pfStartConnected));
			return hr;
		}

		virtual HRESULT __stdcall get_HorizontalScrollBarVisible(long * pfHScrollVisible)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_HorizontalScrollBarVisible(%p)"), pfHScrollVisible);
			HRESULT hr = pIMsTscAx->get_HorizontalScrollBarVisible(pfHScrollVisible);
			dbgprintf(TEXT("IMsTscAx::get_HorizontalScrollBarVisible -> %08X, fHScrollVisible = %s"), hr, BooleanToString(*pfHScrollVisible));
			return hr;
		}

		virtual HRESULT __stdcall get_VerticalScrollBarVisible(long * pfVScrollVisible)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_VerticalScrollBarVisible(%p)"), pfVScrollVisible);
			HRESULT hr = pIMsTscAx->get_VerticalScrollBarVisible(pfVScrollVisible);
			dbgprintf(TEXT("IMsTscAx::get_VerticalScrollBarVisible -> %08X, fVScrollVisible"), hr, *pfVScrollVisible);
			return hr;
		}

		virtual HRESULT __stdcall put_FullScreenTitle(BSTR _arg1)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::put_FullScreenTitle(%ls)"), _arg1);
			HRESULT hr = pIMsTscAx->put_FullScreenTitle(_arg1);
			dbgprintf(TEXT("IMsTscAx::put_FullScreenTitle -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_CipherStrength(long * pCipherStrength)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_CipherStrength(%p)"), pCipherStrength);
			HRESULT hr = pIMsTscAx->get_CipherStrength(pCipherStrength);
			dbgprintf(TEXT("IMsTscAx::get_CipherStrength -> %08X, CipherStrength = %ld"), hr, *pCipherStrength);
			return hr;
		}

		virtual HRESULT __stdcall get_Version(BSTR * pVersion)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_Version(%p)"), pVersion);
			HRESULT hr = pIMsTscAx->get_Version(pVersion);
			dbgprintf(TEXT("IMsTscAx::get_Version -> %08X, Version = %ls"), hr, *pVersion);
			return hr;
		}

		virtual HRESULT __stdcall get_SecuredSettingsEnabled(long * pSecuredSettingsEnabled)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_SecuredSettingsEnabled(%p)"), pSecuredSettingsEnabled);
			HRESULT hr = pIMsTscAx->get_SecuredSettingsEnabled(pSecuredSettingsEnabled);
			dbgprintf(TEXT("IMsTscAx::get_SecuredSettingsEnabled -> %08X, SecuredSettingsEnabled = %s"), hr, BooleanToString(*pSecuredSettingsEnabled));
			return hr;
		}

		virtual HRESULT __stdcall get_SecuredSettings(IMsTscSecuredSettings ** ppSecuredSettings)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_SecuredSettings(%p)"), ppSecuredSettings);
			HRESULT hr = pIMsTscAx->get_SecuredSettings(ppSecuredSettings);
			dbgprintf(TEXT("IMsTscAx::get_SecuredSettings -> %08X, pSecuredSettings = %p"), hr, *ppSecuredSettings);
			return hr;
		}

		virtual HRESULT __stdcall get_AdvancedSettings(IMsTscAdvancedSettings ** ppAdvSettings)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_AdvancedSettings(%p)"), ppAdvSettings);
			HRESULT hr = pIMsTscAx->get_AdvancedSettings(ppAdvSettings);
			dbgprintf(TEXT("IMsTscAx::get_AdvancedSettings -> %08X, pAdvSettings = %p"), hr, *ppAdvSettings);
			return hr;
		}

		virtual HRESULT __stdcall get_Debugger(IMsTscDebug ** ppDebugger)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::get_Debugger(%p)"), ppDebugger);
			HRESULT hr = pIMsTscAx->get_Debugger(ppDebugger);
			dbgprintf(TEXT("IMsTscAx::get_Debugger -> %08X, pDebugger = %p"), hr, *ppDebugger);
			return hr;
		}

		virtual HRESULT __stdcall raw_Connect()
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::Connect()"));
			HRESULT hr = pIMsTscAx->Connect();
			dbgprintf(TEXT("IMsTscAx::Connect -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall raw_Disconnect()
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::Disconnect()"));
			HRESULT hr = pIMsTscAx->Disconnect();
			dbgprintf(TEXT("IMsTscAx::Disconnect -> %08X, "), hr);
			return hr;
		}

		virtual HRESULT __stdcall raw_CreateVirtualChannels(BSTR newVal)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::CreateVirtualChannels(%ls)"), newVal);
			HRESULT hr = pIMsTscAx->CreateVirtualChannels(newVal);
			dbgprintf(TEXT("IMsTscAx::CreateVirtualChannels -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall raw_SendOnVirtualChannel(BSTR chanName, BSTR ChanData)
		{
			IMsTscAx * pIMsTscAx = getIMsTscAx();
			dbgprintf(TEXT("IMsTscAx::SendOnVirtualChannel(%ls, %p)"), chanName, ChanData);
			HRESULT hr = pIMsTscAx->SendOnVirtualChannel(chanName, ChanData);
			dbgprintf(TEXT("IMsTscAx::SendOnVirtualChannel -> %08X"), hr);
			return hr;
		}

		/* IMsRdpClient */
	public:
		virtual HRESULT __stdcall put_ColorDepth(long pcolorDepth)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::put_ColorDepth(%ld)"), pcolorDepth);
			HRESULT hr = pIMsRdpClient->put_ColorDepth(pcolorDepth);
			dbgprintf(TEXT("IMsRdpClient::put_ColorDepth -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_ColorDepth(long * pcolorDepth)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::get_ColorDepth(%p)"), pcolorDepth);
			HRESULT hr = pIMsRdpClient->get_ColorDepth(pcolorDepth);
			dbgprintf(TEXT("IMsRdpClient::get_ColorDepth -> %08X, colorDepth = %ld"), hr, *pcolorDepth);
			return hr;
		}

		virtual HRESULT __stdcall get_AdvancedSettings2(IMsRdpClientAdvancedSettings ** ppAdvSettings)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::get_AdvancedSettings2(%p)"), ppAdvSettings);
			HRESULT hr = pIMsRdpClient->get_AdvancedSettings2(ppAdvSettings);
			dbgprintf(TEXT("IMsRdpClient::get_AdvancedSettings2 -> %08X, pAdvSettings = %p"), hr, *ppAdvSettings);
			return hr;
		}

		virtual HRESULT __stdcall get_SecuredSettings2(IMsRdpClientSecuredSettings ** ppSecuredSettings)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::get_SecuredSettings2(%p)"), ppSecuredSettings);
			HRESULT hr = pIMsRdpClient->get_SecuredSettings2(ppSecuredSettings);
			dbgprintf(TEXT("IMsRdpClient::get_SecuredSettings2 -> %08X, pSecuredSettings = %p"), hr, *ppSecuredSettings);
			return hr;
		}

		virtual HRESULT __stdcall get_ExtendedDisconnectReason(ExtendedDisconnectReasonCode * pExtendedDisconnectReason)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::get_ExtendedDisconnectReason(%p)"), pExtendedDisconnectReason);
			HRESULT hr = pIMsRdpClient->get_ExtendedDisconnectReason(pExtendedDisconnectReason);
			dbgprintf(TEXT("IMsRdpClient::get_ExtendedDisconnectReason -> %08X, ExtendedDisconnectReason = %u"), hr, *pExtendedDisconnectReason);
			return hr;
		}

		virtual HRESULT __stdcall put_FullScreen(VARIANT_BOOL pfFullScreen)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::put_FullScreen(%s)"), BooleanToString(pfFullScreen));
			HRESULT hr = pIMsRdpClient->put_FullScreen(pfFullScreen);
			dbgprintf(TEXT("IMsRdpClient::put_FullScreen -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_FullScreen(VARIANT_BOOL * pfFullScreen)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::get_FullScreen(%p)"), pfFullScreen);
			HRESULT hr = pIMsRdpClient->get_FullScreen(pfFullScreen);
			dbgprintf(TEXT("IMsRdpClient::get_FullScreen -> %08X, pfFullScreen = %s"), hr, BooleanToString(*pfFullScreen));
			return hr;
		}

		virtual HRESULT __stdcall raw_SetVirtualChannelOptions(BSTR chanName, long chanOptions)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::SetVirtualChannelOptions(%ls, %08X)"), chanName, chanOptions);
			HRESULT hr = pIMsRdpClient->raw_SetVirtualChannelOptions(chanName, chanOptions);
			dbgprintf(TEXT("IMsRdpClient::SetVirtualChannelOptions -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall raw_GetVirtualChannelOptions(BSTR chanName, long * pChanOptions)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::GetVirtualChannelOptions(%ls, %p)"), chanName, pChanOptions);
			HRESULT hr = pIMsRdpClient->raw_GetVirtualChannelOptions(chanName, pChanOptions);
			dbgprintf(TEXT("IMsRdpClient::GetVirtualChannelOptions -> %08X, ChanOptions = %08X"), hr, *pChanOptions);
			return hr;
		}

		virtual HRESULT __stdcall raw_RequestClose(ControlCloseStatus * pCloseStatus)
		{
			IMsRdpClient * pIMsRdpClient = getIMsRdpClient();
			dbgprintf(TEXT("IMsRdpClient::RequestClose(%p)"), pCloseStatus);
			HRESULT hr = pIMsRdpClient->raw_RequestClose(pCloseStatus);
			dbgprintf(TEXT("IMsRdpClient::RequestClose -> %08X, CloseStatus = %ld"), hr, *pCloseStatus);
			return hr;
		}

		/* IMsRdpClient2 */
	public:
		virtual HRESULT __stdcall get_AdvancedSettings3(IMsRdpClientAdvancedSettings2 ** ppAdvSettings)
		{
			IMsRdpClient2 * pIMsRdpClient2 = getIMsRdpClient2();
			dbgprintf(TEXT("IMsRdpClient2::get_AdvancedSettings3(%p)"), ppAdvSettings);
			HRESULT hr = pIMsRdpClient2->get_AdvancedSettings3(ppAdvSettings);
			dbgprintf(TEXT("IMsRdpClient2::get_AdvancedSettings3 -> %08X, pAdvSettings = %p"), hr, *ppAdvSettings);
			return hr;
		}

		virtual HRESULT __stdcall put_ConnectedStatusText(BSTR pConnectedStatusText)
		{
			IMsRdpClient2 * pIMsRdpClient2 = getIMsRdpClient2();
			dbgprintf(TEXT("IMsRdpClient2::put_ConnectedStatusText(%ls)"), pConnectedStatusText);
			HRESULT hr = pIMsRdpClient2->put_ConnectedStatusText(pConnectedStatusText);
			dbgprintf(TEXT("IMsRdpClient2::put_ConnectedStatusText -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_ConnectedStatusText(BSTR * pConnectedStatusText)
		{
			IMsRdpClient2 * pIMsRdpClient2 = getIMsRdpClient2();
			dbgprintf(TEXT("IMsRdpClient2::get_ConnectedStatusText(%p)"), pConnectedStatusText);
			HRESULT hr = pIMsRdpClient2->get_ConnectedStatusText(pConnectedStatusText);
			dbgprintf(TEXT("IMsRdpClient2::get_ConnectedStatusText -> %08X, ConnectedStatusText = %ls"), hr, *pConnectedStatusText);
			return hr;
		}

		/* IMsRdpClient3 */
	public:
		virtual HRESULT __stdcall get_AdvancedSettings4(IMsRdpClientAdvancedSettings3 ** ppAdvSettings)
		{
			IMsRdpClient3 * pIMsRdpClient3 = getIMsRdpClient3();
			dbgprintf(TEXT("IMsRdpClient3::get_AdvancedSettings4(%p)"), ppAdvSettings);
			HRESULT hr = pIMsRdpClient3->get_AdvancedSettings4(ppAdvSettings);
			dbgprintf(TEXT("IMsRdpClient3::get_AdvancedSettings4 -> %08X, pAdvSettings = %p"), hr, *ppAdvSettings);
			return hr;
		}

		/* IMsRdpClient4 */
	public:
		virtual HRESULT __stdcall get_AdvancedSettings5(IMsRdpClientAdvancedSettings4 ** ppAdvSettings5)
		{
			IMsRdpClient4 * pIMsRdpClient4 = getIMsRdpClient4();
			dbgprintf(TEXT("IMsRdpClient4::get_AdvancedSettings5(%p)"), ppAdvSettings5);
			HRESULT hr = pIMsRdpClient4->get_AdvancedSettings5(ppAdvSettings5);
			dbgprintf(TEXT("IMsRdpClient4::get_AdvancedSettings5 -> %08X, pAdvSettings5 = %p"), hr, *ppAdvSettings5);
			return hr;
		}

		/* IMsTscNonScriptable */
	public:
		virtual HRESULT __stdcall put_ClearTextPassword(BSTR _arg1)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::put_ClearTextPassword(%ls)"), _arg1);
			HRESULT hr = pIMsTscNonScriptable->put_ClearTextPassword(_arg1);
			dbgprintf(TEXT("IMsTscNonScriptable::put_ClearTextPassword -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall put_PortablePassword(BSTR pPortablePass)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::put_PortablePassword(%ls)"), pPortablePass);
			HRESULT hr = pIMsTscNonScriptable->put_PortablePassword(pPortablePass);
			dbgprintf(TEXT("IMsTscNonScriptable::put_PortablePassword -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_PortablePassword(BSTR * pPortablePass)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::get_PortablePassword(%p)"), pPortablePass);
			HRESULT hr = pIMsTscNonScriptable->get_PortablePassword(pPortablePass);
			dbgprintf(TEXT("IMsTscNonScriptable::get_PortablePassword -> %08X, PortablePass = %ls"), hr, *pPortablePass);
			return hr;
		}

		virtual HRESULT __stdcall put_PortableSalt(BSTR pPortableSalt)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::put_PortableSalt(%ls)"), pPortableSalt);
			HRESULT hr = pIMsTscNonScriptable->put_PortableSalt(pPortableSalt);
			dbgprintf(TEXT("IMsTscNonScriptable::put_PortableSalt -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_PortableSalt(BSTR * pPortableSalt)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::get_PortableSalt()"), pPortableSalt);
			HRESULT hr = pIMsTscNonScriptable->get_PortableSalt(pPortableSalt);
			dbgprintf(TEXT("IMsTscNonScriptable::get_PortableSalt -> %08X, PortableSalt = %ls"), hr, *pPortableSalt);
			return hr;
		}

		virtual HRESULT __stdcall put_BinaryPassword(BSTR pBinaryPassword)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::put_BinaryPassword(%p)"), pBinaryPassword);
			HRESULT hr = pIMsTscNonScriptable->put_BinaryPassword(pBinaryPassword);
			dbgprintf(TEXT("IMsTscNonScriptable::put_BinaryPassword -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_BinaryPassword(BSTR * pBinaryPassword)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::get_BinaryPassword()"), pBinaryPassword);
			HRESULT hr = pIMsTscNonScriptable->get_BinaryPassword(pBinaryPassword);
			dbgprintf(TEXT("IMsTscNonScriptable::get_BinaryPassword -> %08X, BinaryPassword = %ls"), hr, *pBinaryPassword);
			return hr;
		}

		virtual HRESULT __stdcall put_BinarySalt(BSTR pSalt)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::put_BinarySalt(%p)"), pSalt);
			HRESULT hr = pIMsTscNonScriptable->put_BinarySalt(pSalt);
			dbgprintf(TEXT("IMsTscNonScriptable::put_BinarySalt -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall get_BinarySalt(BSTR * pSalt)
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::get_BinarySalt()"), pSalt);
			HRESULT hr = pIMsTscNonScriptable->get_BinarySalt(pSalt);
			dbgprintf(TEXT("IMsTscNonScriptable::get_BinarySalt -> %08X, pSalt = %ls"), hr, *pSalt);
			return hr;
		}

		virtual HRESULT __stdcall raw_ResetPassword()
		{
			IMsTscNonScriptable * pIMsTscNonScriptable = getIMsTscNonScriptable();
			dbgprintf(TEXT("IMsTscNonScriptable::ResetPassword()"));
			HRESULT hr = pIMsTscNonScriptable->raw_ResetPassword();
			dbgprintf(TEXT("IMsTscNonScriptable::ResetPassword -> %08X"), hr);
			return hr;
		}


		/* IMsRdpClientNonScriptable */
	public:
		virtual HRESULT __stdcall IMsRdpClientNonScriptable::raw_NotifyRedirectDeviceChange(UINT_PTR wParam, LONG_PTR lParam)
		{
			IMsRdpClientNonScriptable * pIMsRdpClientNonScriptable = getIMsRdpClientNonScriptable();
			dbgprintf(TEXT("IMsRdpClientNonScriptable::NotifyRedirectDeviceChange(%p, %p)"), wParam, lParam);
			HRESULT hr = pIMsRdpClientNonScriptable->raw_NotifyRedirectDeviceChange(wParam, lParam);
			dbgprintf(TEXT("IMsRdpClientNonScriptable::NotifyRedirectDeviceChange -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall IMsRdpClientNonScriptable::raw_SendKeys(long numKeys, VARIANT_BOOL * pbArrayKeyUp, long * plKeyData) // TBD
		{
			IMsRdpClientNonScriptable * pIMsRdpClientNonScriptable = getIMsRdpClientNonScriptable();
			dbgprintf(TEXT("IMsRdpClientNonScriptable::SendKeys(%ld, %p, %p)"), numKeys, pbArrayKeyUp, plKeyData);
			HRESULT hr = pIMsRdpClientNonScriptable->raw_SendKeys(numKeys, pbArrayKeyUp, plKeyData);
			dbgprintf(TEXT("IMsRdpClientNonScriptable::SendKeys -> %08X"), hr);
			return hr;
		}

		/* IMsRdpClientNonScriptable2 */
	public:
		virtual HRESULT __stdcall IMsRdpClientNonScriptable2::put_UIParentWindowHandle(wireHWND phwndUIParentWindowHandle)
		{
			IMsRdpClientNonScriptable2 * pIMsRdpClientNonScriptable2 = getIMsRdpClientNonScriptable2();
			dbgprintf(TEXT("IMsRdpClientNonScriptable2::put_UIParentWindowHandle(%p)"), phwndUIParentWindowHandle);
			HRESULT hr = pIMsRdpClientNonScriptable2->put_UIParentWindowHandle(phwndUIParentWindowHandle);
			dbgprintf(TEXT("IMsRdpClientNonScriptable2::put_UIParentWindowHandle -> %08X"), hr);
			return hr;
		}

		virtual HRESULT __stdcall IMsRdpClientNonScriptable2::get_UIParentWindowHandle(wireHWND * phwndUIParentWindowHandle)
		{
			IMsRdpClientNonScriptable2 * pIMsRdpClientNonScriptable2 = getIMsRdpClientNonScriptable2();
			dbgprintf(TEXT("IMsRdpClientNonScriptable2::get_UIParentWindowHandle(%p)"), phwndUIParentWindowHandle);
			HRESULT hr = pIMsRdpClientNonScriptable2->get_UIParentWindowHandle(phwndUIParentWindowHandle);
			dbgprintf(TEXT("IMsRdpClientNonScriptable2::get_UIParentWindowHandle -> %08X, hwndUIParentWindowHandle = %p"), hr, *phwndUIParentWindowHandle);
			return hr;
		}

		/*
		{
			 * p = m_;
			dbgprintf(TEXT("::()"), );
			HRESULT hr = p->();
			dbgprintf(TEXT(":: -> %08X, "), hr, );
			return hr;
		}
		*/
	};

	class ClassFactory: public IClassFactory2
	{
	private:
		LONG m_refCount;
		IUnknown * m_IUnknown;
		IClassFactory * m_IClassFactory;
		IClassFactory2 * m_IClassFactory2;

		IClassFactory * getIClassFactory()
		{
			if(m_IClassFactory)
				return m_IClassFactory;

			if(m_IClassFactory2)
				m_IClassFactory = m_IClassFactory2;

			if(m_IClassFactory)
			{
				m_IClassFactory->AddRef();
				return m_IClassFactory;
			}

			m_IUnknown->QueryInterface(&m_IClassFactory);
			return m_IClassFactory;
		}

		IClassFactory2 * getIClassFactory2()
		{
			if(m_IClassFactory2)
				return m_IClassFactory2;

			m_IUnknown->QueryInterface(&m_IClassFactory2);
			return m_IClassFactory2;
		}

	public:
		ClassFactory(IUnknown * pUnknwn):
			m_refCount(1),
			m_IUnknown(pUnknwn),
			m_IClassFactory(NULL),
			m_IClassFactory2(NULL)
		{
			m_IUnknown->AddRef();
		}

		~ClassFactory()
		{
			if(m_IUnknown)
				m_IUnknown->Release();

			if(m_IClassFactory)
				m_IClassFactory->Release();

			if(m_IClassFactory2)
				m_IClassFactory2->Release();
		}

		/* IUnknown */
	public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject)
		{
			HRESULT hr;
			IUnknown * pvObject;
			
			dbgprintf(TEXT("IUnknown::QueryInterface(%ls, %p)"), UUIDToString(riid).c_str(), ppvObject);

#define QIBEGIN() \
	if(riid == IID_IUnknown) \
	{ \
		hr = S_OK; \
		pvObject = (IUnknown *)(this); \
	}

#define QI(I) \
	else if(riid == IID_ ## I) \
	{ \
		if(m_ ## I) \
		{ \
			m_ ## I->AddRef(); \
			hr = S_OK; \
		} \
		else \
		{ \
			hr = m_IUnknown->QueryInterface(&m_ ## I); \
		} \
 \
		if(SUCCEEDED(hr)) \
			pvObject = static_cast<I *>(this); \
	}

#define QIEND() \
	else \
	{ \
		hr = E_NOINTERFACE; \
		pvObject = (IUnknown *)(this); \
	}

			QIBEGIN()
			QI(IClassFactory)
			QI(IClassFactory2)
			QIEND()

#undef QIBEGIN
#undef QIEND
#undef QI

			if(SUCCEEDED(hr))
			{
				assert(pvObject);
				pvObject->AddRef();
			}
			else
			{
				assert(pvObject == NULL);
			}

			*ppvObject = pvObject;

			dbgprintf(TEXT("IUnknown::QueryInterface -> %08X, ppvObject = %p"), hr, *ppvObject);
			return hr;
		}

        virtual ULONG STDMETHODCALLTYPE AddRef(void)
		{
			return InterlockedIncrement(&m_refCount);
		}

        virtual ULONG STDMETHODCALLTYPE Release(void)
		{
			LONG n = InterlockedDecrement(&m_refCount);

			if(n == 0)
				delete this;

			return n;
		}

		/* IClassFactory */
	public:
        virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)
		{
			IClassFactory * pIClassFactory = getIClassFactory();
			dbgprintf(TEXT("IClassFactory::CreateInstance(%p, %ls, %p)"), pUnkOuter, UUIDToString(riid).c_str(), ppvObject);
			HRESULT hr = pIClassFactory->CreateInstance(pUnkOuter, riid, ppvObject);
			dbgprintf(TEXT("IClassFactory::CreateInstance -> %08X, pvObject = %p"), hr, *ppvObject);

			IUnknown * pv = NULL;

			if(SUCCEEDED(hr))
			{
				IUnknown * punk = (IUnknown *)*ppvObject;

				if(pUnkOuter)
					hr = CLASS_E_NOAGGREGATION;
				else
					pv = new CoClass(punk);

				punk->Release();
			}

			if(pv)
			{
				hr = pv->QueryInterface(riid, ppvObject);

				if(FAILED(hr))
					pv->Release();
			}

			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock)
		{
			IClassFactory * pIClassFactory = getIClassFactory();
			dbgprintf(TEXT("IClassFactory::LockServer(%s)"), BooleanToString(fLock));
			HRESULT hr = pIClassFactory->LockServer(fLock);
			dbgprintf(TEXT("IClassFactory::LockServer -> %08X"), hr);
			return hr;
		}

		/* IClassFactory2 */
	public:
        virtual HRESULT STDMETHODCALLTYPE GetLicInfo(LICINFO * pLicInfo)
		{
			IClassFactory2 * pIClassFactory2 = getIClassFactory2();
			dbgprintf(TEXT("IClassFactory2::GetLicInfo(%p)"), pLicInfo);
			HRESULT hr = pIClassFactory2->GetLicInfo(pLicInfo);
			dbgprintf(TEXT("IClassFactory2::GetLicInfo -> %08X, LicInfo = %p"), hr, pLicInfo);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE RequestLicKey(DWORD dwReserved, BSTR * pBstrKey)
		{
			IClassFactory2 * pIClassFactory2 = getIClassFactory2();
			dbgprintf(TEXT("IClassFactory2::RequestLicKey(%lu, %p)"), dwReserved, pBstrKey);
			HRESULT hr = pIClassFactory2->RequestLicKey(dwReserved, pBstrKey);
			dbgprintf(TEXT("IClassFactory2::RequestLicKey -> %08X, bstrKey = %ls"), hr, *pBstrKey);
			return hr;
		}

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceLic(IUnknown * pUnkOuter, IUnknown * pUnkReserved, REFIID riid, BSTR bstrKey, PVOID * ppvObj)
		{
			IClassFactory2 * pIClassFactory2 = getIClassFactory2();
			dbgprintf(TEXT("IClassFactory2::CreateInstanceLic(%p, %p, %ls, %ls, %p)"), pUnkOuter, pUnkReserved, UUIDToString(riid).c_str(), bstrKey, ppvObj);
			HRESULT hr = pIClassFactory2->CreateInstanceLic(pUnkOuter, pUnkReserved, riid, bstrKey, ppvObj);
			dbgprintf(TEXT("IClassFactory2::CreateInstanceLic -> %08X, pvObj = %p"), hr, *ppvObj);

			IUnknown * pv = NULL;

			if(SUCCEEDED(hr))
			{
				IUnknown * punk = (IUnknown *)*ppvObj;

				if(pUnkOuter)
					hr = CLASS_E_NOAGGREGATION;
				else
					pv = new CoClass(punk);

				punk->Release();
			}

			if(pv)
			{
				hr = pv->QueryInterface(riid, ppvObj);

				if(FAILED(hr))
					pv->Release();
			}

			return hr;
		}
	};

	STDAPI DllGetClassObject(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID * ppv)
	{
		init();

		dbgprintf(TEXT("DllGetClassObject(%ls, %ls, %p)"), UUIDToString(rclsid).c_str(), UUIDToString(riid).c_str(), ppv);
		HRESULT hr = pfnDllGetClassObject(rclsid, IID_IUnknown, ppv);
		dbgprintf(TEXT("DllGetClassObject -> %08X, pv = %p"), hr, *ppv);

		IUnknown * pv = NULL;

		if(SUCCEEDED(hr))
		{
			IUnknown * punk = (IUnknown *)*ppv;

			if(rclsid == CLSID_MsTscAx || rclsid == CLSID_MsRdpClient || rclsid == CLSID_MsRdpClient2 || rclsid == CLSID_MsRdpClient3 || rclsid == CLSID_MsRdpClient4)
				pv = new ClassFactory(punk);
			else
				hr = CLASS_E_CLASSNOTAVAILABLE;

			punk->Release();
		}

		if(pv)
		{
			hr = pv->QueryInterface(riid, ppv);

			if(FAILED(hr))
				pv->Release();
		}

		return hr;
	}

	STDAPI DllCanUnloadNow(void)
	{
		init();

		dbgprintf(TEXT("DllCanUnloadNow()"));
		HRESULT hr = pfnDllCanUnloadNow();
		dbgprintf(TEXT("DllCanUnloadNow -> %08X"), hr);

		return hr;
	}

	STDAPI_(ULONG) DllGetTscCtlVer(void)
	{
		init();

		dbgprintf(TEXT("DllGetTscCtlVer()"));
		ULONG ul = pfnDllGetTscCtlVer();
		dbgprintf(TEXT("DllGetTscCtlVer-> %08X"), ul);

		return ul;
	}

}

// EOF
