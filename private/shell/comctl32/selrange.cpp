//-------------------------------------------------------------------
//
// File: SelRange.cpp
//
// Contents:
//      This file contians Selection Range handling code.
//
//-------------------------------------------------------------------

#include "ctlspriv.h"
#include "selrange.h"
#include "stdio.h"
#include <shguidp.h>

#define MINCOUNT 6      // number of sel ranges to start with amd maintain
#define GROWSIZE 150    // percent to grow when needed

#define COUNT_SELRANGES_NONE 2     // When count of selranges really means none

typedef struct tag_SELRANGEITEM
{
    LONG iBegin;
    LONG iEnd;
} SELRANGEITEM, *PSELRANGEITEM;


class CLVRange : public ILVRange

{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** ILVRange methods ***
    STDMETHODIMP IncludeRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP ExcludeRange(LONG iBegin, LONG iEnd);    
    STDMETHODIMP InvertRange(LONG iBegin, LONG iEnd);
    STDMETHODIMP InsertItem(LONG iItem);
    STDMETHODIMP RemoveItem(LONG iItem);

    STDMETHODIMP Clear();
    STDMETHODIMP IsSelected(LONG iItem);
    STDMETHODIMP IsEmpty();
    STDMETHODIMP NextSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP NextUnSelected(LONG iItem, LONG *piItem);
    STDMETHODIMP CountIncluded(LONG *pcIncluded);

protected:
    // Helper Functions.
    friend ILVRange *LVRange_Create();
                CLVRange();
                ~CLVRange();

    BOOL        _Enlarge();
    BOOL        _Shrink();
    BOOL        _InsertRange(LONG iAfterItem, LONG iBegin, LONG iEnd);
    HRESULT     _RemoveRanges(LONG iStartItem, LONG iStopItem, LONG *p);
    BOOL        _FindValue(LONG Value, LONG* piItem);
    void        _InitNew();

    int           _cRef;
    PSELRANGEITEM _VSelRanges;  // Vector of sel ranges
    LONG          _cSize;       // size of above vector in sel ranges
    LONG          _cSelRanges;  // count of sel ranges used
    LONG          _cIncluded;   // Count of Included items...
};

//-------------------------------------------------------------------
//
// Function: _Enlarge
//
// Summary:
//      This will enlarge the number of items the Sel Range can have.
//
// Arguments:
//      PSELRANGE [in]  - SelRange to Enlarge
//
// Return: FALSE if failed.
//
// Notes: Though this function may fail, pselrange structure is still valid
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL CLVRange::_Enlarge()
{
    LONG cNewSize;
    PSELRANGEITEM pTempSelRange;
    BOOL frt = FALSE;


    cNewSize = _cSize * GROWSIZE / 100;
    pTempSelRange = (PSELRANGEITEM) GlobalReAlloc( (HGLOBAL)_VSelRanges,
                                                   cNewSize * sizeof( SELRANGEITEM ),
                                                   GMEM_ZEROINIT | GMEM_MOVEABLE );
    if (NULL != pTempSelRange)
    {
        _VSelRanges = pTempSelRange;
        _cSize = cNewSize;
        frt = TRUE;
    }
    return( frt );
}

//-------------------------------------------------------------------
//
// Function: _Shrink
//
// Summary:
//      This will reduce the number of items the Sel Range can have.
//
// Arguments:
//
// Return: FALSE if failed
//
// Notes: Shrink only happens when a significant size below the next size
//  is obtained and the new size is at least the minimum size.
//      Though this function may fail, pselrange structure is still valid
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL CLVRange::_Shrink()
{
    LONG cNewSize;
    LONG cTriggerSize;
    PSELRANGEITEM pTempSelRange;
    BOOL frt = TRUE;


    // check if we are below last grow area by a small percent
    cTriggerSize = _cSize * 90 / GROWSIZE;
    cNewSize = _cSize * 100 / GROWSIZE;

    if ((_cSelRanges < cTriggerSize) && (cNewSize >= MINCOUNT))
    {
        pTempSelRange = (PSELRANGEITEM) GlobalReAlloc( (HGLOBAL)_VSelRanges,
                                                       cNewSize * sizeof( SELRANGEITEM ),
                                                       GMEM_ZEROINIT | GMEM_MOVEABLE );
        if (NULL != pTempSelRange)
        {
            _VSelRanges = pTempSelRange;
            _cSize = cNewSize;
        }
        else
        {
            frt = FALSE;
        }
    }
    return( frt );
}

//-------------------------------------------------------------------
//
// Function: _InsertRange
//
// Summary:
//      inserts a single range item into the range vector       
//
// Arguments:
//      iAfterItem [in] - Index to insert range after, -1 means insert as first item
//      iBegin [in]     - begin of range
//      iEnd [in]       - end of the range
//
// Return:
//      TRUE if succesful, otherwise FALSE
//
// Notes:
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL CLVRange::_InsertRange(LONG iAfterItem,
                             LONG iBegin,
                             LONG iEnd )
{
    LONG iItem;
    BOOL frt = TRUE;

    ASSERT( iAfterItem >= -1 );
    ASSERT( iBegin >= SELRANGE_MINVALUE );
    ASSERT( iEnd >= iBegin );
    ASSERT( iEnd <= SELRANGE_MAXVALUE );
    ASSERT( _cSelRanges < _cSize );

    // shift all over one
    for (iItem = _cSelRanges; iItem > iAfterItem + 1; iItem--)
    {
        _VSelRanges[iItem] = _VSelRanges[iItem-1];
    }
    _cSelRanges++;

    // make the insertion
    _VSelRanges[iAfterItem+1].iBegin = iBegin;
    _VSelRanges[iAfterItem+1].iEnd = iEnd;

    // make sure we have room next time
    if (_cSelRanges == _cSize)
    {
        frt = _Enlarge();
    }
    return( frt );
}

//-------------------------------------------------------------------
//
// Function: _RemoveRanges
//
// Summary:
//      Removes all ranged between and including the speicifed indexes      
//
// Arguments:
//      iStartItem [in] - Index to start removal
//      iStopItem [in]  - Index to stop removal
//
// Return:
//      SELRANGE_ERROR on memory allocation error
//      The number of items that are unselected by this removal
//
// Notes:
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::_RemoveRanges(LONG iStartItem, LONG iStopItem, LONG *pc )
{
    LONG iItem;
    LONG diff;
    LONG cUnSelected = 0;
    HRESULT hres = S_OK;

    ASSERT( iStartItem > 0 );
    ASSERT( iStopItem >= iStartItem );
    ASSERT( iStartItem < _cSelRanges - 1 );
    ASSERT( iStopItem < _cSelRanges - 1 );
    
    diff = iStopItem - iStartItem + 1;
        
    for (iItem = iStartItem; iItem <= iStopItem; iItem++)
        cUnSelected += _VSelRanges[iItem].iEnd -
                       _VSelRanges[iItem].iBegin + 1;

    // shift all over the difference
    for (iItem = iStopItem+1; iItem < _cSelRanges; iItem++, iStartItem++)
        _VSelRanges[iStartItem] = _VSelRanges[iItem];

    _cSelRanges -= diff;
    
    if (!_Shrink())
    {
        hres = E_FAIL;
    }
    else if (pc)
        *pc = cUnSelected;
    return( hres );
}


//-------------------------------------------------------------------
//
// Function: SelRange_FindValue
//
// Summary:
//      This function will search the ranges for the value, returning true
//  if the value was found within a range.  The piItem will contain the
//  the index at which it was found or the index before where it should be
//  The piItem may be set to -1, meaning that there are no ranges in the list
//      This functions uses a non-recursive binary search algorithm.
//
// Arguments:
//      piItem [out]    - Return of found range index, or one before
//      Value [in]      - Value to find within a range
//
// Return: True if found, False if not found
//
// Notes: The piItem will return one before if return is false.
//
// History:
//      14-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL CLVRange::_FindValue(LONG Value, LONG* piItem )
{
    LONG First;
    LONG Last;
    LONG Item;
    BOOL fFound = FALSE;

    ASSERT( piItem );
    ASSERT( _cSize >= COUNT_SELRANGES_NONE );
    ASSERT( Value >= SELRANGE_MINVALUE );
    ASSERT( Value <= SELRANGE_MAXVALUE );
    

    First = 0;
    Last = _cSelRanges - 1;
    Item = Last / 2;

    do
    {
        if (_VSelRanges[Item].iBegin > Value)
        {   // Value before this Item
            Last = Item;
            Item = (Last - First) / 2 + First;
            if (Item == Last)
            {
                Item = First;   
                break;
            }
        }
        else if (_VSelRanges[Item].iEnd < Value)
        {   // Value after this Item
            First = Item;
            Item = (Last - First) / 2 + First;
            if (Item == First)
            {
                break;
            }
        }
        else
        {   // Value at this Item
            fFound = TRUE;
        }
    } while (!fFound);

    *piItem = Item;
    return( fFound );
}

//-------------------------------------------------------------------
//
// Function: _InitNew
//
// Summary:
//      This function will initialize a SelRange object.
//
// Arguments:
//
// Return:
//
// Notes:
//              
// History:
//      18-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

void CLVRange::_InitNew()
{
    _cSize = MINCOUNT;
    _cSelRanges = COUNT_SELRANGES_NONE;

    _VSelRanges[0].iBegin = LONG_MIN;
    // -2 and +2 below are to stop consecutive joining of end markers
    _VSelRanges[0].iEnd = SELRANGE_MINVALUE - 2;  
    _VSelRanges[1].iBegin =  SELRANGE_MAXVALUE + 2;
    _VSelRanges[1].iEnd = SELRANGE_MAXVALUE + 2;
    _cIncluded = 0;
}

//-------------------------------------------------------------------
//
// Function: SelRange_Create
//
// Summary:
//      This function will create and initialize a SelRange object.
//
// Arguments:
//
// Return: HSELRANGE that is created or NULL if it failed.
//
// Notes:
//
// History:
//      14-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

ILVRange *LVRange_Create( )
{
    CLVRange *pselrange = new CLVRange;

    if (NULL != pselrange)
    {
        pselrange->_VSelRanges = (PSELRANGEITEM) GlobalAlloc( GPTR,
                                       sizeof( SELRANGEITEM ) * MINCOUNT );
        if (NULL != pselrange->_VSelRanges)
        {
            pselrange->_InitNew();
        }
        else
        {
            delete pselrange;
            pselrange = NULL;
        }
    }

    return( pselrange? SAFECAST(pselrange, ILVRange*) : NULL);
}


//-------------------------------------------------------------------
//
// Function: Constructor
//
//-------------------------------------------------------------------
CLVRange::CLVRange()
{
    _cRef = 1;
}

//-------------------------------------------------------------------
//
// Function: Destructor
//
//-------------------------------------------------------------------
CLVRange::~CLVRange()
{
    GlobalFree( _VSelRanges );
}


//-------------------------------------------------------------------
//
// Function: QueryInterface
//
//-------------------------------------------------------------------
HRESULT CLVRange::QueryInterface(REFIID iid, void **ppv)
{
    if (IsEqualIID(iid, IID_ILVRange) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = SAFECAST(this, ILVRange *);
    }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    _cRef++;
    return NOERROR;
}

//-------------------------------------------------------------------
//
// Function: AddRef
//
//-------------------------------------------------------------------
ULONG CLVRange::AddRef()
{
    return ++_cRef;
}

//-------------------------------------------------------------------
//
// Function: Release
//
//-------------------------------------------------------------------
ULONG CLVRange::Release()
{
    if (--_cRef)
        return _cRef;

    delete this;
    return 0;
}

                
//-------------------------------------------------------------------
//
// Function: IncludeRange
//
// Summary:
//      This function will include the range defined into the current
//  ranges, compacting as needed.
//
// Arguments:
//      hselrange [in]  - Handle to the SelRange
//      iBegin [in]     - Begin of new range
//      iEnd [in]       - End of new range
//
// Notes:
//
// History:
//      14-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::IncludeRange(LONG iBegin, LONG iEnd )
{
    LONG iFirst;   // index before or contains iBegin value
    LONG iLast;    // index before or contains iEnd value
    BOOL fExtendFirst;  // do we extend the iFirst or create one after it
    LONG iRemoveStart;  // start of ranges that need to be removed
    LONG iRemoveFinish; // end of ranges that need to be removed

    LONG iNewEnd;   // calculate new end value as we go
    BOOL fEndFound; // was the iEnd found in a range already
    BOOL fBeginFound; // was the iEnd found in a range already

    LONG cSelected = 0;
    HRESULT hres = S_OK;

    ASSERT( iEnd >= iBegin );
    ASSERT( iBegin >= SELRANGE_MINVALUE );
    ASSERT( iEnd <= SELRANGE_MAXVALUE );

    // find approximate locations
    fBeginFound = _FindValue( iBegin, &iFirst );
    fEndFound = _FindValue( iEnd, &iLast );


    //
    // Find First values
    //
    // check for consecutive End-First values
    if ((_VSelRanges[iFirst].iEnd == iBegin - 1) ||
        (fBeginFound))
    {
        // extend iFirst
        fExtendFirst = TRUE;
        iRemoveStart = iFirst + 1;  
    }
    else
    {   
        // create one after the iFirst
        fExtendFirst = FALSE;
        iRemoveStart = iFirst + 2;
    }

    //
    // Find Last values
    //
    if (fEndFound)
    {
        // Use [iLast].iEnd value
        iRemoveFinish = iLast;
        iNewEnd = _VSelRanges[iLast].iEnd;

    }
    else
    {
        // check for consecutive First-End values
        if (_VSelRanges[iLast + 1].iBegin == iEnd + 1)
        {
            // Use [iLast + 1].iEnd value
            iNewEnd = _VSelRanges[iLast+1].iEnd;
            iRemoveFinish = iLast + 1;
        }
        else
        {
            // Use iEnd value
            iRemoveFinish = iLast;
            iNewEnd = iEnd;
        }
    }

    //
    // remove condenced items if needed
    //
    if (iRemoveStart <= iRemoveFinish)
    {
        LONG cChange;

        hres = _RemoveRanges(iRemoveStart, iRemoveFinish, &cChange );
        if (FAILED(hres))
            return hres;
        else
        {
            cSelected -= cChange;
        }
    }
                
    //
    // insert item and reset values as needed
    //          
    if (fExtendFirst)
    {
        cSelected += iNewEnd - _VSelRanges[iFirst].iEnd;
        _VSelRanges[iFirst].iEnd = iNewEnd;   
    }
    else
    {
        if (iRemoveStart > iRemoveFinish + 1)
        {
            cSelected += iEnd - iBegin + 1;
            // create one
            if (!_InsertRange(iFirst, iBegin, iNewEnd ))
            {
                hres = E_FAIL;
            }
        }       
        else
        {
            cSelected += iNewEnd - _VSelRanges[iFirst+1].iEnd;
            cSelected += _VSelRanges[iFirst+1].iBegin - iBegin;
            // no need to create one since the Removal would have left us one
            _VSelRanges[iFirst+1].iEnd = iNewEnd; 
            _VSelRanges[iFirst+1].iBegin = iBegin;
        }
    }
    
    _cIncluded += cSelected;
    return( hres );
}



//-------------------------------------------------------------------
//
// Function: SelRange_ExcludeRange
//
// Summary:
//      This function will exclude the range defined from the current
//  ranges, compacting and enlarging as needed.
//
// Arguments:
//      hselrange [in]  - Handle to the SelRange
//      iBegin [in]     - Begin of range to remove
//      iEnd [in]       - End of range to remove
//
// Return:
//      SELRANGE_ERROR if memory allocation error
//      the number actual items that changed state
//
// Notes:
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::ExcludeRange( LONG iBegin, LONG iEnd )
{
    LONG iFirst;   // index before or contains iBegin value
    LONG iLast;    // index before or contains iEnd value
    LONG iRemoveStart;  // start of ranges that need to be removed
    LONG iRemoveFinish; // end of ranges that need to be removed

    LONG iFirstNewEnd;  // calculate new end value as we go
    BOOL fBeginFound; // was the iBegin found in a range already
    BOOL fEndFound;   // was the iEnd found in a range already
    LONG cUnSelected = 0;
    HRESULT hres = S_OK;

    ASSERT( iEnd >= iBegin );
    ASSERT( iBegin >= SELRANGE_MINVALUE );
    ASSERT( iEnd <= SELRANGE_MAXVALUE );

    // find approximate locations
    fBeginFound = _FindValue( iBegin, &iFirst );
    fEndFound = _FindValue( iEnd, &iLast );

    //
    // Find First values
    //

    // start removal after first
    iRemoveStart = iFirst + 1;
    // save FirstEnd as we may need to modify it
    iFirstNewEnd = _VSelRanges[iFirst].iEnd;

    if (fBeginFound)
    {
        // check for complete removal of first
        //    (first is a single selection or match?)
        if (_VSelRanges[iFirst].iBegin == iBegin)
        {
            iRemoveStart = iFirst;  
        }
        else
        {
            // otherwise truncate iFirst
            iFirstNewEnd = iBegin - 1;
        }
    }
    
    //
    // Find Last values
    //
                
    // end removal on last
    iRemoveFinish = iLast;

    if (fEndFound)
    {
        // check for complete removal of last
        //   (first/last is a single selection or match?)
        if (_VSelRanges[iLast].iEnd != iEnd)
        {   
            if (iFirst == iLast)
            {
                // split
                if (!_InsertRange(iFirst, iEnd + 1, _VSelRanges[iFirst].iEnd ))
                {
                    return( E_FAIL );
                }
                cUnSelected -= _VSelRanges[iFirst].iEnd - iEnd;
            }
            else
            {
                // truncate Last
                iRemoveFinish = iLast - 1;
                cUnSelected += (iEnd + 1) - _VSelRanges[iLast].iBegin;
                _VSelRanges[iLast].iBegin = iEnd + 1;
            }
        }
    }

    // Now set the new end, since Last code may have needed the original values
    cUnSelected -= iFirstNewEnd - _VSelRanges[iFirst].iEnd;
    _VSelRanges[iFirst].iEnd = iFirstNewEnd;


    //
    // remove items if needed
    //
    if (iRemoveStart <= iRemoveFinish)
    {
        LONG cChange;

        if (SUCCEEDED(hres = _RemoveRanges(iRemoveStart, iRemoveFinish, &cChange )))
            cUnSelected += cChange;
    }

    _cIncluded -= cUnSelected;
    return( hres );
}

//-------------------------------------------------------------------
//
// Function: SelRange_Clear
//
// Summary:
//      This function will remove all ranges within the SelRange object.
//
// Arguments:
//      hselrange [in]  - the hselrange object to clear
//
// Return:  FALSE if failed.
//
// Notes:
//      This function may return FALSE on memory allocation problems, but
//  will leave the SelRange object in the last state before this call.  
//
// History:
//      14-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::Clear()
{
    PSELRANGEITEM pNewItems;
    HRESULT hres = S_OK;

    pNewItems = (PSELRANGEITEM) GlobalAlloc( GPTR,
                                       sizeof( SELRANGEITEM ) * MINCOUNT );
    if (NULL != pNewItems)
    {
        GlobalFree( _VSelRanges );
        _VSelRanges = pNewItems;

        _InitNew();
    }
    else
    {
        hres = E_FAIL;
    }
    return( hres );
}

//-------------------------------------------------------------------
//
// Function: SelRange_IsSelected
//
// Summary:
//      This function will return if the value iItem is within a
//  selected range.
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//      iItem [in]      - value to check for
//
// Return:  TRUE if selected, FALSE if not.
//
// Notes:
//
// History:
//      17-Oct-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::IsSelected( LONG iItem )
{   
    LONG iFirst;

    ASSERT( iItem >= 0 );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    return( _FindValue( iItem, &iFirst ) ? S_OK : S_FALSE);
}


//-------------------------------------------------------------------
//
// Function: SelRange_IsEmpty
//
// Summary:
//      This function will return TRUE if the range is empty
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//
// Return:  TRUE if empty
//
// Notes:
//
// History:
//
//-------------------------------------------------------------------
HRESULT CLVRange::IsEmpty()
{   
    return (_cSelRanges == COUNT_SELRANGES_NONE)? S_OK : S_FALSE;
}

HRESULT CLVRange::CountIncluded(LONG *pcIncluded)
{
    *pcIncluded = _cIncluded;
    return S_OK;
}


//-------------------------------------------------------------------
//
// Function: SelRange_InsertItem
//
// Summary:
//      This function will insert a unselected item at the location,
//      which will push all selections up one index.
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//      iItem [in]      - value to check for
//
// Return:
//      False on memory allocation error
//      otherwise TRUE
//
// Notes:
//
// History:
//      20-Dec-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::InsertItem( LONG iItem )
{
    LONG iFirst;
    LONG i;
    LONG iBegin;
    LONG iEnd;

    ASSERT( iItem >= 0 );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    if (_FindValue( iItem, &iFirst ) )
    {
        // split it
        if ( _VSelRanges[iFirst].iBegin == iItem )
        {
            // but don't split if starts with value
            iFirst--;
        }
        else
        {
            if (!_InsertRange(iFirst, iItem, _VSelRanges[iFirst].iEnd ))
            {
                return( E_FAIL );
            }
            _VSelRanges[iFirst].iEnd = iItem - 1;
        }
    }

    // now walk all ranges past iFirst, incrementing all values by one
    for (i = _cSelRanges-2; i > iFirst; i--)
    {
        iBegin = _VSelRanges[i].iBegin;
        iEnd = _VSelRanges[i].iEnd;

        iBegin = min( SELRANGE_MAXVALUE, iBegin + 1 );
        iEnd = min( SELRANGE_MAXVALUE, iEnd + 1 );

        _VSelRanges[i].iBegin = iBegin;
        _VSelRanges[i].iEnd = iEnd;
    }
    return( S_OK );
}

//-------------------------------------------------------------------
//
// Function: SelRange_RemoveItem
//
// Summary:
//      This function will remove an item at the location,
//      which will pull all selections down one index.
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//      iItem [in]      - value to check for
//      pfWasSelected [out] - was the removed item selected before the removal
//
// Return:
//      TRUE if the item was removed
//      FALSE if the an error happend
//
// Notes:
//
// History:
//      20-Dec-94   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::RemoveItem(LONG iItem )
{
    LONG iFirst;
    LONG i;
    LONG iBegin;
    LONG iEnd;
    HRESULT hres = S_OK;

    ASSERT( iItem >= SELRANGE_MINVALUE );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    if (_FindValue( iItem, &iFirst ) )
    {
        // item within, change the end value
        iEnd = _VSelRanges[iFirst].iEnd;
        iEnd = min( SELRANGE_MAXVALUE, iEnd - 1 );
        _VSelRanges[iFirst].iEnd = iEnd;

        _cIncluded--;
    }
    else
    {
        // check for merge situation
        if ((iFirst < _cSelRanges - 1) &&
            (_VSelRanges[iFirst].iEnd == iItem - 1) &&
            (_VSelRanges[iFirst+1].iBegin == iItem + 1))
        {
            _VSelRanges[iFirst].iEnd =
                    _VSelRanges[iFirst + 1].iEnd - 1;
            if (FAILED(hres = _RemoveRanges(iFirst + 1, iFirst + 1, NULL )))
                return( hres );
        }
    }

    // now walk all ranges past iFirst, decrementing all values by one
    for (i = _cSelRanges-2; i > iFirst; i--)
    {
        iBegin = _VSelRanges[i].iBegin;
        iEnd = _VSelRanges[i].iEnd;

        iBegin = min( SELRANGE_MAXVALUE, iBegin - 1 );
        iEnd = min( SELRANGE_MAXVALUE, iEnd - 1 );

        _VSelRanges[i].iBegin = iBegin;
        _VSelRanges[i].iEnd = iEnd;
    }
    return( hres );
}

//-------------------------------------------------------------------
//
// Function: NextSelected
//
// Summary:
//      This function will start with given item and find the next
//      item that is selected.  If the given item is selected, that
//      item number will be returned.
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//      iItem [in]      - value to start check at
//
// Return:
//      -1 if none found, otherwise the item
//
// Notes:
//
// History:
//      04-Jan-95   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::NextSelected( LONG iItem, LONG *piItem )
{
    LONG i;

    ASSERT( iItem >= SELRANGE_MINVALUE );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    if (!_FindValue( iItem, &i ) )
    {
        i++;
        if (i < _cSelRanges-1)
        {
            iItem = _VSelRanges[i].iBegin;
        }
        else
        {
            iItem = -1;
        }
    }

    ASSERT( iItem >= -1 );
    ASSERT( iItem <= SELRANGE_MAXVALUE );
    *piItem = iItem;
    return S_OK;
}

//-------------------------------------------------------------------
//
// Function: NextUnSelected
//
// Summary:
//      This function will start with given item and find the next
//      item that is not selected.  If the given item is not selected, that
//      item number will be returned.
//
// Arguments:
//      hselrange [in]  - the hselrange object to use
//      iItem [in]      - value to start check at
//
// Return:
//      -1 if none found, otherwise the item
//
// Notes:
//
// History:
//      04-Jan-95   MikeMi  Created
//
//-------------------------------------------------------------------

HRESULT CLVRange::NextUnSelected( LONG iItem, LONG *piItem )
{
    LONG i;

    ASSERT( iItem >= SELRANGE_MINVALUE );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    if (_FindValue( iItem, &i ) )
    {
        if (i < _cSelRanges-1)
        {
            iItem = _VSelRanges[i].iEnd + 1;
            if (iItem > SELRANGE_MAXVALUE)
            {
                iItem = -1;
            }
        }
        else
        {
            iItem = -1;
        }
    }

    ASSERT( iItem >= -1 );
    ASSERT( iItem <= SELRANGE_MAXVALUE );

    *piItem = iItem;
    return S_OK;
}

//-------------------------------------------------------------------
//
// Function: InvertRange
//
// Summary:
//      This function will invert the range defined from the current
//  ranges, compacting and enlarging as needed.
//
// Arguments:
//      iBegin [in]     - Begin of range to invert
//      iEnd [in]       - End of range to invert
//
// Return:
//      SELRANGE_ERROR on memory error
//      The difference in items selected from previous to current.
//      negative values means less items are selected in that range now.
//
// Notes:
//
// History:
//      13-Dec-95   MikeMi  Created
//
//-------------------------------------------------------------------

LONG CLVRange::InvertRange( LONG iBegin, LONG iEnd )
{
    LONG iFirst;   // index before or contains iBegin value
    BOOL fSelect;  // are we selecting or unselecting
    LONG iTempE;
    LONG iTempB;
    HRESULT hres = S_OK;

    ASSERT( iEnd >= iBegin );
    ASSERT( iBegin >= SELRANGE_MINVALUE );
    ASSERT( iEnd <= SELRANGE_MAXVALUE );

    // find if first is selected or not
    fSelect = !_FindValue( iBegin, &iFirst );
    
    iTempE = iBegin - 1;

    do
    {
        iTempB = iTempE + 1;

        if (fSelect)
            NextSelected( iTempB, &iTempE );
        else
            NextUnSelected( iTempB, &iTempE );

        if (-1 == iTempE)
        {
            iTempE = SELRANGE_MAXVALUE;
        }
        else
        {
            iTempE--;
        }

        iTempE = min( iTempE, iEnd );

        if (fSelect)
        {
            if (FAILED(hres = IncludeRange( iTempB, iTempE )))
            {
                return( hres );
            }
        }
        else
        {
            if (FAILED(hres = ExcludeRange( iTempB, iTempE )))
            {
                return( hres );
            }
        }

        fSelect = !fSelect;
    } while (iTempE < iEnd );

    return( hres );
}
