////////////////////////////////////////////////////////////////////
// PIDL.cpp: implementation of the CPIDL class.
//
// By Oz Solomonovich (osolo@bigfoot.com)

#include "stdafx.h"
#include <atlbase.h>
#include "PIDL.h"

LPSHELLFOLDER CPIDL::m_sfDesktop  = NULL; // cached destkop folder
LPMALLOC      CPIDL::m_pAllocator = NULL; // cached system allocator

CPIDL::pidl_initializer CPIDL::m_initializer; // automatic init. obj


////////////////////////////////////////////////////////////////////
// Initialization object
////////////////////////////////////////////////////////////////////

// pidl_initializer is used to initialize the static data.  The 
// constructor and destructor are automatically called for us when
// the program starts/ends.

CPIDL::pidl_initializer::pidl_initializer()
{
    SHGetDesktopFolder(&m_sfDesktop); // cache d'top folder obj ptr 
    SHGetMalloc(&m_pAllocator);       // cache sys allocator obj ptr
}

CPIDL::pidl_initializer::~pidl_initializer()
{
    m_sfDesktop->Release();
    m_pAllocator->Release();
}


////////////////////////////////////////////////////////////////////
// CPIDL Implementation
////////////////////////////////////////////////////////////////////
CPIDL::CPIDL(LPCTSTR szPath, LPSHELLFOLDER psf) : 
     m_pidl(NULL) 
{ 
	Set(szPath, psf); 
}


CPIDL::~CPIDL()
{
    Free();  // just free used memory
}


////////////////////////////////////////////////////////////////////
// Assignment Functions

HRESULT CPIDL::Set(const CPIDL& cpidl)
{
    return MakeCopyOf(cpidl.m_pidl);
}

HRESULT CPIDL::Set(LPITEMIDLIST pidl)
{
    Free();
    m_pidl = pidl;
    return ERROR_SUCCESS;
}

HRESULT CPIDL::Set(LPCTSTR szPath, LPSHELLFOLDER psf)
{
    OLECHAR olePath[MAX_PATH];
    ULONG   chEaten;
    ULONG   dwAttributes;
    
    Free();
#ifndef _UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, -1, 
        olePath, MAX_PATH);
#else
	lstrcpy(olePath,szPath);
#endif
    return psf->ParseDisplayName(NULL, NULL, olePath, &chEaten, 
        &m_pidl, &dwAttributes);
}

HRESULT CPIDL::MakeCopyOf(LPITEMIDLIST pidl)
{
    Free();
    if (pidl) {
        UINT sz = m_pAllocator->GetSize((LPVOID)pidl);
        AllocMem(sz);
        CopyMemory((LPVOID)m_pidl, (LPVOID)pidl, sz);
    }
    return ERROR_SUCCESS;
}

HRESULT CPIDL::MakeAbsPIDLOf(LPSHELLFOLDER psf, LPITEMIDLIST pidl)
{
USES_CONVERSION;
    STRRET  str;
    HRESULT hr = psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str);
    if (SUCCEEDED(hr)) {
        ExtractCStr(str);
        hr = Set(A2T(str.cStr));
    }
    return hr;
}


////////////////////////////////////////////////////////////////////
// CPIDL Operations

void CPIDL::Free()
{
    if (m_pidl) {
        m_pAllocator->Free(m_pidl);
        m_pidl = NULL;
    }
}

#define CB_SIZE  (sizeof(piid->cb))  // size of termination item

UINT CPIDL::GetSize() const
{
    UINT        cbTotal = 0;
    LPSHITEMID  piid    = GetFirstItemID();
    
    if (piid) {
        do {
            cbTotal += piid->cb;
            GetNextItemID(piid);
        } while (piid->cb);
        cbTotal += CB_SIZE; // count the terminator
    }
    
    return cbTotal;
}

void CPIDL::Split(CPIDL& parent, CPIDL& obj) const
{
    int         size = 0;
    SHITEMID    *piid, *piidLast;
    
    // find last item-id and calculate total size of pidl
    piid = piidLast = &m_pidl->mkid;
    while (piid->cb)
    {
        piidLast = piid;
        size += (piid->cb);
        piid =  (SHITEMID *)((LPBYTE)piid + (piid->cb));
    }
    
    // copy parent folder portion
    size -= piidLast->cb;  // don't count "object" item-id
	if (size > 0)
	{
	    parent.AllocMem(size + CB_SIZE);
		CopyMemory(parent.m_pidl, m_pidl, size);
		ZeroMemory((LPBYTE)parent.m_pidl + size, CB_SIZE); // terminator
    }
    // copy "object" portion
    size = piidLast->cb + CB_SIZE;
	if (size > 0)
	{
	    obj.AllocMem(size);
		CopyMemory(obj.m_pidl, piidLast, size);
	}
}

CPIDL CPIDL::operator + (CPIDL& pidl) const
{
    CPIDL ret;
    Concat(*this, pidl, ret);
    return ret;
}

void CPIDL::Concat(const CPIDL &a, const CPIDL& b, CPIDL& result)
{
    result.Free();
    
    // both empty->empty | a empty->return b | b empty->return a
    if (a.m_pidl == NULL  &&  b.m_pidl == NULL) return;
    if (a.m_pidl == NULL) { result.Set(b); return; }
    if (a.m_pidl == NULL) { result.Set(a); return; }
    
    UINT cb1 = a.GetSize() - sizeof(a.m_pidl->mkid.cb);
    UINT cb2 = b.GetSize(); 
    result.AllocMem(cb1 + cb2); // allocate enough memory 
    CopyMemory(result.m_pidl, a.m_pidl, cb1);                 // 1st
    CopyMemory(((LPBYTE)result.m_pidl) + cb1, b.m_pidl, cb2); // 2nd
}

HRESULT CPIDL::GetUIObjectOf(REFIID riid, LPVOID *ppvOut, 
    HWND hWnd /*= NULL*/, LPSHELLFOLDER psf /*= m_sfDesktop*/)
{
    CPIDL           parent, obj;
    LPSHELLFOLDER   psfParent;
	HRESULT			hr=S_OK;
    
    Split(parent, obj);
	// if no idl the use desktop folder
	if (parent.m_pidl == NULL || parent.m_pidl->mkid.cb == 0)
	{
		psfParent = psf;
		psfParent->AddRef();
	}
	else
	{
		// otherwise get the parent
	     hr = psf->BindToObject(parent, NULL, IID_IShellFolder, 
		    (LPVOID *)&psfParent); // get the IShellFolder of the parent
	}
    if (SUCCEEDED(hr)) 
	{
        hr = psfParent->GetUIObjectOf(hWnd, 1, obj, riid, 0, ppvOut);
		psfParent->Release();
    }
    return hr;
}

void CPIDL::ExtractCStr(STRRET& strRet) const
{
    switch (strRet.uType) 
    {
        case STRRET_WSTR:
        {
            // pOleStr points to a WCHAR string - convert it to ANSI
            LPWSTR pOleStr = strRet.pOleStr;
            WideCharToMultiByte(CP_ACP, 0, pOleStr, -1,
                strRet.cStr, MAX_PATH, NULL, NULL);
            m_pAllocator->Free(pOleStr);
            break;
        }
        
        case STRRET_OFFSET:
            // The string lives inside the pidl, so copy it out.
            strncpy(strRet.cStr, (LPSTR)
                ((LPBYTE)m_pidl + strRet.uOffset), MAX_PATH);
            break;
    }
}


////////////////////////////////////////////////////////////////////
// CPIDL Private Operations

void CPIDL::AllocMem(int iAllocSize)
{
    Free();
    m_pidl = (LPITEMIDLIST)m_pAllocator->Alloc(iAllocSize);
}
