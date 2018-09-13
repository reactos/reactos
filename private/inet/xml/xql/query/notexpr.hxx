/*
 * @(#)NotExpr.hxx 1.0 3/20/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL NotExpr object
 * 
 */

#ifndef _XQL_QUERY_NOTEXPR
#define _XQL_QUERY_NOTEXPR


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(NotExpr);


/**
 * An ovject that negates an operand's value.
 *
 * Hungarian: nexpr
 *
 */

class NotExpr : public BaseOperand
{
public:

    DECLARE_CLASS_MEMBERS(NotExpr, BaseOperand);


    public: 
        static NotExpr * newNotExpr(Operand * opnd);

        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);

#if DBG == 1
        virtual String * toString();
#endif

    protected:
        NotExpr(Operand * opnd);

        void finalize()
        {
            _opnd = null;
        }

    private:
        NotExpr(){}

        /**
         * this NotExpr's l-value
         */
        ROperand _opnd;


        static TriState s_aInvertTriState[];
};


#endif _XQL_QUERY_NOTEXPR

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
