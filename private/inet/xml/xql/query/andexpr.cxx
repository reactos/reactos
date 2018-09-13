/*
 * @(#)AndExpr.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL AndExpr object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "andexpr.hxx"


DEFINE_CLASS_MEMBERS(AndExpr, _T("AndExpr"), BaseOperand);


/*  ----------------------------------------------------------------------------
    newAndExpr()

    Public "static constructor"

    @param opnd1 -   l-value
    @param opnd2 -   r-value
    @param op    -   operation

    @return      -   Pointer to new AndExpr object
*/
AndExpr *
AndExpr::newAndExpr(Operand * opnd1, Operand * opnd2)
{
    return new AndExpr(opnd1, opnd2);
}        


/*  ----------------------------------------------------------------------------
    AndExpr()

    Protected constructor

    @param opnd1 -   l-value
    @param opnd2 -   r-value

*/
AndExpr::AndExpr(Operand * opnd1, Operand * opnd2):
_opnd1 (opnd1),
_opnd2 (opnd2)
{
    Assert(_opnd1 != null);
    Assert(_opnd2 != null);
}


/*  ----------------------------------------------------------------------------
    getValue()
*/
 
TriState 
AndExpr::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    Assert(_opnd1 != null);
    Assert(_opnd2 != null);

    TriState result = _opnd1->isTrue(inContext, qyContext, eContext);

    if (result != TRI_FALSE)
    {
        TriState result2 = _opnd2->isTrue(inContext, qyContext, eContext);
        if (result == TRI_TRUE)
        {
            result = result2;
        }
        else if (result2 == TRI_FALSE)
        {
            result = result2;
        }
    }

    return result;
}


#if DBG == 1
/**
 * Retrieves the string representation of this expression
 * @return a string representing the expression.
 */

String * 
AndExpr::toString()
{
    return String::add(String::newString(_T("AndExpr")),
        String::newString(_T("(")),
        String::newString(_T("_opnd1=")), _opnd1 ? _opnd1->toString() : String::nullString(),
        String::newString(_T(",")),
        String::newString(_T("_opnd2=")), _opnd2 ? _opnd2->toString() : String::nullString(),
        String::newString(_T(")")), null);
}
#endif


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
