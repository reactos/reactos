/*
 * @(#)NotExpr.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL NotExpr object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "notexpr.hxx"


DEFINE_CLASS_MEMBERS(NotExpr, _T("NotExpr"), BaseOperand);

TriState NotExpr::s_aInvertTriState[] = {TRI_UNKNOWN, TRI_TRUE, TRI_FALSE};

/*  ----------------------------------------------------------------------------
    newNotExpr()

    Public "static constructor"

    @param opnd -   l-value

    @return      -   Pointer to new NotExpr object
*/
NotExpr *
NotExpr::newNotExpr(Operand * opnd)
{
    return new NotExpr(opnd);
}        


/*  ----------------------------------------------------------------------------
    NotExpr()

    Protected constructor

    @param opnd -   l-value

*/
NotExpr::NotExpr(Operand * opnd): 
_opnd (opnd)
{
    Assert(_opnd != null);
}


TriState 
NotExpr::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    Assert(_opnd != null);
    Assert(TRI_UNKNOWN == -1);
    Assert(TRI_FALSE == 0);
    Assert(TRI_TRUE == 1);

    return s_aInvertTriState[_opnd->isTrue(inContext, qyContext, eContext) + 1];
}


#if DBG == 1
/**
 * Retrieves the string representation of this expression
 * @return a string representing the expression.
 */

String * 
NotExpr::toString()
{
    return String::add(String::newString(_T("NotExpr")),
        String::newString(_T("(")),
        String::newString(_T("_opnd=")), _opnd ? _opnd->toString() : String::nullString(),
        String::newString(_T(")")), null);
}
#endif
/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
