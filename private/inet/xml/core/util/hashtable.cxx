/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "core/util/hashtable.hxx"

Enumeration * HashtableEnumerator::newHashtableEnumerator(
              Hashtable *table, EnumType enumType)
{
    HashtableEnumerator * he = new HashtableEnumerator(); 
    he->_table = table;
    he->_position = 0;
    he->_enumType = enumType;

    return he;
}

///////////////////////////////////////////////////////////////////////////////
//////////////////    Hashtable    ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_MEMBERS_CLONING(Hashtable, _T("Hashtable"), Base);

const float atticRatio = 0.85f;

//////////////   Java constructors   //////////////////////////////////////////

Hashtable *
Hashtable::newHashtable(int initialCapacity, 
                        Mutex * pMutex,
                        bool fAddRef)
{
    return new Hashtable(initialCapacity, pMutex, fAddRef);
}

Hashtable::Hashtable(int initialCapacity, Mutex * pMutex, bool fAddRef)
{
    if (initialCapacity < HT_DEFAULT_INITIAL_CAPACITY)
        initialCapacity = HT_DEFAULT_INITIAL_CAPACITY;
    float loadFactor = HT_DEFAULT_LOAD_FACTOR;

    _addref = fAddRef;
    _loadFactor = loadFactor;
    _threshold = (int)(initialCapacity * loadFactor);
    _attic = (int)(initialCapacity * atticRatio);
    _emptyIndex = initialCapacity;
    _table = new (initialCapacity) HashArray;
    _pMutex = pMutex;
}

void Hashtable::finalize()
{ 
    clear(); 
    _table = null; 
    _pMutex = null;
    super::finalize(); 
}

//////////////////////   Java methods   /////////////////////////////////////

// Clears this hashtable so that it contains no keys. 
void
Hashtable::clear()
{
#ifdef _ALPHA_
    Assert(!_fResizing);
#endif
    if (_size > 0)
    {
        for (int i=0; i<_table->length(); ++i)
        {
            HashEntry& entry = (*_table)[i];
            if (entry.isOccupied())
            {
                entry.clear(this);
            }
        }

        _size = 0;
    }
}


// Creates a shallow copy of this hashtable. 
Object *
Hashtable::clone()
{
#ifdef _ALPHA_
    Assert(!_fResizing);
#endif
    Hashtable *result = CAST_TO(Hashtable*, super::clone());
    HashArray *newTable = new (_table->length()) HashArray;

    result->_addref = _addref;
    result->_size = _size;
    result->_loadFactor = _loadFactor;
    result->_threshold = _threshold;
    result->_attic = _attic;
    result->_emptyIndex = _emptyIndex;
    result->_table = newTable;

    for (int i=0; i<_table->length(); ++i)
    {
        HashEntry& myEntry = (*_table)[i];
        HashEntry& newEntry = (*newTable)[i];

        if (myEntry.isOccupied())
        {
            newEntry.assign(this, myEntry);
        }
    }

    return result;
}


// Returns the value to which the specified key is mapped in this hashtable. 
IUnknown *
Hashtable::_get(Object *key)
{
    IUnknown *result;
    HashEntry *pEntry;

    MutexReadLock lock(_pMutex);
    TRY
    {
        result = (find(key, key->hashCode(), &pEntry) == Present) ? pEntry->value()
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


// Maps the specified key to the specified value in this hashtable. 
IUnknown *
Hashtable::_put(Object *key, IUnknown *value)
{
    IUnknown * previousValue;
    MutexLock lock(_pMutex);
    TRY
    {
        previousValue = _set(key, value, false);
    }
    CATCH
    {
        lock.Release();
        Exception::throwAgain();
    }
    ENDTRY
    return previousValue;
}


// Add the value if not there, returns existing value if there
IUnknown *
Hashtable::_add(Object *key, IUnknown *value)
{
    IUnknown * previousValue;
    MutexLock lock(_pMutex);
    TRY
    {
        previousValue = _set(key, value, true);
    }
    CATCH
    {
        lock.Release();
        Exception::throwAgain();
    }
    ENDTRY
    return previousValue;
}


// Maps the specified key to the specified value in this hashtable. 
IUnknown *
Hashtable::_set(Object *key, IUnknown *value, bool add)
{
    IUnknown *previousValue = null;

    if (key != null && value != null)
    {
        int hashcode = key->hashCode();
        HashEntry *pEntry;
    
        switch (find(key, hashcode, &pEntry))
        {
        case Present:
            previousValue = pEntry->value();
            if (!add)
                pEntry->setValue(this, value);
            break;

        case EndOfList:
            findEmptySlot();
            pEntry->appendToChain(_emptyIndex);
            pEntry = &(*_table)[_emptyIndex];
            // fall through!

        case Empty:     // case EndOfList falls through to here!!
            pEntry->set(this, key, value, hashcode);
            ++_size;
            if (add)
                previousValue = value;
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

    return previousValue;
}


// Rehashes the contents of the hashtable into a hashtable with a larger capacity. 
// Note: caller must take lock.
void
Hashtable::rehash()
{
    // BUGBUG: there is an intermittent bug under NT5 on one specific alpha machine
    // in the NT build lab, whereby we end up with a recursive rehash.  That is bad.
    // this is here to stop the problem before it gets out-of-hand.
#ifdef _ALPHA_
    if(_fResizing)
        DebugBreak(); 
    _fResizing = true;
#endif

    HashArray *oldTable = _table;
    int oldCapacity = oldTable->length();
    int newCapacity = 2*oldCapacity;
    HashArray * newTable = new (newCapacity) HashArray;

    // prepare a larger clear table
    _size = 0;
    _threshold = (int)(newCapacity * _loadFactor);
    _attic = (int)(newCapacity * atticRatio);
    _emptyIndex = newCapacity;
    _table = newTable;

    // rehash the old entries into the new table
    for (int i=0; i<oldCapacity; ++i)
    {
        HashEntry& entry = (*oldTable)[i];
        if (entry.isOccupied())
        {
            _set(entry.key(), entry.value(), false);
            entry.clear(this);
        }
    }
#ifdef _ALPHA_
    _fResizing = false;
#endif
}


// Removes the key (and its corresponding value) from this hashtable. 
IUnknown *
Hashtable::_remove(Object *key)
{
    // this implementation is (relatively) simple, but has a bad worst case.
    // It re-inserts everything in the chain after the deleted element, so if
    // it's a long chain we do a lot of work.  Because coalesced chaining tends
    // to yield short chains (length<=5 at loadFactor=0.92), I think it's an
    // acceptable risk.  - SWB

    IUnknown *oldValue;
    MutexLock lock(_pMutex);
    TRY
    {
        int index, prevIndex;
        HashEntry *pEntry;
        int chainFirst;
    
        if (find(key, key->hashCode(), &pEntry, &index, &prevIndex) == Present)
        {
            // remember where we are, so we can start re-inserting the chain
            chainFirst = pEntry->nextIndex();
            oldValue = pEntry->value();
        
            // delete the target entry and cut the chain before it
            pEntry->clear(this);
            --_size;
            if (prevIndex != -1)
            {
                (*_table)[prevIndex].markEndOfList();
            }

            // make the new empty entry available for chaining
            if (_emptyIndex <= index)
                _emptyIndex = index+1;

            // re-insert the remaining elements on the chain
            HashEntry temp;
            temp._value = null;
            TRY
            {
                while (chainFirst != -1)
                {
                    // get the first element on the chain
                    HashEntry & entry = (*_table)[chainFirst];
                    temp._key = entry.key();
                    temp.setValue(this, entry.value());

                    // remove it (temporarily) 
                    if (_emptyIndex <= chainFirst)
                        _emptyIndex = chainFirst+1;
                    chainFirst = entry.nextIndex();
                    entry.clear(this);
                    --_size;

                    // re-insert it
                    // Cannot use _put() here because it will deadlock since
                    // it also tries to grab the lock.
                    // _put(temp._key, temp.value());
                    _set(temp._key, temp.value(), false);
                }
            }
            CATCH
            {
                temp.clear(this);
                Exception::throwAgain();
            }
            ENDTRY
            temp.clear(this);
        }
        else    // key not found
        {
            oldValue = null;
        }
    }
    CATCH
    {
        lock.Release();
        Exception::throwAgain();
    }
    ENDTRY
    return oldValue;
}

// Tests if some key maps into the specified value in this hashtable. 
bool
Hashtable::contains(Object *value)
{
    for (int i=0; i<_table->length(); ++i)
    {
        HashEntry & entry = (*_table)[i];
        if (entry.isOccupied() && equalValue((Object *)entry.value(), (Object *)value))
        {
            return true;
        }
    }

    return false;
}

bool Hashtable::equalValue(Object * pDst, Object * pSrc)
{
    return pDst->equals(pSrc);
}


Hashtable::FindResult
Hashtable::find(Object *key, int hashcode, HashEntry **ppEntry,
                int *pIndex, int *pPrevIndex)
{
    Assert(hashcode == key->hashCode());
    Assert(ppEntry);
    
    int index = (hashcode & 0x7fffffff) % _attic;
    int prevIndex = -1;
    FindResult result = Unknown;
    HashEntry *pEntry = null;

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


// special-purpose implementation of find for use by StringHashtable.
// this enables looking up a key without creating a String object
Hashtable::FindResult
Hashtable::find(const TCHAR *s, int len, int hashcode, HashEntry **ppEntry)
{
    Assert(ppEntry);
    
    int index = (hashcode & 0x7fffffff) % _attic;
    FindResult result = Unknown;
    HashEntry *pEntry = null;

    while (result == Unknown)
    {
        pEntry = &(*_table)[index];

        if (pEntry->isEmpty())
        {
            result = Empty;
        }
        else if (pEntry->holdsKey(s, len, hashcode))
        {
            result = Present;
        }
        else if (pEntry->isEndOfList())
        {
            result = EndOfList;
        }
        else
        {
            index = pEntry->nextIndex();
        }
    }

    *ppEntry = pEntry;
    
    return result;
}


void
Hashtable::findEmptySlot()
{
    do {
        --_emptyIndex;
    } while ((*_table)[_emptyIndex].isOccupied());

    AssertSz(_emptyIndex > 0, "Can't find empty slot");
}


///////////////////////////////////////////////////////////////////////////////
//////////////////    HashtableEnumerator    //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// use ABSTRACT because of no default constructor 
DEFINE_ABSTRACT_CLASS_MEMBERS(HashtableEnumerator, _T("HashtableEnumerator"), Base);


// Tests if this enumeration contains more elements. 
bool
HashtableEnumerator::hasMoreElements()
{
    for (int i=_position; i<_table->_table->length(); ++i)
    {
        HashEntry& entry = (*_table->_table)[i];
        if (entry.isOccupied())
            return true;
    }

    return false;
}

// Returns the next element of this enumeration && position
Object *
HashtableEnumerator::_peekElement(int *position)
{
    for (int i=_position; i<_table->_table->length(); ++i)
    {
        HashEntry& entry = (*_table->_table)[i];
        if (entry.isOccupied())
        {
            *position = i;
            switch (_enumType)
            {
            case Keys:
                return entry.key();
                break;

            case Values:
                return (Object *)entry.value();
                break;
            }
        }
    }
    return null;
}


// Returns the next element of this enumeration. 
Object *
HashtableEnumerator::peekElement()
{
    int i;
    Object * o = _peekElement(&i);
    if (o)
    {
        return o;
    }

    Exception::throwE(E_UNEXPECTED); // Exception::NoSuchElementException);
    return null;
}


// Returns the next element of this enumeration. 
Object *
HashtableEnumerator::nextElement()
{
    Object * o = _peekElement(&_position);
    _position++;
    return o;
}


// resets the enumeration
void HashtableEnumerator::reset()
{
    _position = 0;
}


// Called by the garbage collector on an object when garbage collection 
// determines that there are no more references to the object. 
void
HashtableEnumerator::finalize()
{
    _table = null;
    super::finalize();
}


