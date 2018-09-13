/*
 * @(#)TreeQuery.hxx 1.0 6/14/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XQL_QUERY_TREEQUERY
#define _XQL_QUERY_TREEQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(TreeQuery);

/**
 * A simple Query for iterating over all nodes in a subtree in document order
 *
 * Hungarian: tqy
 *
 */

// BUGBUG - consider deriving from GroupQuery

class TreeQuery : public BaseQuery
{
    DECLARE_CLASS_MEMBERS(TreeQuery, BaseQuery);
    DECLARE_CLASS_CLONING(TreeQuery, BaseQuery);

    public:
   
        static TreeQuery * newTreeQuery(Query * qyParent, Query * _qyCond, Cardinality card, const bool shouldAddRef);

        // Query Interface Methods

        virtual void reset();

        virtual void setContext(QueryContext * inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual void target(Vector * v);

        virtual DWORD getFlags();

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected:
        virtual void finalize();

        virtual Element * advance();

        virtual aint * appendPath(aint * p);

        TreeQuery();
        TreeQuery(Query * qyParent, Query * _qyCond, Cardinality card, const bool shouldAddRef);

        // cloning constructor, shouldn't do anything with data members...
        TreeQuery(CloningEnum e) 
            : super(e), 
            _eRoot(false),
            _ePending(false)
            {}

        // hide these (not implemented)


    private:
        TreeQuery( const TreeQuery &);
        void operator =( const TreeQuery &);

        void init(Query * qyCond, const bool shouldAddRef);

        ElementStack _stk;    

        /**
        * Root of the tree 
        */

        // WAA - replacing RElement
        ROElement  _eRoot;

        // WAA - replacing RElement
        ROElement  _ePending;

        /**
        * Query which tests each element for membership
            For example, given the query Z//A/B/C, then _qyCond implements the query A/B/C
        */

        RQuery _qyCond;

        /**
        * Flags
        */
        unsigned    _fIsAttribute:1; // Enumerate Attributes?
};

#endif _XQL_QUERY_TREEQUERY

