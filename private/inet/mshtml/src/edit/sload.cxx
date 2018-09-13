//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       SLOAD.CXX
//
//  Contents:   Implementation of CSpringLoader class.
//
//  Classes:    CSpringLoader
//
//  History:    07-13-98 - OliverSe - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

using namespace EdUtil;

MtDefine(CSpringLoader, Utilities, "CSpringLoader")

DeclareTag(tagDisableSpringLoader, "Springloader", "Disable Spring Loader")


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::Get(*)Command
//
//  Synopsis:   Inline accessors for commands.
//
//-----------------------------------------------------------------------------

inline IMarkupServices *
CSpringLoader::GetMarkupServices()
{
    return _pCommandTarget->GetEditor()->GetMarkupServices();
}

inline IHTMLViewServices *
CSpringLoader::GetViewServices()
{
    return _pCommandTarget->GetEditor()->GetViewServices();
}

inline CCommand *
CSpringLoader::GetCommand(DWORD cmdID)
{
    return _pCommandTarget->GetEditor()->GetCommandTable()->Get(cmdID);
}

inline CBaseCharCommand *
CSpringLoader::GetBaseCharCommand(DWORD cmdID)
{
    Assert(cmdID == IDM_BOLD || cmdID == IDM_ITALIC || cmdID == IDM_UNDERLINE || cmdID == IDM_SUBSCRIPT || cmdID == IDM_SUPERSCRIPT || 
           cmdID == IDM_FONTSIZE || cmdID == IDM_FONTNAME || cmdID == IDM_FORECOLOR || cmdID == IDM_BACKCOLOR);
    return ((CBaseCharCommand *) GetCommand(cmdID));
}

inline CCharCommand *
CSpringLoader::GetCharCommand(DWORD cmdID)
{
    Assert(cmdID == IDM_BOLD || cmdID == IDM_ITALIC || cmdID == IDM_UNDERLINE || cmdID == IDM_SUBSCRIPT || cmdID == IDM_SUPERSCRIPT);
    return ((CCharCommand *) GetCommand(cmdID));
}

inline CFontCommand *
CSpringLoader::GetFontCommand(DWORD cmdID)
{
    Assert(cmdID == IDM_FONTSIZE || cmdID == IDM_FONTNAME || cmdID == IDM_FORECOLOR || cmdID == IDM_BACKCOLOR);
    return ((CFontCommand *) GetCommand(cmdID));
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::CSpringLoader, ~CSpringLoader
//
//  Synopsis:   Constructor and destructor.
//
//-----------------------------------------------------------------------------

CSpringLoader::CSpringLoader(CMshtmlEd * pCommandTarget)
{
    _grfFlags() = 0;
    _pCommandTarget = pCommandTarget;
    _pmpPosition = NULL;

    Assert(_pCommandTarget);
}

CSpringLoader::~CSpringLoader()
{
    // Free springload cache.
    Reset();

    ReleaseInterface(_pmpPosition);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::SpringLoad
//
//  Synopsis:   Loads the spring loader by copying the formats found found
//              at the position passed in.
//
//-----------------------------------------------------------------------------

HRESULT
CSpringLoader::SpringLoad(IMarkupPointer * pmpPosition, DWORD dwMode /* =0 */)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    CSelectionManager * pSelMan;
    SP_IMarkupPointer   spmpSpringLoadFormatting;
    HTMLCharFormatData  chFmtData;
    CVariant            varName, varSize, varColor, varBackColor;
    HRESULT             hr = S_OK;

    Assert(pMarkupServices && pViewServices && pmpPosition);

    // Reset springloader if requested.
    if (dwMode & SL_RESET)
        Reset();

    if (_fSpringLoaded)
        goto Cleanup;

    pSelMan = _pCommandTarget->GetEditor()->GetSelectionManager();
    Assert(pSelMan);
    if (!pSelMan->CanContextAcceptHTML())
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(CopyMarkupPointer(pMarkupServices, pmpPosition, &spmpSpringLoadFormatting));
    if (hr)
        goto Cleanup;

    // Handle AdjustForInsert modes.

    //
    // BUGBUG (johnbed) passing TRUE here is incorrect. fNotAtBOL needs to be maintained
    // from wherever pmpPosition is set
    //
    if (dwMode & SL_ADJUST_FOR_INSERT_LEFT)
        IGNORE_HR(AdjustPointerForInsert(spmpSpringLoadFormatting, TRUE, LEFT, LEFT));
    else if (dwMode & SL_ADJUST_FOR_INSERT_RIGHT)
        IGNORE_HR(AdjustPointerForInsert(spmpSpringLoadFormatting, TRUE, RIGHT, RIGHT));
    
    // Handle compose settings mode.
    if (dwMode & SL_TRY_COMPOSE_SETTINGS)
    {
        // Attempt to springload compose settings if there is no formatting.
        hr = THR(SpringLoadComposeSettings(spmpSpringLoadFormatting));
        if (hr != S_FALSE)
            goto Cleanup;
    }
    //
    // Obtain current formats.
    //

    hr = THR(pViewServices->GetCharFormatInfo(
            spmpSpringLoadFormatting,
            CHAR_FORMAT_FontStyle|CHAR_FORMAT_FontInfo|CHAR_FORMAT_FontName|CHAR_FORMAT_ColorInfo,
            &chFmtData));
    if (FAILED(hr))
        goto Cleanup;

    //
    // Convert font formats to variants.
    //

    hr = THR(GetFontCommand(IDM_FONTNAME)->ConvertFormatDataToVariant(chFmtData, &varName));
    if (hr)
        goto Cleanup;

    hr = THR(GetFontCommand(IDM_FONTSIZE)->ConvertFormatDataToVariant(chFmtData, &varSize));
    if (hr)
        goto Cleanup;

    hr = THR(GetFontCommand(IDM_FORECOLOR)->ConvertFormatDataToVariant(chFmtData, &varColor));
    if (hr)
        goto Cleanup;

    hr = THR(GetFontCommand(IDM_BACKCOLOR)->ConvertFormatDataToVariant(chFmtData, &varBackColor));
    if (hr)
        goto Cleanup;

    //
    // Copy the formats into the spring loader.
    //

    SetFormats(chFmtData.fBold,
               chFmtData.fItalic,
               chFmtData.fUnderline,
               chFmtData.fSuperScript,
               chFmtData.fSubScript,
               &varName,
               &varSize,
               &varColor,
               &varBackColor);

    // We are now springloaded.
    MarkSpringLoaded(spmpSpringLoadFormatting);

Cleanup:

    VariantClear(&varName);
    VariantClear(&varSize);
    VariantClear(&varColor);
    VariantClear(&varBackColor);

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpringLoader::Fire
//
//  Synopsis:   Applies the springloaded formats information
//              as long as it is valid and wherever it differs
//              from the existing format at the caret.
//
//----------------------------------------------------------------------------

HRESULT
CSpringLoader::Fire(IMarkupPointer * pmpStart, IMarkupPointer * pmpEnd /*=NULL*/, BOOL fMoveCaretToStart /*=TRUE*/)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupPointer    * pmpCaret = NULL;
    IHTMLCaret        * pCaret = NULL;
    IHTMLElement      * pElemBlock = NULL;
    CFontCommand      * pFontCommand;
    HTMLCharFormatData  chFmtData;
    VARIANT             var;
    BOOL                fFontPropertyChange, fHasEnd = pmpEnd != NULL;
    HRESULT             hr = S_OK;
    POINTER_GRAVITY     eGravity = POINTER_GRAVITY_Max;

    if (!_fSpringLoaded)
        goto Cleanup;

    Assert(pMarkupServices && pViewServices && pmpStart);

    //
    // Obtain current formats.
    //

    hr = THR(pViewServices->GetCharFormatInfo(
            pmpStart,
            CHAR_FORMAT_FontStyle|CHAR_FORMAT_FontInfo|CHAR_FORMAT_FontName|CHAR_FORMAT_ColorInfo,
            &chFmtData));
    if (FAILED(hr))
        goto Cleanup;

    //
    // Create ending markup pointer if one wasn't passed in, and position it next to pmpStart.
    //

    if (!fHasEnd)
    {
        hr = THR(pMarkupServices->CreateMarkupPointer(&pmpEnd));
        if (hr)
            goto Cleanup;

        hr = THR(pmpEnd->MoveToPointer(pmpStart));
        if (hr)
            goto Cleanup;
    }

    //
    // Before we add any elements, make sure our segment pointers pmpStart
    // and pmpEnd have opposite gravity so that they move outward as we
    // insert or split elements. 
    //

    hr = THR(EnsureCaretPointers(fMoveCaretToStart?pmpStart:pmpEnd, &pCaret, &pmpCaret));

    if (FAILED(hr))
        goto Cleanup;

    // Remember the gravity so we can restore it at Cleanup
    hr = THR(pmpStart->Gravity(& eGravity));
    if (hr)
        goto Cleanup;

    hr = THR(pmpStart->SetGravity(POINTER_GRAVITY_Left));
    if (hr)
        goto Cleanup;

    hr = THR(pmpEnd->SetGravity(POINTER_GRAVITY_Right));
    if (hr)
        goto Cleanup;

    //
    // If there is any difference between the springloaded formats and the
    // current formats, apply the necessary commands.
    //

    //
    // Span class.
    //

    if (V_VT(&_varSpanClass) != VT_NULL && V_VT(&_varSpanClass) != VT_EMPTY)
    {
        Assert(V_VT(&_varSpanClass) == VT_BSTR);

        BOOL fInSpanScope = InSpanScope(pmpStart);

        if (!fInSpanScope)
        {
            CInsertCommand * pInsertSpanCommand = DYNCAST(CInsertCommand, GetCommand(IDM_INSERTSPAN));

            pInsertSpanCommand->SetAttributeValue(V_BSTR(&_varSpanClass));

            hr = THR(pInsertSpanCommand->ApplyCommandToSegment(pmpStart, pmpEnd, NULL, FALSE));
            if (hr)
                goto Cleanup;

            hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_SPAN, TRUE));
            if (hr)
                goto Cleanup;
        }
    }

    if (_fBold ^ chFmtData.fBold)
    {
        if (_fBold)
            hr = THR(GetCharCommand(IDM_BOLD)->Apply(_pCommandTarget, pmpStart, pmpEnd, NULL, TRUE));
        else
            hr = THR(GetCharCommand(IDM_BOLD)->Remove(_pCommandTarget, pmpStart, pmpEnd));

        if (hr)
            goto Cleanup;

        hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_STRONG, _fBold));
        if (hr)
            goto Cleanup;
    }

    if (_fItalic ^ chFmtData.fItalic)
    {
        if (_fItalic)
            hr = THR(GetCharCommand(IDM_ITALIC)->Apply(_pCommandTarget, pmpStart, pmpEnd, NULL, TRUE));
        else
            hr = THR(GetCharCommand(IDM_ITALIC)->Remove(_pCommandTarget, pmpStart, pmpEnd));

        if (hr)
            goto Cleanup;

        hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_EM, _fItalic));
        if (hr)
            goto Cleanup;
    }

    if (_fUnderline ^ chFmtData.fUnderline)
    {
        if (_fUnderline)
            hr = THR(GetCharCommand(IDM_UNDERLINE)->Apply(_pCommandTarget, pmpStart, pmpEnd, NULL, TRUE));
        else
            hr = THR(GetCharCommand(IDM_UNDERLINE)->Remove(_pCommandTarget, pmpStart, pmpEnd));

        if (hr)
            goto Cleanup;

        hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_U, _fUnderline));
        if (hr)
            goto Cleanup;
    }

    if (_fSubscript ^ chFmtData.fSubScript)
    {
        if (_fSubscript)
            hr = THR(GetCharCommand(IDM_SUBSCRIPT)->Apply(_pCommandTarget, pmpStart, pmpEnd, NULL, TRUE));
        else
            hr = THR(GetCharCommand(IDM_SUBSCRIPT)->Remove(_pCommandTarget, pmpStart, pmpEnd));

        if (hr)
            goto Cleanup;

        hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_SUB, _fSubscript));
        if (hr)
            goto Cleanup;
    }

    if (_fSuperscript ^ chFmtData.fSuperScript)
    {
        if (_fSuperscript)
            hr = THR(GetCharCommand(IDM_SUPERSCRIPT)->Apply(_pCommandTarget, pmpStart, pmpEnd, NULL, TRUE));
        else
            hr = THR(GetCharCommand(IDM_SUPERSCRIPT)->Remove(_pCommandTarget, pmpStart, pmpEnd));

        if (hr)
            goto Cleanup;

        hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_SUP, _fSuperscript));
        if (hr)
            goto Cleanup;
    }

    //
    // Font name.
    //

    Assert(V_VT(&_varName) == VT_BSTR || V_VT(&_varName) == VT_NULL);

    if (V_VT(&_varName) != VT_NULL)
    {
        pFontCommand = GetFontCommand(IDM_FONTNAME);
        hr = THR(pFontCommand->ConvertFormatDataToVariant(chFmtData, &var));
        if (hr)
            goto Cleanup;

        fFontPropertyChange = !pFontCommand->IsVariantEqual(&var, &_varName);
        VariantClear(&var);

        if (fFontPropertyChange)
        {
            hr = THR(pFontCommand->Apply(_pCommandTarget, pmpStart, pmpEnd, &_varName, TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_FONT, TRUE));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Font size.
    //

    if (V_VT(&_varSize) != VT_NULL)
    {
        pFontCommand = GetFontCommand(IDM_FONTSIZE);
        hr = THR(pFontCommand->ConvertFormatDataToVariant(chFmtData, &var));
        if (hr)
            goto Cleanup;

        fFontPropertyChange = !pFontCommand->IsVariantEqual(&var, &_varSize);
        VariantClear(&var);

        if (fFontPropertyChange)
        {
            hr = THR(pFontCommand->Apply(_pCommandTarget, pmpStart, pmpEnd, &_varSize, TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_FONT, TRUE));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Fore color.
    //

    if (V_VT(&_varColor) != VT_NULL)
    {
        pFontCommand = GetFontCommand(IDM_FORECOLOR);
        hr = THR(pFontCommand->ConvertFormatDataToVariant(chFmtData, &var));
        if (hr)
            goto Cleanup;

        fFontPropertyChange = !pFontCommand->IsVariantEqual(&var, &_varColor);
        VariantClear(&var);

        if (fFontPropertyChange)
        {
            hr = THR(pFontCommand->Apply(_pCommandTarget, pmpStart, pmpEnd, &_varColor, TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_FONT, TRUE));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Back color.
    //

    if (V_VT(&_varBackColor) != VT_NULL)
    {
        pFontCommand = GetFontCommand(IDM_BACKCOLOR);
        hr = THR(pFontCommand->ConvertFormatDataToVariant(chFmtData, &var));
        if (hr)
            goto Cleanup;

        fFontPropertyChange = !pFontCommand->IsVariantEqual(&var, &_varBackColor);
        VariantClear(&var);

        if (fFontPropertyChange)
        {
            hr = THR(pFontCommand->Apply(_pCommandTarget, pmpStart, pmpEnd, &_varBackColor, TRUE));
            if (hr)
                goto Cleanup;

            hr = THR(UpdatePointerPositions(pmpStart, pmpEnd, TAGID_FONT, TRUE));
            if (hr)
                goto Cleanup;
        }
    }

    //
    // Move the caret to the position of the segment.
    //

    if (pCaret)
    {
        hr = THR(pmpCaret->MoveToPointer(fMoveCaretToStart?pmpStart:pmpEnd));
        if (hr)
            goto Cleanup;

        hr = THR(pCaret->MoveCaretToPointer( pmpCaret, TRUE, TRUE, CARET_DIRECTION_INDETERMINATE )); // BUGBUG : this should be cached when you get pmpCaret
        if (hr)
            goto Cleanup;
    }
    
Cleanup:

    //
    // Always reset spring loader at the end.
    //

    Reset();
    
    if (!fHasEnd)
    {
        ReleaseInterface(pmpEnd);
    }

    ReleaseInterface(pmpCaret);
    ReleaseInterface(pCaret);
    ReleaseInterface(pElemBlock);

    // 
    // Restore the gravity for pmpStart
    //
    if (eGravity != POINTER_GRAVITY_Max)
        IGNORE_HR(pmpStart->SetGravity(eGravity));

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpringLoader::Reset
//
//  Synopsis:   Resets the springloaded format information.
//
//----------------------------------------------------------------------------

void
CSpringLoader::Reset(IMarkupPointer * pmpPosition)
{
    // Don't reset springloader at the same position it was loaded.
    if (IsSpringLoadedAt(pmpPosition))
        return;

    ClearInterface(&_pmpPosition);

    _grfFlags() = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpringLoader::IsSpringLoadedAt
//
//  Synopsis:   Returns TRUE iff springloader is loaded at spec markup pointer.
//
//----------------------------------------------------------------------------

BOOL
CSpringLoader::IsSpringLoadedAt(IMarkupPointer * pmpPosition)
{
    HRESULT hr;
    int iPosition;

    if (!_fSpringLoaded || !_pmpPosition || !pmpPosition)
        return FALSE;

    hr = THR(OldCompare( _pmpPosition, pmpPosition, &iPosition));
    if (hr)
        return FALSE;
    
    return iPosition == SAME
        || (   iPosition == RIGHT
            && IsSeparatedOnlyByPhraseScopes(_pmpPosition, pmpPosition));
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::SetFormats
//
//  Synopsis:   Saves the format info passed in.
//
//-----------------------------------------------------------------------------

void
CSpringLoader::SetFormats( BOOL       fBold,
                           BOOL       fItalic,
                           BOOL       fUnderline,
                           BOOL       fSuperscript,
                           BOOL       fSubscript,
                           CVariant * pvarName,
                           CVariant * pvarSize,
                           CVariant * pvarColor,
                           CVariant * pvarBackColor )
{
    _fSuperscript = fSuperscript;
    _fSubscript = fSubscript;
    _fBold = fBold;
    _fItalic = fItalic;
    _fUnderline = fUnderline;
    VariantCopy(&_varName, pvarName);
    VariantCopy(&_varSize, pvarSize);
    VariantCopy(&_varColor, pvarColor);
    VariantCopy(&_varBackColor, pvarBackColor);
}


void
CSpringLoader::MarkSpringLoaded(IMarkupPointer * pmpPosition)
{
    IMarkupServices * pMarkupServices = GetMarkupServices();
    HRESULT hr;

    // Remember the position we are being springloaded at.
    ClearInterface(&_pmpPosition);

    if (pmpPosition)
    {
        hr = THR(pMarkupServices->CreateMarkupPointer(&_pmpPosition));
        if (hr)
            goto Cleanup;

        hr = THR(_pmpPosition->MoveToPointer(pmpPosition));
        if (hr)
            goto Cleanup;

        Assert(_pmpPosition);
    }

    _fSpringLoaded = TRUE;

Cleanup:
    return;
}


HRESULT
CSpringLoader::EnsureCaretPointers( IMarkupPointer  * pmpPosition,
                                    IHTMLCaret     ** ppCaret,
                                    IMarkupPointer ** ppmpCaret )
{
    IMarkupPointer    * pmpCaret = NULL;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    IHTMLCaret        * pCaret = NULL;
    BOOL                fEqual = FALSE;
    HRESULT             hr;

    Assert(_fSpringLoaded && pmpPosition && pMarkupServices && pViewServices);

    hr = THR(pViewServices->GetCaret(&pCaret));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpCaret));
    if (hr)
        goto Cleanup;

    hr = THR(pCaret->MovePointerToCaret(pmpCaret));
    if (hr)
        goto Cleanup;

    hr = THR(pmpCaret->IsEqualTo(pmpPosition, &fEqual));
    if (hr)
        goto Cleanup;

    if (!fEqual)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR(pmpCaret->SetGravity(POINTER_GRAVITY_Right));

Cleanup:

    if (hr)
    {
        hr = S_FALSE;
        ReleaseInterface(pmpCaret);
        ReleaseInterface(pCaret);
    }
    else
    {
        *ppmpCaret = pmpCaret;
        *ppCaret = pCaret;
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpringLoader::UpdatePointerPositions
//
//  Synopsis:   Moves springloader pointers into newly created, empty scope.
//
//----------------------------------------------------------------------------

HRESULT
CSpringLoader::UpdatePointerPositions( IMarkupPointer * pmpStart,
                                       IMarkupPointer * pmpEnd,
                                       ELEMENT_TAG_ID   tagIdScope,
                                       BOOL             fApplyElement )
{
    MARKUP_CONTEXT_TYPE mctContext;
    IHTMLElement      * pElemScope = NULL;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    ELEMENT_TAG_ID      tagId;
    HRESULT             hr = S_OK;
    BOOL                bEqual;

    Assert(_fSpringLoaded && pmpStart && pmpEnd && pMarkupServices);

    IFC( pmpStart->IsEqualTo(pmpEnd, &bEqual) );
    if (bEqual)
        return S_OK; // done - already in correct position

    //
    // Move start pointer.
    //

    hr = THR(GetViewServices()->RightOrSlave(pmpStart, FALSE, &mctContext, &pElemScope, NULL, NULL));
    if (hr)
        goto Cleanup;

    if (!pElemScope)
        goto MoveEndPointer;

    hr = THR(pMarkupServices->GetElementTagId(pElemScope, &tagId));
    if (hr)
        goto Cleanup;

    // If we are entering/exiting a scope as expected, go ahead and walk all pointers.
    if (   mctContext == (fApplyElement?CONTEXT_TYPE_EnterScope:CONTEXT_TYPE_ExitScope)
        && tagId == tagIdScope )
    {
        hr = THR(GetViewServices()->RightOrSlave(pmpStart, TRUE, NULL, NULL, NULL, NULL));
        if (hr)
            goto Cleanup;
    } // else an optimization prevented us from creating any gaps

MoveEndPointer:

    ClearInterface(&pElemScope);

    //
    // Move end pointer.
    //

    hr = THR(GetViewServices()->LeftOrSlave(pmpEnd, FALSE, &mctContext, &pElemScope, NULL, NULL));
    if (hr || !pElemScope)
        goto Cleanup;

    hr = THR(pMarkupServices->GetElementTagId(pElemScope, &tagId));
    if (hr)
        goto Cleanup;

    // If we are entering/exiting a scope as expected, go ahead and walk all pointers.
    if (   mctContext == (fApplyElement?CONTEXT_TYPE_EnterScope:CONTEXT_TYPE_ExitScope)
        && tagId == tagIdScope )
    {
        hr = THR(GetViewServices()->LeftOrSlave(pmpEnd, TRUE, NULL, NULL, NULL, NULL));
        if (hr)
            goto Cleanup;
    }

Cleanup:

    ReleaseInterface(pElemScope);
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::PrivateExec
//
//  Synopsis:   Opportunity for spring loader to intercept a command and - if
//              it hasn't already - prime itself.
//
//              This is only done for a selected number of IDM commands and
//              if the selection / range is empty (caret).
//
//  Returns:    S_OK if the command was handled.  S_FALSE if the command
//              has not been handled.
//
//-----------------------------------------------------------------------------

HRESULT
CSpringLoader::PrivateExec( DWORD             nCmdID,
                            VARIANTARG      * pvarargIn,
                            VARIANTARG      * pvarargOut,
                            ISegmentList    * pSegmentList )
{
    IMarkupPointer * pmpPosition = NULL;
    HRESULT          hr;

    Assert(GetMarkupServices());
    Assert(pSegmentList || _fSpringLoaded);
    
    //
    // Determine whether the spring loader can handle this command
    // and if so return start and end markup pointers.
    //

    hr = THR(CanHandleCommand(nCmdID, pSegmentList, &pmpPosition, pvarargIn));
    if (hr)
        goto Cleanup;

    //
    // Springload if we haven't already.
    //

    if (!_fSpringLoaded)
    {
        hr = THR(SpringLoad(pmpPosition));
        if (hr)
            goto Cleanup;
    }

    //
    // Cache / handle the command.
    //

    switch (nCmdID)
    {
    case IDM_BOLD:
        _fBold = !_fBold;
        break;
    case IDM_ITALIC:
        _fItalic = !_fItalic;
        break;
    case IDM_UNDERLINE:
        _fUnderline = !_fUnderline;
        break;
    case IDM_SUBSCRIPT:
        _fSubscript = !_fSubscript;
        break;
    case IDM_SUPERSCRIPT:
        _fSuperscript = !_fSuperscript;
        break;
    case IDM_FONTNAME:
        if (pvarargIn)
            VariantCopy(&_varName, pvarargIn);
        if (pvarargOut)
            VariantCopy(pvarargOut, &_varName);
        break;
    case IDM_FONTSIZE:
        if (pvarargIn)
            VariantCopy(&_varSize, pvarargIn);
            
        if (pvarargOut)
        {
            // Some apps assume VT_I4 is coming back (eg. Home Publisher)
            if (V_VT(&_varSize) == VT_BSTR)
            {
                if (FAILED(VariantChangeTypeSpecial(pvarargOut, &_varSize,  VT_I4)))
                {
                    VariantCopy(pvarargOut, &_varSize);                    
                }
            }
            else
            {
                VariantCopy(pvarargOut, &_varSize);
            }
        }
        break;
    case IDM_FORECOLOR:
        if (pvarargIn)
            VariantCopy(&_varColor, pvarargIn);
        if (pvarargOut)
            VariantCopy(pvarargOut, &_varColor);
        break;
    case IDM_BACKCOLOR:
        if (pvarargIn)
            VariantCopy(&_varBackColor, pvarargIn);
        if (pvarargOut)
            VariantCopy(pvarargOut, &_varBackColor);
        break;
    }

    if (pvarargOut && V_VT(pvarargOut)==VT_NULL)
    {
        // If the springloader couldn't answer, let the command answer.
        Assert(IDM_FONTNAME == nCmdID || IDM_FONTSIZE == nCmdID || IDM_FORECOLOR == nCmdID || IDM_BACKCOLOR == nCmdID);
        Assert(!pvarargIn);
        hr = S_FALSE;
    }

Cleanup:

    ReleaseInterface(pmpPosition);

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::PrivateQueryStatus
//
//  Synopsis:   Opportunity for spring loader to intercept a query status
//              request that has been cached in the spring loader, but hasn't
//              been applied to (or removed from) the selection / range.
//
//              This is only done for a selected number of IDM commands and if
//              the springloader is loaded.
//
//  Returns:    S_OK if the query was handled.  S_FALSE if the query
//              has not been handled.
//
//-----------------------------------------------------------------------------

HRESULT
CSpringLoader::PrivateQueryStatus(DWORD nCmdID, OLECMD rgCmds[])
{
    OLECMD * pCmd = &rgCmds[0];
    HRESULT  hr = S_FALSE;

    Assert(pCmd);

    if (!_fSpringLoaded)
        goto Cleanup;

    //
    // Query the spring loader for the command state.
    //

    hr = S_OK;

    switch (nCmdID)
    {
    case IDM_BOLD:
        pCmd->cmdf = _fBold ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
        break;
    case IDM_ITALIC:
        pCmd->cmdf = _fItalic ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
        break;
    case IDM_UNDERLINE:
        pCmd->cmdf = _fUnderline ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
        break;
    case IDM_SUBSCRIPT:
        pCmd->cmdf = _fSubscript ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
        break;
    case IDM_SUPERSCRIPT:
        pCmd->cmdf = _fSuperscript ? MSOCMDSTATE_DOWN : MSOCMDSTATE_UP;
        break;
    case IDM_FONTNAME:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;
    case IDM_FONTSIZE:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;
    case IDM_FORECOLOR:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;
    case IDM_BACKCOLOR:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;
    default:
        hr = S_FALSE;
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::CanHandleCommand
//
//  Synopsis:   Determines whether the selection passed in is empty and the
//              command is supported.
//
//  Returns:    S_OK if the command can be handled by the springloader.
//              S_FALSE if the command cannot been handled.
//
//              If S_OK, also return pmpPosition markup pointer.
//
//-----------------------------------------------------------------------------

HRESULT
CSpringLoader::CanHandleCommand( DWORD             nCmdID,
                                 ISegmentList    * pSegmentList,
                                 IMarkupPointer ** ppmpPosition,
                                 VARIANTARG      * pvarargIn )
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IMarkupPointer    * pmpStart, * pmpEnd;
    CSegmentListIter    iter;
    int                 iSegmentCount, iPosition;
    HRESULT             hr = S_OK;

    Assert(pMarkupServices);

    switch (nCmdID)
    {
    case IDM_BOLD:
    case IDM_ITALIC:
    case IDM_UNDERLINE:
    case IDM_SUBSCRIPT:
    case IDM_SUPERSCRIPT:
        break;

    case IDM_FONTNAME:
    case IDM_FONTSIZE:
    case IDM_FORECOLOR:
    case IDM_BACKCOLOR:
    case IDM_INSERTSPAN:

        if (_fSpringLoaded || pvarargIn)
            break;

        // fall through
    default:
        hr = S_FALSE;
        goto Cleanup;
    }

    if (!pSegmentList)
    {
        Assert(_fSpringLoaded);
        goto Cleanup;
    }

    //
    // Make sure we have an "empty" selection.
    //

    hr = THR(pSegmentList->GetSegmentCount(&iSegmentCount, NULL ));
    if (FAILED(hr))
        goto Cleanup;

    if (iSegmentCount != 1)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = iter.Init(pMarkupServices, GetViewServices(), pSegmentList);
    if (FAILED(hr))
        goto Cleanup;

    hr = iter.Next(&pmpStart, &pmpEnd);
    if (FAILED(hr))
        goto Cleanup;

    if (!pmpStart || !pmpEnd)
    {
        hr = S_FALSE;
        goto Cleanup;
    }


    if (ppmpPosition)
    {
        // BUGBUG: Outlook is creating an "nbsp" selection before the user
        // can type in a new line.  Thus the pmpStart and pmpEnd pointers are
        // temporarily in different positions, and that is why we perform
        // the following comparison only when we are actually springloading
        // (ppmpStart not NULL) and not when we are already springloaded.

        hr = THR(OldCompare( pmpStart, pmpEnd, &iPosition));
        if (hr)
            goto Cleanup;

        // If the position is different, selection 6is not empty.
        if (iPosition)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        //
        // At this point we know, we have an empty selection (range).
        //

        *ppmpPosition = pmpStart;
        pmpStart->AddRef();
    }

    hr = S_OK;

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::SpringLoadComposeSettings
//
//  Synopsis:   Loads the default font settings made by the host into the spring
//              loader.
//
//-----------------------------------------------------------------------------

HRESULT
CSpringLoader::SpringLoadComposeSettings(IMarkupPointer * pmpNewPosition, BOOL fReset, BOOL fOutsideSpan)
{
    Assert(_pCommandTarget && _pCommandTarget->GetEditor());
    struct COMPOSE_SETTINGS * pComposeSettings = _pCommandTarget->GetEditor()->GetComposeSettings();
    BOOL                      fSpringLoadAcrossLayout = FALSE;
    HRESULT                   hr = S_OK;

    if (fReset)
        Reset();

    // Determine whether we are allowed to springload compose settings at the
    // position specified.

    if (   !pComposeSettings
        || !pComposeSettings->_fComposeSettings
        || _fSpringLoaded) // don't springload again
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    // Check whether we can springload compose settings at the position passed in.
    //

    hr = THR(CanSpringLoadComposeSettings(pmpNewPosition, &fSpringLoadAcrossLayout, fOutsideSpan));
    if (hr || fSpringLoadAcrossLayout)
        goto Cleanup;

    //
    // Finally, springload compose settings.
    //

    Assert(pComposeSettings && pComposeSettings->_fComposeSettings);

    _fBold        = pComposeSettings->_fBold;
    _fItalic      = pComposeSettings->_fItalic;
    _fUnderline   = pComposeSettings->_fUnderline;
    _fSuperscript = pComposeSettings->_fSuperscript;
    _fSubscript   = pComposeSettings->_fSubscript;

    VariantCopy(&_varName, &pComposeSettings->_varFont);
    VariantCopy(&_varSpanClass, &pComposeSettings->_varSpanClass);

    if (pComposeSettings->_color & CColorValue::MASK_FLAG)
        V_VT(&_varColor)= VT_NULL;
    else
    {
        V_VT(&_varColor)    = VT_I4;
        V_I4(&_varColor)    = pComposeSettings->_color;
    }

    if (pComposeSettings->_colorBg & CColorValue::MASK_FLAG)
        V_VT(&_varBackColor)= VT_NULL;
    else
    {
        V_VT(&_varBackColor)= VT_I4;
        V_I4(&_varBackColor)= pComposeSettings->_colorBg;
    }

    if (pComposeSettings->_lSize == -1)
        V_VT(&_varSize)     = VT_NULL;
    else
    {
        V_VT(&_varSize)     = VT_I4;
        // Sizes in this spring loader are in the 1..7 range
        V_I4(&_varSize)     = pComposeSettings->_lSize;
    }

    // We are now springloaded.
    MarkSpringLoaded(pmpNewPosition);

Cleanup:

    RRETURN1(hr, S_FALSE);
}


HRESULT
CSpringLoader::CanSpringLoadComposeSettings(IMarkupPointer * pmpNewPosition, BOOL * pfCanOverrideSpringLoad, BOOL fOutsideSpan, BOOL fDontJump)
{
    IHTMLElement      * pElemContainer = NULL, * pElemWalk = NULL;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    BOOL                fInHTMLEditMode = IsDocumentInHTMLEditMode();
    ELEMENT_TAG_ID      tagidContainer;
    HRESULT             hr;

    Assert(pMarkupServices && pViewServices);

    // Not passing in a position pretty much means a free ticket to springloading.
    if (fInHTMLEditMode && !pmpNewPosition)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // If we are not in html edit mode, don't springload compose settings.
    if ( !fInHTMLEditMode )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Get our nearest parent element (scope).
    hr = THR(GetViewServices()->CurrentScopeOrSlave(pmpNewPosition, &pElemWalk));
    if (hr)
        goto Cleanup;

    // If we are not in a body container, don't springload.
    if (   S_OK != THR(EdUtil::FindContainer(pMarkupServices, pElemWalk, &pElemContainer))
        || !pElemContainer
        || S_OK != THR(pMarkupServices->GetElementTagId(pElemContainer, &tagidContainer))
        || tagidContainer != TAGID_BODY)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (fOutsideSpan)
    {
        // If we are in "format text outside any of our span tags in compose settings" mode, and
        // we are outside one of our span tags, allow springloading occur next to text.
        Assert(_pCommandTarget && _pCommandTarget->GetEditor());
        struct COMPOSE_SETTINGS * pComposeSettings = _pCommandTarget->GetEditor()->GetComposeSettings();
        if (pComposeSettings && pComposeSettings->_fUseOutsideSpan && !InSpanScope(pmpNewPosition))
            goto Cleanup;
    }

    // Walk up the parent element chain in search of the first block elemnent.  If it is
    // empty, allow springloading of compose settings.  If it isn't, don't allow it.
    while (pElemWalk)
    {
        IHTMLElement * pElemParent;
        BOOL           fBlockElement;

        // Look for a block element.
        hr = THR(pViewServices->IsBlockElement(pElemWalk, &fBlockElement));
        if (hr)
            goto Cleanup;

        // If we find a block element, we are making the decision now.
        if (fBlockElement)
        {
            // Blocks that already have formats associated with them
            // don't allow compose settings.
            if (IsCharFormatBlock(pElemWalk))
            {
                hr = S_FALSE;
            }
            else if (!IsBlockEmptyForSpringLoading(pmpNewPosition, pElemWalk, fDontJump))
            {
                if (!fDontJump)
                {
                    // If the position doesn't have formatting, attempt to springload
                    // formatting from text found nearby.
                    BOOL fSpringLoaded = SpringLoadFormatsAcrossLayout(pmpNewPosition, pElemWalk, pfCanOverrideSpringLoad!=NULL);
                    if (fSpringLoaded)
                    {
                        if (pfCanOverrideSpringLoad)
                            *pfCanOverrideSpringLoad = TRUE;
                        goto Cleanup;
                    }
                }

                hr = S_FALSE;
            }

            goto Cleanup;
        }

        hr = THR(pElemWalk->get_parentElement(&pElemParent));
        if (hr)
            goto Cleanup;

        ReleaseInterface(pElemWalk);
        pElemWalk = pElemParent;
    }

    hr = S_FALSE;

Cleanup:

    ReleaseInterface(pElemContainer);
    ReleaseInterface(pElemWalk);

    RRETURN1(hr, S_FALSE);
}


BOOL
CSpringLoader::IsCharFormatBlock(IHTMLElement * pElem)
{
    IMarkupServices * pMarkupServices = GetMarkupServices();
    BOOL              fResult = FALSE;
    ELEMENT_TAG_ID    tagid;
    HRESULT           hr;

    Assert(pMarkupServices);

    hr = THR(pMarkupServices->GetElementTagId(pElem, &tagid));
    if (hr || !pElem)
        goto Cleanup;

    fResult = tagid >= TAGID_H1 && tagid <= TAGID_H6 || tagid == TAGID_PRE;

Cleanup:

    return fResult;
}


BOOL
CSpringLoader::CanJumpOverElement(IHTMLElement * pElem)
{
    BOOL fCanJump = IsCharFormatBlock(pElem);

    if (!fCanJump)
    {
        ELEMENT_TAG_ID tagid;
        HRESULT hr = THR(GetMarkupServices()->GetElementTagId(pElem, &tagid));
        if (!hr && pElem && tagid == TAGID_A)
            fCanJump = TRUE;
    }

    if (!fCanJump)
        IGNORE_HR(GetViewServices()->IsLayoutElement(pElem, &fCanJump));

    return fCanJump;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::InSpanScope
//
//  Synopsis:   Checks whether the FIRST (!) span encountered up the parent
//              chain has the springloaded class.
//              loader.
//
//-----------------------------------------------------------------------------

BOOL
CSpringLoader::InSpanScope(IMarkupPointer * pmpPosition)
{
    IMarkupServices * pMarkupServices = GetMarkupServices();
    IHTMLElement    * pElem = NULL;
    ELEMENT_TAG_ID    tagId = TAGID_NULL;
    BOOL              fInSpanScope = FALSE;
    CVariant          varSpanClass;
    HRESULT           hr;

    Assert(pmpPosition && V_VT(&_varSpanClass) == VT_BSTR);

    //
    // Look for span on parent chain.
    //

    hr = THR(GetViewServices()->CurrentScopeOrSlave(pmpPosition, &pElem));
    if (hr)
        goto Cleanup;

    while (tagId != TAGID_SPAN)
    {
        IHTMLElement * pElemParent = NULL;

        if (!pElem)
            goto Cleanup;

        hr = THR(pMarkupServices->GetElementTagId(pElem, &tagId));
        if (hr)
            goto Cleanup;

        if (tagId == TAGID_SPAN)
            break;
        else if (tagId == TAGID_BODY)
            goto Cleanup;

        hr = THR(pElem->get_parentElement(&pElemParent));
        if (hr)
            goto Cleanup;

        ReleaseInterface(pElem);
        pElem = pElemParent;
    }

    // Found span.
    Assert(tagId == TAGID_SPAN && pElem);

    //
    // Check class name.
    //

    hr = THR(pElem->getAttribute(_T("class"), 0, &varSpanClass));
    if (hr)
        goto Cleanup;

    if (   V_VT(&varSpanClass) == VT_BSTR
        && !_tcscmp(V_BSTR(&varSpanClass), V_BSTR(&_varSpanClass)) )
    {
        fInSpanScope = TRUE;
    }

    if (!fInSpanScope)
    {
        hr = THR(pElem->getAttribute(_T("className"), 0, &varSpanClass));
        if (hr)
            goto Cleanup;

        if (   V_VT(&varSpanClass) == VT_BSTR
            && !_tcscmp(V_BSTR(&varSpanClass), V_BSTR(&_varSpanClass)) )
        {
            fInSpanScope = TRUE;
        }
    }

Cleanup:

    ReleaseInterface(pElem);
    return fInSpanScope;
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::IsSeparatedOnlyByPhraseScopes
//
//  Synopsis:   Checks if the two markup pointers are only separated by phrase
//              or span element enter scopes.
//
//-----------------------------------------------------------------------------

BOOL
CSpringLoader::IsSeparatedOnlyByPhraseScopes(IMarkupPointer * pmpLeft, IMarkupPointer * pmpRight)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    SP_IHTMLElement     spElemWalk;
    SP_IMarkupPointer   spmpWalk;
    MARKUP_CONTEXT_TYPE mctContext;
    int                 iPosition = RIGHT;
    BOOL                fRet = FALSE;
    ELEMENT_TAG_ID      tagId;
    HRESULT             hr;

    Assert(pMarkupServices && pmpLeft && pmpRight);
    Assert(S_OK == THR(OldCompare( pmpLeft, pmpRight, &iPosition)) && iPosition == RIGHT);

    IFC(CopyMarkupPointer(pMarkupServices, pmpLeft, &spmpWalk));

    while (S_OK == THR(OldCompare( spmpWalk, pmpRight, &iPosition)) && iPosition == RIGHT)
    {
        IFC(GetViewServices()->RightOrSlave(spmpWalk, TRUE, &mctContext, &spElemWalk, NULL, NULL));

        if (   (mctContext != CONTEXT_TYPE_EnterScope && mctContext != CONTEXT_TYPE_ExitScope)
            || !spElemWalk
            || S_OK != THR(pMarkupServices->GetElementTagId(spElemWalk, &tagId))
            || (!_pCommandTarget->GetEditor()->IsPhraseElement(spElemWalk) &&
                tagId != TAGID_DIV) // BUGBUG: 57834 Outlook 98 - allow going through DIVs
           )
        {
            goto Cleanup;
        }
    }

    fRet = iPosition == SAME;

Cleanup:

    return fRet;
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::IsDocumentInHTMLEditMode
//
//  Synopsis:   Checks if the document is in HTML edit mode for compose font.
//
//-----------------------------------------------------------------------------

BOOL
CSpringLoader::IsDocumentInHTMLEditMode()
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IOleCommandTarget * pDoc = NULL;
    OLECMD              cmd;
    BOOL                fDocumentInHTMLEditMode = TRUE;
    HRESULT             hr;

    hr = THR(pMarkupServices->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &pDoc));
    if (hr || !pDoc)
        goto Cleanup;

    cmd.cmdID = IDM_HTMLEDITMODE;
    hr = THR(pDoc->QueryStatus((GUID *)&CGID_MSHTML, 1, &cmd, NULL));
    fDocumentInHTMLEditMode = hr != S_OK || MSOCMDSTATE_DOWN == cmd.cmdf;

Cleanup:

    ReleaseInterface(pDoc);

    return fDocumentInHTMLEditMode;
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::IsBlockEmptyForSpringLoading
//
//  Synopsis:   Checks if the block element passed in is empty for spring
//              loading purposes. It is empty if it contains no chars or if
//              all the chars that it contains are synthetic chars.
//
//-----------------------------------------------------------------------------

BOOL
CSpringLoader::IsBlockEmptyForSpringLoading(IMarkupPointer * pmpStart, IHTMLElement * pElemBlock, BOOL fDontJump)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupPointer    * pmpBlockDelimiter = NULL;
    IMarkupPointer    * pmpWalk = NULL;
    IHTMLElement      * pElemWalk = NULL;
    ELEMENT_TAG_ID      tagId;
    MARKUP_CONTEXT_TYPE mctContext;
    long                cch;
    TCHAR               ch;
    BOOL                fEmpty = FALSE;
    int                 iResult, cNBSP = 0, iDirection;
    HRESULT             hr;

#if DBG==1
    BOOL fBlockElement;
    Assert(pMarkupServices);
    Assert(pElemBlock && GetViewServices() && S_OK == THR(GetViewServices()->IsBlockElement(pElemBlock, &fBlockElement)) && fBlockElement);
#endif // DBG==1

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpBlockDelimiter));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpWalk));
    if (hr)
        goto Cleanup;

    //
    // Look to the left and right in search for text.
    //

    for (iDirection = -1 ; iDirection <= 1 ; iDirection += 2)
    {
        hr = THR(pmpWalk->MoveToPointer(pmpStart));
        if (hr)
            goto Cleanup;

        if (iDirection == -1)
            hr = THR(pmpBlockDelimiter->MoveAdjacentToElement(pElemBlock, ELEM_ADJ_AfterBegin));
        else
            hr = THR(pmpBlockDelimiter->MoveAdjacentToElement(pElemBlock, ELEM_ADJ_BeforeEnd));
        if (hr)
            goto Cleanup;

        // Search left or right.
        while (S_OK == THR(OldCompare(pmpWalk, pmpBlockDelimiter, &iResult)) && (iResult*iDirection) < 0)
        {
            BOOL fStopLooking = FALSE;

            // Walk one character at a time.
            ClearInterface(&pElemWalk);
            cch = 1;
            if (iDirection == -1)
                hr = THR(pViewServices->LeftOrSlave(pmpWalk, TRUE, &mctContext, &pElemWalk, &cch, &ch));
            else
                hr = THR(pViewServices->RightOrSlave(pmpWalk, TRUE, &mctContext, &pElemWalk, &cch, &ch));
            if (hr)
                goto Cleanup;

            switch (mctContext)
            {
            case CONTEXT_TYPE_EnterScope:
                // We can jump over layouts (TABLE, nested DIV, BUTTON, etc).
                if (!fDontJump && pElemWalk && CanJumpOverElement(pElemWalk))
                    IGNORE_HR(pmpWalk->MoveAdjacentToElement(pElemWalk, ((iDirection==-1)?ELEM_ADJ_BeforeBegin:ELEM_ADJ_AfterEnd)));
                break;

            case CONTEXT_TYPE_NoScope:
                // BRs mean we're done looking in that direction.
                if (pElemWalk && S_OK == THR(pMarkupServices->GetElementTagId(pElemWalk, &tagId)) && TAGID_BR == tagId)
                    fStopLooking = TRUE;
                break;

            case CONTEXT_TYPE_Text:
                // If we find non-whitespace text, the search is over.
                if (cch && !IsWhiteSpace(ch) && (WCH_NBSP != ch || ++cNBSP >= 2))
                    goto Cleanup;
                break;
            }

            if (fStopLooking)
                break;
        }
    }

    // If we make it here, the blockelement is empty (has no text).
    fEmpty = TRUE;

Cleanup:

    ReleaseInterface(pmpBlockDelimiter);
    ReleaseInterface(pmpWalk);
    ReleaseInterface(pElemWalk);

    return fEmpty;
}


//+----------------------------------------------------------------------------
//
//  Method:     CSpringLoader::SpringLoadFormatsAcrossLayout
//
//  Synopsis:   If the markup ptr is next to a site, then springload using
//              formats copied from the other side of the site.
//
//-----------------------------------------------------------------------------

BOOL
CSpringLoader::SpringLoadFormatsAcrossLayout(IMarkupPointer * pmpPosition, IHTMLElement * pElemBlock, BOOL fActuallySpringLoad)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    IMarkupPointer    * pmpBlockDelimiter = NULL;
    IMarkupPointer    * pmpWalk = NULL;
    IMarkupPointer    * pmpAcrossLayout = NULL;
    IHTMLElement      * pElemWalk = NULL;
    IHTMLElement      * pElemToLeft = NULL, * pElemToRight = NULL;
    MARKUP_CONTEXT_TYPE mctContext;
    long                cch;
    TCHAR               ch;
    DWORD               dwSpringLoad = 0;
    int                 iResult, iDirection, cNBSP = 0;
    HRESULT             hr;

    Assert(pMarkupServices && pViewServices && pmpPosition && pElemBlock);

    //
    // 1. Optimization and important first cut:  If we are directly next to real (formatted) text, don't override formatting.
    //

    cch = 1;
    if (   S_OK != THR(pViewServices->LeftOrSlave(pmpPosition, FALSE, &mctContext, &pElemToLeft, &cch, &ch))
        || (mctContext == CONTEXT_TYPE_Text && cch == 1 && !IsWhiteSpace(ch) && WCH_NBSP != ch))
        goto Cleanup;
    cch = 1;
    if (   S_OK != THR(pViewServices->RightOrSlave(pmpPosition, FALSE, &mctContext, &pElemToRight, &cch, &ch))
        || (mctContext == CONTEXT_TYPE_Text && cch == 1 && !IsWhiteSpace(ch) && WCH_NBSP != ch))
        goto Cleanup;

    //
    // 2. Next see if an adjust for insert gets us next to text: First left, then right.
    //

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpWalk));
    if (hr)
        goto Cleanup;

    // Check on the left side.
    // Only if we don't have a jumpable element.  This assumes adjust for insert doesn't leave tags.
    if (!pElemToLeft || !CanJumpOverElement(pElemToLeft))
    {
        hr = THR(pmpWalk->MoveToPointer(pmpPosition));
        if (hr)
            goto Cleanup;

        //
        // BUGBUG (johnbed) hard-coding TRUE for fNotAtBOL is wrong here. We need to propigate
        // it from the source of pmpWalk
        //
    
        cch = 1;
        if (   S_OK == THR(AdjustPointerForInsert(pmpWalk, TRUE, LEFT, LEFT))
            && S_OK == THR(pViewServices->LeftOrSlave(pmpWalk, FALSE, &mctContext, NULL, &cch, &ch))
            && mctContext == CONTEXT_TYPE_Text && cch == 1 && !IsWhiteSpace(ch) && WCH_NBSP != ch)
        {
            dwSpringLoad = SL_ADJUST_FOR_INSERT_LEFT;
            pmpAcrossLayout = pmpWalk;
            pmpWalk = NULL;
            goto SpringLoad;
        }
    }

    // Check on the right side.
    // Only if we don't have a jumpable element.  This assumes adjust for insert doesn't leave tags.
    if (!pElemToRight || !CanJumpOverElement(pElemToRight))
    {
        hr = THR(pmpWalk->MoveToPointer(pmpPosition));
        if (hr)
            goto Cleanup;

        //
        // BUGBUG (johnbed) hard-coding TRUE for fNotAtBOL is wrong here. We need to propigate
        // it from the source of pmpWalk
        //
    
        cch = 1;
        if (   S_OK == THR(AdjustPointerForInsert(pmpWalk, TRUE, RIGHT, RIGHT))
            && S_OK == THR(pViewServices->RightOrSlave(pmpWalk, FALSE, &mctContext, NULL, &cch, &ch))
            && mctContext == CONTEXT_TYPE_Text && cch == 1 && !IsWhiteSpace(ch) && WCH_NBSP != ch)
        {
            dwSpringLoad = SL_ADJUST_FOR_INSERT_RIGHT;
            pmpAcrossLayout = pmpWalk;
            pmpWalk = NULL;
            goto SpringLoad;
        }
    }

    //
    // 3. Look for formatted text on the other side of elements: First left then right.
    //

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpBlockDelimiter));
    if (hr)
        goto Cleanup;

    for (iDirection = -1 ; iDirection <= 1 ; iDirection += 2)
    {
        // Start out walk pointer at position specified.
        hr = THR(pmpWalk->MoveToPointer(pmpPosition));
        if (hr)
            goto Cleanup;

        // Set up pointer on block element start or end.
        if (iDirection == -1)
            hr = THR(pmpBlockDelimiter->MoveAdjacentToElement(pElemBlock, ELEM_ADJ_AfterBegin));
        else
            hr = THR(pmpBlockDelimiter->MoveAdjacentToElement(pElemBlock, ELEM_ADJ_BeforeEnd));
        if (hr)
            goto Cleanup;

        // Start walking.
        while (S_OK == THR(OldCompare(pmpWalk, pmpBlockDelimiter, &iResult)) && (iResult*iDirection) < 0)
        {
            BOOL fFoundRealText = FALSE;
            BOOL fOutOfBounds = FALSE;

            // Walk one character at a time.
            ClearInterface(&pElemWalk);
            cch = 1;
            if (iDirection == -1)
                hr = THR(pViewServices->LeftOrSlave(pmpWalk, TRUE, &mctContext, &pElemWalk, &cch, &ch));
            else
                hr = THR(pViewServices->RightOrSlave(pmpWalk, TRUE, &mctContext, &pElemWalk, &cch, &ch));
            if (hr)
                goto Cleanup;

            // If we find an element, see if we can jump over it in order to find the formatted text
            // on its other side.
            switch (mctContext)
            {
            case CONTEXT_TYPE_EnterScope:
                // Can jump over layouts (TABLE, nested DIV, BUTTON, etc), charformat blockelements and anchors.
                if (pElemWalk && CanJumpOverElement(pElemWalk))
                {
                    hr = THR(pmpWalk->MoveAdjacentToElement(pElemWalk, ((iDirection==-1)?ELEM_ADJ_BeforeBegin:ELEM_ADJ_AfterEnd)));
                    if (hr)
                        goto Cleanup;

                    hr = THR(OldCompare(pmpWalk, pmpBlockDelimiter, &iResult));
                    if (hr)
                        goto Cleanup;

                    fOutOfBounds = (iResult*iDirection) >= 0;
                }
                // fall through

            case CONTEXT_TYPE_NoScope:
                if (   pElemWalk
                    && !pmpAcrossLayout)
                {
                    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpAcrossLayout));
                    if (hr)
                        goto Cleanup;
                }
                break;

            case CONTEXT_TYPE_Text:
                // Check if we found REAL text (ignore spaces).
                fFoundRealText = cch && !IsWhiteSpace(ch) && (WCH_NBSP != ch || ++cNBSP >= 2);
                break;
            }

            if (fOutOfBounds)
                break;
            
            if (fFoundRealText)
            {
                if (pmpAcrossLayout)
                    dwSpringLoad = ((iDirection==-1) ? SL_ADJUST_FOR_INSERT_LEFT : SL_ADJUST_FOR_INSERT_RIGHT);

                Assert(pmpAcrossLayout || !dwSpringLoad);
                goto SpringLoad;
            }

            if (pmpAcrossLayout)
            {
                // Remember the position in front of the LEFTMOST element.
                hr = THR(pmpAcrossLayout->MoveToPointer(pmpWalk));
                if (hr)
                    goto Cleanup;
            }
        }

        ClearInterface(&pmpAcrossLayout);
    }

SpringLoad:

    if (dwSpringLoad && fActuallySpringLoad)
    {
        Assert(pmpAcrossLayout);

        // Load springloader with format settings found at pmpAcrossLayout.
        // Given that we are here, we know we have found text, and therefore no AdjustForInsert is necessary.
        hr = THR(SpringLoad(pmpAcrossLayout, 0));
        if (hr)
            goto Cleanup;

        Reposition(pmpPosition);
    }

Cleanup:

    ReleaseInterface(pmpWalk);
    ReleaseInterface(pmpBlockDelimiter);
    ReleaseInterface(pmpAcrossLayout);
    ReleaseInterface(pElemWalk);
    ReleaseInterface(pElemToLeft);
    ReleaseInterface(pElemToRight);
    
    return dwSpringLoad != 0;
}


// Springloader copy of adjust pointer for insert (in case no tracker is around).

HRESULT
CSpringLoader::AdjustPointerForInsert(IMarkupPointer * pWhereIThinkIAm, BOOL fNotAtBOL, INT inBlockcDir, INT inTextDir)
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spLeftEdge;
    SP_IMarkupPointer spRightEdge;    
    CHTMLEditor * pEd = _pCommandTarget->GetEditor();
    CSelectionManager * pSelMan = pEd->GetSelectionManager();

    IFC( pSelMan->GetMarkupServices()->CreateMarkupPointer( &spLeftEdge ));
    IFC( pSelMan->GetMarkupServices()->CreateMarkupPointer( &spRightEdge ));
    IFC( pSelMan->MovePointersToContext( spLeftEdge, spRightEdge ));
    hr = THR( pEd->AdjustPointer( pWhereIThinkIAm, fNotAtBOL, inBlockcDir, inTextDir, spLeftEdge, spRightEdge, ADJPTROPT_AdjustIntoURL));
    if( hr == E_HITBOUNDARY )
        hr = S_OK;

Cleanup:
    RRETURN( hr );
}
