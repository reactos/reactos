//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       MiscCmd.cxx
//
//  Contents:   Implementation of miscellaneous edit commands
//
//  Classes:    CComposeSettingsCommand
//
//  History:    08-05-98 - OliverSe - created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDUTIL_HXX_
#define _X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

using namespace EdUtil;

MtDefine(CComposeSettingsCommand, EditCommand, "CComposeSettingsCommand");
MtDefine(COverwriteCommand, EditCommand, "COverwriteCommand");
MtDefine(CAutoDetectCommand, EditCommand, "CAutoDetectCommand");
MtDefine(CMovePointerToSelectionCommand, EditCommand, "CMovePointerToSelectionCommand");
MtDefine(CLocalizeEditorCommand, EditCommand, "CLocalizeEditorCommand");

CRITICAL_SECTION CComposeSettingsCommand::s_csLastComposeSettings;
BSTR             CComposeSettingsCommand::s_bstrLastComposeSettings = NULL;

//+------------------------------------------------------------------------
//
//  Function: CComposeSettingsCommand::PrivateExec
//
//  Synopsis: Pass compose settings to the springloader
//
//-------------------------------------------------------------------------

HRESULT
CComposeSettingsCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    // If an inArg and an outArg was specified, this means we are querying the
    // compose font.
    if (pvarargIn && pvarargOut)
    {
        return THR(QueryComposeSettings(pvarargIn, pvarargOut));
    }

    struct COMPOSE_SETTINGS * pComposeSettings = NULL;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IHTMLViewServices * pViewServices = GetViewServices();
    CSpringLoader     * psl = GetSpringLoader();
    IHTMLCaret        * pCaret = NULL;
    IMarkupPointer    * pmpCaret = NULL;
    BOOL                fToggleComposeSettings = FALSE;
    HRESULT             hr;

    Assert(GetEditor());

    //
    // Check parameters.
    //

    if (!pvarargIn || (V_VT(pvarargIn) != VT_BSTR && V_VT(pvarargIn) != VT_BOOL))
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    //
    // If specified, parse and memorize compose settings.
    //

    if (V_VT(pvarargIn) == VT_BSTR)
    {
        pComposeSettings = GetEditor()->EnsureComposeSettings();
        if (!pComposeSettings)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        Assert(pvarargIn && V_VT(pvarargIn) == VT_BSTR);

        hr = THR(ParseComposeSettings(V_BSTR(pvarargIn), pComposeSettings));
        if (hr)
            goto Error;

        fToggleComposeSettings = TRUE;
    }
    else
    {
        BOOL fComposeSettings;

        fToggleComposeSettings = TRUE;
        pComposeSettings = GetEditor()->GetComposeSettings(fToggleComposeSettings);

        Assert(pvarargIn && V_VT(pvarargIn) == VT_BOOL);
        fComposeSettings = !!V_BOOL(pvarargIn);

        if (!pComposeSettings || !!pComposeSettings->_fComposeSettings == fComposeSettings)
            goto Cleanup;

        pComposeSettings->_fComposeSettings = fComposeSettings;
    }

    //
    // Springload compose settings at the caret.
    //

    hr = THR(pViewServices->GetCaret(&pCaret));
    if (hr || !pCaret)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpCaret));
    if (!hr && pmpCaret)
    {
        if (S_OK != THR(pCaret->MovePointerToCaret(pmpCaret)))
        {
            ClearInterface(&pmpCaret);
        }
    }

    IGNORE_HR(psl->SpringLoadComposeSettings(pmpCaret, fToggleComposeSettings));

Cleanup:

    if (!fToggleComposeSettings)
    {
        Assert(pvarargIn && V_VT(pvarargIn) == VT_BSTR);
        EnterCriticalSection(&s_csLastComposeSettings);

        if (s_bstrLastComposeSettings)
            SysFreeString(s_bstrLastComposeSettings);

        s_bstrLastComposeSettings = SysAllocString(V_BSTR(pvarargIn));

        LeaveCriticalSection(&s_csLastComposeSettings);
    }

    hr = S_OK;

Error:

    ReleaseInterface(pmpCaret);
    ReleaseInterface(pCaret);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CComposeSettingsCommand::ParseComposeSettings
//
//  Synopsis:   This function parses the string coming in and sets up the
//              default composition font.
//
//  Params:     [pbstrComposeSettings]: A BSTR containing the string telling
//                                      us the font settings to use.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CComposeSettingsCommand::ParseComposeSettings(
    BSTR bstrComposeSettings,
    struct COMPOSE_SETTINGS * pComposeSettings )
{
    typedef enum {
        IT_INT,
        IT_COLOR,
        IT_STRING
    } INFO_TYPE;

    static const struct {
        INFO_TYPE itType;
        DWORD     offset;
    } itParseTable[] = {
        {IT_INT,   offsetof(struct COMPOSE_SETTINGS, _fBold)      },
        {IT_INT,   offsetof(struct COMPOSE_SETTINGS, _fItalic)    },
        {IT_INT,   offsetof(struct COMPOSE_SETTINGS, _fUnderline) },
        {IT_INT,   offsetof(struct COMPOSE_SETTINGS, _lSize)      },
        {IT_COLOR, offsetof(struct COMPOSE_SETTINGS, _color)      },
        {IT_COLOR, offsetof(struct COMPOSE_SETTINGS, _colorBg)    },
        {IT_STRING,offsetof(struct COMPOSE_SETTINGS, _varFont)    },
        {IT_STRING,offsetof(struct COMPOSE_SETTINGS, _varSpanClass)},
        {IT_INT,   offsetof(struct COMPOSE_SETTINGS, _fUseOutsideSpan)},
    };

    const TCHAR FIELD_SEPARATOR = _T(',');
    const TCHAR COLOR_SEPARATOR = _T('.');
    const INT REQUIRED_FIELDS = 7; // 8th and 9th fields optional
    TCHAR   achComposeFont[LF_FACESIZE];
    TCHAR * pstr;
    TCHAR * pstrEnd;
    DWORD_PTR pDest;
    INT     index;
    HRESULT hr = E_INVALIDARG;

    Assert(pComposeSettings);

    // Setup the default compose settings.
    SetDefaultComposeSettings(pComposeSettings);

    // Get the string.
    pstr = bstrComposeSettings;

    // Index is used to walk the parse table.
    index = 0;

    while (index < ARRAY_SIZE(itParseTable))
    {
        // Skip commas: empty fields indicate that we should use
        // defaults for that field.
        while(   *pstr == FIELD_SEPARATOR
              && *pstr != 0
              && index < ARRAY_SIZE(itParseTable)
             )
        {
            index++;
            pstr++;
        }

        // Last field was omitted, so quit.
        if (index >= ARRAY_SIZE(itParseTable))
            break;

        // Ran out of the string early --> invalid arg, return
        if (*pstr == 0)
        {
            if (index >= REQUIRED_FIELDS)
                hr = S_OK;
            goto Cleanup;
        }

        pstrEnd = NULL;
        pDest = ((DWORD_PTR)pComposeSettings) + itParseTable[index].offset;
        switch (itParseTable[index].itType)
        {
        case IT_INT:
            *((INT *)pDest) = wcstol(pstr, &pstrEnd, 10);
            break;

        case IT_COLOR:
        {

            INT Colors[3];
            INT i;
            for (i = 0; i < 3; i++)
            {
                Colors[i] = wcstol(pstr, &pstrEnd, 10);

                Assert(    *pstrEnd == COLOR_SEPARATOR
                       || (*pstrEnd == FIELD_SEPARATOR && (i == 2))
                      );
                if  (!(    *pstrEnd == COLOR_SEPARATOR
                       || (*pstrEnd == FIELD_SEPARATOR && (i == 2))
                      )
                    )
                    goto Cleanup;

                // RGB values are separated by a '.'. Go past them.
                pstr = pstrEnd + 1;
            }

            // Finally construct the color.
            *((INT *)pDest) = RGB(Colors[0], Colors[1], Colors[2]);
            break;
        }

        case IT_STRING:
        {
            INT    i = 0;
            TCHAR *pDestChar = achComposeFont;
            INT iSize = ARRAY_SIZE(achComposeFont);

            //
            // Loop thru the string till we reach the end or a comma.
            //

            while (*pstr != 0 && *pstr != FIELD_SEPARATOR && i < iSize)
            {
                // Copy the character into the destination
                *pDestChar++ = *pstr++;
                i++;
            }

            // If some character was copied to the destination then we need
            // to put in the NULL terminator. If no character was copied, then
            // we want to leave the destn as it was since, we want to retain
            // the default values in it.
            if (i > 0)
            {
                // If the while loop terminated because we ran out of space
                // in the destination then put the \0 in the last possible space
                // of the destination.
                if (i >= iSize)
                    pDestChar--;

                // Finally, terminate the string.
                *pDestChar = 0;

                // *(LONG *)pDest = fc().GetAtomFromFaceName(achComposeFont);
                // _tcscpy((TCHAR *)pDest, achComposeFont);
                {
                    CVariant * pvarDest = (CVariant *)pDest;
                    V_VT(pvarDest) = VT_BSTR;
                    THR(EdUtil::FormsAllocString(achComposeFont, &V_BSTR(pvarDest)));
                }
            }

            // This is where the string field ended.
            pstrEnd = pstr;
            break;
        }

        default:
            AssertSz(0, "Unexpected type");
            goto Cleanup;
        }

        // We are either at the EOI, or end of a field
        Assert(*pstrEnd == 0 || *pstrEnd == FIELD_SEPARATOR);
        if  (!(*pstrEnd == 0 || *pstrEnd == FIELD_SEPARATOR))
            goto Cleanup;

        // Read next field from the string
        index++;

        if (*pstrEnd == 0)
        {
            if (index >= REQUIRED_FIELDS)
                hr = S_OK;
            goto Cleanup;
        }

        pstr = pstrEnd + 1;
    }

    hr = S_OK;

Cleanup:

    // Do we have any compose settings?
    pComposeSettings->_fComposeSettings = hr == S_OK;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CComposeSettingsCommand::SetDefaultComposeSettings
//
//  Synopsis:   This function sets up the default values in compose settings.
//
//  Returns:    Nothing.
//
//----------------------------------------------------------------------------

void
CComposeSettingsCommand::SetDefaultComposeSettings(struct COMPOSE_SETTINGS * pComposeSettings)
{
    Assert(pComposeSettings);

    //
    // By default, no underline, bold or italic
    //

    pComposeSettings->_fBold        = FALSE;
    pComposeSettings->_fItalic      = FALSE;
    pComposeSettings->_fUnderline   = FALSE;
    pComposeSettings->_fSuperscript = FALSE;
    pComposeSettings->_fSubscript   = FALSE;

    // Compose sizes in option settings are in 1-7 range
    pComposeSettings->_lSize        = -1;

    // By default, FG and BG undefined
    pComposeSettings->_color        = VALUE_UNDEF;
    pComposeSettings->_colorBg      = VALUE_UNDEF;

    // By default, fontface and spanclass are undefined.
    VariantClear(&pComposeSettings->_varFont);
    V_VT(&pComposeSettings->_varFont)= VT_NULL;

    VariantClear(&pComposeSettings->_varSpanClass);
    V_VT(&pComposeSettings->_varSpanClass)= VT_NULL;

    // Don't use compose settings everywhere outside span by default.
    pComposeSettings->_fUseOutsideSpan = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CComposeSettingsCommand::ExtractLastComposeSettings
//
//  Synopsis:   This function extracts the compose settings of a previous
//              document if we don't have any.
//
//  Returns:    Nothing.
//
//----------------------------------------------------------------------------

void
CComposeSettingsCommand::ExtractLastComposeSettings(CHTMLEditor * pEditor, BOOL fEditorHasComposeSettings)
{
    Assert(pEditor);

    if (!s_bstrLastComposeSettings)
        return;

    EnterCriticalSection(&s_csLastComposeSettings);

    if (s_bstrLastComposeSettings)
    {
        //
        // If we don't have compose settings, parse and memorize last compose settings.
        //

        if (!fEditorHasComposeSettings)
        {
            // Ensure compose settings.
            struct COMPOSE_SETTINGS * pComposeSettings = pEditor->EnsureComposeSettings();
            if (pComposeSettings)
            {
                IGNORE_HR(ParseComposeSettings(s_bstrLastComposeSettings, pComposeSettings));
            }
        }

        SysFreeString(s_bstrLastComposeSettings);
        s_bstrLastComposeSettings = NULL;
    }

    LeaveCriticalSection(&s_csLastComposeSettings);
}


//+------------------------------------------------------------------------
//
//  Function:  CComposeSettingsCommand::QueryComposeSettings
//
//  Synopsis:  Return information about the current compose settings
//
//  Arguments: pvarargIn:  Which IDM command to return the value for.
//                         If this is of type VT_UNKNOWN, we interpret
//                         it as a markup pointer, and ask the springloader
//                         whether it would springload at that position.
//
//                         If no markup pointer is passed in, we are
//                         asking about the current (springloaded) font
//                         instead.
//
//             pvarargOut: The value of the IDM command as found in
//                         the compose settings.
//
//  Returns:   VT_NULL in pvarargOut if we don't have compose settings or
//             they don't apply to the position of the markup pointer
//             passed in.
//
//-------------------------------------------------------------------------

HRESULT
CComposeSettingsCommand::QueryComposeSettings( 
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    CHTMLEditor * pEditor = GetEditor();
    struct COMPOSE_SETTINGS * pComposeSettings = pEditor->GetComposeSettings();  // BUGBUG: Make sure not to "extract-last" here.
    CSpringLoader * psl = GetSpringLoader();
    HRESULT hr = S_OK;
    DWORD cmdId;

    Assert(pvarargIn && pvarargOut);
    Assert(V_VT(pvarargIn) == VT_I4 || V_VT(pvarargIn) == VT_UNKNOWN);
    Assert(psl);

    V_VT(pvarargOut) = VT_I4;

    if (VT_UNKNOWN == V_VT(pvarargIn))
    {
        IMarkupPointer * pmpPosition = ((IMarkupPointer *)V_UNKNOWN(pvarargIn));
        psl->OverrideComposeSettings(FALSE);

        if (pmpPosition)
        {
            hr = THR(psl->CanSpringLoadComposeSettings(pmpPosition, NULL, FALSE, TRUE));
            if (hr)
                goto CantLoadComposeSettings;

        }
        else
        {
            if (!psl->IsSpringLoaded())
                goto CantLoadComposeSettings;

            // Springloader overrides composesettings.
            psl->OverrideComposeSettings(TRUE);
        }


        V_I4(pvarargOut) = 1;
    }
    else if (psl->IsSpringLoaded() && psl->OverrideComposeSettings())
    {
        cmdId = V_I4(pvarargIn);

        switch (cmdId)
        {
        case IDM_BOLD:
        case IDM_ITALIC:
        case IDM_UNDERLINE:
        case IDM_SUPERSCRIPT:
        case IDM_SUBSCRIPT:
        {
            OLECMD cmd;
            hr = THR(psl->PrivateQueryStatus(cmdId, &cmd));
            if (hr)
                goto CantLoadComposeSettings;

            V_I4(pvarargOut) = cmd.cmdf == MSOCMDSTATE_DOWN;
            break;
        }
        case IDM_FONTSIZE:
        case IDM_FONTNAME:
        case IDM_FORECOLOR:
        case IDM_BACKCOLOR:
            hr = THR(psl->PrivateExec(cmdId, NULL, pvarargOut, NULL));
            if (hr)
                goto CantLoadComposeSettings;

            break;
        }
    }
    else
    {
        cmdId = V_I4(pvarargIn);

        if (!pComposeSettings)
            goto CantLoadComposeSettings;

        switch (cmdId)
        {
        case IDM_BOLD:
            V_I4(pvarargOut) = pComposeSettings->_fBold;
            break;
        case IDM_ITALIC:
            V_I4(pvarargOut) = pComposeSettings->_fItalic;
            break;
        case IDM_UNDERLINE:
            V_I4(pvarargOut) = pComposeSettings->_fUnderline;
            break;
        case IDM_SUPERSCRIPT:
            V_I4(pvarargOut) = pComposeSettings->_fSuperscript;
            break;
        case IDM_SUBSCRIPT:
            V_I4(pvarargOut) = pComposeSettings->_fSubscript;
            break;
        case IDM_FONTSIZE:
            V_I4(pvarargOut) = pComposeSettings->_lSize;
            break;
        case IDM_FONTNAME:
            VariantCopy(pvarargOut, &(pComposeSettings->_varFont));
            break;
        case IDM_FORECOLOR:
            V_I4(pvarargOut) = pComposeSettings->_color;
            break;
        case IDM_BACKCOLOR:
            V_I4(pvarargOut) = pComposeSettings->_colorBg;
            break;
        case IDM_INSERTSPAN:
            VariantCopy(pvarargOut, &(pComposeSettings->_varSpanClass));
            break;
        }
    }

Cleanup:

    RRETURN(hr);

CantLoadComposeSettings:

    V_VT(pvarargOut) = VT_NULL;
    hr = S_OK;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Function: CComposeSettingsCommand::PrivateQueryStatus
//
//-------------------------------------------------------------------------

HRESULT
CComposeSettingsCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext )

{
    if (pCmd)
        pCmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Function: CComposeSettingsCommand::Init (static)
//
//-------------------------------------------------------------------------

void
CComposeSettingsCommand::Init()
{
    InitializeCriticalSection(&s_csLastComposeSettings);

    s_bstrLastComposeSettings = NULL;
}


//+------------------------------------------------------------------------
//
//  Function: CComposeSettingsCommand::Deinit (static)
//
//-------------------------------------------------------------------------

void
CComposeSettingsCommand::Deinit()
{
    if (s_bstrLastComposeSettings)
        SysFreeString(s_bstrLastComposeSettings);

    s_bstrLastComposeSettings = NULL;

    DeleteCriticalSection(&s_csLastComposeSettings);
}


//+------------------------------------------------------------------------
//
//  Function: COverwriteCommand::PrivateQueryStatus
//
//-------------------------------------------------------------------------

HRESULT
COverwriteCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext )

{
    if (pCmd)
    {
        if (!GetEditor()->GetSelectionManager()->IsContextEditable())
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        }
        else
        {        
            pCmd->cmdf = (GetEditor()->GetSelectionManager()->GetOverwriteMode()) 
                                          ? MSOCMDSTATE_DOWN
                                          : MSOCMDSTATE_UP;
        }
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function: COverwriteCommand::PrivateExec
//
//-------------------------------------------------------------------------

HRESULT
COverwriteCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT             hr                 = S_OK;
    CSelectionManager   *pSelectionManager = GetEditor()->GetSelectionManager();
    CVariant            var;

    if (pvarargOut && V_VT(pvarargOut) == VT_BOOL)
    {
        // Output param is specified, return current stat of overwrite flag and bool
        V_BOOL(pvarargOut) = (pSelectionManager->GetOverwriteMode())
                ? VB_TRUE
                : VB_FALSE;
    }
    else
    {
        if (!pSelectionManager->IsContextEditable())
        {
            hr = MSOCMDERR_E_DISABLED;
            goto Cleanup;
        }

        if (pvarargIn)
        {

            // Try to convert the argument to boolean
            IFC( VariantChangeType(&var, pvarargIn, 0, VT_BOOL) );
            if (hr == S_OK)
            {
                pSelectionManager->SetOverwriteMode(V_BOOL(&var) == VB_TRUE);
            }
            else 
            {                
                pSelectionManager->SetOverwriteMode(FALSE);
                hr = S_OK;
            }
        }
        else
        {
            // No input argument was specified, toggle the overwrite flag
            pSelectionManager->SetOverwriteMode(!pSelectionManager->GetOverwriteMode());
        }

    }

Cleanup:
    RRETURN(hr);

}


//+------------------------------------------------------------------------
//
//  Function: CAutoDetectCommand::PrivateQueryStatus
//
//-------------------------------------------------------------------------

HRESULT
CAutoDetectCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext )

{
    pCmd->cmdf = MSOCMDSTATE_UP;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Function: CAutoDetectCommand::PrivateExec
//
//-------------------------------------------------------------------------

HRESULT
CAutoDetectCommand::PrivateExec( 
    DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    INT                 iSegmentCount;
    INT                 i;

    IFC( GetSegmentList( &spSegmentList ));
    
    IFC( GetMarkupServices()->CreateMarkupPointer( & spStart ) );
    IFC( GetMarkupServices()->CreateMarkupPointer( & spEnd ) );
    IFC( spSegmentList->GetSegmentCount( & iSegmentCount, NULL ) );

    for (i=0; i < iSegmentCount; i++)
    {
        IFC( spSegmentList->MovePointersToSegment ( i, spStart, spEnd ) );

        IFC( AutoUrl_DetectRange( GetMarkupServices(), spStart, spEnd, FALSE ) );
    }


Cleanup:
    RRETURN(hr);

}

//+------------------------------------------------------------------------
//
//  Function: CLocalizeEditorCommand::PrivateQueryStatus
//
//-------------------------------------------------------------------------
HRESULT
CLocalizeEditorCommand::PrivateQueryStatus(
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    if (!pCmd)
        return E_INVALIDARG;

    pCmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}    


HRESULT 
CLocalizeEditorCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT     hr;
    HINSTANCE   hinst;

    if (pvarargIn == NULL || V_VT(pvarargIn) != VT_BOOL)
        return E_INVALIDARG;

    if (V_BOOL(pvarargIn))
    {
        // Use localized resource dll
        IFR( GetEditResourceLibrary(&hinst) );    
        CGetBlockFmtCommand::LoadDisplayNames(hinst);
    }
    else
    {
        // Use local english version
        CGetBlockFmtCommand::LoadDisplayNames(g_hInstance);
    }
    
    return S_OK;    
}

