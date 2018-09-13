/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef TLS_H
#define TLS_H

// this file defines our structure we keep for every thread
// these are linked together se we can walk all the threads using
// our objects

class Base;
class Exception;
class Object;

typedef struct TLSDATA * (* PFN_ENTRY)();
typedef void (* PFN_EXIT)(struct TLSDATA *);

extern PFN_ENTRY g_pfnEntry;
extern PFN_EXIT g_pfnExit;

#ifdef RENTAL_MODEL
enum RentalEnum { Rental, MultiThread };
#endif

struct TLSDATA
{
    typedef void (*_pfn)();

    struct BASE
    {
        _pfn *  _pfnvtable;
        ULONG_PTR _refs;
#if DBG == 1
        DWORD           _dwTID;             // thread id
#endif
    };
    
    BASE            _baseHead;
    BASE            _baseHeadLocked;
    HANDLE          _hThread;           // handle to thread owning this structure
    TLSDATA *       _pNext;             // next TLS structure
    bool            _fLocked;           // set when locked during GC
    bool            _fCounted;          // set when counted during startGC()
    bool            _fSuspended;        // set during unmarking pointers on stack
#ifdef RENTAL_MODEL
    bool            _fReleaseRental;    // set when releasing rental objects
    bool            _fCheckMarked;      // signal to check REF_MARKED when releasing rental objects
    bool            _fMisAligned;       // mark when we have to adjust for 8 byte alignment on Win95
    Base *          _pRentalList;       // list of rental objects with zero ref count
    unsigned        _uRentals;          // number of objects on rental list
    RentalEnum      _reModel;           // current model to create objects with 

#ifdef FASTRENTALGC
    // not used yet
    #define SIZEOFRENTALGCLIST 2048
    Base *          _apZLObjects[SIZEOFRENTALGCLIST];
    bool            _fPartialRental;    // set when releasing rental objects
#endif
#endif
    int             _iRunning;          // greater than zero when our code is running
    HRESULT         _hrException;       // exception handling error code
    Exception *     _pException;        // last exception object
    struct TEB *    _pTEB;              // pointer to TEB structure
    DWORD           _dwTID;             // thread id
    bool            _fThreadExited;     // we got a DLL_THREAD_DETACH on this thread.
    DWORD           _dwDepth;           // depth of recursion of finalize() during GC
#if DBG == 1
    DWORD           _dwTemp1;
    DWORD           _dwTemp2;
#endif
	// WIN64 REVIEW - Does this really need to be a DWORD_PTR??
    DWORD			_dwHeapHint;        // index for heap manager
    void *          _pPageLocked;       // set when locking a page for freeing objects

#ifdef FAST_OBJECT_LIST
    #define SIZEOFOBJECTLIST 256

    unsigned        _uObjects;
    unsigned        _uNextObject;
    Base *          _ppObjects[SIZEOFOBJECTLIST];
#endif

    TLSDATA();
    ~TLSDATA();

    void init();
    void reinit();

    void * operator new(size_t);
    void operator delete(void *);
};

extern DWORD g_dwTlsIndex;
extern DWORD g_dwPlatformId;

#ifdef RENTAL_MODEL
class Model
{
    public: Model(TLSDATA * ptlsdata, RentalEnum re);
    public: Model(RentalEnum re);
    public: Model(TLSDATA * ptlsdata, Base * pBase);
    public: Model(TLSDATA * ptlsdata, Object * pObject);
    public: ~Model();
    private: void init(TLSDATA * ptlsdata, RentalEnum re);
    public: void Release();
	
    private: TLSDATA * _ptlsdata;
    private: RentalEnum _reSavedModel;
    public: RentalEnum _reModel;
};
#endif

DLLEXPORT DWORD GetTlsIndex();
TLSDATA * AllocTlsData();

#pragma warning ( disable : 4035 )
__inline TLSDATA * EnsureTlsData()
{
#if 0 //_X86_
    __asm mov         eax,fs:[0x00000018]
    __asm mov         edx,g_dwTlsIndex
    __asm mov         eax,dword ptr [eax+edx*4+0x0E10]
    __asm or          eax, eax
    __asm jnz         ok
    __asm call        AllocTlsData
    __asm ok:
#else
    TLSDATA * ptlsdata = (TLSDATA *)TlsGetValue(g_dwTlsIndex);
    if (!ptlsdata)
    {
        ptlsdata = AllocTlsData();
    }
    return ptlsdata;
#endif
}
#pragma warning ( default : 4035 )

#pragma warning ( disable : 4035 )
__inline TLSDATA * GetTlsData()
{
    Assert(TlsGetValue(g_dwTlsIndex) && "SHOULD ALWAYS BE PRESENT");
#if 0 //_X86_
    __asm mov         eax,fs:[0x00000018]
    __asm mov         edx,g_dwTlsIndex
    __asm mov         dword ptr [eax+0x34],0
    __asm mov         eax,dword ptr [eax+edx*4+0x0E10]
#else
    TLSDATA * ptlsdata = (TLSDATA *)TlsGetValue(g_dwTlsIndex);
    Assert(ptlsdata && "SHOULD ALWAYS BE PRESENT");
    return ptlsdata;
#endif
}
#pragma warning ( default : 4035 )

// to set TLS up
class EnsureTls
{
public: EnsureTls()
        {
            _tlsdata = (*g_pfnEntry)();
        }

        ~EnsureTls()
        {
            if (_tlsdata)
            {
                _tlsdata->_iRunning--;
                (*g_pfnExit)(_tlsdata);
            }
        }


        TLSDATA * getTlsData() 
        {
            return _tlsdata;
        }

private:    TLSDATA * _tlsdata;
};

#define STACK_ENTRY EnsureTls _EnsureTls; if (!_EnsureTls.getTlsData()) return E_FAIL

#ifdef RENTAL_MODEL
#define STACK_ENTRY_WRAPPED STACK_ENTRY; Model model(_EnsureTls.getTlsData(), getWrapped())
#define STACK_ENTRY_OBJECT(o) STACK_ENTRY; Model model(_EnsureTls.getTlsData(), o)
#define STACK_ENTRY_IUNKNOWN(i) STACK_ENTRY; Model model(_EnsureTls.getTlsData(), i->model())
#define STACK_ENTRY_MODEL(e) STACK_ENTRY; Model model(_EnsureTls.getTlsData(), e)
#else
#define STACK_ENTRY_WRAPPED STACK_ENTRY
#define STACK_ENTRY_OBJECT(o) STACK_ENTRY
#define STACK_ENTRY_IUNKNOWN(i) STACK_ENTRY
#define STACK_ENTRY_MODEL(e) STACK_ENTRY
#endif

#endif TLS_H
