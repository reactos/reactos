//+----------------------------------------------------------------------------
//
// File:        lshost.cxx
//
// Contents:    Implementation of CQuillGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// @doc INTERNAL
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_QPROPS_HXX_
#define X_QPROPS_HXX_
#include "qprops.hxx"
#endif

//+------------------------------------------------------------------------
//
//  Helper functions.
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@mfunc void | CQuillGlue | SyncTreePointerToCp | Quill support.
@contact sidda

@comm	Synchronize a tree pointer to a cp. In general this operation should
        be performed as least often as possible.
----------------------------------------------------------------------------*/
void
CQuillGlue::SyncTreePointerToCp(
    CTreePos **pptp,    // @parm Points to tree pointer to be synchronized.
    long cpAbs,         // @parm CP that tree pointer should encompass.
    long *pcchOffsetOfCpAbsFromTPStart) // @parm Offset from tree pointer start returned here.
{
    // NOTE:    calling GetCp() is logN and can cause splay tree to be balanced
    //          calling TreePosAtCp() is logN can cause splay tree to be balanced
    //          calling GetCch() is constant time

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptp)->GetCp();
#endif

    *pptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cpAbs, pcchOffsetOfCpAbsFromTPStart);
    Assert(!(*pptp)->IsPointer());
    Assert(   (*pptp)->GetCp() <= cpAbs
           && (*pptp)->GetCp() + (*pptp)->GetCch() > cpAbs
          );
}

/*----------------------------------------------------------------------------
@mfunc CLayout * | CQuillGlue | GetNestedLayoutFromTreePointer | Quill support.
@contact sidda

@comm	Returns the CLayout at the tree pointer. The tree pointer must already
        be known to be positioned at a nested layout object.
----------------------------------------------------------------------------*/
CLayout *
CQuillGlue::GetNestedLayoutFromTreePointer(
    CTreePos **pptp)
{
    Assert(IsNestedLayoutAtTreePos((DWORD)pptp) == S_OK);
#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptp)->GetCp();
#endif
    CTreeNode *pNode = (*pptp)->Branch();
    Assert(pNode->HasLayout());
    return pNode->GetLayout();
}

// @doc QTAPI

//+------------------------------------------------------------------------
//
//  ILineServicesHost implementation.
//
//-------------------------------------------------------------------------

/*----------------------------------------------------------------------------
@interface ILineServicesHost |

	Provides efficient document access in a form suitable for Line Services support.

@meth   HRESULT | GetTreePointer | Create a tree pointer associated with the specified CP position.
@meth   HRESULT | FreeTreePointer | Free a previously-obtained tree pointer.
@meth   HRESULT | SetTreePointerCp | Positions a tree pointer to encompass the specified cp.
@meth   HRESULT | GetTreePointerCch | Retrieve the cp count for a tree pointer.
@meth   HRESULT | MoveToNextTreePos | Efficiently advances to the next tree position in the document.
@meth   HRESULT | IsNestedLayoutAtTreePos | Determine if the tree pos has a nested layout.
@meth   HRESULT | AdvanceBeforeLine | Advance the necessary number of characters before calling Line Services to create a line.
@meth   HRESULT | AdvanceAfterLine | Advance the necessary number of characters after calling Line Services to create a line.
@meth   HRESULT | HiddenBeforeAlignedObj | Skip ignorable text before aligned objects.
@meth   HRESULT | GetLineContext | Create a new context.
@meth   HRESULT | FreeLineContext | Destroy the context.
@meth   HRESULT | SetContext | Associate the context with the current layout.
@meth   HRESULT | ClearContext | Reset the context.
@meth   HRESULT | Setup | Prepare the context for line creation.
@meth   HRESULT | DiscardLine | Reset state information after a line has been formatted.
@meth   HRESULT | CpRelFromCpLs | Convert a Line Services CP to a layout-relative CP.
@meth   HRESULT | CpLsFromCpRel | Convert a layout-relative CP to a Line Services CP.
@meth   HRESULT | FetchHostRun | Fetch a run of document properties and text at the specified CP.
@meth   HRESULT | TerminateLineAfterRun | Force line to be terminated after the specified run.
@meth   HRESULT | GetLineSummaryInfo | Determine line formatting characteristics up to the specified CP.

@comm	This interface exposes the built-in support provided by the host for
		accessing the document properties in response to Line Services callbacks.
		This interface is designed for use by an external layout engine which
		is itself based on Line Services and requires efficient access to
		the document backing store under Line Services callbacks.

@supby  <o TextLayoutSite>
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | GetTreePointer |

    Create a tree pointer associated with the specified CP position.

@rvalue	S_OK | Success.
@rvalue	E_OUTOFMEMORY | Unable to allocate memory.
@rvalue	E_INVALIDARG | <p pdwCookieTP> is NULL.

@comm   The tree pointer must be freed by a matching call to <om .FreeTreePointer>.

@xref	<om .FreeTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetTreePointer(
    long cpRel,         // @parm Document CP the tree pointer to be associated with.
    DWORD *pdwCookieTP, // @parm Tree pointer cookie is returned in *<p pdwCookieTP>.
                        // On error, *<p pdwCookieTP> will be set to NULL.
    long *pcchOffsetOfCpRelFromTPStart) // @parm If non-NULL, *<p pcchOffsetOfCpRelFromTPStart>
                                        // will be set to the offset of <p cpRel> from
                                        // the starting CP of the tree pointer.
{
    long cp = CpAbsFromCpRel(cpRel);
    LONG ich;
    CTreePos **pptp;

    if (pcchOffsetOfCpRelFromTPStart)
        *pcchOffsetOfCpRelFromTPStart = 0;

    if (!pdwCookieTP)
        return E_INVALIDARG;

    *pdwCookieTP = NULL;

    // the cookie is a CTreePos **
    pptp = (CTreePos **)new(CTreePos *);
    if (!pptp)
        return E_OUTOFMEMORY;

    *pptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);
    if (!(*pptp))
        {
        delete pptp;
        return E_OUTOFMEMORY;
        }
    Assert(!(*pptp)->IsPointer());

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptp)->GetCp();
#endif

    *pdwCookieTP = (DWORD)pptp;
    if (pcchOffsetOfCpRelFromTPStart)
        *pcchOffsetOfCpRelFromTPStart = ich;

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | FreeTreePointer |

    Free a previously-obtained tree pointer.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie.

@xref	<om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FreeTreePointer(
    DWORD dwCookieTP)   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>.
{
    CTreePos **pptp;

    if (!dwCookieTP)
        return E_INVALIDARG;

    pptp = (CTreePos **)dwCookieTP;
    delete pptp;

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | SetTreePointerCp |

    Positions a tree pointer to encompass the specified cp.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie.

@comm   A tree pointer refers to portions of the document tree. Note that the
        requested cp might lie in the middle of such a portion.

@comm   Note that this can be a somewhat expensive operation, since it could
        involve adjustment of the splay tree.

@comm   Note to implementors: calling CTreePos->GetCp() is a logN operation.

@xref	<om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::SetTreePointerCp(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>.
    long cpRel,         // @parm Document CP the tree pointer to be associated with.
    long *pcchOffsetOfCpRelFromTPStart) // @parm If non-NULL, *<p pcchOffsetOfCpRelFromTPStart>
                                        // will be set to the offset of <p cpRel> from
                                        // the starting CP of the tree pointer.
{
    long cp = CpAbsFromCpRel(cpRel);
    CTreePos **pptpCookie;
    long ich;

    if (pcchOffsetOfCpRelFromTPStart)
        *pcchOffsetOfCpRelFromTPStart = 0;

    if (!dwCookieTP)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    SyncTreePointerToCp(pptpCookie, cp, &ich);
    if (pcchOffsetOfCpRelFromTPStart)
        *pcchOffsetOfCpRelFromTPStart = ich;

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | GetTreePointerCch |

    Retrieve the cp count for a tree pointer.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie, or
                       <p pcch> is NULL.

@comm   WARNING: this routine is returns the following CP counts:

@flag   Begin-scope node | 1 (use <om ITextStory.GetNestedObjectAttributes> to
                retrieve total object length for a nested object).
@flag   End-scope node | 1.
@flag   Text node | Count of characters in the text.

@comm   Note that if we point to a markup pointer node the count returned will
        be zero. We expect not to be pointing to a markup pointer, and debug
        code asserts this.

@xref	<om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetTreePointerCch(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>.
    long *pcch)         // @parm Must be non-NULL. *<p pcch> will be set to the
                        // number of document characters encompassed by the tree pointer.
{
    CTreePos **pptpCookie;

    if (!dwCookieTP || !pcch)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    Assert(!(*pptpCookie)->IsPointer());

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptpCookie)->GetCp();
#endif

    *pcch = (*pptpCookie)->GetCch();
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | MoveToNextTreePos |

    Efficiently advances to the next tree position in the document.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie.

@comm   This is a low-level function that knows nothing about nested objects.
        It simply moves from one tree pos to the next one after it.

@comm   Note that if the tree pointer is currently positioned at the
        begin-scope node of a nested layout object, this routine will NOT
        skip over the entire layout object.

@comm   This routine automatically skips over any markup pointer nodes that
        are encountered. Recall that markup pointer nodes themselves occupy
        zero CP space in the document, so skipping over them does not cause
        any advance in document CP position.

@xref	<om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::MoveToNextTreePos(
    DWORD dwCookieTP)   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>.
{
    CTreePos **pptpCookie;

    if (!dwCookieTP)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    Assert(!(*pptpCookie)->IsPointer());

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptpCookie)->GetCp();
#endif

    *pptpCookie = (*pptpCookie)->NextTreePos();
    while ((*pptpCookie)->IsPointer())
        *pptpCookie = (*pptpCookie)->NextTreePos();

    Assert(!(*pptpCookie)->IsPointer());
#if DBG==1
    cpCurJunk = (*pptpCookie)->GetCp();
#endif
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | IsNestedLayoutAtTreePos |

    Determine if the tree pos has a nested layout.

@rvalue	S_OK | There is a nested layout at the tree pos.
@rvalue S_FALSE | There is no nested layout at the tree pos.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie.

@xref	<om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::IsNestedLayoutAtTreePos(
    DWORD dwCookieTP)   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>.
{
    CTreePos **pptpCookie;

    if (!dwCookieTP)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    Assert(!(*pptpCookie)->IsPointer());

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptpCookie)->GetCp();
#endif

    if ((*pptpCookie)->IsBeginElementScope())
    {
        CTreeNode *pNode = (*pptpCookie)->Branch();
        if (pNode->HasLayout())
            return S_OK;
    }

    return S_FALSE;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | AdvanceBeforeLine |

    Advance the necessary number of characters before calling Line Services
    to create a line.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie, or
                       <p pcchSkip> is NULL.

@comm   This method is necessary to match the Line Services host document cp
        advance behavior. The Line Services host may not expose some characters
        in the document to Line Services. An example of this is collapsing of
        multiple p tags.

@xref	<om .AdvanceAfterLine>, <om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::AdvanceBeforeLine(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>. The tree pointer will not be
                        // modified.
    long cpRelOrigLineStart,    // @parm CP position of the presumed start of the line.
    long *pcchSkip)     // @parm Number of characters to advance returned in *<p pcchSkip>.
{
    long cp = CpAbsFromCpRel(cpRelOrigLineStart);
    CTreePos **pptpCookie;

    if (!dwCookieTP || !pcchSkip)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    *pcchSkip = 0;

    // sync up tree pointer to cp if necessary
    // With Quill, this will be necessary since constraints can cause a line to be reformatted
    // and so we must "rewind" the tree pointer to the beginning of the line.
    // Also, pre-inspection of objects at the start of the line can cause the tree pointer
    // to be advanced past those objects.
    LONG notNeeded;
    SyncTreePointerToCp(pptpCookie, cp, &notNeeded);

    //
    // WARNING: following code must be kept in sync with CRecalcLinePtr::CalcBeforeSpace() cousin
    //

    {
    CFlowLayout  *pFlowLayout = m_pFlowLayout;
    CMarkup      *pMarkup     = pFlowLayout->GetContentMarkup();
    CDoc         *pDoc        = pFlowLayout->Doc();
    CTreeNode    *pNode       = NULL;
    CElement     *pElement;
    CTreePos     *ptpStop;
    CTreePos     *ptp;

    pFlowLayout->GetContentTreeExtent(NULL, &ptpStop);

    for (ptp = *pptpCookie; ; ptp = ptp->NextTreePos())
    {
        if (ptp->IsPointer())
            continue;

        if (ptp->IsNode())
        {
            if (_fIsEditable && ptp->ShowTreePos())
                break;

            pNode = ptp->Branch();
            pElement = pNode->Element();
            if (ptp->IsEndElementScope())
            {
                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    // $REVIEW sidda: bullets & numbering support

                    // Finally, if we have reached the end of our flowlayout
                    // then we need to quit. We have to process the end ptp
                    // since we want the margins of our flow layout to be
                    // reflected in the afterspace info.
                    if (ptp == ptpStop)
                        break;
                }
                // Else do nothing, just continue looking ahead
            
                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginElementScope())
            {
                const CCharFormat *pCF = pNode->GetCharFormat();

                if (pCF->IsDisplayNone())
                {
                    // The extra one is added in the normal processing.
                    (*pcchSkip) += pNode->Element()->GetElementCch() + 1;
                    pElement->GetTreeExtent(NULL, &ptp);
                }
                else
                {
                    if (pFlowLayout->IsElementBlockInContext(pElement))
                    {
                        // For those block elements which explicitly break lines (even if
                        // they are empty), we stop looking forward here. What this does
                        // is that we will then fall into the measurer and for empty
                        // block elements, the measurer will immly. terminate, creating
                        // an empty line (with height) for us.
                        if (pElement->_fBreakOnEmpty)
                        {
                            // We want to consume the splay character for the
                            // begin of the break on empty element in this line
                            // hence move forward here before breaking out.
                            (*pcchSkip) += ptp->GetCch();
                            ptp = ptp->NextTreePos();
                            break;
                        }
                    }
                    else if (pElement->Tag() == ETAG_BR)
                    {
                        break;
                    }

                    // If we have hit a nested layout which has characters, then we
                    // quit so that it can be measured. Note that we have noted the
                    // before space the site contributes in the code above.
                    if (   !pCF->IsDisplayNone()
                        && pNode->HasLayout()
                        //&& pElement->GetElementCch() - 2
                       )
                        break;
                    // BUGBUG(Take care of empty tables here)
                }
            }
        }
        else
        {
            Assert(ptp->IsText());
            break;
        }

        (*pcchSkip) += ptp->GetCch();
    }
    }

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | AdvanceAfterLine |

    Advance the necessary number of characters after calling Line Services
    to create a line.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie, or
                       <p dwCookieLC> is not a valid line context cookie, or
                       <p pcchSkip> is NULL.

@comm   <p dwCookieLC> should correspond to the cookie for the line context
        that has just been formatted.

@xref	<om .AdvanceBeforeLine>, <om .GetTreePointer>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::AdvanceAfterLine(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>. The tree pointer will be
                        // updated to refer to the first character in the next line.
	DWORD dwCookieLC,   // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    long cpRelOrigLineLim,  // @parm CP position of cpLim of the line.
    long *pcchSkip)     // @parm Number of characters to advance returned in *<p pcchSkip>.
{
    long cp = CpAbsFromCpRel(cpRelOrigLineLim);
    CTreePos **pptpCookie;
    CLineServices * pLS;

    if (!dwCookieTP || !dwCookieLC || !pcchSkip)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    pLS = (CLineServices *)dwCookieLC;
    Assert(pLS->_treeInfo._fInited);
    *pcchSkip = 0;

#if DBG==1
    long cpCurJunk;
    cpCurJunk = (*pptpCookie)->GetCp();
#endif

    // figure next tree pointer position given cpLim of line
    CTreePos *ptpNext = pLS->FigureNextPtp(cp); // the fast way
    if (ptpNext)
    {
        *pptpCookie = ptpNext;
    }
    else
    {
        LONG notNeeded;
        SyncTreePointerToCp(pptpCookie, cp, &notNeeded);
    }
    Assert(cp >= (*pptpCookie)->GetCp() && cp < (*pptpCookie)->GetCp() + (*pptpCookie)->GetCch());

    //
    // WARNING: following code must be kept in sync with CRecalcLinePtr::CalcAfterSpace() cousin
    //

    {
    CFlowLayout  *pFlowLayout = m_pFlowLayout;
    CTreeNode    *pNode;
    CElement     *pElement;
    CTreePos     *ptpStart;
    CTreePos     *ptpStop;
    CTreePos     *ptp;

    pFlowLayout->GetContentTreeExtent(&ptpStart, &ptpStop);

    for (ptp = *pptpCookie; ; ptp = ptp->NextTreePos())
    {
        if (ptp->IsPointer())
            continue;

        if (ptp->IsNode())
        {
            if (_fIsEditable && ptp->ShowTreePos())
                break;

            pNode = ptp->Branch();
            pElement = pNode->Element();

            if (ptp->IsEndElementScope())
            {
                if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    // Finally, if we have reached the end of our flowlayout
                    // then we need to quit. We have to process the end ptp
                    // since we want the margins of our flow layout to be
                    // reflected in the afterspace info.
                    if (ptp == ptpStop)
                        break;
                }
                // Else do nothing, just continue looking ahead

                // Just verifies that an element is block within itself.
                Assert(ptp != ptpStop);
            }
            else if (ptp->IsBeginElementScope())
            {
                const CCharFormat *pCF = pNode->GetCharFormat();

                if (pCF->IsDisplayNone())
                {
                    pElement->GetTreeExtent(NULL, &ptp);

                    LONG cchHidden = pElement->GetElementCch() + 1;

                    // Add the characters to the line.
                    (*pcchSkip) += cchHidden;
                }

                // We need to stop when we see a new block element
                else if (pFlowLayout->IsElementBlockInContext(pElement))
                {
                    break;
                }
            
                // Or a new layout (including aligned ones, since these
                // will now live on lines of their own.
                else if (pNode->HasLayout() || pNode->IsRelative())
                    break;
                else if (pElement->Tag() == ETAG_BR)
                    break;
                else if (!pNode->Element()->IsNoScope())
                    break;
            }
        }
        else
        {
            Assert(ptp->IsText());
            break;
        }

        // Add the characters to the line.
        (*pcchSkip) += ptp->GetCch();
    }

    if (*pptpCookie != ptp)
        *pptpCookie = ptp;
    }
    Assert(!(*pptpCookie)->IsPointer());

    return S_OK;
}



/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | HiddenBeforeAlignedObj |

    Calculate the number of character positions which contain elements which
    will not be visible when displayed and should therefore not be considered
    in any layout calculations.
    These are elements such as spaces, tabs, comments, and floating objects.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookieTP> is not a valid tree pointer cookie, or
                       <p pcchSkip> is NULL.

@xref	<om .AdvanceBeforeLine>, <om .AdvanceAfterLine>
@contact  alexpf
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::HiddenBeforeAlignedObj(
    DWORD dwCookieTP,   // @parm Tree pointer cookie obtained via a previous call
                        // to  <om .GetTreePointer>. The tree pointer will not be
                        // modified.
    long cpRelOrigLineStart,    // @parm CP position of the presumed start of the line.
    long *pcchSkip)     // @parm Number of characters to advance returned in *<p pcchSkip>.

{
    long cpStart = CpAbsFromCpRel(cpRelOrigLineStart);
    CTreePos **pptpCookie;
	CTreePos *ptp;

    if (!dwCookieTP || !pcchSkip)
        return E_INVALIDARG;

    pptpCookie = (CTreePos **)dwCookieTP;
    *pcchSkip = 0;

    LONG notNeeded;
    SyncTreePointerToCp(pptpCookie, cpStart, &notNeeded);
	ptp = *pptpCookie;

    //
    // WARNING: following code must be kept in sync with CRecalcLinePtr::CalcAlignedSitesAtBOL() cousin
    //

    CFlowLayout *pFlowLayout = m_pFlowLayout;
    CMarkup     *pMarkup = pFlowLayout->GetContentMarkup();

    // Quick bailout if chunck is text unless it is whitespace ofcourse.
    if (ptp->IsText() && ptp->Cch())
    {
        
        TCHAR ch = CTxtPtr(pMarkup, cpStart).GetChar();

        if (!(ch == TEXT(' ') || InRange(ch, TEXT('\t'), TEXT('\r'))))
        {            
            return S_OK;
        }
    }
    
    CElement    *pElementFL  = pFlowLayout->ElementContent();
    BOOL         fAnyAlignedSiteMeasured = FALSE; // Did we find any aligned site at all?

    CTreePos    *ptpLayoutLast;
    pFlowLayout->GetContentTreeExtent(NULL, &ptpLayoutLast);

    LONG         cp = cpStart;        // Current character position
 
    CTreeNode   *pNode = ptp->GetBranch();
    CElement    *pElement = pNode->Element();
    const       CCharFormat *pCF = pNode->GetCharFormat();      // The char format

    
    // Loop over the characters in the line.
    for (;;)
    {
	    SyncTreePointerToCp(pptpCookie, cp, &notNeeded);

		if (ptp == ptpLayoutLast)			// We are at the end.
            break;

        if (ptp->IsPointer())				// Some kind of zero width marker.
        {
            ptp = ptp->NextTreePos();
            continue;
        }

        if (ptp->IsNode())					// Get character type of branch node.
        {
            pNode = ptp->Branch();
            pElement = pNode->Element();
            if (ptp->IsEndNode())
                pNode = pNode->Parent();
            pCF = pNode->GetCharFormat();
        }

        //
        // NOTE(SujalP):
        // pCF should never be NULL since though it starts out as NULL, it should
        // be initialized the first time around in the code above. It has happened
        // in stress once that pCF was NULL. Hence the check for pCF and the assert.
        // (Bug 18065).
        //
        AssertSz(pNode && pElement && pCF, "None of these should be NULL");
        if (!(pNode && pElement && pCF))
            break;

        if (   !_fIsEditable
            && ptp->IsBeginElementScope()
            && pCF->IsDisplayNone()
           )
        {
            cp += pNode->Element()->GetElementCch() + 1;
            pNode->Element()->GetTreeExtent(NULL, &ptp);
        }
        else if (   ptp->IsBeginElementScope()
                 && pNode->HasLayout()
                )
        {
            if (!pElement->IsInlinedElement())
            {
                //
                // Absolutely positioned sites are measured separately
                // inside the measurer. They also count as whitespace and
                // hence we will continue looking for aligned sites after
                // them at BOL.
                //
                if (!pNode->IsAbsolute())
                {
                    fAnyAlignedSiteMeasured = TRUE;
                    break;
                }

                cp += pElement->GetElementCch() + 1;
                pElement->GetTreeExtent(NULL, &ptp);
            }
            else // Has no layout.
            {
                break;
            }
        }
        else if (ptp->IsText())
        {
            CTxtPtr tp(pMarkup, cp);
            LONG cch = ptp->Cch();
            BOOL fNonWhitePresent = FALSE;
            TCHAR ch;
            
            while (cch)
            {
                ch = tp.GetChar();
                //
                // These characters need to be treated like whitespace
                //
                if (!(ch == TEXT(' ') || InRange(ch, TEXT('\t'), TEXT('\r'))))
                {
                    fNonWhitePresent = TRUE;
                    break;
                }
                cch--;
                tp.AdvanceCp(1);
            }
            if (fNonWhitePresent)
                break;
        }
        else if (   ptp->IsEdgeScope()
                 && pElement != pElementFL
                 && (   pFlowLayout->IsElementBlockInContext(pElement)
                     || pElement->Tag() == ETAG_BR
                    )
                )
        {
            break;
        }
        
        //
        // Goto the next tree position
        //
        cp += ptp->GetCch();
        ptp = ptp->NextTreePos();
    }

    *pcchSkip = cp - cpStart;
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | GetLineContext |

	Create a new context. The context is allocated from a global pool of
	memory and can subsequently be associated with any layout element.

@rvalue	S_OK | Success.
@rvalue	E_OUTOFMEMORY | Unable to allocate memory.
@rvalue	E_INVALIDARG | <p pdwCookie> is NULL.

@xref	<om .FreeLineContext>

@todo   This method should be on a global interface.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetLineContext(
	DWORD *pdwCookie)	// @parm Cookie, opaque to caller, returned in *<p pdwCookie>.
						// On failure, *<p pdwCookie> will be set to NULL.
						// The context must be freed by a subsequent call to
						// <om .FreeLineContext>.
{
    CLineServices * pLS;
    CMarkup * pMarkup;

	if (!pdwCookie)
		return E_INVALIDARG;

	*pdwCookie = NULL;

    Assert(m_pFlowLayout);

    // code mostly stolen from InitLineServices cousin

    pMarkup = m_pFlowLayout->GetContentMarkup();
    pLS = new CLineServices(pMarkup);
    if (!pLS)
        return E_OUTOFMEMORY;

    //
    // Runtime sanity check
    //

    Assert(!pLS->_treeInfo._fInited);

    //
    // Return value
    //

    *pdwCookie = (DWORD)pLS;

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | FreeLineContext |

	Destroy the context.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@xref	<om .GetLineContext>

@todo   This method should be on a global interface.
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FreeLineContext(
	DWORD dwCookie)	// @parm Cookie obtained from a prior call to <om .GetLineContext>.
{
    CLineServices * pLS;

    if (!dwCookie)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    delete pLS;

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | SetContext |

	Associate the context with the current layout.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@comm	This call initializes the given global context to be associated with
		the layout element of the callee.

		This call should be matched by a subsequent call to <om .ClearContext>.

@xref	<om .ClearContext>, <om .GetLineContext>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::SetContext(
	DWORD dwCookie)	// @parm Cookie obtained from a prior call to <om .GetLineContext>.
{
    CLineServices * pLS;

    if (!dwCookie)
        return E_INVALIDARG;

    Assert(m_pFlowLayout);

    pLS = (CLineServices *)dwCookie;
    pLS->SetPOLS(m_pFlowLayout, NULL);
    _fIsEditable = m_pFlowLayout->IsEditable();

// sidda: DONT CHECK IN. enable once Trident fixes their _cpLayoutFirst bug
#if REVIEW
    long cpFirst;
    cpFirst = CpAbsFromCpRel(0);
    Assert(pLS->_treeInfo._cpLayoutFirst == cpFirst);

    long cpMac;
    GetLength(&cpMac);
    Assert(pLS->_treeInfo._cpLayoutLast - pLS->_treeInfo._cpLayoutFirst == cpMac);
#endif  // REVIEW

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | ClearContext |

	Reset the context.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@comm	This call resets the context. Should be done after the context is no
		longer in use by the layout element of the callee.

@xref	<om .SetContext>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::ClearContext(
	DWORD dwCookie)	// @parm Cookie obtained from a prior call to <om .GetLineContext>.
{
    CLineServices * pLS;

    if (!dwCookie)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    pLS->ClearPOLS();

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | Setup |

	Prepare the context for line creation.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@comm	This call prepares the context for handling Line Services callbacks
		that will result from an impending call to <f LsCreateLine>.

@xref	<om .SetContext>, <om .GetLineContext>, <om .DiscardLine>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::Setup(
	DWORD dwCookie,		// @parm Cookie obtained from a prior call to <om .GetLineContext>.
	long dxtMaxWidth,	// @parm Width available for line layout.
	long cpStart,		// @parm Starting CP of the line.
	BOOL fMinMaxPass)	// @parm This is a min-max calculation pass.
{
    CLineServices * pLS;
    long cp = CpAbsFromCpRel(cpStart);
    LONG ich;
    CTreePos *ptp;

    if (!dwCookie)
        return E_INVALIDARG;

    Assert(m_pFlowLayout);
    ptp = m_pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);

    pLS = (CLineServices *)dwCookie;
    Assert(!pLS->_treeInfo._fInited);
    pLS->Setup(dxtMaxWidth, cp, ptp, NULL, -1, fMinMaxPass);
    Assert(pLS->_treeInfo._fInited);

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | DiscardLine |

	Reset state information after a line has been formatted.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@comm	This call clears state information such as synthetic CP conversion. It
        must be called when after a line has been formatted, in preparation for
        formatting a subsequent line.

@xref	<om .Setup>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::DiscardLine(
	DWORD dwCookie)	// @parm Cookie obtained from a prior call to <om .GetLineContext>.
{
    CLineServices * pLS;

    if (!dwCookie)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    pLS->DiscardLine();
    pLS->_lineFlags.DeleteAll();    // do this manually
    pLS->_lineCounts.DeleteAll();   // do this manually

	return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | CpRelFromCpLs |

    Convert a Line Services CP to a layout-relative CP.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie or <p pcpRel> is NULL.

@comm   The line services host may generate both synthetic and anti-synthetic
        characters in the information it feeds to Line Services. This call
        enables conversion between CPs seen by Line Services and those actually
        in the document.

@xref	<om .CpLsFromCpRel>, <om .SetContext>, <om .GetLineContext>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::CpRelFromCpLs(
	DWORD dwCookie, // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    long cpLs,      // @parm Line Services CP.
    long *pcpRel)   // @parm Relative CP returned in *<p pcpRel>.
{
    CLineServices * pLS;
    LSCP lscp = CpAbsFromCpRel(cpLs);
    long cpAbs;

    if (!dwCookie || !pcpRel)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    Assert(pLS->_treeInfo._fInited);
    cpAbs = pLS->CPFromLSCP(lscp);
    *pcpRel = CpRelFromCpAbs(cpAbs);

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | CpLsFromCpRel |

    Convert a layout-relative CP to a Line Services CP.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie or <p pcpLs> is NULL.

@comm   The line services host may generate both synthetic and anti-synthetic
        characters in the information it feeds to Line Services. This call
        enables conversion between CPs seen by Line Services and those actually
        in the document.

@xref	<om .CpRelFromCpLs>, <om .SetContext>, <om .GetLineContext>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::CpLsFromCpRel(
	DWORD dwCookie, // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    long cpRel,     // @parm Layout-relative CP.
    long *pcpLs)    // @parm Line Services CP returned in *<p pcpLs>.
{
    CLineServices * pLS;
    long cpAbs = CpAbsFromCpRel(cpRel);
    LSCP lscp;

    if (!dwCookie || !pcpLs)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    Assert(pLS->_treeInfo._fInited);
    lscp = pLS->LSCPFromCP(cpAbs);
    *pcpLs = CpRelFromCpAbs(lscp);

    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | FetchHostRun |

    Fetch a run of document properties and text at the specified CP.

@rvalue	S_OK | Success.
@rvalue	E_OUTOFMEMORY | Unable to allocate memory.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie.

@comm   This method is used to efficiently retrieve document properties and text
        in response to Line Services callbacks. The Line Services host should
        have been initialized by a prior call to <om .Setup>.

@comm   Note that the external layout engine is specifically not interested in
        the host's own PLSRUNs.

@comm   <p pdwFlags> can be zero or more of the following flags:

@flag   FETCHFLAGS_OBJECT | This run is owned by a nested layout object.
@flag   FETCHFLAGS_OWNLINE | The nested layout object should appear on its own line.

@comm   Note that for runs that are owned by a nested layout object, *<p pptpl>
        will always be set to NULL. That is, document properties are not fetched
        for nested layout objects.

@xref	<om .Setup>, <om .GetLineContext>, <i ITextPropertyList>,
        <f pfnFetchRun> Line Services callback, <om .TerminateLineAfterRun>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::FetchHostRun(
	DWORD dwCookie, // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    long cpRelLs,               // @parm CP at which properties should be fetched.
                                // This CP is the CP requested by Line Services, and
                                // is layout-relative.
    LPCWSTR *ppwchRun,          // @parm See Line Services <f pfnFetchRun> callback.
    DWORD *pcchRun,             // @parm See Line Services <f pfnFetchRun> callback.
    BOOL *pfHidden,             // @parm See Line Services <f pfnFetchRun> callback.
    void *plsChp,               // @parm See Line Services <f pfnFetchRun> callback.
                                // This is really of type PLSCHP.
    long propertyGroup,         // @parm Property group to be fetched in *<p pptpl>.
                                // Ignored if <p pptpl> is NULL.
    ITextPropertyList **pptpl,  // @parm If <p pptpl> is non-NULL, property list
                                // returned in *<p pptpl>. If this is a nested layout
                                // run, *<p pptpl> will be set to NULL.
    void **ppvHostRun,          // @parm If <p ppvHostRun> is non-NULL, pointer to host
                                // run returned in *<p ppvHostRun>.
    DWORD *pdwFlags)            // @parm If <p pdwFlags> is non-NULL, formatting flags.
                                // See below for a list of values.
{
    CLineServices * pLS;
    long cpLs = CpAbsFromCpRel(cpRelLs);
    LSERR lserr;
    COneRun *por = NULL;
    HRESULT hr = S_OK;

    if (!dwCookie)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    Assert(pLS->_treeInfo._fInited);

    if (pptpl)
        *pptpl = NULL;

    if (ppvHostRun)
        *ppvHostRun = NULL;

    if (pdwFlags)
        *pdwFlags = 0;

    lserr = pLS->FetchRun(cpLs, ppwchRun, pcchRun, pfHidden, (PLSCHP)plsChp, &por);
    if (lserrNone == lserr && (pptpl || pdwFlags))
    {
        Assert(por);

        if (ppvHostRun)
            *ppvHostRun = (void *)por;

        if (por->_fCharsForNestedLayout)
        {
#if DBG==1
            LONG cpRel;
            CpRelFromCpLs(dwCookie, cpRelLs, &cpRel);
            Assert(IsNestedLayoutAtCp(0, cpRel) == S_OK);
#endif
            if (pdwFlags)
            {
                *pdwFlags |= FETCHFLAGS_OBJECT;
                if (pLS->IsOwnLineSite(por))
                    *pdwFlags |= FETCHFLAGS_OWNLINE;
            }
        }
        else if (pptpl)
        {
        CParentInfo PI;
        PI.Init(m_pFlowLayout);

        hr = TLS(_pQLM)->CreateTextPropertyList(pptpl);
        if (FAILED(hr))
            goto LExit;

        if (propertyGroup == propertyGroupChar)
            {
            CCharFormatPropertyAccess propaccessCF;

            propaccessCF.put_ParentInfo(&PI);
            propaccessCF.put_This(por->GetCF());
            propaccessCF.put_CF(por->GetCF());
            propaccessCF.put_FF(NULL);

            hr = (*pptpl)->GetFullList(&dictCF, &propaccessCF);
            }
        else
            {
            Assert(propertyGroup == propertyGroupPara);
            CParaFormatPropertyAccess propaccessPF;

            propaccessPF.put_ParentInfo(&PI);
            propaccessPF.put_This(por->GetPF());
            propaccessPF.put_CF(por->GetCF());
            propaccessPF.put_FF(NULL);

            hr = (*pptpl)->GetFullList(&dictPF, &propaccessPF);
            }
        }
    }

LExit:
    // note that the external layout engine may hold onto the host's PLSRUNs

    if (FAILED(hr))
        return hr;

    return (lserrNone == lserr ? S_OK : E_OUTOFMEMORY);
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | TerminateLineAfterRun |

    Force line to be terminated after the specified run.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie, or <p pvHostRun> is NULL.
@rvalue E_OUTOFMEMORY | Ran out of memory.

@comm   Called to terminate a line after an object which is supposed to be on
        its own line, such as a table.

@xref	<om .FetchHostRun>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::TerminateLineAfterRun(
	DWORD dwCookie, // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    void *pvHostRun)    // @parm Opaque pointer to host run obtained from a prior call
                        // to <om .FetchHostRun>.
{
    CLineServices * pLS;
    COneRun *plsrun;

    if (!dwCookie || !pvHostRun)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;
    Assert(pLS->_treeInfo._fInited);

    plsrun = (COneRun *)pvHostRun;

    // following code is duplicated from CEmbeddedILSObj::Fmt()
    {
        COneRun *porOut;
        COneRun *por = pLS->_listFree.GetFreeOneRun(plsrun);

        if (!por)
            return E_OUTOFMEMORY;

        Assert(plsrun->IsNormalRun());
        Assert(plsrun->_lscch == plsrun->_lscchOriginal);
        por->_lscpBase += plsrun->_lscch;
        
        // If this object has to be on its own line, then it clearly
        // ends the current line.
        if (lserrNone != pLS->TerminateLine(por, TRUE, &porOut))
            return E_OUTOFMEMORY;

        // Free the one run
        pLS->_listFree.SpliceIn(por);
    }
    return S_OK;
}

/*----------------------------------------------------------------------------
@method HRESULT | ILineServicesHost | GetLineSummaryInfo |

    Determine line formatting characteristics up to the specified CP.

@rvalue	S_OK | Success.
@rvalue	E_INVALIDARG | <p dwCookie> is not a valid cookie, or <p pdwFlags> is NULL.

@comm   *<p pdwFlags> will be set to zero or more of the following flags:

@flag   LINE_FLAG_NONE (0x000) | No flags set.
@flag   LINE_FLAG_HAS_ABSOLUTE_ELT (0x001) | Absolute element.
@flag   LINE_FLAG_HAS_ALIGNED (0x002) | Aligned element.
@flag   LINE_FLAG_HAS_EMBED_OR_WBR (0x004) | .
@flag   LINE_FLAG_HAS_NESTED_RO (0x008) |  Nested run owner, such as a table.
@flag   LINE_FLAG_HAS_BREAK_CHAR (0x010) | .
@flag   LINE_FLAG_HAS_BACKGROUND (0x020) | .
@flag   LINE_FLAG_HAS_A_BR (0x040) | .
@flag   LINE_FLAG_HAS_RELATIVE (0x080) | .
@flag   LINE_FLAG_HAS_NBSP (0x100) | .
@flag   LINE_FLAG_HAS_NOBLAST (0x0200) | .
@flag   LINE_FLAG_HAS_CLEARLEFT (0x0400) | .
@flag   LINE_FLAG_HAS_CLEARRIGHT (0x0800) | .

@xref	<om .GetLineContext>
----------------------------------------------------------------------------*/
STDMETHODIMP
CQuillGlue::GetLineSummaryInfo(
	DWORD dwCookie, // @parm Cookie obtained from a prior call to <om .GetLineContext>.
    long cpRelLineLim,  // @parm Lim cp up to which summary information should be provided.
    DWORD *pdwFlags)    // @parm If <p pdwFlags> is non-NULL, line formatting flags.
                        // See below for a list of values.
{
    CLineServices * pLS;
    long cpAbs = CpAbsFromCpRel(cpRelLineLim);

    // keep our constants in sync
    Assert(FLAG_NONE == LINE_FLAG_NONE);
    Assert(FLAG_HAS_ABSOLUTE_ELT == LINE_FLAG_HAS_ABSOLUTE_ELT);
    Assert(FLAG_HAS_ALIGNED == LINE_FLAG_HAS_ALIGNED);
    Assert(FLAG_HAS_EMBED_OR_WBR == LINE_FLAG_HAS_EMBED_OR_WBR);
    Assert(FLAG_HAS_NESTED_RO == LINE_FLAG_HAS_NESTED_RO);
    Assert(FLAG_HAS_BREAK_CHAR == LINE_FLAG_HAS_BREAK_CHAR);
    Assert(FLAG_HAS_BACKGROUND == LINE_FLAG_HAS_BACKGROUND);
    Assert(FLAG_HAS_A_BR == LINE_FLAG_HAS_A_BR);
    Assert(FLAG_HAS_RELATIVE == LINE_FLAG_HAS_RELATIVE);
    Assert(FLAG_HAS_NBSP == LINE_FLAG_HAS_NBSP);
    Assert(FLAG_HAS_NOBLAST == LINE_FLAG_HAS_NOBLAST);
    Assert(FLAG_HAS_CLEARLEFT == LINE_FLAG_HAS_CLEARLEFT);
    Assert(FLAG_HAS_CLEARRIGHT == LINE_FLAG_HAS_CLEARRIGHT);

    if (!dwCookie || !pdwFlags)
        return E_INVALIDARG;

    pLS = (CLineServices *)dwCookie;

    *pdwFlags = pLS->_lineFlags.GetLineFlags(cpAbs);
    return S_OK;
}

