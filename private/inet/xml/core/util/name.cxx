/*
 * @(#)Name.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#include "core.hxx"
#pragma hdrstop

DeclareTag(tagName, "Name", "Name management");

// use ABSTRACT because of no default constructor 
DEFINE_CLASS_MEMBERS_CLASS(Name, _T("Name"), HashtableBase);

/**
 * This is a general purpose name object to allow efficient
 * sharing of duplicate names in the system.  It does this by
 * creating a global HashTable of all names that have been 
 * constructed.  Names are different from Atoms in that they can
 * be qualified by a separate NameSpace string - so in effect
 * that are compound atoms.
 *
 * @version 1.0, 6/3/97
 */

/**
 * Hash table for shared qualified names.
 */
#ifdef STRONGREFATOM
StringHashtable* Name::s_pNames;
#else
WeakRefStringHashtable* Name::s_pNames;
#endif

extern ShareMutex * g_pMutexGC;
extern ApartmentMutex * g_pMutexName;

void 
Name::classInit()
{
    Atom::classInit();
    if (!Name::s_pNames)
    {
#ifdef STRONGREFATOM
        Name::s_pNames = StringHashtable::newStringHashtable(100, null, false);
#else
        Name::s_pNames = new WeakRefStringHashtable(100);
#endif
        Name::s_pNames->AddRef();
    }
}


//Name::Name(Atom * name, Atom * urn, String * key) : super(NoZeroList)
Name::Name(Atom * name, Atom * urn, String * key)
{   
    _pAtomGI = name;
    _pAtomURN = urn;
    _pStringKey = key;
#if DBG == 1
    TraceTag((tagName, "Created %p h = %d n = %s ns = %s",
        this, hashCode(), (char*)AsciiText(name), 
        urn ? (char*)AsciiText(urn) : "<>"));
#endif
}   


bool Name::equals(Object * that) 
{
    if (this == that) 
    {
        return true;
    }
    if (that == null || getClass() != that->getClass()) 
    {
        return false;
    }

    Name * t = CAST_TO(Name *, that);
    if (_pAtomURN != t->_pAtomURN)
    {
        return false;
    }

    return (this->_pAtomGI == t->_pAtomGI);        
}


/**
 * Create a Name * object for the given name.
 * The string is case sensitive.
 */    
Name * Name::create(String * s)
{
    return create(s, null, 0, null);
}

    
/**
 * Create a Name * object for the given name.
 * The string is case sensitive.
 */    
Name * Name::create(const TCHAR * c, int length)
{
    return create(null, c, length, null);
}


/**
 * Create a Name * object for the given name.
 * The string is case sensitive.
 */    
Name * Name::create(const TCHAR * c)
{
    return create(null, c, _tcslen(c), null);
}

/**
 * Create a Name * object for the given name and NameSpace.
 * The strings are case sensitive.
 */    
Name * Name::create(const TCHAR * name, int len, Atom * urn)
{
    Assert(name);
    return create(null, name, len, urn);
}


/**
 * Create a Name * object for the given name and NameSpace.
 * The strings are case sensitive.
 */    
Name * Name::create(String * name, Atom * urn)
{
    return create(name, null, 0, urn);
}


/**
 * Create an unqualified Name.
 */    
Name * Name::create(String * pS, const TCHAR * pch, int iLen, Atom * pAtomURN)
{
    int iLock = 0;
    int iGCLock = 0;
    TLSDATA * ptlsdata = GetTlsData();

    Name * pName;

#ifdef RENTAL_MODEL
    Model model(MultiThread);
#endif
#if 0
    if (pS)
        iLen = pS->length();
    int i = iLen * sizeof(TCHAR);
    BYTE * key =  reinterpret_cast<BYTE *>(_alloca(i + sizeof(Atom *)));
    *(Atom **)key = pAtomURN;
    if (pS)
        pS->copyData((TCHAR *)(key + sizeof(Atom *)), iLen);
    else
        memcpy(key + sizeof(Atom *), pch, i); 
#else
    Atom * pAtomGI = Atom::create(pS, pch, iLen);
#endif

    TRY
    {
#if 0
#else
        Atom * buf[2];

        buf[0] = pAtomGI;
        buf[1] = pAtomURN;
#endif
        g_pMutexName->EnterRead(ptlsdata);
        iLock++;
#if 0
        pName = (Name *)s_pNames->get((TCHAR *)key, iLen + sizeof(Atom *)/sizeof(TCHAR));
#else
        pName = (Name *)s_pNames->get((TCHAR *)buf, sizeof(buf)/sizeof(TCHAR));
#endif
        if (pName)
            pName->warnGC();
        g_pMutexName->LeaveRead();
        iLock = 0;
        if (!pName)
        {
#if 0
            String * pKey = String::newString((TCHAR *)key, 0, iLen + sizeof(Atom *)/sizeof(TCHAR));
            Atom * pAtomGI = Atom::create(pS, pch, iLen);
#else
            String * pKey = String::newString((TCHAR *)buf, 0, sizeof(buf)/sizeof(TCHAR));
#endif
            // add new name to simple names hash table.
            pName = new Name(pAtomGI, pAtomURN, pKey);
            g_pMutexName->Enter(ptlsdata);
            iLock = 2;
            Name * pNamePrev = (Name *)s_pNames->add(pKey, pName);
            pNamePrev->warnGC();
            g_pMutexName->Leave();
            iLock = 0;
            if (pNamePrev != pName)
            {
                // an other thread already added a name for this...
                // signal finalize to not try removing it from the table...
                pName->_pStringKey = null;

                g_pMutexGC->ClaimExclusiveLock();
                iGCLock = 1; // remember to clean up in case of exception!

                // simple drop the object right here...
                pName->spinLock();
                pName->finalize();
                pName->weakRelease();
                g_pMutexGC->ReleaseExclusiveLock();
            }
#if DBG == 1
            created++;
            TraceTag((tagName, "Return %p h = %d n = %s ns = %s",
                pNamePrev, pNamePrev->hashCode(), (char*)AsciiText(pNamePrev->_pAtomGI), 
                pNamePrev->_pAtomURN ? (char*)AsciiText(pNamePrev->_pAtomURN) : "<>"));
#endif
            pName = pNamePrev;
        }
        else
        {
#if DBG == 1
            reused++;
            // return existing name and drop 'result' on the floor.
            TraceTag((tagName, "Reused %p h = %d n = %s ns = %s",
                pName, pName->hashCode(), (char*)AsciiText(pName->_pAtomGI), 
                pName->_pAtomURN ? (char*)AsciiText(pName->_pAtomURN) : "<>"));
#endif
        }
    }
    CATCH
    {
#ifdef RENTAL_MODEL
        model.Release();
#endif
        if (iLock == 2)
            g_pMutexName->Leave();
        else if (iLock)
            g_pMutexName->LeaveRead();
        if (iGCLock)
            g_pMutexGC->ReleaseExclusiveLock();
        Exception::throwAgain();
    }
    ENDTRY
    return pName;
}


String * Name::toString()
{
// The concatenated string is useless.  99% of the time we need the GI in the 
// error messages, so we now just return the GI and avoid having to make the
// extra call to getName() everywhere.

//    if (_pAtomURN != null)
//    {
//        return String::add(_pAtomURN->toString(), _pAtomGI->toString(), null);
//    }
//    else 
//    {
        return _pAtomGI->toString();
//    }
}


int Name::hashCode()
{
    int result = _pAtomGI->hashCode();
    if (_pAtomURN != null) 
        result += _pAtomURN->hashCode();
    return result;
}


void Name::removeFromHashtable()
{
    if (_pStringKey)
    {
        if (s_pNames)
            s_pNames->remove(_pStringKey);
        _pStringKey = null;
    }
}


void Name::finalize()
{
    // we should have already removed it from the hashtable
    Assert(_pStringKey == null);
    _pAtomURN = null;
    _pAtomGI = null;
    super::finalize();
}

#if DBG == 1
long Name::created = 0;
long Name::reused = 0;
#endif
