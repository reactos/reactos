/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _CORE_UTIL_UHASHTABLE
#define _CORE_UTIL_UHASHTABLE

DEFINE_CLASS(UHashtable);
DEFINE_CLASS(UHashtableIter);

#define FINAL
#define VIRTUAL virtual

#define HT_DEFAULT_INITIAL_CAPACITY     (16)
#define HT_DEFAULT_LOAD_FACTOR          (0.92f)

class UHashEntry
{
    friend class UHashtable;
    friend class UHashtableIter;

public:
    Object * key() const { return _key; }
    IUnknown * value() const { _value->AddRef(); return _value; }
    UHashEntry& operator=(const UHashEntry& rhs)
        { _key=rhs._key; _value=rhs._value; _hashcode=rhs._hashcode;
          _link=rhs._link; return *this; }
    
private:
    bool holdsKey(Object *key, int hashcode)
            { return _hashcode==hashcode && _key->equals(key); }
    bool holdsKey(TCHAR *s, int len, int hashcode)
            { return _hashcode==hashcode && ((String*)(Object*)_key)->equals(s, len); }
    bool isEmpty() const { return _link == 0; }
    bool isOccupied() const { return _link != 0; }
    bool isEndOfList() const { return _link == -1; }
    int  nextIndex() const { return _link; }
    void clear() { _key=null; _value=null; _link=0; }
    void markEndOfList() { _link = -1; }
    void appendToChain(int index) { _link = index; }
    void set(Object *key, IUnknown *value, int hashcode)
            { _key=key; _value=value, _hashcode=hashcode; _link=-1; }
    void setValue(IUnknown *value) { _value = value; }

    ///////////////   representation   ///////////////////////////////////
private:
    RObject _key;
    _reference<IUnknown> _value;
    int     _hashcode;
    int     _link;
};

typedef _array<UHashEntry> UHashArray;
typedef _reference<UHashArray> RUHashArray;

class UHashtableIter: public Base
{
    DECLARE_CLASS_MEMBERS(UHashtableIter, Base);

	private: UHashtableIter() {}
    
    public: DLLEXPORT static UHashtableIter * newUHashtableIter(
				UHashtable *table);

    //  hasMoreElements() [Enumeration]
    // Tests if this enumeration contains more elements. 
    public: bool hasMoreElements();

    //  nextElement() [Enumeration]
    // Returns the next element of this enumeration. 
    public: IUnknown * nextElement( Object ** ppKey);

    //  reset() [Enumeration]
    // Resets this enumeration. 
    public: void  reset();

    //  finalize() [Object]
    // Called by the garbage collector on an object when garbage collection 
    // determines that there are no more references to the object. 
    public: VIRTUAL void finalize();

    /////////////   Representation    /////////////////////////////////////
private:
    RUHashtable  _table;
    int         _position;
};

class UHashtable: public Base // implements Cloneable
{
    friend class UHashtableIter;

    DECLARE_CLASS_MEMBERS(UHashtable, Base);
    DECLARE_CLASS_CLONING(UHashtable, Base);

    // cloning constructor, shouldn't do anything with data members...
    protected:  UHashtable(CloningEnum e) : super(e) {}


    //////////////   Java constructors   ////////////////////////////////////
    
    //  Hashtable() 
    // Constructs a new, empty hashtable with a default capacity and load factor. 

    //  Hashtable(int) 
    // Constructs a new, empty hashtable with the specified initial capacity and 
    // default load factor. 

    //  Hashtable(int, float) 
    // Constructs a new, empty hashtable with the specified initial capacity and the 
    // specified load factor. 
    public: UHashtable(int initialCapacity=HT_DEFAULT_LOAD_FACTOR,
                      float loadFactor=HT_DEFAULT_LOAD_FACTOR);

    //////////////////////   Java methods   /////////////////////////////////////
    
    //  clear() 
    // Clears this hashtable so that it contains no keys. 
    public: VIRTUAL void clear();

    //  containsKey(Object) 
    // Tests if the specified object is a key in this hashtable. 
    public: VIRTUAL bool containsKey(Object *key) { return (get(key) != null); }

    //  get(Object) 
    // Returns the value to which the specified key is mapped in this hashtable. 
    public: VIRTUAL IUnknown * get(Object *key);

    //  isEmpty() 
    // Tests if this hashtable maps no keys to values. 
    public: VIRTUAL bool isEmpty() const { return (size() == 0); }

    //  put(Object, Object) 
    // Maps the specified key to the specified value in this hashtable. 
    public: VIRTUAL void put(Object *key, IUnknown *value);

    //  rehash() 
    // Rehashes the contents of the hashtable into a hashtable with a larger capacity. 
    protected: VIRTUAL void rehash();

    //  remove(Object) 
    // Removes the key (and its corresponding value) from this hashtable. 
    public: VIRTUAL IUnknown * remove(Object *key);

    //  size() 
    // Returns the number of keys in this hashtable. 
    public: VIRTUAL int size() const { return _size; }

    //  toString() 
    // Returns a rather long string representation of this hashtable. 

    //  finalize() [Object]
    // Called by the garbage collector on an object when garbage collection 
    // determines that there are no more references to the object. 
    protected: VIRTUAL void finalize();

    ///////////////   helper methods   ///////////////////////////////////
protected:
    enum FindResult { Unknown, Present, Empty, EndOfList };

    FindResult  find(Object *key, int hashcode, UHashEntry **ppEntry,
                     int *pIndex=null, int *pPrevIndex=null);
    void        findEmptySlot();

    ///////////////   representation   ///////////////////////////////////
private:
    int         _size;
    float       _loadFactor;
    int         _threshold;
    int         _attic;
    int         _emptyIndex;
    RUHashArray  _table;
};


#undef FINAL
#undef VIRTUAL

#endif _CORE_UTIL_UHASHTABLE

