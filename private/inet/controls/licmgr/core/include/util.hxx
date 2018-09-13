//+----------------------------------------------------------------------------
//  File:       util.hxx
//
//  Synopsis:   This file contains utility definitions
//
//-----------------------------------------------------------------------------


#ifndef _UTIL_HXX
#define _UTIL_HXX


// Globals --------------------------------------------------------------------
extern const UCHAR SZ_NEWLINE[];
const ULONG CB_NEWLINE = (2 * sizeof(UCHAR));


// Prototypes -----------------------------------------------------------------
#ifdef _NOCRT
extern "C" size_t __cdecl _tcslen(const TCHAR *);
extern "C" int    __cdecl memcmp(const void *, const void *, size_t);
extern "C" void * __cdecl memcpy(void *, const void *, size_t);
extern "C" void * __cdecl memset(void *, int, size_t);
extern "C" void * __cdecl memmove(void *, const void *, size_t);
#endif

inline void * _cdecl operator new(size_t cb)            { return HeapAlloc(g_heap, 0, cb); }
inline void * _cdecl operator new(size_t , void * pv)   { return pv; }
inline void _cdecl operator delete(void * pv)           { HeapFree(g_heap, 0, pv); }
inline SIZE_T MemSize(void * pv)                        { return HeapSize(g_heap, 0, pv); }
inline void * MemReAlloc(void * pv, SIZE_T cb)          { return HeapReAlloc(g_heap, 0, pv, cb); }

void *  CoAlloc(ULONG cb);
void    CoFree(void * pv);
ULONG   CoGetSize(void * pv);
BOOL    CoDidAlloc(void * pv);

HRESULT CopyStream(IStream * pstmDest, IStream * pstmSrc, ULARGE_INTEGER cb,
                    ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten);


//+----------------------------------------------------------------------------
//  Class:      CAry
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> class CAry
{
public:
    CAry();
    CAry(int cItems, TYPE * pItems);
    ~CAry();

    operator TYPE *()   { return _pItems; }

    HRESULT Append(TYPE * pitem);
    void    Delete(int iItem);
    void    DeleteByValue(TYPE item);
    HRESULT EnsureSize(int cItems);
    int     Find(TYPE item);
    HRESULT Insert(int iItem, TYPE item);
    HRESULT SetSize(int cItems);
    int     Size()  { return _cItems; }

protected:
    int     _cItems;
    TYPE *  _pItems;
};

#define DEFINE_ARY(x)   typedef CAry<x> CAry##x


//+----------------------------------------------------------------------------
//  Class:      CMemoryStream
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CMemoryStream : public CComponent,
                      public IStream
{
public:
    CMemoryStream()  { _cbSize = 0; _ibPos = 0; _pbData = NULL; }
    ~CMemoryStream() { delete [] _pbData; }

    // IUnknown methods
    DEFINE_IUNKNOWN_METHODS;

    // IStream methods
    STDMETHOD(Read)(void * pv, ULONG cb, ULONG * pcbRead);
    STDMETHOD(Write)(const void * pv, ULONG cb, ULONG * pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten);
    STDMETHOD(Commit)(DWORD)                                        { return E_NOTIMPL; }
    STDMETHOD(Revert)()                                             { return E_NOTIMPL; }
    STDMETHOD(LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)    { return E_NOTIMPL; }
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)  { return E_NOTIMPL; }
    STDMETHOD(Stat)(STATSTG *, DWORD)                               { return E_NOTIMPL; }
    STDMETHOD(Clone)(IStream **)                                    { return E_NOTIMPL; }

private:
    ULONG   _cbSize;
    ULONG   _ibPos;
    BYTE *  _pbData;
};


//+----------------------------------------------------------------------------
//  Class:      CBufferedStream
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CBufferedStream : public CComponent,
                        public IStream
{
public:
    CBufferedStream(IStream * pstm, ULONG cbNewLine = 0, BOOL fRead = TRUE);
    ~CBufferedStream();

    HRESULT Flush(ULONG * pcbWritten = NULL);
    ULONG   GetTotalWritten()   { return _cbTotal; }
    HRESULT Load();
    HRESULT SetBufferSize(ULONG cb);

    // IUnknown methods
    DEFINE_IUNKNOWN_METHODS;

    // IStream methods
    STDMETHOD(Read)(void * pv, ULONG cb, ULONG * pcbRead);
    STDMETHOD(Write)(const void * pv, ULONG cb, ULONG * pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER , DWORD , ULARGE_INTEGER *)       { return E_NOTIMPL; }
    STDMETHOD(SetSize)(ULARGE_INTEGER)                              { return E_NOTIMPL; }
    STDMETHOD(CopyTo)(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *)    { return E_NOTIMPL; }
    STDMETHOD(Commit)(DWORD)                                        { return E_NOTIMPL; }
    STDMETHOD(Revert)()                                             { return E_NOTIMPL; }
    STDMETHOD(LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)    { return E_NOTIMPL; }
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)  { return E_NOTIMPL; }
    STDMETHOD(Stat)(STATSTG *, DWORD)                               { return E_NOTIMPL; }
    STDMETHOD(Clone)(IStream **)                                    { return E_NOTIMPL; }

private:
    BOOL        _fRead;
    BYTE *      _pb;
    ULONG       _cb;
    ULONG       _ib;
    ULONG       _cbLine;
    ULONG       _cbNewLine;
    ULONG       _cbTotal;
    IStream *   _pstm;
};


//+----------------------------------------------------------------------------
//  Class:      CFileStream
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
class CFileStream : public CComponent,
                    public IStream
{
    typedef CComponent parent;

public:
    CFileStream();
    ~CFileStream();

    HRESULT Init(LPCWSTR szFileName,
                 DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE,      // Access (read-write) mode
                 DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE,    // Share mode (default to exclusive)
                 LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL,          // Pointer to security descriptor
                 DWORD dwCreationDistribution = OPEN_ALWAYS,                // How to create
                 DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,        // File attributes
                 HANDLE hTemplateFile = NULL);                              // Handle of template file
    HRESULT GetFileSize(ULONG * pcbSize);

    // IUnknown methods
    DEFINE_IUNKNOWN_METHODS;

    // IStream methods
    STDMETHOD(Read)(void * pv, ULONG cb, ULONG * pcbRead);
    STDMETHOD(Write)(const void * pv, ULONG cb, ULONG * pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER libNewSize);
    STDMETHOD(CopyTo)(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten);
    STDMETHOD(Commit)(DWORD)                                        { return E_NOTIMPL; }
    STDMETHOD(Revert)()                                             { return E_NOTIMPL; }
    STDMETHOD(LockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)    { return E_NOTIMPL; }
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)  { return E_NOTIMPL; }
    STDMETHOD(Stat)(STATSTG *, DWORD)                               { return E_NOTIMPL; }
    STDMETHOD(Clone)(IStream **)                                    { return E_NOTIMPL; }

private:
    HANDLE  _hFile;

    HRESULT PrivateQueryInterface(REFIID riid, void ** ppvObj);
};


// Inlines --------------------------------------------------------------------
//+----------------------------------------------------------------------------
//  Function:   CAry<TYPE>
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE>
CAry<TYPE>::CAry()
{
    _cItems = 0;
    _pItems = NULL;
}


template <class TYPE>
CAry<TYPE>::CAry(
    int     cItems,
    TYPE *  pItems)
{
    SetSize(cItems);
    if (pItems)
    {
        ::memcpy(_pItems, pItems, (cItems)*sizeof(TYPE));
        _cItems = cItems;
    }
}


template <class TYPE>
CAry<TYPE>::~CAry()
{
    // Delete the array memory
    // (The check for NULL is necessary since the "real" pointer is one word
    //  before the address kept in _pItems)
    if (_pItems)
    {
        delete [] (((int *)_pItems)-1);
    }
}


//+----------------------------------------------------------------------------
//  Function:   Append
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> HRESULT
CAry<TYPE>::Append(
    TYPE *  pitem)
{
    HRESULT hr = EnsureSize(_cItems);
    if (!hr)
    {
        _pItems[_cItems++] = *pitem;
    }
    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   Delete
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> void
CAry<TYPE>::Delete(
    int iItem)
{
    Assert(iItem < _cItems);
    _cItems--;
    ::memmove(_pItems[iItem], _pItems[iItem+1], (_cItems-iItem)*sizeof(TYPE));
}


//+----------------------------------------------------------------------------
//  Function:   DeleteByValue
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> void
CAry<TYPE>::DeleteByValue(
    TYPE  item)
{
    int iItem = Find(item);
    if (iItem >= 0)
        Delete(iItem);
}


//+----------------------------------------------------------------------------
//  Function:   EnsureSize
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> HRESULT
CAry<TYPE>::EnsureSize(
    int cItems)
{
    int cItemsMax = (!_pItems
                        ? 0
                        : *(((int *)_pItems)-1));

    if (cItems > cItemsMax)
    {
        TYPE *  pItems = new TYPE[cItems+1];
        if (!pItems)
            return E_OUTOFMEMORY;

        if (_pItems)
        {
            ::memcpy(((int *)pItems)+1, _pItems, (cItemsMax)*sizeof(TYPE));
            delete [] (((int *)_pItems)-1);
        }
        *((int *)pItems) = cItems;
        _pItems = (TYPE *)(((int *)pItems)+1);
    }
    return S_OK;
}


//+----------------------------------------------------------------------------
//  Function:   Find
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> int
CAry<TYPE>::Find(
    TYPE item)
{
    int iItem;
    for (iItem=_cItems-1; iItem >= 0; iItem--)
    {
        if (_pItems[iItem] == item)
            break;
    }
    return iItem;
}


//+----------------------------------------------------------------------------
//  Function:   Insert
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> HRESULT
CAry<TYPE>::Insert(
    int     iItem,
    TYPE    item)
{
    HRESULT hr = EnsureSize(_cItems);
    if (!hr)
    {
        if (iItem < _cItems)
        {
            ::memmove(_pItems[iItem+1], _pItems[iItem], (_cItems-iItem)*sizeof(TYPE));
        }
        _cItems++;
        _pItems[iItem] = item;
    }
    return hr;
}


//+----------------------------------------------------------------------------
//  Function:   SetSize
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
template <class TYPE> HRESULT
CAry<TYPE>::SetSize(
    int cItems)
{
    HRESULT hr = EnsureSize(cItems);
    if (hr)
        return hr;
    _cItems = cItems;
    return S_OK;
}


#endif  // _UTIL_HXX
