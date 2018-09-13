/*
 * @(#)Condition.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL Condition object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "query.hxx"
#include "condition.hxx"


DEFINE_CLASS_MEMBERS(Condition, _T("Condition"), BaseOperand);


/*  ----------------------------------------------------------------------------
    newCondition()

    Public "static constructor"

    @param opnd1 -   l-value
    @param opnd2 -   r-value
    @param op    -   operation

    @return      -   Pointer to new Condition object
*/
Condition *
Condition::newCondition(OperandValue::RelOp op, Operand * opnd1, Operand * opnd2)
{
    return new Condition(op, opnd1, opnd2);
}        


/*  ----------------------------------------------------------------------------
    Condition()

    Protected constructor

    @param opnd1 -   l-value
    @param opnd2 -   r-value
    @param op    -   operation

*/
Condition::Condition(OperandValue::RelOp op, Operand * opnd1, Operand * opnd2):
_opnd1 (opnd1),
_opnd2 (opnd2),
_op (op)
{
    Assert(OperandValue::isValidRelop(_op));
    Assert(_opnd1 != null);
    Assert(_opnd2 != null);
}


TriState 
Condition::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    Assert(_opnd1 != null);
    Assert(_opnd2 != null);

    TriState triResult = _opnd1->compare(inContext, _op, qyContext, eContext, _opnd2);
    return triResult;
}


#if DBG == 1
/**
 * Retrieves the string representation of this expression
 * @return a string representing the expression.
 */

String * 
Condition::toString()
{
    return String::add(String::newString(_T("Condition")),
        String::newString(_T("(")),
        (Integer::newInteger(_op))->toString(),
        String::newString(_T(",")),
        String::newString(_T("_opnd1=")), _opnd1 ? _opnd1->toString() : String::nullString(),
        String::newString(_T(",")),
        String::newString(_T("_opnd2=")), _opnd2 ? _opnd2->toString() : String::nullString(),
        String::newString(_T(")")), null);
}

#endif
/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

