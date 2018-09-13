/*
 *  @doc INTERNAL
 *
 *  @module _RUNPTR.H -- Text run and run pointer class defintion |
 *  
 *  Original Author:    <nl>
 *      Christian Fortini
 *
 *  History: <nl>
 *      6/25/95 alexgo  Commenting and Cleanup
 */

#ifndef I__RUNPTR_H_
#define I__RUNPTR_H_
#pragma INCMSG("--- Beg '_runptr.h'")

#ifndef X_ARRAY_HXX_
#define X_ARRAY_HXX_
#include "array.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

MtExtern(CRunArray)
MtExtern(CRunArray_pv)

class CRunArray : public CArray<CTxtRun>
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRunArray))
    CRunArray() : CArray<CTxtRun>(Mt(CRunArray_pv)) {};
};

/*
 *  CRunPtrBase
 *
 *  @class  Base run pointer functionality.  Keeps a position within an array
 *      of text runs.
 *
 *  @devnote    Run pointers go through three different possible states :
 *
 *  NULL:   there is no data and no array (frequently a startup condition) <nl>
 *          <mf CRunPtrBsae::SetRunArray> will transition from this state to 
 *          to the Empty state.  It is typically the derived class'
 *          to define when that method should be called.
 *
 *          <md CRunPtrBase::_prgRun> == NULL <nl>
 *          <md CRunPtrBase::_iRun> == 0 <nl>
 *          <md CRunPtrBase::_ich> == 0 <nl>
 *
 *  Empty:  an array class exists, but there is no data (can happen if all 
 *          of the elements in the array are deleted). <nl>
 *          <md CRunPtrBase::_prgRun> != NULL <nl>
 *          <md CRunPtrBase::_iRun> == 0 <nl>
 *          <md CRunPtrBase::_ich> <gt>= 0 <nl>
 *          <md CRunPtrBase::_prgRun>-<gt>Elem[0] == NULL <nl>
 *
 *  Normal: the array class exists and has data <nl>
 *          <md CRunPtrBase::_prgRun> != NULL <nl>
 *          <md CRunPtrBase::_iRun> >= 0 <nl>
 *          <md CRunPtrBase::_ich> >= 0 <nl>
 *          <md CRunPtrBase::_prgRun>-<gt>Elem[<md CRunPtrBase::_iRun>] 
 *                  != NULL <nl>        
 *  
 *  Note that in order to support the empty and normal states, the actual 
 *  array element at <md CRunPtrBase::_iRun> must be explicitly fetched in
 *  any method that may need it.
 *
 *  Currently, there is no way to transition to the NULL state from any of
 *  the other states.  If we needed to, we could support that by explicitly 
 *  fetching the array from the document on demand.
 *
 *  Note that only <md CRunPtrBase::_iRun> is kept.  We could also keep 
 *  a pointer to the actual run (i.e. _pRun).  Earlier versions of this
 *  engine did in fact do this.  I've opted to not do this for several
 *  reasons: <nl>
 *      1. _pRun is *always* available by calling Elem(_iRun).
 *      Therefore, there is nominally no need to keep both _iRun and _pRun.<nl>
 *      2. Run pointers are typically used to either just move around
 *      and then fetch data or move and fetch data every time (like during 
 *      a measuring loop).  In the former case, there is no need to always
 *      bind _pRun; you can just do it on demand.  In the latter case, the
 *      two models are equivalent.  
 *
 */

class CRunPtrBase
{
private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:
    
    WHEN_DBG( BOOL Invariant ( ) const; )
    WHEN_DBG( long GetTotalCch ( ) const; )

    CRunPtrBase ( CRunArray * prgRun )
    {
        _prgRun = prgRun; 
        __iRun = 0; 
        __ich = 0; 
    }
    
    CRunPtrBase ( CRunPtrBase & rp )
    {
        *this = rp;
    }

    void SetRunArray ( CRunArray * prgRun )
    {
        _prgRun = prgRun;
    }
                                    
    BOOL SetRun( long iRun, long ich );

    BOOL NextRun ( );
    BOOL PrevRun ( );
    
    CTxtRun * GetRunRel ( long cRun ) const
    {
        Assert( _prgRun );
        
        return _prgRun->Elem( GetIRun() + cRun );
    }
    
    long NumRuns ( ) const
    {
        return _prgRun->Count();
    }

    BOOL OnLastRun ( )
    {
        Assert( GetIRun() < NumRuns() );
        
        return NumRuns() == 0 || GetIRun() == NumRuns() - 1;
    }

    BOOL OnFirstRun ( )
    {
        Assert( GetIRun() < NumRuns() );
        
        return GetIRun() == 0;
    }

    DWORD   BindToCp ( DWORD cp );
    
    DWORD   GetCp ( ) const;
    
    long    AdvanceCp ( long cch );
    
    BOOL    AdjustBackward ( );

    BOOL    AdjustForward ( );

    long    GetCchRemaining ( ) const { return GetRunRel( 0 )->_cch - GetIch(); }

    long    GetCchRun() { return GetRunRel( 0 )->_cch; }

    long GetIRun ( ) const { return __iRun; }
    
    void SetIRun ( long iRunNew )
    {
        __iRun = iRunNew;
    }

    long GetIch ( ) const { return __ich; }
    
    void SetIch ( long ichNew )
    {
        __ich = ichNew;
    }

    BOOL IsValid() const
    {
        return __iRun < long( _prgRun->Count() );
    }

protected:
    
    CRunArray * _prgRun;

private:
    
    //
    // WARNING: Do NOT access these members directly. Use accessors.
    //
    
    long __iRun;
    long __ich;
};


/*
 *  CRunPtr (template)
 *
 *  @class  a template over CRunPtrBase allowing for type-safe versions of
 *      run pointers
 * 
 *  @tcarg  class   | CElem | run array class to be used
 *
 *  @base   public | CRunPtrBase
 */

// BUGBUG: Make CRunPtrBase private, and provide types accessors to base
//         functionality

template <class CRunElem>
class CRunPtr : public CRunPtrBase
{
private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:
    
    CRunPtr() : CRunPtrBase( 0 ) { }
    
    CRunPtr ( CRunArray * prgRun ) : CRunPtrBase ( prgRun ) { }
    
    CRunPtr ( CRunPtrBase & rp ) : CRunPtrBase ( rp ) { }

    // Array management 
                                        
    CRunElem * Add (DWORD cRun, DWORD *pielIns)    //@cmember Add <p cRun>     
    {                                           // elements at end of array
        Assert(_prgRun != NULL);
        return (CRunElem *)_prgRun->Add(cRun, pielIns);
    }
                                        
    CRunElem * Insert (DWORD cRun)                 //@cmember Insert <p cRun>
    {                                           // elements at current pos
        Assert(_prgRun != NULL);
        return (CRunElem *)_prgRun->Insert(GetIRun(), cRun);
    }
                                        
    CRunElem * InsertAtRel (long dRun, DWORD cRun)
    {                                           // elements at current pos
        Assert(_prgRun != NULL);
        return (CRunElem *)_prgRun->Insert(GetIRun() + dRun, cRun);
    }

    void RemoveRel (LONG cRun, ArrayFlag flag)  //@cmember Remove <p cRun>
    {                                           // elements at current pos
         Assert(_prgRun != NULL);
         _prgRun->Remove (GetIRun(), cRun, flag);
    }
    
    void RemoveAbs ( long iRun, LONG cRun, ArrayFlag flag )
    {
         Assert(_prgRun != NULL);
         _prgRun->Remove( iRun, cRun, flag );
    } 
                                        //@cmember  Replace <p cRun> elements
                                        // at current position with those
                                        // from <p parRun>
    BOOL Replace (LONG cRun, CRunArray *parRun)
    {
        Assert(_prgRun != NULL);
        return _prgRun->Replace(GetIRun(), cRun, parRun);
    }

    CRunElem * GetRunAbs ( LONG iRun ) const
    {
        Assert( _prgRun != NULL );
        return (CRunElem *) _prgRun->Elem( iRun );
    }

    CRunElem * GetRunRel ( LONG dRun ) const
    {
        return (CRunElem *) CRunPtrBase::GetRunRel( dRun );
    }

    CRunElem * GetCurrRun ( void ) const
    {
        return GetRunAbs( GetIRun() );
    }
    
    CRunElem * GetPrevRun ( )
    {
        Assert( GetIRun() == 0 || (GetIRun() >= 0 && GetIRun() < NumRuns()) );

        if (GetIRun() == 0)
            return NULL;

        return GetRunAbs(GetIRun() - 1);
    }
    
    CRunElem * GetNextRun ( )
    {
        long nRuns = NumRuns();
        long iRun = GetIRun() + 1;

        Assert( GetIRun() == 0 || (GetIRun() >= 0 && GetIRun() < nRuns) );

        if (nRuns == 0 || iRun == nRuns)
            return NULL;

        return GetRunAbs( iRun );
    }
};

#pragma INCMSG("--- End '_runptr.h'")
#else
#pragma INCMSG("*** Dup '_runptr.h'")
#endif
