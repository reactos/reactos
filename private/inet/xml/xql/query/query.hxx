/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _XQL_QUERY_QUERY
#define _XQL_QUERY_QUERY

#ifndef _CORE_UTIL_ENUMERATION
#include "core/util/enumeration.hxx"
#endif

DEFINE_CLASS(Element);
DEFINE_CLASS(Operand);
DEFINE_CLASS(Query);
DEFINE_CLASS(QueryContext);

/**
 */

class NOVTABLE Query : public Enumeration
{
public:

    enum Flags
    {
        IS_ELEMENT = 1,             // Query only returns elements not attributes
        STAYS_IN_SUBTREE = 2,       // Query result is in the context's subtree (/, id(foo), .. are queries which are not in the subtree)
        SUPPORTS_CONTAINS = 4,      // Query supports the contains method and may be use as a match pattern
        NOT_FROM_ROOT = 8,          // Query context is not the root, i.e. this isn't /x or //x
        WRAPS_INPUT = 16,           // Query wraps its input, i.e. x/y/id(z)  is exactly id(x/y/z) so it wraps its input
        IS_ABSOLUTE = 128           // Never returned by a query but used by the xqlparser
    };

    /**
     */
    virtual void setContext(QueryContext *inContext, Element * e) = 0;

    /**
     */
    virtual Element * contains(QueryContext *inContext, Element * eRoot, Element * e) = 0; // returns the eContext or null

    /**
     */
    virtual void target(Vector * v) = 0;

    virtual int getIndex(QueryContext *inContext, Element * e) = 0;

    virtual bool isEnd(QueryContext *inContext, Element * e) = 0;

    virtual Operand * toOperand() = 0;

    virtual DWORD getFlags() = 0;       // Returns one of more of the above Flags or'd together

    virtual aint * path(aint * p) = 0;  // Return the path to the node
};



#endif _XQL_QUERY_QUERY