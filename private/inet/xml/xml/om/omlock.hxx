/*
 * @(#)OMLock.hxx 
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _XML_OM_OMLOCK
#define _XML_OM_OMLOCK

class Mutex;

class OMWriteLock
{
public: 
    OMWriteLock(TLSDATA *, Document * pDoc);
    OMWriteLock(TLSDATA *, DOMNode * pElem);
    OMWriteLock(TLSDATA *, Node * pElem);
    ~OMWriteLock();
    bool Locked() const { return _fLocked; }
protected:
    void _init(TLSDATA * ptlsdata, Document * pDoc);
    Mutex * _pMutex;
    bool _fLocked;
#ifdef RENTAL_MODEL
    Model _model;
#if DBG == 1
    bool _fLeave;
#endif
#endif
};

class OMReadLock
{
public:
    OMReadLock(TLSDATA *, Document * pDoc);
    OMReadLock(TLSDATA *, DOMNode * pElem);
    OMReadLock(TLSDATA *, Node * pElem);
    ~OMReadLock();
protected:
    void _init(TLSDATA * ptlsdata, Document * pDoc);
    Mutex * _pMutex;
#ifdef RENTAL_MODEL
    Model _model;
#if DBG == 1
    bool _fLeave;
#endif
#endif
};

#define OMWRITELOCK(x) OMWriteLock lock(_EnsureTls.getTlsData(), x); if (!lock.Locked()) return E_FAIL;
#define OMREADLOCK(x) OMReadLock lock(_EnsureTls.getTlsData(), x);


#endif // _XML_OM_OMLOCK