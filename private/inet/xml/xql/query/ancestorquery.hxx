/*
 * @(#)AncestorQuery.hxx 1.0  7/22/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_ANCESTORQUERY
#define _XQL_QUERY_ANCESTORQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif

DEFINE_CLASS(AncestorQuery);

class AncestorQuery: public BaseQuery
{
    DECLARE_CLASS_MEMBERS_I1(AncestorQuery, BaseQuery, Query);
    DECLARE_CLASS_CLONING(AncestorQuery, BaseQuery);

    public:
        static AncestorQuery * newAncestorQuery(Query * qyInput, Query * qyCond, const bool shouldAddRef);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual void target(Vector * v);

        virtual DWORD getFlags();

#if DBG == 1
        virtual String * toString();
#endif

        // Object Methods
        virtual Object * clone();

    protected: 
        AncestorQuery(Query * qyInput, Query * qyCond, const bool shouldAddRef);

        // cloning constructor, shouldn't do anything with data members...
        AncestorQuery(CloningEnum e) : super(e) {}
	    
        virtual Element * advance();

	    virtual void finalize();

    private:
        AncestorQuery() {}
        AncestorQuery(const AncestorQuery &){};

        /**
        * Query which tests each element for membership
            For example, given the query .ancestor("FOO"), then _qyCond implements the query FOO
        */

        RQuery _qyCond;
};

#endif _XQL_QUERY_ANCESTORQUERY

