#if !defined __DLINK_HXX__
#define __DLINK_HXX__
//+---------------------------------------------------------------------------
//
//  File:   DLINK.HXX
//
//  Contents:   Parametrized doubly linked list and iterators
//
//  History:    15-Jun-92  BartoszM Created.
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CDoubleLink
//
//  Purpose:    Linked element 
//
//  History:    15-Jun-92   BartoszM    Created.
//
//  Notes:      Use as base for your class. No need to override anything.
//
//              class CFoo: public CDoubleLink
//              {
//                  // your data and code goes here
//              };
//
//----------------------------------------------------------------------------

class CDoubleLink
{
public:
    
    CDoubleLink* Next() { return _next; }

    CDoubleLink* Prev() { return _prev; }

    void Close() { _next = this; _prev = this; }

    BOOL IsSingle() const { return _next == this; }
    
    void Unlink()
    {
        _next->_prev = _prev;
        _prev->_next = _next;
    }

    void InsertBefore ( CDoubleLink* pAfter )
    {
        CDoubleLink* pBefore = pAfter->_prev;
        _next = pAfter;
        _prev = pBefore;
        pAfter->_prev  = this;
        pBefore->_next = this;
    }
    
    void InsertAfter ( CDoubleLink* pBefore )
    {
        CDoubleLink* pAfter = pBefore->_next;
        _next = pAfter;
        _prev = pBefore;
        pAfter->_prev  = this;
        pBefore->_next = this;
    }
    
protected:

    CDoubleLink* _next;
    CDoubleLink* _prev;
};
#endif
