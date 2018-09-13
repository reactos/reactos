/*
 * @(#)Atom.hxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_ATOM
#define _CORE_UTIL_ATOM

#ifndef _CORE_LANG_STRING
#include "core/lang/string.hxx"
#endif

#ifndef _CORE_UTIL_STRINGHASHTABLE
#include "core/util/stringhashtable.hxx"
#endif

DEFINE_CLASS(Atom);
DEFINE_CLASS(Name);

/**
 * This is a general purpose object to allow efficient
 * sharing of duplicate strings in the system.  It does this by
 * creating a global HashTable of all Atoms that have been 
 * constructed.
 *
 * @version 1.0, 6/3/97
 */
class DLLEXPORT Atom: public HashtableBase
{
    friend class Name;
    friend class Base;

    public: static void classInit();

    DECLARE_CLASS_MEMBERS(Atom, HashtableBase);
    DECLARE_CLASS_INSTANCE(Atom, HashtableBase);

    // override these methods because this object doesn't
    // get added to the zero count list
    public: ULONG STDMETHODCALLTYPE AddRef() { return _qAddRef(); } 
    public: ULONG STDMETHODCALLTYPE Release() { return _qRelease(); }
            
    /**
     * The shared string
     */
    private: RString s;
    
    /**
     * Cached hash value for improved compare speed.
     */    
    private: int hash;
    
    /**
     * Hash table for shared atoms.
     */
#ifdef STRONGREFATOM
    private: static StringHashtable* atoms;
#else
    private: static WeakRefStringHashtable* atoms;
#endif

    /**
     * Creates Atom * new object with the passed in collation key.
     * This is private because all Atoms should be constructed
     * via the static Atom.create() method.
     */
    private: Atom(String * s, int h);

    /**
     * private constructor
     */
    //private: Atom() : super(NoZeroList)
    private: Atom()
    {
    }

    /**
     * Create a Atom * object for this string.
     * Atoms are case sensitive - i.e. it assumes any case folding
     * has already been done.
     */    
    public: static Atom * create(String * s);
    public: static Atom * create(const TCHAR * c, int length);
    public: static Atom * create(const TCHAR * c);

    protected: static Atom * create(String * pS, const TCHAR * pch, int iLen);

    /**
     * Return the hash code for the name.
     * @return returns the hash code for the name.
     */
    public: virtual int hashCode()
    {
        return hash;
    }
 
    /**
     * Return the string represented by the Atom.
     */
    public: virtual String * toString() 
    {
        return s;
    }
    
    /**
     * Return whether this Atom * is equal to another given Atom.
     */
    public: virtual bool equals(Object * that) 
    {
        if (this == that) 
        {
            return true;
        }
        if (that == null || getClass() != that->getClass()) 
        {
            return false;
        }
        return s->equals(((Atom *)that)->s);
    }

    public: bool equals(const TCHAR * pwc, long ulLen);
    public: bool hasPrefixI(const TCHAR * pwc, long ulLen);

    protected: virtual void finalize();
    protected: virtual void removeFromHashtable();
};



#endif _CORE_UTIL_ATOM

