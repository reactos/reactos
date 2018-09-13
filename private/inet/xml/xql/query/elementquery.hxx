/*
 * @(#)ElementQuery.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_ELEMENTQUERY
#define _XQL_QUERY_ELEMENTQUERY

#ifndef _XQL_QUERY_QUERY
#include "xql/query/query.hxx"
#endif

#ifndef _XQL_QUERY_BASEOPERAND
#include "xql/query/baseoperand.hxx"
#endif


DEFINE_CLASS(ElementQuery);


// BUGBUG - derive ElementQuery from BaseQuery 
// This will simplify the code and allow BaseQuery to implement Query methods.

class ElementQuery: public BaseOperand, public Query
{
    DECLARE_CLASS_MEMBERS_I1(ElementQuery, BaseOperand, Enumeration);
    DECLARE_CLASS_CLONING(ElementQuery, BaseOperand);

    public:

	    static ElementQuery * newElementQuery(Element * e, const bool shouldAddRef);

        virtual bool hasMoreElements();

        virtual Object * peekElement();

        virtual Object * nextElement();

        virtual void reset();

        virtual void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual void target(Vector * v);

        virtual int getIndex(QueryContext *inContext, Element * e);

        virtual bool isEnd(QueryContext *inContext, Element * e);

        virtual Operand * toOperand();

        virtual DWORD getFlags();

        virtual aint * path(aint * p);

        // Operand Interace Methods

        virtual Query * toQuery();

        virtual TriState isTrue(QueryContext *inContext, Query * qyContext, Element * eContext);

        virtual void getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval);

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        ElementQuery():_e(false){};
        // cloning constructor, shouldn't do anything with data members...
        ElementQuery(CloningEnum e) : super(e),_e(false) {};
	    
        ElementQuery(const bool shouldAddRef) : _e(shouldAddRef) {};

	    virtual void finalize();

    private:
        unsigned   _index;
        // WAA - replacing RElement
        ROElement  _e;
};

#endif _XQL_QUERY_ELEMENTQUERY

