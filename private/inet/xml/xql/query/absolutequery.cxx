/*
 * @(#)AbsoluteQuery.cxx 1.0 7/16/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "absolutequery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(AbsoluteQuery, _T("AbsoluteQuery"), ElementQuery);


AbsoluteQuery *
AbsoluteQuery::newAbsoluteQuery(const bool addRef)
{
    AbsoluteQuery *aQuery = new AbsoluteQuery(addRef);

    return aQuery;
}

AbsoluteQuery::AbsoluteQuery(const bool addRef)
: ElementQuery(addRef)
{
}

void 
AbsoluteQuery::setContext(QueryContext *inContext, Element * e)
{
    super::setContext(inContext, e ? e->getDocument()->getDocElem() : null);
}


Element *
AbsoluteQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    if (e == e->getDocument()->getDocElem())
    {
        return e;
    }

    return null;
}




DWORD
AbsoluteQuery::getFlags()
{
    return IS_ELEMENT | SUPPORTS_CONTAINS;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
AbsoluteQuery::toString()
{
    return String::newString(_T("/"));
}
#endif
