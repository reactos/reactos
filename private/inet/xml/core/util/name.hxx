/*
 * @(#)Name.hxx 1.0 6/23/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_NAME
#define _CORE_UTIL_NAME

#ifndef _CORE_UTIL_ATOM
#include "core/util/atom.hxx"
#endif
#ifndef _CORE_UTIL_STRINGHASHTABLE
#include "core/util/stringhashtable.hxx"
#endif

DEFINE_CLASS(Name);
DEFINE_CLASS(NameSpace);


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
class Name: public HashtableBase
{  
    friend class NameDef;
    friend class Base;

    DECLARE_CLASS_MEMBERS(Name, HashtableBase);
    DECLARE_CLASS_INSTANCE(Name, HashtableBase);

    // override these methods because this object doesn't
    // get added to the zero count list
    public: ULONG STDMETHODCALLTYPE AddRef() { return _qAddRef(); } 
    public: ULONG STDMETHODCALLTYPE Release() { return _qRelease(); }
            
    /**
     * The constructor is private because all names should
     * be constructed using the static Name.create() methods
     */
private:

    Name(Atom * name, Atom * urn, String * key);

public: 
    
    static void classInit();

    /**
     * Create an unqualified Name.
     */    
    static Name * create(String * name);
    static Name * create(const TCHAR * c, int length);
    static Name * create(const TCHAR * c);
    static Name * create(const TCHAR * name, int length, Atom * urn);
    static Name * create(String * name, Atom * urn);

protected: static Name * create(String * pS, const TCHAR * pch, int iLen, Atom * pAtomURN);

public:

    virtual Atom * getName()
    {
        return _pAtomGI;
    }

    virtual Atom * getNameSpace()
    {
        return _pAtomURN;
    }

    virtual bool equals(Object * that);
    
    virtual String * toString();

    /**
     * return the hash code for this name object
     */
    virtual int hashCode();

#if DBG == 1
    static long created;
    static long reused;
#endif

protected:

    /**
     * Hash table for shared qualified names.
     */
#ifdef STRONGREFATOM
    static StringHashtable* s_pNames;
#else
    static WeakRefStringHashtable* s_pNames;
#endif

    /**
     * A name is a compound object containing two atoms.
     */
    RAtom _pAtomURN;
    RAtom _pAtomGI;
    RString _pStringKey;
    
    virtual void finalize();
    virtual void removeFromHashtable();
};

#endif _CORE_UTIL_NAME
