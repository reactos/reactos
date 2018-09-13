/*
 * @(#)IDOMNode.cxx 1.0 3/13/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

#include "omlock.hxx"

#ifndef _XML_OM_DOCUMENT
#include "document.hxx"
#endif
#ifndef _XML_OM_IDOMNODE
#include "domnode.hxx"
#endif
#ifndef _XML_OM_NODE_HXX
#include "node.hxx"
#endif
#ifndef _DISPATCH_HXX
#include "core/com/_dispatch.hxx"
#endif

void
OMWriteLock::_init(TLSDATA * ptlsdata, Document * pDoc)
{
#ifdef RENTAL_MODEL
#if DBG == 1
    _fLeave = false;
#endif
    Assert(pDoc);
    if (pDoc->isReadOnly())
    {
        _pMutex = null;
        goto readonly;
    }
    else if (_model._reModel != Rental || pDoc->getReadyStatus() != READYSTATE_COMPLETE)
    {
#endif
        _pMutex = pDoc->getMutexNonReentrant(ptlsdata->_dwTID);
        if (_pMutex)
        {
            _pMutex->AddRef();
            _pMutex->Enter();
#ifdef RENTAL_MODEL
#if DBG == 1
            _fLeave = true;
#endif
#endif
            _fLocked = true;
        }
        else
        {
            goto readonly;
        }
#ifdef RENTAL_MODEL
    }
    else
    {
#if DBG == 1
        _pMutex = pDoc->getMutex();
        Assert(_pMutex);
        _pMutex->AddRef();
        Assert("Tried to access a Rental model document from more than one thread" && ((ShareMutex *)_pMutex)->_ptlsdata == null || ((ShareMutex *)_pMutex)->_ptlsdata == ptlsdata);
        ((ShareMutex *)_pMutex)->_ptlsdata = ptlsdata;
#else
        _pMutex = null;
#endif
        if (pDoc->getMutexNonReentrant(ptlsdata->_dwTID) == null)
            goto readonly;
        _fLocked = true;
    }
#endif
    goto cleanup;

readonly:
    _dispatchImpl::setErrorInfo(XMLOM_READONLY);
    _fLocked = false;
cleanup:
    return;
}

OMWriteLock::~OMWriteLock()
{
    if (_pMutex)
    {
#ifdef RENTAL_MODEL
#if DBG == 1
        if (_fLeave)
#endif
#endif
            _pMutex->Leave();
#ifdef RENTAL_MODEL
#if DBG == 1
        else
            ((ShareMutex *)_pMutex)->_ptlsdata = null;
#endif
#endif
        _pMutex->Release();
    }
}

OMWriteLock::OMWriteLock(TLSDATA * ptlsdata, Document * pDoc) 
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pDoc))
#endif
{
    _init(ptlsdata, pDoc);
}
OMWriteLock::OMWriteLock(TLSDATA * ptlsdata, DOMNode * pNode) 
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pNode->getNodeData()))
#endif
{
    Assert(pNode);
    _init(ptlsdata, pNode->getNodeData()->getDocument());
}
OMWriteLock::OMWriteLock(TLSDATA * ptlsdata, Node * pNode) 
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pNode))
#endif
{
    Assert(pNode);
    _init(ptlsdata, pNode->getDocument());
}

void
OMReadLock::_init(TLSDATA * ptlsdata, Document * pDoc)
{   
#ifdef RENTAL_MODEL
#if DBG == 1
    _fLeave = false;
#endif
    if (_model._reModel != Rental || pDoc->getReadyStatus() != READYSTATE_COMPLETE)
    {
#endif
        _pMutex = pDoc->getMutex();
        Assert(_pMutex);
        _pMutex->AddRef();
        _pMutex->EnterRead();
#ifdef RENTAL_MODEL
#if DBG == 1
        _fLeave = true;
#endif
#endif
#ifdef RENTAL_MODEL
    }
    else
    {
#if DBG == 1
        _pMutex = pDoc->getMutex();
        Assert(_pMutex);
        _pMutex->AddRef();
        Assert("Tried to access a Rental model document from more than one thread" && ((ShareMutex *)_pMutex)->_ptlsdata == null || ((ShareMutex *)_pMutex)->_ptlsdata == ptlsdata);
        ((ShareMutex *)_pMutex)->_ptlsdata = ptlsdata;
#else
        _pMutex = null;
#endif
    }
#endif
}
OMReadLock::~OMReadLock()
{
    if (_pMutex)
    {
#ifdef RENTAL_MODEL
#if DBG == 1
        if (_fLeave)
#endif
#endif
            _pMutex->LeaveRead();
#ifdef RENTAL_MODEL
#if DBG == 1
        else
            ((ShareMutex *)_pMutex)->_ptlsdata = null;
#endif
#endif
        _pMutex->Release();
    }
}

OMReadLock::OMReadLock(TLSDATA * ptlsdata, Document * pDoc)
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pDoc))
#endif
{
    _init(ptlsdata, pDoc);
}
OMReadLock::OMReadLock(TLSDATA * ptlsdata, DOMNode * pNode)
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pNode->getNodeData()))
#endif
{
    Assert(pNode);
    _init(ptlsdata, pNode->getNodeData()->getDocument());
}
OMReadLock::OMReadLock(TLSDATA * ptlsdata, Node * pNode)
#ifdef RENTAL_MODEL
    : _model(ptlsdata, reinterpret_cast<Object *>(pNode))
#endif
{
    Assert(pNode);
    _init(ptlsdata, pNode->getDocument());
}

