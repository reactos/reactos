/*
 * @(#)Condition.hxx 1.0 3/20/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL Condition object
 * 
 */

#ifndef _XQL_QUERY_CONDITION
#define _XQL_QUERY_CONDITION


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(Condition);


/**
 * A condition to evaluate when executing a query
 *
 * Hungarian: cond
 *
 */

class Condition : public BaseOperand
{
public:

    DECLARE_CLASS_MEMBERS(Condition, BaseOperand);


    public: 
        static Condition * newCondition(OperandValue::RelOp op, Operand * opnd1, Operand * opnd2);

        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);

#if DBG == 1
        virtual String * toString();
#endif

    protected:
        Condition(OperandValue::RelOp op, Operand * opnd1, Operand * opnd2);

        void finalize()
        {
            _opnd1 = null;
            _opnd2 = null;
        }

    private:
        Condition(){}

        /**
         * this Condition's l-value
         */
        ROperand _opnd1;

        /**
         * this Condition's r-value
         */
        ROperand _opnd2;

        /**
         * this Condition's operator
         */
        OperandValue::RelOp _op;
};

#endif _XQL_QUERY_CONDITION

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
