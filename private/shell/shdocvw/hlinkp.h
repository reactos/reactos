#define DELAY_LOAD_HLINK

struct Hlink
{
#ifdef DELAY_LOAD_HLINK
#define DELAYHLINKAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
    	HRESULT hres = _Init(); \
	if (SUCCEEDED(hres)) \
	    hres = _pfn##_fn _nargs; \
	return hres;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT 	_Init(void);

    // _fInited must be the first member
    BOOL	_fInited;
    HMODULE 	_hmod;
#else
#define DELAYHLINKAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { return ::#_fn _nargs; }

#endif

    DELAYHLINKAPI(CreateURLMoniker,
		  (LPCWSTR pwsURL, IMoniker ** ppimk),
		  (pwsURL, ppimk));
    DELAYHLINKAPI(HlinkParseDisplayName,
		  (LPBC pbc, LPCOLESTR pozDisplayName, ULONG* pcchEaten, IMoniker** ppimk),
		  (pbc, pozDisplayName, pcchEaten, ppimk));
};


#ifdef DELAY_LOAD_HLINK

HRESULT Hlink::_Init(void)
{
    if (_fInited) {
	return S_OK;
    }

    _fInited = TRUE;
    _hmod = LoadLibrary(TEXT("HLINKD.DLL"));
    if (!_hmod) {
	return E_UNEXPECTED;
    }

#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hmod, #_fn); \
    if (!(_pfn##_fn)) return E_UNEXPECTED;

    CHECKAPI(CreateURLMoniker);
    CHECKAPI(HlinkParseDisplayName);
    return S_OK;
}
#endif

#ifdef DELAY_LOAD_HLINK
Hlink g_hlinkdll = { FALSE } ;
#else // DELAY_LOAD_HLINK
Hlink g_hlinkdll;
#endif // DELAY_LOAD_HLINK

