/*  @doc INTERNAL
 *
 *  @module _ARRAY.H  Generic Array Class |
 *  
 *  This module declares a generic array class for constant sized
 *  elements (although the elements themselves may be of any size).
 *
 *  Original Author: <nl>
 *      Christian Fortini
 *
 *  History: <nl>
 *      6/25/95 alexgo  Cleanup and Commenting
 */

#ifndef I_ARRAY_HXX_
#define I_ARRAY_HXX_
#pragma INCMSG("--- Beg 'array.hxx'")

class CFormatRunPtr;

/*
 *  ArrayFlag
 *
 *  @enum   Defines flags used with the array class
 */
enum tagArrayFlag
{
    AF_KEEPMEM      = 1,    //@emem Don't delete any memory 
    AF_DELETEMEM    = 2    //@emem Delete as much memory as possible
};

//@type ArrayFlag | flags controlling the usage of generic arrays 
//(and specifically how memory is handled)
typedef enum tagArrayFlag ArrayFlag;


/*
 *  CArrayBase
 *  
 *  @class  The CArrayBase class implements a generic array class.  It should
 *  never be used directly; use the type-safe template, <c CArray>, instead.
 *
 *  @devnote There are exactly two legal states for an array: empty or not.
 *  If an array is empty, the following must be true:
 *
 *      <md CArrayBase::_prgel> == NULL; <nl>
 *      <md CArrayBase::_cel> == 0; <nl>
 *      <md CArrayBase::_celMax> == 0;
 *
 *  Otherwise, the following must be true:
 *
 *      <md CArrayBase::_prgel> != NULL; <nl>
 *      <md CArrayBase::_cel> <lt>= <md CArrayBase::_celMax>; <nl>
 *      <md CArrayBase::_celMax> <gt> 0;
 *
 *  
 *  An array starts empty, transitions to not-empty as elements are inserted
 *  and will transistion back to empty if all of the array elements are 
 *  removed.
 *
 */

class CArrayBase
{
//@access Public Methods
public:

#if DBG == 1
    BOOL Invariant( void ) const;   //@cmember Validates state consistentency
#endif
    
    CArrayBase ( DWORD cbElem , PERFMETERTAG mt)
    {
        _prgel = NULL;
        _cel = 0;
        _celMax = 0;
        _cbElem = cbElem;

    #ifdef PERFMETER
        _mt = mt;
    #endif
    }
    
  ~ CArrayBase ( )
    {
        Clear(AF_DELETEMEM);
    }

    void    Clear (ArrayFlag flag); //@cmember Delete all runs in array
    
    DWORD   Count () const          //@cmember Get count of runs in array
    {
        return _cel;
    }

    void CopyFrom(const CArrayBase & other) //copy operator
    {
        if (_cel)
        {
            Clear(AF_KEEPMEM);
        }
        ArAdd(other._celMax, NULL);
        CopyMemory(_prgel, other._prgel, other._cbElem * other._celMax);
    }
        
    /* 
     *  CArrayBase::Elem
     *
     *  @mfunc  Returns a pointer to the element indexed by <p iel>
     *
     *  @rdesc  A pointer to the element indexed by <p iel>.  This pointer may
     *  be cast to a pointer of the appropriate element type.
     */
    
    void* Elem( DWORD iel ) const
    {
        void * pv;

        // NOTE(sujalp): We want the following behaviour: that is if there are
        // no elements in the array (_cel == 0) and we ask for the 0th element
        // then we *want* to return NULL rather than assert. This behaviour is
        // used in multiple places.
        AssertSz(iel == 0 || (iel > 0 && iel < _cel),
            "CArrayBase::Elem() - Index out of range");

        // BUGBUG(sujalp): Originally the condition to check in the if was
        // (!_cel). This is fine for debug builds. However, in a ship build
        // we would still return a seemingly valid pointer if iel was off the
        // right edge of the array. Checking for the right edge and returning
        // NULL, allows us to debug stress bugs easily where we can be sure
        // that iel was off the right edge.
        if( iel < 0 || iel >= _cel )
        {
            pv = NULL;
        }
        else
        {
            pv = _prgel + iel * _cbElem;
        }
        return pv;
    }                                
    
                                    //@cmember Remove <p celFree> runs from
                                    // array, starting at run <p ielFirst>
    void    Remove(DWORD ielFirst, LONG celFree, ArrayFlag flag);

                                    //@cmember Replace runs <p iel> through
                                    // <p iel+cel-1> with runs from <p par>
    BOOL    Replace (DWORD iel, LONG cel, CArrayBase *par);
    
    LONG    Size () const           //@cmember Get size of a run            
    {
        return _cbElem;
    }

//@access Protected Methods
protected:
    void*   ArAdd (DWORD cel, DWORD *pielIns);  //@cmember Add <p cel> runs
    void*   ArInsert(DWORD iel, DWORD celIns);  //@cmember Insert <p celIns>
                                                // runs

//@access Protected Data
protected:

    char *  _prgel;     //@cmember Pointer to actual array data
    DWORD   _cel;       //@cmember Count of used entries in array
    DWORD   _celMax;    //@cmember Count of allocated entries in array
    DWORD   _cbElem;    //@cmember Byte count of an individual array element

#ifdef PERFMETER
    PERFMETERTAG _mt;
#endif

};

/*
 *  CArray
 *
 *  @class
 *      An inline template class providing type-safe access to CArrayBase
 *
 *  @tcarg class | ELEM | the class or struct to be used as array elements
 */

template <class ELEM> 
class CArray : public CArrayBase
{
//@access Public Methods
public:

    WHEN_DBG( ELEM * const & _rpael; )
    
    CArray (PERFMETERTAG mt)
        : CArrayBase(sizeof(ELEM), mt)
#if DBG == 1
            , _rpael( (ELEM *)_prgel )
#endif
    {}

    ELEM * Elem ( DWORD iel ) const
    {
        return (ELEM *) CArrayBase::Elem(iel);
    }

    ELEM & GetAt ( DWORD iel ) const
    {
        return *(ELEM *)CArrayBase::Elem(iel);
    }
    
    ELEM*   Add (DWORD cel, DWORD *pielIns) //@cmember Adds <p cel> elements                        
    {                                       // to end of array
        return (ELEM*)ArAdd(cel, pielIns);
    }

    ELEM*   Insert (LONG iel, LONG celIns)  //@cmember Insert <p celIns>
    {                                       // elements at index <p iel>
        return (ELEM*)ArInsert(iel, celIns);
    }
};

#pragma INCMSG("--- End 'array.hxx'")
#else
#pragma INCMSG("*** Dup 'array.hxx'")
#endif
