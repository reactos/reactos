#ifdef __cplusplus
extern "C" {
#endif

#define NO_CHANGE       0
#define LIST_CHANGED    1
#define LIST_ERROR      2
#define NO_TRANSPORT_LAYER_SELECTED -1

#define MAX_SHORT_NAME  7   // maximum chars in short name
#define MAX_LONG_NAME   255 // maximum for other fields in dialog
// a short name + a tab char + a long name + a null
#define MAX_LIST_BOX_STRING (MAX_SHORT_NAME + MAX_LONG_NAME + 2)

//
// kernel debugger options
//
typedef struct _KDPARAMS {
    BOOL      fEnable;
    BOOL      fVerbose;
    BOOL      fInitialBp;
    BOOL      fDefer;
    BOOL      fUseModem;
    BOOL      fGoExit;
    DWORD     dwBaudRate;
    DWORD     dwPort;
    DWORD     dwCache;
    DWORD     dwPlatform;
} KDPARAMS, *LPKDPARAMS;

//
// transport layer
//
typedef struct _tagTRANSPORT_LAYER {
    LPSTR       szShortName;
    LPSTR       szLongName;
    LPSTR       szDllName;
    LPSTR       szParam;
    BOOL        fDefault;
    KDPARAMS    KdParams;
} TRANSPORT_LAYER, *LPTRANSPORT_LAYER;


#if DBG
#define assert(exp)                  if (!(exp)) { ShowAssert(#exp,__LINE__,__FILE__); }
#define DPRINT(args)                 DebugPrint args;
#define DEBUG_OUT(str)               DPRINT((str))
#define DEBUG_OUT1(str, a1)          DPRINT((str, a1))
#define DEBUG_OUT2(str, a1, a2)      DPRINT((str, a1, a2))
#define DEBUG_OUT3(str, a1, a2, a3)  DPRINT((str, a1, a2, a3))
#else
#define assert(exp)
#define DPRINT(args)
#define DEBUG_OUT(str)
#define DEBUG_OUT1(str, a1)
#define DEBUG_OUT2(str, a1, a2)
#define DEBUG_OUT3(str, a1, a2, a3)
#endif

extern DBF Dbf;
#define lpdbf (&Dbf)

#define TRANSPORT_NAME      "tlpipe.dll"
#define RQ_CONNECT          1
#define RQ_DISCONNECT       2

extern CHAR                szTlName[];
extern TLFUNC              TLFunc;
extern HTID                htidBpt;
extern HANDLE              hTransportDll;

// make help work
BOOL HelpOnInit;
#define IDH_WINDBGRM 2614

void
DebugPrint(
    char * szFormat,
    ...
    );

void
ShowAssert(
    LPSTR condition,
    UINT  line,
    LPSTR file
    );


//----------------------------------------------------------------------------------------------
// GUI functions
//----------------------------------------------------------------------------------------------

void
EnableTransportChange(
    BOOL Enable
    );

//----------------------------------------------------------------------------------------------
// registry functions
//----------------------------------------------------------------------------------------------

LPTRANSPORT_LAYER
RegGetTransportLayers(
    LPDWORD lpdwCount
    );

BOOL
RegSaveTransportLayers(
    LPTRANSPORT_LAYER  lpTl,
    DWORD              dwCount
    );

LPTRANSPORT_LAYER
RegGetTransportLayer(
    LPSTR lpTransportName
    );

LPTRANSPORT_LAYER
RegGetDefaultTransportLayer(
    LPSTR lpTlName
    );

WKSPSetupTL(
    TLFUNC TLFunc,
    LPTRANSPORT_LAYER  lpTl
    );

#ifdef __cplusplus
} // extern "C" {
#endif
