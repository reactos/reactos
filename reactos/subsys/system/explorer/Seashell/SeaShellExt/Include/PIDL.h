////////////////////////////////////////////////////////////////////
// PIDL.h: interface for the CPIDL class.
//

#ifndef __PIDL_H
#define __PIDL_H

#include <shlobj.h>

class CPIDL
{
public:
    LPITEMIDLIST  m_pidl;

// == Construction/Destruction == //

    CPIDL() : m_pidl(NULL) {}

    // Copy constructor
    CPIDL(const CPIDL& cpidl) : m_pidl(NULL) { Set(cpidl); }

    // From path (szPath relative to the folder psf) - see Set()
    CPIDL(LPCTSTR szPath, LPSHELLFOLDER psf = m_sfDesktop); 

    // From a list ptr - *doesn't* copy the actual data - see Set()
    CPIDL(LPITEMIDLIST pidl) : m_pidl(pidl) {}

    virtual ~CPIDL();


// == Assignment == //

    // Make a copy of cpidl's list data
    HRESULT Set(const CPIDL& cpidl);

    // Set by path: szPath relative to the folder psf.
    HRESULT Set(LPCTSTR szPath, LPSHELLFOLDER psf = m_sfDesktop);

    // Points the CPIDL to an existing item list: does *not* copy
    // the actual data - just the pointer (unlike MakeCopyOf()).
    HRESULT Set(LPITEMIDLIST pidl);

    // Special Assignment: Copies the data of an exisiting list.
    HRESULT MakeCopyOf(LPITEMIDLIST pidl);

    // Special Assignment: Makes a PIDL rooted at the desktop.
    HRESULT MakeAbsPIDLOf(LPSHELLFOLDER psf, LPITEMIDLIST pidl);


// == Item Access == //

    // Returns a pointer to the first item in the list
    LPSHITEMID GetFirstItemID() const { return (LPSHITEMID)m_pidl; }

    // Points to the next item in the list
    void GetNextItemID(LPSHITEMID& pid) const 
        { (LPBYTE &)pid += pid->cb; }


// == General Operations == //

    void Free();          // Frees the memory used by the item id list
    UINT GetSize() const; // Counts the actual memory in use

    // Split into direct parent and object pidls
    void Split(CPIDL& parent, CPIDL& obj) const;

    // Concatenation
    CPIDL operator + (CPIDL& pidl) const;  // using + operator
    static void Concat(const CPIDL &a, const CPIDL& b, 
        CPIDL& result);                    // result = a+b (faster)


// == Shell Name-space Access Helper Functions == //

    // 1) Won't always work: psf->GetUIObjectOf(pidl, ... )
    // 2) Will always work:  pidl.GetUIObjectOf(..., psf)
    HRESULT GetUIObjectOf(REFIID riid, LPVOID *ppvOut, 
        HWND hWnd = NULL, LPSHELLFOLDER psf = m_sfDesktop);

    // Places the STRRET string in the cStr field.  
    void ExtractCStr(STRRET& strRet) const;


// == Conversion Operators == //

    operator LPITEMIDLIST&  () { return m_pidl; }
    operator LPITEMIDLIST * () { return &m_pidl; }
    operator LPCITEMIDLIST  () const { return m_pidl; }
    operator LPCITEMIDLIST* () const 
        { return (LPCITEMIDLIST *)&m_pidl; }

protected:
    static LPSHELLFOLDER    m_sfDesktop;    // desktop object
    static LPMALLOC         m_pAllocator;   // system allocator

    // allocate memory for the pidl using the system allocator
    void AllocMem(int iAllocSize);

    // initializer (used for automatic initialization)
    static struct pidl_initializer {
        pidl_initializer();
        ~pidl_initializer();
    } m_initializer;
    friend struct pidl_initializer;
};

#endif  // __PIDL_H
