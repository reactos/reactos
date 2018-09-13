//+------------------------------------------------------------------------
//
//  File:       cary.cxx
//
//  Contents:   Generic dynamic array class
//
//  Classes:    CAry
//
//-------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop

//  CAry class


//+------------------------------------------------------------------------
//
//  Member:     CAry::CAry
//
//  Synopsis:   Constructor for the generic resizeable array class. Note that
//              the array does not initially have any storage; EnsureSize
//              must be called before any index is valid (even index 0!)
//
//  Arguments:  [cb]        Size of each element.
//
//-------------------------------------------------------------------------
CAry::CAry(size_t cb)
{
    _c = _cMac = 0;
    _cb = cb;
    _pv = NULL;

    Assert(_cb <= CARY_MAXELEMSIZE);
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::~CAry
//
//  Synopsis:   Resizeable array destructor. Frees storage allocated for the
//              array.
//
//-------------------------------------------------------------------------
CAry::~CAry( )
{
    delete[] _pv;
}


//+------------------------------------------------------------------------
//
//  Member:     CAry::EnsureSize
//
//  Synopsis:   Ensures that the array is at least the given size. That is,
//              if EnsureSize(c) succeeds, then (c-1) is a valid index. Note
//              that the array maintains a separate count of the number of
//              elements logically in the array, which is obtained with the
//              Size/SetSize methods. The logical size of the array is never
//              larger than the allocated size of the array.
//
//  Arguments:  [c]         New allocated size for the array.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::EnsureSize(int c)
{
    void *  pv;

    if (c <= _cMac)
        return NOERROR;

    //  CONSIDER should we use a more sophisticated array-growing
    //    algorithm?
    c = ((c - 1) & -8) + 8;
    //pv = new (NullOnFail) BYTE[c * _cb];
    pv = new BYTE[c * _cb];
    if (!pv)
    {
        DOUT(L"o2base/CAry::EnsureSize failed\r\n");
        return E_OUTOFMEMORY;
    }

    if (_pv)
    {
        memcpy(pv, _pv, _c * _cb);
        delete[] _pv;
    }

    _cMac = c;
    _pv = pv;
    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::Deref
//
//  Synopsis:   Returns a pointer to the i'th element of the array. This
//              method is normally called by type-safe methods in derived
//              classes.
//
//  Arguments:  [i]
//
//  Returns:    void *
//
//-------------------------------------------------------------------------
void *
CAry::Deref(int i)
{
    //  BUGBUG should be inline in non-DEBUG builds
    Assert(i >= 0);
    Assert(i < _cMac);

    return ((BYTE *) _pv) + i * _cb;
};



//+------------------------------------------------------------------------
//
//  Member:     CAry::Append
//
//  Synopsis:   Appends the given pointer to the end of the array, incrementing
//              the array's logical size, and growing its allocated size if
//              necessary. This method should only be called for arrays of
//              pointers; AppendIndirect should be used for arrays of
//              non-pointers.
//
//  Arguments:  [pv]        Pointer to append.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::Append(void * pv)
{
    HRESULT     hr;

    Assert(_cb == 4);

    hr = EnsureSize(_c + 1);
    if (hr)
        return hr;

    * (void **) Deref(_c) = pv;
    _c++;

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::AppendIndirect
//
//  Synopsis:   Appends the given element to the end of the array, incrementing
//              the array's logical size, and growing the array's allocated
//              size if necessary.  Note that the element is passed with a
//              pointer, rather than directly.
//
//  Arguments:  [pv]        --  Pointer to the element to be appended
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::AppendIndirect(void * pv)
{
    HRESULT     hr;

    hr = EnsureSize(_c + 1);
    if (hr)
        return hr;

    memcpy(Deref(_c), pv, _cb);
    _c++;

    return NOERROR;
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::Delete
//
//  Synopsis:   Removes the i'th element of the array, shuffling all elements
//              that follow one slot towards the beginning of the array.
//
//  Arguments:  [i]         Element to delete
//
//-------------------------------------------------------------------------
void
CAry::Delete(int i)
{
    Assert(i >= 0);
    Assert(i < _c);

    memmove(
        ((BYTE *) _pv) + (i * _cb),
        ((BYTE *) _pv) + ((i + 1) * _cb),
        (_c - i - 1) * _cb);
    _c--;
}


//+------------------------------------------------------------------------
//
//  Member:     CAry::DeleteAll
//
//  Synopsis:   Efficient method for emptying array of any contents
//
//-------------------------------------------------------------------------
void
CAry::DeleteAll(void)
{
    delete[] _pv;
    _pv = NULL;
    _c = 0;
    _cMac = 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CAry::Insert
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//              index i, and all elements that follow it, are shuffled one
//              slot away towards the end of the array.
//              This method should only be called for arrays of
//              pointers; InsertIndirect should be used for arrays of
//              non-pointers.

//
//  Arguments:  [i]         Index to insert...
//              [pv]        ...this pointer at
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::Insert(int i, void * pv)
{
    HRESULT     hr;

    hr = EnsureSize(_c + 1);
    if (hr)
        return hr;

    memmove(
        ((BYTE *) _pv) + ((i + 1) * _cb),
        ((BYTE *) _pv) + (i * _cb),
        (_c - i ) * _cb);

    ((void **) _pv)[i] = pv;
    _c++;
    return NOERROR;
}

//+------------------------------------------------------------------------
//
//  Member:     CAry::InsertIndirect
//
//  Synopsis:   Inserts a pointer pv at index i. The element previously at
//              index i, and all elements that follow it, are shuffled one
//              slot away towards the end of the array.Note that the 
//              clement is passed with a pointer, rather than directly.
//
//  Arguments:  [i]         Index to insert...
//              [pv]        ...this pointer at
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::InsertIndirect(int i, void *pv)
{
    HRESULT     hr;

    hr = EnsureSize(_c + 1);
    if (hr)
        return hr;

    memmove(
        ((BYTE *) _pv) + ((i + 1) * _cb),
        ((BYTE *) _pv) + (i * _cb),
        (_c - i ) * _cb);

    memcpy(Deref(i), pv, _cb);
    _c++;
    return NOERROR;

}

//+------------------------------------------------------------------------
//
//  Member:     CAry::BringToFront
//
//  Synopsis:   Moves the i'th element to the front of the array, shuffling
//              intervening elements to make room.
//
//  Arguments:  [i]
//
//-------------------------------------------------------------------------
void
CAry::BringToFront(int i)
{
    BYTE    rgb[CARY_MAXELEMSIZE];

    memcpy(rgb, ((BYTE *) _pv) + (i * _cb), _cb);
    memmove(((BYTE *) _pv) + _cb, _pv, i * _cb);
    memcpy(_pv, rgb, _cb);
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::SendToBack
//
//  Synopsis:   Moves the i'th element to the back of the array (that is,
//              the largest index less than the logical size.) Any intervening
//              elements are shuffled out of the way.
//
//  Arguments:  [i]
//
//-------------------------------------------------------------------------
void
CAry::SendToBack(int i)
{
    BYTE    rgb[CARY_MAXELEMSIZE];

    memcpy(rgb, ((BYTE *) _pv) + (i * _cb), _cb);
    memmove(
        ((BYTE *) _pv) + (i * _cb),
        ((BYTE *) _pv) + ((i + 1) * _cb),
        (_c - i - 1) * _cb);
    memcpy(((BYTE *) _pv) + ((_c - 1) * _cb), rgb, _cb);
}



//+------------------------------------------------------------------------
//
//  Member:     CAry::Find
//
//  Synopsis:   Returns the index at which the given pointer is found, or -1
//              if it is not found. The pointer values are compared directly;
//              there is no compare function.
//
//  Arguments:  [pv]        Pointer to find
//
//  Returns:    int; index of pointer, or -1 if not found
//
//-------------------------------------------------------------------------
int
CAry::Find(void * pv)
{
    int     i;
    void ** ppv;

    Assert(_cb == 4);

    for (i = 0, ppv = (void **) _pv;
         i < _c;
         i++, ppv++)
    {
        if (pv == *ppv)
            return i;
    }
    return -1;
}




//+------------------------------------------------------------------------
//
//  Member:     CAry::EnumElements
//
//  Synopsis:   Creates and returns an enumerator for the elements of the
//              array. Since the type of the elements (and therefore the type
//              of the enumerator) is unknown by this class, the enumerator
//              type is passed in.
//
//  Arguments:  [iid]       Type of the enumerator
//              [ppv]       Enumerator is returned in *ppv
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CAry::EnumElements(REFIID iid, LPVOID * ppv, BOOL fAddRef)
{
    CEnumGeneric *  pEnum;

    pEnum = CEnumGeneric::Create(iid, _cb, _c, _pv, fAddRef);
    if (!pEnum)
    {
        DOUT(L"o2base/CAry::EnumElements failed\r\n");
        return E_OUTOFMEMORY;
    }

    *ppv = pEnum;
    return NOERROR;
}

