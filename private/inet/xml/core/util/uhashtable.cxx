/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "core/util/uhashtable.hxx"

///////////////////////////////////////////////////////////////////////////////
//////////////////    Hashtable    ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_MEMBERS_CLONING(UHashtable, _T("UHashtable"), Base);

const float atticRatio = 0.85f;

//////////////   Java constructors   //////////////////////////////////////////

UHashtable::UHashtable(int initialCapacity, float loadFactor)
{
    if (initialCapacity < HT_DEFAULT_INITIAL_CAPACITY)
        initialCapacity = HT_DEFAULT_INITIAL_CAPACITY;
    if (loadFactor < 0.0f || loadFactor >= HT_DEFAULT_LOAD_FACTOR)
        loadFactor = HT_DEFAULT_LOAD_FACTOR;

    _loadFactor = loadFactor;
    _threshold = (int)(initialCapacity * loadFactor);
    _attic = (int)(initialCapacity * atticRatio);
    _emptyIndex = initialCapacity;
    _table = new (initialCapacity) UHashArray;
}

void UHashtable::finalize()
{ 
    clear(); 
    _table = null; 
    super::finalize(); 
}

//////////////////////   Java methods   /////////////////////////////////////

// Clears this hashtable so that it contains no keys. 
void
UHashtable::clear()
{
    if (_size > 0)
    {
        for (int i=0; i<_table->length(); ++i)
        {
            UHashEntry& entry = (*_table)[i];
            if (entry.isOccupied())
            {
                entry.clear();
            }
        }

        _size = 0;
    }
}

// Returns the value to which the specified key is mapped in this hashtable. 
IUnknown *
UHashtable::get(Object *key)
{
    IUnknown *result;
    UHashEntry *pEntry;

    result = (find(key, key->hashCode(), &pEntry) == Present) ? pEntry->value()
                                                              : null;
    return result;
}


// Maps the specified key to the specified value in this hashtable. 
void
UHashtable::put(Object *key, IUnknown *value)
{
    if (key != null && value != null)
    {
        int hashcode = key->hashCode();
        UHashEntry *pEntry;
        
        switch (find(key, hashcode, &pEntry))
        {
        case Present:
            pEntry->setValue(value);
            break;

        case EndOfList:
            findEmptySlot();
            pEntry->appendToChain(_emptyIndex);
            pEntry = &(*_table)[_emptyIndex];
            // fall through!

        case Empty:     // case EndOfList falls through to here!!
            pEntry->set(key, value, hashcode);
            ++ _size;
            break;

        default:
            AssertSz(0, "Hashtable::Find() returns unknown code");
            break;
        }

        if (size() > _threshold)
        {
            rehash();
        }
    }
    else
    {
        Exception::throwE(E_POINTER); // Exception::NullPointerException);
    }
}


// Rehashes the contents of the hashtable into a hashtable with a larger capacity. 
void
UHashtable::rehash()
{
    UHashArray *oldTable = _table;
    int oldCapacity = oldTable->length();
    int newCapacity = 2*oldCapacity;
    UHashArray * newTable = new (newCapacity) UHashArray;

    // prepare a larger clear table
    _size = 0;
    _threshold = (int)(newCapacity * _loadFactor);
    _attic = (int)(newCapacity * atticRatio);
    _emptyIndex = newCapacity;
    _table = newTable;

    // rehash the old entries into the new table
    for (int i=0; i<oldCapacity; ++i)
    {
        UHashEntry& entry = (*oldTable)[i];
        if (entry.isOccupied())
        {
            IUnknown * pUnk = entry.value();
            put(entry.key(), pUnk);
            pUnk->Release();
            entry.clear();
        }
    }
}


// Removes the key (and its corresponding value) from this hashtable. 
IUnknown *
UHashtable::remove(Object *key)
{
    // this implementation is (relatively) simple, but has a bad worst case.
    // It re-inserts everything in the chain after the deleted element, so if
    // it's a long chain we do a lot of work.  Because coalesced chaining tends
    // to yield short chains (length<=5 at loadFactor=0.92), I think it's an
    // acceptable risk.  - SWB

    int index, prevIndex;
    UHashEntry *pEntry;
    int chainFirst;
    IUnknown *oldValue;
    
    if (find(key, key->hashCode(), &pEntry, &index, &prevIndex) == Present)
    {
        // remember where we are, so we can start re-inserting the chain
        chainFirst = pEntry->nextIndex();
        oldValue = pEntry->value();

        // delete the target entry and cut the chain before it
        pEntry->clear();
        -- _size;
        if (prevIndex != -1)
        {
            (*_table)[prevIndex].markEndOfList();
        }

        // make the new empty entry available for chaining
        if (_emptyIndex <= index)
            _emptyIndex = index+1;

        // re-insert the remaining elements on the chain
        while (chainFirst != -1)
        {
            // get the first element on the chain
            UHashEntry & entry = (*_table)[chainFirst];
            Object *key = entry.key();
            IUnknown *value = entry.value();

            // remove it (temporarily) 
            if (_emptyIndex <= chainFirst)
                _emptyIndex = chainFirst+1;
            chainFirst = entry.nextIndex();
            entry.clear();
            -- _size;

            // re-insert it
            put(key, value);
            value->Release();
        }
    }
    else    // key not found
    {
        oldValue = null;
    }

    return oldValue;
}


UHashtable::FindResult
UHashtable::find(Object *key, int hashcode, UHashEntry **ppEntry,
                int *pIndex, int *pPrevIndex)
{
    Assert(hashcode == key->hashCode());
    Assert(ppEntry);
    
    int index = (hashcode & 0x7fffffff) % _attic;
    int prevIndex = -1;
    FindResult result = Unknown;
    UHashEntry *pEntry = null;

    while (result == Unknown)
    {
        pEntry = &(*_table)[index];

        if (pEntry->isEmpty())
        {
            result = Empty;
        }
        else if (pEntry->holdsKey(key, hashcode))
        {
            result = Present;
        }
        else if (pEntry->isEndOfList())
        {
            result = EndOfList;
        }
        else
        {
            prevIndex = index;
            index = pEntry->nextIndex();
        }
    }

    *ppEntry = pEntry;
    if (pIndex)
        *pIndex = index;
    if (pPrevIndex)
        *pPrevIndex = prevIndex;
    
    return result;
}


void
UHashtable::findEmptySlot()
{
    do {
        -- _emptyIndex;
    } while ((*_table)[_emptyIndex].isOccupied());

    AssertSz(_emptyIndex > 0, "Can't find empty slot");
}///////////////////////////////////////////////////////////////////////////////
//////////////////    UHashtableIter         //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(UHashtableIter, _T("UHashtableIter"), Base);

UHashtableIter * 
UHashtableIter::newUHashtableIter(
			  UHashtable *table)
{
    UHashtableIter * he = new UHashtableIter(); 
    he->_table = table;
	he->_position = 0;

	return he;
}

// Tests if this enumeration contains more elements. 
bool
UHashtableIter::hasMoreElements()
{
    for (int i=_position; i<_table->_table->length(); ++i)
    {
        UHashEntry& entry = (*_table->_table)[i];
        if (entry.isOccupied())
            return true;
    }

    return false;
}


// Returns the next element of this enumeration. 
IUnknown *
UHashtableIter::nextElement(Object ** ppKey)
{
    for ( ; _position<_table->_table->length(); )
    {
        UHashEntry& entry = (*_table->_table)[_position++];
        if (entry.isOccupied())
        {
            if (ppKey)
                *ppKey = entry.key();
            IUnknown * pUnk = entry.value();
            return pUnk;
        }
    }

    return null;
}


// resets the enumeration
void UHashtableIter::reset()
{
    _position = 0;
}


// Called by the garbage collector on an object when garbage collection 
// determines that there are no more references to the object. 
void
UHashtableIter::finalize()
{
    _table = null;
    super::finalize();
}


