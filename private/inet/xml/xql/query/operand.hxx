/*
 * @(#)Operand.hxx 1.0 4/22/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL Operand Interface
 * 
 */

#ifndef _XQL_QUERY_OPERAND
#define _XQL_QUERY_OPERAND

#ifndef _CORE_DATATYPE_HXX
#include "core/util/datatype.hxx"
#endif

#ifndef _XQL_QUERY_CONTEXT
#include "xql/query/querycontext.hxx"
#endif

#ifndef _XQL_QUERY_OPERANDVALUE
#include "xql/query/operandvalue.hxx"
#endif

DEFINE_CLASS(Query);
DEFINE_CLASS(Operand);


/**
 * An operand of an XQL condition
 *
 * Hungarian: opnd
 *
 */

class NOVTABLE Operand : public Object
{
public:

    virtual bool isScalar() = 0;    
    virtual DataType getDataType() = 0; 
    virtual void setDataType(DataType dt) = 0; 
    virtual Query * toQuery() = 0;

    //
    // BUGBUG - pass in comparison flags so comparison can be locale-specific or case insensitive
    //

    virtual TriState isTrue(QueryContext * inContext, Query * qyContext, Element * eContext) = 0;
    virtual void getValue(QueryContext * inContext, Query * qyContext, Element * eContext, OperandValue * opval) = 0;
    virtual TriState compare(QueryContext * inContext, OperandValue::RelOp op, Query * qyContext, Element * eContext, Operand * opnd) = 0;
};



#endif _XQL_QUERY_OPERAND

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
