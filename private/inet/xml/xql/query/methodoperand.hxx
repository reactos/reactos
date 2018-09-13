/*
 * @(#)MethodOperand.hxx 1.0 6/26/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL MethodOperand object
 * 
 */

#ifndef _XQL_QUERY_METHODOPERAND
#define _XQL_QUERY_METHODOPERAND


#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif

#ifndef _XQL_QUERY_QUERY
#include "xql/query/query.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(MethodOperand);


/**
 * An object that calls a method on a node.
 *
 * Hungarian: mopnd
 *
 */
// BUGBUG should only derive from Operand
class MethodOperand : public BaseOperand
{
public:

    // method types 
    enum MethodType
    {
        INVALID = -1,

        TEXT = 0,
        VALUE,
        NODETYPE,
        NODENAME,
        INDEX,
        COUNT,
        END,
        LAST = END
    };

    DECLARE_CLASS_MEMBERS(MethodOperand, BaseOperand);

    public: 
        static MethodOperand * newMethodOperand(Query * qy, MethodType op);
        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);
        virtual void getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval);

#if DBG == 1
        String * toString();
#endif


    protected:
        MethodOperand(Query * qy, MethodType op);

        void finalize();

        // Define but don't implement these
        // BUGBUG need a macro in core\base (maybe in DECLARE_CLASS_MEMBERS ?)
        // that implements these no-ops

        MethodOperand(){}
        MethodOperand( const MethodOperand &);
        void operator =( const MethodOperand &);


    private:

        /**
         * this MethodOperand's type
         */
        MethodType _mt;

        /**
         * this MethodOperand's object
         */

        RQuery     _qy;
};


#endif _XQL_QUERY_METHODOPERAND

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
