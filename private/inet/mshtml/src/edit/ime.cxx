/*
 *  @doc    INTERNAL
 *
 *  @module ime.cpp -- support for Win95 IME API |
 *
 *      Most everything to do with FE composition string editing passes
 *      through here.
 *
 *  Authors: <nl>
 *      Jon Matousek <nl>
 *      Hon Wah Chan <nl>
 *      Justin Voskuhl <nl>
 *
 *  History: <nl>
 *      10/18/1995      jonmat  Cleaned up level 2 code and converted it into
 *                              a class hierarchy supporting level 3.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 *
 */

#include "headers.hxx"

#ifndef NO_IME /*{*/

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "OptsHold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMANK_HXX_
#include "selman.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef X__IME_HXX_
#define X__IME_HXX_
#include "ime.hxx"
#endif

DeclareTag(tagEdIME, "!!InputMethodEditor", "Trace Input Method Editor")
DeclareTag(tagEdIMEAttr, "!!InputMethodEditor", "Trace Attributes in CompositionString")

MtDefine(CIme, Dialogs, "CIme")

// default caret width
#define DXCARET 1

BOOL forceLevel2 = FALSE;

#if DBG==1
static const LPCTSTR strImeHighlightStart = _T( "    ** IME Highlight Start");
static const LPCTSTR strImeHighlightEnd = _T( "    ** IME Highlight End");
#endif

#define _TODO(x)

//
// LPARAM helpers
//

inline BOOL HaveCompositionString(LONG lparam) { return ( 0 != (lparam & (GCS_COMPSTR | GCS_COMPATTR))); }
inline BOOL CleanupCompositionString(LONG lparam) { return ( 0 == lparam ); }
inline BOOL HaveResultString(LONG lparam) { return (0 != (lparam & GCS_RESULTSTR)); }

//+--------------------------------------------------------------------------
//
//  method:     IsIMEComposition
//
//  returns:    BOOLEAN - Return TRUE if we have a IME object on the textsite.
//              If argument is FALSE, only return TRUE if the IME object is
//              not of class CIme_Protected.
//
//---------------------------------------------------------------------------

BOOL
CSelectionManager::IsIMEComposition( BOOL fProtectedOK /* = TRUE */ )
{
    return (_pIme != NULL && (fProtectedOK || !_pIme->IsProtected()));
};

//+--------------------------------------------------------------------------
//
//  method:     ImmGetContext
//
//  returns:    Get the IMM context associated with the document window
//
//---------------------------------------------------------------------------

HIMC
CSelectionManager::ImmGetContext(void)
{
    HWND hwnd;
    HRESULT hr = THR( GetEditor()->GetViewServices()->GetViewHWND( &hwnd ) );
    Assert( hwnd );
    return OK(hr) ? ::ImmGetContext( hwnd ) : NULL;
}

//+--------------------------------------------------------------------------
//
//  method:     ImmReleaseContext
//
//  returns:    Release the IMM context associated with the document window
//
//---------------------------------------------------------------------------

void
CSelectionManager::ImmReleaseContext( HIMC himc )
{
    HWND hwnd;
    HRESULT hr = THR( GetEditor()->GetViewServices()->GetViewHWND( &hwnd ) );
    Assert( hwnd );
    if (OK(hr))
    {
        ::ImmReleaseContext( hwnd, himc );
    }
}

//+-----------------------------------------------------------------------------
//
//  method:     HandleImeComposition
//
//  synopsis:   Handle a 'naked' WM_IME_COMPOSITION message.  Naked implies
//              that this IME message came *after* the WM_IME_ENDCOMPOSITION
//              message.
//
//------------------------------------------------------------------------------

HRESULT
CSelectionManager::HandleImeComposition( SelectionMessage *pMessage )
{
    HRESULT hr = S_OK;
    CImeDummy ime(this);
    
    // The IME functions will want to know this.

    _codepageKeyboard = GetKeyboardCodePage();

    ime.CheckInsertResultString( pMessage->lParam );

    RRETURN( hr );
}

//+--------------------------------------------------------------------------
//
//  method:     SetCaretVisible
//
//---------------------------------------------------------------------------

HRESULT
CIme::SetCaretVisible( BOOL fVisible )
{
    HRESULT hr;

    if (_fCaretVisible != fVisible)
    {
        hr = THR(SetCaretVisible( _pManager->GetDoc(), _fCaretVisible = fVisible ));
    }
    else
    {
        hr = S_OK;
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  HRESULT StartCompositionGlue( BOOL fIsProtected, TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Initiates an IME composition string edit.
//  @comm
//      Called from the message loop to handle WM_IME_STARTCOMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//
//  @devnote
//      We decide if we are going to do a level 2 or level 3 IME
//      composition string edit. Currently, the only reason to
//      create a level 2 IME is if the IME has a special UI, or it is
//      a "near caret" IME, such as the ones found in PRC and Taiwan.
//      Near caret simply means that a very small window opens up
//      near the caret, but not on or at the caret.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::StartCompositionGlue(
    BOOL fIsProtected,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    CSelectTracker * pSelectTracker = NULL;

    // note that in some locales (PRC), we may still be in composition mode
    // when a new start composition call comes in.  Just reset our state
    // and go on.

    _codepageKeyboard = GetKeyboardCodePage();
    _fIgnoreImeCharMsg = FALSE;

    if ( !IsIMEComposition() )
    {
        if (   _pActiveTracker
            && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Selection)
        {
            pSelectTracker = DYNCAST( CSelectTracker, _pActiveTracker );

            if (!pSelectTracker->EndPointsInSameFlowLayout())
            {
                fIsProtected = TRUE;
            }
        }

        if (   _pActiveTracker
            && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Control )
        {
            fIsProtected = TRUE;
        }            
        
        if ( fIsProtected )
        {
            // protect or read-only, need to ignore all ime input

            _pIme = new CIme_Protected(this);
        }
        else
        {
            // if a special UI, or IME is "near caret", then drop into lev. 2 mode.

        #ifdef MACPORT /*{*/
            if ( !forceLevel2 )
            {
                _pIme = new CIme_Lev3( this );     // level 3 IME->TrueInline.
            }
        #else
            DWORD imeProperties = ImmGetProperty( GetKeyboardLayout(0), IGP_PROPERTY );

            if (    0 != ( imeProperties & IME_PROP_SPECIAL_UI )
                 || 0 == ( imeProperties & IME_PROP_AT_CARET )
                 || forceLevel2 )
            {
                _pIme = new CIme_Lev2( this );     // level 2 IME.
            }
            else
            {
                _pIme = new CIme_Lev3( this );     // level 3 IME->TrueInline.
            }
        #endif /*}*/
        }

        if (_pIme == NULL)
            return E_OUTOFMEMORY;
    }

    if ( IsIMEComposition() )                    // make the method call.
    {
        hr = _pIme->StartComposition();
        if (peTrackerNotify)
        {
            if (pSelectTracker)
            {
                if (!fIsProtected)
                {
                    *peTrackerNotify = TN_FIRE_DELETE_REBUBBLE;
                }
            }
            else if ( !fIsProtected &&
                     ( !_pActiveTracker
                       || _pActiveTracker->GetSelectionType() != SELECTION_TYPE_Caret) )
            {
                *peTrackerNotify = TN_END_TRACKER_CREATE_CARET;
            }
        }
    }

    RRETURN1(hr,  S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT CompositionStringGlue( const LPARAM lparam, TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Handle all intermediary and final composition strings.
//
//  @comm
//      Called from the message loop to handle WM_IME_COMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//      We may be called independently of a WM_IME_STARTCOMPOSITION
//      message, in which case we return S_FALSE to allow the
//      DefWindowProc to return WM_IME_CHAR messages.
//
//  @devnote
//      Side Effect: the _ime object may be deleted if composition
//      string processing is finished.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::CompositionStringGlue(
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr;

    _fIgnoreImeCharMsg = FALSE;

    if ( IsIMEComposition() )
    {
        _pIme->_compMessageRefCount++;            // For proper deletion.

        hr = _pIme->CompositionString(lparam, peTrackerNotify);

        _pIme->_compMessageRefCount--;            // For proper deletion.

        Assert ( _pIme->_compMessageRefCount >= 0);

        CheckDestroyIME(peTrackerNotify);         // Finished processing?
    }
    else if ( IsImeEditable() )
    {
        hr = S_FALSE;

        // even when not in composition mode, we may receive a result string.

        if (peTrackerNotify && _pActiveTracker)
        {
            if (_pActiveTracker->GetSelectionType() == SELECTION_TYPE_Selection)
            {
                CSelectTracker * pSelectTracker = DYNCAST( CSelectTracker, _pActiveTracker );

                if (pSelectTracker->EndPointsInSameFlowLayout())
                {
                    *peTrackerNotify = TN_FIRE_DELETE_REBUBBLE;
                }
            }
            else if (_pActiveTracker->GetSelectionType() == SELECTION_TYPE_Caret)
            {
                CImeDummy ime( this );

                _fIgnoreImeCharMsg = TRUE; // Ignore the next WM_IME_CHAR message
                
                hr = THR( ime.CheckInsertResultString( lparam ) );
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT EndCompositionGlue( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Composition string processing is about to end.
//
//  @comm
//      Called from the message loop to handle WM_IME_ENDCOMPOSITION.
//      This is a glue routine into the IME object hierarchy.
//
//  @devnote
//      The only time we have to handle WM_IME_ENDCOMPOSITION is when the
//      user changes input method during typing.  For such case, we will get
//      a WM_IME_ENDCOMPOSITION message without getting a WM_IME_COMPOSITION
//      message with GCS_RESULTSTR later.  So, we will call CompositionStringGlue
//      with GCS_RESULTSTR to let CompositionString to get rid of the string.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::EndCompositionGlue( TRACKER_NOTIFY * peTrackerNotify )
{
    if ( IsIMEComposition() )
    {
        // set this flag. If we are still in composition mode, then
        // let the CompositionStringGlue() to destroy the ime object.
        _pIme->_fDestroy = TRUE;

        // remove any remaining composition string.
        CompositionStringGlue( GCS_COMPSTR, peTrackerNotify );

        // finished with IME, destroy it.
        CheckDestroyIME( peTrackerNotify );
    }

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  void CheckDestroyIME ( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Check for IME and see detroy if it needs it..
//
//-----------------------------------------------------------------------------

void
CSelectionManager::CheckDestroyIME(TRACKER_NOTIFY * peTrackerNotify)
{
    if ( IsIMEComposition() && _pIme->_fDestroy )
    {
        if ( 0 == _pIme->_compMessageRefCount )
        {
//            BOOL bKorean = _pIme->IsKoreanMode();

            delete _pIme;
            _pIme = NULL;

#if TODO /*{*/
            if ( bKorean )
            {
                // reset the caret position
                DWORD cp = psel->GetCp();
                psel->SetSelection( cp, cp, TRUE,* pIT);
                psel->AdjustForInsert();

            }
#endif /*}*/

            // We need to kill the CImeTracker, and then create a CCaretTracker.
            // If another WM_IME_STARTCOMPOSITION message comes through, we will
            // create another CImeTracker.

            if (peTrackerNotify && 
				! ( _pActiveTracker && _pActiveTracker->GetSelectionType() == SELECTION_TYPE_Control ) )
            {
                *peTrackerNotify = TN_END_TRACKER_CREATE_CARET;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  void PostIMECharGlue()
//
//  @func
//      Called after processing a single WM_IME_CHAR in order to
//      update the position of the IME's composition window. This
//      is glue code to call the CIME virtual equivalent.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::PostIMECharGlue( TRACKER_NOTIFY * peTrackerNotify )
{
    if ( IsIMEComposition() )
    {
        _pIme->PostIMEChar();
    }

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  void CompositionFullGlue( TRACKER_NOTIFY * peTrackerNotify )
//
//  @func
//      Current IME Composition window is full.
//
//  @comm
//      Called from the message loop to handle WM_IME_COMPOSITIONFULL.
//      This message applied to Level 2 only.  We will use the default
//      IME Composition window.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::CompositionFullGlue( TRACKER_NOTIFY * peTrackerNotify )
{
#ifndef MACPORT /*{*/
    if ( IsIMEComposition() )
    {
        HIMC hIMC = ImmGetContext();

        if ( hIMC )
        {
            COMPOSITIONFORM cf;

            // no room for text input in the current level 2 IME window,
            // fall back to use the default IME window for input.

            cf.dwStyle = CFS_DEFAULT;
            ImmSetCompositionWindow( hIMC, &cf );  // Set composition window.
            ImmReleaseContext( hIMC );             // Done with IME context.
        }
    }
#endif /*}*/

    return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  function:   CSelectionManager::ClearHighlightSegments()
//
//-----------------------------------------------------------------------------

HRESULT
CIme::ClearHighlightSegments()
{
    HRESULT hr = THR( _pSelRenSvc->ClearSegments( TRUE ));

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  function:   CSelectionManager::AddHighlightSegment()
//
//-----------------------------------------------------------------------------

HRESULT
CIme::AddHighlightSegment(
    LONG index,
    BOOL fLast,
    LONG ichMin,
    LONG ichMost,
    HIGHLIGHT_TYPE HighlightType )
{
    HRESULT         hr;
    IMarkupServices * pMarkup = NULL;
    CEditPointer    epStart(_pManager->GetEditor());
    CEditPointer    epEnd(_pManager->GetEditor());
    DWORD           dwSearch = BREAK_CONDITION_Text;
    DWORD           dwFound;
    int             ich;
    int             iSegment;


    Assert(   HighlightType == HIGHLIGHT_TYPE_ImeInput
           || HighlightType == HIGHLIGHT_TYPE_ImeTargetConverted
           || HighlightType == HIGHLIGHT_TYPE_ImeConverted
           || HighlightType == HIGHLIGHT_TYPE_ImeTargetNotConverted
           || HighlightType == HIGHLIGHT_TYPE_ImeInputError
           || HighlightType == HIGHLIGHT_TYPE_ImeHangul );

    Assert( ichMost >= ichMin );

    hr = THR( _pManager->GetDoc()->QueryInterface(IID_IMarkupServices, (void**) & pMarkup ));
    if (hr)
        goto Cleanup;

    hr = THR( epStart->MoveToPointer( _pmpStartUncommitted ) );

    if (hr)
        goto Cleanup;

    for (ich = 0; ich < ichMin; ich++)
    {
        hr = THR( epStart.Scan(RIGHT, dwSearch, &dwFound) );
        if (hr)
            goto Cleanup;

        if (!epStart.CheckFlag(dwFound, dwSearch))
            break;
    }

    hr = THR( epEnd->MoveToPointer( epStart ) );
    if (hr)
        goto Cleanup;

    for (; ich < ichMost; ich++)
    {
        hr = THR( epStart.Scan(RIGHT, dwSearch, &dwFound) );
        if (hr)
            goto Cleanup;

        if (!epStart.CheckFlag(dwFound, dwSearch))
            break;
    }        

    {
        HRESULT hr;
        int iSegmentCount;
        SELECTION_TYPE eSelectionType;

        hr = _pSelRenSvc->GetSegmentCount( &iSegmentCount, &eSelectionType );
        if (hr)
            goto Cleanup;
    }
        
    hr = THR( _pSelRenSvc->AddSegment( epStart, epEnd, HighlightType, &iSegment ));
    if (hr)
        goto Cleanup;

Cleanup:

    ReleaseInterface( pMarkup );

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  INT CIme::GetCompositionStringInfo( CCaretTracker * pCT, DWORD dwIndex,
//            WCHAR * achCompStr, INT cchMax, BYTE * pbAttrib, INT cbAttrib
//            LONG * pcchAttrib )
//
//  @mfunc
//      For WM_IME_COMPOSITION string processing to get the requested
//      composition string, by type, and convert it to Unicode.
//
//  @devnote
//      We must use ImmGetCompositionStringA because W is not supported
//      on Win95.
//
//  @rdesc
//      INT-cch of the Unicode composition string.
//      Out param in UniCompStr.
//
//-----------------------------------------------------------------------------

INT
CIme::GetCompositionStringInfo(
    DWORD dwIndex,      // @parm The type of composition string.
    WCHAR *pchCompStr,  // @parm Out param, unicode result string.
    INT cchMax,         // @parm The cch for the Out param.
    BYTE *pbAttrib,     // @parm Out param, If attribute info is needed.
    INT cbMax,          // @parm The cb of the attribute info.
    LONG *pichCursor,   // @parm Out param, returns the CP of cusor.
    LONG *pcchAttrib )  // @parm how many attributes returned.
{
    BYTE abCompStr[256];
    INT cbCompStr, cchCompStr;
    HIMC hIMC = ImmGetContext();

    if (hIMC)
    {
        const BOOL fIsOnNT = _pManager->IsOnNT();
        
        Assert ( pchCompStr );

        AssertSz(    dwIndex == GCS_COMPREADSTR
                  || dwIndex == GCS_COMPSTR
                  || dwIndex == GCS_RESULTREADSTR
                  || dwIndex == GCS_RESULTSTR,
                  "String function expected" );

        if ( pichCursor )                                 // Init cursor out param.
           *pichCursor = -1;
        if ( pcchAttrib )
           *pcchAttrib = 0;
                                                        // Get composition string.
        if (fIsOnNT)
        {
            cbCompStr = ImmGetCompositionStringW( hIMC, dwIndex, pchCompStr, ( cchMax - 1 ) * sizeof(WCHAR) );
            cchCompStr = (cbCompStr > 0) ? (cbCompStr/sizeof(TCHAR)) : cbCompStr;
            pchCompStr[cchCompStr] = 0;
        }
        else
        {
            cbCompStr = ImmGetCompositionStringA( hIMC, dwIndex, abCompStr, sizeof(abCompStr) - 1 );

            if (cbCompStr > 0 && *abCompStr)              // If valid data.
            {
                Assert ( cbCompStr >> 1 < cchMax - 1 ); // Convert to Unicode.
                cchCompStr = MultiByteToWideChar( _pManager->KeyboardCodePage(), 0,
                                                  (CHAR *) abCompStr, cbCompStr,
                                                  pchCompStr, cchMax );
                pchCompStr[cchCompStr] = 0;
            }
            else
            {
                cchCompStr = cbCompStr;
            }
        }

        if ( cchCompStr > 0 && *pchCompStr )            // If valid data.
        {
            if ( pbAttrib || pichCursor )               // Need cursor or attribs?
            {
                INT ichCursor=0, cchAttrib;

                if (fIsOnNT)
                {
                    ichCursor = ImmGetCompositionStringW( hIMC, GCS_CURSORPOS, NULL, 0 );
                    cchAttrib = ImmGetCompositionStringW( hIMC, GCS_COMPATTR, pbAttrib, cbMax );
                }
                else
                {
                    INT ib, ich;
                    INT ibCursor, ibMax, cbAttrib;
                    BYTE abAttribLocal[256];
                    BYTE * pbAttribPtr = pbAttrib;

                    ibCursor = ImmGetCompositionStringA( hIMC, GCS_CURSORPOS, NULL, 0 );
                    cbAttrib = ImmGetCompositionStringA( hIMC, GCS_COMPATTR, abAttribLocal, 255 );

                                                        // MultiToWide conversion.
                    ibMax = max( ibCursor, cbAttrib );
                    if ( NULL == pbAttrib ) cbMax = cbAttrib;

                    for (ib = 0, ich = 0; ib <= ibMax && ich < cbMax; ib++, ich++ )
                    {
                        if ( ibCursor == ib )           // Cursor from DBCS.
                            ichCursor = ich;

                        if ( IsDBCSLeadByteEx( KeyboardCodePage(), abCompStr[ib] ) )
                            ib++;

                        if ( pbAttribPtr && ib < cbAttrib )  // Attrib from DBCS.
                            *pbAttribPtr++ = abAttribLocal[ib];
                    }

                    cchAttrib = ich - 1;
                }

                if ( ichCursor >= 0 && pichCursor )     // If client needs cursor
                    *pichCursor = ichCursor;            //  or cchAttrib.
                if ( cchAttrib >= 0 && pcchAttrib )
                    *pcchAttrib = cchAttrib;
            }
        }
        else
        {
            if ( pichCursor )
                *pichCursor = 0;
            cchCompStr = 0;
        }

        ImmReleaseContext(hIMC);
    }
    else
    {
        cchCompStr = 0;
    }

    return cchCompStr;
}

//+----------------------------------------------------------------------------
//  void CIme::SetCompositionFont ( BOOL *pfUnderLineMode )
//
//  @mfunc
//      Important for level 2 IME so that the composition window
//      has the correct font. The lfw to lfa copy is due to the fact that
//      Win95 does not support the W)ide call.
//      It is also important for both level 2 and level 3 IME so that
//      the candidate list window has the proper. font.
//
//-----------------------------------------------------------------------------

void /* static */
CIme::SetCompositionFont (
    BOOL     *pfUnderLineMode)  // @parm the original char Underline mode
{
    // BUGBUG (cthrash) We don't support this currently.  Didn't in IE4 either.

    pfUnderLineMode = FALSE;
}

//+----------------------------------------------------------------------------
//
//  funtion:    CIme::GetCompositionPos( POINT * ppt, RECT * prc )
//
//  synopsis:   Determine the position for composition windows, etc.
//
//-----------------------------------------------------------------------------

HRESULT
CIme::GetCompositionPos(
    POINT * ppt,
    RECT * prc,
    long * plLineHeight )
{
    HTMLPtrDispInfoRec DispInfoRec;
    HWND hwnd;
    HRESULT hr;
    SP_IHTMLElement spElement;
    SP_IHTMLCaret spCaret;
    CEditPointer epPos( _pManager->GetEditor() );
    
    Assert(ppt && prc && plLineHeight);

    //
    // We get the line dimensions at the position of the caret. I realize we could
    // get some of the data from the caret, but we have to call through this way
    // anyway to get the descent and line height.
    //

    IFC( GetViewServices()->GetCaret( & spCaret ));
    IFC( spCaret->MovePointerToCaret( epPos ));
    IFC( GetViewServices()->GetLineInfo( epPos, FALSE, &DispInfoRec ));
    ppt->x = DispInfoRec.lXPosition;
    ppt->y = DispInfoRec.lBaseline + DispInfoRec.lTextDescent;
    *plLineHeight = DispInfoRec.lTextHeight;

    IFC( GetViewServices()->GetFlowElement( _pmpInsertionPoint, & spElement ));
    if ( ! spElement )
    {
        IFC( _pManager->GetEditableElement( & spElement ));
    }

    IFC( GetViewServices()->TransformPoint( ppt, COORD_SYSTEM_CONTENT, COORD_SYSTEM_GLOBAL, spElement ));
    IFC( GetViewServices()->GetViewHWND( &hwnd ) );

    ::GetClientRect(hwnd, prc);

Cleanup:
    RRETURN(hr);    
}

//+----------------------------------------------------------------------------
//  void CIme::SetCompositionForm ()
//
//  @mfunc
//      Important for level 2 IME so that the composition window
//      is positioned correctly.
//
//  @comm
//      We go through a lot of work to get the correct height. This requires
//      getting information from the font cache and the selection.
//
//-----------------------------------------------------------------------------

void
CIme::SetCompositionForm()
{
#ifndef MACPORT /*{*/
    if ( IME_LEVEL_2 == GetIMELevel() )
    {
        HIMC hIMC = ImmGetContext();                // Get host's IME context.

        if ( hIMC )
        {
            COMPOSITIONFORM cf;
            long lLineHeight;
            HRESULT hr = THR( GetCompositionPos( &cf.ptCurrentPos, &cf.rcArea, &lLineHeight ) );

            if (OK(hr))
            {

                // Bounding rect for the IME (lev 2) composition window, causing
                //  composition text to be wrapped within it.
                cf.dwStyle = CFS_POINT + CFS_FORCE_POSITION;

                // Make sure the starting point is not
                // outside the rcArea.  This happens when
                // there is no text on the current line and the user
                // has selected a large font size.

                if (cf.ptCurrentPos.y < cf.rcArea.top)
                    cf.ptCurrentPos.y = cf.rcArea.top;
                else if (cf.ptCurrentPos.y > cf.rcArea.bottom)
                    cf.ptCurrentPos.y = cf.rcArea.bottom;

                if (cf.ptCurrentPos.x < cf.rcArea.left)
                    cf.ptCurrentPos.x = cf.rcArea.left;
                else if (cf.ptCurrentPos.x > cf.rcArea.right)
                    cf.ptCurrentPos.x = cf.rcArea.right;

                ImmSetCompositionWindow( hIMC, &cf );  // Set composition window.
            }

            ImmReleaseContext( hIMC );             // Done with IME context.
        }
    }
#endif /*}*/
}



//+----------------------------------------------------------------------------
//
//  CIme::TerminateIMEComposition ( TerminateMode mode )
//
//  @mfunc  Terminate the IME Composition mode using CPS_COMPLETE
//  @comm   The IME will generate WM_IME_COMPOSITION with the result string
//
//
//-----------------------------------------------------------------------------

void
CIme::TerminateIMEComposition(
    DWORD dwMode,
    TRACKER_NOTIFY * peTrackerNotify )
{
    TraceTag((tagEdIME, "terminate"));

    DWORD dwTerminateMethod = CPS_COMPLETE;

    if (    IME_LEVEL_2 == GetIMELevel()    // force cancel for near-caret IME
         || dwMode == TERMINATE_FORCECANCEL   // caller wants force cancel
         || _pManager->IsImeCancelComplete() )            // Client wants force cancel
    {
        dwTerminateMethod = CPS_CANCEL;
    }

    HIMC hIMC = ImmGetContext();

    // force the IME to terminate the current session
    if (hIMC)
    {
        _compMessageRefCount++; // primitive addref

        BOOL retCode = ImmNotifyIME( hIMC, NI_COMPOSITIONSTR, dwTerminateMethod, 0 );

        if ( !retCode && !_pManager->IsImeCancelComplete() )
        {
            // CPS_COMPLETE fail, try CPS_CANCEL.  This happen with some ime which do not support
            // CPS_COMPLETE option (e.g. ABC IME version 4 with Win95 simplified Chinese)

            retCode = ImmNotifyIME( hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
        }

        Assert ( retCode );

        ImmReleaseContext( hIMC );

        _compMessageRefCount--; // primitive release

        _pManager->CheckDestroyIME(peTrackerNotify);
    }
    else
    {
        // for some reason, we didn't have a context, yet we thought we were still in IME
        // compostition mode.  Just force a shutdown here.

        _pManager->EndCompositionGlue(peTrackerNotify);
    }
}


//+----------------------------------------------------------------------------
//
//  CIme::CIme
//
//-----------------------------------------------------------------------------

CIme::CIme( CSelectionManager * pManager )
: _undoUnit(pManager->GetEditor()),
  _pManager(pManager)
{
    if (pManager)
        _undoUnit.Begin(IDS_EDUNDOTYPING);

    Init();
}

CIme::~CIme()
{
    Deinit();
}

//+----------------------------------------------------------------------------
//
//  CIme_Lev2::CIme_Lev2()
//
//  @mfunc
//      CIme_Lev2 Constructor/Destructor.
//
//  @comm
//      Needed to make sure _iFormatSave was handled properly.
//
//-----------------------------------------------------------------------------

CIme_Lev2::CIme_Lev2( CSelectionManager * pManager ) : CIme( pManager )     // @parm the containing text edit.
{
    LONG    iFormat = 0;

    SetIMECaretWidth( DXCARET );           // setup initial caret width
    _iFormatSave = iFormat;

    // hold notification unless client has set IMF_IMEALWAYSSENDNOTIFY via EM_SETLANGOPTIONS msg
    _fHoldNotify = !pManager->IsImeAlwaysNotify();
}

CIme_Lev2::~CIme_Lev2()
{
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::StartComposition()
//
//  @mfunc
//      Begin IME Level 2 composition string processing.
//
//  @comm
//      Set the font, and location of the composition window which includes
//      a bounding rect and the start position of the cursor. Also, reset
//      the candidate window to allow the IME to set its position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------


HRESULT
CIme_Lev2::StartComposition( )
{
    TraceTag((tagEdIME, "start (level2)"));

    _imeLevel = IME_LEVEL_2;

    SetCompositionFont( &_fUnderLineMode );     // Set font, & comp window.
    SetCompositionForm( );

    // hide the caret since the IME is displays its own caret in level 2 comp.
    SetCaretVisible( FALSE );

    return S_FALSE;                                 // Allow DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::CompositionString( LPARAM lparam )
//
//  @mfunc
//      Handle Level 2 WM_IME_COMPOSITION messages.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing.
//
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//          The Host needs to mask out the lparam before calling DefWindowProc to
//          prevent unnessary WM_IME_CHAR messages.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev2::CompositionString (
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    TraceTag((tagEdIME, "compstr (%x)", lparam));

    _fIgnoreIMEChar = FALSE;

    if ( HaveResultString(lparam) )
    {
        CheckInsertResultString( lparam );
        SetCompositionForm( );                   // Move Composition window.

        _fHoldNotify = FALSE;                       // OK notify client for change

    #if DUNNO /*{*/
        // In case our host is not turning off the ResultString bit
        // we need to ignore WM_IME_CHAR or else we will get the same
        // DBC again.
        if ( !ts.fInOurHost() )
            _fIgnoreIMEChar = TRUE;
    #else
        _fIgnoreIMEChar = TRUE;
    #endif /*}*/
    }

    // Always return S_FALSE so the DefWindowProc will handle the rest.
    // Host has to mask out the ResultString bit to avoid WM_IME_CHAR coming in.
    return S_FALSE;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme::CheckInsertResultString ( const LPARAM lparam )
//
//  @mfunc
//      handle inserting of GCS_RESULTSTR text, the final composed text.
//
//  @comm
//      When the final composition string arrives we grab it and set it into the text.
//
//  @devnote
//      A GCS_RESULTSTR message can arrive and the IME will *still* be in
//      composition string mode. This occurs because the IME's internal
//      buffers overflowed and it needs to convert the beginning of the buffer
//      to clear out some room. When this happens we need to insert the
//      converted text as normal, but remain in composition processing mode.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//
//-----------------------------------------------------------------------------


HRESULT
CIme::CheckInsertResultString (
    const LPARAM lparam )
{
    HRESULT hr = S_OK;
    CCaretTracker * pCaretTracker = NULL;

    if ( CleanupCompositionString(lparam) || HaveResultString(lparam) )      // If result string..
    {
        LONG cch;
        WCHAR achCompStr[256];

        if (!OK( GetCaretTracker( &pCaretTracker )))
            goto Cleanup;
        
        // Get result string.

        cch = (LONG)GetCompositionStringInfo( GCS_RESULTSTR,
                                              achCompStr, ARRAY_SIZE(achCompStr),
                                              NULL, 0, NULL, NULL );

        if (( achCompStr[0] == 32) && (cch == 1))
        {
            hr = THR( pCaretTracker->HandleSpace( achCompStr[0] ));            
        }
        else
        {
            // Make sure we don't exceed the maximum allowed for edit box.
#if TODO /*{*/
            LONG cchMax = psel->GetCommonContainer()->HasFlowLayout()->GetMaxLength()    // total
                          - ts.GetContentMarkup()->GetTextLength()                       // already occupied
                          + psel->GetCch();                                              // about to be deleted
#else
            LONG cchMax = MAXLONG;
#endif /*}*/
            cch = min( cch, cchMax );

        hr = THR( ReplaceRange(achCompStr, cch, TRUE, -1, TRUE) );
        if (FAILED(hr))
            goto Cleanup;

#if TODO /*{*/
            // BUGBUG (cthrash) necessary?
            if (cch)
            {
                psel->LaunderSpaces( psel->GetCpMin(), cch );
            }
#endif /*}*/
            // If we didn't accept anything, let the caller know to
            // terminate the composition.  We can't directly terminate here
            // as we may end up in a recursive call to our caller.

            hr = (cch == cchMax) ? S_FALSE : S_OK;
        }
    }

Cleanup:

    if (pCaretTracker)
        pCaretTracker->Release();

    RRETURN1(hr,S_FALSE);
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev2::IMENotify( const WPARAM wparam, const LPARAM lparam )
//
//  @mfunc
//      Handle Level 2 WM_IME_NOTIFY messages.
//
//  @comm
//      Currently we are only interested in knowing when to reset
//      the candidate window's position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::NotifyGlue(
    const WPARAM wparam,
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr = S_FALSE;
    
    if (IsIMEComposition())
    {
        hr = THR(_pIme->IMENotify(wparam,lparam));
    }

    RRETURN1(hr,S_FALSE);
}
                             
HRESULT
CIme_Lev2::IMENotify(
    const WPARAM wparam,
    const LPARAM lparam )
{
    TraceTag((tagEdIME, "notify"));

    if ( IMN_OPENCANDIDATE == wparam )
    {
        Assert ( 0 != lparam );

#ifndef MACPORT /*{*/

        HIMC hIMC = ImmGetContext();

        if ( hIMC )
        {
            INT index;
            CANDIDATEFORM cdCandForm;
            LONG l = lparam;

            // Convert the set bit to an index.  Who designed this API anyway?

            AssertSz( lparam, "One bit should have been set." );
            for (index = 0; !(l & 1) && index < 32; index++ )
            {
                l >>= 1;
            }
            AssertSz( (1 << index) == lparam, "Only one bit should be set." );


            if (    ImmGetCandidateWindow( hIMC, index, &cdCandForm )
                 && CFS_DEFAULT != cdCandForm.dwStyle )
            {
                cdCandForm.dwStyle = CFS_DEFAULT;             // Reset to CFS_DEFAULT
                ImmSetCandidateWindow(hIMC, &cdCandForm);
            }

            ImmReleaseContext(hIMC);
        }
#endif /*}*/
    }

    return S_FALSE;                                 // Allow DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//  void CIme_Lev2::PostIMEChar()
//
//  @mfunc
//      Called after processing a single WM_IME_CHAR in order to
//      update the position of the IME's composition window.
//
//
//-----------------------------------------------------------------------------

void
CIme_Lev2::PostIMEChar()
{
    TraceTag((tagEdIME, "postchar"));

    SetCompositionForm();                       // Move Composition window.
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev3::StartComposition()
//
//  @mfunc
//      Begin IME Level 3 composition string processing.
//
//  @comm
//      For rudimentary processing, remember the start and
//      length of the selection. Set the font in case the
//      candidate window actually uses this information.
//
//  @rdesc
//      This is a rudimentary solution for remembering were
//      the composition is in the text. There needs to be work
//      to replace this with a composition "range".
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev3::StartComposition()
{
    TraceTag((tagEdIME, "start (level3)"));

    _imeLevel = IME_LEVEL_3;

#ifndef MACPORT                                     // Korean IME check. /*{*/
    DWORD dwConversion, dwSentence;

    HIMC hIMC = ImmGetContext();

    if ( hIMC )                                     // Set _fKorean flag.
    {                                               //  for block cursor.
        if ( ImmGetConversionStatus( hIMC, &dwConversion, &dwSentence ) )
        {
            // NOTE:- the following is set for all FE system during IME input,
            // so we also need to check keyboard codepage as well.

            if ( dwConversion & IME_CMODE_HANGEUL )
            {
                _fKorean = (_KOREAN_CP == KeyboardCodePage());
            }
        }

        ImmReleaseContext( hIMC );             // Done with IME context.
    }

    SetCompositionFont( &_fUnderLineMode );

    // BUGBUG (cthrash) We need to kill the current selection
#endif /*}*/

    return S_OK;                                    // No DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//
//  HRESULT CIme_Lev3::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle Level 3 WM_IME_COMPOSITION messages.
//
//  @comm
//      Display all of the intermediary composition text as well as the final
//      reading.
//
//  @devnote
//      This is a rudimentary solution for replacing text in the backing store.
//      Work is left to do with the undo list, underlining, and hiliting with
//      colors and the selection.
//
//  @devnote
//      A GCS_RESULTSTR message can arrive and the IME will *still* be in
//      composition string mode. This occurs because the IME's internal
//      buffers overflowed and it needs to convert the beginning of the buffer
//      to clear out some room. When this happens we need to insert the
//      converted text as normal, but remain in composition processing mode.
//
//      Another reason, GCS_RESULTSTR can occur while in composition mode
//      for Korean because there is only 1 correct choice and no additional
//      user intervention is necessary, meaning that the converted string can
//      be sent as the result before composition mode is finished.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

inline HIGHLIGHT_TYPE HighlightTypeFromAttr( BYTE a )
{
    return HIGHLIGHT_TYPE( HIGHLIGHT_TYPE_ImeInput * (a + 1) );
}

HRESULT
CIme_Lev3::CompositionString(
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    TraceTag((tagEdIME, "compstr (%x)", lparam));
    HRESULT hr = S_OK;

#if DBG==1 /*{*/
    static BOOL fNoRecurse = FALSE;

    Assert ( !fNoRecurse );
    fNoRecurse = TRUE;
#endif /*}*/

    long                cchOld;
    WCHAR               achCompStr[256];
    BYTE                abAttrib[256];

    BOOL                fShowCursor;
    LONG                ichCursor, cchAttrib;

    CCharFormat         baseCF, CF;

    LONG                i, selStart, selEnd=0;        // For applying attrib
                                                    //  effects.

    BOOL                fTerminateWhenDone = FALSE;

    if (  CleanupCompositionString(lparam) || HaveResultString(lparam)  ) // Any final readings?
    {
        HRESULT hr;

        SetCaretVisible(FALSE);
        _TODO(psel->ShowSelection(FALSE);) // need a replacement

        hr = THR( CheckInsertResultString( lparam ) );
        if (hr)
        {
            if (hr == S_FALSE)
            {
                fTerminateWhenDone = TRUE;
            }
            else
            {
                goto Cleanup;
            }
        }

        _TODO(psel->ShowSelection(TRUE);)
        SetCaretVisible(TRUE);

        _fHoldNotify = FALSE;                       // OK notify client for change

    }

    if ( HaveCompositionString(lparam) )            // In composition mode?
    {
        LONG cchCompStr = 0;

        cchOld = 0;         // BUGBUG (cthrash) We need to compute this.
        cchAttrib = 0;

        // Get new intermediate composition string, attribs, and caret.

        cchCompStr = GetCompositionStringInfo( GCS_COMPSTR,
                                               achCompStr, ARRAY_SIZE(achCompStr),
                                               abAttrib, ARRAY_SIZE(abAttrib),
                                               &ichCursor, &cchAttrib );

        // When there is no old text or new text, just show the caret
        // This is the case when client used TerminateIMEComposition with
        // CPS_CANCEL option.

        if ( /*!cchOld &&*/ !cchCompStr )
        {
            SetCaretVisible(TRUE);
            ReplaceRange(NULL, 0, TRUE, 0);
        }
        else
        {
#ifndef MACPORT /*{*/
#if NOTYET /*{*/
            // this is to support Korean overstrike mode
            if ( _fKorean && !cchOld && ts.GetContentMarkup()->_fOverstrike )
                psel->PrepareIMEOverstrike ( &undobldr );   // get ride of the next char if necessary
#endif /*}*/
#endif /*}*/
#if TODO /*{*/
            DWORD cpMin = _prgUncommitted->GetCpMin();
            DWORD cpMax = psel->GetCommonContainer()->HasFlowLayout()->GetMaxLength();
            DWORD cpMin = _prgUncommitted->GetCpMin();
            DWORD cpMax = psel->GetCommonContainer()->HasFlowLayout()->GetMaxLength();

            // In Korean, a Jamo can get pushed out of one Hanguel char to
            // form another.  Under that situation we may end up exceeding
            // our buffer.  Don't allow that to happen.  Note in other locales,
            // we allow ourselves to exceed our buffer because the uncommitted
            // text might get sure upon being committed.

            if ( _fKorean && !cchOld && cpMin >= cpMax )
            {
                cchCompStr = 0;
            }
            else
            {
                _prgUncommitted->CopySnapshotFormatInfo( psel );
                _prgUncommitted->Set( cpMin + cchOld, cchOld );
                IGNORE_HR( _prgUncommitted->ReplaceRange( cchCompStr, achCompStr ) );
            }
            _prgUncommitted->Set( _prgUncommitted->GetCpMin() , cchCompStr );
#else
            // if we're not at the end, *and* we're not pointing at some converted text, hide the caret

            BOOL fShowCaret;

            fShowCaret =    !_fKorean
                         && (   ichCursor == cchAttrib
                             || abAttrib[ichCursor] == ATTR_INPUT);

            if (_fKorean)
            {
                ichCursor = 1;
            }
            
            // BUGBUG (cthrash) IGNORE_HR is not the way to go here.

            IGNORE_HR( ReplaceRange( achCompStr, cchCompStr, fShowCaret, ichCursor ) );
#endif /*}*/

            if ( ichCursor > 0 )
            {
                ichCursor = min(cchCompStr, ichCursor);
            }

            selStart = -1;
            _TODO(_prgInverted->Set(0,0);)

            if ( cchCompStr && cchCompStr <= cchAttrib && !_fKorean )     // no Korean style needed
            {
                // NB (cthrash)
                //
                // Each character in IME string has an attribute.  They can
                // be one of the following:
                //
                // 0 - ATTR_INPUT                   dotted underline
                // 1 - ATTR_TARGET_CONVERTED        inverted text, no underline
                // 2 - ATTR_CONVERTED               dotted underline
                // 3 - ATTR_TARGET_NOTCONVERTED     solid underline
                // 4 - ATTR_ERROR                   ???
                //
                // The right column is how the text is rendered in the standard
                // Japanese Windows Edit Widget.
                //

                BYTE abCur = abAttrib[0];
                int  ichMin = 0;
                int  ichMost;
                int  iHighlightSegment = 0;

                //
                // Add the segments
                //

                hr = THR( ClearHighlightSegments() );
                if (hr)
                    goto Cleanup;

                abCur = abAttrib[0];

                for (ichMost = 1; ichMost < cchCompStr; ichMost++)
                {
                    if (abAttrib[ichMost] == abCur)
                        continue;

                    hr = THR( AddHighlightSegment( iHighlightSegment++, FALSE, ichMin, ichMost, HighlightTypeFromAttr(abCur) ) );
                    if (hr)
                        goto Cleanup;

                    ichMin = ichMost;

                    abCur = abAttrib[ichMost];
                }

                if (ichMin != ichMost)
                {
                    hr = THR( AddHighlightSegment( iHighlightSegment, TRUE, ichMin, ichMost, HighlightTypeFromAttr(abCur) ) );
                    if (hr)
                        goto Cleanup;
                }

                if (IsTagEnabled(tagEdIMEAttr))
                {
                    static BYTE hex[17] = "0123456789ABCDEF";

                    for (i=0;i<cchCompStr;i++)
                    {
                        abAttrib[i] = hex[abAttrib[i] & 15];
                    }

                    abAttrib[min(255L,i)] = '\0';
                    TraceTag((tagEdIMEAttr, "attrib %s", abAttrib));
                }
            }

            fShowCursor = FALSE;
            //if ( ichCursor >= 0 )                        // If a cursor pos exist...
            {
                _TODO(ichCursor += _prgUncommitted->GetCpMin();)// Set cursor and scroll.

    #ifndef MACPORT /*{*/
                if ( _fKorean && cchCompStr )
                {
                    // Set the cursor (although invisible) to *after* the
                    // character, so as to force a scroll. (cthrash)

                    hr = THR( AddHighlightSegment( 0, FALSE, 0, cchCompStr, HIGHLIGHT_TYPE_ImeHangul ) );
                    if (hr)
                        goto Cleanup;
                }
                else
    #endif /*}*/
                {
                    fShowCursor = ( selStart < 0 || selEnd == selStart );
                }

#if TODO /*{*/
                if (psel)
                {
                    psel->SetSelection( ichCursor, ichCursor, TRUE, * pIT );
                    psel->AdjustForInsert();
                }
#endif /*}*/
            }

            //pIT->SetCaretVisible(fShowCursor);

            // make sure we have set the call manager text changed flag.  This
            // flag may be cleared when calling SetCharFormat
            //ts.GetPed()->GetCallMgr()->SetChangeEvent(CN_TEXTCHANGED);

            // setup composition window for Chinese in-caret IME
            if ( !_fKorean )
            {
                IMENotify( IMN_OPENCANDIDATE, 0x01 );
            }
        }

        // don't notify client for changes only when there is composition string available
        if ( cchCompStr && !_pManager->IsImeAlwaysNotify() )
        {
            _fHoldNotify = TRUE;
        }

    }

#if DBG==1 /*{*/
    fNoRecurse = FALSE;
#endif /*}*/

    if (fTerminateWhenDone)
    {
        TerminateIMEComposition( TERMINATE_FORCECANCEL, peTrackerNotify );
    }

Cleanup:

    RRETURN(hr);                                    // No DefWindowProc
}                                                   //  processing.

//+----------------------------------------------------------------------------
//
//  BOOL CIme_Lev3::SetCompositionStyle ( CCharFormat &CF, UINT attribute )
//
//  @mfunc
//      Set up a composition clause's character formmatting.
//
//  @comm
//      If we loaded Office's IMEShare.dll, then we ask it what the formatting
//      should be, otherwise we use our own, hardwired default formatting.
//
//  @devnote
//      Note the use of pointers to functions when dealing with IMEShare funcs.
//      This is because we dynamically load the IMEShare.dll.
//
//  @rdesc
//      BOOL - This is because CFU_INVERT is treated like a selection by
//          the renderer, and we need to know the the min invertMin and
//          the max invertMost to know if the rendered line should be treated
//          as if there are selections to be drawn.
//
//-----------------------------------------------------------------------------

BOOL
CIme_Lev3::SetCompositionStyle (
    CCharFormat &CF,
    UINT attribute )
{
    BOOL            fInvertStyleUsed = FALSE;
#if 0 /*{*/

    CF._fUnderline = FALSE;
    CF._bUnderlineType = 0;

#ifndef MACPORT /*{*/
#if IMESHARE /*{*/
    const IMESTYLE  *pIMEStyle;
    UINT            ulID;

    COLORREF        color;
#endif /*}*/

#if IMESHARE /*{*/
    // load ImeShare if it has not been done
    if ( !fLoadIMEShareProcs )
    {
        InitNLSProcTable( LOAD_IMESHARE );
        fLoadIMEShareProcs = TRUE;
    }

    if ( fHaveIMEShareProcs )
    {
        pIMEStyle = pPIMEStyleFromAttr( attribute );
        if ( NULL == pIMEStyle )
            goto defaultStyle;

        CF._fBold = FALSE;
        CF._fItalic = FALSE;

        if ( pFBoldIMEStyle ( pIMEStyle ) )
            CF._fBold = TRUE;

        if ( pFItalicIMEStyle ( pIMEStyle ) )
            CF._fItalic = TRUE;

        if ( pFUlIMEStyle ( pIMEStyle ) )
        {
            CF._fUnderline = TRUE;
            CF._bUnderlineType = CFU_UNDERLINE;

            ulID = pIdUlIMEStyle ( pIMEStyle );
            if ( UINTIMEBOGUS != ulID )
            {
                if ( IMESTY_UL_DOTTED == ulID )
                    CF._bUnderlineType = CFU_UNDERLINEDOTTED;
            }
        }

        color = pRGBFromIMEColorStyle( pPColorStyleTextFromIMEStyle ( pIMEStyle ));
        if ( UINTIMEBOGUS != color )
        {
            CF._ccvTextColor.SetValue( color, FALSE );
        }

        color = pRGBFromIMEColorStyle( pPColorStyleBackFromIMEStyle ( pIMEStyle ));
        if ( UINTIMEBOGUS != color )
        {
            //CF.dwEffects &= ~CFE_AUTOBACKCOLOR;
            CF._ccvrBackColor.SetValue( color, FALSE );

            fInvertStyleUsed = TRUE;
        }
    }
    else // default styles when no IMEShare.dll exist.
#endif //IMESHARE/*}*/
#endif //MACPORT/*}*/
    {
#ifndef MACPORT /*{*/
#if IMESHARE /*{*/
defaultStyle:
#endif /*}*/
#endif /*}*/
        switch ( attribute )
        {                                       // Apply underline style.
            case ATTR_INPUT:
            case ATTR_CONVERTED:
                CF._fUnderline = TRUE;
                CF._bUnderlineType = CFU_UNDERLINEDOTTED;
                break;
            case ATTR_TARGET_NOTCONVERTED:
                CF._fUnderline = TRUE;
                CF._bUnderlineType = CFU_UNDERLINE;
                break;
            case ATTR_TARGET_CONVERTED:         // Target *is* selection.
            {
                CF._ccvTextColor.SetSysColor(COLOR_HIGHLIGHTTEXT);

                fInvertStyleUsed = TRUE;
            }
            break;
        }
    }
#endif /*}*/
    return fInvertStyleUsed;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_Lev3::IMENotify( const WPARAM wparam, const LPARAM lparam )
//
//  @mfunc
//      Handle Level 3 WM_IME_NOTIFY messages.
//
//  @comm
//      Currently we are only interested in knowing when to update
//      the n window's position.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Lev3::IMENotify(
    const WPARAM wparam,
    const LPARAM lparam )
{
    TraceTag((tagEdIME, "notify"));

#ifndef MACPORT /*{*/
    if ( IMN_OPENCANDIDATE == wparam || IMN_CLOSECANDIDATE == wparam  )
    {
        Assert ( 0 != lparam );

        HIMC hIMC = ImmGetContext();                // Get host's IME context.

        if (hIMC)
        {
            CANDIDATEFORM   cdCandForm;
            INT             index;

            // Convert bitID to INDEX because *stupid* API.

            for (index = 0; index < 32; index++ )
            {
                if ( 0 != ((1 << index) & lparam) )
                    break;
            }
            Assert ( ((1 << index) & lparam) == lparam );    // Only 1 set?
            Assert ( index < 32 );

            if ( IMN_OPENCANDIDATE == wparam && !_fKorean )  // Set candidate to caret.
            {
                POINT ptCaret;
                RECT rc;
                long lLineHeight;
                HRESULT hr = THR(GetCompositionPos(&ptCaret, &rc, &lLineHeight));

                if (OK(hr))
                {
                    ptCaret.x = max(0L, ptCaret.x);
                    ptCaret.y = max(0L, ptCaret.y);

                    cdCandForm.dwStyle = CFS_CANDIDATEPOS;

                    if (KeyboardCodePage() == _JAPAN_CP)
                    {
                        // Change style to CFS_EXCLUDE, this is to
                        // prevent the candidate window from covering
                        // the current selection.

                        cdCandForm.dwStyle = CFS_EXCLUDE;
                        cdCandForm.rcArea.left = ptCaret.x;                 

                        // FUTURE: for verticle text, need to adjust
                        // the rcArea to include the character width.

                        cdCandForm.rcArea.right =
                            cdCandForm.rcArea.left + 2;
                        cdCandForm.rcArea.top = ptCaret.y - lLineHeight;
                        ptCaret.y += 4;
                        cdCandForm.rcArea.bottom = ptCaret.y;
                    }
                    else
                    {
                        ptCaret.y += 4;
                    }

                    // Most IMEs will have only 1, #0, candidate window. However, some IMEs
                    //  may want to have a window organized alphabetically, by stroke, and
                    //  by radical.

                    cdCandForm.dwIndex = index;                         
                    cdCandForm.ptCurrentPos = ptCaret;
                    ImmSetCandidateWindow(hIMC, &cdCandForm);
                }
            }
            else                                    // Reset back to CFS_DEFAULT.
            {
                if (   ImmGetCandidateWindow(hIMC, index, &cdCandForm)
                    && CFS_DEFAULT != cdCandForm.dwStyle )
                {
                    cdCandForm.dwStyle = CFS_DEFAULT;
                    ImmSetCandidateWindow(hIMC, &cdCandForm);
                }
            }

            ImmReleaseContext( hIMC );         // Done with IME context.
        }
#endif /*}*/
    }

    return S_FALSE;                                 // Allow DefWindowProc
}                                                   //  processing.


//+----------------------------------------------------------------------------
//
//  HRESULT StartHangeulToHanja()
//
//  @func
//      Initiates an IME composition string edit to convert Korean Hanguel to Hanja.
//  @comm
//      Called from the message loop to handle VK_KANJI_KEY.
//
//  @devnote
//      We decide if we need to do a conversion by checking:
//      - the Fonot is a Korean font,
//      - the character is a valid SBC or DBC,
//      - ImmEscape accepts the character and bring up a candidate window
//
//  @rdesc
//      BOOL - FALSE for no conversion. TRUE if OK.
//
//-----------------------------------------------------------------------------

HRESULT
CSelectionManager::StartHangeulToHanja(
    TRACKER_NOTIFY * peTrackerNotify,
    IMarkupPointer * pPointer /*= NULL */)
{
    int         iResult = 0;
    char        abHangeul[3] = {0, 0, 0};
    WCHAR       achHangeul[2] = { 0, 0 };
    HRESULT     hr;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    long        cch = 1;
    const BOOL  fIsOnNT = CSelectionManager::IsOnNT();
    SP_IMarkupPointer spCaretPosition;
    
    //
    // get the current character
    //

    IFC( GetMarkupServices()->CreateMarkupPointer( &spCaretPosition ));

    if (pPointer)
    {
        IFC( spCaretPosition->MoveToPointer( pPointer ) );
    }
    else
    {
        SP_IHTMLCaret  spCaret;

        IFC( GetViewServices()->GetCaret( &spCaret ));
        IFC( spCaret->MovePointerToCaret( spCaretPosition ));
    }

    IFC( spCaretPosition->Right( FALSE, & eContext, NULL, &cch, achHangeul ));
    
    if (   cch == 1
        && (   fIsOnNT
            || WideCharToMultiByte(_KOREAN_CP, 0, achHangeul, 1, abHangeul, 2, NULL, NULL) > 0
           )
       )
    {
        HIMC hIMC = ImmGetContext();

        if (hIMC)
        {
            HKL hKL = GetKeyboardLayout(0);

            iResult = fIsOnNT
                      ? ImmEscapeW(hKL, hIMC, IME_ESC_HANJA_MODE, achHangeul)
                      : ImmEscapeA(hKL, hIMC, IME_ESC_HANJA_MODE, abHangeul);

            ImmReleaseContext(hIMC);

            if (iResult)
            {
                if (   peTrackerNotify
                    && (   !_pActiveTracker
                        || _pActiveTracker->GetSelectionType() != SELECTION_TYPE_Caret))
                {
                    if (pPointer)
                    {
                        IFC( CopyTempMarkupPointers( pPointer, pPointer ));
                        *peTrackerNotify = TN_END_TRACKER_POS_CARET_REBUBBLE;
                    }
                    else
                    {
                        // BUGBUG (cthrash) This is wrong, but we shouldn't get here.

                        AssertSz(0, "We're not supposed to be here.");

                        *peTrackerNotify = TN_END_TRACKER_CREATE_CARET;
                    }
                }
                else
                {
                    _pIme = new CIme_HangeulToHanja( this, 0 );

                    if ( _pIme && IsIMEComposition() )
                    {
                        // We need to 'select' the character.

                        IFC( _pIme->AdjustUncommittedRangeAroundInsertionPoint() );
                        IFC( _pIme->_pmpEndUncommitted->MoveUnit(MOVEUNIT_NEXTCHAR) );
                        IFC( _pIme->AddHighlightSegment( 0, FALSE, 0, 1, HIGHLIGHT_TYPE_ImeHangul ) );

                        _pIme->StartComposition();
                    }
                }
            }
        }
    }

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  CIme_HangeulToHanja::CIme_HangeulToHanja()
//
//  @mfunc
//      CIme_HangeulToHanja Constructor.
//
//  @comm
//      Needed to save Hangeul character width for Block caret
//
//-----------------------------------------------------------------------------

CIme_HangeulToHanja::CIme_HangeulToHanja( CSelectionManager * pManager, LONG xWidth )
    : CIme_Lev3( pManager )
{
    _xWidth = xWidth;
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_HangeulToHanja::StartComposition()
//
//  @mfunc
//      Begin CIme_HangeulToHanja composition string processing.
//
//  @comm
//      Call Level3::StartComposition.  Then setup the Korean block
//      caret for the Hanguel character.
//
//  @rdesc
//      Need to adjust _ichStart and _cchCompStr to make the Hanguel character
//      "become" a composition character.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_HangeulToHanja::StartComposition( )
{
    HRESULT hr = S_OK;

    hr = CIme_Lev3::StartComposition();

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//  HRESULT CIme_HangeulToHanja::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle CIme_HangeulToHanja WM_IME_COMPOSITION messages.
//
//  @comm
//      call CIme_Lev3::CompositionString to get rid of the selected Hanguel character,
//      then setup the format for the next Composition message.
//
//  @devnote
//      When the next Composition message comes in and that we are no longer in IME,
//      the new character will use the format as set here.
//
//
//
//-----------------------------------------------------------------------------

HRESULT
CIme_HangeulToHanja::CompositionString(
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    CIme_Lev3::CompositionString( lparam, peTrackerNotify );

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  HRESULT CIme_Protected::CompositionString( const LPARAM lparam )
//
//  @mfunc
//      Handle CIme_Protected WM_IME_COMPOSITION messages.
//
//  @comm
//      Just throw away the restlt string since we are
//  in read-only or protected mode
//
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//      Side effect: _fDestroy is set to notify that composition string
//          processing is finished and this object should be deleted.
//
//-----------------------------------------------------------------------------

HRESULT
CIme_Protected::CompositionString (
    const LPARAM lparam,
    TRACKER_NOTIFY * peTrackerNotify )
{
    HRESULT hr;

    if ( CleanupCompositionString(lparam) || HaveResultString(lparam) )
    {
        INT   cch;
        WCHAR achCompStr[256];

        cch = GetCompositionStringInfo( GCS_RESULTSTR,
                                        achCompStr, ARRAY_SIZE(achCompStr),
                                        NULL, 0, NULL, NULL);

        // we should have one or 0 characters to throw away

        Assert ( cch <= 1 );

        hr = S_OK;                                  // Don't want WM_IME_CHARs.
    }
    else
    {
        // terminate composition to force a end composition message

        TerminateIMEComposition( TERMINATE_NORMAL, peTrackerNotify );

        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------------------
//
//  HRESULT IgnoreIMEInput ( HWND hwnd, DWORD lParam  )
//
//  @func
//      Ignore IME character input
//  @comm
//      Called to handle WM_KEYDOWN with VK_PROCESSKEY during
//      protected or read-only mode.
//
//  @devnote
//      This is to ignore the IME character.  By translating
//      message with result from ImmGetVirtualKey, we
//      will not receive START_COMPOSITION message.  However,
//      if the host has already called TranslateMessage, then,
//      we will let START_COMPOSITION message to come in and
//      let IME_PROTECTED class to do the work.
//
//  @rdesc
//      HRESULT-S_FALSE for DefWindowProc processing, S_OK if not.
//
//-----------------------------------------------------------------------------

HRESULT IgnoreIMEInput(
    HWND    hwnd,               // @parm parent window handle
    DWORD   dwFlags)            // @parm lparam of WM_KEYDOWN msg
{
    HRESULT     hr = S_FALSE;

#ifndef MACPORT /*{*/
    MSG         msg;

    Assert ( hwnd );
    if (VK_PROCESSKEY != (msg.wParam  = ImmGetVirtualKey( hwnd )))
    {
        // if ImmGetVirtualKey is still returning VK_PROCESSKEY
        // That means the host has already called TranslateMessage.
        // In such case, we will let START_COMPOSITION message
        // to come in and let IME_PROTECTED class to do the work
        msg.hwnd = hwnd;
        msg.message = WM_KEYDOWN;
        msg.lParam  = dwFlags;
        if (::TranslateMessage ( &msg ))
            hr = S_OK;
    }
#endif /*}*/
    return hr;
}

HRESULT
CIme::GetCaretTracker( CCaretTracker **ppCaretTracker )
{
    HRESULT hr;

    if (   _pManager->_pActiveTracker
        && _pManager->_pActiveTracker->GetSelectionType() == SELECTION_TYPE_Caret)
    {
        *ppCaretTracker = DYNCAST(CCaretTracker, _pManager->_pActiveTracker);
        (*ppCaretTracker)->AddRef();
        
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    RRETURN1(hr,S_FALSE);
}

HRESULT
CSelectionManager::TerminateIMEComposition(
    DWORD dwMode,
    TRACKER_NOTIFY * peTrackerNotify,
    DWORD * pdwFollowUpActionFlag )
{
    HRESULT hr = S_OK;

    if (IsIMEComposition())
    {
        TRACKER_NOTIFY eNotify;

        _pIme->TerminateIMEComposition( dwMode, &eNotify );

        if (!peTrackerNotify)
        {
            hr = (eNotify != TN_NONE)
                 ? S_OK
                 : TrackerNotify( eNotify, NULL, pdwFollowUpActionFlag );
        }
        else
        {
            *peTrackerNotify = eNotify;
            hr = S_FALSE;
        }
    }

    RRETURN1(hr,S_FALSE);
}

DWORD CSelectionManager::s_dwPlatformId = DWORD(-1);

void
CSelectionManager::CheckVersion()
{
#ifndef WINCE
    OSVERSIONINFOA ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    Verify(GetVersionExA(&ovi));
#else //WINCE
    OSVERSIONINFO ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    Verify(GetVersionEx(&ovi));
#endif //WINCE

    s_dwPlatformId = ovi.dwPlatformId;
}
#endif /*}*/
