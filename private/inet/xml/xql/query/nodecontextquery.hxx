/*
 * @(#)NodeContextQuery.hxx 1.0  7/22/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_NODECONTEXTQUERY
#define _XQL_QUERY_NODECONTEXTQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif

// BUGBUG - derive from ElementQuery
//  also needs to implement targetType
// 

DEFINE_CLASS(NodeContextQuery);

class NodeContextQuery: public BaseQuery
{
    DECLARE_CLASS_MEMBERS(NodeContextQuery, BaseQuery);
    DECLARE_CLASS_CLONING(NodeContextQuery, BaseQuery);

    public:
        static NodeContextQuery * newNodeContextQuery(int i, const bool addRef);

        virtual void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext * inContext, Element * eRoot, Element * e);

        virtual int getIndex(QueryContext *inContext, Element * e);

        virtual bool isEnd(QueryContext *inContext, Element * e);

        virtual DWORD getFlags();

        virtual void target(Vector * v);

        virtual void getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval);

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        NodeContextQuery(int i, const bool addRef);

        // cloning constructor, shouldn't do anything with data members...
        NodeContextQuery(CloningEnum e) : super(e) {}
	    
        virtual Element * advance();

        virtual aint * appendPath(aint * p);


    private:
        NodeContextQuery() {}
        NodeContextQuery(const NodeContextQuery &){};

        int    _levels;

        bool   _gaveOut;
};

#endif _XQL_QUERY_NODECONTEXTQUERY

