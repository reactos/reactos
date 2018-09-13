//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       formsary.hxx
//
//  Contents:   CImplAry* classes
//
//----------------------------------------------------------------------------

#ifndef I_FORMSARY_HXX_
#define I_FORMSARY_HXX_
#pragma INCMSG("--- Beg 'formsary.hxx'")

//+------------------------------------------------------------------------
//
// This is the implementation of the generic resizeable array classes. There
// are four array classes:
//
// CPtrAry<ELEM> --
//
//       Dynamic array class which is optimized for sizeof(ELEM) equal
//       to 4. The array is initially empty with no space or memory allocated
//       for data.
//
// CDataAry<ELEM> --
//
//       Same as CPtrAry but where sizeof(ELEM) is != 4 and less than 128.
//
// CStackPtrAry<ELEM, N> --
//
//       Dynamic array class optimized for sizeof(ELEM) equal to 4.
//       Space for N elements is allocated as member data of the class. If
//       this class is created on the stack, then space for N elements will
//       be created on the stack. The class can grow beyond N elements, at
//       which point memory will be allocated for the array data.
//
// CStackDataAry<ELEM, N> --
//
//       Same as CStackPtrAry, but where sizeof(ELEM) is != 4 and less than 128.
//
//
// All four classes have virtually the same methods, and are used the same.
// The only difference is that the DataAry classes have AppendIndirect and
// InsertIndirect, while the PtrAry classes use Append and Insert. The reason
// for the difference is that the Indirect methods take a pointer to the data,
// while the non-indirect methods take the actual data as an argument.
//
// The Stack arrays (CStackPtrAry and CStackDataAry) are used to pre-allocate
// space for elements in the array. This is useful if you create the array on
// the stack and you know that most of the time the array will be less than
// a certain number of elements. Creating one of these arrays on the stack
// allocates the array on the stack as well, preventing a separate memory
// allocation. Only if the array grows beyond the initial size will any
// additional memory be allocated.
//
// The fastest and most efficient way of looping through all elements in
// the array is as follows:
//
//            ELEM * pElem;
//            int    i;
//
//            for (i = aryElems.Size(), pElem = aryElems;
//                 i > 0;
//                 i--, pElem++)
//            {
//                (*pElem)->DoSomething();
//            }
//
// This loop syntax has been shown to be the fastest and produce the smallest
// code. Here's an example using a real data type:
//
//            CStackPtrAry<CSite*, 16> arySites;
//            CSite **ppSite;
//            int     i;
//
//            // Populate the array.
//            ...
//
//            // Now loop through every element in the array.
//            for (i = arySites.Size(), ppSite = arySites;
//                 i > 0;
//                 i--, ppSite++)
//            {
//                (*ppSite)->DoSomething();
//            }
//
// METHOD DESCRIPTIONS:
//
// Commonly used methods:
//
//        Size()             Returns the number of elements currently stored
//                           in the array.
//
//        operator []        Returns the given element in the array.
//
//        Item(int i)        Returns the given element in the array.
//
//        operator ELEM*     Allows the array class to be cast to a pointer
//                           to ELEM. Returns a pointer to the first element
//                           in the array. (Same as a Base() method).
//
//        Append(ELEM e)     Adds a new pointer to the end of the array,
//                           growing the array if necessary.  Only valid
//                           for arrays of pointers (CPtrAry, CStackPtrAry).
//
//        AppendIndirect(ELEM *pe, ELEM** ppePlaced)
//                           As Append, for non-pointer arrays
//                           (CDataAry, CStackDataAry).
//                           pe [in] - Pointer to element to add to array. The
//                                     data is copied into the array. Can be
//                                     NULL, in which case the new element is
//                                     initialized to all zeroes.
//                           ppePlaced [out] - Returns pointer to the new
//                                     element. Can be NULL.
//
//        Insert(int i, ELEM e)
//                           Inserts a new element (e) at the given index (i)
//                           in the array, growing the array if necessary. Any
//                           elements at or following the index are moved
//                           out of the way.
//
//        InsertIndirect(int i, ELEM *pe)
//                           As Insert, for non-pointer arrays
//                                 (CDataAry, CStackDataAry).
//
//        Find(ELEM e)       Returns the index at which a given element (e)
//                           is found (CPtrAry, CStackPtrAry).
//
//        FindIndirect(ELEM *pe)
//                           As Find, for non-pointer arrays
//                                 (CDataAry, CStackDataAry).
//
//        DeleteAll()        Empties the array and de-allocates associated
//                           memory.
//
//        Delete(int i)      Deletes an element of the array, moving any
//                           elements that follow it to fill
//
//        DeleteMultiple(int start, int end)
//                           Deletes a range of elements from the array,
//                           moving to fill. [start] and [end] are the indices
//                           of the start and end elements (inclusive).
//
//        DeleteByValue(ELEM e)
//                           Delete the element matching the given value.
//
//        DeleteByValueIndirect(ELEM *pe)
//                           As DeleteByValue, for non-pointer arrays.
//                                    (CDataAry, CStackDataAry).
//
//
// Less commonly used methods:
//
//        EnsureSize(long c) If you know how many elements you are going to put
//                          in the array before you actually do it, you can use
//                          EnsureSize to allocate the memory all at once instead
//                          of relying on Append(Indirect) to grow the array. This
//                          can be much more efficient (by causing only a single
//                          memory allocation instead of many) than just using
//                          Append(Indirect). You pass in the number of elements
//                          that memory should be allocated for. Note that this
//                          does not affect the "Size" of the array, which is
//                          the number of elements currently stored in it.
//
//        SetSize(int c)    Sets the "Size" of the array, which is the number
//                          of elements currently stored in it. SetSize will not
//                          allocate memory if you're growing the array.
//                          EnsureSize must be called first to reserve space if
//                          the array is growing. Setting the size smaller does
//                          not de-allocate memory, it just chops off the
//                          elements at the end of the array.
//
//        Grow(int c)       Equivalent to calling EnsureSize(c) followed by
//                          SetSize(c).
//
//        BringToFront(int i) Moves the given element of the array to index 0,
//                          shuffling elements to make room.
//
//        SendToBack(int i) Moves the given element to the end of the array,
//                          shuffling elements to make room.
//
//        Swap(int i, int j) Swaps the given two elements.
//
//        ReleaseAll()      (CPtrAry and CStackPtrAry only) Calls Release()
//                          on each element in the array and empties the array.
//
//        ReleaseAndDelete(int idx)
//                          (CPtrAry and CStackPtrAry only) Calls Release() on
//                          the given element and removes it from the array.
//
//           (See the class definitions below for signatures of the following
//            methods and src\core\cdutil\formsary.cxx for argument
//            descriptions)
//
//        CopyAppend        Appends data from another array (of the same type)
//                          to the end.
//
//        Copy              Copies data from another array (of the same type)
//                          into this array, replacing any existing data.
//
//        CopyAppendIndirect  Appends data from a C-style array of element data
//                          to the end of this array.
//
//        CopyIndirect      Copies elements from a C-style array into this array
//                          replacing any existing data.
//
//        EnumElements      Create an enumerator which supports the given
//                          interface ID for the contents of the array
//
//        EnumVARIANT       Create an IEnumVARIANT enumerator.
//
//        operator void *   Allow the CImplAry class to be cast
//                          to a (void *). Avoid using if possible - use
//                          the type-safe operator ELEM * instead.
//
//        ClearAndReset     Obsolete. Do not use.
//
//
//----------------------------------------------------------------------------





//----------------------------------------------------------------------------
//
// Class:     CImplAry
//
// Purpose:   Base implementation of all the dynamic array classes.
//
// Interface:
//
//        Deref       Returns a pointer to an element of the array;
//                    should only be used by derived classes. Use the
//                    type-safe methods operator[] or Item() instead.
//
//        GetAlloced  Get number of elements allocated
//
//  Members:    _c          Current size of the array
//              _pv         Buffer storing the elements
//
//  Note:       The CImplAry class only supports arrays of elements
//              whose size is less than 128.
//
//-------------------------------------------------------------------------


class CImplAry
{
    friend class CBaseEnum;
    friend class CEnumGeneric;
    friend class CEnumVARIANT;
    friend class CImplPtrAry;

private:
                CImplAry();
                DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
                DECLARE_MEMMETER_NEW
                ~CImplAry();
    inline int         Size() const    { return _c; } // UNIX: long->int for min() macro
    inline void        SetSize(int c)  { _c = c; }
    inline operator void *()           { return PData(); }
    void        DeleteAll();

    // BUGBUG -- This method should be protected, but I don't want to convert
    // existing code that uses it. (lylec)
    void *      Deref(size_t cb, int i);

#if DBG == 1
    BOOL _fCheckLock ; // If set with TraceTag CImplAryLock then any change
                       // (addition or deletion to the DataAry will generate an assert.

    void        LockCheck(BOOL fState)
      { _fCheckLock = fState; }
#else
    void        LockCheck(BOOL)
      {  }
#endif

    NO_COPY(CImplAry);

protected:

    //  Methods which are wrapped by inline subclass methods

                CImplAry(PERFMETERTAG mt);

    HRESULT     EnsureSize(size_t cb, long c);
    HRESULT     Grow(size_t cb, int c);
    HRESULT     AppendIndirect(size_t cb, void * pv, void ** ppvPlaced=NULL);
    HRESULT     InsertIndirect(size_t cb, int i, void * pv);
    int         FindIndirect(size_t cb, void *);

    void        Delete(size_t cb, int i);
    BOOL        DeleteByValueIndirect(size_t cb, void *pv);
    void        DeleteMultiple(size_t cb, int start, int end);

#ifdef NEVER
    void        BringToFront(size_t cb, int i);
    void        SendToBack(size_t cb, int i);

    void        Swap(size_t cb, int i1, int i2);

    HRESULT     CopyAppendIndirect(size_t cb, int c, void * pv, BOOL fAddRef);
#endif // NEVER
    HRESULT     CopyAppend(size_t cb, const CImplAry& ary, BOOL fAddRef);
    HRESULT     Copy(size_t cb, const CImplAry& ary, BOOL fAddRef);
    HRESULT     CopyIndirect(size_t cb, int c, void * pv, BOOL fAddRef);

    ULONG       GetAlloced(size_t cb);

    HRESULT     EnumElements(
                        size_t  cb,
                        REFIID  iid,
                        void ** ppv,
                        BOOL    fAddRef,
                        BOOL    fCopy = TRUE,
                        BOOL    fDelete = TRUE);

    HRESULT     EnumVARIANT(
                        size_t  cb,
                        VARTYPE         vt,
                        IEnumVARIANT ** ppenum,
                        BOOL            fCopy = TRUE,
                        BOOL            fDelete = TRUE);

    inline BOOL        UsingStackArray()
                    { return _fDontFree; }

    UINT        GetStackSize()
                    { Assert(_fStack);
                      return *(UINT*)((BYTE*)this + sizeof(CImplAry)); }
    void *      GetStackPtr()
                    { Assert(_fStack);
                      return (void*)((BYTE*)this + sizeof(CImplAry) + sizeof(int)); }

#ifdef WIN16
    BOOL _fStack     ;  // Set if we're a stack-based array.
    BOOL _fDontFree  ;  // Cleared if _pv points to alloced memory.
    unsigned int _c  ; // Count of elements
#else
    unsigned long   _fStack     :1  ;  // Set if we're a stack-based array.
    unsigned long   _fDontFree  :1  ;  // Cleared if _pv points to alloced memory.
    unsigned long   _c          :30 ; // Count of elements
#endif

#ifdef PERFMETER
    PERFMETERTAG    _mt;
#endif

    void *          _pv;

    inline void * & PData()    { return _pv; }
};

//+------------------------------------------------------------------------
//
//  Member:     CImplAry::CImplAry
//
//+------------------------------------------------------------------------
inline
CImplAry::CImplAry()
{
    memset(this, 0, sizeof(CImplAry));
}

inline
CImplAry::CImplAry(PERFMETERTAG mt)
{
    memset(this, 0, sizeof(CImplAry));

#ifdef PERFMETER
    _mt = mt;
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CImplAry::Deref
//
//  Synopsis:   Returns a pointer to the i'th element of the array. This
//              method is normally called by type-safe methods in derived
//              classes.
//
//  Arguments:  i
//
//  Returns:    void *
//
//-------------------------------------------------------------------------

inline void *
CImplAry::Deref(size_t cb, int i)
{
    Assert(i >= 0);
    Assert(ULONG( i ) < GetAlloced(cb));
    return ((BYTE *) PData()) + i * cb;
}

//+------------------------------------------------------------------------
//
//  Class:      CImplPtrAry (ary)
//
//  Purpose:    Subclass used for arrays of pointers.  In this case, the
//              element size is known to be sizeof(void *).  Normally, the
//              CPtrAry template is used to define a specific concrete
//              implementation of this class, to hold a specific type of
//              pointer.
//
//              See documentation above for use.
//
//-------------------------------------------------------------------------

class CImplPtrAry : public CImplAry
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    CImplPtrAry() : CImplAry() {};

protected:
    DECLARE_MEMMETER_NEW

    CImplPtrAry(PERFMETERTAG mt) : CImplAry(mt) {}

    HRESULT     Append(void * pv);
    HRESULT     Insert(int i, void * pv);
    int         Find(void * pv);
    BOOL        DeleteByValue(void *pv);

    HRESULT     CopyAppend(const CImplAry& ary, BOOL fAddRef);
#ifdef NEVER
    HRESULT     CopyAppendIndirect(int c, void * pv, BOOL fAddRef);
#endif // NEVER
    HRESULT     Copy(const CImplAry& ary, BOOL fAddRef);
    HRESULT     CopyIndirect(int c, void * pv, BOOL fAddRef);


public:

    HRESULT     ClearAndReset();

    HRESULT     EnsureSize(long c);

    HRESULT     Grow(int c);

#ifdef NEVER
    void        BringToFront(int i);
    void        SendToBack(int i);

    void        Swap(int i1, int i2);
#endif // NEVER

    void        Delete(int i);
    void        DeleteMultiple(int start, int end);

    void        ReleaseAll();
    void        ReleaseAndDelete(int idx);

    HRESULT     EnumElements(
                        REFIID iid,
                        void ** ppv,
                        BOOL fAddRef,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE);

    HRESULT     EnumVARIANT(
                        VARTYPE vt,
                        IEnumVARIANT ** ppenum,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE);
};


//+---------------------------------------------------------------------------
//
//  Class:      CDataAry
//
//  Purpose:    This template class declares a concrete derived class
//              of CImplAry.
//
//              See documentation above for use.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CDataAry : public CImplAry
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(mtMem)
    CDataAry();
public:
    DECLARE_MEMMETER_NEW
    CDataAry(PERFMETERTAG mt) : CImplAry(mt) { }
    operator ELEM *() { return (ELEM *)PData(); }
    CDataAry(const CDataAry &);

#ifdef WIN16
    ELEM & operator[](int i) { return *(ELEM*)(((BYTE *) _pv) + i * sizeof(ELEM)); }
#endif  // !WIN16

#ifdef UNIX
    ELEM & operator[](int i) { return *(ELEM*)Deref(sizeof(ELEM), i); }
#endif  // UNIX so that the [] operator is defined

    ELEM & Item(int i) { return *(ELEM*)Deref(sizeof(ELEM), i); }

    HRESULT     EnsureSize(long c)
                    { return CImplAry::EnsureSize(sizeof(ELEM), c); }
    HRESULT     Grow(int c)
                    { return CImplAry::Grow(sizeof(ELEM), c); }
    HRESULT     AppendIndirect(ELEM * pe, ELEM ** ppePlaced=NULL)
                    { return CImplAry::AppendIndirect(sizeof(ELEM), (void*)pe, (void**)ppePlaced); }
    ELEM *      Append()
                    { ELEM * pElem; return AppendIndirect( NULL, & pElem ) ? NULL : pElem; }
    HRESULT     InsertIndirect(int i, ELEM * pe)
                    { return CImplAry::InsertIndirect(sizeof(ELEM), i, (void*)pe); }
    int         FindIndirect(ELEM * pe)
                    { return CImplAry::FindIndirect(sizeof(ELEM), (void*)pe); }

    void        Delete(int i)
                    { CImplAry::Delete(sizeof(ELEM), i); }
    BOOL        DeleteByValueIndirect(ELEM *pe)
                    { return CImplAry::DeleteByValueIndirect(sizeof(ELEM), (void*)pe); }
    void        DeleteMultiple(int start, int end)
                    { CImplAry::DeleteMultiple(sizeof(ELEM), start, end); }

#ifdef NEVER
    void        BringToFront(int i)
                    { CImplAry::BringToFront(sizeof(ELEM), i); }
    void        SendToBack(int i)
                    { CImplAry::SendToBack(sizeof(ELEM), i); }

    void        Swap(int i1, int i2)
                    { CImplAry::Swap(sizeof(ELEM), i1, i2); }

    HRESULT     CopyAppendIndirect(int c, ELEM * pv, BOOL fAddRef)
                    { return CImplAry::CopyAppendIndirect(sizeof(ELEM), c, (void*)pv, fAddRef); }
#endif // NEVER
    HRESULT     CopyAppend(const CDataAry<ELEM>& ary, BOOL fAddRef)
                    { return CImplAry::Copy(sizeof(ELEM), ary, fAddRef); }
    HRESULT     Copy(const CDataAry<ELEM>& ary, BOOL fAddRef)
                    { return CImplAry::Copy(sizeof(ELEM), ary, fAddRef); }
    HRESULT     CopyIndirect(int c, ELEM * pv, BOOL fAddRef)
                    { return CImplAry::CopyIndirect(sizeof(ELEM), c, (void*)pv, fAddRef); }

    HRESULT     EnumElements(
                        REFIID iid,
                        void ** ppv,
                        BOOL fAddRef,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE)
                    { return CImplAry::EnumElements(sizeof(ELEM), iid, ppv, fAddRef, fCopy, fDelete); }

    HRESULT     EnumVARIANT(
                        VARTYPE vt,
                        IEnumVARIANT ** ppenum,
                        BOOL fCopy = TRUE,
                        BOOL fDelete = TRUE)
                    { return CImplAry::EnumVARIANT(sizeof(ELEM), vt, ppenum, fCopy, fDelete); }
};

#define DECLARE_CDataAry(tAry, tElem, mtAry, mtPv) \
    class tAry : public CDataAry<tElem> \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CDataAry<tElem>(mtPv) {} };

//+---------------------------------------------------------------------------
//
//  Class:      CPtrAry
//
//  Purpose:    This template class declares a concrete derived class
//              of CImplPtrAry.
//
//              See documentation above for use.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CPtrAry : public CImplPtrAry
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(mtMem);
    CPtrAry();

public:
    DECLARE_MEMMETER_NEW
    CPtrAry(PERFMETERTAG mt) : CImplPtrAry(mt) { Assert(sizeof(ELEM) == sizeof(void*)); }
    operator ELEM *() { return (ELEM *)PData(); }
    CPtrAry(const CPtrAry &);

#ifdef WIN16
    ELEM & operator[](int i) { return *(ELEM*)Deref(sizeof(ELEM), i); }
#endif //!WIN16

#ifdef UNIX
    ELEM & operator[](int i) { return *(ELEM*)Deref(sizeof(ELEM), i); }
#endif // UNIX so that the [] operator is defined.

    ELEM & Item(int i) { return *(ELEM*)Deref(sizeof(ELEM), i); }

    HRESULT     Append(ELEM e)
                    { return CImplPtrAry::Append((void*)(DWORD_PTR)e); }
    HRESULT     Insert(int i, ELEM e)
                    { return CImplPtrAry::Insert(i, (void*)(DWORD_PTR)e); }
    BOOL        DeleteByValue(ELEM e)
                    { return CImplPtrAry::DeleteByValue((void*)(DWORD_PTR)e); }
    int         Find(ELEM e)
                    { return CImplPtrAry::Find((void*)(DWORD_PTR)e); }

    HRESULT     CopyAppend(const CPtrAry<ELEM>& ary, BOOL fAddRef)
                    { return CImplPtrAry::Copy(ary, fAddRef); }
#ifdef NEVER
    HRESULT     CopyAppendIndirect(int c, ELEM *pe, BOOL fAddRef)
                    { return CImplPtrAry::CopyAppendIndirect(c, (void*)pe, fAddRef); }
#endif // NEVER
    HRESULT     Copy(const CPtrAry<ELEM>& ary, BOOL fAddRef)
                    { return CImplPtrAry::Copy(ary, fAddRef); }
    HRESULT     CopyIndirect(int c, ELEM *pe, BOOL fAddRef)
                    { return CImplPtrAry::CopyIndirect(c, (void*)pe, fAddRef); }

};

#define DECLARE_CPtrAry(tAry, tElem, mtAry, mtPv) \
    class tAry : public CPtrAry<tElem> \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CPtrAry<tElem>(mtPv) {} };

//+---------------------------------------------------------------------------
//
//  Class:      CStackDataAry
//
//  Purpose:    Declares a CDataAry that has initial storage on the stack.
//              N elements are declared on the stack, and the array will
//              grow dynamically beyond that if necessary.
//
//              See documentation above for use.
//
//----------------------------------------------------------------------------

template <class ELEM, int N>
class CStackDataAry : public CDataAry<ELEM>
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    CStackDataAry();
public:
    CStackDataAry(PERFMETERTAG mt) : CDataAry<ELEM>(mt)
    {
        _cStack     = N;
        _fStack     = TRUE;
        _fDontFree  = TRUE;
        PData()     = (void *) & _achTInit;
        WHEN_DBG( _pElemsInit = (ELEM * ) _achTInit; )
    }

protected:
    int   _cStack;                     // Must be first data member.
    char  _achTInit[N*sizeof(ELEM)];
    WHEN_DBG( ELEM * _pElemsInit; )
};

#define DECLARE_CStackDataAry(tAry, tElem, cElem, mtAry, mtPv) \
    class tAry : public CStackDataAry<tElem, cElem> \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CStackDataAry<tElem, cElem>(mtPv) {} };

//+---------------------------------------------------------------------------
//
//  Class:      CStackPtrAry
//
//  Purpose:    Same as CStackDataAry except for pointer types.
//
//              See documentation above for use.
//
//----------------------------------------------------------------------------

template <class ELEM, int N>
class CStackPtrAry : public CPtrAry<ELEM>
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem));
    CStackPtrAry();
public:
    CStackPtrAry(PERFMETERTAG mt) : CPtrAry<ELEM>(mt)
    {
        _cStack     = N;
        _fStack     = TRUE;
        _fDontFree  = TRUE;
        PData()     = (void *) & _achTInit;
        WHEN_DBG( _pElemsInit = (ELEM *) _achTInit; )
    }

protected:
    int   _cStack;                     // Must be first data member.
    char  _achTInit[N*sizeof(ELEM)];
    WHEN_DBG( ELEM * _pElemsInit; )
};

#define DECLARE_CStackPtrAry(tAry, tElem, cElem, mtAry, mtPv) \
    class tAry : public CStackPtrAry<tElem, cElem> \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CStackPtrAry<tElem, cElem>(mtPv) {} };

//+---------------------------------------------------------------------------
//
//  Class:      CBaseEnum (benum)
//
// BUGBUG (garybu) Replace this class with more generic CEnumXXX in stdenum.cxx.
//
//  Purpose:    Base OLE enumerator class for a CImplAry.
//
//  Interface:  DECLARE_FORMS_STANRARD_IUNKNOWN
//
//              Next                   -- Per IEnum*
//              Skip                   --    ""
//              Reset                  --    ""
//              Clone                  --    ""
//              CBaseEnum              -- ctor.
//              CBaseEnum              -- ctor.
//              ~CBaseEnum             -- dtor.
//              Init                   -- 2nd stage initialization.
//              Deref                  -- gets pointer to element.
//
//  Notes:      Since there is no IEnum interface, we create a vtable
//              with the same layout as all IEnum interfaces.  Be careful
//              where you put virtual function declarations!
//
//----------------------------------------------------------------------------


class CBaseEnum : public IUnknown
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_FORMS_STANDARD_IUNKNOWN(CBaseEnum);

    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched) PURE;
    STDMETHOD(Skip) (ULONG celt);
    STDMETHOD(Reset) ();
    STDMETHOD(Clone) (CBaseEnum ** ppenum) PURE;

    //
    // Ensure that vtable contains virtual destructor after other virtual methods.
    //

    virtual ~CBaseEnum();

protected:
    CBaseEnum(size_t cb, REFIID iid, BOOL fAddRef, BOOL fDelete);
    CBaseEnum(const CBaseEnum & benum);

    CBaseEnum& operator=(const CBaseEnum & benum); // don't define

    HRESULT Init(CImplAry * pary, BOOL fCopy);
    void *  Deref(int i);

    CImplAry * _pary;
    const IID * _piid;
    int         _i;
    size_t      _cb;
    BOOL        _fAddRef;
    BOOL        _fDelete;

#ifdef OBJCNTCHK
    DWORD       _dwObjCnt;
#endif

};

//+---------------------------------------------------------------------------
//
//  Member:     CBaseEnum::Deref
//
//  Synopsis:   Forwards deref to _pary.  Required because classes derived
//              from CBaseEnum are friends of CImplAry.
//
//----------------------------------------------------------------------------
inline void *
CBaseEnum::Deref(int i)
{
    Assert(i >= 0);
    return (BYTE *)_pary->PData() + i * _cb;
}

//+---------------------------------------------------------------------------
//
//  Class:      CStackCStrAry
//
//  Synopsis:   holds CStr's which are Free()'ed when we destruct
//
//----------------------------------------------------------------------------

class CStackCStrAry : public CStackDataAry<CStr, 10>
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
    CStackCStrAry();
public:
    DECLARE_MEMMETER_NEW
    CStackCStrAry(PERFMETERTAG mt) : CStackDataAry<CStr, 10> (mt) {}
    ~CStackCStrAry();
};

#define DECLARE_CStackCStrAry(tAry, mtAry, mtPv) \
    class tAry : public CStackCStrAry \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CStackCStrAry(mtPv) {} };

MtExtern(CURLAry)
MtExtern(CURLAry_pv)
DECLARE_CStackCStrAry(CURLAry, Mt(CURLAry), Mt(CURLAry_pv));

//+---------------------------------------------------------------------------
//
//  Class:      CStackIPrintAry
//
//  Synopsis:   holds CStr's which are Free()'ed when we destruct
//
//----------------------------------------------------------------------------

interface IPrint;

class CStackIPrintAry : public CStackDataAry<IPrint *, 10>
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem));
    CStackIPrintAry();
public:
    DECLARE_MEMMETER_NEW
    CStackIPrintAry(PERFMETERTAG mt) : CStackDataAry<IPrint *, 10> (mt) {}
    ~CStackIPrintAry();
};

#define DECLARE_CStackIPrintAry(tAry, mtAry, mtPv) \
    class tAry : public CStackIPrintAry \
        { public: DECLARE_MEMALLOC_NEW_DELETE(mtAry); DECLARE_MEMMETER_NEW; tAry() : CStackIPrintAry(mtPv) {} };

MtExtern(CIPrintAry)
MtExtern(CIPrintAry_pv)
MtExtern(CPrintInfoFlagsAry)
MtExtern(CPrintInfoFlagsAry_pv)

DECLARE_CStackIPrintAry(CIPrintAry, Mt(CIPrintAry), Mt(CIPrintAry_pv))
DECLARE_CPtrAry(CPrintInfoFlagsAry, DWORD, Mt(CPrintInfoFlagsAry), Mt(CPrintInfoFlagsAry_pv))

#define PIF_HTMLDOCUMENT 1
#define PIF_USEPRETTYURL 2

#pragma INCMSG("--- End 'formsary.hxx'")
#else
#pragma INCMSG("*** Dup 'formsary.hxx'")
#endif
