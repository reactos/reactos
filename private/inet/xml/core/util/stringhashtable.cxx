/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

DEFINE_CLASS_MEMBERS_CLONING(StringHashtable, _T("StringHashtable"), Hashtable);


StringHashtable *
StringHashtable::newStringHashtable(int initialCapacity, Mutex * pMutex, bool fAddRef)
{
    return new StringHashtable(initialCapacity, pMutex, fAddRef);
}


Object * StringHashtable::get(const TCHAR * c, int length)
{
    Object *result;
    HashEntry *pEntry;

    MutexReadLock lock(_pMutex);
    TRY
    {
        result = (find(c, length, String::hashCode(c, length), &pEntry) == Present)
                        ? (Object *)pEntry->value()
                        : null;
    }
    CATCH
    {
        lock.Release();
        Exception::throwAgain();
    }
    ENDTRY

    return result;
}

