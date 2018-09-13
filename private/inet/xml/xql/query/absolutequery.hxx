/*
 * @(#)AbsoluteQuery.hxx 1.0 7/16/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_ABSOLUTEQUERY
#define _XQL_QUERY_ABSOLUTEQUERY

#ifndef _XQL_QUERY_ELEMENTQUERY
#include "xql/query/elementquery.hxx"
#endif

DEFINE_CLASS(AbsoluteQuery);

class AbsoluteQuery: public ElementQuery
{
    DECLARE_CLASS_MEMBERS_I1(AbsoluteQuery, ElementQuery, Query);
    DECLARE_CLASS_CLONING(AbsoluteQuery, ElementQuery);

    public:
        static AbsoluteQuery * newAbsoluteQuery(const bool addRef);

        void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual DWORD getFlags();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        AbsoluteQuery(const bool addRef);
        // cloning constructor, shouldn't do anything with data members...
        AbsoluteQuery(CloningEnum e) : super(e) {}
	    
    private:
        AbsoluteQuery(){};

};

#endif _XQL_QUERY_ABSOLUTEQUERY

