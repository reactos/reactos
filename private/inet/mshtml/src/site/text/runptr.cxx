/*
 *  @doc    INTERNAL
 *
 *  @module RUNPTR.C -- Text run and run pointer class |
 *  
 *  Original Authors: <nl>
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 *
 *  History: <nl>
 *      6/25/95 alexgo  Commented and Cleaned up.
 */

#include "headers.hxx"

#ifndef X__RUNPTR_H_
#define X__RUNPTR_H_
#include "_runptr.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

//
//  Invariant stuff
//
#define DEBUG_CLASSNAME CRunPtrBase

#include "_invar.h"

// ===========================  CRunPtrBase class  ==================================================

#if DBG==1
   
/*
 *  CRunPtrBase::Invariant()
 *
 *  @mfunc
 *      Debug-only function that validates the internal state consistency
 *      for CRunPtrBase
 *
 *  @rdesc
 *      TRUE always (failures assert)
 */
BOOL CRunPtrBase::Invariant() const
{
    CTxtRun *pRun;

    if( _prgRun == NULL )
    {
        Assert( GetIRun() == 0 );
        // we let ich zoom around a conceptual 
        // run so it can stay in sync with a text pointer
        Assert( GetIch() >= 0 );
        
        return TRUE;
    }

    pRun = _prgRun->Elem( GetIRun() );

    if( pRun == NULL )
    {
        Assert(GetIRun() == 0);
        // we let ich zoom around a conceptual 
        // run so it can stay in sync with a text pointer
        Assert( GetIch() >= 0 );
    }
    else
    {
        Assert( GetIRun() < NumRuns() );
        Assert( GetIch() <= long( pRun->_cch ) );
    }

    return TRUE;
}

/*
 *  CRunPtrBase::GetTotalCch()
 *
 *  @mfunc
 *      Calculate length of text by summing text runs accessible by this
 *      run ptr
 *
 *  @rdesc
 *      length of text so calculated, or -1 if failed
 */

long
CRunPtrBase::GetTotalCch ( ) const
{
    long iRun;
    long cchText = 0;

    AssertSz(_prgRun, "CTxtPtr::GetCch() - Invalid operation on single run CRunPtr");

    for( iRun = NumRuns() ; iRun && iRun-- ; )
        cchText += _prgRun->Elem(iRun)->_cch;

    return cchText;
}

#endif


/* 
 *  CRunPtrBase::SetRun(iRun, ich)
 *
 *  @mfunc
 *      Sets this run ptr to the given run.  If it does not
 *      exist, then we set ourselves to the closest valid run
 *
 *  @rdesc
 *      TRUE if moved to iRun
 */

BOOL
CRunPtrBase::SetRun( long iRun, long ich )
{
    BOOL      bRet = TRUE;
    long      nRuns = NumRuns();
    CTxtRun * pRun;
 
    if (!_prgRun)
        return FALSE;

    if (iRun >= nRuns)
    {
        bRet = FALSE;
        iRun = nRuns - 1;
    }
    
    if (iRun < 0)
    {
        bRet = FALSE;
        iRun = 0;
    }
    
    SetIRun( iRun );

    // Set the offset

    pRun = _prgRun->Elem( iRun );

    if (pRun)
        SetIch( min( ich, long( pRun->_cch ) ) );

    return bRet;
}
                                                
/*
 *  CRunPtrBase::NextRun()
 *
 *  @mfunc
 *      Change this RunPtr to that for the next text run
 *
 *  @rdesc
 *      TRUE if succeeds, i.e., target run exists
 */

BOOL
CRunPtrBase::NextRun()
{
    _TEST_INVARIANT_

    if (_prgRun)
    {    
        if (GetIRun() < NumRuns() - 1)
        {
            SetIRun( GetIRun() + 1 );
            SetIch( 0 );
            
            return TRUE;
        }
    }
    
    return FALSE;
}

/*
 *  CRunPtrBase::PrevRun()
 *
 *  @mfunc
 *      Change this RunPtr to that for the previous text run
 *
 *  @rdesc
 *      TRUE if succeeds, i.e., target run exists
 */

BOOL
CRunPtrBase::PrevRun()
{
    _TEST_INVARIANT_

    if (_prgRun)
    {
        SetIch( 0 );
        
        if (GetIRun() > 0)
        {
            SetIRun( GetIRun() - 1 );
            
            return TRUE;
        }
    }
    
    return FALSE;
}

/*
 *  CRunPtrBase::GetCp()
 *
 *  @mfunc
 *      Get cp of this RunPtr
 *
 *  @rdesc
 *      cp of this RunPtr
 *
 *  @devnote
 *      May be computationally expensive if there are many elements
 *      in the array (we have to run through them all to sum cch's.
 *      Used by TOM collections and Move commands, so needs to be fast.
 */

DWORD
CRunPtrBase::GetCp () const
{
    DWORD       cp   = GetIch();
    DWORD       iRun = GetIRun();
    CTxtRun *   pRun;

    _TEST_INVARIANT_

    if (_prgRun && iRun)
    {
        DWORD cb = _prgRun->Size();
        
        pRun = GetRunRel( -1 );
        
        while ( iRun-- )
        {
            Assert( pRun );
            
            cp += pRun->_cch;
            
            pRun = (CTxtRun *)((BYTE *)pRun - cb);
        }
    }
    
    return cp;
}

/*
 *  CRunPtrBase::BindToCp(cp)
 *
 *  @mfunc
 *      Set this RunPtr to correspond to a cp.
 *
 *  @rdesc
 *      the cp actually set to
 */

DWORD
CRunPtrBase::BindToCp( DWORD cp )
{
    SetIRun( 0 );
    SetIch( 0 );
    
    return DWORD( AdvanceCp( cp ) );
}

/*
 *  CRunPtrBase::AdvanceCp(cch)
 *
 *  @mfunc
 *      Advance this RunPtr by (signed) cch chars.  If it lands on the
 *      end of a run, it automatically goes to the start of the next run
 *      (if one exists). If this is violated, all hell will break loose,
 *      so change at your peril.
 *
 *  @rdesc
 *      Count of characters actually moved
 */
LONG CRunPtrBase::AdvanceCp(
    LONG cch)           //@parm signed count of chars to move this RunPtr by
{
    DWORD   cchSave = cch;
    WHEN_DBG( long lRunCount = NumRuns(); )

    AssertSz(GetIRun() == 0 || (GetIRun() > 0 && _prgRun), "Invalid CRunPtr");

    // No runs, so just update _ich as if there were a run
    if(!IsValid())
    {
        SetIch( GetIch() + cch );

        // We have to assume that caller ensures that cch isn't too large,
        // since a runless run ptr doesn't know the cch of the document.
        // But we can check for too-negative values of cch: 
        if( GetIch() < 0 )
        {
            cch = -cch + GetIch();              // Calculate actual cch moved
            SetIch( 0 );
        }
        return cch;
    }


    if(cch < 0)
    {
        while(cch < 0)
        {
            // this cast to LONG is OK, since -cch will be positive
            // (and we aren't likely to have 3 billion characters in 
            // a given run :-)
            if( -cch <= GetIch() )
            {
                SetIch( GetIch() + cch );
                cch = 0;
                break;
            }
            // otherwise, we need to go to the previous run

            cch += GetIch();                        // we moved by the number of
                                                // characters left in the 
                                                // current run.
            if (GetIRun() <= 0)                      // Already in first run
            {
                SetIRun( 0 );
                SetIch( 0 );                       // Move to run beginning
                break;
            }
            
            // move to previous run.
            
            Assert(_prgRun->Elem(GetIRun() - 1));

            SetIRun( GetIRun() - 1 );
            
            SetIch( _prgRun->Elem( GetIRun() )->_cch );
        }
    }
    else
    {
        while(cch > 0)                          // Move forward
        {
            const long cchRun = _prgRun->Elem(GetIRun())->_cch;

            SetIch( GetIch() + cch );

            if (GetIch() < cchRun)                   // Target is in this run
            {
                cch = 0;                        // Signal countdown completed
                break;                          // (if _ich = cchRun, go to
            }                                   //  next run)   

            cch = GetIch() - cchRun;                // Advance to next run

            if (GetIRun() + 1 >= NumRuns())
            {
                Assert(GetIRun() == NumRuns() - 1);
                Assert(_prgRun->Elem(GetIRun())->_cch == cchRun);
                SetIch( cchRun );
                break;
            }

            SetIRun( GetIRun() + 1 );
            
            SetIch( 0 );  // Start at beginning of new run.
        }
    }

#if DBG == 1
    // Guarantee that a) we're not changing the run count and
    //                b) We're not at the end of a non-empty run.
    // Much code depends on this behaviour of AdvanceCp().
    // - Arye
    Assert (lRunCount == NumRuns());
    Assert (GetCchRemaining() != 0 ||
            cch == 0 ||
            GetIRun() == NumRuns() - 1);
#endif


    // NB! we check the invariant at the end to handle the case where
    // we are updating the cp for a floating range (i.e., we know that
    // the cp is invalid, so we fix it up).  So we have to check for
    // validity *after* the fixup.
    _TEST_INVARIANT_

    return cchSave - cch;                       // Return TRUE if countdown
}                                               // completed

/*
 *  CRunPtrBase::AdjustBackward()
 *
 *  @mfunc
 *      If the cp for this run ptr is at the "boundary" or edge between two
 *      runs, then make sure this run ptr points to the end of the first run.
 *
 *  @comm
 *      This function does nothing unless this run ptr points to the beginning
 *      or the end of a run.  This function may be needed in those cases
 *      because a cp at the beginning of a run is identical to the cp for the
 *      end of the previous run (if it exists), i.e., such an "edge" cp is
 *      ambiguous, and you may need to be sure that this run ptr points to the
 *      end of the first run.
 *
 *      For example, consider a run that describes characters at cp's 0 to 10
 *      followed by a run that describes characters at cp's 11 through 12. For
 *      a cp of 11, it is possible for the run ptr to be either at the *end*
 *      of the first run or at the *beginning* of the second run.
 *
 *
 *  @rdesc  nothing
 */
BOOL CRunPtrBase::AdjustBackward()
{
    _TEST_INVARIANT_

    //
    // If not at beginning of run or can't go to rev run then can't
    // adjust forward
    //

    if (GetIch() || !PrevRun())
        return FALSE;

    SetIch( _prgRun->Elem(GetIRun())->_cch );
    
    return TRUE;
}

/*
 *  CRunPtrBase::AdjustForward()
 *
 *  @mfunc
 *      If the cp for this run ptr is at the "boundary" or edge between two
 *      runs, then make sure this run ptr points to the start of the second
 *      run.
 *
 *  @rdesc
 *      nothing
 *
 *  @xref
 *      <mf CRunPtrBase::AdjustBackward>
 */
BOOL CRunPtrBase::AdjustForward()
{
    _TEST_INVARIANT_

    if( !_prgRun )
        return FALSE;

    CTxtRun *pRun = _prgRun->Elem(GetIRun());

    Assert( pRun );

    Assert( GetIch() <= long( pRun->_cch ) );
        
    if (long( pRun->_cch ) == GetIch())
        return NextRun();
    
    return FALSE;
}

