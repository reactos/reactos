#ifndef __JSPROXY_H__
#define __JSPROXY_H__

#include <windows.h>
#include <olectl.h>
#include "utils.h"
#include "regexp.h"

#define VAL_isPlainHostName			0x01f9
#define VAL_dnsDomainIs				0x01f8
#define VAL_localHostOrDomainIs		0x020b
#define VAL_isResolvable			0x0206
#define VAL_isInNet					0x01e1
#define VAL_dnsResolve				0x01fc
#define VAL_myIpAddress				0x01e0
#define VAL_dnsDomainLevels			0x01f8
#define VAL_shExpMatch				0x0208
#define VAL_weekdayRange			0x0210
#define VAL_dateRange				0x01f0
#define VAL_timeRange				0x0201
#define VAL_alert				0x0218

#define DISPID_isPlainHostName		0x0001
#define DISPID_dnsDomainIs			0x0002
#define DISPID_localHostOrDomainIs	0x0003
#define DISPID_isResolvable			0x0004
#define DISPID_isInNet				0x0005
#define DISPID_dnsResolve			0x0006
#define DISPID_myIpAddress			0x0007
#define DISPID_dnsDomainLevels		0x0008
#define DISPID_shExpMatch			0x0009
#define DISPID_weekdayRange			0x000a
#define DISPID_dateRange			0x000b
#define DISPID_timeRange			0x000c
#define DISPID_alert				0x000d

/************************************************************************************************/
// This class implements the Dispatch interface that will allow the script engine to call the 
// auto-proxy configuration functions.  This interface does not have a typelib and does not provide type
// info.
class CJSProxy : public IDispatch
{

public:

	CJSProxy();
	~CJSProxy();
	// IUnknown Methods
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObject)
	{

#ifdef INET_DEBUG
        OutputDebugString( "IDispatch::QueryInterface\n" );
#endif

		if (riid == IID_IUnknown || 
            riid == IID_IDispatch)
		{
			*ppvObject = (LPVOID)(LPUNKNOWN)this;
            AddRef();
			return S_OK;
		}
		else
		{
			if (riid == IID_IDispatch)
			{
				*ppvObject = (LPVOID)(IDispatch*)this;
                AddRef();
				return S_OK;
			}
			else
			{
				*ppvObject = 0;
				return E_NOINTERFACE;
			}
		}

	}

	STDMETHODIMP_(ULONG) AddRef()
	{
#ifdef INET_DEBUG
        char szBuff[256];

        wsprintf(szBuff, "IDispatch::AddRef ref=%u\n", m_refCount );
        OutputDebugString(szBuff);
#endif

		return ++m_refCount;
	}

	STDMETHODIMP_(ULONG) Release()
	{
#ifdef INET_DEBUG
        char szBuff[256];

        wsprintf(szBuff, "IDispatch::Release ref=%u\n", m_refCount );
        OutputDebugString(szBuff);
#endif

		if (--m_refCount)
			return m_refCount;

		delete this;
		return 0;
	}

	// IDispatch Methods
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)
	{
		*pctinfo = 0;
		return S_OK;
	}

    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return TYPE_E_ELEMENTNOTFOUND;
	}

    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames,UINT cNames, LCID lcid, DISPID FAR* rgdispid);

    STDMETHODIMP Invoke(
					DISPID dispidMember,
					REFIID riid,
					LCID lcid,
					WORD wFlags,
					DISPPARAMS* pdispparams,
					VARIANT* pvarResult,
					EXCEPINFO* pexcepinfo,
					UINT* puArgErr);

	//  JScript Auto-Proxy config functions.
	STDMETHODIMP isPlainHostName(BSTR host, VARIANT* retval);
	STDMETHODIMP dnsDomainIs(BSTR host,BSTR domain, VARIANT* retval);
	STDMETHODIMP localHostOrDomainIs(BSTR host,BSTR hostdom, VARIANT* retval);
	STDMETHODIMP isResolvable(BSTR host, VARIANT* retval);
	STDMETHODIMP isInNet(BSTR host, BSTR pattern, BSTR mask, VARIANT* retval);
	STDMETHODIMP dnsResolve(BSTR host, VARIANT* retval);
	STDMETHODIMP myIpAddress(VARIANT* retval);
	STDMETHODIMP dnsDomainLevels(BSTR host, VARIANT* retval);
	STDMETHODIMP shExpMatch(BSTR str, BSTR shexp, VARIANT* retval);
	STDMETHODIMP alert(BSTR message, VARIANT* retval);

	// These are to do last!!!.
	STDMETHODIMP weekdayRange(BSTR wd1, BSTR wd2, BSTR gmt, VARIANT* retval);
	STDMETHODIMP dateRange(long day, BSTR month, BSTR gmt, VARIANT* retval);
	STDMETHODIMP timeRange(long hour, long min, long sec, BSTR gmt, VARIANT* retval);
	//	ProxyConfig.bindings 

	STDMETHODIMP Init(AUTO_PROXY_HELPER_APIS* pAPHA);
	STDMETHODIMP DeInit();
	
	// JScript private members
private:
	long					m_refCount;
	BOOL					m_fDestroyable;
	BOOL					m_fInitialized;
	AUTO_PROXY_HELPER_APIS*	m_pCallout;
	LPCWSTR					m_strings[13];
};

#endif