/*
 * @(#)OrQuery.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XQL OrQuery object
 * 
 */


#ifndef _XQL_QUERY_ORQUERY
#define _XQL_QUERY_ORQUERY

#ifndef _XQL_QUERY_BASEQUERY
#include "xql/query/basequery.hxx"
#endif


DEFINE_CLASS(Element);
DEFINE_CLASS(OrQuery);

typedef _array<raint>       aaint;
typedef _reference<aaint>  raaint;


/**
 * A simple Enumeration for iterating over nodes that satisfy an XQL query.
 *
 * Hungarian: oqy
 *
 */
class OrQuery : public BaseQuery
{

    DECLARE_CLASS_MEMBERS(OrQuery, BaseQuery);
    DECLARE_CLASS_CLONING(OrQuery, BaseQuery);

    public:

        static OrQuery * newOrQuery(Query * qyParent, Cardinality card, const bool shouldAddRef=false);

        void    addQuery(Query * qry);

        // Query Interface Methods

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
        OrQuery() : _e(false){};
        OrQuery(const OrQuery &);// :  _e(false){};
        OrQuery(Query * qyParent, Cardinality card, const bool shouldAddRef=false) :
             BaseQuery(qyParent, card, shouldAddRef), 
                 _queries(REF_NOINIT), 
                 _index(REF_NOINIT), 
                 _paths(REF_NOINIT),
                _e(shouldAddRef) {};

        // cloning constructor, shouldn't do anything with data members...
        OrQuery(CloningEnum e) 
            : super(e), 
            _e(false) {};

        void finalize();

        virtual Element * advance();

        virtual aint * appendPath(aint * p);


    private:
        
        Element * fetchElements();
        void mergeNextElement(int i, Query * qry);
        bool mergePath(int i, aint * path1);

        enum {DEFAULT_NUM_QUERIES = 4};

        int     _cQueries;

        RAQuery _queries;

        raint   _index;

        raaint  _paths;

        // WAA - replacing RElement
        ROElement _e;   
        
        raint   _path;

        RQuery   _qy;

        int     _iFetch;

        int     _iNext;
};


#endif _XQL_QUERY_ORQUERY

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
