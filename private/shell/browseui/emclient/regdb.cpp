#include "priv.h"
#include <runtask.h>
#include "stream.h"
#include "uacount.h"
#include "regdb.h"
#include "uemapp.h"
#include "uareg.h"
#include "dbgmem.h"

#define DM_UEMTRACE     TF_UEM
#define DM_PERF         0           // perf tune

#define DB_NOLOG        FALSE

#define SZ_CTLSESSION       TEXT("UEME_CTLSESSION")
#define SZ_CUACount_ctor    TEXT("UEME_CTLCUACount:ctor")

#define SZ_DEL_PREFIX       TEXT("del.")
#define SZ_RUN_PREFIX       TEXT("UEME_RUN")

//***
// DESCRIPTION
//  inc this any time you change the format of *anything* below {guid}
//  doing so will cause us to nuke the {guid} subtree and start fresh
#define UA_VERSION      3

#if 0
char c_szDotDot[] = TEXT("..");     // RegStrFS does *not* support
#endif


// kind of hoaky to do INITGUID, but we want the GUID private to this file
#define INITGUID
#include <initguid.h>
// {C28EB156-523C-11d2-A561-00A0C92DBFE8}
DEFINE_GUID(CLSID_GCTaskTOID,
    0xc28eb156, 0x523c, 0x11d2, 0xa5, 0x61, 0x0, 0xa0, 0xc9, 0x2d, 0xbf, 0xe8);
#undef  INITGUID


class CGCTask : public CRunnableTask
{
public:
    //*** IUnknown
    // (... from CRunnableTask)

    //*** THISCLASS
    HRESULT Initialize(CEMDBLog *that);
    virtual STDMETHODIMP RunInitRT();

protected:
    CGCTask();
    virtual ~CGCTask();

    friend CGCTask *CGCTask_Create(CEMDBLog *that);

    CEMDBLog    *_that;
};


// {
//***   CEMDBLog --

//CRITICAL_SECTION g_csDbSvr /*=0*/ ;

CEMDBLog *g_uempDbSvr[UEMIND_NSTANDARD + UEMIND_NINSTR];    // 0=shell 1=browser

//***   g_fDidUAGC -- breadcrumbs in case we die (even non-DEBUG)
// keep minimal state in case we deadlock or die or whatever
// 0:not 1:pre-task 2:pre-GC 3:post-GC
int g_fDidUAGC;


FNNRW3 CEMDBLog::s_Nrw3Info = {
    CEMDBLog::s_Read,
    CEMDBLog::s_Write,
    CEMDBLog::s_Delete,
};

//*** helpers {

//***
// NOTES
//  BUGBUG clone: move to shlwapi
//  or, could use Reg_CreateOpenKey(..., STGM_READ) if we removed the asserts
STDAPI_(DWORD) SHRegOpenKey(HKEY hk, LPCTSTR ptszSubKey, PHKEY phkOut)
{
    long i;
    CHAR szSubKey[MAXIMUM_SUB_KEY_LENGTH];
    CHAR *psz;

    if (ptszSubKey) {
        SHTCharToAnsi(ptszSubKey, szSubKey, ARRAYSIZE(szSubKey));
        psz = szSubKey;
    }
    else {
        psz = NULL;
    }

    i = RegOpenKeyA(hk, psz, phkOut);
    return (DWORD)i;
}

STDAPI_(DWORD) SHRegOpenKeyEx(HKEY hk, LPCTSTR ptszSubKey, DWORD dwReserved, REGSAM samDesired, PHKEY phkOut)
{
    long i;
    CHAR szSubKey[MAXIMUM_SUB_KEY_LENGTH];
    CHAR *psz;

    if (ptszSubKey) {
        SHTCharToAnsi(ptszSubKey, szSubKey, ARRAYSIZE(szSubKey));
        psz = szSubKey;
    }
    else {
        psz = NULL;
    }

    i = RegOpenKeyExA(hk, psz, dwReserved, samDesired, phkOut);
    return (DWORD)i;
}

#define E_NUKE      (E_FAIL + 1)

//***   RegGetVersion -- check registry tree 'Version'
// ENTRY/EXIT
//  (see RegChkVersion)
//  hr          (ret) S_OK:ok  S_FALSE:no tree  E_NUKE:old  E_FAIL:new
HRESULT RegGetVersion(HKEY hk, LPTSTR pszSubkey, LPTSTR pszValue, DWORD dwVers)
{
    HRESULT hr;
    HKEY hk2;
    DWORD dwData;
    DWORD cbSize;

    if (!pszValue)
        pszValue = TEXT("Version");

    hr = S_FALSE;               // assume nothing there at all
    if (SHRegOpenKey(hk, pszSubkey, &hk2) == ERROR_SUCCESS) {
        hr = E_NUKE;                    // assume version mismatch
        cbSize = SIZEOF(dwData);
        if (SHGetValue(hk2, NULL, pszValue, NULL, (BYTE*)&dwData, &cbSize) == ERROR_SUCCESS) {
            if (dwData == dwVers)
                hr = S_OK;              // great!
            else if (dwData > dwVers)
                hr = E_FAIL;            // we're an old client, fail
            else
                ASSERT(hr == E_NUKE);   // we're a new client, nuke it
        }
        RegCloseKey(hk2);
    }

    return hr;
}

//***   RegChkVersion -- check registry tree 'version', nuke if outdated 
// ENTRY/EXIT
//  hk          e.g. hkey for "HKCU/.../Uassist"
//  pszSubkey   e.g. "{clsid}"
//  pszValue    e.g. "Version"
//  dwVers      e.g. 3
//  hr          (ret) S_OK:matched, S_FAIL:mismatched and del'ed, E_FAIL:o.w.
//  (other)     (SE) pszSubkey deleted if not matched
HRESULT RegChkVersion(HKEY hk, LPTSTR pszSubkey, LPTSTR pszValue, DWORD dwVers)
{
    HRESULT hr;
    DWORD i;

    // RegGetVersion()  S_OK:ok  S_FALSE:new  E_NUKE:old  E_FAIL:fail
    hr = RegGetVersion(hk, pszSubkey, pszValue, dwVers);

    // at this point, we have:
    //  S_OK: ok
    //  S_FALSE: entire tree missing
    //  E_NUKE: no "Version" or old version (nuke it)
    //  E_FAIL: new version (we can't handle it)
    if (hr == E_FAIL) {
        TraceMsg(DM_UEMTRACE, "bui.rcv: incompat (uplevel)");
    }

    if (hr == E_NUKE) {
        TraceMsg(DM_UEMTRACE, "bui.rcv: bad tree, try delete");
        hr = S_FALSE;       // assume nuked
        i = SHDeleteKey(hk, pszSubkey);
        if (i != ERROR_SUCCESS) {
            TraceMsg(DM_UEMTRACE, "bui.rcv: delete failed!");
            hr = E_FAIL;    // bogus tree left laying around
        }
    }

    TraceMsg(DM_UEMTRACE, "bui.rcv: hr=0x%x", hr);

    return hr;
}

//***   GetUEMLogger -- get the (shared) instance of our logger object
// NOTES
//  BUGBUG we leak g_uempDbSvr.  this is by design.
//  race condition on g_uempDbSvr.  our caller guards against this.
//  the 5 billion ASSERTs below were for diagnosing nt5:145449 (fixed).
HRESULT GetUEMLogger(int iSvr, CEMDBLog **p)
{
    HRESULT hr, hrVers;
    CEMDBLog *pDbSvr;
    DWORD dwData, cbSize;

    ASSERT(iSvr < ARRAYSIZE(g_uempDbSvr));
    pDbSvr = g_uempDbSvr[iSvr];

    if (pDbSvr) {
        pDbSvr->AddRef();
        *p = pDbSvr;
        return S_OK;
    }

    pDbSvr = CEMDBLog_Create(SHGetExplorerHkey(), STGM_WRITE);

    if (EVAL(pDbSvr)) {
        TCHAR szClass[GUIDSTR_MAX];     // "{clsid}"

        SHStringFromGUID(IND_NONINSTR(iSvr) ? UEMIID_BROWSER : UEMIID_SHELL, szClass, GUIDSTR_MAX);
        TraceMsg(DM_UEMTRACE, "bui.gul: UEMIID_%s=%s", IND_NONINSTR(iSvr) ? TEXT("BROWSER") : TEXT("SHELL"), szClass);

        hr = pDbSvr->ChDir(!IND_ISINSTR(iSvr) ? SZ_UASSIST : SZ_UASSIST2);
        if (SUCCEEDED(hr)) {
            hrVers = RegChkVersion(pDbSvr->GetHkey(), szClass, SZ_UAVERSION, UA_VERSION);
            if (FAILED(hrVers)) {
                TraceMsg(DM_UEMTRACE, "bui.gul: rcv()=0x%x (!)", hrVers);
                hr = hrVers;
            }
        }
        if (SUCCEEDED(hr)) {
            hr = pDbSvr->ChDir(szClass);
            ASSERT(hrVers == S_OK || hrVers == S_FALSE);
            if (SUCCEEDED(hr) && hrVers == S_FALSE) {
                dwData = UA_VERSION;
                cbSize = SIZEOF(dwData);
                hr = pDbSvr->SetValue(SZ_UAVERSION, REG_DWORD, (BYTE*)&dwData, cbSize);
            }
        }
        if (SUCCEEDED(hr))
            hr = pDbSvr->ChDir(SZ_COUNT);

        // n.b. we can't call pDbSvr->GarbageCollect here since flags
        // (e.g. _fNoDecay) not set yet
        // pDbSvr->GarbageCollect(FALSE);

        if (FAILED(hr)) {
            // this fails during RunOnce
            ASSERT(0);              // unexpected, but handled
            SAFERELEASE(pDbSvr);
            ASSERT(pDbSvr == 0);    // for assign below
        }
    }

    if (pDbSvr) {
        ENTERCRITICAL;
        if (g_uempDbSvr[iSvr] == 0) {
            g_uempDbSvr[iSvr] = pDbSvr;     // xfer refcnt
            remove_from_memlist( pDbSvr );  // cached globally
            pDbSvr = NULL;
        }
        LEAVECRITICAL;
        if (pDbSvr)
            pDbSvr->Release();
    }

    *p = g_uempDbSvr[iSvr];

    return *p ? S_OK : E_FAIL;
}

// }

//***   THIS::CEMDBLog::* {

CEMDBLog *CEMDBLog_Create(HKEY hk, DWORD grfMode)
{
    CEMDBLog *p = new CEMDBLog;

    if (p && FAILED(p->Initialize(hk, grfMode))) {
        ASSERT(0);
        p->Release();
        p = NULL;
    }

    ASSERT(p);
    return p;
}

CEMDBLog::CEMDBLog()
{
    ASSERT(_fBackup == FALSE);
    ASSERT(_fNoEncrypt == FALSE);
    return;
}

CEMDBLog::~CEMDBLog()
{
#if XXX_CACHE
    int i;

    for (i = 0; i < ARRAYSIZE(_rgCache); i++) 
    {
        if (_rgCache[i].pv) 
        {
            LocalFree(_rgCache[i].pv);
            _rgCache[i].pv = NULL;
            _rgCache[i].cbSize = 0;

        }
    }
#endif

    return;
}

void CEMDBLog_CleanUp()
{
    int i;
    CEMDBLog *pDbSvr;

    TraceMsg(DM_UEMTRACE, "bui.uadb_cu: cleaning up");
    for (i = 0; i < UEMIND_NSTANDARD + UEMIND_NINSTR; i++) {
        if ((pDbSvr = (CEMDBLog *)InterlockedExchangePointer((void**) &g_uempDbSvr[i], (LPVOID) -1)))
            delete pDbSvr;
    }
    return;
}

//***   THIS::Count -- increment profile count for command
// ENTRY/EXIT
//  fUpdate     FALSE for the GC case (since can't update reg during RegEnum)
// NOTES
//  BUGBUG NYI: *must* be made thread-safe
HRESULT CEMDBLog::GetCount(LPCTSTR pszCmd)
{
    return _GetCountRW(pszCmd, TRUE);
}

// Returns the Filetime that is encoded in the Count Object. 
// note: we do a delayed upgrade of the binary stream in the registry. We will
// use the old uem count info, but tack on the new filetime information when we increment the useage.
FILETIME CEMDBLog::GetFileTime(LPCTSTR pszCmd)
{
    NRWINFO rwi;
    HRESULT hres;
    CUACount aCnt;
    rwi.self = this;
    rwi.pszName = pszCmd;
    // This is a bizzar way of reading a string from the registry....
    hres = aCnt.LoadFrom(&s_Nrw3Info, &rwi);
    return aCnt.GetFileTime();
}


HRESULT CEMDBLog::_GetCountRW(LPCTSTR pszCmd, BOOL fUpdate)
{
    HRESULT hr;
    CUACount aCnt;
    NRWINFO rwi;
    int i;

    hr = _GetCountWithDefault(pszCmd, TRUE, &aCnt);
    ASSERT(SUCCEEDED(hr));

    i = aCnt.GetCount();

    if (fUpdate) {
        rwi.self = this;
        rwi.pszName = pszCmd;
        hr = aCnt.SaveTo(FALSE, &s_Nrw3Info, &rwi);
    }

    return i;
}

//***
// ENTRY/EXIT
//  hr  (ret) S_OK if dead, o.w. != S_OK
HRESULT CEMDBLog::IsDead(LPCTSTR pszCmd)
{
    HRESULT hr;

    hr = _GetCountRW(pszCmd, FALSE);
    return hr;
}

extern DWORD g_dCleanSess;

//***
// NOTES
//  we need to be careful not to party on guys that either aren't counts
// (e.g. UEME_CTLSESSION), or are 'special' (e.g. UEME_CTLCUACOUNT), or
// shouldn't be deleted (e.g. "del.xxx").  for now we take a conservative
// approach and just nuke things w/ UEME_RUN* as a prefix.  better might
// be to use a dope vector and delete anything that's marked as 'cleanup'.
HRESULT CEMDBLog::GarbageCollect(BOOL fForce)
{
    int i;

    if (!fForce) {
        if (g_dCleanSess != 0) {
            i = GetSessionId();
            if ((i % g_dCleanSess) != 0) {
                TraceMsg(DM_UEMTRACE, "uadb.gc: skip");
                return S_FALSE;
            }
        }
    }

    g_fDidUAGC = 1;     // breadcrumbs in case we die (even non-DEBUG)

    // do _GarbageCollectSlow(), in the background
    HRESULT hr = E_FAIL;
    CGCTask *pTask = CGCTask_Create(this);
    if (pTask) {
        IShellTaskScheduler *pSched;
        hr = CoCreateInstance(CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_IShellTaskScheduler, (void**)&pSched);

        if (SUCCEEDED(hr)) {
            hr = pSched->AddTask(pTask, CLSID_GCTaskTOID, 0L, ITSAT_DEFAULT_PRIORITY);
            pSched->Release();  // (o.k. even if task hasn't completed)
        }
        pTask->Release();
    }

    return hr;
}

HRESULT CEMDBLog::_GarbageCollectSlow()
{
    HKEY hk;
    int i;
    DWORD dwI, dwCch, dwType;
    HDSA hdsa;
    TCHAR *p;
    TCHAR szKey[MAXIMUM_SUB_KEY_LENGTH];

    TraceMsg(DM_UEMTRACE, "uadb.gc: hit");

    hdsa = DSA_Create(SIZEOF(szKey), 4);    // max size, oh well...
    if (hdsa) {
        TCHAR szRun[SIZEOF(SZ_RUN_PREFIX)];
        TCHAR *pszRun;

        pszRun = _MayEncrypt(SZ_RUN_PREFIX, szRun, ARRAYSIZE(szRun));
        if (pszRun != szRun)
            lstrcpy(szRun, pszRun);
        ASSERT(lstrlen(szRun) == lstrlen(SZ_RUN_PREFIX));
        hk = GetHkey();
        for (dwI = 0; ; dwI++) {
            dwCch = ARRAYSIZE(szKey);
            if (SHEnumValue(hk, dwI, szKey, &dwCch, &dwType, NULL, NULL) != NOERROR)
                break;
            if (StrCmpN(szKey, szRun, ARRAYSIZE(szRun) - 1) == 0) {
                if (IsDead(szKey) == S_OK)
                    DSA_AppendItem(hdsa, szKey);
            }
        }

        for (i = DSA_GetItemCount(hdsa) - 1; i > 0; i--) {
            p = (TCHAR *)DSA_GetItemPtr(hdsa, i);
            TraceMsg(DM_UEMTRACE, "uadb.gc: nuke %s", p);
            GetCount(p);    // decay to 0 will delete
        }

        DSA_Destroy(hdsa);
    }

    return S_OK;
}

HRESULT CEMDBLog::IncCount(LPCTSTR pszCmd)
{
    HRESULT hr;
    NRWINFO rwi;

    TraceMsg(DM_UEMTRACE, "uemt: ic <%s>", pszCmd);

    if (DB_NOLOG)
        return E_FAIL;

#if 0 // ChDir is currently done at create time 
    hr = ChDir(SZ_COUNT);
#endif

    CUACount aCnt;

    hr = _GetCountWithDefault(pszCmd, TRUE, &aCnt);
    ASSERT(SUCCEEDED(hr));

    aCnt.IncCount();

    // Since we are incrementing the count,
    // We should update the last execute time
    aCnt.UpdateFileTime();

    rwi.self = this;
    rwi.pszName = pszCmd;
    hr = aCnt.SaveTo(TRUE, &s_Nrw3Info, &rwi);
    ASSERT(SUCCEEDED(hr));

#if 0
    ASSERT(0);      // BUGBUG: ".." NYI!!!
    hr = ChDir(c_szDotDot);
#endif

    return hr;
}

HRESULT CEMDBLog::SetCount(LPCTSTR pszCmd, int cCnt)
{
    HRESULT hr;
    NRWINFO rwi;

    TraceMsg(DM_UEMTRACE, "uemt: ic <%s>", pszCmd);

    if (DB_NOLOG)
        return E_FAIL;

#if 0 // ChDir is currently done at create time 
    hr = ChDir(SZ_COUNT);
#endif

    CUACount aCnt;

    // fDef=FALSE so don't create if doesn't exist
    hr = _GetCountWithDefault(pszCmd, /*fDef=*/FALSE, &aCnt);

    if (SUCCEEDED(hr)) {       // don't want default...
        aCnt.SetCount(cCnt);

        rwi.self = this;
        rwi.pszName = pszCmd;
        hr = aCnt.SaveTo(TRUE, &s_Nrw3Info, &rwi);
        ASSERT(SUCCEEDED(hr));
    }

#if 0
    ASSERT(0);      // BUGBUG: ".." NYI!!!
    hr = ChDir(c_szDotDot);
#endif

    return hr;
}

//***
// ENTRY/EXIT
//  fDefault    provide default if entry not found
//  ret         S_OK: found w/o default; S_FALSE: needed default; E_xxx: error
// NOTES
//  calling w/ fDefault=FALSE can still return S_FALSE
HRESULT CEMDBLog::_GetCountWithDefault(LPCTSTR pszCmd, BOOL fDefault, CUACount *pCnt)
{
    HRESULT hr, hrDef;
    NRWINFO rwi;

    rwi.self = this;
    rwi.pszName = pszCmd;
    hr = pCnt->LoadFrom(&s_Nrw3Info, &rwi);

    hrDef = S_OK;
    if (FAILED(hr)) {
        hrDef = S_FALSE;
        if (fDefault) {
            rwi.pszName = SZ_CUACount_ctor;
            hr = pCnt->LoadFrom(&s_Nrw3Info, &rwi);

            // pCnt->Initialize happens below (possibly 2x)
            if (FAILED(hr)) {
                TraceMsg(DM_UEMTRACE, "uadb._gcwd: create ctor %s", SZ_CUACount_ctor);
                hr = pCnt->Initialize(SAFECAST(this, IUASession *));

                ASSERT(pCnt->_GetCount() == 0);
                pCnt->_SetMru(SID_SNOWINIT);    // start clock ticking...

                // cnt=UAC_NEWCOUNT, age=Now
                int i = _fNoDecay ? 1 : UAC_NEWCOUNT;
                pCnt->SetCount(i);      // force age
                ASSERT(pCnt->_GetCount() == i);

                hr = pCnt->SaveTo(/*fForce*/TRUE, &s_Nrw3Info, &rwi);
            }

#if XXX_DELETE
            pCnt->_SetFlags(UACF_INHERITED, UACF_INHERITED);
#endif
        }
    }

    ASSERT(SUCCEEDED(hr) || !pCnt->DBIsInit());

    hr = pCnt->Initialize(SAFECAST(this, IUASession *));
    ASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
        pCnt->_SetFlags(UAXF_XMASK, _SetFlags(0, 0) & UAXF_XMASK);

    return SUCCEEDED(hr) ? hrDef : hr;
}

#if XXX_DELETE
#define BTOM(b, m)  ((b) ? (m) : 0)

DWORD CEMDBLog::_SetFlags(DWORD dwMask, DWORD dwFlags)
{
    // standard guys
    if (dwMask & UAXF_NOPURGE)
        _fNoPurge = BOOLIFY(dwFlags & UAXF_NOPURGE);
    if (dwMask & UAXF_BACKUP)
        _fBackup = BOOLIFY(dwFlags & UAXF_BACKUP);
    if (dwMask & UAXF_NOENCRYPT)
        _fNoEncrypt = BOOLIFY(dwFlags & UAXF_NOENCRYPT);
    if (dwMask & UAXF_NODECAY)
        _fNoDecay = BOOLIFY(dwFlags & UAXF_NODECAY);

    // my guys
    // (none)

    return 0    // n.b. see continuation line(s)!!!
        | BTOM(_fNoPurge  , UAXF_NOPURGE)
        | BTOM(_fBackup   , UAXF_BACKUP)
        | BTOM(_fNoEncrypt, UAXF_NOENCRYPT)
        | BTOM(_fNoDecay  , UAXF_NODECAY)
        ;
}
#endif

#define ROT13(i)    (((i) + 13) % 26)

#define XXX_HASH    0       // proto code for way-shorter regnames
#if !defined(DEBUG) && XXX_HASH
#pragma message("warning: XXX_HASH defined non-DEBUG")
#endif

//***   _MayEncrypt -- encrypt registry key/value name
// NOTES
//  BUGBUG uh-oh, gotta figure out an intl-aware encryption scheme...
TCHAR *CEMDBLog::_MayEncrypt(LPCTSTR pszSrcPlain, LPTSTR pszDstEnc, int cchDst)
{
    TCHAR *pszName;

    if (!_fNoEncrypt) {
#if XXX_HASH
        DWORD dwHash;

        HashData((BYTE*)pszSrcPlain, lstrlen(pszSrcPlain), (BYTE*)&dwHash, SIZEOF(dwHash));
        pszName = pszDstEnc;
        if (EVAL(cchDst >= (8 + 1)))
            wsprintf(pszName, TEXT("%x"), dwHash);
        else
            pszName = (TCHAR *)pszSrcPlain;
#else
        TCHAR ch;

        // uh-oh, gotta figure out an intl-aware encryption scheme...
        pszName = pszDstEnc;
        pszDstEnc[--cchDst] = 0;      // pre-terminate for overflow case
        ch = -1;
        while (cchDst-- > 0 && ch != 0) {
            ch = *pszSrcPlain++;

            if (TEXT('a') <= ch && ch <= TEXT('z'))
                ch = TEXT('a') + ROT13(ch - TEXT('a'));
            else if (TEXT('A') <= ch && ch <= TEXT('Z'))
                ch = TEXT('A') + ROT13(ch - TEXT('A'));
            else
                ;

            *pszDstEnc++ = ch;
        }
#endif
        TraceMsg(DM_UEMTRACE, "uadb._me: plain=%s(enc=%s)", pszSrcPlain - (pszDstEnc - pszName), pszName);
    }
    else {
        pszName = (TCHAR *)pszSrcPlain;
    }

    return pszName;
}

#if XXX_CACHE // {
//***
// ENTRY/EXIT
//  op      0:read, 1:write, 2:delete
//
HRESULT CEMDBLog::CacheOp(CACHEOP op, void *pvBuf, DWORD cbBuf, PNRWINFO prwi)
{
    static TCHAR * const pszNameTab[] = { SZ_CTLSESSION, SZ_CUACount_ctor, };
    int i;

    ASSERT(ARRAYSIZE(pszNameTab) == ARRAYSIZE(_rgCache));

    for (i = 0; i < ARRAYSIZE(pszNameTab); i++) 
    {
        if (lstrcmp(prwi->pszName, pszNameTab[i]) == 0) 
        {
            TraceMsg(DM_PERF, "cedl.s_%c: this'=%x n=%s", TEXT("rwd")[op], prwi->self, prwi->pszName);

            switch (op) 
            {
                // Read from the cache
            case CO_READ:
                // Do we have a cached item?
                if (_rgCache[i].pv) 
                {
                    // The cached buffer should be smaller than or equal to the 
                    // passed buffer size, or we get a buffer overflow
                    ASSERT (_rgCache[i].cbSize <= cbBuf);
                    // Load the cache into the buffer. Note that the
                    // size requested may be larger than the size cached. This
                    // is due to upgrade senarios
                    memcpy(pvBuf, _rgCache[i].pv, _rgCache[i].cbSize);
                    return S_OK;
                }
                break;

                // Write to the Cache
            case CO_WRITE:

                // Is the size different or not initialized?
                // When we first allocate this spot, it's size is zero. The
                // incomming buffer should be greater.
                if (_rgCache[i].cbSize != cbBuf)
                {
                    // The size is different or uninialized.
                    if (_rgCache[i].pv)                         // Free whatever we've got 
                    {                                           // because we're getting a new one.
                        _rgCache[i].cbSize = 0;                 // Set the size to zero.
                        LocalFree(_rgCache[i].pv);
                    }

                    // Allocate a new buffer of the current size.
                    _rgCache[i].pv = LocalAlloc(LPTR, cbBuf);
                }


                // Were we successful in allocating a cache buffer?
                if (_rgCache[i].pv) 
                {
                    // Yes, make the buffer size the same... Do this here incase the
                    // allocate fails.
                    _rgCache[i].cbSize = cbBuf;
                    memcpy(_rgCache[i].pv, pvBuf, _rgCache[i].cbSize);
                    return S_OK;
                }
                break;

            case CO_DELETE:     // delete
                if (_rgCache[i].pv) 
                {
                    LocalFree(_rgCache[i].pv);
                    _rgCache[i].pv = NULL;
                    _rgCache[i].cbSize = 0;
                }
                return S_OK;

            default:
                ASSERT(0);  // 'impossible'
                break;
            }

            TraceMsg(DM_PERF, "cedl.s_%c: this'=%x n=%s cache miss", TEXT("rwd")[op], prwi->self, prwi->pszName);
            break;
        }
    }
    return S_FALSE;
}
#endif // }

HRESULT CEMDBLog::s_Read(void *pvBuf, DWORD cbBuf, PNRWINFO prwi)
{
    HRESULT hr;
    CEMDBLog *pdb = (CEMDBLog *)prwi->self;
    TCHAR *pszName;
    TCHAR szNameEnc[MAX_URL_STRING];

#if XXX_CACHE
    if (pdb->CacheOp(CO_READ, pvBuf, cbBuf, prwi) == S_OK)
        return S_OK;
#endif
    pszName = pdb->_MayEncrypt(prwi->pszName, szNameEnc, ARRAYSIZE(szNameEnc));
    hr = pdb->QueryValue(pszName, (BYTE *)pvBuf, &cbBuf);
#if XXX_CACHE
    pdb->CacheOp(CO_WRITE, pvBuf, cbBuf, prwi);
#endif
    return hr;
}

HRESULT CEMDBLog::s_Write(void *pvBuf, DWORD cbBuf, PNRWINFO prwi)
{
    HRESULT hr;
    CEMDBLog *pdb = (CEMDBLog *)prwi->self;
    TCHAR *pszName;
    TCHAR szNameEnc[MAX_URL_STRING];

#if XXX_CACHE
    // CO_DELETE not CO_WRITE (easier/safer) (perf fine since rarely write)
    pdb->CacheOp(CO_DELETE, pvBuf, cbBuf, prwi);
#endif
    pszName = pdb->_MayEncrypt(prwi->pszName, szNameEnc, ARRAYSIZE(szNameEnc));
    hr = pdb->SetValue(pszName, REG_BINARY, (BYTE *)pvBuf, cbBuf);
    return hr;
}

HRESULT CEMDBLog::s_Delete(void *pvBuf, DWORD cbBuf, PNRWINFO prwi)
{
    HRESULT hr;
    CEMDBLog *pdb = (CEMDBLog *)prwi->self;
    TCHAR *pszName;
    TCHAR szNameEnc[MAX_URL_STRING];

#if XXX_CACHE
    pdb->CacheOp(CO_DELETE, pvBuf, cbBuf, prwi);
#endif
    pszName = pdb->_MayEncrypt(prwi->pszName, szNameEnc, ARRAYSIZE(szNameEnc));
    if (pdb->_fBackup) {
        TCHAR szDel[MAX_URL_STRING];

        wnsprintf(szDel, ARRAYSIZE(szDel), SZ_DEL_PREFIX TEXT("%s"), pszName);
        if (pvBuf == NULL) {
            // happily we already have the data
            // o.w. we'd need to QueryValue into a mega-buffer
            TraceMsg(TF_WARNING, "uadb.s_d: _fBackup && !pvBuf (!)");
            ASSERT(0);
        }
        if (pvBuf != NULL) {
            hr = pdb->SetValue(szDel, REG_BINARY, (BYTE *)pvBuf, cbBuf);
            if (FAILED(hr))
                TraceMsg(TF_WARNING, "uadb.s_d: _fBackup hr=%x (!)", hr);
        }
        // (we'll do delete whether or not the _fBackup works)
    }

    hr = pdb->DeleteValue(pszName);
    TraceMsg(DM_UEMTRACE, "uadb.s_d: delete s=%s(%s) (_fBackup=%d) pRaw=0x%x hr=%x", pszName, prwi->pszName, pdb->_fBackup, pvBuf, hr);
#if 1 // unneeded?
    if (FAILED(hr))
        hr = s_Write(pvBuf, cbBuf, prwi);
#endif
    return hr;
}

// }

//***   THIS::IUASession::* {

int CEMDBLog::GetSessionId()
{
    HRESULT hr;
    NRWINFO rwi;
    CUASession aSess;
    int i;

    rwi.self = this;
    rwi.pszName = SZ_CTLSESSION;
    hr = aSess.LoadFrom(&s_Nrw3Info, &rwi);
    aSess.Initialize();

    i = aSess.GetSessionId();

    hr = aSess.SaveTo(FALSE, &s_Nrw3Info, &rwi);

    return i;
}

void CEMDBLog::SetSession(UAQUANTUM uaq, BOOL fForce)
{
    HRESULT hr;
    NRWINFO rwi;
    CUASession aSess;

    rwi.self = this;
    rwi.pszName = SZ_CTLSESSION;
    hr = aSess.LoadFrom(&s_Nrw3Info, &rwi);
    aSess.Initialize();

    aSess.SetSession(uaq, fForce);

    hr = aSess.SaveTo(TRUE, &s_Nrw3Info, &rwi);

    return;
}

// }

//***   THIS::CUASession::* {

extern DWORD g_dSessTime;

CUASession::CUASession()
{
    _fInited = FALSE;
    _fDirty = FALSE;
    return;
}

HRESULT CUASession::Initialize()
{
    if (!_fInited) {
        _fInited = TRUE;

        _cCnt = 0;
        _qtMru = 0;
        _fDirty = TRUE;
    }

    return S_OK;
}

//***   THIS::GetSessionId -- increment profile count for command
//
int CUASession::GetSessionId()
{
    return _cCnt;
}

//***
// ENTRY/EXIT
//  fForce  ignore threshhold rules (e.g. for DEBUG)
void CUASession::SetSession(UAQUANTUM uaq, BOOL fForce)
{
    UATIME qtNow;

    qtNow = GetUaTime(NULL);
    if (qtNow - _qtMru >= g_dSessTime || fForce) {
        TraceMsg(DM_UEMTRACE, "uadb.ss: sid=%d++", _cCnt);
        _cCnt++;
        // nt5:173090
        // if we wrap, there's nothing we can do.  it would be pretty
        // bad, since everything would get promoted (since 'now' will
        // be *older* than 'mru' so there will be no decay).  worse still
        // they'd stay promoted for a v. long time.  we could detect that
        // in the decay code and (lazily) reset the count to 'now,1' or
        // somesuch, but it should never happen so we simply ASSERT.
        ASSERT(_cCnt != 0);     // 'impossible'
        _qtMru = qtNow;

        _fDirty = TRUE;
    }

    return;
}

HRESULT CUASession::LoadFrom(PFNNRW3 pfnIO, PNRWINFO pRwi)
{
    HRESULT hr;

    hr = (*pfnIO->_pfnRead)(_GetRawData(), _GetRawCount(), pRwi);
    if (SUCCEEDED(hr))
        _fInited = TRUE;
    return hr;
}

HRESULT CUASession::SaveTo(BOOL fForce, PFNNRW3 pfnIO, PNRWINFO pRwi)
{
    HRESULT hr;

    hr = S_FALSE;
    if (fForce || _fDirty) {
        hr = (*pfnIO->_pfnWrite)(_GetRawData(), _GetRawCount(), pRwi);
        ASSERT(SUCCEEDED(hr));
        _fDirty = FALSE;
    }
    return hr;
}

// }

//*** CGCTask::* {
CGCTask *CGCTask_Create(CEMDBLog *that)
{
    CGCTask *pthis = new CGCTask;
    if (pthis) {
        remove_from_memlist( pthis );   // Tasks move from thread to thread
        if (FAILED(pthis->Initialize(that))) {
            delete pthis;
            pthis = NULL;
        }
    }
    return pthis;
}

HRESULT CGCTask::Initialize(CEMDBLog *that)
{
    ASSERT(!_that);
    ASSERT(that);
    that->AddRef();
    _that = that;
    return S_OK;
}

CGCTask::CGCTask() : CRunnableTask(RTF_DEFAULT)
{
}

CGCTask::~CGCTask()
{
    SAFERELEASE(_that);
    return;
}

//***   CGCTask::CRunnableTaskRT::* {

HRESULT CGCTask::RunInitRT()
{
    HRESULT hr;

    ASSERT(_that);
    g_fDidUAGC = 2;     // breadcrumbs in case we die (even non-DEBUG)
    hr = _that->_GarbageCollectSlow();
    g_fDidUAGC = 3;     // breadcrumbs in case we die (even non-DEBUG)
    return hr;
}

// }

// }

#if 0
#ifdef DEBUG
void emdbtst()
{
    HRESULT hr;
    CEMDBLog *pdb = new CEMDBLog;

    if (pdb)
    {
        hr = pdb->Initialize(HKEY_CURRENT_USER, TEXT("UIProf"));
        ASSERT(SUCCEEDED(hr));

        pdb->CountIncr("foo");
        pdb->CountIncr("bar");
        pdb->CountIncr("foo");

        delete pdb;
    }

    return;
}
#endif
#endif

// }
