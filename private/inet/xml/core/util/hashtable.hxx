/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_HASHTABLE
#define _CORE_UTIL_HASHTABLE

#include "enumeration.hxx"
#include "mt.hxx"

DEFINE_CLASS(HashtableEnumerator);
DEFINE_CLASS(Hashtable);

#define HT_DEFAULT_INITIAL_CAPACITY     (16)
#define HT_DEFAULT_LOAD_FACTOR          (0.92f)

class HashEntry
{
    friend class HashtableEnumerator;
    friend class Hashtable;

public:
    Object * key() const { return _key; }
    IUnknown * value() const { return _value; }
    HashEntry& operator=(const HashEntry& rhs) 
        { Assert(FALSE && "SHOULDN'T BE USED"); return *this; }
    
public:
    bool holdsKey(Object *key, int hashcode)
            { return _hashcode==hashcode && _key->equals(key); }
    bool holdsKey(const TCHAR *s, int len, int hashcode)
            { return _hashcode==hashcode && ((String*)(Object*)_key)->equals(s, len); }
    bool isEmpty() const { return _link == 0; }
    bool isOccupied() const { return _link != 0; }
    bool isEndOfList() const { return _link == -1; }
    int  nextIndex() const { return _link; }
    void markEndOfList() { _link = -1; }
    void appendToChain(int index) { _link = index; }

    inline void clear(Hashtable * pHT);
    inline void set(Hashtable * pHT, Object *key, IUnknown *value, int hashcode);
    inline void setValue(Hashtable * pHT, IUnknown *value);
    inline void assign(Hashtable * pHT, const HashEntry& rhs);

    inline void _release(Hashtable * pHT);
    inline void _assign(Hashtable * pHT, IUnknown * value);

    ///////////////   representation   ///////////////////////////////////
private:
    RObject     _key;
    IUnknown *  _value;
    int         _hashcode;
    int         _link;
};


typedef _array<HashEntry> HashArray;
typedef _reference<HashArray> RHashArray;


class HashtableEnumerator: public Base, public Enumeration 
{
    DECLARE_CLASS_MEMBERS_I1(HashtableEnumerator, Base, Enumeration);

public: 
    enum EnumType { Keys, Values };

public: 
    // construct a new Hashtable Enumeraton
    static Enumeration * newHashtableEnumerator(Hashtable *table, EnumType enumType);

    //  hasMoreElements() [Enumeration]
    // Tests if this enumeration contains more elements. 
    bool hasMoreElements();

    //  peekElement() [Enumeration]
    // Peeks the next element of this enumeration but does not advance. 
    Object * peekElement();

    //  nextElement() [Enumeration]
    // Returns the next element of this enumeration. 
    Object * nextElement();


    //  reset() [Enumeration]
    // Resets this enumeration. 
    void reset();

    /////////////   Non-Public methods  /////////////////////////////////////
protected: 
    Object * _peekElement(int *position);

    //  finalize() [Object]
    // Called by the garbage collector on an object when garbage collection 
    // determines that there are no more references to the object. 
    void finalize();

private:
    // use newHashTableEnumerator(...)
    HashtableEnumerator() {}
    
    /////////////   Representation    /////////////////////////////////////
private:
    RHashtable  _table;
    int         _position;
    EnumType    _enumType;
};

class Hashtable: public Base // implements Cloneable
{
    friend class HashtableEnumerator;
    friend class HashEntry;
    friend BOOL Base::FreeObjects(Hashtable * pht);

    DECLARE_CLASS_MEMBERS(Hashtable, Base);
    DECLARE_CLASS_CLONING(Hashtable, Base);

protected:
    // cloning constructor, shouldn't do anything with data members...
    Hashtable(CloningEnum e) : super(e) {}

protected:
    //////////////   Java constructors   ////////////////////////////////////
    
    //  Hashtable(int, Mutex, float) 
    // Constructs a new, empty hashtable with the specified initial capacity and the 
    // specified load factor, using the given Mutex for locking
    Hashtable(int initialCapacity = HT_DEFAULT_INITIAL_CAPACITY, 
              Mutex * pMutex = null,
              bool fAddRef = true);

public:
    //  Hashtable(int, Mutex, float) 
    // Constructs a new, empty hashtable with the specified initial capacity and the 
    // specified load factor, using the given Mutex for locking
    static
    Hashtable *
    newHashtable(int initialCapacity, 
                 Mutex * pMutex = null,
                 bool fAddRef = true);

    //////////////////////   Java methods   /////////////////////////////////////
    
    //  clear() 
    // Clears this hashtable so that it contains no keys. 
    void clear();

    //  clone() 
    // Creates a shallow copy of this hashtable. 
    virtual Object * clone();

    //  containsKey(Object) 
    // Tests if the specified object is a key in this hashtable. 
    bool containsKey(Object *key) { return (get(key) != null); }

    //  elements() 
    // Returns an enumeration of the values in this hashtable. 
    Enumeration * elements()
        { 
            return HashtableEnumerator::newHashtableEnumerator(this, HashtableEnumerator::Values); 
        }

    //  isEmpty() 
    // Tests if this hashtable maps no keys to values. 
    bool isEmpty() const { return (size() == 0); }

    //  keys() 
    // Returns an enumeration of the keys in this hashtable. 
    Enumeration * keys()
        { 
            return HashtableEnumerator::newHashtableEnumerator(this, HashtableEnumerator::Keys); 
        }

    //  get(Object) 
    // Returns the value to which the specified key is mapped in this hashtable. 
    IUnknown * _get(Object *key);

    //  put(Object, Object) 
    // Maps the specified key to the specified value in this hashtable. 
    IUnknown * _put(Object *key, IUnknown *value);

    //  add(Object, Object) 
    // Adds the value with the specified key if it is not there, returns previous value. 
    IUnknown * _add(Object *key, IUnknown *value);

    //  remove(Object) 
    // Removes the key (and its corresponding value) from this hashtable. 
    IUnknown * _remove(Object *key);

    //  size() 
    // Returns the number of keys in this hashtable. 
    int size() const { return _size; }

    ///////////////   These return Object *   ///////////////////////////////////

    //  get(Object) 
    // Returns the value to which the specified key is mapped in this hashtable. 
    Object * get(Object *key) { return (Object *)_get(key); }

    //  put(Object, Object) 
    // Maps the specified key to the specified value in this hashtable. 
    Object * put(Object *key, Object *value) { return (Object *)_put(key, value); }

    //  add(Object, Object) 
    // Adds the value with the specified key if it is not there, returns previous value. 
    Object * add(Object *key, Object *value) { return (Object *)_add(key, value); }

    //  remove(Object) 
    // Removes the key (and its corresponding value) from this hashtable. 
    Object * remove(Object *key) { return (Object *)_remove(key); }

    //  contains(Object) 
    // Tests if some key maps into the specified value in this hashtable. 
    bool contains(Object *value);

    bool equalValue(Object * pDst, Object * pSrc);


protected: 
    IUnknown * _set(Object *key, IUnknown *value, bool add);

    //  rehash() 
    // Rehashes the contents of the hashtable into a hashtable with a larger capacity. 
    void rehash();

    //  finalize() [Object]
    // Called by the garbage collector on an object when garbage collection 
    // determines that there are no more references to the object. 
    virtual void finalize();


    ///////////////   helper methods   ///////////////////////////////////
protected:
    enum FindResult { Unknown, Present, Empty, EndOfList };

    FindResult  find(Object *key, int hashcode, HashEntry **ppEntry,
                     int *pIndex=null, int *pPrevIndex=null);
    FindResult  find(const TCHAR *s, int len, int hashcode, HashEntry **ppEntry);
    void        findEmptySlot();

    ///////////////   representation   ///////////////////////////////////
protected:

    bool        _addref;
    int         _size;
    float       _loadFactor;
    int         _threshold;
    int         _attic;
    int         _emptyIndex;
    RHashArray  _table;
    RMutex      _pMutex;

#ifdef _ALPHA_
private:
    // HACK: this is scary, 1 machine in the NT build lab ends up resizing recursively and 
    // that causes it to eat ~infinite memory.... not good.  No one can look at the code and 
    // explain how it could happen.  Thus this hack.
    bool        _fResizing;
#endif

};



inline 
void 
HashEntry::clear(Hashtable * pHT)
{ _key=null; _release(pHT); _link=0; }

inline 
void 
HashEntry::set(Hashtable * pHT, Object *key, IUnknown *value, int hashcode)
{ _key=key; _assign(pHT, value); _hashcode=hashcode; _link=-1; }

inline 
void 
HashEntry::setValue(Hashtable * pHT, IUnknown *value)
{ _assign(pHT, value); }

inline 
void 
HashEntry::assign(Hashtable * pHT, const HashEntry& rhs)
{ _key=rhs._key; _assign(pHT, rhs._value); 
  _hashcode=rhs._hashcode; _link=rhs._link; }

inline 
void 
HashEntry::_release(Hashtable * pHT) 
{ if (pHT->_addref) ::release(&_value); else _value = null; }

inline 
void 
HashEntry::_assign(Hashtable * pHT, IUnknown * value) 
{ if (pHT->_addref) ::assign(&_value, value); else _value = value; }


#endif _CORE_UTIL_HASHTABLE

