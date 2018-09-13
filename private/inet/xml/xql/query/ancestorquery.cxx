/*
 * @(#)AncestorQuery.cxx 1.0  7/22/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "ancestorquery.hxx"

DEFINE_CLASS_MEMBERS_CLONING(AncestorQuery, _T("AncestorQuery"), BaseQuery);


/*  ----------------------------------------------------------------------------
    newAncestorQuery()

    Public "static constructor"

*/
AncestorQuery *
AncestorQuery::newAncestorQuery(Query * qyInput, Query * qyCond, const bool shouldAddRef)
{
    return new AncestorQuery(qyInput, qyCond, shouldAddRef);
}        


/*  ----------------------------------------------------------------------------
    AncestorQuery()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/
AncestorQuery::AncestorQuery(Query * qyInput, Query * qyCond, const bool shouldAddRef)
: BaseQuery(qyInput, BaseQuery::SCALAR, shouldAddRef),
  _qyCond(qyCond)
{
}


DWORD
AncestorQuery::getFlags()
{
    return IS_ELEMENT | NOT_FROM_ROOT | SUPPORTS_CONTAINS;
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
  
*/
Element *
AncestorQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    // Return true if this element has a child and satisfies the _qyCond.

    if (e && e != eRoot)
    {
        Element * eFirst;
        HANDLE h;

        eFirst = e->getFirstChild(&h);
        if (!eFirst)
        {
            return null;
        }

        if (_qyCond)
        {
            return _qyCond->contains(inContext, eRoot, e);
        }

        return e;
    }

    return null;
}


/*  ----------------------------------------------------------------------------
    target()
*/
void
AncestorQuery::target(Vector * v)
{
    if (_qyCond)
    {
        _qyCond->target(v);
    }
    else 
    {
        super::target(v);
    }
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
AncestorQuery::advance()
{
    Element * e = peekInput();

    if (e)
    {
        while (true)
        {
            e = e->getParent();

            if (!e)
            {
                break;
            }

            if (!_qyCond || _qyCond->contains(_qctxt, null, e))
            {
                break;
            }
        }
    }

    // OK to call advanceInput because ancestor may not be used in union.

    advanceInput();

    return e;
}


void
AncestorQuery::finalize()
{
    _qyCond = null;
    super::finalize();
} 


Object *
AncestorQuery::clone()
{
    AncestorQuery * aqy = ICAST_TO(AncestorQuery *, super::clone());
    aqy->_qyCond = _qyCond;
    return (Query *) aqy;
}

#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
AncestorQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    sb->append(_T("ancestor("));
    if (_qyCond)
    {
        sb->append(_qyCond->toString());
    }
    sb->append(_T(")"));
    return sb->toString();
}
#endif


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
