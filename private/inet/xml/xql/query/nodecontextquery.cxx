/*
 * @(#)NodeContextQuery.cxx 1.0  7/22/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "nodecontextquery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(NodeContextQuery, _T("NodeContextQuery"), BaseQuery);


/*  ----------------------------------------------------------------------------
    newNodeContextQuery()

    Public "static constructor"

*/
NodeContextQuery *
NodeContextQuery::newNodeContextQuery(int i, const bool addRef)
{
    return new NodeContextQuery(i, addRef);
}        


/*  ----------------------------------------------------------------------------
    NodeContextQuery()

    Protected constructor

    @param qyInput  -  parent context
    @param qyCond -   

*/
NodeContextQuery::NodeContextQuery(int i, const bool addRef)
: BaseQuery(null, BaseQuery::SCALAR, addRef),
  _levels(i)
{
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element * 
NodeContextQuery::contains(QueryContext * inContext, Element * eRoot, Element * e)
{
    if (inContext)
    {
        Query * qy = inContext->getQuery(_levels);
        if (qy != this)
        {
            return qy->contains(inContext, eRoot, e);
        }
        else if (e == inContext->getDataElement(_levels))
        {
            return e;
        }
    }

    return null;
}


/*  ----------------------------------------------------------------------------
    getIndex()
*/
int
NodeContextQuery::getIndex(QueryContext *inContext, Element * e)
{
    if (inContext)
    {
        Query * qy = inContext->getQuery(_levels);
        if (qy)
        {
            if (qy != this)
            {
                return qy->getIndex(inContext, e);
            }
            else if (e == inContext->getDataElement(_levels))
            {
                return 0;
            }
        }
    }
    return -1;
}


/*  ----------------------------------------------------------------------------
    isEnd()
*/
bool
NodeContextQuery::isEnd(QueryContext *inContext, Element * e)
{
    if (inContext)
    {
        Query * qy = inContext->getQuery(_levels);
        if (qy)
        {
            if (qy != this)
            {
                return qy->isEnd(inContext, e);
            }
            else
            {
                return e == inContext->getDataElement(_levels);
            }
        }
    }
    return true;
}


void 
NodeContextQuery::getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval)
{
    if (inContext && popval)
    {
        popval->init(getDT(), inContext->getDataElement(_levels));
    }
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
NodeContextQuery::advance()
{
    if (!_qctxt || _gaveOut == true)
    {
        return null;
    }

    Element * e = _qctxt->getDataElement(_levels);
    _gaveOut = true;

    return e;
}


/*  ----------------------------------------------------------------------------
    setContext(QueryContext *inContext, Element * e)
*/
void
NodeContextQuery::setContext(QueryContext *inContext, Element * e)
{
    _gaveOut = false;
    super::setContext(inContext, null);
}


DWORD
NodeContextQuery::getFlags()
{
    DWORD dwFlags = IS_ELEMENT | SUPPORTS_CONTAINS | NOT_FROM_ROOT;
    if (_levels == -1)
    {
        dwFlags |= STAYS_IN_SUBTREE;
    }

    return dwFlags;
}


aint *
NodeContextQuery::appendPath(aint * p)
{
    if (_qctxt)
    {
        Query * qy = _qctxt->getQuery(_levels);
        if (qy)
        {
            return qy->path(p);
        }
    }

    return p;
}


void
NodeContextQuery::target(Vector * v)
{
    // The context() doesn't know its type.
    if (!v->contains(null))
    {
        v->addElement(null);
    }
}


Object *
NodeContextQuery::clone()
{
    NodeContextQuery * nqy = ICAST_TO(NodeContextQuery *, super::clone());
    nqy->_levels = _levels;
    return (Query *) nqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
NodeContextQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    sb->append(_T("context("));
    sb->append(String::newString(_levels));
    sb->append(_T(")"));
    return sb->toString();
}
#endif


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
