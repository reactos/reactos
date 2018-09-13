//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       regionstack.hxx
//
//  Contents:   Store regions associated with particular display nodes.
//
//----------------------------------------------------------------------------

#ifndef I_REGIONSTACK_HXX_
#define I_REGIONSTACK_HXX_
#pragma INCMSG("--- Beg 'regionstack.hxx'")

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CRegionStack
//
//  Synopsis:   Store regions associated with particular keys.
//
//----------------------------------------------------------------------------

class CRegionStack
{
public:
                    CRegionStack()
                            {_stackIndex = _stackMax = 0;}
                    CRegionStack(const CRegionStack& rgnStack, const CRect& rcBand);
#if DBG==1
                    ~CRegionStack();
#else
                    ~CRegionStack() {}
#endif
                    
    void            DeleteStack()
                            {while (_stackMax > 0)
                                delete _stack[--_stackMax]._prgn;}
                            
    void            DeleteStack(CRegion* prgnDontDelete)
                            {int last = 0;
                            if (_stackMax > 0 && _stack[0]._prgn == prgnDontDelete)
                                last = 1;
                            while (_stackMax > last)
                                delete _stack[--_stackMax]._prgn;
                            _stackMax = 0;}
                            
    BOOL            MoreToPop() const {return _stackIndex > 0;}
                    // leave room for root to add one more region
    BOOL            IsFull() const {return _stackIndex >= REGIONSTACKSIZE-1;}
    BOOL            IsFullForRoot() const {return _stackIndex >= REGIONSTACKSIZE;}
    void            Restore() {_stackIndex = _stackMax;}
    
    void            PushRegion(
                        const CRegion* prgn,
                        void* key,
                        const RECT& rcBounds)
                            {Assert(!IsFull() && _stackIndex == _stackMax);
                            stackElement* p = &_stack[_stackIndex];
                            p->_prgn = prgn;
                            p->_key = key;
                            p->_rcBounds = rcBounds;
                            _stackMax = ++_stackIndex;}
    
    void            PushRegionForRoot(
                        const CRegion* prgn,
                        void* key,
                        const RECT& rcBounds)
                            {Assert(!IsFullForRoot() && _stackIndex == _stackMax);
                            stackElement* p = &_stack[_stackIndex];
                            p->_prgn = prgn;
                            p->_key = key;
                            p->_rcBounds = rcBounds;
                            _stackMax = ++_stackIndex;}
    
    BOOL            PopRegionForKey(void* key, CRegion** pprgn)
                            {if (_stackIndex == 0 ||
                                 _stack[_stackIndex-1]._key != key)
                                return FALSE;
                            *pprgn = (CRegion*) _stack[--_stackIndex]._prgn;
                            return TRUE;}
    
    void*           PopFirstRegion(CRegion** pprgn)
                            {Assert(_stackIndex > 0);
                            stackElement* p = &_stack[--_stackIndex];
                            *pprgn = (CRegion*) p->_prgn;
                            return p->_key;}
    
    void*           PopKey()
                            {return (_stackIndex > 0)
                                ? _stack[--_stackIndex]._key
                                : NULL;}
    
    void*           GetKey()
                            {return (_stackIndex > 0)
                                ? _stack[_stackIndex-1]._key
                                : NULL;}
    
    CRegion*        RestorePreviousRegion()
                            {Assert(_stackIndex > 0 && _stackIndex == _stackMax);
                            _stackMax--;
                            return (CRegion*) _stack[--_stackIndex]._prgn;}
    
private:
    enum            {REGIONSTACKSIZE = 32};
    struct stackElement
    {
        const CRegion*      _prgn;
        void*               _key;
        RECT                _rcBounds;
    };
               
    int             _stackIndex;
    int             _stackMax;
    stackElement    _stack[REGIONSTACKSIZE];
};


#pragma INCMSG("--- End 'regionstack.hxx'")
#else
#pragma INCMSG("*** Dup 'regionstack.hxx'")
#endif

