/*
 * @(#)BaseOperand.hxx 1.0 4/22/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL BaseOperand object
 * 
 */

#ifndef _XQL_QUERY_BASEOPERAND
#define _XQL_QUERY_BASEOPERAND

#ifndef _XQL_QUERY_OPERAND
#include "xql/query/operand.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(BaseOperand);


/**
 * An operand of an XQL condition
 *
 * Hungarian: opnd
 *
 */

class BaseOperand : public Base, public Operand
{
public:

    DECLARE_CLASS_MEMBERS_I1(BaseOperand, Base, Operand);
    DECLARE_CLASS_CLONING(BaseOperand, Base);

public: 
    virtual bool isScalar();
    virtual DataType getDataType();
    virtual void setDataType(DataType dt);
    virtual Query * toQuery();

    virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);
    virtual void getValue(QueryContext * inContext, Query * qyContext, Element * eContext, OperandValue * opval);
    virtual TriState compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd);

    DataType    getDT() {return _dt;}

protected:
    BaseOperand() : _dt(DT_NONE) {}
    void finalize();

    // cloning constructor, shouldn't do anything with data members...
    BaseOperand(CloningEnum e) : super(e) {}

    // Object Methods
    virtual Object * clone();

private:
    DataType    _dt;
};


#endif _XQL_OPERAND

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
