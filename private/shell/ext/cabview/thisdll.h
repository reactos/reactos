//*******************************************************************************************
//
// Filename : ThisDll.h
//	
//				Generic OLE header file 
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************



#ifndef _THISDLL_H_
#define _THISDLL_H_



class CWaitCursor
{
public:
	CWaitCursor() {m_cOld=SetCursor(LoadCursor(NULL, IDC_WAIT));}
	~CWaitCursor() {SetCursor(m_cOld);}

private:
	HCURSOR m_cOld;
} ;

class CRefCount
{
public:
	CRefCount() : m_cRef(0) {};

	UINT AddRef()  {return(++m_cRef);}
	UINT Release() {return(--m_cRef);}
	UINT GetRef()  {return(  m_cRef);}

private:
	UINT m_cRef;

} ;

class CThisDll
{
public:
	CThisDll() {
        m_hInst=NULL;
	}
    // Make no destructor for global classes (requires CRT stuff)

	void SetInstance(HINSTANCE hInst) {m_hInst=hInst;}
	HINSTANCE GetInstance() {return(m_hInst);}

	CRefCount m_cRef;
	CRefCount m_cLock;

private:
	HINSTANCE	m_hInst;
} ;

extern CThisDll g_ThisDll;

class CRefDll
{
public:
	CRefDll()  {g_ThisDll.m_cRef.AddRef ();}
	~CRefDll() {g_ThisDll.m_cRef.Release();}
} ;

extern HRESULT CabFolder_CreateInstance(REFIID riid, LPVOID *ppvObj);
extern HRESULT CabViewDataObject_CreateInstance(REFIID riid, LPVOID *ppvObj);

class CSafeMalloc
{
public:
    CSafeMalloc()
    {
        if (!SUCCEEDED(SHGetMalloc(&m_ShIMalloc)))
        {
            m_ShIMalloc = NULL;
        }
    }
    ~CSafeMalloc() {if (m_ShIMalloc) m_ShIMalloc->Release();}

    LPVOID Alloc(UINT cb) {return(m_ShIMalloc ? m_ShIMalloc->Alloc(cb) : NULL);}
    void Free(LPVOID pv) {if (m_ShIMalloc) m_ShIMalloc->Free(pv);}

private:
    IMalloc* m_ShIMalloc;
} ;

#endif	// _THISDLL_H_
