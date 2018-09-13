/*
 * @(#)OrExpr.hxx 1.0 3/20/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL OrExpr object
 * 
 */

#ifndef _XQL_QUERY_OREXPR
#define _XQL_QUERY_OREXPR


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(OrExpr);


/**
 * An object for performing boolean and on two operands.
 *
 * Hungarian: bexpr
 *
 */

class OrExpr : public BaseOperand
{
public:

    DECLARE_CLASS_MEMBERS(OrExpr, BaseOperand);


    public: 
        static OrExpr * newOrExpr(Operand * opnd1, Operand * opnd2);
        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);

#if DBG == 1
        virtual String * toString();
#endif

    protected:
        OrExpr(Operand * opnd1, Operand * opnd2);

        void finalize()
        {
            _opnd1 = null;
            _opnd2 = null;
        }

    private:
        OrExpr(){}

        /**
         * this OrExpr's l-value
         */
        ROperand _opnd1;

        /**
         * this OrExpr's r-value
         */
        ROperand _opnd2;
};


#endif _XQL_QUERY_OREXPR

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
