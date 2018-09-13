//#ifdef __cplusplus
//extern "C" {
//#endif

#define NO_CHANGE       0
#define LIST_CHANGED    1
#define LIST_ERROR      2
#define NO_TRANSPORT_LAYER_SELECTED -1


/****************************************************************************

    GLOBAL LIMITS CONSTANTS:

****************************************************************************/
#define MAX_MSG_TXT         4096    //Max text width in message boxes
#define MAX_VAR_MSG_TXT     8192    //Max size of a message built at run-time



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
#define assert(exp)                  exp
#define DPRINT(args)
#define DEBUG_OUT(str)
#define DEBUG_OUT1(str, a1)
#define DEBUG_OUT2(str, a1, a2)
#define DEBUG_OUT3(str, a1, a2, a3)
#endif

// Make the windbg asserts compatible with windbgrm
#define Assert(exp)                  if (!(exp)) { ShowAssert(#exp,__LINE__,__FILE__); }
#define Dbg                          assert

extern DBF Dbf;
#define lpdbf (&Dbf)

#define TRANSPORT_NAME      "tlpipe.dll"
#define RQ_CONNECT          1
#define RQ_DISCONNECT       2

extern PSTR                 pszTlName;
extern TLFUNC               TLFunc;
extern HTID                 htidBpt;
extern HINSTANCE            hTransportDll;
extern HINSTANCE            g_hInst;

// make help work
extern BOOL bHelpOnInit;
#define _Setting_Up_a_User_Mode_Remote_Debugging_Session 2701

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


//#ifdef __cplusplus
//} // extern "C" {
//#endif
