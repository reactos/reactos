//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       recalc.cxx
//
//  Contents:   CBase recalc support
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RECALC_H_
#define X_RECALC_H_
#include <recalc.h>
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

MtDefine(CRecalcInfo, ObjectModel, "CRecalcInfo")

//---------------------------------------------------------------
//
//  Member:     CBase::removeExpression
//
//---------------------------------------------------------------
STDMETHODIMP
CBase::removeExpression(BSTR strPropertyName, VARIANT_BOOL *pfSuccess)
{
    AssertSz(0, "CBase recalc methods should never be called");
    return E_FAIL;
}

//---------------------------------------------------------------
//
//  Member:     CBase::setExpression
//
//---------------------------------------------------------------
STDMETHODIMP
CBase::setExpression(BSTR strPropertyName, BSTR strExpression, BSTR strLanguage)
{
    AssertSz(0, "CBase recalc methods should never be called");
    return E_FAIL;
}

//---------------------------------------------------------------
//
//  Member:     CBase::getExpression
//
// REVIEW michaelw : need to add a language parameter to allow
// REVIEW michaelw : the caller to get the expression language
//
//---------------------------------------------------------------
STDMETHODIMP
CBase::getExpression(BSTR strPropertyName, VARIANT *pvExpression)
{
    AssertSz(0, "CBase recalc methods should never be called");
    return E_FAIL;
}
