/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _BASE_HXX
#define _BASE_HXX

#undef DEBUGRENTAL

enum CloningEnum { Cloning };

enum NoZeroListEnum { NoZeroList };

    typedef Object * (*CREATEOBJECT)();


#if DBG == 1 // put _create, getClass, _getClass  in to catch calls where they are not defined

#define DECLARE_CLASS_INSTANCE(C, S) \
    private: static SRClass _class; 

#define DECLARE_CLASS_CLONING(C, S) DECLARE_CLASS_INSTANCE(C, S) 

#define _DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    public: static C * new##C(); \
    protected: static Object * _cloningCreate(); \
    public: virtual Class * getClass() const; \
    public: static Class * _getClass(); \
    typedef S super;

#define DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    public: void * __cdecl operator new(size_t cb) { return(::MemAllocObject(cb)); } \
    public: void __cdecl operator delete(void * pv) { ::MemFree(pv); } \
    _DECLARE_CLASS_MEMBERS_NOQI(C, S)

#else   // if DBG == 1

#define DECLARE_CLASS_INSTANCE(C, S) \
    private: static SRClass _class; \
    public: virtual Class * getClass() const; \
    public: static Class * _getClass(); 

#define DECLARE_CLASS_CLONING(C, S) DECLARE_CLASS_INSTANCE(C, S) \
    protected: static Object * _cloningCreate(); 

#define _DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    public: static C * new##C(); \
    typedef S super;

#define DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    public: void * __cdecl operator new(size_t cb) { return(::MemAllocObject(cb)); } \
    public: void __cdecl operator delete(void * pv) { ::MemFree(pv); } \
    _DECLARE_CLASS_MEMBERS_NOQI(C, S)

#endif  // if DBG == 1

#define _DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S) \
    private: virtual Base * getBase() { return this; } \
    public: void weakAddRef() { S::weakAddRef(); } \
    public: void weakRelease() { S::weakRelease(); } 

#define _DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S) _DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S) \
    public: ULONG STDMETHODCALLTYPE AddRef() { return _addRef(); } \
    public: ULONG STDMETHODCALLTYPE Release() { return _release(); } 

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S) DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    _DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_I1(C, S, I1) DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_I2(C, S, I1, I2) DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_I3(C, S, I1, I2, I3) DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_I4(C, S, I1, I2, I3, I4) DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQIADDREF_I5(C, S, I1, I2, I3, I4, I5) DECLARE_CLASS_MEMBERS_NOQIADDREF_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S) DECLARE_CLASS_MEMBERS_NOQI(C, S) \
    _DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_I1(C, S, I1) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_I2(C, S, I1, I2) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_I3(C, S, I1, I2, I3) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_I4(C, S, I1, I2, I3, I4) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_NOQI_I5(C, S, I1, I2, I3, I4, I5) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS(C, S) DECLARE_CLASS_MEMBERS_NOQI(C, S) 

#define DECLARE_CLASS_MEMBERS_INTERFACE(C, S) DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(C, S) \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject) { return super::QueryInterface(iid, ppvObject); } 

#define DECLARE_CLASS_MEMBERS_I1(C, S, I1) DECLARE_CLASS_MEMBERS_INTERFACE(C, S) 

#define DECLARE_CLASS_MEMBERS_I2(C, S, I1, I2) DECLARE_CLASS_MEMBERS_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_I3(C, S, I1, I2, I3) DECLARE_CLASS_MEMBERS_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_I4(C, S, I1, I2, I3, I4) DECLARE_CLASS_MEMBERS_INTERFACE(C, S)

#define DECLARE_CLASS_MEMBERS_I5(C, S, I1, I2, I3, I4, I5) DECLARE_CLASS_MEMBERS_INTERFACE(C, S)

#define DEFINE_NEWOBJECT(C, N, S) \
    C * C::new##C() { return new C(); } 

#define DEFINE_GETCLASS(C, N, S) \
    SRClass C::_class; \
    Class * C::getClass() const \
    { \
        return C::_getClass(); \
    } \
    Class * C::_getClass() \
    { \
        if (!_class) \
            _class = Base::newClass(N, ##S::_getClass(), (CREATEOBJECT)(C * (*)())C::new##C, C::_cloningCreate); \
        return _class; \
    } 

#define DEFINE_CLASS_MEMBERS_CLONING(C, N, S) \
    DEFINE_NEWOBJECT(C, N, S) \
    Object * C::_cloningCreate() { return (Object *)(Base *)new C(Cloning); } \
    DEFINE_GETCLASS(C, N, S)

#if DBG == 1
#define DEFINE_CLASS_MEMBERS_NEWINSTANCE(C, N, S) \
    DEFINE_NEWOBJECT(C, N, S) \
    Object * C::_cloningCreate() { Assert(FALSE && "Shouldn't be called"); return null; } \
    DEFINE_GETCLASS(C, N, S) 
#else
#define DEFINE_CLASS_MEMBERS_NEWINSTANCE(C, N, S) \
    DEFINE_NEWOBJECT(C, N, S) \
    DEFINE_GETCLASS(C, N, S) 
#endif

#if DBG == 1
#define _DEFINE_CLASS_MEMBERS_CLASS(C, N, S) \
    C * C::new##C() { Assert(FALSE && "Shouldn't be called"); return null; } \
    Object * C::_cloningCreate() { Assert(FALSE && "Shouldn't be called"); return null; } \
    SRClass C::_class; \
    Class * C::getClass() const \
    { \
        return C::_getClass(); \
    } \
    Class * C::_getClass() \
    { \
        if (!_class) \
            _class = Base::newClass(N, ##S::_getClass(), (CREATEOBJECT)null, (CREATEOBJECT)null); \
        return _class; \
    } 
#define DEFINE_CLASS_MEMBERS_CLASS(C, N, S) _DEFINE_CLASS_MEMBERS_CLASS(C, N, S)
#else
#define _DEFINE_CLASS_MEMBERS_CLASS(C, N, S) \
    SRClass C::_class; \
    Class * C::getClass() const \
    { \
        return C::_getClass(); \
    } \
    Class * C::_getClass() \
    { \
        if (!_class) \
            _class = Base::newClass(N, ##S::_getClass(), (CREATEOBJECT)null, (CREATEOBJECT)null); \
        return _class; \
    } 
#define DEFINE_CLASS_MEMBERS_CLASS(C, N, S) _DEFINE_CLASS_MEMBERS_CLASS(C, N, S)
#endif

#if DBG == 1
#define _DEFINE_CLASS_MEMBERS(C, N, S) \
    DEFINE_NEWOBJECT(C, N, S) \
    Class * C::getClass() const { Assert(FALSE && "Shouldn't be called"); return null; } \
    Class * C::_getClass() { Assert(FALSE && "Shouldn't be called"); return null; } \
    Object * C::_cloningCreate() { Assert(FALSE && "Shouldn't be called"); return null; } 
#define DEFINE_CLASS_MEMBERS(C, N, S) _DEFINE_CLASS_MEMBERS(C, N, S)
#else
#define DEFINE_CLASS_MEMBERS(C, N, S) \
    DEFINE_NEWOBJECT(C, N, S) 
#endif

#if DBG == 1
#define DEFINE_ABSTRACT_CLASS_MEMBERS(C, N, S) \
    C * C::new##C() { Assert(FALSE && "Shouldn't be called"); return null; } \
    Class * C::getClass() const { Assert(FALSE && "Shouldn't be called"); return null; } \
    Class * C::_getClass() { Assert(FALSE && "Shouldn't be called"); return null; } \
    Object * C::_cloningCreate() { Assert(FALSE && "Shouldn't be called"); return null; } 
#else
#define DEFINE_ABSTRACT_CLASS_MEMBERS(C, N, S) 
#endif


 
#if DBG == 1
ULONG_PTR SpinLock(ULONG_PTR * p, Base * b = null);
void SpinUnlock(ULONG_PTR *p, LONG_PTR l, Base * b = null);
#else
ULONG_PTR SpinLock(ULONG_PTR * p);
void SpinUnlock(ULONG_PTR *p, LONG_PTR l);
#endif


class BusyLock
{
    ULONG_PTR  _lOldValue;
    ULONG_PTR *_plLock;
    
public:
    BusyLock(ULONG_PTR *plLock)
    {
        _plLock = plLock;
        _lOldValue = ::SpinLock(_plLock);
    }

    ~BusyLock()
    {
        ::SpinUnlock(_plLock, _lOldValue);
    }
};

class DLLEXPORT Base : public Object
{
friend class Object;
friend class GenericBase;
friend void * ::MemAllocObject(size_t cb);
friend void ::ClearPointerCache();
friend Base * isObject(void *p, ULONG_PTR * pulRef, TLSDATA * ptlsFrozen);
#if DBG == 1
friend ULONG_PTR ::SpinLock(ULONG_PTR *p, Base * b);
friend void ::SpinUnlock(ULONG_PTR *p, LONG_PTR  l, Base * b);
#endif

    DECLARE_CLASS_MEMBERS_NOQI(Base, Object);
    DECLARE_CLASS_INSTANCE(Base, Object);

    // cloning constructor, shouldn't do anything with data members...
protected:  Base(CloningEnum);
    // constructor for atoms, shoudn't add to zero list
protected:  Base(NoZeroListEnum);
    // constructor for objects with explicit threading model
//#ifdef RENTAL_MODEL
//protected:  Base(RentalEnum);
//#endif
protected:  Base();

protected:  virtual ~Base();

public:     ULONG _addRef();
public:     ULONG _release();

public:     ULONG _qAddRef();
public:     ULONG _qRelease();

public:     virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);

private:    virtual Base * getBase() { return this; } 

public:     ULONG STDMETHODCALLTYPE AddRef() { return _addRef(); } 
public:     ULONG STDMETHODCALLTYPE Release() { return _release(); } 

public:     void weakAddRef();
public:     void weakRelease();

protected:  static Class * newClass(const TCHAR * name, Class * parent, CREATEOBJECT createObject, CREATEOBJECT cloneCreateObject = null);
#if DBG != 1
protected:  static Object * _cloningCreate() { Assert(FALSE && "Shouldn't be called"); return null; } 
#endif

protected:  ULONG_PTR _refs;

#if DBG == 1
protected:  DWORD _dwTID; // locked on which thread
#endif

#ifdef DEBUGRENTAL
public:     DWORD   _dwTIDCreated;
public:     long    _lModelCreated;
#endif

enum GCEnum 
{ 
    PerThreadGCFrequency = 2048,
    PerThreadGCAllocated = 1024 * 1024
};

private:    static LONG s_lZeroListCount;
            // last count rememberd in testForGC()
private:    static LONG s_lLastZeroListCount;
            // GC when there at least this number of objects on the zerolist
            // this is an approximate number of bytes for performance reasons
private:    static LONG s_lcbAllocated;
            // this is an approximate number of bytes at the last gc for performance reasons
private:    static LONG s_lcbLastAllocated;
            // this is an approximate count maintained with 'normal' increment/decrement
            // for performance reasons
private:    static LONG s_lGCFrequency;
            // amount of memory that can be allocated before gc
private:    static LONG s_lGCAllocated;
            // number of object which could be freed with a full gc
private:    static LONG s_lObjectsWaiting;
            // set when inside FreeObject so isMarked return false in this case
private:    static bool s_fInFreeObjects;
            // set to current thread when full GC is on 
private:    static TLSDATA * s_ptlsCheckZeroList;
            // set to current thread when GC is on (full or partial)
private:    static TLSDATA * s_ptlsGC;
public:     static TLSDATA * getTlsGC() { return s_ptlsGC; }

            // counter set when GC is on
private:    static LONG s_lInGC;
            // count of threads GC is waiting on in partial GC
private:    static LONG s_lRunning;
            // set when doing partial GC
private:    static bool s_fStartedPartialGC;
            // counter of the number of GC cycles
private:    static ULONG s_ulGCCycle;
public:     static ULONG getGCCycle() { return s_ulGCCycle; }
            // counter to help avoid having >1 threads trying to all enter the GC
private:    static LONG s_lGCTestEntryCount;

public: enum GCFlags
        {
            GC_FULL         = 1 << 1,   // do full gc on all stacks
            GC_STACKTOP     = 1 << 2,   // current thread is at the top of the stack
            GC_FORCE        = 1 << 3,   // do GC right now (will be full)
        };
public:     static void testForGC(DWORD dwFlags);
public:     static void reportObjects(LONG lObjects);
private:    static void checkZeroCountList(DWORD dwFlags);
private:    static void markStackObjects(INT_PTR * bottom, INT_PTR * top, BOOL fCurrentThread, TLSDATA * ptlsFrozen);
public:     ULONG_PTR tryLock();
#if DBG == 1
public:     ULONG_PTR spinLock() {return ::SpinLock(&_refs, this);}
public:     void unLock(ULONG_PTR l) {::SpinUnlock(&_refs, l, this);}
#else
public:     ULONG_PTR spinLock() {return ::SpinLock(&_refs);}
public:     void unLock(ULONG_PTR l) {::SpinUnlock(&_refs, l);}
#endif
private:    Base * addToZeroList(TLSDATA * ptlsdata);
private:    ULONG_PTR removeFromZeroList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata);
private:    int isMarked(ULONG_PTR refs);
#ifdef FAST_OBJECT_LIST
private:    void addToObjectList(TLSDATA * ptlsdata);
private:    static void flushToZeroList(TLSDATA * ptlsdata);
#endif
private:    static BOOL FreeObjects(TLSDATA * ptlsdata);
private:    static BOOL FreeObjects(class Hashtable *);
public:     static void StartFreeObjects();
public:     static void FinishFreeObjects();
private:    static void StartGC();
private:    static void FinishGC();
public:     static TLSDATA * StackEntryNormal();
public:     static void StackExitNormal(TLSDATA * ptlsdata);
private:    static TLSDATA * StackEntryBlocked();
private:    static void StackExitBlocked(TLSDATA * ptlsdata);
#ifdef RENTAL_MODEL
public:     BOOL isRental();
public:     RentalEnum model();
private:    void addToRentalList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata);
private:    void removeFromRentalList(ULONG_PTR lBaseRefs, TLSDATA * ptlsdata);
private:    static void freeRentalObjects(TLSDATA * ptlsdata, bool fCheckMarked);
#ifdef FASTRENTALGC
private:    static void partialFreeRentalObjects(TLSDATA * ptlsdata);
private:    static void markRentalStackObjects(INT_PTR * bottom, INT_PTR * top, Base ** apObjects);
#endif
#endif
};

class DLLEXPORT GenericBase : public Base
{
    DECLARE_CLASS_MEMBERS_NOQI(GenericBase, Base);
    DECLARE_CLASS_INSTANCE(GenericBase, Base);

protected:  GenericBase()
            {
                _allRefs = 1;
            }

            // constructor for atoms, shoudn't add to zero list
protected:  GenericBase(NoZeroListEnum) : super(NoZeroList) 
            {
                _allRefs = 1;
            }

    // constructor for objects with explicit threading model
//#ifdef RENTAL_MODEL
//protected:  GenericBase(RentalEnum reModel) : super(reModel)
            //{
                //_allRefs = 1;
            //}
//#endif

protected:  long _allRefs;    // weak reference count

public:     void _weakAddRef();
public:     void _weakRelease();

public:     void weakAddRef() { _weakAddRef(); } 
public:     void weakRelease() { _weakRelease(); } 

};

class NOVTABLE HashtableBase : public GenericBase
{
    friend class Base;

// We don't need anything beyond what GenericBase already defines...
//    DECLARE_CLASS_MEMBERS_NOQI(HashtableBase, GenericBase);
//    DECLARE_CLASS_INSTANCE(HashtableBase, GenericBase);

protected:  HashtableBase()
            : GenericBase(NoZeroList) 
            {
                warnGC();
            }

protected:  virtual void removeFromHashtable() = 0;

            // this marks this object as being unsafe to GC in the current GC cycle
protected:  void warnGC() { _ulCycle = Base::getGCCycle(); }

            // test if it is safe to
public:     bool wasGCWarned() const { return (_ulCycle == Base::getGCCycle()); }

    /**
     * The GCCycle count when this was allocated.
     */    
    private: ULONG _ulCycle;
    
};

#include "_array.hxx"

#define DEFINE_CLASS(C) \
        class C; \
        typedef _reference<##C> R##C; \
        typedef _readonlyreference<##C> RO##C; \
        typedef _weakreference<##C> W##C; \
        typedef _staticreference<##C> SR##C; \
        typedef _array<R##C> A##C; \
        typedef _reference<A##C> RA##C; \
        typedef _weakreference<A##C> WA##C; 

// COM interfaces are defined with struct so 
// DEFINE_CLASS can not be used with them.  Use
// DEFINE_STRUCT instead.  i.e. DEFINE_STRUCT(IStream)
// to define RIStream, WIStream, AIStream and RAIStream

#define DEFINE_STRUCT(C) \
        struct C; \
        typedef _reference<##C> R##C; \
        typedef _readonlyreference<##C> RO##C; \
        typedef _weakreference<##C> W##C; \
        typedef _array<R##C> A##C; \
        typedef _reference<A##C> RA##C; \
        typedef _weakreference<A##C> WA##C; 


//
// Casting macros
//
// SAFE_CAST - use this to safely convert one type to another. It will do a 
//      dynamic cast in the debug build and an ordinary cast in the retail 
//      build. SAFE_CAST can be used with any types.
//
// CAST_TO and ICAST_TO are for use with objects derived from Object * and Base *
//
// CAST_TO - use this to safely convert from one Object * to another Object *. For
//      example, when enumerating the children of an Element, the enumeration returns
//      Object * which are really Element *. Use CAST_TO to convert from Object * to
//      Element *.
//
// ICAST_TO - use this to safely convert from an interface object * to an Base *.  For
//      example, when enumerating the children of an Element, the enumeration returns
//      Object * which are really Element *. If you know these are really ElementImpl *
//      then use ICAST_TO to convert the Object * to ElementImpl *.
//
//

#ifdef UNIX
#define SAFE_CAST(to, from) (static_cast<to>(from))
#define CAST_TO(C, o) (SAFE_CAST(C, SAFE_CAST(Base *, o)))   
#else
#if DBG==1
#define SAFE_CAST(to, from) (dynamic_cast<to>(from))
template <class T>
inline T  cast_to(Object * o)
{
    T  dynamicCast = SAFE_CAST(T, SAFE_CAST(Base *, o));
    T  staticCast = (T ) (Base *) o;
    Assert(dynamicCast == staticCast && "Invalid cast from interface to object use ICAST_TO instead");
    return dynamicCast;
}
#define CAST_TO(C, o) cast_to<C>(o)   
#else
#define SAFE_CAST(to, from) (static_cast<to>(from))
#define CAST_TO(C, o) (SAFE_CAST(C, SAFE_CAST(Base *, o)))   
#endif
#endif /* UNIX */

#define ICAST_TO(C, o) (SAFE_CAST(C, o->getBase()))
#define A(C) A##C

#if DBG==1
Base * LockingIsObject(void *p);
#endif

#ifdef SPECIAL_OBJECT_ALLOCATION

#define REGION_GRANULARITY_BITS 16

#ifdef _WIN64
// WIN64 BUGBUG
// With the current 64 bit compiler/linker I need to do this otherwise g_abRegion is unresolved
extern "C"{
#endif 
	extern BYTE g_abRegion[];
#ifdef _WIN64
}
#endif 

bool inline isObjectRegion(void * p)
{
    UINT_PTR q = ((UINT_PTR)p) >> REGION_GRANULARITY_BITS;
    return (g_abRegion[q >> 3] & (1 << (q & 7))) != 0;
}
void addObjectRegion(void * pRegion, long lSize);
void delObjectRegion(void * pRegion, long lSize);

#endif // SPECIAL_OBJECT_ALLOCATION

 
#endif _BASE_HXX
