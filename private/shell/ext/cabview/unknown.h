//*******************************************************************************************
//
// Filename : Unknown.h
//	
//				Definitions for some customized routines
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _UNKNOWN_H_
#define _UNKNOWN_H_

class CUnknown
{
public:
	CUnknown() {}
	virtual ~CUnknown();	// virtual destructor called from Release

	HRESULT QIHelper(REFIID riid, LPVOID *ppvObj, const IID *apiid[], LPUNKNOWN aobj[]);
	ULONG AddRefHelper();
	ULONG ReleaseHelper();

private:
	CRefCount m_cRef;
	CRefDll m_cRefDll;
} ;

// Only for ensuring Release in all control paths
class CEnsureRelease
{
public:
	CEnsureRelease(IUnknown *pUnk) : m_pUnk(pUnk) {}
	~CEnsureRelease() {if (m_pUnk) m_pUnk->Release();}

	operator IUnknown*() {return(m_pUnk);}

	void Attach(IUnknown *pUnk) {m_pUnk = pUnk;}

private:
	IUnknown *m_pUnk;
} ;

class CObjTemp
{
public:
	CObjTemp() : m_hObj(0) {}
	~CObjTemp() {}

	operator HANDLE() const {return(m_hObj);}

	HANDLE Attach(HANDLE hObjNew) {HANDLE hObj=m_hObj; m_hObj=hObjNew; return(hObj);}
	HANDLE Detach() {return(Attach(0));}

protected:
	HANDLE m_hObj;
} ;

class CMenuTemp : public CObjTemp
{
public:
	CMenuTemp() : CObjTemp() {}
	CMenuTemp(HMENU hm) : CObjTemp() {Attach(hm);}
	~CMenuTemp() {if (m_hObj) DestroyMenu(Detach());}

	operator HMENU() const {return((HMENU)m_hObj);}

	HMENU Attach(HMENU hObjNew) {return((HMENU)CObjTemp::Attach((HANDLE)hObjNew));}
	HMENU Detach() {return((HMENU)CObjTemp::Detach());}

	BOOL CreatePopupMenu() {Attach(::CreatePopupMenu()); return(m_hObj!=NULL);}
} ;

class CGotGlobal : public CObjTemp
{
public:
	CGotGlobal(HGLOBAL hData, IUnknown *pUnk) : m_erData(pUnk) {Attach(hData);}
	~CGotGlobal() {if (!(IUnknown*)m_erData) GlobalFree(m_hObj);}

private:
	CEnsureRelease m_erData;
} ;

class CHIDA : public CGotGlobal
{
public:
	CHIDA(HGLOBAL hIDA, IUnknown *pUnk) : CGotGlobal(hIDA, pUnk)
	{
		m_lpIDA = (LPIDA)GlobalLock(hIDA);
	}
	~CHIDA() {GlobalUnlock(m_hObj);}

	LPCITEMIDLIST operator [](UINT nIndex)
	{
		if (!m_lpIDA || nIndex>m_lpIDA->cidl)
		{
			return(NULL);
		}

		return((LPCITEMIDLIST)(((LPSTR)m_lpIDA)+m_lpIDA->aoffset[nIndex]));
	}

	int GetCount() {return(m_lpIDA ? m_lpIDA->cidl : 0);}

private:
	LPIDA m_lpIDA;
} ;

#endif // _UNKNOWN_H_
