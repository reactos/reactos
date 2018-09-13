/*
 * @(#)Atom.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

DeclareTag(tagAtom, "Atom", "Atom management");

DEFINE_CLASS_MEMBERS_NEWINSTANCE(Atom, _T("Atom"), HashtableBase);

/**
 * This is a general purpose object to allow efficient
 * sharing of duplicate strings in the system.  It does this by
 * creating a global HashTable of all Atoms that have been 
 * constructed.
 *
 * @version 1.0, 6/3/97
 */
    
/**
 * Hash table for shared atoms.
 */
#ifdef STRONGREFATOM
StringHashtable* Atom::atoms;
#else
WeakRefStringHashtable* Atom::atoms;
#endif

extern ShareMutex * g_pMutexGC;
extern ApartmentMutex * g_pMutexAtom;

void 
Atom::classInit()
{
    String::classInit();

    if (!Atom::atoms)
    {
#ifdef STRONGREFATOM
        Atom::atoms = StringHashtable::newStringHashtable(500, null, false);
#else
        Atom::atoms = new WeakRefStringHashtable(500);
#endif
        Atom::atoms->AddRef();
    }
}

class TraceTagChars
{
public: TraceTagChars(String * pS) {}
public: ~TraceTagChars() {}

public: operator char * () { return _chars; }

    char _chars[80];
};


//Atom::Atom(String * s, int h) : super(NoZeroList)
Atom::Atom(String * s, int h)
{
    this->s = s;
    this->hash = h;

    TraceTag((tagAtom, "Created atom: %p h = %d s = %s", this, h, (char *)AsciiText(s)));       
}


Atom * Atom::create(String * s)
{
    return create(s, null, 0);
}

    
Atom * Atom::create(const TCHAR * c, int length)
{
    return create(null, c, length);
}


Atom * Atom::create(const TCHAR * c)
{
    return create(null, c, _tcslen(c));
}


/**
 * Create a Atom * object for this string, use character array if null
 * Atoms are case sensitive - i.e. it assumes any case folding
 * has already been done.
 */    
Atom * Atom::create(String * pS, const TCHAR * pch, int iLen)
{
    Atom * pAtom;
    int iLock = 0;
    int iGCLock = 0;
    TLSDATA * ptlsdata = GetTlsData();

#ifdef RENTAL_MODEL
    Model model(MultiThread);
#endif
    TRY
    {
        g_pMutexAtom->EnterRead(ptlsdata);
        iLock++;
        if (pS)
        {
            pAtom = (Atom *)atoms->get(pS);
        }
        else
        {
            pAtom = (Atom *)atoms->get(pch, iLen);
        }
        if (pAtom)
            pAtom->warnGC();
        g_pMutexAtom->LeaveRead();
        iLock = 0;
        if (!pAtom)
        {
            String * pStringNew;
            if (pS)      
            {
#ifdef RENTAL_MODEL
                if (pS->model() != MultiThread)
                    // BUGBUG GC danger on str
                    pStringNew = String::newString(pS->getData(), 0, pS->length());
                else
#endif
                    pStringNew = pS;
            }
            else
            {
                pStringNew = String::newString(pch, 0, iLen);
            }
            pAtom = new Atom(pStringNew, pStringNew->hashCode());
            g_pMutexAtom->Enter(ptlsdata);
            iLock = 2;
            Atom * pAtomPrev = (Atom *)atoms->add(pStringNew, pAtom);
            pAtomPrev->warnGC();
            g_pMutexAtom->Leave();
            iLock = 0;
            if (pAtomPrev != pAtom)
            {
                // an other thread already added an atom for this...
                // signal finalize to not try removing it from the table...
                pAtom->s = null;
                // simple drop the object right here...

                g_pMutexGC->ClaimExclusiveLock();
                iGCLock = 1; // remember to clean up in case of exception!

                pAtom->spinLock();
                pAtom->finalize();
                pAtom->weakRelease();
                g_pMutexGC->ReleaseExclusiveLock();
            }
#if DBG == 1
            TraceTag((tagAtom, "Return atom: %p h = %d s = %s", pAtomPrev, pAtomPrev->hash, (char *)AsciiText(pAtomPrev->s)));        
#endif
            pAtom = pAtomPrev;
        }
        else
        {
#if DBG == 1
            TraceTag((tagAtom, "Reused atom: %p h = %d s = %s", pAtom, pAtom->hash, (char *)AsciiText(pAtom->s)));        
#endif
        }
    }
    CATCH
    {
#ifdef RENTAL_MODEL
        model.Release();
#endif
        if (iLock == 2)
            g_pMutexAtom->Leave();
        else if (iLock)
            g_pMutexAtom->LeaveRead();
        if (iGCLock)
            g_pMutexGC->ReleaseExclusiveLock();

        Exception::throwAgain();
        
        // add this to avoid compiler warning (error)
        pAtom = null;
    }
    ENDTRY
    return pAtom;
}


bool Atom::equals(const TCHAR * pwc, long ulLen) 
{
    if (s->length() == (long)ulLen)
    {
        const TCHAR * pwcThis = s->getWCHARPtr();
        if ((*pwcThis == *pwc) && (StrCmpN(pwcThis, pwc, ulLen) == 0))
            return true;
    }
    return false;
}

bool Atom::hasPrefixI(const TCHAR * pwc, long ulLen) 
{
    if (s->length() >= (long)ulLen)
    {
        const TCHAR * pwcThis = s->getWCHARPtr();
        if ((*pwcThis == *pwc) && (StrCmpNI(pwcThis, pwc, ulLen) == 0))
            return true;
    }
    return false;
}


void Atom::removeFromHashtable()
{
    if (s)
    {
        if (atoms)
            atoms->remove(s);
        s = null;
    }
}


void Atom::finalize()
{
    // we should have already removed it from the hashtable
    Assert(s == null);
    super::finalize();
}
