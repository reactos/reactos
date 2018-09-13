/*
 * @(#)AndExpr.hxx 1.0 3/20/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL AndExpr object
 * 
 */

#ifndef _XQL_QUERY_ANDEXPR
#define _XQL_QUERY_ANDEXPR


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(AndExpr);


/**
 * An object for performing boolean and on two operands.
 *
 * Hungarian: bexpr
 *
 */

class AndExpr : public BaseOperand
{
public:

    DECLARE_CLASS_MEMBERS(AndExpr, BaseOperand);

    public: 
        static AndExpr * newAndExpr(Operand * opnd1, Operand * opnd2);
        virtual TriState isTrue(QueryContext * inContext, Query * qyContext, Element * eContext);

#if DBG == 1
        virtual String * toString();
#endif

    protected:
        AndExpr(Operand * opnd1, Operand * opnd2);

        void finalize()
        {
            _opnd1 = null;
            _opnd2 = null;
        }

    private:
        AndExpr() {};

        /**
         * this AndExpr's l-value
         */
        ROperand _opnd1;

        /**
         * this AndExpr's r-value
         */
        ROperand _opnd2;
};


#endif _XQL_QUERY_ANDEXPR

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
