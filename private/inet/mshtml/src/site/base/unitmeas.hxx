//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       image.hxx
//
//  Contents:   CImgElement, etc...
//
//----------------------------------------------------------------------------

#ifndef I_UNITMEAS_HXX_
#define I_UNITMEAS_HXX_
#pragma INCMSG("--- Beg 'unitmeas.hxx'")

#define _hxx_
#include "unitmeas.hdl"

class CUnitMeasurement : public CBase,
                         public IUnitMeasurement
{
public:
    enum MEASURETYPE
    {
        TOP,
        LEFT,
        WIDTH,
        HEIGHT,
        STYLETOP,
        STYLELEFT,
        STYLEWIDTH,
        STYLEHEIGHT,
		STYLEOTHERX,
		STYLEOTHERY,
        OTHERX,
        OTHERY
    };  
private:
    CElement *_pElem;
    CUnitMeasurement::MEASURETYPE _MeasureType;
    PROPERTYDESC *_pPropdesc;
    // We get the calculated value from pCalculatedRect ( in pixels )
    HRESULT GetStoredUnitValue ( CUnitValue *puvValue );
    HRESULT SetStoredUnitValue ( CUnitValue uvValue );
    CUnitValue::UNITVALUETYPE CUnitMeasurement::GetDocUnits ( void )
    {
        return (CUnitValue::UNITVALUETYPE) _pElem->Doc()->_DefaultUnit;
    }
    BOOL IsMeasureInXDirection ( void )
    {
        return (_MeasureType == OTHERX || _MeasureType == WIDTH || 
                _MeasureType == LEFT || _MeasureType == STYLEOTHERX  ) ? TRUE : FALSE;
    }
    HRESULT CloseErrorInfo ( HRESULT hr )
    {
        RRETURN( _pElem->CloseErrorInfo ( hr ) );
    }
    long GetValueFromPixelRect ( void );
    HRESULT CallHandler ( DWORD dwFlags, void *pvArg );
public:
    DECLARE_PLAIN_IUNKNOWN(CUnitMeasurement)
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    #define _CUnitMeasurement_
    #include "unitmeas.hdl"

    // IUnitMeasurement fns
    DECLARE_CLASSDESC_MEMBERS;

    static HRESULT CreateSubObject ( CElement *pElemObj, PROPERTYDESC *pPropdesc, MEASURETYPE MeasureType, IUnitMeasurement **ppObj );
    CUnitMeasurement ( CElement *pElemObj,PROPERTYDESC *pPropdesc, MEASURETYPE MeasureType );
    ~CUnitMeasurement();
    HRESULT SetPixelValue ( long lValue );
};

#pragma INCMSG("--- End 'unitmeas.hxx'")
#else
#pragma INCMSG("*** Dup 'unitmeas.hxx'")
#endif
