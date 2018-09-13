MtExtern(CRowArray)

//+------------------------------------------------------------------------
//
//  Class:      CRowArray (ary)
//
//  Purpose:    Subclass used for arrays of Row Handles (ULONG).
//              In this case, the element size is known to be ULONG.  
//
//-------------------------------------------------------------------------

template<class Elem>
class CRowArray
{
protected:
 
    LONG        _cCount;        // Number of entries in use.
    ULONG       _Size;          // Count of entries allocated.
    Elem *      _pav;           // Pointer to array of values.

public:
    CRowArray()     { _Size = 0, _cCount = 0; _pav = 0; }
    ~CRowArray() {MemFree (_pav);};
    HRESULT     EnsureSize(LONG c);
    LONG        Count() { return _cCount; }
    operator Elem *() { return (Elem *)PData(); }

    // If element is out of range, return a constructed NULL element
    Elem        GetElem(LONG i) {if (i>=_cCount) return Elem::Invalid();
                                  else return _pav[i]; }

    Elem        GetElemNoCheck(LONG i) {return _pav[i];}

    HRESULT     SetElem(LONG i, Elem v);
    void        SetElemNoCheck(LONG i, Elem v) {_pav[i] = v;}

    HRESULT     InsertMultiple(LONG i, int c, Elem v);
    void        DeleteMultiple(LONG i, int c);
    Elem * & PData()    { return _pav; }
    NO_COPY(CRowArray);
};


//+------------------------------------------------------------------------
//
//  Member: CRowArray::SetElem
//
//  Synopsis:   Assign value v to element at index i
//
//-------------------------------------------------------------------------
template<class Elem>
HRESULT
CRowArray<Elem>::SetElem(LONG i, Elem v)
{
    HRESULT hr;

    hr = EnsureSize(i+1);
    if (S_OK==hr)
    {
        _pav[i] = v;
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member: CRowArray::InsertMultiple
//
//  Synopsis:   Inserts c rows into an array, before the element i,
//              filling newly opened spaces with value v
//
//  Arguments:  i     index at which to insert new element (0 based!)
//                    (i.e. 0 inserts before all other elements)
//              c     count of the number of elements to insert
//              v     value to use to fill newly opened space
//
//-------------------------------------------------------------------------
template<class Elem>
HRESULT
CRowArray<Elem>::InsertMultiple(LONG i, int c, Elem v)
{
    HRESULT hr;
    LONG j;
    LONG cMove = max(_cCount-i, (LONG)0);
    
    hr = EnsureSize(max(i, _cCount) + c);   // Make sure we can add c elements
    if (hr)
        goto Error;

    // Insert by copying Last element to Last+c position, backwards
    // to avoid overwriting of course.
    memmove((BYTE *) &(_pav[i+c]), (BYTE *) &(_pav[i]), cMove * sizeof(Elem));

    // Initialize inserted elements
    for (j=i+c-1; j>=i; j--)
        _pav[j] = v;

Error:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member: CRowArray::DeleteMultiple
//
//  Synopsis:   Removes a range of elements of the array, shuffling all
//              elements that follow the last element being deleted slot
//              towards the beginning of the array.
//
//  Arguments:  i     index of first element to delete (0 based!)
//              c     count of elements to delete
//
//-------------------------------------------------------------------------
template<class Elem>
void
CRowArray<Elem>::DeleteMultiple(LONG i, int c)
{
    Assert((i >= 0) && (c >= 0));
    Assert((i < _cCount) && ((i+c-1) < _cCount));

    // We are counting on memove(x,y,0) to not fault
    // (whether or not it does anything is irrelevant).
    memmove((BYTE *) &(_pav[i]),
            (BYTE *) &(_pav[i+c]),
            (_cCount - (i+c)) * sizeof(Elem));
    _cCount -= c;
}

//+------------------------------------------------------------------------
//
//  Member: CRowArray::EnsureSize
//
//  Synopsis:   Ensures that the array is at least the given size. That is,
//      if EnsureSize(i) succeeds, then i-1 is a valid index.
//
//  Arguments:  i     1 + Highest index that should be valid
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
template<class Elem>
HRESULT
CRowArray<Elem>::EnsureSize(LONG i)
{
    HRESULT hr = S_OK;

    Assert(i >= 0);

    // Immediately test to see if we're below high water mark so common
    // case if as fast as possible.
    if (i > _cCount)
    {
        if ((ULONG)i > _Size)
        {
            if (_Size == 0)
            {
                _Size = 16;     // 64 entries: default initial size

                // double alloc unit until enough
                while ((ULONG)i > _Size) _Size = _Size << 1;
                _pav = (Elem *)MemAlloc(Mt(CRowArray), _Size*sizeof(Elem));
                if (!_pav)
                {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
            }
            else
            {
                Assert(_Size >= (ULONG)_cCount);
                // double alloc unit until enough
                while ((ULONG)i > _Size) _Size = _Size << 1;

                hr = MemRealloc(Mt(CRowArray), (void **)&_pav, _Size*sizeof(Elem));
                if (hr) goto error;
            }
        }

        // Zero entries after old high water mark, until (and including)
        // new high water mark.
        while (_cCount < i)
            _pav[_cCount++] = Elem::Invalid();
    }

error:
    RRETURN(hr);
}


class COSPData;                         // forward reference

//+----------------------------------------------------------------------------
//
//  Class CIndex
//
//  Purpose:
//      This class defines the entry for the table that is used to map an hRow
//      into an OSP pointer and Index.
//
//
class CIndex
{
    private:
        int _iRowNumber;        // Simply a row index, unless the row it refers to
                                // has been deleted, in which case its value is
                                // negative the index of the next undeleted row.
        COSPData *_pChap;   // Pointer to chapter data.  I.e. points to the
                                // OSPdata required for this chapter

    public:
        // Override null constructor to make sure the compiler doesn't make one!
        CIndex()
        {
            _iRowNumber = 0;
            _pChap = NULL;
        }

        static CIndex Invalid() {return CIndex();} // construct & return NULL element

        // Override = operator to make sure the compiler doesn't make one.
        CIndex operator = (const CIndex &CIndex)
        {
            _iRowNumber = CIndex._iRowNumber;
            _pChap = CIndex._pChap;
            return *this;            
        }

        CIndex (int i, COSPData *pChap) // need an available constructor
        {
            _iRowNumber = i;
            _pChap = pChap;
        } 

        COSPData * GetpChap()
        {
            return _pChap;
        }

        void SetpChap(COSPData *pChap)
        {
            _pChap = pChap;
        }

        // The following functions impelement the scheme for storing the deleted
        // flag by negating the RowNumber value.  Note that the knowledge of how this
        // is done should remain isolated to these functions.
        // Note that the pChap is untouched by all this.
        BOOL    FDeleted() { return (_iRowNumber < 0); }
        ULONG   Row()
                { return (int) (_iRowNumber >= 0 ? _iRowNumber : -_iRowNumber); }
        CIndex    SetRef(int i) {_iRowNumber = i; return *this;}; // Assign & return
        CIndex    SetRow(int i, BOOL DelFlag)
                {_iRowNumber = (DelFlag ? -i : i); return *this;}
        CIndex    SetRowDel(int i) {_iRowNumber = -i; return *this;}
        CIndex    SetRowNotDel(ULONG i) {_iRowNumber = i; return *this;}

};


//+----------------------------------------------------------------------------
//
//  Class ChRow
//
//  Purpose:
//      Implement a type that is used to hide the representation of hRow handles.
//      For now these are indexes into an array, but could just as well be changed
//      to return pointers into the array.  (There is no particular ordering
//      amoung ChRow handles).
//
//      Note that this class also hides the bookmark property that the first
//      16 values are special, by doing the +16 and -16 arithmetic when converting
//      from ChRows to HROWs and HROWs to ChRows.
//
class ChRow
{
    private:
        int     _index;                 // today this is an _index.  Tomorrow it
                                        // could be a (CIindex *).

        // Coercion from ChRow to int is private to ensure type safety
        operator const int&() {return _index;};

    public:
        // Override null constructor to make sure the compiler doesn't make one!
        ChRow() {(_index = 0);}         // create a Null _index
        static ChRow Invalid() {return ChRow();} // construct & return NULL element

        // Override = operator to make sure the compiler doesn't make one.
        ChRow operator = (const ChRow &href) {_index = href._index; return *this;}

        // We use functions instead of conversion operators here to hide the
        // data representation from our customers.
        const ULONG DeRef() {return (ULONG)_index;};
        ChRow   SetRef(ULONG i) {_index = i; return *this;}; // Assign & return

        // construct & init an element
        BOOL    FHrefValid() {return _index != 0;}

        ChRow   NextHRef() {++_index; return *this;}
        const ChRow   FirstHRef() {_index = 1; return *this;}

        // HROW to ChRow conversions -- external HROWs start at 17, internal
        // ChRows start at 1.
        HROW ToNileHRow() {return (HROW) (_index+16);}
        ChRow (HROW hRow) {_index = (int)hRow-16;}
};

#ifdef NEVER
// As part the hiearchy scheme, we can't have a combined rowmap object anymore.
// The _mapIndex2hRow array is associated with the per STD data now.
// The _maphRow2Index array is associated with the per Rowset data now.

class CRowMap
{
    protected:
        CRowArray<ChRow> _mapIndex2hRow;   // Row table (entries are ChRow's)
        CRowArray<CRow>  _maphRow2Index;   // Handle table (entries are CRow's)
        LONG             _NextH2R;      // Index of next free _mpHandle2Row


    public:
        CRowMap() { _NextH2R = 1; }
        ChRow   HRowFromRow(ULONG);
        int     RowFromHRow(ChRow href)
                { return _maphRow2Index.GetElem(href.DeRef()).Row();}
        BOOL    FhRowDeleted(ChRow href)
                { return _maphRow2Index.GetElem(href.DeRef()).FDeleted();}
        
        // InsertRows returns FALSE for out of memory
        HRESULT InsertRows(ULONG Row,    // row idx to delete (0 is before 1st)
                           int crows);  // # of rows to delete
        // DeleteRows has no failure modes
        void    DeleteRows(ULONG Row,    // row idx to delete (0 is first row)
                           int crows);  // # of rows to delete
        void    InvalidateMap();        // Invalidates whole rowmap
};
#endif
