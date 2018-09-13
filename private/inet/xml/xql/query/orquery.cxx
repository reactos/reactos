/*
 * @(#)OrQuery.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XQL OrQuery object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "orquery.hxx"


DEFINE_CLASS_MEMBERS_CLONING(OrQuery, _T("OrQuery"), BaseQuery);

#ifdef UNIX
#ifdef TraceTag
#undef TraceTag
#define TraceTag(x)
#endif // TraceTag
#endif UNIX

/*  ----------------------------------------------------------------------------
    newRefQuery()

    Public "static constructor"

    @param strName  -   query's node name
    @param enParent -   parent enumeration

    @return         -   Pointer to new OrQuery object
*/
OrQuery *
OrQuery::newOrQuery(Query * qyParent, Cardinality card, const bool shouldAddRef)
{
    OrQuery * oqry = new OrQuery(qyParent, card, shouldAddRef);
    oqry->_queries = new(DEFAULT_NUM_QUERIES) AQuery;

    return oqry;
}        


void
OrQuery::finalize()
{
    _queries = null;
    _index = null;
    _paths = null;
    _e = null;
    _path = null;
    _qy = null;
}


void
OrQuery::setContext(QueryContext *inContext, Element * e)
{
    if (!_index)
    {
        _index = new (_cQueries) aint;
    }
    else
    {
        memset(&(*_index)[0], 0, _cQueries * sizeof(int));
    }
    
    if (!_paths)
    {
        _paths = new (_cQueries) aaint;
    }
    else
    {
        for (int i = 0; i < _cQueries; i++)
        {
            Path::clear((*_paths)[i]);
        }

        _path = null;
    }

    super::setContext(inContext, e); 
    _iFetch = 0;
    _iNext = 0;
    _e = null;
    _qy = null;
}


aint *
OrQuery::appendPath(aint * p)
{
    return Path::append(p, _path);
}


DWORD
OrQuery::getFlags()
{
    DWORD dwFlags = IS_ELEMENT | STAYS_IN_SUBTREE | NOT_FROM_ROOT | SUPPORTS_CONTAINS;

    for (int i = 0; i < _cQueries; i++)
    {
        dwFlags &= (*_queries)[i]->getFlags();
    }

    return dwFlags;
}


void
OrQuery::target(Vector * v)
{
    for (int i = 0; i < _cQueries; i++)
    {
        (*_queries)[i]->target(v);
    }
}


/*  ----------------------------------------------------------------------------
    contains(Element * e)
*/
Element * 
OrQuery::contains(QueryContext *inContext, Element * eRoot, Element * e)
{
    for (int i = 0; i < _cQueries; i++)
    {
        Element * eResult = (*_queries)[i]->contains(inContext, eRoot, e);
        if (eResult)
        {
            return eResult;
        }
    }

    return null;
}


void    
OrQuery::addQuery(Query * qry)
{
    int len = _queries->length();
    if (_cQueries >= len)
    {
        _queries = _queries->resize(2 * len);
    }

    (*_queries)[_cQueries] = qry;

    _cQueries++;
}


/*  ----------------------------------------------------------------------------
    advance()

    Advances this query's enumeration to the next matching element

    @return         -   No return value
*/

#if DBG == 1
static int count = 0;
#endif

Element * 
OrQuery::advance()
{
    Element * e;
    int i, iLast, *pi;
    
    // If the index is null then the context hasn't been set.
    if (!_index)
    {
        return null;
    }

    pi = &(*_index)[0]; 

#if DBG == 1
    {
        int * pj = pi;
        while (*pj)
        {
            Query * qy = (*_queries)[*pj - 1];
            Element * e = (Element *) qy->peekElement();
            Assert("OrQuery - e must be not be null" && e != null);
            pj++;
        }
    }
#endif

    if (_e)
    {
        goto Cleanup;
    }
  
    if (!*pi || _iFetch < _cQueries)
    {
        if (_iFetch == _cQueries)
        {
            advanceInput();
            _iFetch = 0;
        }

        if (!fetchElements())
        {
            return null;
        }
    }
    

    // The async case could throw an E_PENDING when getting the nextElement
    // so the element has to be saved in _e.

    _iNext = *pi - 1;
    _qy = (*_queries)[_iNext];
    _e = (Element *) _qy->nextElement();
    Path::clear(_path);
    _path = Path::append(_path, (*_paths)[_iNext]);

    // Compact the element index

    // BUGBUG - consider using a linked list so index doesn't have
    // to be compacted.

    iLast = _cQueries - 1;
    memmove(pi, pi + 1, iLast * sizeof(int));
    pi[iLast] = 0;

    // Fetch the next element and merge it with the others

Cleanup:
    TraceTag((0, "Count = %d", count++));

    mergeNextElement(_iNext, _qy);
    e = _e;
    _e = null;

    TraceTag((0, "O Query %p eNext = %p", this, e));

#if DBG == 1
    {
        int * pj = pi;
        while (*pj)
        {
            Query * qy = (*_queries)[*pj - 1];
            Element * e = (Element *) qy->peekElement();
            Assert("OrQuery - e must be not be null" && e != null);
            pj++;
        }
    }
#endif

    return e;
}

Element *
OrQuery::fetchElements()
{
    Element * e;
    Query * qry;
    int *pi = &(*_index)[0];  

    while (true)
    {
        e = peekInput();

        if (!e)
        {
            break;
        }

        for (; _iFetch < _cQueries; _iFetch++)
        {
            qry = (*_queries)[_iFetch];
            qry->setContext(_qctxt, e);
            mergeNextElement(_iFetch, qry);
        }

        if (*pi)
        {
            break;
        }

        advanceInput();

        _iFetch = 0;
    }

    return e;
}


void
OrQuery::mergeNextElement(int i, Query * qry)
{
    Element * eNext;
    aint * pathResult;
    aint * path;

    while (true)
    {
        eNext = (Element *) qry->peekElement();
        if (!eNext)
        {
            break;
        }

        path = (*_paths)[i];

        // Set the path length to 0 before
        // filling int the path.

        Path::clear(path);

        pathResult = qry->path(path);
        
    
        // Don't assign the result back unless its
        // different to avoid the extra addref's.

        if (pathResult != path)
        {
            (*_paths)[i] = pathResult;
        }

        // Insert this path into index

        if (mergePath(i, pathResult))
        {
            TraceTag((0, "O Query %p Merge = %p", this, eNext));

            break;
        }

        qry->nextElement();
    }
}


bool
OrQuery::mergePath(int i, aint * path1)
{
    aint * path2;
    int result;
    int * pi0 = &(*_index)[0];
    int * pi = pi0;

    while (*pi)
    {
        path2 = (*_paths)[*pi - 1];
        result = Path::compare(path1, path2);
        if (result < 0)
        {
            memmove(pi + 1, pi, (size_t)(_cQueries  - (pi - pi0) - 1) * sizeof(int));
            break;
        }
        else if (result == 0)
        {
            // duplicate found

            return false;
        }
        pi++;
    }

    *pi = i + 1;
    return true;
}


Object *
OrQuery::clone()
{
    OrQuery * oqy = ICAST_TO(OrQuery *, super::clone());
    oqy->_cQueries = _cQueries;
    AQuery * queries = new (_cQueries) AQuery;
    for (int i = 0; i < _cQueries; i++)
    {
        (*queries)[i] = SAFE_CAST(Query *, (*_queries)[i]->clone());
    }
    oqy->_queries = queries;
    oqy->_e.setAddRef(shouldAddRef());

    return (Query *) oqy;
}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
OrQuery::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    Query * qy = getInput();
    if (qy)
    {
        sb->append(qy->toString());
    }

    sb->append(_T("/"));
    int i = 0;
    while(true)
    {
        sb->append((*_queries)[i]->toString());
        i++;
        if (i < _cQueries)
            sb->append(_T("|"));
        else
            break;
    }

    return sb->toString();

}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
