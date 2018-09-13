/*
 * @(#)ConstantOperand.cxx 1.0 4/22/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL string operand object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "constantoperand.hxx"
#include "core/util/datatype.hxx"


DEFINE_CLASS_MEMBERS(ConstantOperand, _T("ConstantOperand"), BaseOperand);


ConstantOperand * 
ConstantOperand::newString(String * str)
{
    ConstantOperand * copnd = new ConstantOperand();
    copnd->_opval.initString(str);
    copnd->_s = str;
    return copnd;
}


ConstantOperand * 
ConstantOperand::newR8(String * s, double r8)
{
    ConstantOperand * copnd = newString(s);
    copnd->_opval.initR8(r8);
    return copnd;
}


ConstantOperand * 
ConstantOperand::newDate(String * s, DATE d)
{
    ConstantOperand * copnd = newString(s);
    copnd->_opval.initDATE(d);
    return copnd;
}


void
ConstantOperand::setDataType(DataType dt)
{
    Assert(dt == DT_NONE || dt == DT_R8 || dt == DT_DATETIME_ISO8601TZ);
    if (dt != DT_NONE && dt != getDT())
    {
        VARIANT var;

        V_VT(&var) = VT_EMPTY;

        if (SUCCEEDED(ParseDatatype(_s->getWCHARPtr(), 
                _s->length(),
                dt, 
                &var, 
                null)))
        {
            switch (dt)
            {
            case DT_R8:
                _opval.initR8(V_R8(&var));
                break;

            case DT_DATETIME_ISO8601TZ:            
                _opval.initDATE(V_DATE(&var));
                break;
            }
            super::setDataType(dt);
        }
    }
}


void 
ConstantOperand::getValue(QueryContext * inContext, Query * qyContext, Element * eContext, OperandValue * popval)
{
    *popval = _opval;
}


TriState 
ConstantOperand::compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd)
{
    OperandValue opval;
    TriState triResult; 

    opnd->getValue(inContext, qyContext, eContext, &opval);
    triResult = _opval.compare(op, &opval);
    return triResult;
}


ConstantOperand::ConstantOperand(OperandValue * popval)
{
    _opval = *popval;
}


void
ConstantOperand::finalize()
{
    _s = null;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
ConstantOperand::toString()
{
    return _s;
}
#endif



/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
