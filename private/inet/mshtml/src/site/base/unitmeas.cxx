//+---------------------------------------------------------------------
//
//   File:      uniutmeas.cxx
//
//  Contents:   Unit Measurement element class, etc..
//
//  Classes:    CUnitMeasurement, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_UNITMEAS_HXX_
#define X_UNITMEAS_HXX_
#include "unitmeas.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#define _cxx_
#include "unitmeas.hdl"

DEFINE_SUBOBJECT_CREATOR(CUnitMeasurement );

CBase::CLASSDESC CUnitMeasurement::s_classdesc =
{
    &CLSID_UnitMeasurement,         // _pclsid
    0,                              // _idrBase
    NULL,                           // property pages
    0,                              // _ccp
    0,                              // _pcpi
    0,                              // _dwFlags
    &IID_IUnitMeasurement,          // _piidDispinterface
    0,                              // _lAccRole
    0,                              // _pPropCats;
    0,                              // _appropdescs;
    s_apVTableInterf,               // _apvtableinterf
};

CUnitMeasurement::CUnitMeasurement ( CElement *pElemObj,
    PROPERTYDESC *pPropdesc,
    MEASURETYPE MeasureType )
{
    Assert ( pElemObj );
    Assert ( pPropdesc );
    _pElem = pElemObj;
    _pPropdesc = pPropdesc;
    _MeasureType = MeasureType;
    // Ref count the main object so it doesn't go away
    _pElem -> AddRef();
}

HRESULT CUnitMeasurement::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
    default:
        if (iid == IID_IUnitMeasurement)
        {
           *ppv = (IUnitMeasurement *) this;
        }
        else if ( iid == CLSID_UnitMeasurement )
        {
            // Internaly used QI to get the object ptr, just cast return type to class ptr
            *ppv = this;
            // Weak ref, don't AddRef() object
            return S_OK;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT CUnitMeasurement::GetStoredUnitValue ( CUnitValue *puvValue )
{
    HRESULT hr = CallHandler ( HANDLEPROP_AUTOMATION, (void *)puvValue );
    RRETURN ( hr );
}

HRESULT CUnitMeasurement::CallHandler ( DWORD dwFlags, void *pvArg )
{
    BASICPROPPARAMS *pbpp = (BASICPROPPARAMS *)(_pPropdesc+1);
    HRESULT hr;
    CBase  *pBaseObject;
    void *pSubObject;

    if ( pbpp->dwPPFlags & PROPPARAM_ATTRARRAY )
    {
        pSubObject = (void *)&_pElem->_pAA;
    }
    else
    {
        pSubObject = (void *)_pElem;
    }
    pBaseObject = _pElem;

    hr = THR ( CALL_METHOD(_pPropdesc, _pPropdesc->pfnHandleProperty,
        ( dwFlags  , pvArg, pBaseObject, (CVoid *)pSubObject ) ));

    RRETURN ( hr );
}

HRESULT CUnitMeasurement::SetStoredUnitValue ( CUnitValue uvValue )
{
    HRESULT hr;

    hr = CallHandler ( HANDLEPROP_SET | HANDLEPROP_AUTOMATION, (void *)&uvValue );
    RRETURN ( hr );
}

long CUnitMeasurement::GetValueFromPixelRect ( void )
{
    POINT pos;
    SIZE  size;

    pos.x = pos.y = szie.cx size.cy = 0;

    if ( OTHERX != _MeasureType && OTHERY != _MeasureType &&
         STYLEOTHERX != _MeasureType && STYLEOTHERY != _MeasureType)
    {   // get's the layout position of site regardless if it is parked
        CLayout *pLayout = _pElem->GetUpdatedLayout();

        if(pLayout)
            pLayout->GetUnparkedPosition( &pos, &size );
    }

    switch ( _MeasureType )
    {
    case TOP:
    case STYLETOP:
        return pos.y;

    case LEFT:
    case STYLELEFT:
        return pos.x;

    case WIDTH:
    case STYLEWIDTH:
        return size.cx;

    case HEIGHT:
    case STYLEHEIGHT:
        return size.cy;

    default:
        return 0;
    }
}


HRESULT CUnitMeasurement::SetPixelValue ( long lValue )
{
    CUnitValue uvValue;
    HRESULT hr;

    // Set the value to the equivalent value of lValue pixels when expressed in
    // the current units of the unit value
    hr = GetStoredUnitValue ( &uvValue );
    if ( hr )
        goto Cleanup;

    if ( IsMeasureInXDirection() )
        hr = THR(uvValue.XSetFloatValueKeepUnits ( (float)lValue, CUnitValue::UNIT_PIXELS,
            GetValueFromPixelRect(), 1 ));
    else
        hr = THR(uvValue.YSetFloatValueKeepUnits ( (float)lValue, CUnitValue::UNIT_PIXELS,
            GetValueFromPixelRect(),1  ) );

    if ( hr )
        goto Cleanup;

    hr = THR(SetStoredUnitValue ( uvValue ));

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::put_value (float vValue)
{
    CUnitValue uvValue;
    CUnitValue::UNITVALUETYPE uvt;
    HRESULT hr;

#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif

    // vValue represents the value of the property in the current
    // document units ( document.DefaultUnits ). Set the internal
    // value but keep the persisted unit the same.
    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    uvt = uvValue.GetUnitType();

    // BUGBUG rgardner. If we are put'ing an OTHERX/OTHERY, which has no
    // pixel value to get, we won't be able to convert if the unit is
    // currently in percent.
    if ( ( _MeasureType == OTHERX || _MeasureType == OTHERY || _MeasureType == STYLEOTHERX || _MeasureType == STYLEOTHERY ) &&
        ( uvt == CUnitValue::UNIT_PERCENT || uvt == CUnitValue::UNIT_TIMESRELATIVE ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if ( IsMeasureInXDirection() )
    
        hr = THR(uvValue.XSetFloatValueKeepUnits ( vValue, GetDocUnits(),
            GetValueFromPixelRect(), 1 ));
    else
        hr = THR(uvValue.YSetFloatValueKeepUnits ( vValue, GetDocUnits(),
            GetValueFromPixelRect(), 1 ) );

    if ( hr )
        goto Cleanup;

    hr = SetStoredUnitValue ( uvValue );

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}



HRESULT CUnitMeasurement::get_value ( float *pvValue )
{
    // Get the current value into pvValue, expressed in the
    // default document units
    CUnitValue uvValue;
    HRESULT hr;

#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif
	
    if ( !pvValue )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    if ( ! ( _MeasureType == OTHERX || _MeasureType == OTHERY || _MeasureType == STYLEOTHERX || _MeasureType == STYLEOTHERY ||
        CUnitValue::IsScalerUnit ( uvValue.GetUnitType() ) ) )
    {
        uvValue.SetValue ( GetValueFromPixelRect(), CUnitValue::UNIT_PIXELS );
    }

    if ( IsMeasureInXDirection() )
        *pvValue = uvValue.XGetFloatValueInUnits ( GetDocUnits()); 
    else
        *pvValue = uvValue.YGetFloatValueInUnits ( GetDocUnits());
Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );

}

HRESULT CUnitMeasurement::put_unit ( BSTR bstrUnit )
{
    HRESULT hr;
    CUnitValue uvValue;
    DWORD dwPPFlags;
    CUnitValue::UNITVALUETYPE uvt;

    htmlUnits Unit;

    hr = ENUMFROMSTRING ( htmlUnits, bstrUnit, (long *)&Unit );
    if ( hr )
        goto Cleanup;

#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif

    // Convert unit value into new units
    uvt =  (CUnitValue::UNITVALUETYPE) Unit;

    if ( uvt < 0 || uvt > CUnitValue::UNIT_TIMESRELATIVE )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    dwPPFlags = _pPropdesc -> GetPPFlags();

    // Is the unit valid as identified in the propdesc
    // but pixels are always valid
    if ( ( CUnitValue::IsScalerUnit ( uvt ) && 
                (uvt != CUnitValue::UNIT_PIXELS) && 
                !(dwPPFlags & PROPPARAM_LENGTH ) ) ||
         ( uvt == CUnitValue::UNIT_TIMESRELATIVE && !(dwPPFlags & PROPPARAM_TIMESRELATIVE) ) ||
         ( uvt == CUnitValue::UNIT_PERCENT && !(dwPPFlags & PROPPARAM_PERCENTAGE) ) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Set the persisted HTML unit,
    // Convert value to new units
    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    if ( IsMeasureInXDirection() )
        hr = uvValue.XConvertToUnitType ( uvt, GetValueFromPixelRect(), 1 );
    else
        hr = uvValue.YConvertToUnitType ( uvt, GetValueFromPixelRect(), 1 );

    if ( hr )
        goto Cleanup;


    // Store the new value away
    hr = SetStoredUnitValue ( uvValue );

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::get_unit (BSTR *pbstrUnit)
{
    HRESULT hr ;
    CUnitValue uvValue;
    htmlUnits Unit;

#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif
    if ( !pbstrUnit )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Get the unit type from the persisted value
    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    Unit = (htmlUnits) uvValue.GetUnitType() ;

    hr = STRINGFROMENUM ( htmlUnits, (long)Unit, pbstrUnit );

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::put_unitValue ( float fValue )
{
    CUnitValue uvValue;
    HRESULT hr;

#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif

    // vValue represents the value of the property in the current
    // document units ( document.DefaultUnits ). Set the internal
    // value but keep the persisted unit the same.
    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    hr = THR ( uvValue.SetFloatUnitValue ( fValue ) );

    if ( hr )
        goto Cleanup;

    // Store the new value away
    hr = SetStoredUnitValue ( uvValue );

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::get_unitValue (float * pfUnitValue)
{
    HRESULT hr ;
    CUnitValue uvValue;

    if ( !pfUnitValue )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = E_INVALIDARG;
        goto Cleanup;
    }    
#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif

    hr = THR ( GetStoredUnitValue ( &uvValue ) );
    if ( hr )
        goto Cleanup;

    if ( uvValue.IsNull() )
    {
        *pfUnitValue = 0.0;
    }
    else
    {
        if ( IsMeasureInXDirection() )
            *pfUnitValue = uvValue.XGetFloatValueInUnits ( uvValue.GetUnitType() );
        else
            *pfUnitValue = uvValue.YGetFloatValueInUnits ( uvValue.GetUnitType() );
    }
Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::put_htmlText ( BSTR bstrText )
{
    HRESULT hr;
    
#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif
    
    hr = CallHandler ( HANDLEPROP_SET | HANDLEPROP_AUTOMATION |
        (PROPTYPE_LPWSTR << 16), (void *)bstrText );
#ifdef RGARDNER
Cleanup:
#endif
    RRETURN ( SetErrorInfo ( hr ) );
}

HRESULT CUnitMeasurement::get_htmlText (BSTR * pbstrText)
{
    VARIANT vt;
    HRESULT hr;

    if ( !pbstrText )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = E_INVALIDARG;
        goto Cleanup;
    }
#ifdef RGARDNER
    if ( !_pElem->NeedsLayout() )
    {
        // Not Valid to set unit measurements on non-site objects
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }
#endif

    hr = CallHandler ( HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
        (void *)&vt );

    // I know the UnitValue handler will put a BSTR in the variant
    // for me
    *pbstrText = V_BSTR ( &vt );

Cleanup:
    RRETURN ( SetErrorInfo ( hr ) );
}



/*static*/
HRESULT CUnitMeasurement::CreateSubObject ( CElement *pElemObj,
    PROPERTYDESC *pPropdesc, MEASURETYPE MeasureType, IUnitMeasurement **ppObj )
{
    HRESULT hr = S_OK;

    Assert ( ppObj );

    *ppObj = (IUnitMeasurement *) new CUnitMeasurement ( pElemObj, pPropdesc, MeasureType );
    if ( !*ppObj )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    RRETURN ( hr );
}

CUnitMeasurement::~CUnitMeasurement()
{
    _pElem -> Release();
}
