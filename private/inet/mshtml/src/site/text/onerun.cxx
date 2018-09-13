/*
 *  @doc    INTERNAL
 *
 *  @module ONERUN.CXX -- line services one run interface.
 *
 *
 *  Owner: <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      5/6/97     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#if DBG == 1 || defined(DUMPRUNS)
long COneRunFreeList::s_NextSerialNumber = 0;
#endif

//-----------------------------------------------------------------------------
//
//  Function:   Deinit
//
//  Synopsis:   This function is called during the destruction of the
//              COneRunFreeList. It frees up any allocated COneRun objects.
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
COneRunFreeList::Deinit()
{
    COneRun *por;
    COneRun *porNext;

    por = _pHead;
    while (por)
    {
        Assert(por->_pCF == NULL);
        Assert(por->_pComplexRun == NULL);
        porNext = por->_pNext;
        delete por;
        por = porNext;
    }
}

//-----------------------------------------------------------------------------
//
//  Function: Clone
//
//  Synopsis: Clones a one run object making copies of all the stuff which
//            is needed.
//
//  Returns:  The cloned run -- either this or NULL depending upon if we could
//            allocate mem for subobjects.
//
//-----------------------------------------------------------------------------
COneRun *
COneRun::Clone(COneRun *porClone)
{
    COneRun *porRet = this;

    // Copy over all the memory
    memcpy (this, porClone, sizeof(COneRun));
    
    _pchBase = NULL;

    // If we have a pcf then it needs to be cloned too
    if (_fMustDeletePcf)
    {
        Assert(porClone->GetCF() != NULL);
        _pCF = new CCharFormat(*(porClone->GetCF()));
        if (!_pCF)
        {
            _fMustDeletePcf = FALSE;
            porRet = NULL;
            goto Cleanup;
        }
    }

    // Clone the CStr properly.
    memset (&_cstrRunChars, 0, sizeof(CStr));
    _cstrRunChars.Set(porClone->_cstrRunChars);

    // Cloned runs do not inherit their selection status from the guy
    // it clones from
    _fSelected = FALSE;
    
    // Setup the complex run related stuff properly
    porRet->_pComplexRun = NULL;
    porRet->_lsCharProps.fGlyphBased = FALSE;
    porRet->SetSidFromTreePos(porClone->_ptp);

    // Structure stuff should not copy
    porRet->_pNext = porRet->_pPrev = NULL;

Cleanup:
    return porRet;
}

//-----------------------------------------------------------------------------
//
//  Function: GetFreeOneRun
//
//  Synopsis: Gets a free one run object. If we already have some in the free
//            list, then we need to use those, else allocate off the heap.
//            If porClone is non-NULL then we will clone in that one run
//            into the newly allocated one.
//
//  Returns:  The run
//
//-----------------------------------------------------------------------------
COneRun *
COneRunFreeList::GetFreeOneRun(COneRun *porClone)
{
    COneRun *por = NULL;

    if (_pHead)
    {
        por = _pHead;
        _pHead = por->_pNext;
    }
    else
    {
        por = new COneRun();
    }
    if (por)
    {
        if (porClone)
        {
            if (por != por->Clone(porClone))
            {
                SpliceIn(por);
                por = NULL;
                goto Cleanup;
            }
        }
        else
        {
            memset(por, 0, sizeof(COneRun));
            por->_bConvertMode = CM_UNINITED;
        }
        
#if DBG == 1 || defined(DUMPRUNS)
        por->_nSerialNumber = s_NextSerialNumber++;
#endif
    }
Cleanup:    
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceIn
//
//  Synopsis:   Returns runs which are no longer needed back to the free list.
//              It also uninits all the runs.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunFreeList::SpliceIn(COneRun *pFirst)
{
    Assert(pFirst);
    COneRun *por = pFirst;
    COneRun *porLast = NULL;
    
    // Clear out the runs when they are put into the free list.
    while(por)
    {
        porLast = por;
        por->Deinit();

        // TODO(SujalP): por->_pNext is valid after Deinit!!!! Change this so
        // that this code does not depend on this.
        por = por->_pNext;
    }

    Assert(porLast);
    porLast->_pNext = _pHead;
    _pHead = pFirst;
}

//-----------------------------------------------------------------------------
//
//  Function:   Init
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::Init()
{
    _pHead = _pTail = NULL;
}

//-----------------------------------------------------------------------------
//
//  Function:   Deinit
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::Deinit()
{
    Assert(_pHead == NULL && _pTail == NULL);
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceOut
//
//  Synopsis:   Removes a chunk of runs from pFirst to pLast from the current list.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::SpliceOut(COneRun *pFirst, COneRun *pLast)
{
    Assert(pFirst && pLast);
    
    //
    // If the first node being removed is the head node then
    // let us deal with that
    //
    if (pFirst->_pPrev == NULL)
    {
        Assert(pFirst == _pHead);
        _pHead = pLast->_pNext;
    }
    else
    {
        pFirst->_pPrev->_pNext = pLast->_pNext;
    }

    //
    // If the last node being removed is the tail node then
    // let us deal with that
    //
    if (pLast->_pNext == NULL)
    {
        Assert(pLast == _pTail);
        _pTail = pFirst->_pPrev;
    }
    else
    {
        pLast->_pNext->_pPrev = pFirst->_pPrev;
    }

    //
    // Clear the next and prev pointers in the spliced out portion
    //
    pFirst->_pPrev = NULL;
    pLast->_pNext = NULL;
}

#if DBG==1
//-----------------------------------------------------------------------------
//
//  Function:   VerifyStuff
//
//  Synopsis:   A debug only function which verifies that the state of the
//              current onerun list is good.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::VerifyStuff(CLineServices *pLS)
{
    COneRun *por = _pHead;
    LONG lscp;
    LONG cchSynths;
    
    if (!por)
        goto Cleanup;
    lscp = por->_lscpBase;
    cchSynths = por->_chSynthsBefore;
    
    while (por)
    {
        Assert(por->_lscch == por->_lscchOriginal);
        Assert(por->_lscpBase == lscp);
        lscp += (por->IsAntiSyntheticRun() ? 0 : por->_lscch);
        
        Assert(cchSynths == por->_chSynthsBefore);
        cchSynths += por->IsSyntheticRun() ? por->_lscch : 0;
        cchSynths -= por->IsAntiSyntheticRun() ? por->_lscch : 0;

        // NestedElement should be true if NestedLayout is true.
        Assert(!por->_fCharsForNestedLayout || por->_fCharsForNestedElement);
        // Nestedlayout should be true if nestedrunowner is true.
        Assert(!por->_fCharsForNestedRunOwner || por->_fCharsForNestedLayout);
        
        por = por->_pNext;
    }

    por = _pTail;
    Assert(por);
    if (por->_fNotProcessedYet)
    {
        // Only one not processed yet run at the end
        por = por->_pPrev;
        while (por)
        {
            Assert(!por->_fNotProcessedYet);
            por = por->_pPrev;
        }
    }
    
Cleanup:
    return;
}
#endif

//-----------------------------------------------------------------------------
//
//  Function:   SpliceInAfterMe
//
//  Synopsis:   Adds pFirst into the currentlist after the position
//              indicated by pAfterMe. If pAfterMe is NULL its added to the head.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
#if DBG==1
COneRunCurrList::SpliceInAfterMe(CLineServices *pLS, COneRun *pAfterMe, COneRun *pFirst)
#else
COneRunCurrList::SpliceInAfterMe(COneRun *pAfterMe, COneRun *pFirst)
#endif
{
    COneRun **ppor;
#if DBG==1
    COneRun *pOldTail = _pTail;
#endif
    
    WHEN_DBG(VerifyStuff(pLS));
    
    ppor = (pAfterMe == NULL) ? &_pHead : &pAfterMe->_pNext;
    pFirst->_pNext = *ppor;
    *ppor = pFirst;
    pFirst->_pPrev = pAfterMe;
    
    COneRun *pBeforeMe = pFirst->_pNext;
    ppor = pBeforeMe == NULL ? &_pTail : &pBeforeMe->_pPrev;
    *ppor = pFirst;

#if DBG==1    
    {
        LONG chSynthsBefore = 0;

        if (pOldTail != NULL)
        {
            chSynthsBefore = pOldTail->_chSynthsBefore;

            chSynthsBefore += pOldTail->IsSyntheticRun()     ? pOldTail->_lscch : 0;
            chSynthsBefore -= pOldTail->IsAntiSyntheticRun() ? pOldTail->_lscch : 0;
        }
        
        Assert(chSynthsBefore == pFirst->_chSynthsBefore);
    }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:   SpliceInBeforeMe
//
//  Synopsis:   Adds the onerun identified by pFirst before the run
//              identified by pBeforeMe. If pBeforeMe is NULL then it
//              adds it at the tail.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
COneRunCurrList::SpliceInBeforeMe(COneRun *pBeforeMe, COneRun *pFirst)
{
    COneRun **ppor;

    ppor = pBeforeMe == NULL ? &_pTail : &pBeforeMe->_pPrev;
    pFirst->_pPrev = *ppor;
    *ppor = pFirst;
    pFirst->_pNext = pBeforeMe;

    COneRun *pAfterMe = pFirst->_pPrev;
    ppor = pAfterMe == NULL ? &_pHead : &pAfterMe->_pNext;
    *ppor = pFirst;
}

//-----------------------------------------------------------------------------
//
//  Function:   DiscardOneRuns
//
//  Synopsis:   Removes all the runs from the current list and gives them
//              back to the free list.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CLineServices::DiscardOneRuns()
{
    COneRun *pFirst = _listCurrent._pHead;
    COneRun *pTail  = _listCurrent._pTail;
    
    if (pFirst)
    {
        _listCurrent.SpliceOut(pFirst, pTail);
        _listFree.SpliceIn(pFirst);
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   AdvanceOneRun
//
//  Synopsis:   This is our primary function to get the next run at the
//              frontier.
//
//  Returns:    The one run.
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::AdvanceOneRun(LONG lscp)
{
    COneRun *por;
    BOOL fRet = FALSE;
    
    //
    // Get the memory for a one run
    //
    por = _listFree.GetFreeOneRun(NULL);
    if (!por)
        goto Cleanup;

    // Setup the lscp...
    por->_lscpBase = lscp;
    
    if (!_treeInfo._fHasNestedElement)
    {
        //
        // If we have run out of characters in the tree pos then we need
        // advance to the next tree pos.
        //
        if (!_treeInfo._cchRemainingInTreePos)
        {
            if (!_treeInfo.AdvanceTreePos())
                goto Cleanup;
            por->_fCannotMergeRuns = TRUE;
        }

        //
        // If we have run out of characters in the text then we need
        // to advance to the next text pos
        //
        if (!_treeInfo._cchValid)
        {
            if (!_treeInfo.AdvanceTxtPtr())
                goto Cleanup;
            por->_fCannotMergeRuns = TRUE;
        }

        Assert(_treeInfo._lscpFrontier == lscp);
    }

    //
    // If we have a nested run owner then the number of characters given to
    // the run are the number of chars in that nested run owner. Else the
    // number of chars is the minimum of the chars in the tree pos and that
    // in the text node.
    //
    if (!_treeInfo._fHasNestedElement)
    {
        por->_lscch = min(_treeInfo._cchRemainingInTreePos, _treeInfo._cchValid);
        BEGINSUPPRESSFORQUILL
        if (_lsMode == LSMODE_MEASURER)
        {
            por->_lscch = min(por->_lscch, MAX_CHARS_FETCHRUN_RETURNS);
        }
        else
        {
            // NB Additional 5 chars corresponds to a fudge factor.
            por->_lscch = min(por->_lscch, LONG(_pMeasurer->_li._cch + 5));
        }
        AssertSz(por->_lscch > 0, "Cannot measure 0 or -ve chars!");
        ENDSUPPRESSFORQUILL

        por->_pchBase = _treeInfo._pchFrontier;
    }
    else
    {
        //
        // NOTE(SujalP): The number of characters _returned_ to LS will not be
        // _lscch. We will catch this case in FetchRun and feed only a single
        // char with the pch pointing to a valid location so that LS does not
        // choke on it.
        //
        CElement *pElemNested = _treeInfo._ptpFrontier->Branch()->Element();

        por->_lscch = GetNestedElementCch(pElemNested);
        por->_fCannotMergeRuns = TRUE;
        por->_pchBase = NULL;
        por->_fCharsForNestedElement  = TRUE;
        por->_fCharsForNestedLayout   = _treeInfo._fHasNestedLayout;
        por->_fCharsForNestedRunOwner = _treeInfo._fHasNestedRunOwner;
    }

    //
    // Update all the other information in the one run
    //
    por->_chSynthsBefore = _treeInfo._chSynthsBefore;
    por->_lscchOriginal = por->_lscch;
    por->_pchBaseOriginal = por->_pchBase;
    por->_ptp = _treeInfo._ptpFrontier;
    por->SetSidFromTreePos(por->_ptp);
    
    por->_pCF = (CCharFormat *)_treeInfo._pCF;
    por->_fInnerCF = _treeInfo._fInnerCF;
    por->_pPF = _treeInfo._pPF;
    por->_fInnerPF = _treeInfo._fInnerPF;
    por->_pFF = _treeInfo._pFF;

    //
    // At last let us go and move out frontier
    //
    _treeInfo.AdvanceFrontier(por);
    
    fRet = TRUE;
    
Cleanup:
    if (!fRet && por)
    {
        delete por;
        por = NULL;
    }
    
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   CanMergeTwoRuns
//
//  Synopsis:   Decided if the 2 runs can be merged into one
//
//  Returns:    BOOL
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::CanMergeTwoRuns(COneRun *por1, COneRun *por2)
{
    BOOL fRet;
    
    if (   por1->_dwProps
        || por2->_dwProps
        || por1->_pCF != por2->_pCF
        || por1->_bConvertMode != por2->_bConvertMode
        || por1->_ccvBackColor.GetRawValue() != por2->_ccvBackColor.GetRawValue()
        || por1->_pComplexRun
        || por2->_pComplexRun
        || (por1->_pchBase + por1->_lscch != por2->_pchBase) // happens with passwords.
       )
        fRet = FALSE;
    else
        fRet = TRUE;
    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   MergeIfPossibleIntoCurrentList
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::MergeIfPossibleIntoCurrentList(COneRun *por)
{
    COneRun *pTail = _listCurrent._pTail;
    if (   pTail != NULL
        && CanMergeTwoRuns(pTail, por)
       )
    {
        Assert(pTail->_lscpBase + pTail->_lscch == por->_lscpBase);
        Assert(pTail->_pchBase  + pTail->_lscch == por->_pchBase);
        Assert(!pTail->_fNotProcessedYet); // Cannot merge into a run not yet processed
        
        pTail->_lscch += por->_lscch;
        pTail->_lscchOriginal += por->_lscchOriginal;
        
        //
        // Since we merged our por into the previous one, let us put the
        // present one back on the free list.
        //
        _listFree.SpliceIn(por);
        por = pTail;
    }
    else
    {
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, pTail, por);
#else
        _listCurrent.SpliceInAfterMe(pTail, por);
#endif
    }
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   SplitRun
//
//  Synopsis:   Splits a single run into 2 runs. The original run remains
//              por and the number of chars it has is cchSplitTill, while
//              the new split off run is the one which is returned and the
//              number of characters it has is cchOriginal-cchSplit.
//
//  Returns:    The 2nd run (which we got from cutting up por)
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::SplitRun(COneRun *por, LONG cchSplitTill)
{
    LONG cchDelta;
    
    //
    // Create an exact copy of the run
    //
    COneRun *porNew = _listFree.GetFreeOneRun(por);
    if (!porNew)
        goto Cleanup;
    por->_lscch = cchSplitTill;
    cchDelta = por->_lscchOriginal - por->_lscch;
    por->_lscchOriginal = por->_lscch;
    Assert(por->_lscch);
    
    //
    // Then setup the second run so that it can be spliced in properly
    //
    porNew->_pPrev = porNew->_pNext = NULL;
    porNew->_lscpBase += por->_lscch;
    porNew->_lscch = cchDelta;
    porNew->_lscchOriginal = porNew->_lscch;
    porNew->_pchBase = por->_pchBaseOriginal + cchSplitTill;
    porNew->_pchBaseOriginal = porNew->_pchBase;
    porNew->_fGlean = TRUE;
    porNew->_fNotProcessedYet = TRUE;
    Assert(porNew->_lscch);

Cleanup:
    return porNew;
}


//-----------------------------------------------------------------------------
//
//  Function:   AttachOneRunToCurrentList
//
//  Note: We always return the pointer to the run which is contains the
//  lscp for por. Consider the following cases:
//  1) No splitting:
//          If merged then return the ptr of the run we merged por into
//          If not merged then return por itself
//  2) Splitting:
//          Split into por and porNew
//          If por is merged then return ptr of the run we merged por into
//          If not morged then return por itself
//          Just attach/merge porNew
//
//  Returns:    The attached/merged-into run.
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::AttachOneRunToCurrentList(COneRun *por)
{
    COneRun *porRet;

    Assert(por);
    Assert(por->_lscchOriginal >= por->_lscch);

    if (por->_lscchOriginal > por->_lscch)
    {
        Assert(por->IsNormalRun());
        COneRun *porNew = SplitRun(por, por->_lscch);
        if (!porNew)
        {
            porRet = NULL;
            goto Cleanup;
        }
        
        //
        // Then splice in the current run and then the one we split out.
        //
        porRet = MergeIfPossibleIntoCurrentList(por);

        // can replace this with a SpliceInAfterMe
        MergeIfPossibleIntoCurrentList(porNew);
    }
    else
        porRet = MergeIfPossibleIntoCurrentList(por);

Cleanup:
    return porRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendSynth
//
//  Synopsis:   Appends a synthetic into the current one run store.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendSynth(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    COneRun *pTail = _listCurrent._pTail;
    LONG     lscp  = por->_lscpBase;
    LSERR    lserr = lserrNone;
    BOOL     fAdd;
    LONG     lscpLast;
    
    // Atmost one node can be un-processed
    if (pTail && pTail->_fNotProcessedYet)
    {
        pTail = pTail->_pPrev;
    }

    if (pTail)
    {
        lscpLast  = pTail->_lscpBase + (pTail->IsAntiSyntheticRun() ? 0 : pTail->_lscch);
        Assert(lscp <= lscpLast);
        if (lscp == lscpLast)
        {
            fAdd = TRUE;
        }
        else
        {
            fAdd = FALSE;
            while (pTail)
            {
                Assert(pTail->_fNotProcessedYet == FALSE);
                if (pTail->_lscpBase == lscp)
                {
                    Assert(pTail->IsSyntheticRun());
                    *pporOut = pTail;
                    break;
                }
                pTail = pTail->_pNext;
            }

            AssertSz(*pporOut, "Cannot find the synthetic char which should have been there!");
        }
    }
    else
        fAdd = TRUE;

    if (fAdd)
    {
        COneRun *porNew;
        
        porNew = _listFree.GetFreeOneRun(por);
        if (!porNew)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }

        //
        // Tell our clients which run the synthetic character was added
        //
        *pporOut = porNew;
        
        //
        // Let us change our synthetic run
        //
        porNew->MakeRunSynthetic();
        porNew->FillSynthData(synthtype);
        
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, pTail, porNew);
#else
        _listCurrent.SpliceInAfterMe(pTail, porNew);
#endif
        
        //
        // Now change the original one run itself
        //
        por->_lscpBase++;       // for the synthetic character
        por->_chSynthsBefore++;
        
        //
        // Update the tree info
        //
        _treeInfo._lscpFrontier++;
        _treeInfo._chSynthsBefore++;
    }
    
Cleanup:
    WHEN_DBG(_listCurrent.VerifyStuff(this));
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FillSynthData
//
//  Synopsis:   Fills information about a synthetic into the run
//
//  Returns:    nothing
//
//-----------------------------------------------------------------------------
void
COneRun::FillSynthData(CLineServices::SYNTHTYPE synthtype)
{
    const CLineServices::SYNTHDATA & synthdata = CLineServices::s_aSynthData[synthtype];
    
    _lscch = 1;
    _lscchOriginal = 1;
    _synthType = synthtype;
    _pchBase = (TCHAR *)&synthdata.wch;
    _pchBaseOriginal = _pchBase;
    _fHidden = synthdata.fHidden;
    _lsCharProps.idObj = synthdata.fObjStart ? synthdata.idObj : CLineServices::LSOBJID_TEXT;
    _fIsStartOrEndOfObj = synthdata.fObjStart || synthdata.fObjEnd;
    _fCharsForNestedElement  = FALSE;
    _fCharsForNestedLayout   = FALSE;
    _fCharsForNestedRunOwner = FALSE;

    // We only want the run to be considered processed if it is a true synthetic.
    // For the normal runs with synthetic data, this flag will be turned off
    // later in the FetchRun code.
    _fNotProcessedYet = IsSyntheticRun() ? FALSE : TRUE;
    
    _fCannotMergeRuns = TRUE;
    _fGlean = FALSE;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendAntiSynthetic
//
//  Synopsis:   Appends a anti-synthetic run
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendAntiSynthetic(COneRun *por)
{
    LSERR lserr = lserrNone;
    LONG  cch   = por->_lscch;

    Assert(por->IsAntiSyntheticRun());
    Assert(por->_lscch == por->_lscchOriginal);
    
    //
    // If the run is not in the list yet, please go and add it to the list
    //
    if (   por->_pNext == NULL
        && por->_pPrev == NULL
       )
    {
#if DBG==1
        _listCurrent.SpliceInAfterMe(this, _listCurrent._pTail, por);
#else
        _listCurrent.SpliceInAfterMe(_listCurrent._pTail, por);
#endif
    }

    //
    // This run has now been processed
    //
    por->_fNotProcessedYet = FALSE;

    //
    // Update the tree info
    //
    _treeInfo._lscpFrontier   -= cch;
    _treeInfo._chSynthsBefore -= cch;

    //
    // Now change all the subsequent runs in the list
    //
    por = por->_pNext;
    while(por)
    {
        por->_lscpBase       -= cch;
        por->_chSynthsBefore -= cch;
        por = por->_pNext;
    }
    
    WHEN_DBG(_listCurrent.VerifyStuff(this));
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineServices::TerminateLine
//
//  Synopsis:   Close any open LS objects. This will add end of object
//              characters to the synthetic store for any open LS objects and
//              also optionally add a synthetic WCH_SECTIONBREAK (fAddEOS).
//              If it adds any synthetic character it will set *psynthAdded to
//              the type of the first synthetic character added. FetchRun()
//              callers should be sure to check the *psynthAdded value; if it
//              is not SYNTHTYPE_NONE then the run should be filled using
//              FillSynthRun() and returned to Line Services.
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::TerminateLine(COneRun * por,
                             TL_ENDMODE tlEndMode,
                             COneRun **pporOut
                            )
{
    LSERR lserr = lserrNone;
    SYNTHTYPE synthtype;
    COneRun *porOut = NULL;
    COneRun *porRet;
    COneRun *pTail = _listCurrent._pTail;
    
    if (pTail)
    {
        int aObjRef[LSOBJID_COUNT];

        // Zero out the object refcount array.
        ZeroMemory( aObjRef, LSOBJID_COUNT * sizeof(int) );

        // End any open LS objects.
        for (; pTail; pTail = pTail->_pPrev)
        {
            if (!pTail->_fIsStartOrEndOfObj)
                continue;

            synthtype = pTail->_synthType;
            WORD idObj = s_aSynthData[synthtype].idObj;

            // If this synthetic character starts or stops an LS object...
            if (idObj != idObjTextChp)
            {
                // Adjust the refcount up or down depending on whether the object
                // is started or ended.
                if (s_aSynthData[synthtype].fObjEnd)
                {
                    aObjRef[idObj]--;
                }
                if (s_aSynthData[synthtype].fObjStart)
                {
                    aObjRef[idObj]++;
                }

                // If the refcount is positive we have an unclosed object (we're
                // walking backward). Close it.
                if (aObjRef[idObj] > 0)
                {
                    synthtype = s_aSynthData[synthtype].typeEndObj;
                    Assert(synthtype != SYNTHTYPE_NONE &&
                           s_aSynthData[synthtype].idObj == idObj &&
                           s_aSynthData[synthtype].fObjStart == FALSE &&
                           s_aSynthData[synthtype].fObjEnd == TRUE);

                    // If we see an open ruby object but the ruby main text
                    // has not been closed yet, then we must close it here
                    // before we can close off the ruby object by passing
                    // and ENDRUBYTEXT to LS.
                    if(idObj == LSOBJID_RUBY && _fIsRuby && !_fIsRubyText)
                    {
                        synthtype = SYNTHTYPE_ENDRUBYMAIN;
                        _fIsRubyText = TRUE;
                    }

                    // Be sure to inc lscp.
                    // BUGBUG (mikejoch) We should really be adjusting por here. As
                    // it is we aren't necessarily pointing at the correct run.
                    // This should be fixed when por positioning is fixed.
                    lserr = AppendSynth(por, synthtype, &porRet);
                    if (lserr != lserrNone)
                    {
                        //
                        // NOTE(SujalP): The linker (even in debug build) will
                        // not link in DumpList() since it is not called anywhere.
                        // This call here forces the linker to link in the DumpList
                        // function, so that we can use it during debugging.
                        //
                        WHEN_DBG(DumpList());
                        WHEN_DBG(DumpFlags());
                        WHEN_DBG(DumpTree());
                        WHEN_DBG(_lineFlags.DumpFlags());
                        WHEN_DBG(DumpCounts());
                        WHEN_DBG(_lineCounts.DumpCounts());
                        WHEN_DBG(DumpUnicodeInfo(0));
                        WHEN_DBG(DumpSids(sidsAll));
                        WHEN_DBG(fc().DumpFontInfo());
                        goto Cleanup;
                    }

                    //
                    // Terminate line needs to return the pointer to the run
                    // belonging to the first synthetic character added.
                    //
                    if (!porOut)
                        porOut = porRet;
                    
                    aObjRef[idObj]--;
                    
                    Assert(aObjRef[idObj] == 0);
                }
            }
        }
    }

    if (tlEndMode != TL_ADDNONE)
    {
        // Add a synthetic section break character.  Note we add a section
        // break character as this has no width.
        synthtype = tlEndMode == TL_ADDLBREAK ? SYNTHTYPE_LINEBREAK : SYNTHTYPE_SECTIONBREAK;
        lserr = AppendSynth(por, synthtype, &porRet);
        if (lserr != lserrNone)
            goto Cleanup;

        porRet->_fNoTextMetrics = TRUE;

        if (tlEndMode == TL_ADDLBREAK)
        {
            porRet->_fMakeItASpace = TRUE;
            if (GetRenderer())
            {
                lserr = SetRenderingHighlights(porRet);
                if (lserr != lserrNone)
                    goto Cleanup;
            }
        }
        
        if (!porOut)
            porOut = porRet;
    }

    // Lock up the synthetic character store. We've terminated the line, so we
    // don't want anyone adding any more synthetics.
    FreezeSynth();

Cleanup:
    *pporOut = porOut;
    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::IsSynthEOL
//
//  Synopsis:   Determines if there is some synthetic end of line mark at the
//              end of the synthetic array.
//
//  Returns:    TRUE if the synthetic array is terminated by a synthetic EOL
//              mark; otherwise FALSE.
//
//-----------------------------------------------------------------------------
BOOL
CLineServices::IsSynthEOL()
{
    COneRun *pTail = _listCurrent._pTail;
    SYNTHTYPE synthEnd = SYNTHTYPE_NONE;
    
    if (pTail != NULL)
    {
        if (pTail->_fNotProcessedYet)
            pTail = pTail->_pPrev;
        while (pTail)
        {
            if (pTail->IsSyntheticRun())
            {
                synthEnd = pTail->_synthType;
                break;
            }
            pTail = pTail->_pNext;
        }
    }

    return (   synthEnd == SYNTHTYPE_SECTIONBREAK
            || synthEnd == SYNTHTYPE_ENDPARA1
            || synthEnd == SYNTHTYPE_ALTENDPARA
           );
}

//-----------------------------------------------------------------------------
//
//  Function:   CPFromLSCPCore
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CPFromLSCPCore(LONG lscp, COneRun **ppor)
{
    COneRun *por = _listCurrent._pHead;
    LONG     cp  = lscp;

    while (por)
    {
        if (por->IsAntiSyntheticRun())
            cp += por->_lscch;
        else if (   lscp >= por->_lscpBase
                 && lscp <  por->_lscpBase + por->_lscch
                )
            break;
        else if (por->IsSyntheticRun())
            cp--;
        por = por->_pNext;
    }

    if (ppor)
        *ppor = por;

    return cp;
}

//-----------------------------------------------------------------------------
//
//  Function:   LSCPFromCPCore
//
//  FUTURE(SujalP): The problem with this function is that it is computing lscp
//                  and is using it to terminate the loop. Probably the better
//                  approach would be to use Cp's to determine termination
//                  conditions. That would be a radical change and we leave
//                  that to be fixed in IE5+.
//
//                  Another change which we need to make is that we do not
//                  inc/dec the lscp (and cp in the function above) as we
//                  march along the linked list. All the loop has to do is
//                  ensure that we end up with the correct COneRun and from
//                  that it should be pretty easy for us to determine the
//                  lscp/cp to be returned.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::LSCPFromCPCore(LONG cp, COneRun **ppor)
{
    COneRun *por = _listCurrent._pHead;
    LONG lscp = cp;

    while (por)
    {
        if (   lscp >= por->_lscpBase
            && lscp <  por->_lscpBase + por->_lscch
           )
            break;
        if (por->IsAntiSyntheticRun())
            lscp -= por->_lscch;
        else if (por->IsSyntheticRun())
            lscp++;
        por = por->_pNext;
    }

    // If we have stopped at an anti-synthetic it means that the cp is within this
    // run. This implies that the lscp is the same for all the cp's in this run.
    if (por && por->IsAntiSyntheticRun())
    {
        Assert(por->WantsLSCPStop());
        lscp = por->_lscpBase;
    }
    else
    {
        while(por && !por->WantsLSCPStop())
        {
            por = por->_pNext;
            lscp++;
        }
    }
    
    Assert( !por || por->WantsLSCPStop() );

    // It is possible that we can return a NULL por if there is a semi-valid
    // lscp that could be returned.
    if (ppor)
        *ppor = por;

    return lscp;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   FindOneRun
//
//  Synopsis:   Given an lscp, find the one run if it exists in the current list
//
//  Returns:    The one run
//
//-----------------------------------------------------------------------------
COneRun *
CLineServices::FindOneRun(LSCP lscp)
{
    COneRun *por = _listCurrent._pTail;

    if (!por)
    {
        por = NULL;
    }
    else if (lscp >= por->_lscpBase + por->_lscch)
    {
        por = NULL;
    }
    else
    {
        while (por)
        {
            if (   lscp >= por->_lscpBase
                && lscp <  por->_lscpBase + por->_lscch
               )
                break;
            por = por->_pPrev;
        }
    }
    return por;
}

//-----------------------------------------------------------------------------
//
//  Function:   FindPrevLSCP (member)
//
//  Synopsis:   Find the LSCP of the first actual character to preceed lscp.
//              Synthetic characters are ignored.
//
//  Returns:    LSCP of the character prior to lscp, ignoring synthetics. If no
//              characters preceed lscp, or lscp is beyond the end of the line,
//              then lscp itself is returned. Also returns a flag indicating if
//              any reverse objects exist between these two LSCPs.
//
//-----------------------------------------------------------------------------

LSCP
CLineServices::FindPrevLSCP(
    LSCP lscp,
    BOOL * pfReverse)
{
    COneRun * por;
    LSCP lscpPrev = lscp;
    BOOL fReverse = FALSE;

    // Find the por matching this lscp.
    por = FindOneRun(lscp);

    // If lscp was outside the limits of the line just bail out.
    if (por == NULL)
    {
        goto cleanup;
    }

    Assert(lscp >= por->_lscpBase && lscp < por->_lscpBase + por->_lscch);

    if (por->_lscpBase < lscp)
    {
        // We're in the midst of a run. lscpPrev is just lscp - 1.
        lscpPrev--;
        goto cleanup;
    }

    // Loop over the pors
    while (por->_pPrev != NULL)
    {
        por = por->_pPrev;

        // If the por is a reverse object set fReverse.
        if (por->IsSyntheticRun() &&
            s_aSynthData[por->_synthType].idObj == LSOBJID_REVERSE)
        {
            fReverse = TRUE;
        }

        // If the por is a text run then find the last lscp in it and break.
        if (por->IsNormalRun())
        {
            lscpPrev = por->_lscpBase + por->_lscch - 1;
            break;
        }
    }

cleanup:

    Assert(lscpPrev <= lscp);

    if (pfReverse != NULL)
    {
        // If we hit a reverse object but lscpPrev == lscp, then the reverse
        // object preceeds the first character in the line. In this case there
        // isn't actually a reverse object between the two LSCPs, since the
        // LSCPs are the same.
        *pfReverse = (fReverse && lscpPrev < lscp);
    }

    return lscpPrev;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchRun (member, LS callback)
//
//  Synopsis:   This is a key callback from lineservices.  LS calls this method
//              when performing LsCreateLine.  Here it is asking for a run, or
//              an embedded object -- whatever appears next in the stream.  It
//              passes us cp, and CLineServices (which we fool C++ into getting
//              to be the object of this method).  We return a bunch of stuff
//              about the next thing to put in the stream.
//
//  Returns:    lserrNone
//              lserrOutOfMemory
//
//-----------------------------------------------------------------------------
LSERR WINAPI
CLineServices::FetchRun(
    LSCP lscp,          // IN
    LPCWSTR* ppwchRun,  // OUT
    DWORD* pcchRun,     // OUT
    BOOL* pfHidden,     // OUT
    PLSCHP plsChp,      // OUT
    PLSRUN* pplsrun )   // OUT
{
    LSERR         lserr = lserrNone;
    COneRun      *por;
    COneRun      *pTail;
    LONG          cchDelta;
    COneRun      *porOut;

    AssertSz(_lockRecrsionGuardFetchRun == FALSE,
             "Cannot call FetchRun recursively!");
    WHEN_DBG(_lockRecrsionGuardFetchRun = TRUE;)
            
    ZeroMemory(plsChp, sizeof(LSCHP));  // Otherwise, we're gonna forget and leave some bits on that we shouldn't.
    *pfHidden = FALSE;
    
    if (IsAdornment())
    {
        por = GetRenderer()->FetchLIRun(lscp, ppwchRun, pcchRun);
        CHPFromCF(por, por->GetCF());
        goto Cleanup;
    }

    pTail = _listCurrent._pTail;
    //
    // If this was already cached before
    //
    if (lscp < _treeInfo._lscpFrontier)
    {
        Assert(pTail);
        Assert(_treeInfo._lscpFrontier == pTail->_lscpBase + pTail->_lscch);
        WHEN_DBG(_listCurrent.VerifyStuff(this));
        while (pTail)
        {
            if (lscp >= pTail->_lscpBase)
            {
                //
                // Should never get a AS run since the actual run will the run
                // following it and we should have found that one before since
                // we are looking from the end.
                //
                Assert(!pTail->IsAntiSyntheticRun());
                
                // We should be in this run since 1) if check above 2) walking backwards
                AssertSz(lscp <  pTail->_lscpBase + pTail->_lscch, "Inconsistent linked list");
               
                //
                // Gotcha. Got a previously cached sucker
                //
                por = pTail;
                Assert(por->_lscchOriginal == por->_lscch);
                cchDelta = lscp - por->_lscpBase;
                if (por->_fGlean)
                {
                    // We never have to reglean a synth or an antisynth run
                    Assert(por->IsNormalRun());

                    //
                    // NOTE(SujalP+MikeJoch):
                    // This can never happen because ls always fetches sequentially.
                    // If this happened it would mean that we were asked to fetch
                    // part of the run which was not gleaned. Hence the part before
                    // this one was not gleaned and hence not fetched. This violates
                    // the fact that LS will fetch all chars before the present one
                    // atleast once before it fetches the present one.
                    //
                    AssertSz(cchDelta == 0, "CAN NEVER HAPPEN!!!!");
#if 0
                    //
                    // If we are going to glean info from a run, then we need
                    // to split out the run if the lscp is not at the beginning
                    // of that run -- this is needed to avoid gleaning chars
                    // in por which are before lscp
                    //
                    if (cchDelta)
                    {
                        
                        // We cannot be asked to split an unprocessed run...
                        Assert(!por->_fNotProcessedYet);
                        
                        Assert(lscp > por->_lscpBase);

                        COneRun *porNew = SplitRun(por, cchDelta);
                        if (!porNew)
                        {
                            lserr = lserrOutOfMemory;
                            goto Cleanup;
                        }
#if DBG==1                        
                        _listCurrent.SpliceInAfterMe(this, por, porNew);
#else
                        _listCurrent.SpliceInAfterMe(por, porNew);
#endif
                        por = porNew;
                    }
#endif // if 0
                    
                    for(;;)
                    {
                        // We still have to be interested in gleaning
                        Assert(por->_fGlean);

                        // We will should never have a anti-synthetic run here
                        Assert(!por->IsAntiSyntheticRun());
                        
                        //
                        // Now go and glean information into the run ...
                        //
                        lserr = GleanInfoFromTheRun(por, &porOut);
                        if (lserr != lserrNone)
                            goto Cleanup;

                        //
                        // Did the run get marked as Antisynth. If so then
                        // we need to ignore that run and go onto the next one.
                        //
                        if (por->IsAntiSyntheticRun())
                        {
                            Assert(por == porOut);
                            
                            //
                            // The run was marked as an antisynthetic run. Be sure
                            // that no splitting was requested...
                            //
                            Assert(por->_lscch == por->_lscchOriginal);
                            Assert(por->_fNotProcessedYet);
                            AppendAntiSynthetic(por);
                            por = por->_pNext;
                        }
                        else
                            break;
                        
                        //
                        // If we ran out of already cached runs (all the cached runs
                        // turned themselves into anti-synthetics) then we need to
                        // advance the frontier and fetch new runs from the story.
                        //
                        if (por == NULL)
                            goto NormalProcessing;
                    }

                    //
                    // The only time a different run is than the one passed in is returned is
                    // when the run has not been procesed as yet, and during processing we
                    // notice that to process it we need to append a synthetic character.
                    // The case to handle here is:
                    // <table><tr><td nowrap>abcd</td></tr></table>
                    //
                    Assert(porOut == por || por->_fNotProcessedYet);

                    if (porOut != por)
                    {
                        //
                        // If we added a synthetic, then the present run should not be split!
                        //
                        Assert(por->_lscch == por->_lscchOriginal);
                        Assert(por->_fNotProcessedYet);
                        
                        //
                        // Remember we have to re-glean the information the next time we come around.
                        // However, we will not make the decision to append a synth that time since
                        // the synth has already been added this time and hence will fall into the
                        // else clause of this if and everything should be fine.
                        //
                        // DEPENDING ON WHETHER porOut WAS ADDED IN THE PRESENT GLEAN
                        // OR WAS ALREADY IN THE LIST, WE EITHER RETURN porOut OR por
                        //
                        por->_fGlean = TRUE;
                        por = porOut;
                    }
                    else
                    {
                        por->_fNotProcessedYet = FALSE;

                        //
                        // Did gleaning give us reason to further split the run?
                        //
                        if (por->_lscchOriginal > por->_lscch)
                        {
                            COneRun *porNew = SplitRun(por, por->_lscch);
                            if (!porNew)
                            {
                                lserr = lserrOutOfMemory;
                                goto Cleanup;
                            }
#if DBG==1
                            _listCurrent.SpliceInAfterMe(this, por, porNew);
#else
                            _listCurrent.SpliceInAfterMe(por, porNew);
#endif
                        }
                    }
                    por->_fGlean = FALSE;
                    cchDelta = 0;
                }

                //
                // This is our quickest way outta here! We had already done all
                // the hard work before so just reuse it here
                //
                *ppwchRun  = por->_pchBase + cchDelta;
                *pcchRun   = por->_lscch   - cchDelta;
                goto Cleanup;
            }
            pTail = pTail->_pPrev;
        } // while
        AssertSz(0, "Should never come here!");
    } // if


NormalProcessing:
    for(;;)
    {
        por = AdvanceOneRun(lscp);
        if (!por)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }

        lserr = GleanInfoFromTheRun(por, &porOut);
        if (lserr != lserrNone)
            goto Cleanup;
    
        Assert(porOut);

        if (por->IsAntiSyntheticRun())
        {
            Assert(por == porOut);
            AppendAntiSynthetic(por);
        }
        else
            break;
    }
    
    if (por != porOut)
    {
        *ppwchRun = porOut->_pchBase;
        *pcchRun  = porOut->_lscch;
        por->_fGlean = TRUE;
        por->_fNotProcessedYet = TRUE;
        Assert(por->_lscch == por->_lscchOriginal); // be sure no splitting takes place
        Assert(porOut->_fCannotMergeRuns);
        Assert(porOut->IsSyntheticRun());
        
        if (por->_lscch)
        {
            COneRun *porLastSynth = porOut;
            COneRun *porTemp = porOut;

            //
            // GleanInfoFromThrRun can add multiple synthetics to the linked
            // list, in which case we will have to jump across all of them
            // before we can add por to the list. (We need to add the por
            // because the frontier has already moved past that run).
            //
            while (porTemp && porTemp->IsSyntheticRun())
            {
                porLastSynth = porTemp;
                porTemp = porTemp->_pNext;
            }

            // FUTURE: porLastSynth should equal pTail.  Add a check for this, and
            // remove above while loop.
#if DBG==1        
            _listCurrent.SpliceInAfterMe(this, porLastSynth, por);
#else
            _listCurrent.SpliceInAfterMe(porLastSynth, por);
#endif
        }
        else
        {
            // Run not needed, please do not leak memory
            _listFree.SpliceIn(por);
        }
        
        // Finally remember that por is the run which we give to LS
        por = porOut;
    }
    else
    {
        por->_fNotProcessedYet = FALSE;
        *ppwchRun  = por->_pchBase;
        *pcchRun  = por->_lscch;
        por = AttachOneRunToCurrentList(por);
        if (!por)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
    }
    
Cleanup:
    if (lserr == lserrNone)
    {
        Assert(por);

        //
        // We can never return an antisynthetic run to LS!
        //
        Assert(!por->IsAntiSyntheticRun());

        if (por->_fCharsForNestedLayout && !IsAdornment())
        {
            // Give LS a junk character in this case. Fini will jump
            // accross the number of chars actually taken up by the
            // nested run owner.
            *ppwchRun = por->SetCharacter('A');
            *pcchRun = 1;
        }
        
        *pfHidden  = por->_fHidden;
        *plsChp    = por->_lsCharProps;
        *(PLSRUN *)pplsrun = por;
    }
    else
    {
        *(PLSRUN *)pplsrun = NULL;
    }
    
    WHEN_DBG(_lockRecrsionGuardFetchRun = FALSE;)
    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLineServices::AppendILSControlChar
//
//  Synopsis:   Appends an ILS object control character to the synthetic store.
//              This function allows us to begin and end line services objects
//              by inserting the control characters that begin and end them
//              into the text stream. It also keeps track of the state of the
//              object stack at the end of the synthetic store and returns the
//              synthetic type that matches the first added charcter.
//
//              A curiousity of Line Services dictates that ILS objects cannot
//              be overlapped; it is not legal to have:
//
//                  <startNOBR><startReverse><endNOBR><endReverse>
//
//              If this case were to arise the <endNOBR> would get ignored and
//              the NOBR object would continue until the line was terminated
//              by a TerminateLine() call. Furthermore, the behavior of ILS
//              objects is not always inherited; the reverse object inside of
//              a NOBR will still break.
//
//              To get around these problems it is necessary to keep the object
//              stack in good order. We define a hierarchy of objects and make
//              certain that whenever a new object is created any objects which
//              are lower in the heirarchy are closed and reopened after the
//              new object is opened. The current heirarchy is:
//
//                  Reverse objects     (highest)
//                  NOBR objects    
//                  Embedding objects   (lowest)
//
//              Additional objects (FE objects) will be inserted into this
//              heirarchy.
//
//              Embedding objects require no special handling due to their
//              special handling (recursive calls to line services). Thus the
//              only objects which currently require handling are the reverse
//              and NOBR objects.
//
//              If we apply our strategy to the overlapped case above, we will
//              end up with the following:
//
//                  <startNOBR><endNOBR><startReverse><startNOBR><endNOBR><endReverse>
//
//              As can be seen the objects are well ordered and each object's
//              start character is paired with its end character. NOBRs are
//              kept at the top of the stack, and reverse objects preceed them.
//
//              One problem which is introduced by this solution is the fact
//              that a break opprotunity is introduced between the two NOBR
//              objects. This can be fixed in the NOBR breaking code.
//
//  Returns:    An LSERR value. The function also returns synthetic character
//              at lscp in *psynthAdded. This is the first charcter added by
//              this function, NOT necessarily the character that matches idObj
//              and fOpen. For example (again using the case above) when we
//              ask to open the LSOBJID_REVERSE inside the NOBR object we will
//              return SYNTHTYPE_ENDNOBR in *psynthAdded (though we will also
//              append the SYNTHTYPE_REVERSE and START_NOBR to the store).
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendILSControlChar(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    Assert(synthtype != SYNTHTYPE_NONE);
    const SYNTHDATA & synthdata = s_aSynthData[synthtype];
    LSERR lserr = lserrNone;

#if DBG==1
    BOOL fOpen = synthdata.fObjStart;
#endif
    
    // We can only APPEND REAL OBJECTS to the store.
    Assert(synthdata.idObj != LSOBJID_TEXT);
    
    *pporOut = NULL;

    if (IsFrozen())
    {
        // We cannot add to the store if it is frozen.
        goto Cleanup;
    }

    // Handle the object.
    switch (synthdata.idObj)
    {
    case LSOBJID_NOBR:
    {
        // NOBR objects just need to be opened or closed. Note that we cannot
        // close an object unless it has been opened, nor can we open an object
        // if one is already open.
        Assert(!!_fNoBreakForMeasurer != !!fOpen);

#if DBG == 1
        if (!fOpen)
        {
            COneRun *porTemp = _listCurrent._pTail;
            BOOL fFoundTemp = FALSE;
            
            while (porTemp)
            {
                if (porTemp->IsSyntheticRun())
                {
                    if (porTemp->_synthType != SYNTHTYPE_NOBR)
                    {
                        AssertSz(0, "Should have found an STARTNOBR before anyother synth");
                    }
                    fFoundTemp = TRUE;
                    break;
                }
                porTemp = porTemp->_pPrev;
            }
            AssertSz(fFoundTemp, "Did not find the STARTNOBR you are closing!");
        }
#endif
        lserr = AppendSynth(por, synthtype, pporOut);
        if (lserr != lserrNone)
            goto Cleanup;
        
        break;
    }
    
    case LSOBJID_REVERSE:
    {
        COneRun *porTemp = NULL;

        // If we've got an open NOBR object, we need to close it first.
        if (_fNoBreakForMeasurer)
        {
            COneRun * porNobrStart;
            
            lserr = AppendSynth(por, SYNTHTYPE_ENDNOBR, pporOut);
            if (lserr != lserrNone)
                goto Cleanup;

            // We need to mark the starting por that it was artificially
            // terminated, so we can break appropriate in the ILS handlers.
            
            for (porNobrStart = (*pporOut)->_pPrev; porNobrStart; porNobrStart = porNobrStart->_pPrev)
            {
                if (   porNobrStart->IsSyntheticRun()
                    && porNobrStart->_synthType == SYNTHTYPE_NOBR)
                    break;
            }

            Assert(porNobrStart);

            // Can't break after this END-NOBR

            porNobrStart->_fIsArtificiallyTerminatedNOBR = 1;            
        }
        
        // Open or close the reverse object and update _nReverse.
        lserr = AppendSynth(por, synthtype, *pporOut ? &porTemp : pporOut);
        if (lserr != lserrNone)
            goto Cleanup;
        
        // If there was a NOBR object before we opened or closed the
        // reverse object re-open a new NOBR object.
        if (_fNoBreakForMeasurer)
        {
            Assert(*pporOut);
            lserr = AppendSynth(por, SYNTHTYPE_NOBR, &porTemp);
            if (lserr != lserrNone)
                goto Cleanup;

            // Can't break before this BEGIN-NOBR

            porTemp->_fIsArtificiallyStartedNOBR = 1;            
        }
    }
    break;

    case LSOBJID_RUBY:
    {
        // Here we close off all open LS objects, append the ruby synthetic
        // and then reopen the LS objects.
        lserr = AppendSynthClosingAndReopening(por, synthtype, pporOut);
        if(lserr != lserrNone)
        goto Cleanup;
    }
    break;
     
#if DBG==1
    default:
        // We only handle NOBR and reverse objects so far. Embedding objects
        // should not come here, and we don't support the FE objects so far.
        Assert(FALSE);
        break;
#endif
    }

Cleanup:
    // Make sure the store is still in good shape.
    WHEN_DBG(_listCurrent.VerifyStuff(this));

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AppendSynthClosingAndReopening
//
//  Synopsis:   Appends a synthetic into the current one run store, but first
//              closes all open LS objects and then reopens them afterwards.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLineServices::AppendSynthClosingAndReopening(COneRun *por, SYNTHTYPE synthtype, COneRun **pporOut)
{
    LSERR lserr = lserrNone;
    COneRun *porOut = NULL, *porTail;
    SYNTHTYPE curSynthtype;
    WORD idObj = s_aSynthData[synthtype].idObj;
    CStackDataAry<SYNTHTYPE, 16> arySynths(0);
    int i;

    int aObjRef[LSOBJID_COUNT];

    // Zero out the object refcount array.
    ZeroMemory( aObjRef, LSOBJID_COUNT * sizeof(int) );

    *pporOut = NULL;

    // End any open LS objects.
    for (porTail = _listCurrent._pTail; porTail; porTail = porTail->_pPrev)
    {
        if (!porTail->_fIsStartOrEndOfObj)
            continue;

        curSynthtype = porTail->_synthType;
        WORD curIdObj = s_aSynthData[curSynthtype].idObj;

        // If this synthetic character starts or stops an LS object...
        if (curIdObj != idObj)
        {
            // Adjust the refcount up or down depending on whether the object
            // is started or ended.
            if (s_aSynthData[curSynthtype].fObjEnd)
            {
                aObjRef[curIdObj]--;
            }
            if (s_aSynthData[curSynthtype].fObjStart)
            {
                aObjRef[curIdObj]++;
            }

            // If the refcount is positive we have an unclosed object (we're
            // walking backward). Close it.
            if (aObjRef[curIdObj] > 0)
            {
                arySynths.AppendIndirect(&curSynthtype);
                curSynthtype = s_aSynthData[curSynthtype].typeEndObj;
                Assert(curSynthtype != SYNTHTYPE_NONE &&
                       s_aSynthData[curSynthtype].idObj == curIdObj &&
                       s_aSynthData[curSynthtype].fObjStart == FALSE &&
                       s_aSynthData[curSynthtype].fObjEnd == TRUE);

                // NOTE (t-ramar): closing a Ruby object may require adding
                // two synths (endrubymain and endrubytext) depending on 
                // how it is currently open.  Right now, this function is being 
                // called only when Ruby synths are being appended so this situation 
                // will never occur.
                // if this function is used to append a non-Ruby synth,
                // the assert should be removed and code added that will close
                // a ruby object properly; that is, it will make sure that one or both
                // of [endrubymain] and [endrubytext] are appended appropriately.
                // Also, the code that reopens the closed objects (in the for loop below)
                // may need to be changed.
                Assert(curIdObj != LSOBJID_RUBY);

                lserr = AppendSynth(por, curSynthtype, &porOut);
                if(*pporOut == NULL) 
                {
                    *pporOut = porOut;
                }

                if (lserr != lserrNone)
                {
                    goto Cleanup;
                }

                aObjRef[curIdObj]--;
                
                Assert(aObjRef[curIdObj] == 0);
            }
        }
        else
        {
            break;
        }
    }

    // Append the synth that was passed in
    lserr = AppendSynth(por, synthtype, &porOut);
    if(lserr != lserrNone)
    {
        porOut = NULL;
        goto Cleanup;
    }
    if(*pporOut == NULL) 
    {
        *pporOut = porOut;
    }

    // Re-open the LS objects that we closed
    for (i = arySynths.Size();
         i > 0;
         i--)
    {
        curSynthtype = arySynths[i-1];        
        // NOTE (t-ramar): reopening a Ruby object may require adding
        // two synths (rubymain and endrubymain) depending on how it was
        // closed.  Right now, this function is being called only when
        // Ruby synths are being appended so this situation will never
        // occur.
        Assert(s_aSynthData[curSynthtype].idObj != LSOBJID_RUBY);
        
        Assert(curSynthtype != SYNTHTYPE_NONE &&
               s_aSynthData[curSynthtype].fObjStart == TRUE &&
               s_aSynthData[curSynthtype].fObjEnd == FALSE);
        lserr = AppendSynth(por, curSynthtype, &porOut);
        if (lserr != lserrNone)
        {
            goto Cleanup;
        }
    }

Cleanup:
    // Make sure the store is still in good shape.
    WHEN_DBG(_listCurrent.VerifyStuff(this));

    return lserr;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   GetCharWidthClass
//
//  Synopsis:   Determines the characters width class within this run
//
//  Returns:    character width class
//
//-----------------------------------------------------------------------------
COneRun::charWidthClass
COneRun::GetCharWidthClass()
{
    charWidthClass cwc = charWidthClassUnknown;
    if (_ptp->IsText())
    {
        switch (_ptp->Sid())
        {
        case sidHan:
        case sidHangul:
        case sidKana:
        case sidBopomofo:
        case sidYi:
            cwc = charWidthClassFullWidth;
            break;

        case sidHebrew:
        case sidArabic:
            cwc = charWidthClassCursive;
            break;

        default:
            cwc = charWidthClassHalfWidth;
        }
    }
    return cwc;
}
   
//-----------------------------------------------------------------------------
//
//  Function:   DumpList
//
//-----------------------------------------------------------------------------
#if DBG==1
void
CLineServices::DumpList()
{
    int nCount = 0;
    COneRun *por = _listCurrent._pHead;
    
    if (!InitDumpFile())
        goto Cleanup;

    WriteString( g_f,
                 _T("\r\n------------- OneRunList Dump -------------------------------\r\n" ));
    while(por)
    {
        nCount++;
        WriteHelp(g_f, _T("\r\n[<0d>]: lscp:<1d>, lscch:<2d>, synths:<3d>, ptp:<4d> "),
                          por->_nSerialNumber, por->_lscpBase, por->_lscch,
                          por->_chSynthsBefore, por->_ptp == NULL ? -1 : por->_ptp->_nSerialNumber);
        if (por->IsNormalRun())
            WriteHelp(g_f, _T("Normal, "));
        else if (por->IsSyntheticRun())
            WriteHelp(g_f, _T("Synth, "));
        else if (por->IsAntiSyntheticRun())
            WriteHelp(g_f, _T("ASynth, "));
        if (por->_fGlean)
            WriteHelp(g_f, _T("Glean, "));
        if (por->_fNotProcessedYet)
            WriteHelp(g_f, _T("!Processed, "));
        if (por->_fHidden)
            WriteHelp(g_f, _T("Hidden, "));
        if (por->_fCannotMergeRuns)
            WriteHelp(g_f, _T("NoMerge, "));
        if (por->_fCharsForNestedElement)
            WriteHelp(g_f, _T("NestedElem, "));
        if (por->_fCharsForNestedLayout)
            WriteHelp(g_f, _T("NestedLO, "));
        if (por->_fCharsForNestedRunOwner)
            WriteHelp(g_f, _T("NestedRO, "));
        if (por->_fSelected)
            WriteHelp(g_f, _T("Sel, "));
        if (por->_fNoTextMetrics)
            WriteHelp(g_f, _T("NoMetrics, "));
        if (por->_fMustDeletePcf)
            WriteHelp(g_f, _T("DelCF, "));
        if (por->_lsCharProps.fGlyphBased)
            WriteHelp(g_f, _T("Glyphed, "));
        WriteHelp(g_f, _T("\r\nText:'"));
        if(por->_synthType != SYNTHTYPE_NONE)
        {
            WriteFormattedString(g_f,  s_aSynthData[por->_synthType].pszSynthName, _tcslen(s_aSynthData[por->_synthType].pszSynthName));
        }
        else
        {
            WriteFormattedString(g_f,  (TCHAR*)por->_pchBase, por->_lscch);
        }        
        WriteHelp(g_f, _T("'\r\n"));

        por = por->_pNext;
    }

    WriteHelp(g_f, _T("\r\nTotalRuns: <0d>\r\n"), (long)nCount);

Cleanup:
    CloseDumpFile();
}

void
CLineServices::DumpFlags()
{
    _lineFlags.DumpFlags();
}

void
CLineFlags::DumpFlags()
{
    TCHAR *str;
    int i;
    LONG nCount;
    
    if (!InitDumpFile())
        goto Cleanup;
    
    WriteString( g_f,
                 _T("\r\n------------- Line Flags Dump -------------------------------\r\n" ));

    nCount = _aryLineFlags.Size();
    WriteHelp(g_f, _T("Total Flags: <0d>\r\n"), (long)nCount);
    for (i = 0; i < nCount; i++)
    {
        switch(_aryLineFlags[i]._dwlf)
        {
            case FLAG_NONE              : str = _T("None"); break;
            case FLAG_HAS_ABSOLUTE_ELT  : str = _T("Absolute"); break;
            case FLAG_HAS_ALIGNED       : str = _T("Aligned"); break;
            case FLAG_HAS_EMBED_OR_WBR  : str = _T("Embed/Wbr"); break;
            case FLAG_HAS_NESTED_RO     : str = _T("NestedRO"); break;
            case FLAG_HAS_RUBY          : str = _T("Ruby"); break;
            case FLAG_HAS_BACKGROUND    : str = _T("Background"); break;
            case FLAG_HAS_A_BR          : str = _T("BR"); break;
            case FLAG_HAS_RELATIVE      : str = _T("Relative"); break;
            case FLAG_HAS_NBSP          : str = _T("NBSP"); break;
            case FLAG_HAS_NOBLAST       : str = _T("NoBlast"); break;
            case FLAG_HAS_CLEARLEFT     : str = _T("ClearLeft"); break;
            case FLAG_HAS_CLEARRIGHT    : str = _T("ClearRight"); break;
            case FLAG_HAS_LETTERSPACING : str = _T("LetterSpacing"); break;
            default                     : str = _T(""); break;
        }
        WriteHelp(g_f, _T("cp=<0d> has <1s>\r\n"), _aryLineFlags[i]._cp, str);
    }
    WriteString(g_f, _fForced ? _T("Forced") : _T("NotForced"));

Cleanup:
    CloseDumpFile();
}

void
CLineServices::DumpCounts()
{
    _lineCounts.DumpCounts();
}

void
CLineCounts::DumpCounts()
{
    LONG nCount;
    int  i;
    
    static WCHAR *g_astr[] = {_T("Undefined"), _T("Inlined"), _T("Aligned"),
                              _T("Hidden"),    _T("Absolute")};
    
    if (!InitDumpFile())
        goto Cleanup;

    WriteString( g_f,
                 _T("\r\n------------- Line Counts Dump -------------------------------\r\n" ));

    nCount = _aryLineCounts.Size();
    WriteHelp(g_f, _T("Total Counts: <0d>\r\n"), (long)nCount);
    for (i = 0; i < nCount; i++)
    {
        WriteHelp(g_f, _T("cp=<0d> cch=<1d> for <2s>\r\n"),
                  _aryLineCounts[i]._cp,
                  _aryLineCounts[i]._count,
                  g_astr[_aryLineCounts[i]._lcType]);
    }
Cleanup:
    CloseDumpFile();
}

#endif
