/*
 * @(#)ConstantOperand.hxx 1.0 4/22/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL ConstantOperand object
 * 
 */

#ifndef _XQL_QUERY_CONSTANTOPERAND
#define _XQL_QUERY_CONSTANTOPERAND


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(ConstantOperand);


/**
 * A element operand of an XQL condition
 *
 * Hungarian: copnd
 *
 */

class ConstantOperand : public BaseOperand
{
    public: 

        DECLARE_CLASS_MEMBERS(ConstantOperand, BaseOperand);

        static ConstantOperand * newString(String * str);
        static ConstantOperand * newI4(String * str, int i);
        static ConstantOperand * newR8(String * str, double r8);
        static ConstantOperand * newDate(String * str, DATE d);

        virtual void setDataType(DataType dt);
        virtual void getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * opval);
        virtual TriState compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd);

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        virtual void finalize();

        ConstantOperand(){}
        ConstantOperand(OperandValue * popval);
        ConstantOperand( const ConstantOperand &);
        void operator =( const ConstantOperand &);

    private:

        OperandValue    _opval;
        RString         _s;
};



#endif _XQL_QUERY_CONSTANTOPERAND

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
