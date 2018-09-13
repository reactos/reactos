//+-----------------------------------------------------------------------
//
//  Template Array Implementation
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      tarray.hxx
//  Author:    Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Template for resizeable arrays.  Allows for the creation
//             and manipulation of arrays of any type.  Arrays can
//             be dynamically reallocated, and thus can "grow and shrink".
//             Constructors and destructors of array elements is
//             automatically handles, even when the size of the array is
//             changed.
//  Templates: TSTDArray
//
//
//  06/22/95:   t-idoben    Added two member functions:
//               InsertElems   Allows elements to be inserted anywhere in array
//               DeleteElems   Remove elements anywhere in array
//  07/18/95:   TerryLu     Added sentries
//
//------------------------------------------------------------------------

#ifndef _TARRAY_HXX_
#define _TARRAY_HXX_

MtExtern(STDArray)

//+-----------------------------------------------------------------------
//
//  Class:     TSTDArray
//
//  Synopsis:  Contains an array of "type".  Allows array to grow
//             dynamically.  During debug, can check bounds on indices.
//             Array is indexed 0 to _cArraySize-1.  _cArraySize holds
//               number of elements.
//
//  Methods:   Init          allocate memory for array
//             Passivate
//             []            allows indexing of array
//             GetSize       returns size of array
//             InsertElems   insert elements anywhere in array
//             DeleteElems   delete elements anywhere in array
//

template <class TYPE>
class TSTDArray
{
public:
    TSTDArray();
#if DBG == 1
    ~TSTDArray();
#endif
    HRESULT Init(const size_t cSize); // initialize data structures
    void Passivate();

    TYPE& operator[](const size_t iElement);
    const TYPE& operator[](const size_t iElement) const; // constant reference
    size_t GetSize() const { return _cArraySize; }

    HRESULT InsertElems(const size_t iElem, const size_t cElems);
    void DeleteElems(const size_t iElem, const size_t cElems);

private:
#if DBG == 1
    void IsValidObject() const;
#endif

// All elements are packaged inside a class CElem.  This allows us to
//   overload the new operator so that we can manually invoke the
//   constructors.

    class CElem
    {
        friend TSTDArray;
    private:
        // Now we overload the new operator to allow placement argument
        void *operator new(size_t uSize, void *pv) { return pv; }

        // Internal data:
        TYPE _Element;       // actual element
    };

// Internal data:
    CElem *_paArray;        // pointer to actual data
    size_t _cArraySize;

#if DBG == 1
    // Ensure we call constructors and destructors right number of times.
    //   Used only as a check.
    size_t _cNumElems;
#endif
};


//+---------------------------------------------------------------------------
//
//  Member:     IsValidObject
//
//  Synopsis:   Validation method, called by macro IS_VALID(p).
//              Checks that array structure is valid.  It is usefull to
//              call this member function at the beginning of each member
//              function that uses the internal array to ensure that the
//              array is not corrupt before attempting to modify it.
//

#if DBG == 1
template <class TYPE>
void
TSTDArray<TYPE>::IsValidObject() const
{
    Assert("Must have valid this pointer" &&
           this );

    Assert("Array has no memory" &&
           _paArray );

    Assert("destructors called wrong number of times" &&
           (_cNumElems == _cArraySize) );
}
#endif


//+-----------------------------------------------------------------------
//
//  Constructor for TSTDArray
//
//  Synopsis:  Doesn't do anything.  Must call member function init to
//             actually initialize.  Only call init once.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

template <class TYPE>
TSTDArray<TYPE>::TSTDArray()
{
// We null the internal data, so that they are not actually used
// until the init member function is called.

    _paArray = 0;
    _cArraySize = 0;

#if DBG == 1
    _cNumElems = 0;
#endif
}


//+-----------------------------------------------------------------------
//
//  Destructor for TSTDArray
//
//  Synopsis:  Must call member function passivate
//             to actually de-initialize.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//

#if DBG == 1
template <class TYPE>
TSTDArray<TYPE>::~TSTDArray()
{
    Assert("Passivate must be called first" &&
           !_paArray );

    Assert("Destructors called wrong number of times" &&
           (_cNumElems == 0) );
}
#endif


//+-----------------------------------------------------------------------
//
//  Member:    Init
//
//  Synopsis:  Initializes the array abstract data type.  Allocates
//             memory for the array.  Also sets the cArraySize to
//             the number of elements.
//
//  Arguments: cSize    initial size of array (# of elements)
//
//  Returns:   Success if memory can be allocated for table.
//             Returns E_OUTOFMEMORY if can't get memory.
//

template <class TYPE>
HRESULT
TSTDArray<TYPE>::Init(const size_t cSize)
{
    HRESULT hr;

    Assert(this);

    Assert("Only call init once" &&
           !_paArray );

// Get memory:

    // 0 element array is made into 1 element array so that it functions
    //   normally.
    {
        // BUGBUG:
        // MSVC 2.0 has a bug in it.  Evaluating the expression sizeof(CElem)
        //   seems to confuse it.  CElem is a class containing a variable
        //   whose size can only be calculated when the template containing it
        //   is instantiated.  In addition to this, CElem is a member of that
        //   template.  Placing the sizeof(CElem) expression within a more
        //   complicated expression is not possible.
        size_t uCElemSize;

        uCElemSize = sizeof(CElem);
        uCElemSize *= (cSize == 0 ? 1 : cSize);
        _paArray = (CElem *) MemAlloc(Mt(STDArray), uCElemSize);
    }
    if (!_paArray)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        CElem *pTemp;                   // index used to call constructors

        // We need to call the constructors manually for each element:
        for (pTemp = _paArray; pTemp < _paArray + cSize; pTemp++)
        {
            new (pTemp) CElem;
#if DBG == 1
            _cNumElems++;
#endif
        }

        _cArraySize = cSize;
        hr = S_OK;
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    Passivate
//
//  Synopsis:  Releases memory held in array.  Should be called before
//             the object is destroyed.  Should only be called once on an
//             object.
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

template <class TYPE>
void
TSTDArray<TYPE>::Passivate()
{
    IS_VALID(this);

    Assert("Only call Passivate once" &&
           _paArray );

    // We need to call the destructors manually for each element:

    {
        CElem *pTemp;                   // index used to call destructors

        for (pTemp = _paArray; pTemp < _paArray + _cArraySize; pTemp++)
        {
            pTemp->CElem::~CElem();
#if DBG == 1
            _cNumElems--;
#endif
        }
    }

    MemFree(_paArray);

    _paArray = 0;         // make sure we don't call Passivate again
    _cArraySize = 0;
}


//+-----------------------------------------------------------------------
//
//  Member:    operator[]
//
//  Synopsis:  Allows indexing of array's elements.  Use this to either
//             store an element in the array or read an element from
//             the array.  It is the user's responsibility to ensure
//             that the index is within the proper range, 0.._cArraySize-1.
//             During debugging, the index range is checked.
//
//  Arguments: iElement       Element index
//
//  Returns:   Reference to element.
//

template <class TYPE>
inline
TYPE&
TSTDArray<TYPE>::operator[](const size_t iElement)
{
    IS_VALID(this);

    Assert("Index is out of range" &&
           (iElement < _cArraySize) );

    return _paArray[iElement]._Element;
}


//+-----------------------------------------------------------------------
//
//  Member:    operator[] const
//
//  Synopsis:  Same as previous [] operator, but returns a constant
//             reference so that it can't be used as an l-value.
//
//  Arguments: iElement       Element index
//
//  Returns:   Constant reference to element.
//

template <class TYPE>
inline
const TYPE&
TSTDArray<TYPE>::operator[](const size_t iElement) const
{
    IS_VALID(this);

    Assert("Index is out of range" &&
           (iElement < _cArraySize) );

    return _paArray[iElement]._Element;
}


//+-----------------------------------------------------------------------
//
//  Member:    InsertElems
//
//  Synopsis:  Changes the size of the array by using MemRealloc().
//             Inserts a number of elements cElems into the array at
//             iElem.  This can be used to add new elements to the end of
//             the array by specifying iElem equal to _cArraySize.  It is
//             the responsibility of the user to make sure that iElem is
//             within the proper bounds of the array, although this will
//             be checked during debug mode.
//
//  Arguments: iElem       place to insert first element
//             cElems      number of new elements
//
//  Returns:   Returns success if elements can be added.
//             Returns E_OUTOFMEMORY if request cannot be met.
//             Array retains its old size if the request cannot be met.
//

template<class TYPE>
HRESULT
TSTDArray<TYPE>::InsertElems(const size_t iElem, const size_t cElems)
{
    HRESULT hr;

    IS_VALID(this);

    // Note that you can insert past the END of an array (appending to it):
    Assert("iElem is too large" &&
           (iElem <= _cArraySize) );

    // Resize the current array we have:
    // 0 element array is made into 1 element array so that it functions
    //   normally.
    hr = MemRealloc(Mt(STDArray), (void**)&_paArray,
                    sizeof(CElem) *
                    ((_cArraySize+cElems == 0) ? 1 : _cArraySize + cElems) );
    if (!hr)
    {

        // Now we have to shift elements to allow space for the new elements:
        memmove(_paArray + iElem + cElems,      // dest
                _paArray + iElem,
                (_cArraySize - iElem) * sizeof(CElem));

        // Call constructors on all new elements:
        {
            CElem *pTemp;   // index used to call constructors

            for (pTemp = _paArray + iElem;
                 pTemp < _paArray + iElem + cElems;
                 pTemp++)
            {
                new (pTemp) CElem;
#if DBG == 1
                _cNumElems++;
#endif
            }
        }

        _cArraySize += cElems;
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    DeleteElems
//
//  Synopsis:  Changes the size of the array by using MemRealloc().
//             Deletes a number of elements cElems from the array at
//             iElem.  It is the responsibility of the user to make sure
//             that the region to be deleted is within the proper bounds
//             of the array, although this will be checked during
//             debug mode.
//
//  Arguments: iElem       place to delete first element
//             cElems      number of elements to delete
//
//  Returns:   Returns success.
//

template<class TYPE>
void
TSTDArray<TYPE>::DeleteElems(const size_t iElem, const size_t cElems)
{
    IS_VALID(this);

    Assert("Region to delete is too large" &&
           (iElem+cElems-1 < _cArraySize) );


    // First we need to call destructors on elements:
    {
        CElem *pTemp;   // index used to call destructors

        for (pTemp = _paArray + iElem;
             pTemp < _paArray + iElem + cElems;
             pTemp++)
        {
            pTemp->CElem::~CElem();
#if DBG == 1
            _cNumElems--;
#endif
        }
    }

    // Now we need to shift the remaining elements in:
    memmove(_paArray + iElem,                // dest
            _paArray + iElem + cElems,
            (_cArraySize - (iElem + cElems)) * sizeof(CElem));

    // Resize the current array we have:
    // 0 element array is made into 1 element array so that it functions
    //   normally.
    IGNORE_HR(MemRealloc(Mt(STDArray), (void**)&_paArray, sizeof(CElem) *
                  ((_cArraySize - cElems == 0) ? 1 : (_cArraySize - cElems)) ));

    _cArraySize -= cElems;
}


#endif  // _TARRAY_HXX_
