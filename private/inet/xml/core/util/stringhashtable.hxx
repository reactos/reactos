/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_STRINGHASHTABLE
#define _CORE_UTIL_STRINGHASHTABLE

#ifndef _CORE_UTIL_HASHTABLE
#include "core/util/hashtable.hxx"
#endif

/**
 * A hashtable which is using the String objects as the key.
 */

DEFINE_CLASS(StringHashtable);

class StringHashtable : public Hashtable
{
    DECLARE_CLASS_MEMBERS(StringHashtable, Hashtable);
    DECLARE_CLASS_CLONING(StringHashtable, Hashtable);

protected:
    StringHashtable(int initialCapacity = HT_DEFAULT_INITIAL_CAPACITY, 
                    Mutex * pMutex = null,
                    bool fAddRef = true)
    : super(initialCapacity, pMutex, fAddRef)
    {}

public:
    static
    StringHashtable *
    newStringHashtable(int initialCapacity = HT_DEFAULT_INITIAL_CAPACITY, 
                       Mutex * pMutex = null,
                       bool fAddRef = true);

    Object * get(const TCHAR * c, int length);
   
    // have to override otherwise first get will hide this one !
    Object * get(Object * key)
    {
        Assert(String::_getClass()->isInstance(key));
        return super::get(key);
    }
};



#endif _CORE_UTIL_STRINGHASHTABLE
