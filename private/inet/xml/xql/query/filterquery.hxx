/*
 * @(#)FilterQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL FilterQuery object
 * 
 */

 
#ifndef _XQL_QUERY_FILTERQUERY
#define _XQL_QUERY_FILTERQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(FilterQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: fqy
 *
 */
class FilterQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(FilterQuery, BaseQuery);
    DECLARE_CLASS_CLONING(FilterQuery, BaseQuery);

    public:

        static FilterQuery * newFilterQuery(Query * qyParent, Operand * opnd, Cardinality card, const bool addRef);

        // Query Interface Methods

        virtual Object * nextElement();

        virtual Element * contains(QueryContext * inContext, Element * eRoot, Element * e);

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        FilterQuery(Query * qyParent, Operand * opnd, Cardinality card, const bool addRef);

        // cloning constructor, shouldn't do anything with data members...
        FilterQuery(CloningEnum e) : super(e) {}

        virtual void finalize();

        virtual Element * peekInput();

         // Define but don't implement these
        // BUGBUG need a macro in core\base (maybe in DECLARE_CLASS_MEMBERS ?)
        // that implements these no-ops

        FilterQuery(){}
        FilterQuery( const FilterQuery &);
        void operator =( const FilterQuery &);

    private:

        void init(Operand * opnd) {_opnd = opnd;}
        bool matches(QueryContext * inContext, Query * qy, Element * e);

        virtual Element * advance();

        /**
         * condition to evaluate to determine query output set membership
         */
        ROperand _opnd;
};


#endif _XQL_QUERY_FILTERQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
