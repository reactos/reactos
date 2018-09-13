/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "variant.hxx"

DEFINE_CLASS_MEMBERS(Variant, _T("Variant"), Base);

Variant::Variant()
{
    VariantInit(&variant);
}

Variant::Variant(int i)
{
    VariantInit(&variant);
    variant.vt = VT_I4;
    V_I4(&variant) = i;
}

Variant::Variant(String * s)
{
    VariantInit(&variant);
    variant.vt = VT_BSTR;
    V_BSTR(&variant) = s->getBSTR();
}

void Variant::finalize()
{
    VariantClear(&variant);
}

String * Variant::toString()
{
    String * s;
    if (variant.vt != VT_BSTR)
    {
        Variant * v = new Variant();
        HRESULT hr = VariantChangeTypeEx(&v->variant, &variant,
           LOCALE_USER_DEFAULT, 0, VT_BSTR);
        s = hr ? String::nullString() : String::newString(V_BSTR(&v->variant));
    }
    else
    {
        s = String::newString(V_BSTR(&variant));
    }
    return s;
}

Object * Variant::toObject()
{
    switch(variant.vt)
    {
    case VT_I4:
        return Integer::newInteger(V_I4(&variant));
    case VT_BSTR:
        return String::newString(V_BSTR(&variant));
    case VT_DISPATCH:
        {
            // return null string for empty variant...
            if (V_DISPATCH(&variant))
            {
                Object * o;
                HRESULT hr = (V_DISPATCH(&variant))->QueryInterface(IID_Object, (void **)&o);
                if (!hr)
                    return o;
            }
            // FALL THRU !!!
        }
    default:
        return String::emptyString();
    }
}

IUnknown * 
Variant::QIForIID( VARIANT * pVar, const IID * piid)
{
    // CLEANUP - we could one day re-work this method to use getUnknown
    HRESULT hr;
    IUnknown * punk = null;
    if ((pVar->vt == VT_UNKNOWN) || (pVar->vt == VT_DISPATCH))
    {
        if (!pVar->punkVal)
            return null;
        else
            hr = pVar->punkVal->QueryInterface( *piid, (void **)&punk);
        if (hr || !punk)
            Exception::throwE((HRESULT)(hr == S_OK ? E_FAIL : hr));
        return punk;
    }
    else if ((pVar->vt == (VT_BYREF|VT_UNKNOWN)) || (pVar->vt == (VT_BYREF|VT_DISPATCH)))
    {
        if (!pVar->ppunkVal || !*(pVar->ppunkVal))
            return null;
        else
            hr = (*(pVar->ppunkVal))->QueryInterface( *piid, (void **)&punk);
        if (hr || !punk)
            Exception::throwE((HRESULT)(hr == S_OK ? E_FAIL : hr));
        return punk;
    }
    else if (pVar->vt == (VT_BYREF|VT_VARIANT))
    {
        if (!pVar->punkVal)
            hr = E_INVALIDARG;
        else
        {
            hr = S_OK;
            punk = QIForIID(pVar->pvarVal, piid);
        }
        if (hr || !punk)
            Exception::throwE((HRESULT)(hr == S_OK ? E_FAIL : hr));
        return punk;
    }
    else if (pVar->vt == VT_NULL  ||
             pVar->vt == VT_EMPTY ||
             pVar->vt == VT_ERROR)
        return null;

    Exception::throwE((HRESULT)E_INVALIDARG);
    return null; // to make the compiler happy...
}

IUnknown* // static method
Variant::getUnknown(VARIANT* varObject) 
{
    //
    // Extract the IUnknown from the VARIANT.  We handle VT_UNKNOWN and VT_DISPATCH,
    // with or without VT_BYREF.  Using VariantChangeType is potentially problematic, 
    // since it does a QI - not to mention slow !
    //
    IUnknown* punk = NULL;

    if ((V_VT(varObject) & VT_UNKNOWN) == VT_UNKNOWN)
    {
        if ((V_VT(varObject) & VT_BYREF) == VT_BYREF)
        {
            punk = (LPUNKNOWN) (*V_UNKNOWNREF(varObject));
        }
        else
        {
            punk = (LPUNKNOWN) V_UNKNOWN(varObject);
        }
    }
    //
    // If it's not an IUnknown, try IDispatch
    // 
    else if ((V_VT(varObject) & VT_DISPATCH) == VT_DISPATCH)
    {

        if ((V_VT(varObject) & VT_BYREF) == VT_BYREF)
        {
            punk = (LPUNKNOWN) (*V_DISPATCHREF(varObject));
        }
        else
        {
            punk = (LPUNKNOWN) V_DISPATCH(varObject);
        }
    }
    else if (varObject->vt == (VT_BYREF|VT_VARIANT))
    {
        if (NULL != varObject->pvarVal)
        {
            // recurrse the VT_BYREF until we find a primitive type !
            punk = getUnknown(varObject->pvarVal);
        }
    }
    return punk;
}
