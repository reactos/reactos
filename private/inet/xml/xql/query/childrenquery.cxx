/*
 * @(#)ChildrenQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL ChildrenQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "elementquery.hxx"
#include "childrenquery.hxx"


DEFINE_CLASS_MEMBERS_CLONING(ChildrenQuery, _T("ChildrenQuery"), BaseQuery);


/*  ----------------------------------------------------------------------------
    newChildrenQuery()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new ChildrenQuery object
*/
ChildrenQuery *
ChildrenQuery::newChildrenQuery(Query * qyParent, 
                                Atom * atomURN, 
                                Atom * atomPrefix, 
                                Atom * atomName, 
                                Cardinality card, 
                                Element::NodeType type,
                                const bool shouldAddRef)
{
    return new ChildrenQuery(qyParent, atomURN, atomPrefix, atomName, type, card, shouldAddRef);
}        


/*  ----------------------------------------------------------------------------
    ChildrenQuery()

    Protected constructor

    @param strName  -   query's node name
    @param enParent -   parent enumeration

*/


ChildrenQuery::ChildrenQuery(Query * qyParent, Atom * atomURN, Atom * atomPrefix, Atom * atomName, Element::NodeType type, Cardinality card, const bool shouldAddRef)
    : BaseQuery(qyParent, card, shouldAddRef),
    _atomPrefix(REF_NOINIT), 
    _atomGI(REF_NOINIT)
{
    bool fPrefixIsURN;

    // Don't use a URN for an attribute without a prefix.  Attributes
    // don't pick up default namespaces.

    if (atomURN && (atomPrefix || type != Node::ATTRIBUTE))
    {
        fPrefixIsURN = true;
        atomPrefix = atomURN;
    }
    else
    {
        fPrefixIsURN = false;
    }

    init(fPrefixIsURN, atomPrefix, atomName, type, shouldAddRef);
}


void
ChildrenQuery::init(bool fPrefixIsURN, Atom * atomPrefix, Atom * atomGI, Element::NodeType type, const bool shouldAddRef)
{
    _atomPrefix = atomPrefix;
    _atomGI = atomGI;
    _type = type;
    _fMatchName = _atomPrefix || _atomGI;
    _fPrefixIsURN = fPrefixIsURN;
}


void 
ChildrenQuery::finalize()
{
    _atomPrefix = null;
    _atomGI = null;
    _frame.init(null, false, shouldAddRef());

    super::finalize();
}


/*  ----------------------------------------------------------------------------
    reset()
*/
void
ChildrenQuery::reset()
{
    _frame.init(null, false, shouldAddRef());
    super::reset();
}


/*  ----------------------------------------------------------------------------
    setContext(QueryContext *inContext, Element * e)
*/
void
ChildrenQuery::setContext(QueryContext *inContext, Element * e)
{
    Element * eParent;

    if (getInput())
    {
        eParent = null;
    }
    else
    {
        eParent = e;
        e = null;
    }

    _frame.init(eParent, _type == Element::ATTRIBUTE, shouldAddRef());

    super::setContext(inContext, e);

}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element * 
ChildrenQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    Element * eParent;
    if (e && e != eRoot)
    {
        if (matches(e))
        {
            Query * qy = getInput();
            eParent = e->getParent();
            if (qy)
            {
                return qy->contains(inContext, eRoot, eParent);
            }

            return eParent;
        }
    }

    return null;
}


DWORD 
ChildrenQuery::getFlags()
{
    DWORD flags = super::getFlags();

    if (_type == Element::ATTRIBUTE)
    {
        flags &= ~IS_ELEMENT;
    }

    return flags;
}


/*  ----------------------------------------------------------------------------
    target()
*/
void
ChildrenQuery::target(Vector * v)
{
     if (!v->contains(_atomGI))
     {
        v->addElement(_atomGI);
     }
}


aint *
ChildrenQuery::appendPath(aint * p)
{
    p = Path::append(p, _frame._index);
    return p;
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/
Element * 
ChildrenQuery::advance()
{
    Element * eNext;
    bool addRef = shouldAddRef();

    while(true)
    {
        if (_frame.getParent())
        {            
            eNext = getNext(&_frame);
        }
        else
        {
            _frame.init(peekInput(), _type == Element::ATTRIBUTE, addRef);
            if (_frame.getParent())
            {
                eNext = getNext(&_frame);
            }
            else
            {
                return null;
            }
        }

        if (eNext)
        {
            if (matches(eNext))
            {
                return eNext;
            }
        }
        else
        {
            advanceInput();
            _frame.init(null, false, addRef);
        }
    }
    return null;
}


bool ChildrenQuery::matches(Element * e)
{
    int type;
    NameDef * namedef;

    if (_type != Element::ANY)
    {
        type = e->getType();
        if (type != _type)
        {
            return false;
        }
    }

    if (_fMatchName)
    {
        namedef = e->getNameDef();

        // If this node doesn't have a namedef then
        // it can't possibly match.

        if (!namedef)
        {
            return false;
        }

        Name * nm = namedef->getName();

        // NOTE null name indicates a * query, which matches any node
        // This handles
        //    *
        //    namespace:book
        //    prefix:book
        //    book
        //    my:*
        //
        // but not *:book or *:*

        if (_atomGI && _atomGI != nm->getName())
        {
            return false;
        }

        if (_fPrefixIsURN)
        {
            if (_atomPrefix != nm->getNameSpace())
            {
                return false;
            }
        }
        else if (_atomPrefix != namedef->getPrefix())
        {
            return false;
        }
    }

    return true;
}


Object *
ChildrenQuery::clone()
{
    ChildrenQuery * cqy = ICAST_TO(ChildrenQuery *, super::clone());
    cqy->init(_fPrefixIsURN, _atomPrefix , _atomGI, _type, shouldAddRef());

    return (Query *) cqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
ChildrenQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();

    Query * qy = getInput();
    if (qy)
    {
        sb->append(qy->toString());
    }

    switch(_type)
    {
        case Node::ATTRIBUTE:
            sb->append(_T("@"));

            // fall through
        case Node::ELEMENT:
            if (_atomPrefix)
            {
                sb->append(_atomPrefix->toString());
                sb->append(_T(":"));
            }
            
            if (_atomGI)
                sb->append(_atomGI->toString());
            else
                sb->append(_T("*"));
            break;

        default:
            sb->append(_T("node("));
            sb->append(_type);
            sb->append(_T(")"));
    }

    return sb->toString();

}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
