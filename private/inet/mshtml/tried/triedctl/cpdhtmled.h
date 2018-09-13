// CProxy_DHTMLEditEvents
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#include "triedtctlid.h"

template <class T>
class CProxy_DHTMLSafeEvents : public IConnectionPointImpl<T, &DIID__DHTMLSafeEvents, CComDynamicUnkArray>
{
public:
//methods:
//_DHTMLEditEvents : IDispatch
public:

	void Fire_Generic_Event ( DISPID dispid )
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
				pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
	}
	VARIANT_BOOL Fire_Generic_Boolean_Event ( DISPID dispid )
	{
		VARIANT_BOOL	vbCancel = FALSE;
		CComVariant		varCancel ( VARIANT_FALSE );
		T*				pT = (T*)this;

		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			varCancel.ChangeType ( VT_BOOL );
			if ( varCancel.boolVal )
			{
				break;	// give up once we've received a cancel.
			}
			if (*pp != NULL)
			{
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varCancel, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		return varCancel.boolVal;
	}
	void Fire_DocumentComplete()
	{
		Fire_Generic_Event ( DISPID_DOCUMENTCOMPLETE );
	}
	void Fire_DisplayChanged()
	{
		Fire_Generic_Event ( DISPID_DISPLAYCHANGED );
	}
	void Fire_ShowContextMenu( long xPos, long yPos)
	{
		VARIANTARG* pvars = new VARIANTARG[2];
		for (int i = 0; i < 2; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[0].vt = VT_I4;
				pvars[0].lVal= yPos;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= xPos;
				DISPPARAMS disp = { pvars, NULL, 2, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(DISPID_SHOWCONTEXTMENU, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_ContextMenuAction(
		long itemIndex)
	{
		VARIANTARG* pvars = new VARIANTARG[1];
		for (int i = 0; i < 1; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[0].vt = VT_I4;
				pvars[0].lVal= itemIndex;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(DISPID_CONTEXTMENUACTION, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_onmousedown ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEDOWN );
	}
	void Fire_onmousemove ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEMOVE );
	}
	void Fire_onmouseup ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEUP );
	}
	void Fire_onmouseout ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEOUT );
	}
	void Fire_onmouseover ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEOVER );
	}
	void Fire_onclick ()
	{
		Fire_Generic_Event ( DISPID_ONCLICK );
	}
	void Fire_ondblclick ()
	{
		Fire_Generic_Event ( DISPID_ONDBLCLICK );
	}
	void Fire_onkeydown ()
	{
		Fire_Generic_Event ( DISPID_ONKEYDOWN );
	}
	void Fire_onkeypress ()
	{
		Fire_Generic_Event ( DISPID_ONKEYPRESS );
	}
	void Fire_onkeyup ()
	{
		Fire_Generic_Event ( DISPID_ONKEYUP );
	}
	void Fire_onblur ()
	{
		Fire_Generic_Event ( DISPID_ONBLUR );
	}
	void Fire_onreadystatechange ()
	{
		Fire_Generic_Event ( DISPID_ONREADYSTATECHANGE );
	}
};


template <class T>
class CProxy_DHTMLEditEvents : public IConnectionPointImpl<T, &DIID__DHTMLEditEvents, CComDynamicUnkArray>
{
public:
//methods:
//_DHTMLEditEvents : IDispatch
public:

	void Fire_Generic_Event ( DISPID dispid )
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
				pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
	}
	VARIANT_BOOL Fire_Generic_Boolean_Event ( DISPID dispid )
	{
		VARIANT_BOOL	vbCancel = FALSE;
		CComVariant		varCancel = VARIANT_FALSE;
		T*				pT = (T*)this;

		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			varCancel.ChangeType ( VT_BOOL );
			if ( VARIANT_TRUE == varCancel.boolVal )
			{
				break;	// give up once we've received a cancel.
			}
			if (*pp != NULL)
			{
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varCancel, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		return varCancel.boolVal;
	}
	void Fire_DocumentComplete()
	{
		Fire_Generic_Event ( DISPID_DOCUMENTCOMPLETE );
	}
	void Fire_DisplayChanged()
	{
		Fire_Generic_Event ( DISPID_DISPLAYCHANGED );
	}
	void Fire_ShowContextMenu( long xPos, long yPos)
	{
		VARIANTARG* pvars = new VARIANTARG[2];
		for (int i = 0; i < 2; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[0].vt = VT_I4;
				pvars[0].lVal= yPos;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= xPos;
				DISPPARAMS disp = { pvars, NULL, 2, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(DISPID_SHOWCONTEXTMENU, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_ContextMenuAction(
		long itemIndex)
	{
		VARIANTARG* pvars = new VARIANTARG[1];
		for (int i = 0; i < 1; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[0].vt = VT_I4;
				pvars[0].lVal= itemIndex;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(DISPID_CONTEXTMENUACTION, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_onmousedown ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEDOWN );
	}
	void Fire_onmousemove ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEMOVE );
	}
	void Fire_onmouseup ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEUP );
	}
	void Fire_onmouseout ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEOUT );
	}
	void Fire_onmouseover ()
	{
		Fire_Generic_Event ( DISPID_ONMOUSEOVER );
	}
	void Fire_onclick ()
	{
		Fire_Generic_Event ( DISPID_ONCLICK );
	}
	void Fire_ondblclick ()
	{
		Fire_Generic_Event ( DISPID_ONDBLCLICK );
	}
	void Fire_onkeydown ()
	{
		Fire_Generic_Event ( DISPID_ONKEYDOWN );
	}
	void Fire_onkeypress ()
	{
		Fire_Generic_Event ( DISPID_ONKEYPRESS );
	}
	void Fire_onkeyup ()
	{
		Fire_Generic_Event ( DISPID_ONKEYUP );
	}
	void Fire_onblur ()
	{
		Fire_Generic_Event ( DISPID_ONBLUR );
	}
	void Fire_onreadystatechange ()
	{
		Fire_Generic_Event ( DISPID_ONREADYSTATECHANGE );
	}
};

