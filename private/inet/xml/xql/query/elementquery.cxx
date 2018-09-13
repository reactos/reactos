/*
 * @(#)ElementQuery.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "basequery.hxx"
#include "elementquery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(ElementQuery, _T("ElementQuery"), BaseOperand);

ElementQuery * ElementQuery::newElementQuery(Element * e, const bool shouldAddRef)
{
	ElementQuery * eq = new ElementQuery(shouldAddRef);
    eq->setContext(null, e);

	return eq;
}


void
ElementQuery::finalize()
{
    _e = null;
    super::finalize();
} 


bool 
ElementQuery::hasMoreElements()
{
    return _index == 0 && _e;
}


Object *
ElementQuery::peekElement()
{
    if (_index == 0)
        return _e;
    return null;
}    


Object * 
ElementQuery::nextElement()
{
    if (_index == 0) 
    {
        _index = 1;
        return _e;
    }

    return null;
}


void
ElementQuery::reset()
{
    _index = 0;
}


void 
ElementQuery::setContext(QueryContext *inContext, Element * e)
{
    _e = e;
    _index = 0;
}


Element *
ElementQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    return e;
}


void
ElementQuery::target(Vector * v)
{
    if (!v->contains(null))
    {
        v->addElement(null);
    }
}


Operand * 
ElementQuery::toOperand()
{
    return this;
}


DWORD
ElementQuery::getFlags()
{
    return IS_ELEMENT | STAYS_IN_SUBTREE | NOT_FROM_ROOT |  SUPPORTS_CONTAINS;
}


aint *
ElementQuery::path(aint *p)
{
    return p;
}


/*  ----------------------------------------------------------------------------
    getIndex()
*/
int
ElementQuery::getIndex(QueryContext *inContext, Element * e)
{
    if (e && e != _e)
    {
        return -1;
    }
    
    return _index;
}


/*  ----------------------------------------------------------------------------
    isEnd()
*/
bool
ElementQuery::isEnd(QueryContext *inContext, Element * e)
{
    return ElementQuery::getIndex(inContext, e) != 0;
}


Query * 
ElementQuery::toQuery()
{
    return this;
}


TriState 
ElementQuery::isTrue(QueryContext *inContext, Query * qyContext, Element * eContext)
{
    return eContext ? TRI_TRUE : TRI_FALSE;
}


void
ElementQuery::getValue(QueryContext *inContext, Query * qyContext, Element * eContext, OperandValue * popval)
{
    if (eContext)
    {
        popval->init(getDT(), eContext);
    }
}


Object *
ElementQuery::clone()
{
    ElementQuery *eqy = ICAST_TO(ElementQuery *, super::clone());
    eqy->_e.setAddRef(_e.isAddRef());
    eqy->_index = _index;

    return (Query *) eqy;
}

#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
ElementQuery::toString()
{
    return String::newString(_T("ElementQuery"));
}
#endif
