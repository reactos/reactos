/*
 * @(#)GroupQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL GroupQuery object
 * 
 */


#ifndef _XQL_QUERY_GROUPQUERY
#define _XQL_QUERY_GROUPQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(GroupQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: gqy
 *
 */
class GroupQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(GroupQuery, BaseQuery);
    DECLARE_CLASS_CLONING(GroupQuery, BaseQuery);

    public:

        static GroupQuery * newGroupQuery(Query * qyInput, Query * qy, Cardinality card, const bool addRef);

        // Query Interface Methods

        virtual Object * nextElement();

        virtual void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual DWORD getFlags();

        virtual void target(Vector * v);

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        GroupQuery(Query * qyParent, Query * qy, Cardinality card, const bool addRef);

        // cloning constructor, shouldn't do anything with data members...
        GroupQuery(CloningEnum e) : super(e) {}

        virtual void finalize();

        // Define but don't implement these
        // BUGBUG need a macro in core\base (maybe in DECLARE_CLASS_MEMBERS ?)
        // that implements these no-ops

        GroupQuery(){}
        GroupQuery( const GroupQuery &);
        void operator =( const GroupQuery &);

    private:

        void init(Query * qy) {_qy = qy;}

        virtual Element * advance();

        virtual aint * appendPath(aint * p);

        /**
         * The query contained within parentheses
         */

        RQuery _qy;
};


#endif _XQL_QUERY_GROUPQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
