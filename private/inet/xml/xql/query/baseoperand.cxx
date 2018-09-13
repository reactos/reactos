/*
 * @(#)BaseOperand.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL BaseOperand object
 * 
 */


#include "core.hxx"
#pragma hdrstop

#include "baseoperand.hxx"

DEFINE_CLASS_MEMBERS_CLONING(BaseOperand, _T("BaseOperand"), Base);

void
BaseOperand::finalize()
{
    super::finalize();
}

bool 
BaseOperand::isScalar()
{
    return true;
}


DataType 
BaseOperand::getDataType()
{
    return getDT();
}


void 
BaseOperand::setDataType(DataType dt)
{
    _dt = dt;
}

Query * 
BaseOperand::toQuery()
{
    return null;
}


TriState 
BaseOperand::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
   return TRI_FALSE;
}


void 
BaseOperand::getValue(QueryContext * inContext, Query * qyContext, Element * eContext, OperandValue * opval)
{
}


/*  ----------------------------------------------------------------------------
    compare()
*/
TriState
BaseOperand::compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd)
{
    TriState triResult; 
    OperandValue opval1, opval2;

    Assert(opnd->isScalar());

    getValue(inContext, qyContext, eContext, &opval1);

    if (!opval1.isEmpty())
    {
        opnd->getValue(inContext, qyContext, eContext, &opval2);
        triResult = opval1.compare(op, &opval2);
    }
    else
    {
        triResult = TRI_UNKNOWN;
    }

    return triResult;
}


Object *
BaseOperand::clone()
{
    return super::clone();
}




