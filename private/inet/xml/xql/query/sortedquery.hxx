/*
 * @(#)SortedQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL SortedQuery object
 * 
 */


#ifndef _XQL_QUERY_SORTEDQUERY
#define _XQL_QUERY_SORTEDQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(FilterQuery);


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: sqy
 *
 */
class SortedQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(SortedQuery, BaseQuery);
    DECLARE_CLASS_CLONING(SortedQuery, BaseQuery);

    public:

        static SortedQuery * newSortedQuery(Query * qyParent, Cardinality card, const bool addRef);

        // Query Interface Methods

        virtual void setContext(QueryContext * inContext, Element * e);

        // Creates a shallow copy of the query
        virtual Object * clone();

#if DBG == 1
        virtual String * toString();
#endif
        void    addKey(Query * qyKey, bool fDescending, DataType dt);

    protected: 
        SortedQuery(Query * qyParent, Cardinality card, const bool addRef);

        // cloning constructor, shouldn't do anything with data members...
        SortedQuery(CloningEnum e) : super(e) {}

        virtual void finalize();

        // Define but don't implement these
        // BUGBUG need a macro in core\base (maybe in DECLARE_CLASS_MEMBERS ?)
        // that implements these no-ops

        SortedQuery(){}
        SortedQuery( const SortedQuery &);
        void operator =( const SortedQuery &);

    private:

        void sortElements();
        static int __cdecl compare (void * context, const void *elem1, const void *elem2 );
        void clearKeyValues();


        enum 
        {
            DEFAULT_RESULT_SIZE = 32,
            DEFAULT_NUM_KEYS = 2
        };

        enum
        {
            GETTING_INPUT = 0,
            GETTING_KEYS = 1,
            SORTED = 2
        };

        struct KeyInfo
        {
            RQuery      _qy;
            DataType    _dt;
            bool        _fDescending;
        };

        typedef _array<KeyInfo>      AKeyInfo;
        typedef _reference<AKeyInfo> RAKeyInfo;


        virtual Element * advance();

        /**
         * Cached results from input query
         */

        RAElement _elements;

        /**
         * Sorted element index
         */

        rauint    _aindex;  

        /**
         * Key information for sorting the query results
         */

        RAKeyInfo _ki;

        /**
         * Number of elements in the result
         */

        unsigned    _cElements;

        /**
         * Number of keyinfo's
         */

        unsigned    _cki:8;

        /**
         * Number of keys found for this element
         */

        unsigned    _j: 8;

        /**
         * Have the input elements been fetched and sorted?
         */

        unsigned    _state:2;


        /**
         * Number of elements for which keys have been fetched
         */

        unsigned    _i;

        /**
         * Number of keys have been fetched
         */

        unsigned    _k;

        /**
         * Cached key elements
         */

        RAOperandValue _keys;
};


#endif _XQL_QUERY_SORTEDQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
