//***   CUACount -- user-assistance counter w/ decay
//
#define XXX_DELETE      1
#define XXX_VERSIONED   0

//***   NRW -- named i/o
// DESCRIPTION
//  i/o to a 'named' location (e.g. registry)
typedef struct {
    void *self;
    LPCTSTR pszName;
} NRWINFO, *PNRWINFO;

typedef HRESULT (*PFNNRW)(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
typedef struct {
    PFNNRW _pfnRead;
    PFNNRW _pfnWrite;
    PFNNRW _pfnDelete;
} FNNRW3, *PFNNRW3;

//***   UAQ_* -- quantum
// NOTES
//  todo: for now everything is flat
typedef enum {
    UAQ_TASK=0,
    UAQ_DOC=0,
    UAQ_APP=0,
    UAQ_SESSION=0
} UAQUANTUM;
#define UAQ_DEFAULT UAQ_SESSION         // currently implemented quanta
#define UAQ_COUNT   (UAQ_SESSION + 1)

typedef DWORD   UATIME;                 // 1 minute (approx)

#define UAT_MINUTE1 ((UATIME)1)         // 1  minute
#define UAT_HOUR12  ((UATIME)(12 * 60)) // 12 hours (see FTToUATime)

extern UATIME GetUaTime(LPSYSTEMTIME pst);

//***   UATTOMSEC -- convert UATIME to mSec's
// NOTES
//  BUGBUG we should be more accurate.  currently we just assume a UATIME
// is exactly 1 minute, which it's not...  easy enough to do, but we'll
// wait until i have time to do the math.
#define UATTOMSEC(uat)      ((uat) * 60 * 1000)

class IUASession
{
public:
    virtual void SetSession(UAQUANTUM uaq, BOOL fForce) PURE;
    virtual int GetSessionId() PURE;
};

class CUASession : public IUASession
{
    struct SUASession {
#if XXX_VERSIONED
        UINT    _cbSize;
#endif
        UATIME  _qtMru;
        int     _cCnt;
    };

public:
    void SetSession(UAQUANTUM uaq, BOOL fForce);
    int GetSessionId();

    CUASession();           // n.b. public so can stack-alloc
    HRESULT Initialize();
    HRESULT LoadFrom(PFNNRW3 pfnIO, PNRWINFO pRwi);
    HRESULT SaveTo(BOOL fForce, PFNNRW3 pfnIO, PNRWINFO pRwi);

protected:
    // for xfers directly into me w/o an extra copy-ctor
    // e.g. p->QueryValue(pszName, aUac.GetRawData(), &cb);
    // e.g. p->SetValue  (pszName, aUac.GetRawData(), aUac.GetRawCount());
    _inline BYTE *  _GetRawData() { return (BYTE *)&_qtMru; };
    _inline DWORD   _GetRawCount() { return SIZEOF(SUASession); };

    //struct SUASession {
#if XXX_VERSIONED
        UINT    _cbSize;
#endif
        UATIME  _qtMru;
        int     _cCnt;
    //};

    BITBOOL         _fInited : 1;   // 1:we've been initialized
    BITBOOL         _fDirty : 1;    // 1:save me (e.g. _sidMru was xformed)
};

// all special values are < 0 for easy check
// BUGBUG should be SID_ not UAC_
#define SID_SNOWREAD    (-1)    // like SID_SNOWINIT, but no auto-save
#define SID_SNOWINIT    (-2)    // convert to 'now' on 1st read
#define SID_SNOWALWAYS  (-3)    // always 'now'

#define ISSID_SSPECIAL(s) ((int)(s) < 0)

// tunable values for IncCount (talk to your neighborhood PM)
#define UAC_NEWCOUNT    2      // brand-new count starts from here
#define UAC_MINCOUNT    6      // incr to a minimum of this

class CUACount
{
    // must match CUACount semi-embedded struct
    struct SUACount 
    {
#define UAC_d0  _sidMruDisk
#if XXX_VERSIONED
#undef  UAC_d0
#define UAC_d0  _cbSize
        UINT    _cbSize;
#endif
        UINT    _sidMruDisk;    // MRU for this entry
        // todo: eventually we'll want task,doc,app,session
        // so this will be _cCnt[UAQ_COUNT], and we'll index by _cCnt[quanta]
        int     _cCnt;      // use count (lazily decayed)
        FILETIME _ftExecuteTime;
    };

public:
    CUACount();         // n.b. public so can stack-alloc
    HRESULT Initialize(IUASession *puas);
    HRESULT LoadFrom(PFNNRW3 pfnIO, PNRWINFO pRwi);
    HRESULT SaveTo(BOOL fForce, PFNNRW3 pfnIO, PNRWINFO pRwi);

#ifdef DEBUG
    BOOL    DBIsInit();
#endif
    int     GetCount();
    void    IncCount();
    void    AddCount(int i);
    void    SetCount(int cCnt);
    void    UpdateFileTime();
    FILETIME GetFileTime();

    // most people should *not* call these
    void    _SetMru(UINT sidMru) { _sidMruDisk = sidMru; Initialize(_puas); };
    int     _GetCount() { return _cCnt; };
#if XXX_DELETE
    DWORD   _SetFlags(DWORD dwMask, DWORD dwValue);
        #define UACF_INHERITED  0x01
        #define UACF_NODECAY    0x02
#endif

protected:
    int     _DecayCount(BOOL fWrite);
    UINT    _ExpandSpecial(UINT sidMru);

    // for xfers directly into me w/o an extra copy-ctor
    // e.g. p->QueryValue(pszName, aUac.GetRawData(), &cb);
    // e.g. p->SetValue  (pszName, aUac.GetRawData(), aUac.GetRawCount());
    _inline BYTE *  _GetRawData() { return (BYTE *)&UAC_d0; };
    _inline DWORD   _GetRawCount() { return SIZEOF(SUACount); };

    // struct SUACount {
#if XXX_VERSIONED
    UINT    _cbSize;        // SIZEOF
#endif
    UINT    _sidMruDisk;    // MRU for this entry
    // todo: eventually we'll want task,doc,app,session
    // so this will be cCnt[UAQ_COUNT], and we'll index by cCnt[quanta]
    int     _cCnt;      // use count (lazily decayed)
    FILETIME _ftExecuteTime;
    // }
    UINT    _sidMru;    // MRU for this entry

    IUASession *    _puas;          // session callback
    BITBOOL         _fInited : 1;   // 1:we've been initialized
    BITBOOL         _fDirty : 1;    // 1:save me (e.g. _sidMru was xformed)
#if XXX_DELETE
    BITBOOL         _fInherited : 1;    // 1:we didn't exist
#else
    BITBOOL         _fUnused : 1;
#endif
    BITBOOL         _fNoDecay : 1;      // 1:don't decay me
    BITBOOL         _fNoPurge : 1;      // 1:don't auto-delete me (debug)

private:
};
