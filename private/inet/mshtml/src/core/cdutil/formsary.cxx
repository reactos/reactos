//+------------------------------------------------------------------------
//
//  File:   formsary.cxx
//
//  Contents:   Generic dynamic array class
//
//  Classes:    CImplAry
//
//-------------------------------------------------------------------------

#include <headers.hxx>

DeclareTag(CImplAryLock, "CImplAry", "Detect data array changing when lock is on.");

#define CFORMSARY_MAXELEMSIZE    128

MtDefine(CURLAry, Utilities, "CURLAry")
MtDefine(CURLAry_pv, CURLAry, "CURLAry::_pv")
MtDefine(CIPrintAry, Printing, "CIPrintAry")
MtDefine(CIPrintAry_pv, CIPrintAry, "CIPrintAry::_pv")
MtDefine(CPrintInfoFlagsAry, Printing, "CPrintInfoFlagsAry")
MtDefine(CPrintInfoFlagsAry_pv, CPrintInfoFlagsAry, "CPrintInfoFlagsAry::_pv")

//  CImplAry class

//
//  NOTE that this file does not include support for artificial
//    error simulation.  There are common usage patterns for arrays
//    which break our normal assumptions about errors.  For instance,
//    ary.EnsureSize() followed by ary.Append(); code which makes
//    this sequence of calls expects ary.Append() to always succeed.
//
//    Because of this, the Ary methods do not use THR internally.
//    Instead, the code which is calling Ary is expected to follow
//    the normal THR rules and use THR() around any call to an
//    Ary method which could conceivably fail.
//
//    This relies on the Ary methods having solid internal error
//    handling, since the error handling within will not be exercised
//    by the normal artifical failure code.
//

//+------------------------------------------------------------------------
//
//  Member: CImplAry::~CImplAry
//
//  Synopsis:   Resizeable array destructor. Frees storage allocated for the
//      array.
//
//-------------------------------------------------------------------------
CImplAry::~CImplAry( )
{
    if (!UsingStackArray())
    {
        MemFree(PData());
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::GetAlloced, public
//
//  Synopsis:   Returns the number of bytes that have been allocated.
//
//  Arguments:  [cb] -- Size of each element
//
//  Notes:      For the CStackAry classes the value returned is _cStack*cb if
//              we're still using the stack-allocated array.
//
//----------------------------------------------------------------------------

ULONG
CImplAry::GetAlloced(size_t cb)
{
    if (UsingStackArray())
    {
        return GetStackSize() * cb;
    }
    else
    {
        return MemGetSize(PData());
    }
}

//+------------------------------------------------------------------------
//
//  Member: CImplAry::EnsureSize
//
//  Synopsis:   Ensures that the array is at least the given size. That is,
//      if EnsureSize(c) succeeds, then (c-1) is a valid index. Note
//      that the array maintains a separate count of the number of
//      elements logically in the array, which is obtained with the
//      Size/SetSize methods. The logical size of the array is never
//      larger than the allocated size of the array.
//
//  Arguments:  cb    Element size
//              c     New allocated size for the array.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CImplAry::EnsureSize ( size_t cb, long c )
{
    HRESULT  hr = S_OK;
    unsigned long cbAlloc;

    if (UsingStackArray() && (long)(c * cb) <= (long)GetAlloced(cb))
        goto Cleanup;

    Assert( c >= 0 );

    cbAlloc = ((c < 8) ? c : ((c + 7) & ~7)) * cb;
    
    if (UsingStackArray() ||
        (((unsigned long) c > ((_c < 8) ? _c : ((_c + 7) & ~7))) && cbAlloc > MemGetSize(PData())))
    {
        Assert(!(_fCheckLock && IsTagEnabled(CImplAryLock)) && "CDataAry changing while CImplAryLock is on");

        if (UsingStackArray())
        {
            //
            // We have to switch from the stack-based array to an allocated
            // one, so allocate the memory and copy the data over.
            //
            
            void * pbDataOld = PData();
            int    cbOld     = GetAlloced( cb );

            PData() = MemAlloc( _mt, cbAlloc );
            
            if (!PData())
            {
                PData() = pbDataOld;

                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            memcpy( PData(), pbDataOld, cbOld );
        }
        else
        {
            hr = MemRealloc( _mt, (void **) & PData(), cbAlloc );
            
            if (hr)
                goto Cleanup;
        }

        _fDontFree = FALSE;

        MemSetName((PData(), "CImplAry data (%d elements)", c));
    }

Cleanup:
    
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::Grow, public
//
//  Synopsis:   Ensures enough memory is allocated for c elements and then
//              sets the size of the array to that much.
//
//  Arguments:  [cb] -- Element Size
//              [c]  -- Number of elements to grow array to.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CImplAry::Grow(size_t cb, int c)
{
    HRESULT hr = EnsureSize(cb, c);
    if (!hr)
    {
        SetSize(c);
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CImplAry::AppendIndirect
//
//  Synopsis:   Appends the given element to the end of the array,
//              incrementing the array's logical size, and growing the
//              array's allocated size if necessary.  Note that the element
//              is passed with a pointer, rather than directly.
//
//  Arguments:  cb        Element size
//              pv        Pointer to the element to be appended
//              ppvPlaced Pointer to the element that's inside the array
//
//  Returns:    HRESULT
//
//  Notes:      If pv is NULL, the element is appended and initialized to
//              zero.
//
//-------------------------------------------------------------------------
HRESULT
CImplAry::AppendIndirect(size_t cb, void * pv, void ** ppvPlaced)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        RRETURN(hr);

    if (ppvPlaced)
    {
        *ppvPlaced = Deref(cb, _c);
    }

    if (!pv)
    {
        memset(Deref(cb, _c), 0, cb);
    }
    else
    {
        memcpy(Deref(cb, _c), pv, cb);
    }

    _c++;

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member: CImplAry::Delete
//
//  Synopsis:   Removes the i'th element of the array, shuffling all
//              elements that follow one slot towards the beginning of the
//              array.
//
//  Arguments:  cb  Element size
//              i   Element to delete
//
//-------------------------------------------------------------------------
void
CImplAry::Delete(size_t cb, int i)
{
    Assert(i >= 0);
    Assert(i < (int)_c);

    Assert(!(_fCheckLock && IsTagEnabled(CImplAryLock)) && "CDataAry changing while CImplAryLock is on");

    memmove(((BYTE *) PData()) + (i * cb),
            ((BYTE *) PData()) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    _c--;
}

//+------------------------------------------------------------------------
//
//  Member: CImplAry::DeleteByValueIndirect
//
//  Synopsis:   Removes the element matching the given value.
//
//  Arguments:  cb  Element size
//              pv  Element to delete
//
//  Returuns:   True if found & deleted.
//
//-------------------------------------------------------------------------
BOOL
CImplAry::DeleteByValueIndirect(size_t cb, void *pv)
{
    int i = FindIndirect(cb, pv);
    if (i >= 0)
    {
        Delete(cb, i);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//+------------------------------------------------------------------------
//
//  Member: CImplAry::DeleteMultiple
//
//  Synopsis:   Removes a range of elements of the array, shuffling all
//              elements that follow the last element being deleted slot
//              towards the beginning of the array.
//
//  Arguments:  cb    Element size
//              start First element to delete
//              end   Last element to delete
//
//-------------------------------------------------------------------------
void
CImplAry::DeleteMultiple(size_t cb, int start, int end)
{
    Assert((start >= 0) && (end >= 0));
    Assert((start < (int)_c) && (end < (int)_c));
    Assert(end >= start);

    if ((unsigned)end < (_c - 1))
    {
        memmove(((BYTE *) PData()) + (start * cb),
                ((BYTE *) PData()) + ((end + 1) * cb),
                (_c - end - 1) * cb);
    }

    _c -= (end - start) + 1;
}

//+------------------------------------------------------------------------
//
//  Member: CImplAry::DeleteAll
//
//  Synopsis:   Efficient method for emptying array of any contents
//
//-------------------------------------------------------------------------
void
CImplAry::DeleteAll(void)
{
    Assert(!(_fCheckLock && IsTagEnabled(CImplAryLock)) && "CDataAry changing while CImplAryLock is on");

    if (!UsingStackArray())
    {
        MemFree(PData());

        if (_fStack)
        {
            PData() = GetStackPtr();
            _fDontFree = TRUE;
        }
        else
        {
            PData() = NULL;
        }
    }

    _c = 0;
}


//+------------------------------------------------------------------------
//
//  Member: CImplAry::InsertIndirect
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//      index i, and all elements that follow it, are shuffled one
//      slot away towards the end of the array.Note that the
//      clement is passed with a pointer, rather than directly.
//
//  Arguments:  cb    Element size
//              i     Index to insert...
//              pv        ...this pointer at
//
//              if pv is NULL then the element is initialized to all zero.
//
//-------------------------------------------------------------------------
HRESULT
CImplAry::InsertIndirect(size_t cb, int i, void *pv)
{
    HRESULT hr;

    hr = EnsureSize(cb, _c + 1);
    if (hr)
        RRETURN(hr);

    memmove(((BYTE *) PData()) + ((i + 1) * cb),
            ((BYTE *) PData()) + (i * cb),
            (_c - i ) * cb);

    if (!pv)
    {
        memset(Deref(cb, i), 0, cb);
    }
    else
    {
        memcpy(Deref(cb, i), pv, cb);
    }
    _c++;
    return NOERROR;

}

#ifdef NEVER
//+------------------------------------------------------------------------
//
//  Member: CImplAry::BringToFront
//
//  Synopsis:   Moves the i'th element to the front of the array, shuffling
//              intervening elements to make room.
//
//  Arguments:  i
//
//-------------------------------------------------------------------------
void
CImplAry::BringToFront(size_t cb, int i)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    Assert(cb <= CFORMSARY_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) PData()) + (i * cb), cb);
    memmove(((BYTE *) PData()) + cb, PData(), i * cb);
    memcpy(PData(), rgb, cb);
}



//+------------------------------------------------------------------------
//
//  Member: CImplAry::SendToBack
//
//  Synopsis:   Moves the i'th element to the back of the array (that is,
//      the largest index less than the logical size.) Any intervening
//      elements are shuffled out of the way.
//
//  Arguments:  i
//
//-------------------------------------------------------------------------
void
CImplAry::SendToBack(size_t cb, int i)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    Assert(cb <= CFORMSARY_MAXELEMSIZE);

    memcpy(rgb, ((BYTE *) PData()) + (i * cb), cb);
    memmove(((BYTE *) PData()) + (i * cb),
            ((BYTE *) PData()) + ((i + 1) * cb),
            (_c - i - 1) * cb);

    memcpy(((BYTE *) PData()) + ((_c - 1) * cb), rgb, cb);
}


//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::Swap
//
//  Synopsis:   swap two members of array with each other.
//
//  Arguments:  cb  size of elements
//              i1  1st element
//              i2  2nd element
//----------------------------------------------------------------------------
void
CImplAry::Swap(size_t cb, int i1, int i2)
{
    BYTE    rgb[CFORMSARY_MAXELEMSIZE];

    Assert(cb <= CFORMSARY_MAXELEMSIZE);

    if ((unsigned)i1 >= _c)
        i1 = _c - 1;
    if ((unsigned)i2 >= _c)
        i2 = _c - 1;

    if (i1 != i2)
    {
        memcpy(rgb, ((BYTE *) PData()) + (i1 * cb), cb);
        memcpy(((BYTE *) PData()) + (i1 * cb), ((BYTE *) PData()) + (i2 * cb), cb);
        memcpy(((BYTE *) PData()) + (i2 * cb), rgb, cb);
    }
}
#endif // NEVER

//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::FindIndirect
//
//  Synopsis:   Finds an element of a non-pointer array.
//
//  Arguments:  cb  The size of the element.
//              pv  Pointer to the element.
//
//  Returns:    The index of the element if found, otherwise -1.
//
//----------------------------------------------------------------------------

int
CImplAry::FindIndirect(size_t cb, void * pv)
{
    int     i;
    void *  pvT;

    pvT = PData();
    for (i = _c; i > 0; i--)
    {
        if (!memcmp(pv, pvT, cb))
            return _c - i;

        pvT = (char *) pvT + cb;
    }

    return -1;
}



//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::CopyAppend
//
//  Synopsis:   Copies the entire contents of another CImplAry object and
//              appends it to the end of the array.
//
//  Arguments:  ary     Object to copy.
//              fAddRef Addref the elements on copy?
//
//----------------------------------------------------------------------------

#ifdef NEVER
HRESULT
CImplAry::CopyAppend(size_t cb, const CImplAry& ary, BOOL fAddRef)
{
    RRETURN(CopyAppendIndirect(cb, ary._c, ((CImplAry *)&ary)->PData(), fAddRef));
}


HRESULT
CImplAry::CopyAppendIndirect(size_t cb, int c, void * pv, BOOL fAddRef)
{
    IUnknown ** ppUnk;                  // elem to addref

    if (EnsureSize(cb, _c + c))
        RRETURN(E_OUTOFMEMORY);

    if (pv)
    {
        memcpy((BYTE*) PData() + (_c * cb), pv, c * cb);
    }

    _c += c;

    if (fAddRef)
    {
        for (ppUnk = (IUnknown **) pv; c > 0; c--, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
}
#endif // NEVER

//+---------------------------------------------------------------------------
//
//  Member:     CImplAry::Copy
//
//  Synopsis:   Creates a copy from another CImplAry object.
//
//  Arguments:  ary     Object to copy.
//              fAddRef Addref the elements on copy?
//
//----------------------------------------------------------------------------

HRESULT
CImplAry::Copy(size_t cb, const CImplAry& ary, BOOL fAddRef)
{
    RRETURN(CopyIndirect(cb, ary._c, ((CImplAry *)&ary)->PData(), fAddRef));
}



//+------------------------------------------------------------------------
//
//  Member:     CImplAry::CopyIndirect
//
//  Synopsis:   Fills a forms array from a C-style array of raw data
//
//  Arguments:  [cb]
//              [c]
//              [pv]
//              [fAddRef]
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CImplAry::CopyIndirect(size_t cb, int c, void * pv, BOOL fAddRef)
{
    IUnknown **     ppUnk;

    if (pv == PData())
        return S_OK;

    DeleteAll();
    if (pv)
    {
        if (EnsureSize(cb, c))
            RRETURN(E_OUTOFMEMORY);

        memcpy(PData(), pv, c * cb);
    }

    _c = c;

    if (fAddRef)
    {
        for (ppUnk = (IUnknown **) PData(); c > 0; c--, ppUnk++)
        {
            (*ppUnk)->AddRef();
        }
    }

    return S_OK;
}


HRESULT
CImplPtrAry::ClearAndReset()
{
    //  BUGBUG why does this function reallocate memory, rather than
    //    just memset'ing to 0? (chrisz)

    // BUGBUG -- Do not use this method! Use DeleteAll to clear the array.
    Assert(!PData());

    PData() = NULL;
    HRESULT hr = EnsureSize(_c);
    _c = 0;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CImplPtrAry::*
//
//  Synopsis:   CImplPtrAry elements are always of size four.
//              The following functions encode this knowledge.
//
//-------------------------------------------------------------------------

HRESULT
CImplPtrAry::EnsureSize(long c)
{
    return CImplAry::EnsureSize(sizeof(void *), c);
}

HRESULT
CImplPtrAry::Grow(int c)
{
    return CImplAry::Grow(sizeof(void *), c);
}

HRESULT
CImplPtrAry::Append(void * pv)
{
    return CImplAry::AppendIndirect(sizeof(void *), &pv);
}

HRESULT
CImplPtrAry::Insert(int i, void * pv)
{
    return CImplAry::InsertIndirect(sizeof(void *), i, &pv);
}

int
CImplPtrAry::Find(void * pv)
{
    int     i;
    void ** ppv;

    for (i = 0, ppv = (void **) PData(); (unsigned)i < _c; i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }

    return -1;
}


void
CImplPtrAry::Delete(int i)
{
    CImplAry::Delete(sizeof(void *), i);
}

BOOL
CImplPtrAry::DeleteByValue(void *pv)
{
    int i = Find(pv);
    if (i >= 0)
    {
        CImplAry::Delete(sizeof(void *), i);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
CImplPtrAry::DeleteMultiple(int start, int end)
{
    CImplAry::DeleteMultiple(sizeof(void*), start, end);
}

void
CImplPtrAry::ReleaseAndDelete(int idx)
{
    IUnknown * pUnk;

    Assert(idx < (int)_c);

    // grab element at idx
    pUnk = ((IUnknown **) PData())[idx];

#if defined(UNIX) && defined(ux10)
    if (pUnk)
        (pUnk)->Release();

    Delete(idx);
#else
    Delete(idx);

    if (pUnk)
        (pUnk)->Release();
#endif
}


void
CImplPtrAry::ReleaseAll(void)
{
    int         i;
    IUnknown ** ppUnk;

    for (i = 0, ppUnk = (IUnknown **) PData(); (unsigned)i < _c; i++, ppUnk++)
    {
        if (*ppUnk)
            (*ppUnk)->Release();
    }

    DeleteAll();
}

#ifdef NEVER
void
CImplPtrAry::BringToFront(int i)
{
    CImplAry::BringToFront(sizeof(void *), i);
}


void
CImplPtrAry::SendToBack(int i)
{
    CImplAry::SendToBack(sizeof(void *), i);
}

void
CImplPtrAry::Swap(int i1, int i2)
{
    CImplAry::Swap(sizeof(void *), i1, i2);
}


HRESULT
CImplPtrAry::CopyAppendIndirect(int c, void * pv, BOOL fAddRef)
{
    return CImplAry::CopyAppendIndirect(sizeof(void *), c, pv, fAddRef);
}

HRESULT
CImplPtrAry::CopyAppend(const CImplAry& ary, BOOL fAddRef)
{
    return CImplAry::CopyAppend(sizeof(void *), ary, fAddRef);
}
#endif // NEVER

HRESULT
CImplPtrAry::CopyIndirect(int c, void * pv, BOOL fAddRef)
{
    return CImplAry::CopyIndirect(sizeof(void *), c, pv, fAddRef);
}

HRESULT
CImplPtrAry::Copy(const CImplAry& ary, BOOL fAddRef)
{
    return CImplAry::Copy(sizeof(void *), ary, fAddRef);
}


HRESULT
CImplPtrAry::EnumElements(
        REFIID iid,
        void ** ppv,
        BOOL fAddRef,
        BOOL fCopy,
        BOOL fDelete)
{
    return CImplAry::EnumElements(
            sizeof(void *),
            iid,
            ppv,
            fAddRef,
            fCopy,
            fDelete);
}


HRESULT
CImplPtrAry::EnumVARIANT(
        VARTYPE vt,
        IEnumVARIANT ** ppenum,
        BOOL fCopy,
        BOOL fDelete)
{
    return CImplAry::EnumVARIANT(
            sizeof(void *),
            vt,
            ppenum,
            fCopy,
            fDelete);
}


CStackCStrAry::~CStackCStrAry()
{
    CStr *pcstr = (CStr*) PData();
    int iSize = Size();

    while (iSize--) 
    {
        pcstr->Free();
        pcstr++;
    }
}


CStackIPrintAry::~CStackIPrintAry()
{
    IPrint **ppPrint = (IPrint**) PData();
    int iSize = Size();

    while (iSize--) 
    {
        ClearInterface(ppPrint);
        ppPrint++;
    }
}

