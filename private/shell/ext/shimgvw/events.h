// Events.h: Definition of the FooBarEvents class
//
//////////////////////////////////////////////////////////////////////

#if !defined(__EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_)
#define __EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "shimgvw.h"

/////////////////////////////////////////////////////////////////////////////
// FooBarEvents

template <class T>
class CPreviewEvents : public IConnectionPointImpl<T, &DIID_DPreviewEvents, CComDynamicUnkArray>
{
public:
	void OnClose()
	{
        Invoke( 0x1 );
	}
    void OnPreviewReady()
    {
        Invoke( 0x2 );
    }
protected:
    void Invoke( DWORD dwID )
    {
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(dwID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
    }
};

#endif // !defined(__EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_)
