/*
 * @(#)NameSpaceContext.hxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */ 
#ifndef _CORE_UTIL_NAMESPACECONTEXT
#define _CORE_UTIL_NAMESPACECONTEXT

#ifndef _CORE_UTIL_HASHTABLE
#include "core/util/hashtable.hxx"
#endif

#ifndef _CORE_UTIL_STACK
#include "core/util/stack.hxx"
#endif

#ifndef _CORE_UTIL_ATOM
#include "core/util/atom.hxx"
#endif


DEFINE_CLASS(NameSpaceContext);

class NameSpaceContext: public Base
{
    DECLARE_CLASS_MEMBERS(NameSpaceContext, Base);

    public: NameSpaceContext();

    /**
     *  add name space. long name is the key
     */
    public: void addNameSpace(Atom * url, Atom * n);

    /**
     * find name space, long name is the key
     */
    public: Atom * findNameSpace(Atom * n);

    public: void push();
    
    public: void pop();

    RHashtable current;

    /**
     * Stack * to keep track of contexts
     */
    RStack contexts;

    protected: virtual void finalize()
    {
        current = null;
        contexts = null;
        super::finalize();
    }
};



#endif _CORE_UTIL_NAMESPACECONTEXT

