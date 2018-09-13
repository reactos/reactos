//+------------------------------------------------------------------------
//
//  File:       prophelp.cxx
//
//  Contents:   Some functions to help in dealing with object properties
//
//  Functions:  GetCommonPropertyValue
//              SetCommonPropertyValue
//
//  History:    29-Jun-93   SumitC      Created.
//              26-Oct-93   DonCl       Error code usage
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

HRESULT
SetFontProperty(
        UINT        cUnk,
        IUnknown ** apUnk,
        LOGFONT     lf,
        CY          cy);

//+------------------------------------------------------------------------
//
//  Function:   IsSameFontValue
//
//  Synopsis:   Checks to see if two font objects have the same value
//
//-------------------------------------------------------------------------

BOOL
IsSameFontValue(VARIANT * pvar1, VARIANT * pvar2)
{
    HRESULT     hr = S_OK;
    IFont *     pFont1 = NULL;
    IFont *     pFont2 = NULL;

    Assert(V_VT(pvar1) == VT_DISPATCH);
    Assert(V_VT(pvar2) == VT_DISPATCH);

    hr = V_DISPATCH(pvar1)->QueryInterface(IID_IFont, (void **)&pFont1);
    if (hr)
        goto Cleanup;

    hr = V_DISPATCH(pvar2)->QueryInterface(IID_IFont, (void **)&pFont2);
    if (hr)
        goto Cleanup;

    hr = pFont1->IsEqual(pFont2);

Cleanup:
    ReleaseInterface(pFont1);
    ReleaseInterface(pFont2);
    return hr ? FALSE : TRUE;
}


//+---------------------------------------------------------------
//
//  Member:     FindFontObject
//
//  Synopsis:   Find font object in a control set
//
//  Notes:      Helper function for OpenFontDialog()
//
//---------------------------------------------------------------
#ifndef NO_PROPERTY_PAGE
HRESULT
FindFontObject(
        UINT        cUnk,
        IUnknown ** apUnk,
        IFont **    ppFont)
{
    HRESULT         hr = S_OK;
    UINT            i;
    VARIANT         var;
    IDispatch *     pDispatch = NULL;

    // Find the first control supports font
    for (i = 0; i < cUnk; i++)
    {
        hr = apUnk[i]->QueryInterface(IID_IDispatch, (LPVOID *) &pDispatch);
        if (hr)
            goto Cleanup;

        hr = GetDispProp(
            pDispatch,
            DISPID_FONT,
            g_lcidUserDefault,
            &var,
            NULL);

        if (hr)
        {
            // The control does not suport font, continue on next one
            ClearInterface(&pDispatch);
            VariantClear(&var);
            continue;
        }
        else
        {
            // Find a font object, stop here
            Assert(V_VT(&var) == VT_DISPATCH);
            hr = V_DISPATCH(&var)->QueryInterface(IID_IFont, (void **)ppFont);
            goto Cleanup;
       }
    }

Cleanup:
    ReleaseInterface(pDispatch);
    VariantClear(&var);
    RRETURN(hr);
}
#endif // NO_PROPERTY_PAGE

HRESULT
GetCommonSubObjectPropertyValue(
        DISPID      dispidMainObject,
        DISPID      dispidSubObject,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar)
{
    HRESULT     hr                  = S_OK;
    UINT        i;
    VARIANT     var;
    VARIANT     varDispatch;
    BOOL        fCommonValueExists  = TRUE;
    EXCEPINFO   ei;

    VariantInit(pVar);
    VariantInit(&var);
    VariantInit(&varDispatch);

    InitEXCEPINFO(&ei);

    if (cDisp == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (i = 0; fCommonValueExists && i < cDisp; i++)
    {
        hr = GetDispProp(
                apDisp[i], 
                dispidMainObject,
                g_lcidUserDefault,
                &varDispatch,
                &ei);

        if (hr)
            goto Error;

        // Now get the sub object identified by dispidSubObject
        hr = GetDispProp(
                V_DISPATCH(&varDispatch), 
                dispidSubObject,
                g_lcidUserDefault,
                i == 0 ? pVar : &var,
                &ei);

        if (hr)
            goto Error;

        if (i > 0)
        {
            fCommonValueExists = IsVariantEqual(pVar, &var);
        }

        VariantClear(&var);
        VariantClear(&varDispatch);
    }

Cleanup:
    if (hr)
    {
        IGNORE_HR(SetErrorInfoFromEXCEPINFO(&ei));
    }

    if (!hr)
    {
        hr = !fCommonValueExists;
    }

    FreeEXCEPINFO(&ei);
    RRETURN2(hr, S_FALSE, DISP_E_MEMBERNOTFOUND);

Error:
    VariantClear(pVar);
    VariantClear(&varDispatch);
    VariantClear(&var);
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Function:   GetCommonPropertyValue
//
//  Synopsis:   Checks to see if an array of objects all have the same value
//              for a given property, returning the value of that property
//              if so.
//
//  Arguments:  [dispid]        --  Property id
//              [cDisp]         --  Number of objects in array
//              [apDisp]        --  Array of objects
//              [pVar]          --  The shared property value is returned
//                                  in *pVar
//
//  Notes:
//              If all the objects support the given property, but the
//              values don't match, this function returns S_FALSE.  The
//              variant returned will have the value of the first
//              object's property.
//
//              If cUnk == 0, this function returns E_INVALIDARG
//
//  Returns:    HRESULT (STDAPI); S_OK, S_FALSE, or error
//              Also sets the current error object.
//
//-------------------------------------------------------------------------

HRESULT
GetCommonPropertyValue(
        DISPID      dispid,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar)
{
    HRESULT     hr                  = S_OK;
    UINT        i;
    VARIANT     var;
    BOOL        fCommonValueExists  = TRUE;
    EXCEPINFO   ei;

    VariantInit(pVar);
    VariantInit(&var);
    InitEXCEPINFO(&ei);

    if (cDisp == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (i = 0; fCommonValueExists && i < cDisp; i++)
    {
        hr = GetDispProp(
                apDisp[i], 
                dispid,
                g_lcidUserDefault,
                i == 0 ? pVar : &var,
                &ei);

        if (hr)
            goto Error;

        if (i > 0)
        {
            if (DISPID_FONT == dispid)
            {
                fCommonValueExists = IsSameFontValue(pVar, &var);
            }
            else
            {
                fCommonValueExists = IsVariantEqual(pVar, &var);
            }
        }

        VariantClear(&var);
    }

Cleanup:
    if (hr)
    {
        IGNORE_HR(SetErrorInfoFromEXCEPINFO(&ei));
    }

    if (!hr)
    {
        hr = !fCommonValueExists;
    }

    FreeEXCEPINFO(&ei);
    RRETURN2(hr, S_FALSE, DISP_E_MEMBERNOTFOUND);

Error:
    VariantClear(pVar);
    goto Cleanup;
}

HRESULT
SetCommonSubObjectPropertyValue(
        DISPID      dispidMainObject,
        DISPID      dispidSubObject,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar)
{
    HRESULT     hr       = S_OK;
    UINT        i;
    EXCEPINFO   ei;
    VARIANT     varDispatch;

    VariantInit(&varDispatch);

    InitEXCEPINFO(&ei);
    if (cDisp == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (i = 0; i < cDisp; i++)
    {
        hr = GetDispProp(
                apDisp[i], 
                dispidMainObject,
                g_lcidUserDefault,
                &varDispatch,
                &ei);

        if (hr)
            goto Cleanup;

        hr = SetDispProp(
                V_DISPATCH(&varDispatch),
                dispidSubObject,
                g_lcidUserDefault,
                pVar,
                &ei);

        VariantClear(&varDispatch);

        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr)
    {
        IGNORE_HR(SetErrorInfoFromEXCEPINFO(&ei));
    }

    FreeEXCEPINFO(&ei);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   SetCommonPropertyValue
//
//  Synopsis:   Sets a property value on an array of objects
//
//  Arguments:  [dispid]        --  Property id
//              [cDisp]         --  Number of objects in array
//              [apDisp]        --  Objects
//              [pVar]          --  New property value
//
//  Notes:      If cUnk == 0, this function returns E_INVALIDARG
//
//  Returns:    HRESULT (STDAPI)
//              Also sets the current error object.
//
//-------------------------------------------------------------------------

HRESULT
SetCommonPropertyValue(
        DISPID      dispid,
        UINT        cDisp,
        IDispatch ** apDisp,
        VARIANT *   pVar)
{
    HRESULT     hr       = S_OK;
    UINT        i;
    EXCEPINFO   ei;

    InitEXCEPINFO(&ei);
    if (cDisp == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (i = 0; i < cDisp; i++)
    {
        hr = SetDispProp(
                apDisp[i],
                dispid,
                g_lcidUserDefault,
                pVar,
                &ei);

        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (hr)
    {
        IGNORE_HR(SetErrorInfoFromEXCEPINFO(&ei));
    }

    FreeEXCEPINFO(&ei);
    RRETURN(hr);
}


#ifndef NO_PROPERTY_PAGE
UINT_PTR CALLBACK ChooseFontCallback
(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp
)
{
#if 0
// BUGBUG: (anandra)
    if (msg == WM_INITDIALOG)
    {
        HWND hwndItem = NULL;

        //
        //remove color chooser
        //
//lookatme
//        hwndItem = GetDlgItem(hwnd, cmb4);
        if (hwndItem)
        {
            ShowWindow(hwndItem, SW_HIDE);
        }
//lookatme
//        hwndItem = GetDlgItem(hwnd, stc4);
        if (hwndItem)
        {
            ShowWindow(hwndItem, SW_HIDE);
        }
    }
#endif
    return 0;
}

//+------------------------------------------------------------------------
//
//  Function:   OpenFontDialog
//
//  Synopsis:   Open a font dialog, which is used to set font property.
//
//  Arguments:  [pBase]         --  point to class where this func is called
//                                  It is used to create undo action
//              hWnd            --  point to owner window of dialog
//              [cUnk]          --  Number of objects in array
//              [apUnk]         --  Array of objects
//              [fUndo]         --  Indicate if need to create undo action
//              [pfRet]         --  reture value indicating if use click OK
//
//
//  Notes:
//              The objects are not IDispatch pointers, just regular old
//              IUnknowns.  If an object in the array doesn't support
//              IID_IDispatch, this function will return an error.
//
//              The font value displayed in dialog is the first
//              object's property.
//
//              If cUnk == 0, this function returns E_INVALIDARG
//
//  Returns:    HRESULT (STDAPI); S_OK, S_FALSE, or error
//              Also sets the current error object.
//
//-------------------------------------------------------------------------
HRESULT
OpenFontDialog(
        CBase *     pBase,
        HWND        hWnd,
        UINT        cUnk,
        IUnknown ** apUnk,
        BOOL *      pfRet)
{
    HRESULT         hr;
    LOGFONT         lf;
    CHOOSEFONT      cf;
    IFont *         pFont = NULL;
    CY              cy;
    BOOL            fBold = FALSE;
    BOOL            fItalic = FALSE;
    BOOL            fUnderline = FALSE;
    BOOL            fStrikethrough = FALSE;
#ifdef _MAC
    short           sWeight;
#endif
    BSTR            bstr = NULL;
    BOOL            fRet = FALSE;
    short           sCharSet = FALSE;
    
    cy.Lo = cy.Hi = 0;

    if (cUnk == 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(FindFontObject(cUnk, apUnk, &pFont));
    if (hr)
        goto Cleanup;

    // Get current font information
    if (pFont)
    {
        pFont->get_Name(&bstr);
        pFont->get_Size(&cy);
        pFont->get_Bold(&fBold);
        pFont->get_Italic(&fItalic);
        pFont->get_Underline(&fUnderline);
        pFont->get_Strikethrough(&fStrikethrough);
#ifdef _MAC
        pFont->get_Weight(&sWeight);
#endif
        pFont->get_Charset(&sCharSet);
    }

    // Fill in LOGFONT struct
    memset(&lf, 0, sizeof(LOGFONT));
    _tcsncpy(lf.lfFaceName, bstr, SysStringLen(bstr));
    lf.lfHeight = (int) (cy.Lo * g_sizePixelsPerInch.cy / 720000L);

    lf.lfItalic     = !!fItalic;
    lf.lfWeight     = fBold ? 700: 400;
    lf.lfStrikeOut  = !!fStrikethrough;
    lf.lfUnderline  = !!fUnderline;
    lf.lfCharSet    = (BYTE) sCharSet;
#ifdef _MAC
    // mac note: we are using the high order bits of the IFont weight member
    //      to hold the outline and shadow flags.  Since the IFont Weight
    //      interface has the weight as a short, we must move the flags
    //      to the "long" end of the lfWeight field passing it on.
    if( sWeight & MAC_OUTLINE )
        lf.lfWeight     |= FW_OUTLINE;
    if( sWeight & MAC_SHADOW )
        lf.lfWeight     |= FW_SHADOW;
#endif

    // Fill in CHOOSEFONT struct
    memset(&cf, 0, sizeof(CHOOSEFONT));
    cf.lStructSize  = sizeof(CHOOSEFONT);
    cf.hwndOwner    = hWnd;
    cf.hDC          = NULL;
    cf.lpLogFont    = &lf;
    cf.iPointSize   = cy.Lo/1000;
    cf.Flags        = CF_SCREENFONTS |
                      CF_INITTOLOGFONTSTRUCT |
                      CF_FORCEFONTEXIST |
                      CF_EFFECTS |
                      CF_ENABLEHOOK;
    cf.lCustData    = 0;
    cf.lpfnHook     = ChooseFontCallback;
    cf.hInstance    = NULL;
    cf.nFontType    = SCREEN_FONTTYPE;

    //
    //  Open font dialog
    //
    fRet = ChooseFont(&cf);
    if (fRet)
    {
#ifndef NO_EDIT
        CParentUndoUnit *pCPUU = NULL;

        pCPUU = pBase->OpenParentUnit(pBase, IDS_UNDOPROPCHANGE);
#endif // NO_EDIT
        cy.Lo = cf.iPointSize * 1000;

        hr = SetFontProperty(cUnk, apUnk, lf, cy);
#ifndef NO_EDIT
        pBase->CloseParentUnit(pCPUU, hr);
#endif // NO_EDIT
    }

Cleanup:
    if (pfRet)
    {
        *pfRet = fRet;
    }
    FormsFreeString(bstr);
    ReleaseInterface(pFont);

    RRETURN(hr);
}
#endif // NO_PROPERTY_PAGE



//+---------------------------------------------------------------
//
//  Member:     SetFontProperty
//
//  Synopsis:   Set font property to selected objects
//
//  Notes:      Helper function for OpenFontDialog()
//
//---------------------------------------------------------------
HRESULT
SetFontProperty(
        UINT        cUnk,
        IUnknown ** apUnk,
        LOGFONT     lf,
        CY          cy)
{
    HRESULT         hr = S_OK;
    IDispatch *     pDispatch = NULL;
    IFont *         pFont = NULL;
    BSTR            bstr = NULL;
    VARIANT         var;
    UINT            i;

    //MAKEBSTR does nothing on Intel Platform, recent change in
    //CFontNew::put_NameHelper(BSTR bstrName) of fontutil.cxx
    //has some code calling SysStringLen, which must be a real
    //BSTR
    hr = FormsReAllocString(&bstr , lf.lfFaceName);
    if (hr)
        goto Cleanup;

    VariantInit(&var);
    for (i = 0; i < cUnk; i++)
    {
        hr = apUnk[i]->QueryInterface(IID_IDispatch, (LPVOID *) &pDispatch);
        if (hr)
            goto Cleanup;

        hr = GetDispProp(
                pDispatch,
                DISPID_FONT,
                g_lcidUserDefault,
                &var,
                NULL);
        if (hr)
            goto Cleanup;

        Assert(V_VT(&var) == VT_DISPATCH);
        hr = V_DISPATCH(&var)->QueryInterface(IID_IFont, (void **)&pFont);
        if (hr)
            goto Cleanup;

        pFont->put_Size(cy);
        pFont->put_Name(bstr);
        pFont->put_Bold((lf.lfWeight == 400) ? FALSE : TRUE);
        pFont->put_Italic(lf.lfItalic);
        pFont->put_Underline(lf.lfUnderline);
        pFont->put_Strikethrough(lf.lfStrikeOut);
        pFont->put_Charset(lf.lfCharSet);
#ifdef _MAC
        // mac note: we are using the high order bits of the weight member
        //      to hold the outline and shadow flags.  Since the IFont put_Weight
        //      interface has the weight as a short, we must move the flags
        //      to the "short" end of the field before casting it out.
        if( lf.lfWeight & FW_OUTLINE )
            lf.lfWeight     |= MAC_OUTLINE;
        if( lf.lfWeight & FW_SHADOW )
            lf.lfWeight     |= MAC_SHADOW;

        pFont->put_Weight((short)lf.lfWeight);
#endif

        ClearInterface(&pDispatch);
        ClearInterface(&pFont);
        VariantClear(&var);
    }

Cleanup:
    ReleaseInterface(pDispatch);
    ReleaseInterface(pFont);
    FormsFreeString(bstr);
    VariantClear(&var);
    RRETURN(hr);
}
