/*
 * @(#)ChildrenQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL ChildrenQuery object
 * 
 */


#ifndef _XQL_QUERY_CHILDRENQUERY
#define _XQL_QUERY_CHILDRENQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(ChildrenQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: cqy
 *
 */
class ChildrenQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(ChildrenQuery, BaseQuery);
    DECLARE_CLASS_CLONING(ChildrenQuery, BaseQuery);

    public:

        static ChildrenQuery * newChildrenQuery(Query * qyParent, Atom * atomURN, Atom * atomPrefix, Atom * atomName, Cardinality card, Element::NodeType type, const bool shouldAddRef);

        // Query Interface Methods

        virtual void reset();

        virtual void setContext(QueryContext *inContext, Element * e);

        virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e);

        virtual void target(Vector * v);

        virtual DWORD getFlags();

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        ChildrenQuery(Query * qnParent, Atom * atomURN, Atom * atomPrefix, Atom * atomName, Element::NodeType type, Cardinality card, const bool shouldAddRef);

        // cloning constructor, shouldn't do anything with data members...
        ChildrenQuery(CloningEnum e) : super(e), _frame(false) {}

        virtual void finalize();

        virtual Element * advance();

        virtual aint * appendPath(aint * p);

         // hide these (not implemented)

        ChildrenQuery(){}

    private:

        void init(bool fPrefixIsURN, Atom * atomPrefix, Atom * atomGI, Element::NodeType type, const bool shouldAddRef);
        bool matches(Element * e);

        /**
         * Element's GI - may be null
         */

        RAtom   _atomGI;

         /**
         * Element's prefix - may be null
         */

        RAtom   _atomPrefix;

        // BUGBUG - Merge type and flags into one unsigned.

        /**
         * Element type for this query
         */
        Element::NodeType _type;

        unsigned _fMatchName:1;
        unsigned _fPrefixIsURN:1;   

        /**
         * the current context element
         */

        ElementFrame   _frame;  
};


#endif _XQL_QUERY_CHILDRENQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
