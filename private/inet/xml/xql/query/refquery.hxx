/*
 * @(#)RefQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL RefQuery object
 * 
 */


#ifndef _XQL_QUERY_REFQUERY
#define _XQL_QUERY_REFQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(RefQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: rqy
 *
 */
class RefQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(RefQuery, BaseQuery);
    DECLARE_CLASS_CLONING(RefQuery, BaseQuery);

    public:

        static RefQuery * newRefQuery(String * s, Query * qyParent, Cardinality card, const bool addRef);

        // Query Interface Methods

        virtual void setContext(QueryContext * inContext, Element * e);

        virtual Element * contains(QueryContext * inContext, Element * eRoot, Element * e);

        virtual DWORD getFlags();

        // Object Methods
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        RefQuery(String * s, Query * qyParent, Cardinality card, const bool addRef) :
             BaseQuery(qyParent, card, addRef), 
             _s(s), _d(REF_NOINIT), _v(REF_NOINIT) {}

        // cloning constructor, shouldn't do anything with data members...
        RefQuery(CloningEnum e) : super(e) {}

        void finalize();

        virtual Element * advance();

         // hide these (not implemented)

        RefQuery(){}

    private:

        RDocument    _d;

        RVector      _v;

        RHashtable   _table;

        int          _i;

        RString      _s;
};


#endif _XQL_QUERY_REFQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
