

#define SYM_MAXNAMELEN 96
#define SYM_STACKLEVEL 12

struct THREADSTATE      // tag: pts
{
    THREADSTATE *   ptsNext;
    THREADSTATE *   ptsPrev;

    // Add globals below
    void *  pvRequest;              // Last pointer seen by pre-hook function.
    size_t  cbRequest;              // Last size seen by pre-hook function.
    int     cTrackDisable;          // Disable memory tracking count/flag
    BOOL    fSpyAlloc;              // Allocation is from IMallocSpy
    DWORD   dwThreadId;
};

struct SPYBLK
{
    DWORD       dwSig;
    SPYBLK *    psbNext;
    void *      pvRequest;
    size_t      cbRequest;
    DWORD       cRealloc;
    BOOL        bOKToLeak;
    DWORD       dwThreadId;
    DWORD       rdwStack[SYM_STACKLEVEL];
    CHAR szStackSym[SYM_STACKLEVEL * SYM_MAXNAMELEN];
};


class CMallocSpy :  public IMallocSpy,
                    public IShellMallocSpy
{
public:
    ~CMallocSpy();
    CMallocSpy();

    // IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IMallocSpy methods

    STDMETHOD_(ULONG,  PreAlloc)(ULONG cbRequest);
    STDMETHOD_(void *, PostAlloc)(void *pvActual);
    STDMETHOD_(void *, PreFree)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(void,   PostFree)(BOOL fSpyed);
    STDMETHOD_(ULONG,  PreRealloc)(void *pvRequest, ULONG cbRequest, void **ppvActual, BOOL fSpyed);
    STDMETHOD_(void *, PostRealloc)(void *pvActual, BOOL fSpyed);
    STDMETHOD_(void *, PreGetSize)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(ULONG,  PostGetSize)(ULONG cbActual, BOOL fSpyed);
    STDMETHOD_(void *, PreDidAlloc)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(BOOL,   PostDidAlloc)(void *pvRequest, BOOL fSpyed, BOOL fActual);
    STDMETHOD_(void,   PreHeapMinimize)();
    STDMETHOD_(void,   PostHeapMinimize)();

    // IShellMallocSpy methods

    STDMETHOD(RegisterSpy) (THIS);
    STDMETHOD(RevokeSpy) (THIS);
    STDMETHOD(SetTracking) (THIS_ BOOL bTrack);
    STDMETHOD(AddToList) (THIS_ void *pv, UINT cb);
    STDMETHOD(RemoveFromList) (THIS_ void *pv);
private:

    // Helper Functions
    void SpyEnqueue(SPYBLK * psb);
    SPYBLK *SpyDequeue(void * pvRequest);
    SPYBLK *_SpyFindBlock(void * pvRequest);
    
    LPVOID SpyPostAlloc(void * pvActual);
    LPVOID SpyPreFree(void * pvRequest);
    size_t SpyPreRealloc(void *pvRequest, size_t cbRequest, void **ppv);
    LPVOID SpyPostRealloc(void * pvActual);

    BOOL SymInit();
    int GetStackBacktrace(int ifrStart, int cfrTotal, DWORD *pdwEip, LPSTR szSym);
    BOOL WriteToFile(HANDLE hFile, LPCSTR szFormat, ...);
    BOOL DumpLeaks(LPCTSTR szFile);

    // Member variables
    ULONG               _ulRef;
    LONG    _iAllocs;
    LONG    _iPeakAllocs;
    LONG    _iBytes;
    LONG    _iPeakBytes;


    CRITICAL_SECTION _csImgHlp;
    HANDLE _hProcess;
    DWORD _dwLeakSetting;
    SPYBLK *_psbHead;
public:
    BOOL                _fRegistered;
};


// ImageHlp.DLL prototypes
 
typedef BOOL (__stdcall *pfnImgHlp_StackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );
typedef BOOL (__stdcall *pfnImgHlp_SymGetModuleInfo)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    );
typedef BOOL (__stdcall *pfnImgHlp_SymLoadModule)(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           SizeOfDll
    );
typedef BOOL (__stdcall * pfnImgHlp_SymUnDName)(
    IN  PIMAGEHLP_SYMBOL sym,               // Symbol to undecorate
    OUT LPSTR            UnDecName,         // Buffer to store undecorated name in
    IN  DWORD            UnDecNameLength    // Size of the buffer
    );
typedef BOOL (__stdcall * pfnImgHlp_SymGetLineFromAddr)(
    IN  HANDLE                  hProcess,
    IN  DWORD                   dwAddr,
    OUT PDWORD                  pdwDisplacement,
    OUT PIMAGEHLP_LINE          Line
    );
typedef BOOL (__stdcall * pfnImgHlp_SymGetSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PDWORD              pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    );

typedef HRESULT (__stdcall *pfnCoRegisterMallocSpy)(IMallocSpy * pms);
typedef HRESULT (__stdcall *pfnCoRevokeMallocSpy)();

