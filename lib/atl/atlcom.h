/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
 * Copyright 2013 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

namespace ATL
{

template <class Base, const IID *piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class CComEnum;

#define DECLARE_CLASSFACTORY_EX(cf) typedef ATL::CComCreator<ATL::CComObjectCached<cf> > _ClassFactoryCreatorClass;
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(ATL::CComClassFactory)

class CComObjectRootBase
{
public:
	LONG									m_dwRef;
public:
	CComObjectRootBase()
	{
		m_dwRef = 0;
	}

	~CComObjectRootBase()
	{
	}

	void SetVoid(void *)
	{
	}

	HRESULT _AtlFinalConstruct()
	{
		return S_OK;
	}

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void InternalFinalConstructAddRef()
	{
	}

	void InternalFinalConstructRelease()
	{
	}

	void FinalRelease()
	{
	}

	static void WINAPI ObjectMain(bool)
	{
	}

	static const struct _ATL_CATMAP_ENTRY *GetCategoryMap()
	{
		return NULL;
	}

	static HRESULT WINAPI InternalQueryInterface(void *pThis, const _ATL_INTMAP_ENTRY *pEntries, REFIID iid, void **ppvObject)
	{
		return AtlInternalQueryInterface(pThis, pEntries, iid, ppvObject);
	}

};

template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
private:
	typename ThreadModel::AutoDeleteCriticalSection		m_critsec;
public:
	~CComObjectRootEx()
	{
	}

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef >= 0);
		return ThreadModel::Increment(&m_dwRef);
	}

	ULONG InternalRelease()
	{
		ATLASSERT(m_dwRef > 0);
		return ThreadModel::Decrement(&m_dwRef);
	}

	void Lock()
	{
		m_critsec.Lock();
	}

	void Unlock()
	{
		m_critsec.Unlock();
	}

	HRESULT _AtlInitialConstruct()
	{
		return m_critsec.Init();
	}
};

template <class Base>
class CComObject : public Base
{
public:
	CComObject(void * = NULL)
	{
		_pAtlModule->Lock();
	}

	virtual ~CComObject()
	{
		this->FinalRelease();
		_pAtlModule->Unlock();
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return this->InternalAddRef();
	}

	STDMETHOD_(ULONG, Release)()
	{
		ULONG								newRefCount;

		newRefCount = this->InternalRelease();
		if (newRefCount == 0)
			delete this;
		return newRefCount;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		return this->_InternalQueryInterface(iid, ppvObject);
	}

	static HRESULT WINAPI CreateInstance(CComObject<Base> **pp)
	{
		CComObject<Base>					*newInstance;
		HRESULT								hResult;

		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;

		hResult = E_OUTOFMEMORY;
		newInstance = NULL;
		ATLTRY(newInstance = new CComObject<Base>())
		if (newInstance != NULL)
		{
			newInstance->SetVoid(NULL);
			newInstance->InternalFinalConstructAddRef();
			hResult = newInstance->_AtlInitialConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->FinalConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->_AtlFinalConstruct();
			newInstance->InternalFinalConstructRelease();
			if (hResult != S_OK)
			{
				delete newInstance;
				newInstance = NULL;
			}
		}
		*pp = newInstance;
		return hResult;
	}


};

template <class Base>
class CComContainedObject : public Base
{
public:
	IUnknown* m_pUnkOuter;
	CComContainedObject(void * pv = NULL) : m_pUnkOuter(static_cast<IUnknown*>(pv))
	{
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return m_pUnkOuter->AddRef();
	}

	STDMETHOD_(ULONG, Release)()
	{
		return m_pUnkOuter->Release();
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		return m_pUnkOuter->QueryInterface(iid, ppvObject);
	}

	IUnknown* GetControllingUnknown()
	{
		return m_pUnkOuter;
	}
};

template <class contained>
class CComAggObject : public contained
{
public:
	CComContainedObject<contained> m_contained;

	CComAggObject(void * pv = NULL) : m_contained(static_cast<contained*>(pv))
	{
		_pAtlModule->Lock();
	}

	virtual ~CComAggObject()
	{
		this->FinalRelease();
		_pAtlModule->Unlock();
	}

	HRESULT FinalConstruct()
	{
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		m_contained.FinalRelease();
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return this->InternalAddRef();
	}

	STDMETHOD_(ULONG, Release)()
	{
		ULONG newRefCount;
		newRefCount = this->InternalRelease();
		if (newRefCount == 0)
			delete this;
		return newRefCount;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		if (ppvObject == NULL)
			return E_POINTER;
		if (iid == IID_IUnknown)
			*ppvObject = reinterpret_cast<void*>(this);
		else
			return m_contained._InternalQueryInterface(iid, ppvObject);
		return S_OK;
	}

	static HRESULT WINAPI CreateInstance(IUnknown * punkOuter, CComAggObject<contained> **pp)
	{
		CComAggObject<contained>	*newInstance;
		HRESULT						hResult;

		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;

		hResult = E_OUTOFMEMORY;
		newInstance = NULL;
		ATLTRY(newInstance = new CComAggObject<contained>(punkOuter))
		if (newInstance != NULL)
		{
			newInstance->SetVoid(NULL);
			newInstance->InternalFinalConstructAddRef();
			hResult = newInstance->_AtlInitialConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->FinalConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->_AtlFinalConstruct();
			newInstance->InternalFinalConstructRelease();
			if (hResult != S_OK)
			{
				delete newInstance;
				newInstance = NULL;
			}
		}
		*pp = newInstance;
		return hResult;
	}
};

template <class contained>
class CComPolyObject : public contained
{
public:
	CComContainedObject<contained> m_contained;

	CComPolyObject(void * pv = NULL)
		: m_contained(pv ? static_cast<contained*>(pv) : this)
	{
		_pAtlModule->Lock();
	}

	virtual ~CComPolyObject()
	{
		this->FinalRelease();
		_pAtlModule->Unlock();
	}

	HRESULT FinalConstruct()
	{
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		m_contained.FinalRelease();
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return this->InternalAddRef();
	}

	STDMETHOD_(ULONG, Release)()
	{
		ULONG newRefCount;
		newRefCount = this->InternalRelease();
		if (newRefCount == 0)
			delete this;
		return newRefCount;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		if (ppvObject == NULL)
			return E_POINTER;
		if (iid == IID_IUnknown)
			*ppvObject = reinterpret_cast<void*>(this);
		else
			return m_contained._InternalQueryInterface(iid, ppvObject);
		return S_OK;
	}

	static HRESULT WINAPI CreateInstance(IUnknown * punkOuter, CComPolyObject<contained> **pp)
	{
		CComPolyObject<contained>	*newInstance;
		HRESULT						hResult;

		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;

		hResult = E_OUTOFMEMORY;
		newInstance = NULL;
		ATLTRY(newInstance = new CComPolyObject<contained>(punkOuter))
		if (newInstance != NULL)
		{
			newInstance->SetVoid(NULL);
			newInstance->InternalFinalConstructAddRef();
			hResult = newInstance->_AtlInitialConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->FinalConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->_AtlFinalConstruct();
			newInstance->InternalFinalConstructRelease();
			if (hResult != S_OK)
			{
				delete newInstance;
				newInstance = NULL;
			}
		}
		*pp = newInstance;
		return hResult;
	}
};

template <HRESULT hResult>
class CComFailCreator
{
public:
	static HRESULT WINAPI CreateInstance(void *, REFIID, LPVOID *ppv)
	{
		ATLASSERT(ppv != NULL);
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		return hResult;
	}
};

template <class T1>
class CComCreator
{
public:
	static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, LPVOID *ppv)
	{
		T1									*newInstance;
		HRESULT								hResult;

		ATLASSERT(ppv != NULL);
		if (ppv == NULL)
			return E_POINTER;
		*ppv = NULL;

		hResult = E_OUTOFMEMORY;
		newInstance = NULL;
		ATLTRY(newInstance = new T1(pv))
		if (newInstance != NULL)
		{
			newInstance->SetVoid(pv);
			newInstance->InternalFinalConstructAddRef();
			hResult = newInstance->_AtlInitialConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->FinalConstruct();
			if (SUCCEEDED(hResult))
				hResult = newInstance->_AtlFinalConstruct();
			newInstance->InternalFinalConstructRelease();
			if (SUCCEEDED(hResult))
				hResult = newInstance->QueryInterface(riid, ppv);
			if (FAILED(hResult))
			{
				delete newInstance;
				newInstance = NULL;
			}
		}
		return hResult;
	}
};

template <class T1, class T2>
class CComCreator2
{
public:
	static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, LPVOID *ppv)
	{
		ATLASSERT(ppv != NULL && riid != NULL);

		if (pv == NULL)
			return T1::CreateInstance(NULL, riid, ppv);
		else
			return T2::CreateInstance(pv, riid, ppv);
	}
};

template <class Base>
class CComObjectCached : public Base
{
public:
	CComObjectCached(void * = NULL)
	{
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		ULONG								newRefCount;

		newRefCount = this->InternalAddRef();
		if (newRefCount == 2)
			_pAtlModule->Lock();
		return newRefCount;
	}

	STDMETHOD_(ULONG, Release)()
	{
		ULONG								newRefCount;

		newRefCount = this->InternalRelease();
		if (newRefCount == 0)
			delete this;
		else if (newRefCount == 1)
			_pAtlModule->Unlock();
		return newRefCount;
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		return this->_InternalQueryInterface(iid, ppvObject);
	}
};

#define BEGIN_COM_MAP(x)														\
public:																			\
	typedef x _ComMapClass;														\
	HRESULT _InternalQueryInterface(REFIID iid, void **ppvObject)				\
	{																			\
		return this->InternalQueryInterface(this, _GetEntries(), iid, ppvObject);		\
	}																			\
	const static ATL::_ATL_INTMAP_ENTRY *WINAPI _GetEntries()					\
	{																			\
		static const ATL::_ATL_INTMAP_ENTRY _entries[] = {

#define END_COM_MAP()															\
			{NULL, 0, 0}														\
		};																		\
		return _entries;														\
	}																			\
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;								\
	virtual ULONG STDMETHODCALLTYPE Release() = 0;								\
	STDMETHOD(QueryInterface)(REFIID, void **) = 0;

#define COM_INTERFACE_ENTRY_IID(iid, x)											\
	{&iid, offsetofclass(x, _ComMapClass), _ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY(x)                                                  \
	{&_ATL_IIDOF(x),                                                            \
	offsetofclass(x, _ComMapClass),                                             \
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY2_IID(iid, x, x2)									\
	{&iid,																		\
			reinterpret_cast<DWORD_PTR>(static_cast<x *>(static_cast<x2 *>(reinterpret_cast<_ComMapClass *>(_ATL_PACKING)))) - _ATL_PACKING,	\
			_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_BREAK(x)											\
    {&_ATL_IIDOF(x),															\
    NULL,																		\
    _Break},    // Break is a function that issues int 3.

#define COM_INTERFACE_ENTRY_NOINTERFACE(x)										\
    {&_ATL_IIDOF(x),															\
    NULL,																		\
    _NoInterface}, // NoInterface returns E_NOINTERFACE.

#define COM_INTERFACE_ENTRY_FUNC(iid, dw, func)									\
    {&iid,																		\
    dw,																			\
    func},

#define COM_INTERFACE_ENTRY_FUNC_BLIND(dw, func)								\
    {NULL,																		\
    dw,																			\
    func},

#define COM_INTERFACE_ENTRY_CHAIN(classname)									\
    {NULL,																		\
    reinterpret_cast<DWORD>(&_CComChainData<classname, _ComMapClass>::data),	\
    _Chain},

#define DECLARE_NO_REGISTRY()\
	static HRESULT WINAPI UpdateRegistry(BOOL /*bRegister*/)					\
	{																			\
		return S_OK;															\
	}

#define DECLARE_REGISTRY_RESOURCEID(x)											\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)						\
	{																			\
		return ATL::_pAtlModule->UpdateRegistryFromResource(x, bRegister);		\
	}

#define DECLARE_NOT_AGGREGATABLE(x)												\
public:																			\
	typedef ATL::CComCreator2<ATL::CComCreator<ATL::CComObject<x> >, ATL::CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;

#define DECLARE_AGGREGATABLE(x)													\
public:																			\
	typedef ATL::CComCreator2<ATL::CComCreator<ATL::CComObject<x> >, ATL::CComCreator<ATL::CComAggObject<x> > > _CreatorClass;

#define DECLARE_ONLY_AGGREGATABLE(x)											\
public:																			\
	typedef ATL::CComCreator2<ATL::CComFailCreator<E_FAIL>, ATL::CComCreator<ATL::CComAggObject<x> > > _CreatorClass;

#define DECLARE_POLY_AGGREGATABLE(x)											\
public:																			\
	typedef ATL::CComCreator<ATL::CComPolyObject<x> > _CreatorClass;

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)                                \
	{&iid,                                                                      \
	(DWORD_PTR)offsetof(_ComMapClass, punk),                                    \
	_Delegate},

#define DECLARE_GET_CONTROLLING_UNKNOWN()										\
public:																			\
	virtual IUnknown *GetControllingUnknown()									\
	{																			\
		return GetUnknown();													\
	}

#define DECLARE_PROTECT_FINAL_CONSTRUCT()										\
	void InternalFinalConstructAddRef()											\
	{																			\
		InternalAddRef();														\
	}																			\
	void InternalFinalConstructRelease()										\
	{																			\
		InternalRelease();														\
	}

#define BEGIN_OBJECT_MAP(x) static ATL::_ATL_OBJMAP_ENTRY x[] = {

#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL}};

#define OBJECT_ENTRY(clsid, class)												\
{																				\
	&clsid,																		\
	class::UpdateRegistry,														\
	class::_ClassFactoryCreatorClass::CreateInstance,							\
	class::_CreatorClass::CreateInstance,										\
	NULL,																		\
	0,																			\
	class::GetObjectDescription,												\
	class::GetCategoryMap,														\
	class::ObjectMain },

class CComClassFactory :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	_ATL_CREATORFUNC						*m_pfnCreateInstance;

	virtual ~CComClassFactory()
	{
	}

public:
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void **ppvObj)
	{
		HRESULT								hResult;

		ATLASSERT(m_pfnCreateInstance != NULL);

		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;

		if (pUnkOuter != NULL && InlineIsEqualUnknown(riid) == FALSE)
			hResult = CLASS_E_NOAGGREGATION;
		else
			hResult = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
		return hResult;
	}

	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_pAtlModule->Lock();
		else
			_pAtlModule->Unlock();
		return S_OK;
	}

	void SetVoid(void *pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC *)pv;
	}

	BEGIN_COM_MAP(CComClassFactory)
		COM_INTERFACE_ENTRY_IID(IID_IClassFactory, IClassFactory)
	END_COM_MAP()
};

template <class T, const CLSID *pclsid = &CLSID_NULL>
class CComCoClass
{
public:
	DECLARE_CLASSFACTORY()

	static LPCTSTR WINAPI GetObjectDescription()
	{
		return NULL;
	}
};

template <class T>
class _Copy
{
public:
	static HRESULT copy(T *pTo, const T *pFrom)
	{
		memcpy(pTo, pFrom, sizeof(T));
		return S_OK;
	}

	static void init(T *)
	{
	}

	static void destroy(T *)
	{
	}
};

template<>
class _Copy<CONNECTDATA>
{
public:
	static HRESULT copy(CONNECTDATA *pTo, const CONNECTDATA *pFrom)
	{
		*pTo = *pFrom;
		if (pTo->pUnk)
			pTo->pUnk->AddRef();
		return S_OK;
	}

	static void init(CONNECTDATA *)
	{
	}

	static void destroy(CONNECTDATA *p)
	{
		if (p->pUnk)
			p->pUnk->Release();
	}
};

template <class T>
class _CopyInterface
{
public:
	static HRESULT copy(T **pTo, T **pFrom)
	{
		*pTo = *pFrom;
		if (*pTo)
			(*pTo)->AddRef();
		return S_OK;
	}

	static void init(T **)
	{
	}

	static void destroy(T **p)
	{
		if (*p)
			(*p)->Release();
	}
};

enum CComEnumFlags
{
	AtlFlagNoCopy = 0,
	AtlFlagTakeOwnership = 2,		// BitOwn
	AtlFlagCopy = 3					// BitOwn | BitCopy
};

template <class Base, const IID *piid, class T, class Copy>
class CComEnumImpl : public Base
{
private:
	typedef CComObject<CComEnum<Base, piid, T, Copy> > enumeratorClass;
public:
	CComPtr<IUnknown>						m_spUnk;
	DWORD									m_dwFlags;
	T										*m_begin;
	T										*m_end;
	T										*m_iter;
public:
	CComEnumImpl()
	{
		m_dwFlags = 0;
		m_begin = NULL;
		m_end = NULL;
		m_iter = NULL;
	}

	virtual ~CComEnumImpl()
	{
		T									*x;

		if ((m_dwFlags & BitOwn) != 0)
		{
			for (x = m_begin; x != m_end; x++)
				Copy::destroy(x);
			delete [] m_begin;
		}
	}

	HRESULT Init(T *begin, T *end, IUnknown *pUnk, CComEnumFlags flags = AtlFlagNoCopy)
	{
		T									*newBuffer;
		T									*sourcePtr;
		T									*destPtr;
		T									*cleanupPtr;
		HRESULT								hResult;

		if (flags == AtlFlagCopy)
		{
			ATLTRY(newBuffer = new T[end - begin])
			if (newBuffer == NULL)
				return E_OUTOFMEMORY;
			destPtr = newBuffer;
			for (sourcePtr = begin; sourcePtr != end; sourcePtr++)
			{
				Copy::init(destPtr);
				hResult = Copy::copy(destPtr, sourcePtr);
				if (FAILED(hResult))
				{
					cleanupPtr = m_begin;
					while (cleanupPtr < destPtr)
						Copy::destroy(cleanupPtr++);
					delete [] newBuffer;
					return hResult;
				}
				destPtr++;
			}
			m_begin = newBuffer;
			m_end = m_begin + (end - begin);
		}
		else
		{
			m_begin = begin;
			m_end = end;
		}
		m_spUnk = pUnk;
		m_dwFlags = flags;
		m_iter = m_begin;
		return S_OK;
	}

	STDMETHOD(Next)(ULONG celt, T *rgelt, ULONG *pceltFetched)
	{
		ULONG								numAvailable;
		ULONG								numToFetch;
		T									*rgeltTemp;
		HRESULT								hResult;

		if (pceltFetched != NULL)
			*pceltFetched = 0;
		if (celt == 0)
			return E_INVALIDARG;
		if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
			return E_POINTER;
		if (m_begin == NULL || m_end == NULL || m_iter == NULL)
			return E_FAIL;

		numAvailable = static_cast<ULONG>(m_end - m_iter);
		if (celt < numAvailable)
			numToFetch = celt;
		else
			numToFetch = numAvailable;
		if (pceltFetched != NULL)
			*pceltFetched = numToFetch;
		rgeltTemp = rgelt;
		while (numToFetch != 0)
		{
			hResult = Copy::copy(rgeltTemp, m_iter);
			if (FAILED(hResult))
			{
				while (rgelt < rgeltTemp)
					Copy::destroy(rgelt++);
				if (pceltFetched != NULL)
					*pceltFetched = 0;
				return hResult;
			}
			rgeltTemp++;
			m_iter++;
			numToFetch--;
		}
		if (numAvailable < celt)
			return S_FALSE;
		return S_OK;
	}

	STDMETHOD(Skip)(ULONG celt)
	{
		ULONG								numAvailable;
		ULONG								numToSkip;

		if (celt == 0)
			return E_INVALIDARG;

		numAvailable = static_cast<ULONG>(m_end - m_iter);
		if (celt < numAvailable)
			numToSkip = celt;
		else
			numToSkip = numAvailable;
		m_iter += numToSkip;
		if (numAvailable < celt)
			return S_FALSE;
		return S_OK;
	}

	STDMETHOD(Reset)()
	{
		m_iter = m_begin;
		return S_OK;
	}

	STDMETHOD(Clone)(Base **ppEnum)
	{
		enumeratorClass						*newInstance;
		HRESULT								hResult;

		hResult = E_POINTER;
		if (ppEnum != NULL)
		{
			*ppEnum = NULL;
			hResult = enumeratorClass::CreateInstance(&newInstance);
			if (SUCCEEDED(hResult))
			{
				hResult = newInstance->Init(m_begin, m_end, (m_dwFlags & BitOwn) ? this : m_spUnk);
				if (SUCCEEDED(hResult))
				{
					newInstance->m_iter = m_iter;
					hResult = newInstance->_InternalQueryInterface(*piid, (void **)ppEnum);
				}
				if (FAILED(hResult))
					delete newInstance;
			}
		}
		return hResult;
	}

protected:
	enum FlagBits
	{
		BitCopy = 1,
		BitOwn = 2
	};
};

template <class Base, const IID *piid, class T, class Copy, class ThreadModel>
class CComEnum :
	public CComEnumImpl<Base, piid, T, Copy>,
	public CComObjectRootEx<ThreadModel>
{
public:
	typedef CComEnum<Base, piid, T, Copy > _CComEnum;
	typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;

	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

class CComDynamicUnkArray
{
public:
	int										m_nSize;
	IUnknown								**m_ppUnk;
public:
	CComDynamicUnkArray()
	{
		m_nSize = 0;
		m_ppUnk = NULL;
	}

	~CComDynamicUnkArray()
	{
		free(m_ppUnk);
	}

	IUnknown **begin()
	{
		return m_ppUnk;
	}

	IUnknown **end()
	{
		return &m_ppUnk[m_nSize];
	}

	IUnknown *GetAt(int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if (nIndex >= 0 && nIndex < m_nSize)
			return m_ppUnk[nIndex];
		else
			return NULL;
	}

	IUnknown *WINAPI GetUnknown(DWORD dwCookie)
	{
		ATLASSERT(dwCookie != 0 && dwCookie <= static_cast<DWORD>(m_nSize));
		if (dwCookie != 0 && dwCookie <= static_cast<DWORD>(m_nSize))
			return GetAt(dwCookie - 1);
		else
			return NULL;
	}

	DWORD WINAPI GetCookie(IUnknown **ppFind)
	{
		IUnknown							**x;
		DWORD								curCookie;

		ATLASSERT(ppFind != NULL && *ppFind != NULL);
		if (ppFind != NULL && *ppFind != NULL)
		{
			curCookie = 1;
			for (x = begin(); x < end(); x++)
			{
				if (*x == *ppFind)
					return curCookie;
				curCookie++;
			}
		}
		return 0;
	}

	DWORD Add(IUnknown *pUnk)
	{
		IUnknown							**x;
		IUnknown							**newArray;
		int									newSize;
		DWORD								curCookie;

		ATLASSERT(pUnk != NULL);
		if (m_nSize == 0)
		{
			newSize = _DEFAULT_VECTORLENGTH * sizeof(IUnknown *);
			ATLTRY(newArray = reinterpret_cast<IUnknown **>(malloc(newSize)));
			if (newArray == NULL)
				return 0;
			memset(newArray, 0, newSize);
			m_ppUnk = newArray;
			m_nSize = _DEFAULT_VECTORLENGTH;
		}
		curCookie = 1;
		for (x = begin(); x < end(); x++)
		{
			if (*x == NULL)
			{
				*x = pUnk;
				return curCookie;
			}
			curCookie++;
		}
		newSize = m_nSize * 2;
		newArray = reinterpret_cast<IUnknown **>(realloc(m_ppUnk, newSize * sizeof(IUnknown *)));
		if (newArray == NULL)
			return 0;
		m_ppUnk = newArray;
		memset(&m_ppUnk[m_nSize], 0, (newSize - m_nSize) * sizeof(IUnknown *));
		curCookie = m_nSize + 1;
		m_nSize = newSize;
		m_ppUnk[curCookie - 1] = pUnk;
		return curCookie;
	}

	BOOL Remove(DWORD dwCookie)
	{
		DWORD								index;

		index = dwCookie - 1;
		ATLASSERT(index < dwCookie && index < static_cast<DWORD>(m_nSize));
		if (index < dwCookie && index < static_cast<DWORD>(m_nSize) && m_ppUnk[index] != NULL)
		{
			m_ppUnk[index] = NULL;
			return TRUE;
		}
		return FALSE;
	}

private:
	CComDynamicUnkArray &operator = (const CComDynamicUnkArray &)
	{
		return *this;
	}

	CComDynamicUnkArray(const CComDynamicUnkArray &)
	{
	}
};

struct _ATL_CONNMAP_ENTRY
{
	DWORD_PTR								dwOffset;
};

template <const IID *piid>
class _ICPLocator
{
public:
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void **ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
	virtual ULONG STDMETHODCALLTYPE Release() = 0;
};

template<class T, const IID *piid, class CDV = CComDynamicUnkArray>
class IConnectionPointImpl : public _ICPLocator<piid>
{
	typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA, _Copy<CONNECTDATA> > CComEnumConnections;
public:
	CDV										m_vec;
public:
	~IConnectionPointImpl()
	{
		IUnknown							**x;

		for (x = m_vec.begin(); x < m_vec.end(); x++)
			if (*x != NULL)
				(*x)->Release();
	}

	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void **ppvObject)
	{
		IConnectionPointImpl<T, piid, CDV>	*pThis;

		pThis = reinterpret_cast<IConnectionPointImpl<T, piid, CDV>*>(this);

		ATLASSERT(ppvObject != NULL);
		if (ppvObject == NULL)
			return E_POINTER;

		if (InlineIsEqualGUID(riid, IID_IConnectionPoint) || InlineIsEqualUnknown(riid))
		{
			*ppvObject = this;
			pThis->AddRef();
			return S_OK;
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	STDMETHOD(GetConnectionInterface)(IID *piid2)
	{
		if (piid2 == NULL)
			return E_POINTER;
		*piid2 = *piid;
		return S_OK;
	}

	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer **ppCPC)
	{
		T									*pThis;

		pThis = static_cast<T *>(this);
		return pThis->QueryInterface(IID_IConnectionPointContainer, reinterpret_cast<void **>(ppCPC));
	}

	STDMETHOD(Advise)(IUnknown *pUnkSink, DWORD *pdwCookie)
	{
		IUnknown							*adviseTarget;
		IID									interfaceID;
		HRESULT								hResult;

		if (pdwCookie != NULL)
			*pdwCookie = 0;
		if (pUnkSink == NULL || pdwCookie == NULL)
			return E_POINTER;
		GetConnectionInterface(&interfaceID);			// can't fail
		hResult = pUnkSink->QueryInterface(interfaceID, reinterpret_cast<void **>(&adviseTarget));
		if (SUCCEEDED(hResult))
		{
			*pdwCookie = m_vec.Add(adviseTarget);
			if (*pdwCookie != 0)
				hResult = S_OK;
			else
			{
				adviseTarget->Release();
				hResult = CONNECT_E_ADVISELIMIT;
			}
		}
		else if (hResult == E_NOINTERFACE)
			hResult = CONNECT_E_CANNOTCONNECT;
		return hResult;
	}

	STDMETHOD(Unadvise)(DWORD dwCookie)
	{
		IUnknown							*adviseTarget;
		HRESULT								hResult;

		adviseTarget = m_vec.GetUnknown(dwCookie);
		if (m_vec.Remove(dwCookie))
		{
			if (adviseTarget != NULL)
				adviseTarget->Release();
			hResult = S_OK;
		}
		else
			hResult = CONNECT_E_NOCONNECTION;
		return hResult;
	}

	STDMETHOD(EnumConnections)(IEnumConnections **ppEnum)
	{
		CComObject<CComEnumConnections>		*newEnumerator;
		CONNECTDATA							*itemBuffer;
		CONNECTDATA							*itemBufferEnd;
		IUnknown							**x;
		HRESULT								hResult;

		ATLASSERT(ppEnum != NULL);
		if (ppEnum == NULL)
			return E_POINTER;
		*ppEnum = NULL;

		ATLTRY(itemBuffer = new CONNECTDATA[m_vec.end() - m_vec.begin()])
		if (itemBuffer == NULL)
			return E_OUTOFMEMORY;
		itemBufferEnd = itemBuffer;
		for (x = m_vec.begin(); x < m_vec.end(); x++)
		{
			if (*x != NULL)
			{
				(*x)->AddRef();
				itemBufferEnd->pUnk = *x;
				itemBufferEnd->dwCookie = m_vec.GetCookie(x);
				itemBufferEnd++;
			}
		}
		ATLTRY(newEnumerator = new CComObject<CComEnumConnections>)
		if (newEnumerator == NULL)
			return E_OUTOFMEMORY;
		newEnumerator->Init(itemBuffer, itemBufferEnd, NULL, AtlFlagTakeOwnership);		// can't fail
		hResult = newEnumerator->_InternalQueryInterface(IID_IEnumConnections, (void **)ppEnum);
		if (FAILED(hResult))
			delete newEnumerator;
		return hResult;
	}
};

template <class T>
class IConnectionPointContainerImpl : public IConnectionPointContainer
{
		typedef const _ATL_CONNMAP_ENTRY * (*handlerFunctionType)(int *);
		typedef CComEnum<IEnumConnectionPoints, &IID_IEnumConnectionPoints, IConnectionPoint *, _CopyInterface<IConnectionPoint> >
						CComEnumConnectionPoints;

public:
	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum)
	{
		const _ATL_CONNMAP_ENTRY			*entryPtr;
		int									connectionPointCount;
		IConnectionPoint					**itemBuffer;
		int									destIndex;
		handlerFunctionType					handlerFunction;
		CComEnumConnectionPoints			*newEnumerator;
		HRESULT								hResult;

		ATLASSERT(ppEnum != NULL);
		if (ppEnum == NULL)
			return E_POINTER;
		*ppEnum = NULL;

		entryPtr = T::GetConnMap(&connectionPointCount);
		ATLTRY(itemBuffer = new IConnectionPoint * [connectionPointCount])
		if (itemBuffer == NULL)
			return E_OUTOFMEMORY;

		destIndex = 0;
		while (entryPtr->dwOffset != static_cast<DWORD_PTR>(-1))
		{
			if (entryPtr->dwOffset == static_cast<DWORD_PTR>(-2))
			{
				entryPtr++;
				handlerFunction = reinterpret_cast<handlerFunctionType>(entryPtr->dwOffset);
				entryPtr = handlerFunction(NULL);
			}
			else
			{
				itemBuffer[destIndex++] = reinterpret_cast<IConnectionPoint *>((char *)this + entryPtr->dwOffset);
				entryPtr++;
			}
		}

		ATLTRY(newEnumerator = new CComObject<CComEnumConnectionPoints>)
		if (newEnumerator == NULL)
		{
			delete [] itemBuffer;
			return E_OUTOFMEMORY;
		}

		newEnumerator->Init(&itemBuffer[0], &itemBuffer[destIndex], NULL, AtlFlagTakeOwnership);	// can't fail
		hResult = newEnumerator->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum);
		if (FAILED(hResult))
			delete newEnumerator;
		return hResult;
	}

	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP)
	{
		IID									interfaceID;
		const _ATL_CONNMAP_ENTRY			*entryPtr;
		handlerFunctionType					handlerFunction;
		IConnectionPoint					*connectionPoint;
		HRESULT								hResult;

		if (ppCP == NULL)
			return E_POINTER;
		*ppCP = NULL;
		hResult = CONNECT_E_NOCONNECTION;
		entryPtr = T::GetConnMap(NULL);
		while (entryPtr->dwOffset != static_cast<DWORD_PTR>(-1))
		{
			if (entryPtr->dwOffset == static_cast<DWORD_PTR>(-2))
			{
				entryPtr++;
				handlerFunction = reinterpret_cast<handlerFunctionType>(entryPtr->dwOffset);
				entryPtr = handlerFunction(NULL);
			}
			else
			{
				connectionPoint = reinterpret_cast<IConnectionPoint *>(reinterpret_cast<char *>(this) + entryPtr->dwOffset);
				if (SUCCEEDED(connectionPoint->GetConnectionInterface(&interfaceID)) && InlineIsEqualGUID(riid, interfaceID))
				{
					*ppCP = connectionPoint;
					connectionPoint->AddRef();
					hResult = S_OK;
					break;
				}
				entryPtr++;
			}
		}
		return hResult;
	}
};

#define BEGIN_CONNECTION_POINT_MAP(x)											\
	typedef x _atl_conn_classtype;												\
	static const ATL::_ATL_CONNMAP_ENTRY *GetConnMap(int *pnEntries) {			\
	static const ATL::_ATL_CONNMAP_ENTRY _entries[] = {

#define END_CONNECTION_POINT_MAP()												\
	{(DWORD_PTR)-1} };															\
	if (pnEntries)																\
		*pnEntries = sizeof(_entries) / sizeof(ATL::_ATL_CONNMAP_ENTRY) - 1;	\
	return _entries;}

#define CONNECTION_POINT_ENTRY(iid)												\
	{offsetofclass(ATL::_ICPLocator<&iid>, _atl_conn_classtype) -				\
	offsetofclass(ATL::IConnectionPointContainerImpl<_atl_conn_classtype>, _atl_conn_classtype)},

}; // namespace ATL
