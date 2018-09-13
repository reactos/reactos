
//////////////////////////////////////////////////////////////////////////////
// CProxy_DIPropertyTreeCtl
template <class T>
class CProxy_DIPropertyTreeCtl : public IConnectionPointImpl<T, &DIID__DIPropertyTreeCtl, CComDynamicUnkArray>
{
public:
//methods:
//_DIPropertyTreeCtl : IDispatch
public:
	void Fire_Emptied(
		long nReason)
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
				pvars[0].lVal= nReason;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_PropertyShow(
		BSTR FmtID,
		long nPropID,
		long nDataType)
	{
		VARIANTARG* pvars = new VARIANTARG[3];
		for (int i = 0; i < 3; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[2].vt = VT_BSTR;
				pvars[2].bstrVal= FmtID;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= nPropID;
				pvars[0].vt = VT_I4;
				pvars[0].lVal= nDataType;
				DISPPARAMS disp = { pvars, NULL, 3, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_PropertyHide(
		BSTR FmtID,
		long nPropID,
		long nDataType)
	{
		VARIANTARG* pvars = new VARIANTARG[3];
		for (int i = 0; i < 3; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[2].vt = VT_BSTR;
				pvars[2].bstrVal= FmtID;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= nPropID;
				pvars[0].vt = VT_I4;
				pvars[0].lVal= nDataType;
				DISPPARAMS disp = { pvars, NULL, 3, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_PropertyGetFocus(
		BSTR FmtID,
		long nPropID,
		long nDataType)
	{
		VARIANTARG* pvars = new VARIANTARG[3];
		for (int i = 0; i < 3; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[2].vt = VT_BSTR;
				pvars[2].bstrVal= FmtID;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= nPropID;
				pvars[0].vt = VT_I4;
				pvars[0].lVal= nDataType;
				DISPPARAMS disp = { pvars, NULL, 3, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x4, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_PropertyKillFocus(
		BSTR FmtID,
		long nPropID,
		long nDataType)
	{
		VARIANTARG* pvars = new VARIANTARG[3];
		for (int i = 0; i < 3; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[2].vt = VT_BSTR;
				pvars[2].bstrVal= FmtID;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= nPropID;
				pvars[0].vt = VT_I4;
				pvars[0].lVal= nDataType;
				DISPPARAMS disp = { pvars, NULL, 3, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x5, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_FolderShow(
		long nFolderID)
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
				pvars[0].lVal= nFolderID;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x6, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_FolderHide(
		long nFolderID)
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
				pvars[0].lVal= nFolderID;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x7, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_FolderGetFocus(
		long nFolderID)
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
				pvars[0].lVal= nFolderID;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x8, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_FolderKillFocus(
		long nFolderID)
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
				pvars[0].lVal= nFolderID;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0x9, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_PropertyDirty(
		BSTR FmtID,
		long nPropID,
		long nDataType,
		VARIANT_BOOL bDirty)
	{
		VARIANTARG* pvars = new VARIANTARG[4];
		for (int i = 0; i < 4; i++)
			VariantInit(&pvars[i]);
		T* pT = (T*)this;
		pT->Lock();
		IUnknown** pp = m_vec.begin();
		while (pp < m_vec.end())
		{
			if (*pp != NULL)
			{
				pvars[3].vt = VT_BSTR;
				pvars[3].bstrVal= FmtID;
				pvars[2].vt = VT_I4;
				pvars[2].lVal= nPropID;
				pvars[1].vt = VT_I4;
				pvars[1].lVal= nDataType;
				pvars[0].vt = VT_BOOL;
				pvars[0].boolVal= bDirty;
				DISPPARAMS disp = { pvars, NULL, 4, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0xa, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}
	void Fire_DirtyCountChanged(
		long cDirtyCount,
		long cDirtyCountVis)
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
				pvars[1].vt = VT_I4;
				pvars[1].lVal= cDirtyCount;
				pvars[0].vt = VT_I4;
				pvars[0].lVal= cDirtyCountVis;
				DISPPARAMS disp = { pvars, NULL, 2, 0 };
				IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
				pDispatch->Invoke(0xb, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
			pp++;
		}
		pT->Unlock();
		delete[] pvars;
	}

};

