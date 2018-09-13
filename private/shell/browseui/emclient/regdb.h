//***   CEMDBLog --
//
#define XXX_CACHE   1       // caching on

class CEMDBLog : public CRegStrFS, public IUASession
{
public:
    // IUASession
    virtual void SetSession(UAQUANTUM uaq, BOOL fForce);
    virtual int GetSessionId();

    // THISCLASS
    HRESULT GetCount(LPCTSTR pszCmd);
    HRESULT IncCount(LPCTSTR pszCmd);
    FILETIME GetFileTime(LPCTSTR pszCmd);
    HRESULT SetCount(LPCTSTR pszCmd, int cCnt);
    DWORD _SetFlags(DWORD dwMask, DWORD dwFlags);
    HRESULT GarbageCollect(BOOL fForce);

protected:
    CEMDBLog();
    virtual ~CEMDBLog();
    friend CEMDBLog *CEMDBLog_Create(HKEY hk, DWORD grfMode);
    friend void CEMDBLog_CleanUp();
    friend class CGCTask;

    // THISCLASS helpers
    HRESULT _GetCountWithDefault(LPCTSTR pszCmd, BOOL fDefault, CUACount *pCnt);
    HRESULT _GetCountRW(LPCTSTR pszCmd, BOOL fUpdate);
    static HRESULT s_Read(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
    static HRESULT s_Write(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
    static HRESULT s_Delete(void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
#if XXX_CACHE
    typedef enum e_cacheop { CO_READ=0, CO_WRITE=1, CO_DELETE=2, } CACHEOP;
    HRESULT CacheOp(CACHEOP op, void *pvBuf, DWORD cbBuf, PNRWINFO prwi);
#endif
    TCHAR *_MayEncrypt(LPCTSTR pszSrcPlain, LPTSTR pszDstEnc, int cchDst);
    HRESULT IsDead(LPCTSTR pszCmd);
    HRESULT _GarbageCollectSlow();

    static FNNRW3 s_Nrw3Info;
#if XXX_CACHE
    struct
    {
        UINT  cbSize;
        void* pv;
    } _rgCache[2];
#endif

    BITBOOL     _fNoPurge : 1;      // 1:...
    BITBOOL     _fBackup : 1;       // 1:simulate delete (debug)
    BITBOOL     _fNoEncrypt : 1;    // 1:...
    BITBOOL     _fNoDecay : 1;      // 1:...

private:
};
