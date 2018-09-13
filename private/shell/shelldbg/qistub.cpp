//***   qistub.cpp -- QI helpers (retail and debug)
// DESCRIPTION
//  this file has the shared-source 'master' implementation.  it is
// #included in each DLL that uses it.
//  clients do something like:
//      #include "priv.h"   // for types, ASSERT, DM_*, DF_*, etc.
//      #include "../lib/qistub.cpp"

#define DM_MISC2            0       // misc stuff (verbose)
#include "priv.h"

#ifdef DEBUG // {
//***   CUniqueTab {
// DESCRIPTION
//  key/data table insert and lookup, w/ interlock.
class CUniqueTab
{
public:
    void * Add(int val);
    void * Find(int val, int delta);
    void Reset(void);

    // n.b. *not* protected
    CUniqueTab(int cbElt);
    virtual ~CUniqueTab();

protected:

private:
    void _Lock(void) { EnterCriticalSection(&_hLock); }
    void _Unlock(void) { LeaveCriticalSection(&_hLock); }

    CRITICAL_SECTION    _hLock;
// key + (arbitrary) limit of 4 int's of client data
#define CUT_CBELTMAX    (SIZEOF(int) + 4 * SIZEOF(int))
    int     _cbElt;                 // size of an entry (key + data)
// (arbitrary) limit to catch clients running amuck
#define CUT_CVALMAX 256         // actually, a LIM not a MAX
    HDSA    _hValTab;
};

CUniqueTab::CUniqueTab(int cbElt)
{
    InitializeCriticalSection(&_hLock);
    ASSERT(cbElt >= SIZEOF(DWORD));     // need at least a key; data optional
    _cbElt = cbElt;
    _hValTab = DSA_Create(_cbElt, 4);
    return;
}

CUniqueTab::~CUniqueTab()
{
    DSA_Destroy(_hValTab);
    DeleteCriticalSection(&_hLock);
    return;
}

struct cutent {
    int iKey;
    char bData[CUT_CBELTMAX - SIZEOF(int)];
};
struct cfinddata {
    int iKey;
    int dRange;
    void *pEntry;
};

int _UTFindCallback(void *pEnt, void *pData)
{
#define INFUNC(base, p, range) ((base) <= (p) && (p) <= (base) + (range))
    struct cfinddata *pcd = (struct cfinddata *)pData;
    if (INFUNC(*(int *)pEnt, pcd->iKey, pcd->dRange)) {
        pcd->pEntry = pEnt;
        return 0;
    }
    return 1;
#undef  INFUNC
}

//***   CUniqueTab::Add -- add entry if not already there
//
void * CUniqueTab::Add(int val)
{
    struct cfinddata cd = { val, 0, NULL };

    _Lock();

    DSA_EnumCallback(_hValTab, _UTFindCallback, &cd);
    if (!cd.pEntry) {
        int i;
        // lazy,lazy,lazy: alloc max size and let DSA_AppendItem sort it out
        struct cutent elt = { val, 0 /*,0,...,0*/ };

        TraceMsg(DM_MISC2, "cut.add: add %x", val);
        if (DSA_GetItemCount(_hValTab) <= CUT_CVALMAX) {
            i = DSA_AppendItem(_hValTab, &elt);
            cd.pEntry = DSA_GetItemPtr(_hValTab, i);
        }
    }

    _Unlock();

    return cd.pEntry;
}

//***   CUniqueTab::Find -- find entry
//
void * CUniqueTab::Find(int val, int delta)
{
    struct cfinddata cd = { val, delta, NULL };

    DSA_EnumCallback(_hValTab, _UTFindCallback, &cd);
    if (cd.pEntry) {
        // TODO: add p->data[0] dump
        TraceMsg(DM_MISC2, "cut.find: found %x+%d", val, delta);
    }
    return cd.pEntry;
}

//***   _UTResetCallback -- helper for CUniqueTab::Reset
int _UTResetCallback(void *pEnt, void *pData)
{
    struct cutent *pce = (struct cutent *)pEnt;
    int cbEnt = *(int *)pData;
    // perf: could move the SIZEOF(int) into caller, but seems safer here
    memset(pce->bData, 0, cbEnt - SIZEOF(int));
    return 1;
}

//***   Reset -- clear 'data' part of all entries
//
void CUniqueTab::Reset(void)
{
    if (EVAL(_cbElt > SIZEOF(int))) {
        _Lock();
        DSA_EnumCallback(_hValTab, _UTResetCallback, &_cbElt);
        _Unlock();
    }
    return;
}
// }
#endif // }

//***   QueryInterface helpers {

//***   FAST_IsEqualIID -- fast compare
// (cast to 'LONG_PTR' so don't get overloaded ==)
#define FAST_IsEqualIID(piid1, piid2)   ((LONG_PTR)(piid1) == (LONG_PTR)(piid2))

#ifdef DEBUG // {
//***   DBNoOp -- do nothing (but suppress compiler optimizations)
// NOTES
//  this won't fool compiler when it gets smarter, oh well...
void DBNoOp()
{
    return;
}

void DBBrkpt()
{
    DBNoOp();
    return;
}

//***   DBBreakGUID -- debug hook (gets readable name, allows brkpt on IID)
// DESCRIPTION
//  search for 'BRKPT' for various hooks.
//  patch 'DBQIiid' to brkpt on a specific iface
//  patch 'DBQIiSeq' to brkpt on Nth QI of specific iface
//  brkpt on interesting events noted below
// NOTES
//  warning: returns ptr to *static* buffer!

typedef enum {
    DBBRK_NIL   = 0,
    DBBRK_ENTER = 0x01,
    DBBRK_TRACE = 0x02,
    DBBRK_S_XXX = 0x04,
    DBBRK_E_XXX = 0x08,
    DBBRK_BRKPT = 0x10,
} DBBRK;

DBBRK DBQIuTrace = DBBRK_NIL;   // BRKPT patch to enable brkpt'ing
GUID *DBQIiid = NULL;           // BRKPT patch to brkpt on iface
int DBQIiSeq = -1;              // BRKPT patch to brkpt on Nth QI of DBQIiid
long DBQIfReset = FALSE;        // BRKPT patch to reset counters

TCHAR *DBBreakGUID(const GUID *piid, DBBRK brkCmd)
{
    static TCHAR szClass[GUIDSTR_MAX];

    SHStringFromGUID(*piid, szClass, ARRAYSIZE(szClass));

    // BUGBUG fold these 2 if's together
    if ((DBQIuTrace & brkCmd) &&
      (DBQIiid == NULL || IsEqualIID(*piid, *DBQIiid))) {
        TraceMsg(DM_TRACE, "util: DBBreakGUID brkCmd=%x clsid=%s (%s)", brkCmd, szClass, Dbg_GetREFIIDName(*piid));
        // BRKPT put brkpt here to brkpt on 'brkCmd' event
        DBBrkpt();
    }

    if (DBQIiid != NULL && IsEqualIID(*piid, *DBQIiid)) {
        //TraceMsg(DM_TRACE, "util: DBBreakGUID clsid=%s (%s)", szClass, Dbg_GetREFIIDName(*piid));
        if (brkCmd != DBBRK_TRACE) {
            // BRKPT put brkpt here to brkpt on 'DBQIiid' iface
            DBNoOp();
        }
    }

    // BRKPT put your brkpt(s) here for various events
    switch (brkCmd) {
    case DBBRK_ENTER:
        // QI called w/ this iface
        DBNoOp();
        break;
    case DBBRK_TRACE:
        // looped over this iface
        DBNoOp();
        break;
    case DBBRK_S_XXX:
        // successful QI for this iface
        DBNoOp();
        break;
    case DBBRK_E_XXX:
        // failed QI for this iface
        DBNoOp();
        break;
    case DBBRK_BRKPT:
        // various brkpt events, see backtrace to figure out which one
        DBNoOp();
        break;
    }

    return szClass;
}
#endif // }

#ifdef DEBUG
CUniqueTab *DBpQIFuncTab;

STDAPI_(BOOL) DBIsQIFunc(int ret, int delta)
{
    BOOL fRet = FALSE;

    if (DBpQIFuncTab)
        fRet = (BOOL) DBpQIFuncTab->Find(ret, delta);

    return fRet;
}
#endif

//***   QISearch -- table-driven QI
// ENTRY/EXIT
//  this        IUnknown* of calling QI
//  pqit        QI table of IID,cast_offset pairs
//  ppv         the usual
//  hr          the usual S_OK/E_NOINTERFACE, plus other E_* for errors
LPVOID QIStub_CreateInstance(void* that, IUnknown* punk, REFIID riid);	// qistub.cpp

STDAPI QISearch(void* that, LPCQITAB pqitab, REFIID riid, LPVOID* ppv)
{
    // do *not* move this!!! (must be 1st on frame)
#ifdef DEBUG
#if (_X86_)
    int var0;       // *must* be 1st on frame
#endif
#endif

    LPCQITAB pqit;
#ifdef DEBUG
    TCHAR *pst;

#if ( _X86_) // QIStub only works for X86
    if (IsFlagSet(g_dwDumpFlags, DF_DEBUGQI)) {
        if (DBpQIFuncTab == NULL)
            DBpQIFuncTab = new CUniqueTab(SIZEOF(DWORD));   // LONG_PTR?
        if (DBpQIFuncTab) {
            int n;
            int fp = (int) (1 + (int *)&var0);
            struct DBstkback sbtab[1] = { 0 };
            n = DBGetStackBack(&fp, sbtab, ARRAYSIZE(sbtab));
            DBpQIFuncTab->Add(sbtab[n - 1].ret);
        }
    }
#endif

    if (DBQIuTrace)
        pst = DBBreakGUID(&riid, DBBRK_ENTER);
#endif

    if (ppv == NULL)
        return E_POINTER;

    if (FAST_IsEqualIID(&riid, &IID_IUnknown)
      || IsEqualIID(riid, IID_IUnknown)) {
        // just use 1st table entry
        pqit = pqitab;

LhitUnknown:
        IUnknown* punk = (IUnknown*)((LONG_PTR)that + pqit->dwOffset);
#ifdef DEBUG
        pst = DBBreakGUID(&riid, DBBRK_S_XXX);
#endif
        punk->AddRef();
        *ppv = punk;
        return S_OK;
    }
    else {
        // 1st try the fast way
        for (pqit = pqitab; pqit->piid != NULL; pqit++) {
#ifdef DEBUG
            if (DBQIuTrace)
                pst = DBBreakGUID(pqit->piid, DBBRK_TRACE);
#endif
            if (FAST_IsEqualIID(&riid, pqit->piid))
                goto Lhit;
        }

        // no luck, try the hard way
        for (pqit = pqitab; pqit->piid != NULL; pqit++) {
#ifdef DEBUG
            if (DBQIuTrace)
                pst = DBBreakGUID(pqit->piid, DBBRK_TRACE);
#endif
            if (IsEqualIID(riid, *(pqit->piid)))
            {
Lhit:
#ifdef DEBUG
#if ( _X86_) // QIStub only works for X86
                if (IsFlagSet(g_dwDumpFlags, DF_DEBUGQI))
                {
                    IUnknown* punk = (IUnknown*)((LONG_PTR)that + pqit->dwOffset);
                    *ppv = QIStub_CreateInstance(that, punk, riid);
                    if (*ppv)
                    {
                        pst = DBBreakGUID(&riid, DBBRK_S_XXX);
                        return S_OK;
                    }
                }
#endif
#endif
                goto LhitUnknown;
            }
        }

#ifdef DEBUG
        if (DBQIuTrace)
            pst = DBBreakGUID(&riid, DBBRK_E_XXX);
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    /*NOTREACHED*/
}

// }

#ifdef DEBUG // {
#if ( _X86_) // { QIStub only works for X86

//***   QIStub helpers {

class CQIStub
{
public:
    virtual void thunk0();
    // BUGBUG should AddRef/Release up _iSeq?
    virtual STDMETHODIMP_(ULONG) AddRef(void)
        { _cRef++; return _cRef; }
    virtual STDMETHODIMP_(ULONG) Release(void)
        { _cRef--; if (_cRef>0) return _cRef; delete this; return 0; }
    virtual void thunk3();
    virtual void thunk4();
    virtual void thunk5();
    virtual void thunk6();
    virtual void thunk7();
    virtual void thunk8();
    virtual void thunk9();
    virtual void thunk10();
    virtual void thunk11();
    virtual void thunk12();
    virtual void thunk13();
    virtual void thunk14();
    virtual void thunk15();
    virtual void thunk16();
    virtual void thunk17();
    virtual void thunk18();
    virtual void thunk19();
    virtual void thunk20();
    virtual void thunk21();
    virtual void thunk22();
    virtual void thunk23();
    virtual void thunk24();
    virtual void thunk25();
    virtual void thunk26();
    virtual void thunk27();
    virtual void thunk28();
    virtual void thunk29();
    virtual void thunk30();
    virtual void thunk31();
    virtual void thunk32();
    virtual void thunk33();
    virtual void thunk34();
    virtual void thunk35();
    virtual void thunk36();
    virtual void thunk37();
    virtual void thunk38();
    virtual void thunk39();
    virtual void thunk40();
    virtual void thunk41();
    virtual void thunk42();
    virtual void thunk43();
    virtual void thunk44();
    virtual void thunk45();
    virtual void thunk46();
    virtual void thunk47();
    virtual void thunk48();
    virtual void thunk49();
    virtual void thunk50();
    virtual void thunk51();
    virtual void thunk52();
    virtual void thunk53();
    virtual void thunk54();
    virtual void thunk55();
    virtual void thunk56();
    virtual void thunk57();
    virtual void thunk58();
    virtual void thunk59();
    virtual void thunk60();
    virtual void thunk61();
    virtual void thunk62();
    virtual void thunk63();
    virtual void thunk64();
    virtual void thunk65();
    virtual void thunk66();
    virtual void thunk67();
    virtual void thunk68();
    virtual void thunk69();
    virtual void thunk70();
    virtual void thunk71();
    virtual void thunk72();
    virtual void thunk73();
    virtual void thunk74();
    virtual void thunk75();
    virtual void thunk76();
    virtual void thunk77();
    virtual void thunk78();
    virtual void thunk79();
    virtual void thunk80();
    virtual void thunk81();
    virtual void thunk82();
    virtual void thunk83();
    virtual void thunk84();
    virtual void thunk85();
    virtual void thunk86();
    virtual void thunk87();
    virtual void thunk88();
    virtual void thunk89();
    virtual void thunk90();
    virtual void thunk91();
    virtual void thunk92();
    virtual void thunk93();
    virtual void thunk94();
    virtual void thunk95();
    virtual void thunk96();
    virtual void thunk97();
    virtual void thunk98();
    virtual void thunk99();

protected:
    CQIStub(void *that, IUnknown* punk, REFIID riid);
    friend LPVOID QIStub_CreateInstance(void *that, IUnknown* punk, REFIID riid);
    friend BOOL __stdcall DBIsQIStub(void *that);
    friend void __stdcall DBDumpQIStub(void *that);
    friend TCHAR *DBGetQIStubSymbolic(void *that);

private:
    ~CQIStub();

    static void *_sar;              // C (not C++) ptr to CQIStub::AddRef

    int       _cRef;
    IUnknown* _punk;                // vtable we hand off to
    void*     _that;                // "this" pointer of object we stub (for reference)
    IUnknown* _punkRef;             // "punk" (for reference)
    REFIID    _riid;                // iid of interface (for reference)
    int       _iSeq;                // sequence #
    TCHAR     _szName[GUIDSTR_MAX]; // legible name of interface (for reference)
};

struct DBQISeq
{
    GUID *  pIid;
    int     iSeq;
};
//CASSERT(SIZEOF(GUID *) == SIZEOF(DWORD));   // CUniqueTab uses DWORD's

// BUGBUG todo: _declspec(thread)
CUniqueTab * DBpQISeqTab = NULL;

extern "C" void *Dbg_GetREFIIDAtom(REFIID riid);    // lib/dump.c (priv.h?)

//***
// NOTES
//  there's actually a race condition here -- another thread can come in
// and do seq++, then we do the reset, etc. -- but the assumption is that
// the developer has set the flag in a scenario where this isn't an issue.
void DBQIReset(void)
{
    ASSERT(!DBQIfReset);    // caller should do test-and-clear
    if (DBpQISeqTab)
        DBpQISeqTab->Reset();

    return;
}

void *DBGetVtblEnt(void *that, int i);
#define VFUNC_ADDREF  1     // AddRef is vtbl[1]

void * CQIStub::_sar = NULL;

CQIStub::CQIStub(void* that, IUnknown* punk, REFIID riid) : _cRef(1), _riid(riid)
{
    _that = that;

    _punk = punk;
    if (_punk)
        _punk->AddRef();

    _punkRef = _punk; // for reference, so don't AddRef it!

    // c++ won't let me get &CQIStub::AddRef as a 'real' ptr (!@#$),
    // so we need to get it the hard way, viz. new'ing an object which
    // we know inherits it.
    if (_sar == NULL) {
        _sar = DBGetVtblEnt((void *)this, VFUNC_ADDREF);
        ASSERT(_sar != NULL);
    }

    lstrcpyn(_szName, Dbg_GetREFIIDName(riid), ARRAYSIZE(_szName));

    // generate sequence #
    if (DBpQISeqTab == NULL)
        DBpQISeqTab = new CUniqueTab(SIZEOF(struct DBQISeq));
    if (DBpQISeqTab) {
        struct DBQISeq *pqiseq;

        if (InterlockedExchange(&DBQIfReset, FALSE))
            DBQIReset();

        pqiseq = (struct DBQISeq *) DBpQISeqTab->Add((DWORD) Dbg_GetREFIIDAtom(riid));
        if (EVAL(pqiseq))       // (might fail on table overflow)
            _iSeq = pqiseq->iSeq++;
    }

    TraceMsg(TF_QISTUB, "ctor QIStub %s seq=%d (that=%x punk=%x) %x", _szName, _iSeq, _that, _punk, this);
}

CQIStub::~CQIStub()
{
    TraceMsg(TF_QISTUB, "dtor QIStub %s (that=%x punk=%x) %x", _szName, _that, _punk, this);

    ATOMICRELEASE(_punk);
}

LPVOID QIStub_CreateInstance(void* that, IUnknown* punk, REFIID riid)
{
    CQIStub* pThis = new CQIStub(that, punk, riid);

    if (DBQIiSeq == pThis->_iSeq && IsEqualIID(riid, *DBQIiid)) {
        TCHAR *pst;
        // BRKPT put brkpt here to brkpt on seq#'th call to 'DBQIiid' iface
        pst = DBBreakGUID(&riid, DBBRK_BRKPT);
    }

    return(pThis);
}

//***   DBGetVtblEnt -- get vtable entry
// NOTES
//  always uses 1st vtbl (so MI won't work...).
void *DBGetVtblEnt(void *that, int i)
{
    void **vptr;
    void *pfunc;

    __try {
        vptr = (void **) *(void **) that;
        pfunc = (vptr == 0) ? 0 : vptr[i];
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        // since we're called from the DebMemLeak, we're only *guessing*
        // that we have a vptr etc., so we might fault.
        TraceMsg(TF_ALWAYS, "gve: GPF");
        pfunc = 0;
    }

    return pfunc;
}

//***   DBIsQIStub -- is 'this' a ptr to a 'CQIStub' object?
// DESCRIPTION
//  we look at the vtbl and assume that if we have a ptr to CQIStub::AddRef,
// then it's us.
// NOTES
//  M00BUG we do a 'new' in here, which can cause pblms if we're in the middle
// of intelli-leak and we end up doing a ReAlloc which moves the heap (raymondc
// found such a case).
//  M00BUG in a release build (w/ identical COMDAT folding) we'll get false
// hits since most/all AddRefs are identical and folded.  if we ever need to
// be more exact we can add a signature and key off that.
//  M00BUG hack hack we actually return a void *, just in case you want to
// know the 'real' object.  if that turns out to be useful, we should change
// to return a void * instead of a BOOL.

BOOL DBIsQIStub(void* that)
{
    void *par;

#if 0
    if (_sar == NULL)
        TraceMsg(DM_TRACE, "qis: _sar == NULL");
#endif

    par = DBGetVtblEnt(that, VFUNC_ADDREF);

#if 0
    TraceMsg(TF_ALWAYS, "IsQIStub(%x): par=%x _sar=%x", that, _sar, par);
#endif

    return (CQIStub::_sar == par && CQIStub::_sar != NULL) ? (BOOL)((CQIStub *)that)->_punk : 0;
#undef  VFUNC_ADDREF
}

TCHAR *DBGetQIStubSymbolic(void* that)
{
    class CQIStub *pqis = (CQIStub *) that;
    return pqis->_szName;
}

//***   DBDumpQIStub -- pretty-print a 'CQIStub'
//
STDAPI_(void) DBDumpQIStub(void* that)
{
    class CQIStub *pqis = (CQIStub *) that;
    TraceMsg(TF_ALWAYS, "\tqistub(%x): cRef=0x%x iSeq=%x iid=%s", that, pqis->_cRef, pqis->_iSeq, pqis->_szName);
}

// Memory layout of CQIStub is:
//    lpVtbl  // offset 0
//    _cRef   // offset 4
//    _punk   // offset 8
//
// "this" pointer stored in stack
//
// mov eax, ss:4[esp]          ; get pThis
// mov ecx, 8[eax]             ; get real object (_punk)
// mov eax, [ecx]              ; load the real vtable (_punk->lpVtbl)
//                             ; the above will fault if referenced after we're freed
// mov ss:4[esp], ecx          ; fix up stack object (_punk)
// jmp dword ptr cs:(4*i)[eax] ; jump to the real function
//
#define QIStubThunk(i) \
void _declspec(naked) CQIStub::thunk##i() \
{ \
    _asm mov eax, ss:4[esp]          \
    _asm mov ecx, 8[eax]             \
    _asm mov eax, [ecx]              \
    _asm mov ss:4[esp], ecx          \
    _asm jmp dword ptr cs:(4*i)[eax] \
}

QIStubThunk(0);
QIStubThunk(3);
QIStubThunk(4);
QIStubThunk(5);
QIStubThunk(6);
QIStubThunk(7);
QIStubThunk(8);
QIStubThunk(9);
QIStubThunk(10);
QIStubThunk(11);
QIStubThunk(12);
QIStubThunk(13);
QIStubThunk(14);
QIStubThunk(15);
QIStubThunk(16);
QIStubThunk(17);
QIStubThunk(18);
QIStubThunk(19);
QIStubThunk(20);
QIStubThunk(21);
QIStubThunk(22);
QIStubThunk(23);
QIStubThunk(24);
QIStubThunk(25);
QIStubThunk(26);
QIStubThunk(27);
QIStubThunk(28);
QIStubThunk(29);
QIStubThunk(30);
QIStubThunk(31);
QIStubThunk(32);
QIStubThunk(33);
QIStubThunk(34);
QIStubThunk(35);
QIStubThunk(36);
QIStubThunk(37);
QIStubThunk(38);
QIStubThunk(39);
QIStubThunk(40);
QIStubThunk(41);
QIStubThunk(42);
QIStubThunk(43);
QIStubThunk(44);
QIStubThunk(45);
QIStubThunk(46);
QIStubThunk(47);
QIStubThunk(48);
QIStubThunk(49);
QIStubThunk(50);
QIStubThunk(51);
QIStubThunk(52);
QIStubThunk(53);
QIStubThunk(54);
QIStubThunk(55);
QIStubThunk(56);
QIStubThunk(57);
QIStubThunk(58);
QIStubThunk(59);
QIStubThunk(60);
QIStubThunk(61);
QIStubThunk(62);
QIStubThunk(63);
QIStubThunk(64);
QIStubThunk(65);
QIStubThunk(66);
QIStubThunk(67);
QIStubThunk(68);
QIStubThunk(69);
QIStubThunk(70);
QIStubThunk(71);
QIStubThunk(72);
QIStubThunk(73);
QIStubThunk(74);
QIStubThunk(75);
QIStubThunk(76);
QIStubThunk(77);
QIStubThunk(78);
QIStubThunk(79);
QIStubThunk(80);
QIStubThunk(81);
QIStubThunk(82);
QIStubThunk(83);
QIStubThunk(84);
QIStubThunk(85);
QIStubThunk(86);
QIStubThunk(87);
QIStubThunk(88);
QIStubThunk(89);
QIStubThunk(90);
QIStubThunk(91);
QIStubThunk(92);
QIStubThunk(93);
QIStubThunk(94);
QIStubThunk(95);
QIStubThunk(96);
QIStubThunk(97);
QIStubThunk(98);
QIStubThunk(99);

// }

#endif // }
#endif // }
