//***   CUACount -- user-assistance counter w/ decay
// NOTES
//  todo: scavenging to clean out registry.  but see caveats in UAC_CDEF.

#include "priv.h"
#include "uacount.h"
#include "uareg.h"

#define DM_UEMTRACE     TF_UEM

#define MAX(a, b)   (((a) > (b)) ? (a) : (b))

//***   UAC_CDEFAULT -- initial _cCnt for entry (we *always* show items)
// NOTES
//  eventually we might want to scavenge all entries, decaying them down
// and deleting any that decay to 0.  note however that this will cause
// them to look like they have a default count of 1 (see CUAC::Init), so
// they'll suddenly appear on the menus again.
#define UAC_CDEFAULT    0       // initial _cCnt for entry

#define SID_SDEFAULT    SID_SNOWREAD    // initial _sidMru for new entry


//***
// NOTES
//  BUGBUG it's getting to the point that we should disallow stack-alloc'ed
// guys and instead count on new() to 0-init us.
CUACount::CUACount()
{
    // Since this is created on the stack, we don't get the benefits of the 
    // Heap allocator's zero initialization...
    ZeroMemory(_GetRawData(), _GetRawCount());

    _fInited = FALSE;   // need to call Initialize
#if XXX_VERSIONED
    _cbSize = -1;
#endif
#if XXX_DELETE
    _fInherited = FALSE;
#endif
    _fDirty = FALSE;
    _fNoDecay = _fNoPurge = FALSE;

    return;
}

#ifdef DEBUG
BOOL CUACount::DBIsInit()
{
#if XXX_VERSIONED
    ASSERT((_cbSize == SIZEOF(SUACount)) == BOOLIFY(_fInited));
#endif
    return _fInited;
}
#endif

HRESULT CUACount::Initialize(IUASession *puas)
{
    _puas = puas;
    if (!_fInited) {
        _fInited = TRUE;
#if XXX_VERSIONED
        // todo: _cbSize -1 means no entry, < SIZEOF means version upgrade
        _cbSize = SIZEOF(SUACount);
#endif
        // hardcode the SZ_CUACount_ctor values here
        _cCnt = UAC_CDEFAULT;       // all items start out visible
        _sidMruDisk = SID_SNOWREAD; // ... and non-aged
    }

    _sidMru = _sidMruDisk;
    if (ISSID_SSPECIAL(_sidMruDisk)) {
        _sidMru = _ExpandSpecial(_sidMruDisk);
        if (_sidMruDisk == SID_SNOWINIT) {
            _sidMruDisk = _sidMru;
            _fDirty = TRUE;
        }
        else if (_sidMruDisk == SID_SNOWREAD) {
            _sidMruDisk = _sidMru;
            ASSERT(!_fDirty);
        }
    }

    return S_OK;
}

HRESULT CUACount::LoadFrom(PFNNRW3 pfnIO, PNRWINFO pRwi)
{
    HRESULT hr;

    hr = (*pfnIO->_pfnRead)(_GetRawData(), _GetRawCount(), pRwi);
    if (SUCCEEDED(hr))
        _fInited = TRUE;
    return hr;
}

HRESULT CUACount::SaveTo(BOOL fForce, PFNNRW3 pfnIO, PNRWINFO pRwi)
{
    HRESULT hr;

    hr = S_FALSE;
    if (fForce || _fDirty) {
        if (!ISSID_SSPECIAL(_sidMruDisk)) 
            _sidMruDisk = _sidMru;
#if XXX_DELETE
        if (_cCnt == 0 && !_fNoPurge && pfnIO->_pfnDelete)
            hr = (*pfnIO->_pfnDelete)(_GetRawData(), _GetRawCount(), pRwi);
        else
#endif
        hr = (*pfnIO->_pfnWrite)(_GetRawData(), _GetRawCount(), pRwi);
        ASSERT(SUCCEEDED(hr));
        _fDirty = FALSE;
    }
    return hr;
}

//***   GetCount -- get count info (w/ lazy decay)
//
int CUACount::GetCount()
{
    ASSERT(DBIsInit());

    int cCnt = _DecayCount(FALSE);

    return cCnt;
}

void CUACount::IncCount()
{
    AddCount(1);
    return;
}

void CUACount::AddCount(int i)
{
    ASSERT(DBIsInit());

    _DecayCount(TRUE);
    _cCnt += i;

    if (_cCnt == 0 && i > 0) {
        // nt5:173048
        // handle wrap
        // should never happen, but what the heck
        // do *not* remove this assert, if we ever let people do DecCount
        // we'll need to rethink it...
        ASSERT(0);  // 'impossible'
        _cCnt++;
    }

    // 981029 new incr algorithm per ie5 PM
    // UAC_MINCOUNT: initial inc starts at 6
    // _fNoDecay: but, UAssist2 doesn't do this
    if (_cCnt < UAC_MINCOUNT && !_fNoDecay)
        _cCnt = UAC_MINCOUNT;

    return;
}

//***
// NOTES
//  should we update the timestamp?  maybe add a fMru param?
void CUACount::SetCount(int cCnt)
{
    ASSERT(DBIsInit());

    //_DecayCount(TRUE);  // n.b. this ups the timestamp (BUGBUG?)
    _cCnt = cCnt;

    return;
}

#if XXX_DELETE
#define BTOM(b, m)  ((b) ? (m) : 0)

DWORD CUACount::_SetFlags(DWORD dwMask, DWORD dwFlags)
{
    // standard guys
    if (dwMask & UAXF_NOPURGE)
        _fNoPurge = BOOLIFY(dwFlags & UAXF_NOPURGE);
#if 0
    if (dwMask & UAXF_BACKUP)
        _fBackup = BOOLIFY(dwFlags & UAXF_BACKUP);
#endif
    if (dwMask & UAXF_NODECAY)
        _fNoDecay = BOOLIFY(dwFlags & UAXF_NODECAY);

    // my guys
    if (dwMask & UACF_INHERITED)
        _fInherited = BOOLIFY(dwFlags & UACF_INHERITED);

    return 0    // n.b. see continuation line(s)!!!
#if XXX_DELETE
        | BTOM(_fInherited, UACF_INHERITED)
#endif
        | BTOM(_fNoPurge, UAXF_NOPURGE)
        | BTOM(_fNoDecay, UAXF_NODECAY)
        ;
}
#endif

//***   PCTOF -- p% of n (w/o floating point!)
//
#define PCTOF(n, p)   (((n) * (p)) / 100)

//***   _DecayCount -- decay (and propagate) count
// ENTRY/EXIT
//  fWrite  TRUE if want to update object and timestamp, o.w. FALSE
//  cNew    (return) new count
// DESCRIPTION
//  on a read, we do the decay but don't update the object.  on the write
// we decay and update.
// NOTES
//  todo: if/when we make cCnt a vector, we can propagate stuff here.
// this would allow us to usually inc a single small-granularity elt,
// and propagate to the large-gran elts only when we really need them.
//  perf: we could make the table 'cumulative', then we wouldn't have
// to do as much computation.  not worth the trouble...
int CUACount::_DecayCount(BOOL fWrite)
{
    int cCnt;

    cCnt = _cCnt;
    if (cCnt > 0 || fWrite) {
        UINT sidNow;

        sidNow = _puas->GetSessionId();

        if (!_fNoDecay) {
            // from mso-9 spec
            // last used 'timTab' sessions ago => dec by >-of abs, pct
            // n.b. this table is non-cumulative
            static const int timTab[] = { 3, 6, 9, 12, 17, 23, 29,  31,  -1, };
            static const int absTab[] = { 1, 1, 1,  2,  3,  4,  5,   0,   0, };
            static const int pctTab[] = { 0, 0, 0, 25, 25, 50, 75, 100, 100, };

            UINT sidMru;
            int dt;
            int i;

            sidMru = _sidMru;
            ASSERT(!ISSID_SSPECIAL(_sidMru));

            ASSERT(sidMru != SID_SDEFAULT);
            if (sidMru != SID_SDEFAULT) {
                dt = sidNow - sidMru;
                // iterate fwd not bkwd so bail early in common case
                for (i = 0; i < ARRAYSIZE(timTab); i++) {
                    if ((UINT)dt < (UINT)timTab[i])
                        break;

                    cCnt -= MAX(absTab[i], PCTOF(cCnt, pctTab[i]));
                    // don't go negative!
                    // gotta check *each* time thru loop (o.w. PCT is bogus)
                    cCnt = MAX(0, cCnt);
                }
            }
        }

        if (cCnt != _cCnt)
            TraceMsg(DM_UEMTRACE, "uac.dc: decay %d->%d", _cCnt, cCnt);

        if (fWrite) {
            _sidMru = sidNow;
            _cCnt = cCnt;
        }

#if XXX_DELETE
        if (cCnt == 0 && !_fInherited) {
            // if we decay down to 0, mark so it will be deleted
            TraceMsg(DM_UEMTRACE, "uac.dc: decay %d->%d => mark dirty pRaw=0x%x", _cCnt, cCnt, _GetRawData());
            _cCnt = 0;
            _fDirty = TRUE;
        }
#endif
    }

    return cCnt;
}

//***
// NOTES
//   perf: currently all special guys return sidNow so no 'switch' necessary
UINT CUACount::_ExpandSpecial(UINT sidMru)
{
    UINT sidNow;

    if (EVAL(ISSID_SSPECIAL(sidMru))) {
        ASSERT(_puas);
        sidNow = _puas->GetSessionId();     // perf: multiple calls
        switch (sidMru) {
        case SID_SNOWALWAYS:
            return sidNow;
            //break;

        case SID_SNOWREAD:
        case SID_SNOWINIT:
            return sidNow;
            //break;

#ifdef DEBUG
        default:
            ASSERT(0);
            break;
#endif
        }
    }

    return sidMru;
}


// Return the encoded filetime. This is read from the registry or
// generated from UpdateFileTime.
FILETIME CUACount::GetFileTime()
{
    return _ftExecuteTime;
}

// Updates the internal filetime information. This info
// will be later persisted to the registry.
void CUACount::UpdateFileTime()
{
    SYSTEMTIME st;
    // Get the current system time.
    GetSystemTime(&st);

    // This is done for ARP. They use filetimes, not the system time 
    // for the calculation of the last execute time.
    SystemTimeToFileTime(&st, &_ftExecuteTime);
}


// {
//***   UATIME --

//***   FTToUATime -- convert FILETIME to UATIME
// DESCRIPTION
//  UATIME granularity is (approximately) 1 minute.  the math works out
// roughly as follows:
//      filetime granularity is 100 nanosec
//      1 ft = 10^-7 sec
//      highword is 2^32 ft = 2^32 * 10^-7 sec
//      1 sec = hiw / (2^32 * 10^-7)
//      1 min = hiw * 60 / (2^32 * 10^-7)
//          = hiw * 60 / (1G * 10^-7)
//          ~= hiw * 60 / ~429
//          = hiw / 7.15
//          ~= hiw / 8 approx
//  the exact granularity is:
//      ...
#define FTToUATime(pft)  ((DWORD)(*(_int64 *)(pft) >> 29))  // 1 minute (approx)

//***   GetUaTime -- convert systemtime (or 'now') to UATIME
//
UATIME GetUaTime(LPSYSTEMTIME pst)
{
    FILETIME ft;
    UATIME uat;

    if (pst == NULL)
    {
        GetSystemTimeAsFileTime(&ft);
    }
    else
    {
        SystemTimeToFileTime(pst, &ft);
    }

    uat = FTToUATime(&ft);    // minutes

    return uat;
}

// }
