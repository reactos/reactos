/*
 * @(#)GroupQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL GroupQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "groupquery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(GroupQuery, _T("GroupQuery"), BaseQuery);

DeclareTag(tagGroupQuery, "GroupQuery", "Trace GroupQuery actions");

#define GQY_SIGNATURE this, (char *)AsciiText(_qy ? _qy->toString() : String::nullString())

/*  ----------------------------------------------------------------------------
    newGroupQuery()

    Public "static constructor"

    @param qyParent -   parent query
    @param cond     -   query's condition
    @param op       -   query op (SCALAR, ANY, ALL)

    @return         -   Pointer to new GroupQuery object
*/
GroupQuery *
GroupQuery::newGroupQuery(Query * qyParent, Query * qy, Cardinality card, const bool addRef)
{
    return new GroupQuery(qyParent, qy, card, addRef);
}        


/*  ----------------------------------------------------------------------------
    GroupQuery()

    Protected constructor

    @param strName  -   query's node name
    @param cond     -   query's condition
    @param enParent -   parent enumeration

*/
GroupQuery::GroupQuery(Query * qyParent, Query * qy, Cardinality card, const bool addRef)
    : BaseQuery(qyParent, card, addRef)
{
    init(qy);
    Assert(_qy != null);
}


void 
GroupQuery::finalize()
{
    _qy = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    nextElement()
*/
Object *
GroupQuery::nextElement()
{
    bool fAdvance = !_fLookahead;
    Element * e = (Element *) super::nextElement();
    if (fAdvance)
    {
        _qy->nextElement();
    }
    return e;
}

/*  ----------------------------------------------------------------------------
    setContext(QueryContext *inContext, Element * e)
*/
void
GroupQuery::setContext(QueryContext *inContext, Element * e)
{
    super::setContext(inContext, e);
    Element * eNext = peekInput();
    _qy->setContext(inContext, eNext);
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element *
GroupQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    Element * eResult = null;

    if (e && e != eRoot)
    {
        eResult = _qy->contains(inContext, eRoot, e);
        if (eResult)
        {
            eResult = super::contains(inContext, eRoot, eResult);
        }
    }

    return eResult;
}


DWORD
GroupQuery::getFlags()
{
    DWORD dwFlags = super::getFlags();
    return dwFlags & _qy->getFlags();
}


aint *
GroupQuery::appendPath(aint *p)
{
    return _qy->path(p);
}


/*  ----------------------------------------------------------------------------
    target()
*/
void
GroupQuery::target(Vector * v)
{
     _qy->target(v);
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
GroupQuery::advance()
{
    Element * eNext = (Element *) _qy->peekElement();

    while (!eNext)
    {
        advanceInput();
        eNext = peekInput();
        if (!eNext)
        {
            break;
        }

        _qy->setContext(_qctxt, eNext);
        eNext = (Element *) _qy->peekElement();
    }

    return eNext;
}


Object *
GroupQuery::clone()
{
    GroupQuery * gqy = ICAST_TO(GroupQuery *, super::clone());
    gqy->init(_qy);
    return (Query *) gqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
GroupQuery::toString()
{
    return String::add(String::newString(_T("(")),
        _qy ? _qy->toString() : String::nullString(),
        String::newString(_T(")")), null);
}
#endif



/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
