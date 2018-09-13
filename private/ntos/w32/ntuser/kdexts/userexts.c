/****************************** Module Header ******************************\
* Module Name: userexts.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains user related debugging extensions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
* 23-Mar-1993 JerrySh   Moved from winsrv.dll to userexts.dll
* 21-Oct-1993 JerrySh   Modified to work with WinDbg
* 18-Oct-1994 ChrisWil  Added Object Tracking extent.
* 26-May-1995 Sanfords  Made it more general for the good of humanity.
* 6/9/1995 SanfordS     made to fit stdexts motif and to dual compile for
*                       either USER or KERNEL mode.
\***************************************************************************/
#include "userkdx.h"
#include "vkoem.h"

#ifdef KERNEL
PSTR pszExtName         = "USERKDX";
#else
PSTR pszExtName         = "USEREXTS";
#endif

#include <stdexts.h>
#include <stdexts.c>

/***************************************************************************\
* Constants
\***************************************************************************/
#define CDWORDS 16
#define BF_MAX_WIDTH    80
#define BF_COLUMN_WIDTH 19


/***************************************************************************\
* Global variables
\***************************************************************************/
BOOL bServerDebug = TRUE;
BOOL bShowFlagNames = TRUE;
char gach1[80];
char gach2[80];
CLS gcls;
PGETEPROCESSDATAFUNC GetEProcessData;
SHAREDINFO gShi;
SERVERINFO gSi;
int giBFColumn;                     // bit field: current column
char gaBFBuff[BF_MAX_WIDTH + 1];    // bit field: buffer

// used in dsi() and dinp()
typedef struct {
    int     iMetric;
    LPSTR   pstrMetric;
} SYSMET_ENTRY;
#define SMENTRY(sm) {SM_##sm, #sm}

extern ULONG_PTR UserProbeAddress;
extern int gnIndent; // indentation of !dso
/***************************************************************************\
* Macros
\***************************************************************************/

#define NELEM(array) (sizeof(array)/sizeof(array[0]))

#ifdef KERNEL // ########### KERNEL MODE ONLY MACROS ###############

#define VAR(v)  "win32k!" #v
#define FIXKP(p) p
#define RebaseSharedPtr(p)       (p)

#define FOREACHWINDOWSTATION(pwinsta)           \
    moveExpValuePtr(&pwinsta, VAR(grpWinStaList)); \
    SAFEWHILE (pwinsta != NULL) {

#define NEXTEACHWINDOWSTATION(pwinsta)          \
        move(pwinsta, &pwinsta->rpwinstaNext);  \
    }



#define FOREACHDESKTOP(pdesk)                       \
    {                                               \
        WINDOWSTATION *pwinsta;                     \
                                                    \
        FOREACHWINDOWSTATION(pwinsta)               \
        move(pdesk, &pwinsta->rpdeskList);          \
        SAFEWHILE (pdesk != NULL) {

#define NEXTEACHDESKTOP(pdesk)                      \
            move(pdesk, &pdesk->rpdeskNext);        \
        }                                           \
        NEXTEACHWINDOWSTATION(pwinsta)              \
    }



#define FOREACHPPI(ppi)                                              \
    {                                                                \
    PLIST_ENTRY ProcessHead;                                         \
    LIST_ENTRY List;                                                 \
    PLIST_ENTRY NextProcess;                                         \
    PEPROCESS pEProcess;                                             \
    PW32PROCESS pW32Process;                                         \
                                                                     \
    ProcessHead = EvalExp( "PsActiveProcessHead" );                  \
    if (!ProcessHead) {                                              \
        Print("Unable to get value of PsActiveProcessHead\n");       \
        return FALSE;                                                \
    }                                                                \
                                                                     \
    if (!tryMove(List, ProcessHead)) {                               \
        Print("Unable to get value of PsActiveProcessHead\n");       \
        return FALSE;                                                \
    }                                                                \
    NextProcess = List.Flink;                                        \
    if (NextProcess == NULL) {                                       \
        Print("PsActiveProcessHead->Flink is NULL!\n");              \
        return FALSE;                                                \
    }                                                                \
                                                                     \
    SAFEWHILE(NextProcess != ProcessHead) {                          \
        pEProcess = GetEProcessData((PEPROCESS)NextProcess,          \
                                    PROCESS_PROCESSHEAD,             \
                                    NULL);                           \
                                                                     \
        if (GetEProcessData(pEProcess, PROCESS_PROCESSLINK,          \
                &List) == NULL) {                                    \
            Print("Unable to read _EPROCESS at %lx\n",pEProcess);    \
            break;                                                   \
        }                                                            \
        NextProcess = List.Flink;                                    \
                                                                     \
        if (GetEProcessData(pEProcess, PROCESS_WIN32PROCESS,         \
                &pW32Process) == NULL || pW32Process == NULL) {      \
            continue;                                                \
        }                                                            \
                                                                     \
        ppi = (PPROCESSINFO)pW32Process;


#define NEXTEACHPPI() } }



#define FOREACHPTI(pti) {                                            \
    PPROCESSINFO ppi;                                                \
                                                                     \
    FOREACHPPI(ppi)                                                  \
        if (!tryMove(pti, &ppi->ptiList)) {                          \
            DEBUGPRINT("FOREACHPTI:Cant get ptiList from %x.\n", &ppi->ptiList); \
        }                                                            \
        SAFEWHILE (pti != NULL) {


#define NEXTEACHPTI(pti)                                                  \
            if (!tryMove(pti, &pti->ptiSibling)) {                        \
                DEBUGPRINT("NEXTEACHPTI:Can't get ptiSibling from %x.\n", \
                            &pti->ptiSibling);                            \
                pti = NULL;                                               \
            }                                                             \
        }                                                                 \
    NEXTEACHPPI()  }


#else //!KERNEL  ############## USER MODE ONLY MACROS ################

void PrivateSetRipFlags(DWORD dwRipFlags, DWORD pid);

#define VAR(v)  "user32!" #v
#define FIXKP(p) FixKernelPointer(p)

#endif //!KERNEL ############## EITHER MODE MACROS ###################

#define GETSHAREDINFO(psi) moveExp(&psi, VAR(gSharedInfo))

#define FOREACHHANDLEENTRY(phe, he, i)                               \
    {                                                                \
        PSHAREDINFO pshi;                                            \
        SHAREDINFO shi;                                              \
        SERVERINFO si;                                               \
                                                                     \
        GETSHAREDINFO(pshi);                                         \
        if (!tryMove(shi, pshi)) {                                   \
            Print("FOREACHHANDLEENTRY:Could not get SHAREDINFO.\n"); \
            return FALSE;                                           \
        }                                                            \
        if (!tryMove(si, shi.psi)) {                                 \
            Print("FOREACHHANDLEENTRY:Could not get SERVERINFO.\n"); \
        }                                                            \
        phe = shi.aheList;                                           \
        for (i = 0; si.cHandleEntries; si.cHandleEntries--, i++, phe++) { \
            if (IsCtrlCHit()) {                                      \
                break;                                               \
            }                                                        \
            if (!tryMove(he, phe)) {                                 \
                Print("FOREACHHANDLEENTRY:Cant get handle entry from %x.\n", phe); \
                continue;                                            \
            }

#define NEXTEACHHANDLEENTRY()                                        \
        }                                                            \
    }


#define DUMPHOOKS(s, hk)   \
    if (di.aphkStart[hk + 1]) { \
        Print("\t" s " @0x%p\n", di.aphkStart[hk + 1]); \
        SAFEWHILE (di.aphkStart[hk + 1]) { \
            move(hook, di.aphkStart[hk + 1]); \
            if (di.aphkStart[hk + 1] == hook.phkNext) \
                break; \
            di.aphkStart[hk + 1] = hook.phkNext; \
            Print("\t  iHook %d, offPfn=0x%08lx, flags=0x%04lx, ihmod=%d\n", \
                    hook.iHook, hook.offPfn, hook.flags, hook.ihmod); \
        } \
    }

#define DUMPLHOOKS(s, hk)   \
    if (ti.aphkStart[hk + 1]) { \
        Print("\t" s " @0x%p\n", ti.aphkStart[hk + 1]); \
        SAFEWHILE (ti.aphkStart[hk + 1]) { \
            move(hook, ti.aphkStart[hk + 1]); \
            if (ti.aphkStart[hk + 1] == hook.phkNext) \
                break; \
            ti.aphkStart[hk + 1] = hook.phkNext; \
            Print("\t  iHook %d, offPfn=0x%08lx, flags=0x%04lx, ihmod=%d\n", \
                    hook.iHook, hook.offPfn, hook.flags, hook.ihmod); \
        } \
    }


/*
 * Use these macros to print field values, globals, local values, etc.
 * This assures consistent formating plus make the extensions easier to read and to maintain.
 */
#define STRWD1 "67"
#define STRWD2 "28"
#define DWSTR1 "%08lx %." STRWD1 "s"
#define DWSTR2 "%08lx %-" STRWD2 "." STRWD2 "s"
#define DWPSTR1 "%p %." STRWD1 "s"
#define DWPSTR2 "%p %-" STRWD2 "." STRWD2 "s"
#define PRTFDW1(p, f1) Print(DWSTR1 "\n", (DWORD)##p##f1, #f1)
#define PRTVDW1(s1, v1) Print(DWSTR1 "\n", v1, #s1)
#define PRTFDW2(p, f1, f2) Print(DWSTR2 "\t" DWSTR2 "\n", (DWORD)##p##f1, #f1, (DWORD)##p##f2, #f2)
#define PRTVDW2(s1, v1, s2, v2) Print(DWSTR2 "\t" DWSTR2 "\n", v1, #s1, v2, #s2)
#define PRTFRC(p, rc) Print("%-" STRWD2 "s{%#lx, %#lx, %#lx, %#lx}\n", #rc, ##p##rc.left, ##p##rc.top, ##p##rc.right, ##p##rc.bottom)
#define PRTFPT(p, pt) Print("%-" STRWD2 "s{%#lx, %#lx}\n", #pt, ##p##pt.x, ##p##pt.y)
#define PRTVPT(s, pt) Print("%-" STRWD2 "s{%#lx, %#lx}\n", #s, pt.x, pt.y)
#define PRTFDWP1(p, f1) Print(DWPSTR1 "\n", (DWORD_PTR)##p##f1, #f1)
#define PRTFDWP2(p, f1, f2) Print(DWPSTR2 "\t" DWPSTR2 "\n", (DWORD_PTR)##p##f1, #f1, (DWORD_PTR)##p##f2, #f2)
#define PRTFDWPDW(p, f1, f2) Print(DWPSTR2 "\t" DWSTR2 "\n", (DWORD_PTR)##p##f1, #f1, (DWORD)##p##f2, #f2)
#define PRTFDWDWP(p, f1, f2) Print(DWSTR2 "\t" DWPSTR2 "\n", (DWORD)##p##f1, #f1, (DWORD_PTR)##p##f2, #f2)

/*
 * Bit Fields
 */
#define BEGIN_PRTFFLG()
#define PRTFFLG(p, f)   PrintBitField(#f, (BOOLEAN)!!(p.f))
#define END_PRTFFLG()   PrintEndBitField()

#define PRTGDW1(g1) \
        { DWORD _dw1; \
            moveExpValue(&_dw1, VAR(g1)); \
            Print(DWSTR1 "\n", _dw1, #g1); }

#define PRTGDW2(g1, g2) \
        { DWORD _dw1, _dw2; \
            moveExpValue(&_dw1, VAR(g1)); \
            moveExpValue(&_dw2, VAR(g2)); \
            Print(DWSTR2 "\t" DWSTR2 "\n",  _dw1, #g1, _dw2, #g2); }

/* This macro requires char ach[80]; to be previously defined */
#define PRTWND(s, pwnd) \
        { DebugGetWindowTextA(pwnd, ach); \
            Print("%-" STRWD2 "s" DWSTR2 "\n", #s, pwnd, ach); }

#define PRTGWND(gpwnd) \
        { PWND _pwnd; \
            moveExpValuePtr(&_pwnd, VAR(gpwnd)); \
            DebugGetWindowTextA(_pwnd, ach); \
            Print("%-" STRWD2 "s" DWSTR2 "\n", #gpwnd, _pwnd, ach); }

/****************************************************************************\
* PROTOTYPES
*  Note that all Ixxx proc prototypes are generated by stdexts.h
\****************************************************************************/
#ifdef KERNEL

PETHREAD DummyGetCurrentThreadAddress(USHORT Processor, HANDLE hCurrentThread);
PEPROCESS GetCurrentProcessAddress(DWORD Processor, HANDLE hCurrentThread, PETHREAD CurrentThread);
BOOL PrintMessages(PQMSG pqmsgRead);
BOOL GetAndDumpHE(ULONG_PTR dwT, PHE phe, BOOL fPointerTest);
LPSTR ProcessName(PPROCESSINFO ppi);

#else // !KERNEL

PVOID FixKernelPointer(PVOID pKernel);
BOOL DumpConvInfo(PCONV_INFO pcoi);
BOOL GetTargetTEB(PTEB pteb, PTEB *ppteb);

#endif // !KERNEL

LPSTR GetFlags(WORD wType, DWORD dwFlags, LPSTR pszBuf, BOOL fPrintZero);
BOOL HtoHE(ULONG_PTR h, HANDLEENTRY *phe, HANDLEENTRY **pphe);
BOOL dbgPtoH(PVOID p, ULONG_PTR *ph);
BOOL dbgHtoP(DWORD h, PVOID *pp);
PVOID GetPfromH(ULONG_PTR h, HANDLEENTRY **pphe, HANDLEENTRY *phe);
BOOL getHEfromP(HANDLEENTRY **pphe, HANDLEENTRY *phe, PVOID p);
PVOID HorPtoP(PVOID p, int type);
BOOL DebugGetWindowTextA(PWND pwnd, char *achDest);
BOOL DebugGetClassNameA(LPSTR lpszClassName, char *achDest);
BOOL dwrWorker(PWND pwnd, int tab);

BOOL IsRemoteSession(VOID)
{
    PPEB ppeb;

    ppeb = NtCurrentPeb();

    return (ppeb->SessionId != 0);
}

/****************************************************************************\
* Flags stuff
\****************************************************************************/

typedef struct _WFLAGS {
    PSZ     pszText;
    WORD    wFlag;
} WFLAGS;

#define WF_ENTRY(flag)  #flag, flag

WFLAGS aWindowFlags[] = { // sorted alphabetically
    WF_ENTRY(BFBITMAP),
    WF_ENTRY(BFBOTTOM),
    WF_ENTRY(BFCENTER),
    WF_ENTRY(BFFLAT),
    WF_ENTRY(BFICON),
    WF_ENTRY(BFLEFT),
    WF_ENTRY(BFMULTILINE),
    WF_ENTRY(BFNOTIFY),
    WF_ENTRY(BFPUSHLIKE),
    WF_ENTRY(BFRIGHT),
    WF_ENTRY(BFRIGHTBUTTON),
    WF_ENTRY(BFTOP),
    WF_ENTRY(BFVCENTER),
    WF_ENTRY(CBFAUTOHSCROLL),
    WF_ENTRY(CBFBUTTONUPTRACK),
    WF_ENTRY(CBFDISABLENOSCROLL),
    WF_ENTRY(CBFDROPDOWN),
    WF_ENTRY(CBFDROPDOWNLIST),
    WF_ENTRY(CBFDROPPABLE),
    WF_ENTRY(CBFDROPTYPE),
    WF_ENTRY(CBFEDITABLE),
    WF_ENTRY(CBFHASSTRINGS),
    WF_ENTRY(CBFLOWERCASE),
    WF_ENTRY(CBFNOINTEGRALHEIGHT),
    WF_ENTRY(CBFOEMCONVERT),
    WF_ENTRY(CBFOWNERDRAW),
    WF_ENTRY(CBFOWNERDRAWFIXED),
    WF_ENTRY(CBFOWNERDRAWVAR),
    WF_ENTRY(CBFSIMPLE),
    WF_ENTRY(CBFSORT),
    WF_ENTRY(CBFUPPERCASE),
    WF_ENTRY(DF3DLOOK),
    WF_ENTRY(DFCONTROL),
    WF_ENTRY(DFLOCALEDIT),
    WF_ENTRY(DFNOFAILCREATE),
    WF_ENTRY(DFSYSMODAL),
    WF_ENTRY(EFAUTOHSCROLL),
    WF_ENTRY(EFAUTOVSCROLL),
    WF_ENTRY(EFCOMBOBOX),
    WF_ENTRY(EFLOWERCASE),
    WF_ENTRY(EFMULTILINE),
    WF_ENTRY(EFNOHIDESEL),
    WF_ENTRY(EFNUMBER),
    WF_ENTRY(EFOEMCONVERT),
    WF_ENTRY(EFPASSWORD),
    WF_ENTRY(EFREADONLY),
    WF_ENTRY(EFUPPERCASE),
    WF_ENTRY(EFWANTRETURN),
    WF_ENTRY(SBFSIZEBOX),
    WF_ENTRY(SBFSIZEBOXBOTTOMRIGHT),
    WF_ENTRY(SBFSIZEBOXTOPLEFT),
    WF_ENTRY(SBFSIZEGRIP),
    WF_ENTRY(SFCENTERIMAGE),
    WF_ENTRY(SFEDITCONTROL),
    WF_ENTRY(SFELLIPSISMASK),
    WF_ENTRY(SFNOPREFIX),
    WF_ENTRY(SFNOTIFY),
    WF_ENTRY(SFREALSIZEIMAGE),
    WF_ENTRY(SFRIGHTJUST),
    WF_ENTRY(SFSUNKEN),
    WF_ENTRY(WEFACCEPTFILES),
    WF_ENTRY(WEFAPPWINDOW),
    WF_ENTRY(WEFCLIENTEDGE),
    WF_ENTRY(WEFCONTEXTHELP),
    WF_ENTRY(WEFCONTROLPARENT),
    WF_ENTRY(WEFDLGMODALFRAME),
    WF_ENTRY(WEFDRAGOBJECT),
    WF_ENTRY(WEFLEFTSCROLL),
    WF_ENTRY(WEFMDICHILD),
    WF_ENTRY(WEFNOACTIVATE),
    WF_ENTRY(WEFNOPARENTNOTIFY),
    WF_ENTRY(WEFRIGHT),
    WF_ENTRY(WEFRTLREADING),
    WF_ENTRY(WEFSTATICEDGE),
    WF_ENTRY(WEFLAYERED),
    WF_ENTRY(WEFTOOLWINDOW),
    WF_ENTRY(WEFTOPMOST),
    WF_ENTRY(WEFTRANSPARENT),
    WF_ENTRY(WEFTRUNCATEDCAPTION),
    WF_ENTRY(WEFWINDOWEDGE),
    WF_ENTRY(WFALWAYSSENDNCPAINT),
    WF_ENTRY(WFANSICREATOR),
    WF_ENTRY(WFANSIPROC),
    WF_ENTRY(WFANYHUNGREDRAW),
    WF_ENTRY(WFBEINGACTIVATED),
    WF_ENTRY(WFBORDER),
    WF_ENTRY(WFBOTTOMMOST),
    WF_ENTRY(WFCAPTION),
    WF_ENTRY(WFCEPRESENT),
    WF_ENTRY(WFCHILD),
    WF_ENTRY(WFCLIPCHILDREN),
    WF_ENTRY(WFCLIPSIBLINGS),
    WF_ENTRY(WFCLOSEBUTTONDOWN),
    WF_ENTRY(WFCPRESENT),
    WF_ENTRY(WFDESTROYED),
    WF_ENTRY(WFDIALOGWINDOW),
    WF_ENTRY(WFDISABLED),
    WF_ENTRY(WFDLGFRAME),
    WF_ENTRY(WFDONTVALIDATE),
    WF_ENTRY(WFERASEBKGND),
    WF_ENTRY(WFFRAMEON),
    WF_ENTRY(WFFULLSCREEN),
    WF_ENTRY(WFGOTQUERYSUSPENDMSG),
    WF_ENTRY(WFGOTSUSPENDMSG),
    WF_ENTRY(WFGROUP),
    WF_ENTRY(WFHASPALETTE),
    WF_ENTRY(WFHASSPB),
    WF_ENTRY(WFHELPBUTTONDOWN),
    WF_ENTRY(WFHIDDENPOPUP),
    WF_ENTRY(WFHPRESENT),
    WF_ENTRY(WFHSCROLL),
    WF_ENTRY(WFICONICPOPUP),
    WF_ENTRY(WFINDESTROY),
    WF_ENTRY(WFINTERNALPAINT),
    WF_ENTRY(WFLINEDNBUTTONDOWN),
    WF_ENTRY(WFLINEUPBUTTONDOWN),
    WF_ENTRY(WFMAXBOX),
    WF_ENTRY(WFMAXFAKEREGIONAL),
    WF_ENTRY(WFMAXIMIZED),
    WF_ENTRY(WFMENUDRAW),
    WF_ENTRY(WFMINBOX),
    WF_ENTRY(WFMINIMIZED),
    WF_ENTRY(WFMPRESENT),
    WF_ENTRY(WFMSGBOX),
    WF_ENTRY(WFNOANIMATE),
    WF_ENTRY(WFNOIDLEMSG),
    WF_ENTRY(WFNONCPAINT),
    WF_ENTRY(WFOLDUI),
    WF_ENTRY(WFPAGEUPBUTTONDOWN),
    WF_ENTRY(WFPAGEDNBUTTONDOWN),
    WF_ENTRY(WFPAINTNOTPROCESSED),
    WF_ENTRY(WFPIXIEHACK),
    WF_ENTRY(WFPOPUP),
    WF_ENTRY(WFREALLYMAXIMIZABLE),
    WF_ENTRY(WFREDRAWFRAMEIFHUNG),
    WF_ENTRY(WFREDRAWIFHUNG),
    WF_ENTRY(WFREDUCEBUTTONDOWN),
    WF_ENTRY(WFSCROLLBUTTONDOWN),
    WF_ENTRY(WFSENDERASEBKGND),
    WF_ENTRY(WFSENDNCPAINT),
    WF_ENTRY(WFSENDSIZEMOVE),
    WF_ENTRY(WFSERVERSIDEPROC),
    WF_ENTRY(WFSHELLHOOKWND),
    WF_ENTRY(WFSIZEBOX),
    WF_ENTRY(WFSMQUERYDRAGICON),
    WF_ENTRY(WFSTARTPAINT),
    WF_ENTRY(WFSYNCPAINTPENDING),
    WF_ENTRY(WFSYSMENU),
    WF_ENTRY(WFTABSTOP),
    WF_ENTRY(WFTILED),
    WF_ENTRY(WFTITLESET),
    WF_ENTRY(WFTOGGLETOPMOST),
    WF_ENTRY(WFTOPLEVEL),
    WF_ENTRY(WFUPDATEDIRTY),
    WF_ENTRY(WFVERTSCROLLTRACK),
    WF_ENTRY(WFVISIBLE),
    WF_ENTRY(WFVPRESENT),
    WF_ENTRY(WFVSCROLL),
    WF_ENTRY(WFWIN31COMPAT),
    WF_ENTRY(WFWIN40COMPAT),
    WF_ENTRY(WFWIN50COMPAT),
    WF_ENTRY(WFWMPAINTSENT),
    WF_ENTRY(WFZOOMBUTTONDOWN),
};

LPCSTR aszTypeNames[TYPE_CTYPES] = {
    "Free",
    "Window",
    "Menu",
    "Icon/Cursor",
    "WPI(SWP) struct",
    "Hook",
    "Clipboard Data",
    "CallProcData",
    "Accelerator",
    "DDE access",
    "DDE conv",
    "DDE Transaction",
    "Monitor",
    "Keyboard Layout",
    "Keyboard File",
    "WinEvent hook",
    "Timer",
    "Input Context",
};

#include "ptagdbg.h"   // derived from ntuser\kernel\ptag.lst and .\ptagdbg.bat

#define NO_FLAG (LPCSTR)(LONG_PTR)0xFFFFFFFF  // use this for non-meaningful entries.
#define _MASKENUM_START         (NO_FLAG-1)
#define _MASKENUM_END           (NO_FLAG-2)
#define _SHIFT_BITS             (NO_FLAG-3)
#define _CONTINUE_ON            (NO_FLAG-4)

#define MASKENUM_START(mask)    _MASKENUM_START, (LPCSTR)(mask)
#define MASKENUM_END(shift)     _MASKENUM_END, (LPCSTR)(shift)
#define SHIFT_BITS(n)           _SHIFT_BITS, (LPCSTR)(n)
#define CONTINUE_ON(arr)        _CONTINUE_ON, (LPCSTR)(arr)

LPCSTR apszSmsFlags[] = {
   "SMF_REPLY"                , // 0x0001
   "SMF_RECEIVERDIED"         , // 0x0002
   "SMF_SENDERDIED"           , // 0x0004
   "SMF_RECEIVERFREE"         , // 0x0008
   "SMF_RECEIVEDMESSAGE"      , // 0x0010
    NO_FLAG                   , // 0x0020
    NO_FLAG                   , // 0x0040
    NO_FLAG                   , // 0x0080
   "SMF_CB_REQUEST"           , // 0x0100
   "SMF_CB_REPLY"             , // 0x0200
   "SMF_CB_CLIENT"            , // 0x0400
   "SMF_CB_SERVER"            , // 0x0800
   "SMF_WOWRECEIVE"           , // 0x1000
   "SMF_WOWSEND"              , // 0x2000
   "SMF_RECEIVERBUSY"         , // 0x4000
    NULL                        // 0x8000
};

LPCSTR apszTifFlags[] = {
   "TIF_INCLEANUP"                   , // 0x00000001
   "TIF_16BIT"                       , // 0x00000002
   "TIF_SYSTEMTHREAD"                , // 0x00000004
   "TIF_CSRSSTHREAD"                 , // 0x00000008
   "TIF_TRACKRECTVISIBLE"            , // 0x00000010
   "TIF_ALLOWFOREGROUNDACTIVATE"     , // 0x00000020
   "TIF_DONTATTACHQUEUE"             , // 0x00000040
   "TIF_DONTJOURNALATTACH"           , // 0x00000080
   "TIF_WOW64"                       , // 0x00000100
   "TIF_INACTIVATEAPPMSG"            , // 0x00000200
   "TIF_SPINNING"                    , // 0x00000400
   "TIF_PALETTEAWARE"                , // 0x00000800
   "TIF_SHAREDWOW"                   , // 0x00001000
   "TIF_FIRSTIDLE"                   , // 0x00002000
   "TIF_WAITFORINPUTIDLE"            , // 0x00004000
   "TIF_MOVESIZETRACKING"            , // 0x00008000
   "TIF_VDMAPP"                      , // 0x00010000
   "TIF_DOSEMULATOR"                 , // 0x00020000
   "TIF_GLOBALHOOKER"                , // 0x00040000
   "TIF_DELAYEDEVENT"                , // 0x00080000
   "TIF_MSGPOSCHANGED"               , // 0x00100000
   "TIF_SHUTDOWNCOMPLETE"            , // 0x00200000
   "TIF_IGNOREPLAYBACKDELAY"         , // 0x00400000
   "TIF_ALLOWOTHERACCOUNTHOOK"       , // 0x00800000
   NO_FLAG                           , // 0x01000000
   "TIF_GUITHREADINITIALIZED"        , // 0x02000000
   "TIF_DISABLEIME"                  , // 0x04000000
   "TIF_INGETTEXTLENGTH"             , // 0x08000000
   "TIF_ANSILENGTH"                  , // 0x10000000
   "TIF_DISABLEHOOKS"                , // 0x20000000
    NULL                               // no more
};

LPCSTR apszQsFlags[] = {
     "QS_KEY"             , //  0x0001
     "QS_MOUSEMOVE"       , //  0x0002
     "QS_MOUSEBUTTON"     , //  0x0004
     "QS_POSTMESSAGE"     , //  0x0008
     "QS_TIMER"           , //  0x0010
     "QS_PAINT"           , //  0x0020
     "QS_SENDMESSAGE"     , //  0x0040
     "QS_HOTKEY"          , //  0x0080
     "QS_ALLPOSTMESSAGE"  , //  0x0100
     "QS_SMSREPLY"        , //  0x0200
     "QS_SYSEXPUNGE"      , //  0x0400
     "QS_THREADATTACHED"  , //  0x0800
     "QS_EXCLUSIVE"       , //  0x1000
     "QS_EVENT"           , //  0x2000
     "QS_TRANSFER"        , //  0X4000
     NULL                   //  0x8000
};

LPCSTR apszMfFlags[] = {
    "MF_GRAYED"             , // 0x0001
    "MF_DISABLED"           , // 0x0002
    "MF_BITMAP"             , // 0x0004
    "MF_CHECKED"            , // 0x0008
    "MF_POPUP"              , // 0x0010
    "MF_MENUBARBREAK"       , // 0x0020
    "MF_MENUBREAK"          , // 0x0040
    "MF_HILITE"             , // 0x0080
    "MF_OWNERDRAW"          , // 0x0100
    "MF_USECHECKBITMAPS"    , // 0x0200
    NO_FLAG                 , // 0x0400
    "MF_SEPARATOR"          , // 0x0800
    "MF_DEFAULT"            , // 0x1000
    "MF_SYSMENU"            , // 0x2000
    "MF_RIGHTJUSTIFY"       , // 0x4000
    "MF_MOUSESELECT"        , // 0x8000
     NULL
};

LPCSTR apszCsfFlags[] = {
    "CSF_SERVERSIDEPROC"      , // 0x0001
    "CSF_ANSIPROC"            , // 0x0002
    "CSF_WOWDEFERDESTROY"     , // 0x0004
    "CSF_SYSTEMCLASS"         , // 0x0008
    "CSF_WOWCLASS"            , // 0x0010
    "CSF_WOWEXTRA"            , // 0x0020
    "CSF_CACHEDSMICON"        , // 0x0040
    "CSF_WIN40COMPAT"         , // 0x0080
    NULL                        // 0x0100
};

LPCSTR apszCsFlags[] = {
    "CS_VREDRAW"          , // 0x0001
    "CS_HREDRAW"          , // 0x0002
    "CS_KEYCVTWINDOW"     , // 0x0004
    "CS_DBLCLKS"          , // 0x0008
    NO_FLAG               , // 0x0010
    "CS_OWNDC"            , // 0x0020
    "CS_CLASSDC"          , // 0x0040
    "CS_PARENTDC"         , // 0x0080
    "CS_NOKEYCVT"         , // 0x0100
    "CS_NOCLOSE"          , // 0x0200
    NO_FLAG               , // 0x0400
    "CS_SAVEBITS"         , // 0x0800
    "CS_BYTEALIGNCLIENT"  , // 0x1000
    "CS_BYTEALIGNWINDOW"  , // 0x2000
    "CS_GLOBALCLASS"      , // 0x4000
    NO_FLAG               , // 0x8000
    "CS_IME"              , // 0x10000
    NULL                    // no more
};

LPCSTR apszQfFlags[] = {
    "QF_UPDATEKEYSTATE"         , // 0x0000001
    "used to be ALTTAB"         , // 0x0000002
    "QF_FMENUSTATUSBREAK"       , // 0x0000004
    "QF_FMENUSTATUS"            , // 0x0000008
    "QF_FF10STATUS"             , // 0x0000010
    "QF_MOUSEMOVED"             , // 0x0000020
    "QF_ACTIVATIONCHANGE"       , // 0x0000040
    "QF_TABSWITCHING"           , // 0x0000080
    "QF_KEYSTATERESET"          , // 0x0000100
    "QF_INDESTROY"              , // 0x0000200
    "QF_LOCKNOREMOVE"           , // 0x0000400
    "QF_FOCUSNULLSINCEACTIVE"   , // 0x0000800
    NO_FLAG                     , // 0x0001000
    NO_FLAG                     , // 0x0002000
    "QF_DIALOGACTIVE"           , // 0x0004000
    "QF_EVENTDEACTIVATEREMOVED" , // 0x0008000
    NO_FLAG                     , // 0x0010000
    "QF_TRACKMOUSELEAVE"        , // 0x0020000
    "QF_TRACKMOUSEHOVER"        , // 0x0040000
    "QF_TRACKMOUSEFIRING"       , // 0x0080000
    "QF_CAPTURELOCKED"          , // 0x00100000
    "QF_ACTIVEWNDTRACKING"      , // 0x00200000
    NULL
};

LPCSTR apszW32pfFlags[] = {
    "W32PF_CONSOLEAPPLICATION"       , // 0x00000001
    "W32PF_FORCEOFFFEEDBACK"         , // 0x00000002
    "W32PF_STARTGLASS"               , // 0x00000004
    "W32PF_WOW"                      , // 0x00000008
    "W32PF_READSCREENACCESSGRANTED"  , // 0x00000010
    "W32PF_INITIALIZED"              , // 0x00000020
    "W32PF_APPSTARTING"              , // 0x00000040
    "W32PF_WOW64"                    , // 0x00000080
    "W32PF_ALLOWFOREGROUNDACTIVATE"  , // 0x00000100
    "W32PF_OWNDCCLEANUP"             , // 0x00000200
    "W32PF_SHOWSTARTGLASSCALLED"     , // 0x00000400
    "W32PF_FORCEBACKGROUNDPRIORITY"  , // 0x00000800
    "W32PF_TERMINATED"               , // 0x00001000
    "W32PF_CLASSESREGISTERED"        , // 0x00002000
    "W32PF_THREADCONNECTED"          , // 0x00004000
    "W32PF_PROCESSCONNECTED"         , // 0x00008000
    "W32PF_WAKEWOWEXEC"              , // 0x00010000
    "W32PF_WAITFORINPUTIDLE"         , // 0x00020000
    "W32PF_IOWINSTA"                 , // 0x00040000
    "W32PF_CONSOLEFOREGROUND"        , // 0x00080000
    "W32PF_OLELOADED"                , // 0x00100000
    "W32PF_SCREENSAVER"              , // 0x00200000
    "W32PF_IDLESCREENSAVER"          , // 0x00400000
    NULL
};


LPCSTR apszHeFlags[] = {
   "HANDLEF_DESTROY"               , // 0x0001
   "HANDLEF_INDESTROY"             , // 0x0002
   "HANDLEF_INWAITFORDEATH"        , // 0x0004
   "HANDLEF_FINALDESTROY"          , // 0x0008
   "HANDLEF_MARKED_OK"             , // 0x0010
   "HANDLEF_GRANTED"               , // 0x0020
    NULL                             // 0x0040
};


LPCSTR apszHdataFlags[] = {
     "HDATA_APPOWNED"          , // 0x0001
     NO_FLAG                   , // 0x0002
     NO_FLAG                   , // 0x0004
     NO_FLAG                   , // 0x0008
     NO_FLAG                   , // 0x0010
     NO_FLAG                   , // 0x0020
     NO_FLAG                   , // 0x0040
     NO_FLAG                   , // 0x0080
     "HDATA_EXECUTE"           , // 0x0100
     "HDATA_INITIALIZED"       , // 0x0200
     NO_FLAG                   , // 0x0400
     NO_FLAG                   , // 0x0800
     NO_FLAG                   , // 0x1000
     NO_FLAG                   , // 0x2000
     "HDATA_NOAPPFREE"         , // 0x4000
     "HDATA_READONLY"          , // 0x8000
     NULL
};

LPCSTR apszXiFlags[] = {
     "XIF_SYNCHRONOUS"    , // 0x0001
     "XIF_COMPLETE"       , // 0x0002
     "XIF_ABANDONED"      , // 0x0004
     NULL
};

LPCSTR apszIifFlags[] = {
     "IIF_IN_SYNC_XACT"   , // 0x0001
     NO_FLAG              , // 0x0002
     NO_FLAG              , // 0x0004
     NO_FLAG              , // 0x0008
     NO_FLAG              , // 0x0010
     NO_FLAG              , // 0x0020
     NO_FLAG              , // 0x0040
     NO_FLAG              , // 0x0080
     NO_FLAG              , // 0x0100
     NO_FLAG              , // 0x0200
     NO_FLAG              , // 0x0400
     NO_FLAG              , // 0x0800
     NO_FLAG              , // 0x1000
     NO_FLAG              , // 0x2000
     NO_FLAG              , // 0x4000
     "IIF_UNICODE"        , // 0x8000
     NULL
};

LPCSTR apszTmrfFlags[] = {
     "TMRF_READY"         , // 0x0001
     "TMRF_SYSTEM"        , // 0x0002
     "TMRF_RIT"           , // 0x0004
     "TMRF_INIT"          , // 0x0008
     "TMRF_ONESHOT"       , // 0x0010
     "TMRF_WAITING"       , // 0x0020
     NULL                 , // 0x0040
};


LPCSTR apszSbFlags[] = {
    "SB_VERT"             , // 0x0001
    "SB_CTL"              , // 0x0002
     NULL                 , // 0x0004
};


LPCSTR apszCSFlags[] = {
    "FS_LATIN1"           , // 0x00000001L
    "FS_LATIN2"           , // 0x00000002L
    "FS_CYRILLIC"         , // 0x00000004L
    "FS_GREEK"            , // 0x00000008L
    "FS_TURKISH"          , // 0x00000010L
    "FS_HEBREW"           , // 0x00000020L
    "FS_ARABIC"           , // 0x00000040L
    "FS_BALTIC"           , // 0x00000080L
    "FS_VIETNAMESE"       , // 0x00000100L
     NO_FLAG              , // 0x00000200L
     NO_FLAG              , // 0x00000400L
     NO_FLAG              , // 0x00000800L
     NO_FLAG              , // 0x00001000L
     NO_FLAG              , // 0x00002000L
     NO_FLAG              , // 0x00004000L
     NO_FLAG              , // 0x00008000L
    "FS_THAI"             , // 0x00010000L
    "FS_JISJAPAN"         , // 0x00020000L
    "FS_CHINESESIMP"      , // 0x00040000L
    "FS_WANSUNG"          , // 0x00080000L
    "FS_CHINESETRAD"      , // 0x00100000L
    "FS_JOHAB"            , // 0x00200000L
     NO_FLAG              , // 0x00400000L
     NO_FLAG              , // 0x00800000L
     NO_FLAG              , // 0x01000000L
     NO_FLAG              , // 0x02000000L
     NO_FLAG              , // 0x04000000L
     NO_FLAG              , // 0x08000000L
     NO_FLAG              , // 0x10000000L
     NO_FLAG              , // 0x20000000L
     NO_FLAG              , // 0x40000000L
    "FS_SYMBOL"           , // 0x80000000L
    NULL
};


LPCSTR apszMenuTypeFlags[] = {
    NO_FLAG               , // 0x0001
    NO_FLAG               , // 0x0002
    "MFT_BITMAP"          , // 0x0004 MF_BITMAP
    NO_FLAG               , // 0x0008
    "MF_POPUP"            , // 0x0010
    "MFT_MENUBARBREAK"    , // 0x0020 MF_MENUBARBREAK
    "MFT_MENUBREAK"       , // 0x0040 MF_MENUBREAK
    NO_FLAG               , // 0x0080
    "MFT_OWNERDRAW"       , // 0x0100 MF_OWNERDRAW
    NO_FLAG               , // 0x0200
    NO_FLAG               , // 0x0400
    "MFT_SEPARATOR"       , // 0x0800 MF_SEPARATOR
    NO_FLAG               , // 0x1000
    "MF_SYSMENU"          , // 0x2000
    "MFT_RIGHTJUSTIFY"    , // 0x4000 MF_RIGHTJUSTIFY
    NULL
};

LPCSTR apszMenuStateFlags[] = {
    "MF_GRAYED"           , // 0x0001
    "MF_DISABLED"         , // 0x0002
    NO_FLAG               , // 0x0004
    "MFS_CHECKED"         , // 0x0008 MF_CHECKED
    NO_FLAG               , // 0x0010
    NO_FLAG               , // 0x0020
    NO_FLAG               , // 0x0040
    "MFS_HILITE"          , // 0x0080 MF_HILITE
    NO_FLAG               , // 0x0100
    NO_FLAG               , // 0x0200
    NO_FLAG               , // 0x0400
    NO_FLAG               , // 0x0800
    "MFS_DEFAULT"         , // 0x1000 MF_DEFAULT
    NO_FLAG               , // 0x2000
    NO_FLAG               , // 0x4000
    "MF_MOUSESELECT"      , // 0x8000
    NULL
};


LPCSTR apszCursorfFlags[] = {
    "CURSORF_FROMRESOURCE", //    0x0001
    "CURSORF_GLOBAL",       //    0x0002
    "CURSORF_LRSHARED",     //    0x0004
    "CURSORF_ACON",         //    0x0008
    "CURSORF_WOWCLEANUP"  , //    0x0010
    NO_FLAG               , //    0x0020
    "CURSORF_ACONFRAME",    //    0x0040
    "CURSORF_SECRET",       //    0x0080
    "CURSORF_LINKED",       //    0x0100
    NULL
};

LPCSTR apszMonfFlags[] = {
    "MONF_VISIBLE",         // 0x01
    "MONF_PALETTEDISPLAY",  // 0x02
    NULL,
};

LPCSTR apszSifFlags[] = {
    "PUSIF_PALETTEDISPLAY",         // 0x00000001
    "PUSIF_SNAPTO",                 // 0x00000002
    "PUSIF_COMBOBOXANIMATION",      // 0x00000004
    "PUSIF_LISTBOXSMOOTHSCROLLING", // 0x00000008
    "PUSIF_KEYBOARDCUES",           // 0x00000020
    NULL,
};

LPCSTR apszRipFlags[] = {
    "RIPF_PROMPTONERROR",   // 0x0001
    "RIPF_PROMPTONWARNING", // 0x0002
    "RIPF_PROMPTONVERBOSE", // 0x0004
    NO_FLAG,                // 0x0008
    "RIPF_PRINTONERROR",    // 0x0010
    "RIPF_PRINTONWARNING",  // 0x0020
    "RIPF_PRINTONVERBOSE",  // 0x0040
    NO_FLAG,                // 0x0080
    "RIPF_PRINTFILELINE",   // 0x0100
    NULL
};

LPCSTR apszSRVIFlags[] = {
    "SRVIF_CHECKED",        // 0x0001
    "SRVIF_WINEVENTHOOKS",  // 0x0002
    "SRVIF_DBCS",           // 0x0004
    "SRVIF_IME",            // 0x0008
    "SRVIF_MIDEAST",        // 0x0010
    NULL
};

LPCSTR apszPROPFlags[] = {
    "PROPF_INTERNAL",       // 0x0001
    "PROPF_STRING",         // 0x0002
    "PROPF_NOPOOL",         // 0x0004
};

LPCSTR apszLpkEntryPoints[] = {
    "LpkTabbedTextOut"    , // 0x00000001L
    "LpkPSMTextOut"       , // 0x00000002L
    "LpkDrawTextEx"       , // 0x00000004L
    "LpkEditControl"      , // 0x00000008L
    NULL
};

/*
 * We need one of these per DWORD
 */
LPCSTR aszUserPreferencesMask0[sizeof(DWORD) * 8] = {
    "ACTIVEWINDOWTRACKING",     /*    0x1000 */
    "MENUANIMATION",            /*    0x1002 */
    "COMBOBOXANIMATION",        /*    0x1004 */
    "LISTBOXSMOOTHSCROLLING",   /*    0x1006 */
    "GRADIENTCAPTIONS",         /*    0x1008 */
    "KEYBOARDCUES",             /*    0x100A */
    "ACTIVEWNDTRKZORDER",       /*    0x100C */
    "HOTTRACKING",              /*    0x100E */
    NO_FLAG,                    /*    0x1010 */
    "MENUFADE",                 /*    0x1012 */
    "SELECTIONFADE",            /*    0x1014 */
    "TOOLTIPANIMATION",         /*    0x1016 */
    "TOOLTIPFADE",              /*    0x1018 */
    "CURSORSHADOW",             /*    0x101A */
    NO_FLAG,                    /*    0x101C */
    NO_FLAG,                    /*    0x101E */
    NO_FLAG,                    /*    0x1020 */
    NO_FLAG,                    /*    0x1022 */
    NO_FLAG,                    /*    0x1024 */
    NO_FLAG,                    /*    0x1026 */
    NO_FLAG,                    /*    0x1028 */
    NO_FLAG,                    /*    0x102A */
    NO_FLAG,                    /*    0x102C */
    NO_FLAG,                    /*    0x102E */
    NO_FLAG,                    /*    0x1030 */
    NO_FLAG,                    /*    0x1032 */
    NO_FLAG,                    /*    0x1034 */
    NO_FLAG,                    /*    0x1036 */
    NO_FLAG,                    /*    0x1038 */
    NO_FLAG,                    /*    0x103A */
    NO_FLAG,                    /*    0x103C */
    "UIEFFECTS",                /*    0x103E */
};

LPCSTR aszUserPreferences[SPI_DWORDRANGECOUNT] = {
    "FOREGROUNDLOCKTIMEOUT",    /*    0x2000 */
    "ACTIVEWNDTRKTIMEOUT",      /*    0x2002 */
    "FOREGROUNDFLASHCOUNT",     /*    0x2004 */
    "CARETWIDTH",               /*    0x2006 */
};

LPCSTR aszKeyEventFlags[] = {
    "KEYEVENTF_EXTENDEDKEY",    // 0x0001
    "KEYEVENTF_KEYUP",          // 0x0002
    "KEYEVENTF_UNICODE",        // 0x0004
    "KEYEVENTF_SCANCODE",       // 0x0008
    NULL,
};

LPCSTR aszMouseEventFlags[] = {
    "MOUSEEVENTF_MOVE",         // 0x0001
    "MOUSEEVENTF_LEFTDOWN",     // 0x0002
    "MOUSEEVENTF_LEFTUP",       // 0x0004
    "MOUSEEVENTF_RIGHTDOWN",    // 0x0008
    "MOUSEEVENTF_RIGHTUP",      // 0x0010
    "MOUSEEVENTF_MIDDLEDOWN",   // 0x0020
    "MOUSEEVENTF_MIDDLEUP",     // 0x0040
    NO_FLAG,                    // 0x0080
    NO_FLAG,                    // 0x0100
    NO_FLAG,                    // 0x0200
    NO_FLAG,                    // 0x0400
    "MOUSEEVENTF_WHEEL",        // 0x0800
    NO_FLAG,                    // 0x1000
    NO_FLAG,                    // 0x2000
    "MOUSEEVENTF_VIRTUALDESK",  // 0x4000
    "MOUSEEVENTF_ABSOLUTE",     // 0x8000
    NULL,
};

const char* aszWindowStyle[] = {
    NO_FLAG,                    // 0x00000001
    NO_FLAG,                    // 0x00000002
    NO_FLAG,                    // 0x00000004
    NO_FLAG,                    // 0x00000008
    NO_FLAG,                    // 0x00000010
    NO_FLAG,                    // 0x00000020
    NO_FLAG,                    // 0x00000040
    NO_FLAG,                    // 0x00000080
    NO_FLAG,                    // 0x00000100
    NO_FLAG,                    // 0x00000200
    NO_FLAG,                    // 0x00000400
    NO_FLAG,                    // 0x00000800
    NO_FLAG,                    // 0x00001000
    NO_FLAG,                    // 0x00002000
    NO_FLAG,                    // 0x00004000
    NO_FLAG,                    // 0x00008000
    "WS_TABSTOP",               // 0x00010000
    "WS_GROUP",                 // 0x00020000
    "WS_THICKFRAME",            // 0x00040000
    "WS_SYSMENU",               // 0x00080000
    "WS_HSCROLL",               // 0x00100000
    "WS_VSCROLL",               // 0x00200000
    "WS_DLGFRAME",              // 0x00400000
    "WS_BORDER",                // 0x00800000
    "WS_MAXIMIZE",              // 0x01000000
    "WS_CLIPCHILDREN",          // 0x02000000
    "WS_CLIPSIBLINGS",          // 0x04000000
    "WS_DISABLED",              // 0x08000000
    "WS_VISIBLE",               // 0x10000000
    "WS_MINIMIZE",              // 0x20000000
    "WS_CHILD",                 // 0x40000000
    "WS_POPUP",                 // 0x80000000
    NULL,
};

const char* aszDialogStyle[] = {
    "DS_ABSALIGN",              // 0x00000001
    "DS_SYSMODAL",              // 0x00000002
    "DS_3DLOOK",                // 0x00000004
    "DS_FIXEDSYS",              // 0x00000008
    "DS_NOFAILCREATE",          // 0x00000010
    "DS_LOCALEDIT",             // 0x00000020
    "DS_SETFONT",               // 0x00000040
    "DS_MODALFRAME",            // 0x00000080
    "DS_NOIDLEMSG",             // 0x00000100
    "DS_SETFOREGROUND",         // 0x00000200
    "DS_CONTROL",               // 0x00000400
    "DS_CENTER",                // 0x00000800
    "DS_CENTERMOUSE",           // 0x00001000
    "DS_CONTEXTHELP",           // 0x00002000
    NO_FLAG,                    // 0x00004000
    NO_FLAG,                    // 0x00008000

    CONTINUE_ON(aszWindowStyle + 16),
};


const char* aszButtonStyle[] = {
    MASKENUM_START(BS_TYPEMASK),
    "BS_PUSHBUTTON",            // 0
    "BS_DEFPUSHBUTTON",         // 1
    "BS_CHECKBOX",              // 2
    "BS_AUTOCHECKBOX",          // 3
    "BS_RADIOBUTTON",           // 4
    "BS_3STATE",                // 5
    "BS_AUTO3STATE",            // 6
    "BS_GROUPBOX",              // 7
    "BS_USERBUTTON",            // 8
    "BS_AUTORADIOBUTTON",       // 9
    "BS_PUSHBOX",               // a
    "BS_OWNERDRAW",             // b
    MASKENUM_END(4),

    NO_FLAG,                    // 0x00000010
    "BS_LEFTTEXT",              // 0x00000020

    MASKENUM_START(BS_IMAGEMASK),
    "BS_TEXT",                  // 0
    "BS_ICON",
    "BS_BITMAP",
    MASKENUM_END(2),

    MASKENUM_START(BS_HORZMASK),
    NO_FLAG,
    "BS_LEFT",
    "BS_RIGHT",
    "BS_CENTER",
    MASKENUM_END(2),

    MASKENUM_START(BS_VERTMASK),
    NO_FLAG,
    "BS_TOP", "BS_BOTTOM", "BS_VCENTER",
    MASKENUM_END(2),

    "BS_PUSHLIKE",              // 0x00001000
    "BS_MULTILINE",             // 0x00002000
    "BS_NOTIFY",                // 0x00004000
    "BS_FLAT",                  // 0x00008000

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszComboBoxStyle[] = {
    MASKENUM_START(0x0f),
    NO_FLAG,                    // 0
    "CBS_SIMPLE",               // 1
    "CBS_DROPDOWN",             // 2
    "CBS_DROPDOWNLIST",         // 3
    MASKENUM_END(4),

    "CBS_OWNERDRAWFIXED",       // 0x0010L
    "CBS_OWNERDRAWVARIABLE",    // 0x0020L
    "CBS_AUTOHSCROLL",          // 0x0040L
    "CBS_OEMCONVERT",           // 0x0080L
    "CBS_SORT",                 // 0x0100L
    "CBS_HASSTRINGS",           // 0x0200L
    "CBS_NOINTEGRALHEIGHT",     // 0x0400L
    "CBS_DISABLENOSCROLL",      // 0x0800L
    NO_FLAG,                    // 0x1000L
    "CBS_UPPERCASE",            // 0x2000L
    "CBS_LOWERCASE",            // 0x4000L
    NO_FLAG,                    // 0x8000L

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszStaticStyle[] = {
    MASKENUM_START(SS_TYPEMASK),
    "SS_LEFT",              // 0x00000000L
    "SS_CENTER",            // 0x00000001L
    "SS_RIGHT",             // 0x00000002L
    "SS_ICON",              // 0x00000003L
    "SS_BLACKRECT",         // 0x00000004L
    "SS_GRAYRECT",          // 0x00000005L
    "SS_WHITERECT",         // 0x00000006L
    "SS_BLACKFRAME",        // 0x00000007L
    "SS_GRAYFRAME",         // 0x00000008L
    "SS_WHITEFRAME",        // 0x00000009L
    "SS_USERITEM",          // 0x0000000AL
    "SS_SIMPLE",            // 0x0000000BL
    "SS_LEFTNOWORDWRAP",    // 0x0000000CL
    "SS_OWNERDRAW",         // 0x0000000DL
    "SS_BITMAP",            // 0x0000000EL
    "SS_ENHMETAFILE",       // 0x0000000FL
    "SS_ETCHEDHORZ",        // 0x00000010L
    "SS_ETCHEDVERT",        // 0x00000011L
    "SS_ETCHEDFRAME",       // 0x00000012L
    MASKENUM_END(5),

    NO_FLAG,                // 0x00000020L
    NO_FLAG,                // 0x00000040L
    "SS_NOPREFIX",          // 0x00000080L /* Don't do "&" character translation */
    "SS_NOTIFY",            // 0x00000100L
    "SS_CENTERIMAGE",       // 0x00000200L
    "SS_RIGHTJUST",         // 0x00000400L
    "SS_REALSIZEIMAGE",     // 0x00000800L
    "SS_SUNKEN",            // 0x00001000L
    "SS_EDITCONTROL",       // 0x00002000L ;internal

    MASKENUM_START(SS_ELLIPSISMASK),
    NO_FLAG,
    "SS_ENDELLIPSIS",       // 0x00004000L
    "SS_PATHELLIPSIS",      // 0x00008000L
    "SS_WORDELLIPSIS",      // 0x0000C000L
    MASKENUM_END(2),

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszListBoxStyle[] = {
    "LBS_NOTIFY",               // 0x0001L
    "LBS_SORT",                 // 0x0002L
    "LBS_NOREDRAW",             // 0x0004L
    "LBS_MULTIPLESEL",          // 0x0008L
    "LBS_OWNERDRAWFIXED",       // 0x0010L
    "LBS_OWNERDRAWVARIABLE",    // 0x0020L
    "LBS_HASSTRINGS",           // 0x0040L
    "LBS_USETABSTOPS",          // 0x0080L
    "LBS_NOINTEGRALHEIGHT",     // 0x0100L
    "LBS_MULTICOLUMN",          // 0x0200L
    "LBS_WANTKEYBOARDINPUT",    // 0x0400L
    "LBS_EXTENDEDSEL",          // 0x0800L
    "LBS_DISABLENOSCROLL",      // 0x1000L
    "LBS_NODATA",               // 0x2000L
    "LBS_NOSEL",                // 0x4000L
    NO_FLAG,                    // 0x8000L

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszEditStyle[] = {
    MASKENUM_START(ES_FMTMASK),
    "ES_LEFT",              // 0x0000L
    "ES_CENTER",            // 0x0001L
    "ES_RIGHT",             // 0x0002L
    MASKENUM_END(2),

    "ES_MULTILINE",         // 0x0004L
    "ES_UPPERCASE",         // 0x0008L
    "ES_LOWERCASE",         // 0x0010L
    "ES_PASSWORD",          // 0x0020L
    "ES_AUTOVSCROLL",       // 0x0040L
    "ES_AUTOHSCROLL",       // 0x0080L
    "ES_NOHIDESEL",         // 0x0100L
    "ES_COMBOBOX",          // 0x0200L     ;internal
    "ES_OEMCONVERT",        // 0x0400L
    "ES_READONLY",          // 0x0800L
    "ES_WANTRETURN",        // 0x1000L
    "ES_NUMBER",            // 0x2000L     ;public_winver_400
    NO_FLAG,                // 0x4000L
    NO_FLAG,                // 0x8000L

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszScrollBarStyle[] = {
    "SBS_HORZ",                     // 0x0000L
    "SBS_VERT",                     // 0x0001L
    "SBS_TOPALIGN",                 // 0x0002L
    "SBS_LEFTALIGN",                // 0x0002L
    "SBS_BOTTOMALIGN",              // 0x0004L
    "SBS_RIGHTALIGN",               // 0x0004L
    "SBS_SIZEBOXTOPLEFTALIGN",      // 0x0002L
    "SBS_SIZEBOXBOTTOMRIGHTALIGN",  // 0x0004L
    "SBS_SIZEBOX",                  // 0x0008L
    "SBS_SIZEGRIP",                 // 0x0010L
    SHIFT_BITS(8),                  // 8 bits

    CONTINUE_ON(aszWindowStyle + 16),
};

const char* aszWindowExStyle[] = {
    "WS_EX_DLGMODALFRAME",      // 0x00000001L
    "WS_EX_DRAGOBJECT",         // 0x00000002L  ;internal
    "WS_EX_NOPARENTNOTIFY",     // 0x00000004L
    "WS_EX_TOPMOST",            // 0x00000008L
    "WS_EX_ACCEPTFILES",        // 0x00000010L
    "WS_EX_TRANSPARENT",        // 0x00000020L
    "WS_EX_MDICHILD",           // 0x00000040L
    "WS_EX_TOOLWINDOW",         // 0x00000080L
    "WS_EX_WINDOWEDGE",         // 0x00000100L
    "WS_EX_CLIENTEDGE",         // 0x00000200L
    "WS_EX_CONTEXTHELP",        // 0x00000400L
    NO_FLAG,                    // 0x00000800L

    "WS_EX_RIGHT",              // 0x00001000L
//  "WS_EX_LEFT",               // 0x00000000L
    "WS_EX_RTLREADING",         // 0x00002000L
//  "WS_EX_LTRREADING",         // 0x00000000L
    "WS_EX_LEFTSCROLLBAR",      // 0x00004000L
//  "WS_EX_RIGHTSCROLLBAR",     // 0x00000000L
    NO_FLAG,                    // 0x00008000L

    "WS_EX_CONTROLPARENT",      // 0x00010000L
    "WS_EX_STATICEDGE",         // 0x00020000L
    "WS_EX_APPWINDOW",          // 0x00040000L
    "WS_EX_LAYERED",            // 0x00080000
    NULL
};

const char* aszClientImcFlags[] = {
    "IMCF_UNICODE",         // 0x0001
    "IMCF_ACTIVE",          // 0x0002
    "IMCF_CHGMSG",          // 0x0004
    "IMCF_SAVECTRL",        // 0x0008
    "IMCF_PROCESSEVENT",    // 0x0010
    "IMCF_FIRSTSELECT",     // 0x0020
    "IMCF_INDESTROY",       // 0x0040
    "IMCF_WINNLSDISABLE",   // 0x0080
    "IMCF_DEFAULTIMC",      // 0x0100
    NULL,
};

const char* aszConversionModes[] = {
    "IME_CMODE_NATIVE",                 // 0x0001
    "IME_CMODE_KATAKANA",               // 0x0002  // only effect under IME_CMODE_NATIVE
    NO_FLAG,                            // 0x0004
    "IME_CMODE_FULLSHAPE",              // 0x0008
    "IME_CMODE_ROMAN",                  // 0x0010
    "IME_CMODE_CHARCODE",               // 0x0020
    "IME_CMODE_HANJACONVERT",           // 0x0040
    "IME_CMODE_SOFTKBD",                // 0x0080
    "IME_CMODE_NOCONVERSION",           // 0x0100
    "IME_CMODE_EUDC",                   // 0x0200
    "IME_CMODE_SYMBOL",                 // 0x0400
    "IME_CMODE_FIXED",                  // 0x0800
    NULL
};

const char* aszSentenceModes[] = {
    "IME_SMODE_PLAURALCLAUSE",          // 0x0001
    "IME_SMODE_SINGLECONVERT",          // 0x0002
    "IME_SMODE_AUTOMATIC",              // 0x0004
    "IME_SMODE_PHRASEPREDICT",          // 0x0008
    "IME_SMODE_CONVERSATION",           // 0x0010
    NULL
};

const char* aszImeInit[] = {
    "INIT_STATUSWNDPOS",            // 0x00000001
    "INIT_CONVERSION",              // 0x00000002
    "INIT_SENTENCE",                // 0x00000004
    "INIT_LOGFONT",                 // 0x00000008
    "INIT_COMPFORM",                // 0x00000010
    "INIT_SOFTKBDPOS",              // 0x00000020
    NULL
};

const char* aszImeSentenceMode[] = {
    "IME_SMODE_PLAURALCLAUSE",      // 0x0001
    "IME_SMODE_SINGLECONVERT",      // 0x0002
    "IME_SMODE_AUTOMATIC",          // 0x0004
    "IME_SMODE_PHRASEPREDICT",      // 0x0008
    "IME_SMODE_CONVERSATION",       // 0x0010
    NULL
};

const char* aszImeConversionMode[] = {
    "IME_CMODE_NATIVE",             // 0x0001
    "IME_CMODE_KATAKANA",           // 0x0002  // only effect under IME_CMODE_NATIVE
    NO_FLAG,
    "IME_CMODE_FULLSHAPE",          // 0x0008
    "IME_CMODE_ROMAN",              // 0x0010
    "IME_CMODE_CHARCODE",           // 0x0020
    "IME_CMODE_HANJACONVERT",       // 0x0040
    "IME_CMODE_SOFTKBD",            // 0x0080
    "IME_CMODE_NOCONVERSION",       // 0x0100
    "IME_CMODE_EUDC",               // 0x0200
    "IME_CMODE_SYMBOL",             // 0x0400
    "IME_CMODE_FIXED",              // 0x0800
    NULL
};

const char* aszImeDirtyFlags[] = {
    "IMSS_UPDATE_OPEN",             // 0x0001
    "IMSS_UPDATE_CONVERSION",       // 0x0002
    "IMSS_UPDATE_SENTENCE",         // 0x0004
    NO_FLAG,                        // 0x0008
    NO_FLAG,                        // 0x0010
    NO_FLAG,                        // 0x0020
    NO_FLAG,                        // 0x0040
    NO_FLAG,                        // 0x0080
    "IMSS_INIT_OPEN",               // 0x0100
    NULL
};

const char* aszImeCompFormFlags[] = {
//  "CFS_DEFAULT",                  // 0x0000
    "CFS_RECT",                     // 0x0001
    "CFS_POINT",                    // 0x0002
    "CFS_SCREEN",                   // 0x0004          @Internal
    "CFS_VERTICAL",                 // 0x0008          @Internal
    "CFS_HIDDEN",                   // 0x0010          @Internal
    "CFS_FORCE_POSITION",           // 0x0020
    "CFS_CANDIDATEPOS",             // 0x0040
    "CFS_EXCLUDE",                  // 0x0080
};


const char* aszEdUndoType[] = {
    "UNDO_INSERT",                  // 0x0001
    "UNDO_DELETE",                  // 0x0002
    NULL,
};

const char* aszDeviceInfoActionFlags[] = {
    "GDIAF_ARRIVED",                // 0x0001
    "GDIAF_QUERYREMOVE",            // 0x0002
    "GDIAF_REMOVECANCELLED",        // 0x0004
    "GDIAF_DEPARTED",               // 0x0008
    "GDIAF_IME_STATUS",             // 0x0010
    "GDIAF_REFRESH_MOUSE",          // 0x0020
    NO_FLAG,                        // 0x0040
    "GDIAF_FREEME",                 // 0x0080
    "GDIAF_PNPWAITING",             // 0x0100
    "GDIAF_RETRYREAD",              // 0x0200
    NULL,
};

enum GF_FLAGS {
    GF_SMS = 0,
    GF_TIF,
    GF_QS,
    GF_MF,
    GF_CSF,
    GF_CS,
    GF_QF,
    GF_W32PF,
    GF_HE,
    GF_HDATA,
    GF_XI,
    GF_IIF,
    GF_TMRF,
    GF_SB,
    GF_CHARSETS,
    GF_MENUTYPE,
    GF_MENUSTATE,
    GF_CURSORF,
    GF_MON,
    GF_SI,
    GF_RIP,
    GF_SRVI,
    GF_PROP,
    GF_UPM0,
    GF_KI,
    GF_MI,
    GF_DS,
    GF_WS,
    GF_ES,
    GF_BS,
    GF_CBS,
    GF_SS,
    GF_LBS,
    GF_SBS,
    GF_WSEX,
    GF_CLIENTIMC,
    GF_CONVERSION,
    GF_SENTENCE,
    GF_IMEINIT,
    GF_IMEDIRTY,
    GF_IMECOMPFORM,
    GF_EDUNDO,
    GF_DIAF,

    GF_LPK,
    GF_MAX
};

LPCSTR* aapszFlag[GF_MAX] = {
    apszSmsFlags,
    apszTifFlags,
    apszQsFlags,
    apszMfFlags,
    apszCsfFlags,
    apszCsFlags,
    apszQfFlags,
    apszW32pfFlags,
    apszHeFlags,
    apszHdataFlags,
    apszXiFlags,
    apszIifFlags,
    apszTmrfFlags,
    apszSbFlags,
    apszCSFlags,
    apszMenuTypeFlags,
    apszMenuStateFlags,
    apszCursorfFlags,
    apszMonfFlags,
    apszSifFlags,
    apszRipFlags,
    apszSRVIFlags,
    apszPROPFlags,
    aszUserPreferencesMask0,
    aszKeyEventFlags,
    aszMouseEventFlags,
    aszDialogStyle,
    aszWindowStyle,
    aszEditStyle,
    aszButtonStyle,
    aszComboBoxStyle,
    aszStaticStyle,
    aszListBoxStyle,
    aszScrollBarStyle,
    aszWindowExStyle,
    aszClientImcFlags,
    aszConversionModes,
    aszSentenceModes,
    aszImeInit,
    aszImeDirtyFlags,
    aszImeCompFormFlags,
    aszEdUndoType,
    aszDeviceInfoActionFlags,

    apszLpkEntryPoints,
};


/************************************************************************\
* Procedure: GetFlags
*
* Description:
*
* Converts a 32bit set of flags into an appropriate string.
* pszBuf should be large enough to hold this string, no checks are done.
* pszBuf can be NULL, allowing use of a local static buffer but note that
* this is not reentrant.
* Output string has the form: "FLAG1 | FLAG2 ..." or "0"
*
* Returns: pointer to given or static buffer with string in it.
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
LPSTR GetFlags(
    WORD    wType,
    DWORD   dwFlags,
    LPSTR   pszBuf,
    BOOL    fPrintZero)
{
    static char szT[512];
    WORD i;
    BOOL fFirst = TRUE;
    BOOL fNoMoreNames = FALSE;
    LPCSTR *apszFlags;
    LPSTR apszFlagNames[sizeof(DWORD) * 8], pszT;
    const char** ppszNextFlag;
    UINT uFlagsCount, uNextFlag;
    DWORD dwUnnamedFlags, dwLoopFlag;
    DWORD dwShiftBits;
    DWORD dwOrigFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    if (!bShowFlagNames) {
        sprintf(pszBuf, "%x", dwFlags);
        return pszBuf;
    }

    if (wType >= GF_MAX) {
        strcpy(pszBuf, "Invalid flag type.");
        return pszBuf;
    }

    /*
     * Initialize output buffer and names array
     */
    *pszBuf = '\0';
    RtlZeroMemory(apszFlagNames, sizeof(apszFlagNames));

    apszFlags = aapszFlag[wType];

    /*
     * Build a sorted array containing the names of the flags in dwFlags
     */
    uFlagsCount = 0;
    dwUnnamedFlags = dwOrigFlags = dwFlags;
    dwLoopFlag = 1;
    dwShiftBits = 0;

reentry:
    for (i = 0; dwFlags; dwFlags >>= 1, i++, dwLoopFlag <<= 1, ++dwShiftBits) {
        const char* lpszFlagName = NULL;

        /*
         * Bail if we reached the end of the flag names array
         */
        if (apszFlags[i] == NULL) {
            break;
        }

        if (apszFlags[i] == _MASKENUM_START) {
            //
            // Masked enumerative items.
            //
            DWORD en = 0;
            DWORD dwMask = (DWORD)(ULONG_PTR)apszFlags[++i];

            // First, clear up the handled bits.
            dwUnnamedFlags &= ~dwMask;
            lpszFlagName = NULL;
            for (++i; apszFlags[i] != NULL && apszFlags[i] != _MASKENUM_END; ++i, ++en) {
                if ((dwOrigFlags & dwMask) == (en << dwShiftBits )) {
                    if (apszFlags[i] != NO_FLAG) {
                        lpszFlagName = apszFlags[i];
                    }
                }
            }
            //
            // Shift the bits and get ready for the next item.
            // Next item right after _MASKENUM_END holds the bits to shift.
            //
            dwFlags >>= (int)(ULONG_PTR)apszFlags[++i] - 1;
            dwLoopFlag <<= (int)(ULONG_PTR)apszFlags[i] - 1;
            dwShiftBits += (int)(ULONG_PTR)apszFlags[i] - 1;
            if (lpszFlagName == NULL) {
                //
                // Could not find the match. Skip to the next item.
                //
                continue;
            }
        }
        else if (apszFlags[i] == _CONTINUE_ON) {
            //
            // Refer the other item array. Pointer to the array is stored at [i+1].
            //
            apszFlags = (LPSTR*)apszFlags[i + 1];
            goto reentry;
        }
        else if (apszFlags[i] == _SHIFT_BITS) {
            //
            // To save some space, just shift some bits..
            //
            dwFlags >>= (int)(ULONG_PTR)apszFlags[++i] - 1;
            dwLoopFlag <<= (int)(ULONG_PTR)apszFlags[i] - 1;
            dwShiftBits += (int)(ULONG_PTR)apszFlags[i] - 1;
            continue;
        }
        else {
            /*
             * continue if this bit is not set or we don't have a name for it
             */
            if (!(dwFlags & 1) || (apszFlags[i] == NO_FLAG)) {
                continue;
            }
            lpszFlagName = apszFlags[i];
        }

        /*
         * Find the sorted position where this name should go
         */
        ppszNextFlag = apszFlagNames;
        uNextFlag = 0;
        while (uNextFlag < uFlagsCount) {
            if (strcmp(*ppszNextFlag, lpszFlagName) > 0) {
                break;
            }
            ppszNextFlag++;
            uNextFlag++;
        }
        /*
         * Insert the new name
         */
        RtlMoveMemory((char*)(ppszNextFlag + 1), ppszNextFlag, (uFlagsCount - uNextFlag) * sizeof(DWORD));
        *ppszNextFlag = lpszFlagName;
        uFlagsCount++;
        /*
         * We got a name so clear it from the unnamed bits.
         */
        dwUnnamedFlags &= ~dwLoopFlag;
    }

    /*
     * Build the string now
     */
    ppszNextFlag = apszFlagNames;
    pszT = pszBuf;
    /*
     * Add the first name
     */
    if (uFlagsCount > 0) {
        pszT += sprintf(pszT, "%s", *ppszNextFlag++);
        uFlagsCount--;
    }
    /*
     * Concatenate all other names with " |"
     */
    while (uFlagsCount > 0) {
        pszT += sprintf(pszT, " | %s", *ppszNextFlag++);
        uFlagsCount--;
    }
    /*
     * If there are unamed bits, add them at the end
     */
    if (dwUnnamedFlags != 0) {
        pszT += sprintf(pszT, " | %#lx", dwUnnamedFlags);
    }
    /*
     * Print zero if needed and asked to do so
     */
    if (fPrintZero && (pszT == pszBuf)) {
        sprintf(pszBuf, "0");
    }

    return pszBuf;
}

///////////////////////////////////////////////////////////////////////////
//
// Enumerated items with mask
//
///////////////////////////////////////////////////////////////////////////

typedef struct {
    LPCSTR  name;
    DWORD   value;
} EnumItem;

#define EITEM(a)     { #a, a }

const EnumItem aClsTypes[] = {
    EITEM(ICLS_BUTTON),
    EITEM(ICLS_EDIT),
    EITEM(ICLS_STATIC),
    EITEM(ICLS_LISTBOX),
    EITEM(ICLS_SCROLLBAR),
    EITEM(ICLS_COMBOBOX),
    EITEM(ICLS_CTL_MAX),
    EITEM(ICLS_DESKTOP),
    EITEM(ICLS_DIALOG),
    EITEM(ICLS_MENU),
    EITEM(ICLS_SWITCH),
    EITEM(ICLS_ICONTITLE),
    EITEM(ICLS_MDICLIENT),
    EITEM(ICLS_COMBOLISTBOX),
    EITEM(ICLS_DDEMLEVENT),
    EITEM(ICLS_DDEMLMOTHER),
    EITEM(ICLS_DDEML16BIT),
    EITEM(ICLS_DDEMLCLIENTA),
    EITEM(ICLS_DDEMLCLIENTW),
    EITEM(ICLS_DDEMLSERVERA),
    EITEM(ICLS_DDEMLSERVERW),
    EITEM(ICLS_IME),
    EITEM(ICLS_TOOLTIP),
    NULL,
};

const EnumItem aCharSets[] = {
    EITEM(ANSI_CHARSET),
    EITEM(DEFAULT_CHARSET),
    EITEM(SYMBOL_CHARSET),
    EITEM(SHIFTJIS_CHARSET),
    EITEM(HANGEUL_CHARSET),
    EITEM(HANGUL_CHARSET),
    EITEM(GB2312_CHARSET),
    EITEM(CHINESEBIG5_CHARSET),
    EITEM(OEM_CHARSET),
    EITEM(JOHAB_CHARSET),
    EITEM(HEBREW_CHARSET),
    EITEM(ARABIC_CHARSET),
    EITEM(GREEK_CHARSET),
    EITEM(TURKISH_CHARSET),
    EITEM(VIETNAMESE_CHARSET),
    EITEM(THAI_CHARSET),
    EITEM(EASTEUROPE_CHARSET),
    EITEM(RUSSIAN_CHARSET),
    NULL,
};

const EnumItem aImeHotKeys[] = {
    // Windows for Simplified Chinese Edition hot key ID from 0x10 - 0x2F
    EITEM(IME_CHOTKEY_IME_NONIME_TOGGLE),
    EITEM(IME_CHOTKEY_SHAPE_TOGGLE),
    EITEM(IME_CHOTKEY_SYMBOL_TOGGLE),
    // Windows for Japanese Edition hot key ID from 0x30 - 0x4F
    EITEM(IME_JHOTKEY_CLOSE_OPEN),
    // Windows for Korean Edition hot key ID from 0x50 - 0x6F
    EITEM(IME_KHOTKEY_SHAPE_TOGGLE),
    EITEM(IME_KHOTKEY_HANJACONVERT),
    EITEM(IME_KHOTKEY_ENGLISH),
    // Windows for Traditional Chinese Edition hot key ID from 0x70 - 0x8F
    EITEM(IME_THOTKEY_IME_NONIME_TOGGLE),
    EITEM(IME_THOTKEY_SHAPE_TOGGLE),
    EITEM(IME_THOTKEY_SYMBOL_TOGGLE),
    // direct switch hot key ID from 0x100 - 0x11F
    EITEM(IME_HOTKEY_DSWITCH_FIRST),
    EITEM(IME_HOTKEY_DSWITCH_LAST),
    // IME private hot key from 0x200 - 0x21F
    EITEM(IME_ITHOTKEY_RESEND_RESULTSTR),
    EITEM(IME_ITHOTKEY_PREVIOUS_COMPOSITION),
    EITEM(IME_ITHOTKEY_UISTYLE_TOGGLE),
    EITEM(IME_ITHOTKEY_RECONVERTSTRING),
    EITEM(IME_HOTKEY_PRIVATE_LAST),
    NULL,
};

const EnumItem aCandidateListStyle[] = {
    EITEM(IME_CAND_UNKNOWN),//                0x0000
    EITEM(IME_CAND_READ),//                   0x0001
    EITEM(IME_CAND_CODE),//                   0x0002
    EITEM(IME_CAND_MEANING),//                0x0003
    EITEM(IME_CAND_RADICAL),//                0x0004
    EITEM(IME_CAND_STROKE),//                 0x0005
    NULL
};



enum {
    EI_CLSTYPE = 0,
    EI_CHARSETTYPE,
    EI_IMEHOTKEYTYPE,
    EI_IMECANDIDATESTYLE,
    EI_MAX
};

typedef struct {
    DWORD dwMask;
    const EnumItem* items;
} MaskedEnum;

const MaskedEnum aEnumItems[] = {
    ~0,             aClsTypes,
    ~0,             aCharSets,
    ~0,             aImeHotKeys,
    ~0,             aCandidateListStyle,
};

LPCSTR GetMaskedEnum(WORD wType, DWORD dwValue, LPSTR buf)
{
    const EnumItem* item;
    static char ach[32];

    if (wType >= EI_MAX) {
        strcpy(buf, "Invalid type.");
        return buf;
    }

    dwValue &= aEnumItems[wType].dwMask;

    item = aEnumItems[wType].items;

    for (; item->name; ++item) {
        if (item->value == dwValue) {
            if (buf) {
                strcpy(buf, item->name);
                return buf;
            }
            return item->name;
        }
    }

    if (buf) {
        *buf = 0;
        return buf;
    }

    sprintf(ach, "%x", wType);
    return ach;
}

#define WM_ITEM(x)  { x, #x }

struct {
    DWORD msg;
    LPSTR pszMsg;
} gaMsgs[] = {
    #include "wm.txt"
};

#undef WM_ITEM


#ifdef KERNEL

PETHREAD (*GetCurrentThreadAddress)(USHORT, HANDLE) = DummyGetCurrentThreadAddress;

/************************************************************************\
* Procedure: DummyGetCurrentThreadAddress
*
* Description:
*
* Calls out to the default debug extension dll to get the current thread.
*
* Returns: pEThread of current thread.
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
/*
 */
PETHREAD
DummyGetCurrentThreadAddress(
    USHORT Processor,
    HANDLE hCurrentThread
    )
{
    WCHAR awchKDName[MAX_PATH];
    LPWSTR lpszKDExts;
    HANDLE hmodKDExts;

    /*
     * Get the kernel debugger name and map it to its
     * debug extension dll.
     */
    GetModuleFileNameW(NULL, awchKDName, MAX_PATH);
    _wcslwr(awchKDName);
    if (wcsstr(awchKDName, L"alphakd.exe"))
        lpszKDExts = L"kdextalp.dll";
    else if (wcsstr(awchKDName, L"i386kd.exe"))
        lpszKDExts = L"kdextx86.dll";
    else if (wcsstr(awchKDName, L"mipskd.exe"))
        lpszKDExts = L"kdextmip.dll";
    else if (wcsstr(awchKDName, L"ppckd.exe"))
        lpszKDExts = L"kdextppc.dll";
    else {
        Print("Unknown kernel debugger: %s\n", awchKDName);
        return NULL;
    }

    /*
     * Load the extension dll and get the real procedure name
     */
    hmodKDExts = LoadLibraryW(lpszKDExts);
    if (hmodKDExts == NULL) {
        Print("Could not load %s\n", lpszKDExts);
        return NULL;
    }
    GetCurrentThreadAddress = (PVOID)GetProcAddress(hmodKDExts, "GetCurrentThreadAddress");
    if (GetCurrentThreadAddress == NULL) {
        Print("Could not find GetCurrentThreadAddress\n");
        FreeLibrary(hmodKDExts);
        GetCurrentThreadAddress = DummyGetCurrentThreadAddress;
        return NULL;
    }

    /*
     * Make the call
     */
    return GetCurrentThreadAddress(Processor, hCurrentThread);
}



/************************************************************************\
* Procedure: GetCurrentProcessAddress
*
* Description:
*
* Returns: Current EProcess pointer.
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
PEPROCESS
GetCurrentProcessAddress(
    DWORD    Processor,
    HANDLE   hCurrentThread,
    PETHREAD CurrentThread
    )
{
    ETHREAD Thread;

    if (CurrentThread == NULL) {
        CurrentThread = (PETHREAD)GetCurrentThreadAddress( (USHORT)Processor, hCurrentThread );
        if (CurrentThread == NULL) {
            DEBUGPRINT("GetCurrentProcessAddress: failed to get thread addr.\n");
            return NULL;
        }
    }

    if (!tryMove(Thread, CurrentThread)) {
        DEBUGPRINT("GetCurrentProcessAddress: failed to read thread memory.\n");
        return NULL;
    }

    return CONTAINING_RECORD(Thread.Tcb.ApcState.Process,EPROCESS,Pcb);
}

/************************************************************************\
* Procedure: GetProcessName
*
* 06/27/97 GerardoB Created
*
\************************************************************************/
BOOL
GetProcessName(
    PEPROCESS pEProcess,
    LPWSTR lpBuffer)
{
    UCHAR ImageFileName[16];
    if (GetEProcessData(pEProcess, PROCESS_IMAGEFILENAME, ImageFileName)) {
        swprintf(lpBuffer, L"%.16hs", ImageFileName);
        return TRUE;
    } else {
        Print("Unable to read _EPROCESS at %lx\n", pEProcess);
        return FALSE;
    }
}

/************************************************************************\
* Procedure: GetAppName
*
* Description:
*
* Returns: TRUE for success, FALSE for failure.
*
* 10/6/1995 Created JimA
*
\************************************************************************/
BOOL
GetAppName(
    PETHREAD pEThread,
    PTHREADINFO pti,
    LPWSTR lpBuffer,
    DWORD cbBuffer)
{
    PUNICODE_STRING pstrAppName;
    UNICODE_STRING strAppName;
    BOOL fRead = FALSE;

    if (pti->pstrAppName != NULL) {
        pstrAppName = pti->pstrAppName;
        if (pstrAppName != NULL && tryMove(strAppName, pstrAppName)) {
            cbBuffer = min(cbBuffer - sizeof(WCHAR), strAppName.Length);
            if (tryMoveBlock(lpBuffer, strAppName.Buffer, cbBuffer)) {
                lpBuffer[cbBuffer / sizeof(WCHAR)] = 0;
                fRead = TRUE;
            }
        }
    } else {
        fRead = GetProcessName(pEThread->ThreadsProcess, lpBuffer);
    }

    if (!fRead) {
        wcsncpy(lpBuffer, L"<unknown name>", cbBuffer / sizeof(WCHAR));
    }
    return fRead;
}

#endif // KERNEL


#ifdef KERNEL
/************************************************************************\
* Procedure: PrintMessages
*
* Description: Prints out qmsg structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL PrintMessages(
    PQMSG pqmsgRead)
{
    QMSG qmsg;
    ASYNCSENDMSG asm;
    char *aszEvents[] = {
        "MSG",  // QEVENT_MESSAGE
        "SHO",  // QEVENT_SHOWWINDOW"
        "CMD",  // QEVENT_CANCLEMODE"
        "SWP",  // QEVENT_SETWINDOWPOS"
        "UKS",  // QEVENT_UPDATEKEYSTATE"
        "DEA",  // QEVENT_DEACTIVATE"
        "ACT",  // QEVENT_ACTIVATE"
        "PST",  // QEVENT_POSTMESSAGE"
        "EXE",  // QEVENT_EXECSHELL"
        "CMN",  // QEVENT_CANCELMENU"
        "DSW",  // QEVENT_DESTROYWINDOW"
        "ASY",  // QEVENT_ASYNCSENDMSG"
        "HNG",  // QEVENT_HUNGTHREAD"
        "CMT",  // QEVENT_CANCELMOUSEMOVETRK"
        "NWE",  // QEVENT_NOTIFYWINEVENT"
        "RAC",  // QEVENT_RITACCESSIBILITY"
        "RSO",  // QEVENT_RITSOUND"
        "?  ",  // "?"
        "?  ",  // "?"
        "?  "   // "?"
    };
    #define NQEVENT (sizeof(aszEvents)/sizeof(aszEvents[0]))

    Print("typ pqmsg    hwnd    msg  wParam   lParam   time     ExInfo   dwQEvent pti\n");
    Print("-------------------------------------------------------------------------------\n");

    SAFEWHILE (TRUE) {
        move(qmsg, FIXKP(pqmsgRead));
        if (qmsg.dwQEvent < NQEVENT)
            Print("%s %08lx ", aszEvents[qmsg.dwQEvent], pqmsgRead);
        else
            Print("??? %08lx ", pqmsgRead);

        switch (qmsg.dwQEvent) {
        case QEVENT_ASYNCSENDMSG:
            move(asm, (PVOID)qmsg.msg.wParam);

            Print("%07lx %04lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
                asm.hwnd, asm.message, asm.wParam, asm.lParam,
                qmsg.msg.time, qmsg.ExtraInfo, qmsg.dwQEvent, qmsg.pti);
            break;

        case 0:
        default:
            Print("%07lx %04lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
                qmsg.msg.hwnd, qmsg.msg.message, qmsg.msg.wParam, qmsg.msg.lParam,
                qmsg.msg.time, qmsg.ExtraInfo, qmsg.dwQEvent, qmsg.pti);
            break;

        }

        if (qmsg.pqmsgNext != NULL) {
            if (pqmsgRead == qmsg.pqmsgNext) {
                Print("loop found in message list!");
                return FALSE;
            }
            pqmsgRead = qmsg.pqmsgNext;
        } else {
            return TRUE;
        }
    }
    return TRUE;
}
#endif // KERNEL


/************************************************************************\
* Procedure: GetAndDumpHE
*
* Description: Dumps given handle (dwT) and returns its phe.
*
* Returns: fSuccess
*
* 6/9/1995 Documented SanfordS
*
\************************************************************************/
BOOL
GetAndDumpHE(
    ULONG_PTR dwT,
    PHE phe,
    BOOL fPointerTest)
{

    DWORD dw;
    HEAD head;
    PHE pheT;
    PSHAREDINFO pshi;
    SHAREDINFO shi;
    SERVERINFO si;
    ULONG_PTR cHandleEntries;


    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    dw = HMIndexFromHandle(dwT);

    /*
     * First see if it is a pointer because the handle index is only part of
     * the 32 bit DWORD, and we may mistake a pointer for a handle.
     * HACK: If dwCurPc == 0, then we've recursed with a handle.
     */
    if (!fPointerTest && IS_PTR(dwT)) {
        head.h = NULL;
        move(head, (PVOID)dwT);
        if (head.h != NULL) {
            if (GetAndDumpHE((ULONG_PTR)head.h, phe, TRUE)) {
                return TRUE;
            }
        }
    }

    /*
     * Is it a handle? Does it's index fit our table length?
     */
    GETSHAREDINFO(pshi);
    move(shi, pshi);
    move(si, shi.psi);
    cHandleEntries = si.cHandleEntries;
    if (dw >= cHandleEntries)
        return FALSE;

    /*
     * Grab the handle entry and see if it is ok.
     */
    pheT = shi.aheList;
    pheT = &pheT[dw];
    move(*phe, pheT);

    /*
     * If the type is too big, it's not a handle.
     */
    if (phe->bType >= TYPE_CTYPES) {
        pheT = NULL;
    } else {
        move(head, phe->phead);
        if (phe->bType != TYPE_FREE) {
            /*
             * See if the object references this handle entry: the clincher
             * for a handle, if it is not FREE.
             */
            if (HMIndexFromHandle(head.h) != dw)
                pheT = NULL;
        }
    }

    if (pheT == NULL) {
        if (!fPointerTest)
            Print("0x%p is not a valid object or handle.\n", dwT);
        return FALSE;
    }

    /*
     * Dump the ownership info and the handle entry info
     */
    Idhe(0, head.h, NULL);
    Print("\n");

    return TRUE;
}


/************************************************************************\
* Procedure: HtoHE
*
* Description:
*
*   Extracts HE and phe from given handle.  Handle cao be just an index.
*   Assumes h is a valid handle.  Returns FALSE only if it's totally wacko.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL HtoHE(
ULONG_PTR h,
HANDLEENTRY *phe,
HANDLEENTRY **pphe) // Optional
{
    SHAREDINFO si, *psi;
    SERVERINFO svi;
    DWORD index;

    index = HMIndexFromHandle(h);
    GETSHAREDINFO(psi);
    if (!tryMove(si, psi)) {
        DEBUGPRINT("HtoHE(%x): SHAREDINFO move failed. Bad symbols?\n", h);
        return FALSE;
    }
    if (!tryMove(svi, si.psi)) {
        DEBUGPRINT("HtoHE(%x): SERVERINFO move failed. Bad symbols?\n", h);
        return FALSE;
    }
    if (index >= svi.cHandleEntries) {
        DEBUGPRINT("HtoHE(%x): index %d is too large.\n", h, index);
        return FALSE;
    }
    if (pphe != NULL) {
        *pphe = &si.aheList[index];
    }
    if (!tryMove(*phe, &si.aheList[index])) {
        DEBUGPRINT("HtoHE(%x): aheList[%d] move failed.\n", h, index);
        return FALSE;
    }
    return TRUE;
}


/************************************************************************\
* Procedure: dbgPtoH
*
* Description: quick conversion of pointer to handle
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL dbgPtoH(
PVOID p,
ULONG_PTR *ph)
{
    THROBJHEAD head;

    if (tryMove(head, p)) {
        *ph = (ULONG_PTR)head.h;
        return TRUE;
    }
    DEBUGPRINT("dbgPtoH(%x): failed.\n", p);
    return FALSE;
}


/************************************************************************\
* Procedure: dbgHtoP
*
* Description: Quick conversion of handle to pointer
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL dbgHtoP(
DWORD h,
PVOID *pp)
{
    HANDLEENTRY he;

    if (HtoHE(h, &he, NULL)) {
        *pp = FIXKP(he.phead);
        return TRUE;
    }
    DEBUGPRINT("dbgHtoP(%x): failed.\n", h);
    return FALSE;
}


/************************************************************************\
* Procedure: GetPfromH
*
* Description: Converts a handle to a pointer and extracts he and phe info.
*
* Returns: pointer for object or NULL on failure.
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
PVOID GetPfromH(
ULONG_PTR h,
HANDLEENTRY **pphe, // optional
HANDLEENTRY *phe)   // optional
{
    HANDLEENTRY he, *pheT;
    HEAD head;

    if (!HtoHE(h, &he, &pheT)) {
        DEBUGPRINT("GetPfromH(%x): failed to get HE.\n", h);
        return NULL;
    }
    if (!tryMove(head, FIXKP(he.phead))) {
        DEBUGPRINT("GetPfromH(%x): failed to get phead.\n", h);
        return NULL;
    }
    if (head.h != (HANDLE)h) {
        DEBUGPRINT("WARNING: Full handle for 0x%x is 0x%08lx.\n", h, head.h);
    }
    if (pphe != NULL) {
        *pphe = pheT;
    }
    if (phe != NULL) {
        *phe = he;
    }
    return FIXKP(he.phead);
}


/************************************************************************\
* Procedure: getHEfromP
*
* Description: Converts a pointer to a handle and extracts the he and
*   phe info.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL getHEfromP(
HANDLEENTRY **pphe, // optional
HANDLEENTRY *phe,
PVOID p)
{
    PVOID pLookup;
    THROBJHEAD head;

    p = FIXKP(p);
    if (!tryMove(head, p)) {
        return FALSE;
    }

    pLookup = GetPfromH((ULONG_PTR)head.h, pphe, phe);
    if (FIXKP(pLookup) != p) {
        DEBUGPRINT("getHEfromP(%x): invalid.\n", p);
        return FALSE;
    }
    return TRUE;
}


/************************************************************************\
* Procedure: HorPtoP
*
* Description:
*
* Generic function to accept either a user handle or pointer value and
* validate it and convert it to a pointer.  type=-1 to allow any non-free
* type.  type=-2 to allow any type.
*
* Returns: pointer or NULL on error.
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
PVOID HorPtoP(
PVOID p,
int type)
{
    HANDLEENTRY he;
    PVOID pT;

    if (p == NULL) {
        DEBUGPRINT("HorPtoP(%x, %d): failed.  got NULL.\n", p, type);
        return NULL;
    }

    p = FIXKP(p);
    if (tryMove(pT, p) && getHEfromP(NULL, &he, p)) {
        /*
         * It was a pointer
         */
        if ((type == -2 || he.bType != TYPE_FREE) &&
                he.bType < TYPE_CTYPES &&
                ((int)type < 0 || he.bType == type)) {
            return (PVOID)FIXKP(he.phead);
        }
    }

    pT = GetPfromH((ULONG_PTR)p, NULL, &he);
    if (pT == NULL) {
        Print("WARNING: dumping %#p even though is not a valid pointer or handle!\n", p);
        return p;  // let it pass anyway so we can see how it got corrupted.
    }

    return FIXKP(pT);
}


/************************************************************************\
* Procedure: DebugGetWindowTextA
*
* Description: Places pwnd title into achDest.  No checks for size are
*   made.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL DebugGetWindowTextA(
    PWND pwnd,
    char *achDest)
{
    WND wnd;
    WCHAR awch[80];

    if (pwnd == NULL) {
        achDest[0] = '\0';
        return FALSE;
    }

    if (!tryMove(wnd, FIXKP(pwnd))) {
        strcpy(achDest, "<< Can't get WND >>");
        return FALSE;
    }

    if (wnd.strName.Buffer == NULL) {
        strcpy(achDest, "<null>");
    } else {
        ULONG cbText;
        cbText = min(sizeof(awch), wnd.strName.Length + sizeof(WCHAR));
        if (!(tryMoveBlock(awch, FIXKP(wnd.strName.Buffer), cbText))) {
            strcpy(achDest, "<< Can't get title >>");
            return FALSE;
        }
        awch[sizeof(awch) / sizeof(WCHAR) - 1] = L'\0';
        RtlUnicodeToMultiByteN(achDest, cbText / sizeof(WCHAR), NULL,
                awch, cbText);
    }
    return TRUE;
}


/************************************************************************\
* Procedure: DebugGetClassNameA
*
* Description: Placed pcls name into achDest.  No checks for size are
*   made.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL DebugGetClassNameA(
    LPSTR lpszClassName,
    char *achDest)
{
    CHAR ach[80];

    if (lpszClassName == NULL) {
        strcpy(achDest, "<null>");
    } else {
        if (!tryMove(ach, FIXKP(lpszClassName))) {
            strcpy(achDest, "<inaccessible>");
        } else {
            strcpy(achDest, ach);
        }
        strcpy(achDest, ach);
    }
    return TRUE;
}

/************************************************************************\
* Procedure: PrintBitField, PrintEndBitField
*
* Description: Printout specified boolean value in a structure.
*  Assuming strlen(pszFieldName) will not exceeds BF_COLUMN_WIDTH.
*
* Returns: None
*
* 10/12/1997 Created HiroYama
*
\************************************************************************/
void PrintBitField(LPSTR pszFieldName, BOOLEAN fValue)
{
    int iWidth;
    int iStart = giBFColumn;

    sprintf(gach1, fValue ? "*%-s " : " %-s ", pszFieldName);

    iWidth = (strlen(gach1) + BF_COLUMN_WIDTH - 1) / BF_COLUMN_WIDTH;
    iWidth *= BF_COLUMN_WIDTH;

    if ((giBFColumn += iWidth) >= BF_MAX_WIDTH) {
        giBFColumn = iWidth;
        Print("%s\n", gaBFBuff);
        iStart = 0;
    }

    sprintf(gaBFBuff + iStart, "%-*s", iWidth, gach1);
}

void PrintEndBitField()
{
    if (giBFColumn != 0) {
        giBFColumn = 0;
        Print("%s\n", gaBFBuff);
    }
}


char *pszObjStr[] = {
                     "Free",
                     "Window",
                     "Menu",
                     "Cursor",
                     "SetWindowPos",
                     "Hook",
                     "Thread Info",
                     "Clip Data",
                     "Call Proc",
                     "Accel Table",
                     "WindowStation",
                     "DeskTop",
                     "DdeAccess",
                     "DdeConv",
                     "DdeExact",
                     "Monitor",
                     "Ctypes",
                     "Console",
                     "Generic"
                    };


#ifdef KERNEL
/***********************************************************************\
* DumpGdiHandleType
*
* Returns: a static buffer address which will contain a >0 length
* string if the type makes sense.
*
* 12/1/1995 Created SanfordS
\***********************************************************************/

LPCSTR GetGDIHandleType(
HANDLE handle)
{
    HOBJ    ho;                             // dump this handle
    PENTRY  pent;                           // base address of hmgr entries
    ENTRY   ent;                            // copy of handle entry
    BASEOBJECT obj;
    ULONG ulTemp;
    static CHAR szT[20];
    ULONG gcMaxHmgr, index;

// filched from gre\hmgr.h
#define INDEX_MASK          ((1 << INDEX_BITS) - 1)
#define HmgIfromH(h)          ((ULONG)(h) & INDEX_MASK)

    szT[0] = '\0';
    ho = (HOBJ) handle;
    moveExpValue(&pent, VAR(gpentHmgr));
    moveExp(&gcMaxHmgr, VAR(gcMaxHmgr));
    index = HmgIfromH((ULONG_PTR) ho);
    if (index > gcMaxHmgr) {
        return szT;
    }
    if (!tryMove(ent,  &(pent[index]))) {
        return szT;
    }
    if (ent.FullUnique != ((ULONG_PTR)ho >> 16)) {
        return szT;
    }
    if (!tryMove(obj, ent.einfo.pobj)) {
        return szT;
    }
    if (obj.hHmgr != ho) {
        return szT;
    }
    ulTemp = (ULONG) ent.Objt;

    switch(ulTemp) {
    case DEF_TYPE:
        strcpy(szT, "DEF");
        break;

    case DC_TYPE:
        strcpy(szT, "DC");
        break;

    case RGN_TYPE:
        strcpy(szT, "RGN");
        break;

    case SURF_TYPE:
        strcpy(szT, "SURF");
        break;

    case PATH_TYPE:
        strcpy(szT, "PATH");
        break;

    case PAL_TYPE:
        strcpy(szT, "PAL");
        break;

    case ICMLCS_TYPE:
        strcpy(szT, "ICMLCS");
        break;

    case LFONT_TYPE:
        strcpy(szT, "LFONT");
        break;

    case RFONT_TYPE:
        strcpy(szT, "RFONT");
        break;

    case PFE_TYPE:
        strcpy(szT, "PFE");
        break;

    case PFT_TYPE:
        strcpy(szT, "PFT");
        break;

    case ICMCXF_TYPE:
        strcpy(szT, "ICMCXF");
        break;

    case SPRITE_TYPE:
        strcpy(szT, "SPRITE");
        break;

    case SPACE_TYPE:
        strcpy(szT, "SPACE");
        break;

    case META_TYPE:
        strcpy(szT, "META");
        break;

    case EFSTATE_TYPE:
        strcpy(szT, "EFSTATE");
        break;

    case BMFD_TYPE:
        strcpy(szT, "BMFD");
        break;

    case VTFD_TYPE:
        strcpy(szT, "VTFD");
        break;

    case TTFD_TYPE:
        strcpy(szT, "TTFD");
        break;

    case RC_TYPE:
        strcpy(szT, "RC");
        break;

    case TEMP_TYPE:
        strcpy(szT, "TEMP");
        break;

    case DRVOBJ_TYPE:
        strcpy(szT, "DRVOBJ");
        break;

    case DCIOBJ_TYPE:
        strcpy(szT, "DCIOBJ");
        break;

    case SPOOL_TYPE:
        strcpy(szT, "SPOOL");
        break;

    default:
        ulTemp = LO_TYPE(ent.FullUnique << TYPE_SHIFT);
        switch (ulTemp) {
        case LO_BRUSH_TYPE:
            strcpy(szT, "BRUSH");
            break;

        case LO_PEN_TYPE:
            strcpy(szT, "LO_PEN");
            break;

        case LO_EXTPEN_TYPE:
            strcpy(szT, "LO_EXTPEN");
            break;

        case CLIENTOBJ_TYPE:
            strcpy(szT, "CLIENTOBJ");
            break;

        case LO_METAFILE16_TYPE:
            strcpy(szT, "LO_METAFILE16");
            break;

        case LO_METAFILE_TYPE:
            strcpy(szT, "LO_METAFILE");
            break;

        case LO_METADC16_TYPE:
            strcpy(szT, "LO_METADC16");
            break;
        }
    }
    return szT;
}

#endif // KERNEL


/***********************************************************************\
* Isas
*
* Analyzes the stack.  Looks at a range of dwords and tries to make
* sense out of them.  Identifies handles, user objects, and code
* addresses.
*
* Returns: fSuccess
*
* 11/30/1995 Created SanfordS
\***********************************************************************/

VOID DirectAnalyze(
ULONG_PTR dw,
ULONG_PTR adw,
BOOL fNoSym)
{
    PHE         phe;
    HANDLEENTRY he;
    DWORD       index;
    DWORD_PTR   dwOffset;
    WORD        uniq, w, aw;
    HEAD        head;
    CHAR        ach[80];
#ifdef KERNEL
    LPCSTR      psz;
#endif

    Print("0x%p ", dw);
    if (HIWORD(dw) != 0) {
        /*
         * See if its a handle
         */
        index = HMIndexFromHandle(dw);
        if (index < gSi.cHandleEntries) {
            uniq = HMUniqFromHandle(dw);
            phe = &gShi.aheList[index];
            move(he, phe);
            if (he.wUniq == uniq) {
                Print("= a %s handle. ", pszObjStr[he.bType]);
                fNoSym = TRUE;
            }
        }
#ifdef KERNEL
        /*
         * See if its a GDI object handle
         */
        psz = GetGDIHandleType((HANDLE)dw);
        if (*psz) {
            Print("= a GDI %s type handle. ", psz);
            fNoSym = TRUE;
        }
#endif // KERNEL
        /*
         * See if its an object pointer
         */
        if (tryMove(head, (PVOID)dw)) {
            if (head.h) {
                index = HMIndexFromHandle(head.h);
                if (index < gSi.cHandleEntries) {
                    phe = &gShi.aheList[index];
                    move(he, phe);
                    if (he.phead == (PVOID)dw) {
                        Print("= a pointer to a %s.", pszObjStr[he.bType]);
                        fNoSym = TRUE;
                    }
                }
            }
            /*
             * Does this reference the stack itself?
             */
            w = HIWORD(dw);
            aw = HIWORD(adw);
            if (w == aw || w == aw - 1 || w == aw + 1) {
                Print("= Stack Reference ");
                fNoSym = TRUE;
            }
            if (!fNoSym) {
                /*
                 * Its accessible so print its symbolic reference
                 */
                GetSym((PVOID)dw, ach, &dwOffset);
                if (*ach) {
                    Print("= symbol \"%s\"", ach);
                    if (dwOffset) {
                        Print(" + %x", dwOffset);
                    }
                }
            }
        }
    }
    Print("\n");
}



BOOL Isas(
DWORD opts,
PVOID param1,
PVOID param2)
{
    PSHAREDINFO pshi;
    DWORD count = PtrToUlong(param2);
    LPDWORD pdw;
    DWORD_PTR dw;

    if (param1 == 0) {
        return FALSE;
    }
    /*
     * Set up globals for speed.
     */
    GETSHAREDINFO(pshi);
    move(gShi, pshi);

    if (!tryMove(gSi, gShi.psi)) {
        Print("Could not access shared info\n");
        return TRUE;
    }

    if (opts & OFLAG(d)) {
        DirectAnalyze((ULONG_PTR)param1, 0, OFLAG(s) & opts);
    } else {
        pdw = param1;
        if (pdw == NULL) {
            Print("Hay bud, give me an address to look analyze.\n");
            return FALSE;
        }
        if (count == 0) {
            count = 25;    // default span
        }
        Print("--- Stack analysis ---\n");
        for ( ; count; count--, pdw++) {
            if (IsCtrlCHit()) {
                break;
            }
            Print("[0x%p]: ", pdw);
            if (tryMove(dw, pdw))
                DirectAnalyze(dw, (DWORD_PTR)pdw, OFLAG(s) & opts);
            else
                Print("No access\n");
        }
    }
    return TRUE;
}



#ifdef KERNEL

/************************************************************************\
* Procedure: DumpAtomTable
*
* Description: Dumps an atom or entire atom table.
*
* Returns:  fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
VOID DumpAtomTable(
PRTL_ATOM_TABLE *ppat,
ATOM a)
{
    RTL_ATOM_TABLE at, *pat;
    RTL_ATOM_TABLE_ENTRY ate, *pate;
    int iBucket;
    LPWSTR pwsz;
    BOOL fFirst;

    move(pat, ppat);
    if (pat == NULL) {
        Print("is not initialized.\n");
        return;
    }
    move(at, pat);
    if (a) {
        Print("\n");
    } else {
        Print("at %x\n", pat);
    }
    for (iBucket = 0; iBucket < (int)at.NumberOfBuckets; iBucket++) {
        move(pate, &pat->Buckets[iBucket]);
        if (pate != NULL && !a) {
            Print("Bucket %2d:", iBucket);
        }
        fFirst = TRUE;
        SAFEWHILE (pate != NULL) {
            if (!fFirst && !a) {
                Print("          ");
            }
            fFirst = FALSE;
            move(ate, pate);
            pwsz = (LPWSTR)LocalAlloc(LPTR, (ate.NameLength + 1) * sizeof(WCHAR));
            moveBlock(pwsz, FIXKP(&pate->Name), ate.NameLength * sizeof(WCHAR));
            pwsz[ate.NameLength] = L'\0';
            if (a == 0 || a == (ATOM)(ate.HandleIndex | MAXINTATOM)) {
                Print("%hx(%2d) = %ls (%d)%s\n",
                        (ATOM)(ate.HandleIndex | MAXINTATOM),
                        ate.ReferenceCount,
                        pwsz, ate.NameLength,
                        ate.Flags & RTL_ATOM_PINNED ? " pinned" : "");

                if (a) {
                    LocalFree(pwsz);
                    return;
                }
            }
            LocalFree(pwsz);
            if (pate == ate.HashLink) {
                Print("Bogus hash link at %x\n", pate);
                break;
            }
            pate = ate.HashLink;
        }
    }
    if (a)
        Print("\n");
}


/************************************************************************\
* Procedure: Iatom
*
* Description: Dumps an atom or the entire local USER atom table.
*
* Returns:  fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Iatom(
DWORD opts,
PVOID param1)
{
    PRTL_ATOM_TABLE *ppat;
    ATOM a;
    PWINDOWSTATION pwinsta;

    UNREFERENCED_PARAMETER(opts);

    try {
        a = (ATOM)param1;

        ppat = EvalExp(VAR(UserAtomTableHandle));
        if (ppat != NULL) {
            Print("\nPrivate atom table for WIN32K ");
            DumpAtomTable(ppat, a);
        }

        FOREACHWINDOWSTATION(pwinsta)
            ppat = (PRTL_ATOM_TABLE *)&pwinsta->pGlobalAtomTable;
            if (ppat != NULL) {
                Print("\nGlobal atom table for window station %lx ",
                      pwinsta);
                DumpAtomTable(ppat, a);
            }
        NEXTEACHWINDOWSTATION(pwinsta);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
    return TRUE;
}
#endif // KERNEL

#ifndef KERNEL
/************************************************************************\
* Procedure: DumpConvInfo
*
* Description: Dumps DDEML client conversation info structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL DumpConvInfo(
PCONV_INFO pcoi)
{
    CL_CONV_INFO coi;
    ADVISE_LINK al;
    XACT_INFO xi;

    move(coi, pcoi);
    Print("    next              = 0x%08lx\n", coi.ci.next);
    Print("    pcii              = 0x%08lx\n", coi.ci.pcii);
    Print("    hUser             = 0x%08lx\n", coi.ci.hUser);
    Print("    hConv             = 0x%08lx\n", coi.ci.hConv);
    Print("    laService         = 0x%04x\n",  coi.ci.laService);
    Print("    laTopic           = 0x%04x\n",  coi.ci.laTopic);
    Print("    hwndPartner       = 0x%08lx\n", coi.ci.hwndPartner);
    Print("    hwndConv          = 0x%08lx\n", coi.ci.hwndConv);
    Print("    state             = 0x%04x\n",  coi.ci.state);
    Print("    laServiceRequested= 0x%04x\n",  coi.ci.laServiceRequested);
    Print("    pxiIn             = 0x%08lx\n", coi.ci.pxiIn);
    Print("    pxiOut            = 0x%08lx\n", coi.ci.pxiOut);
    SAFEWHILE (coi.ci.pxiOut) {
        move(xi, coi.ci.pxiOut);
        Print("      hXact           = (0x%08lx)->0x%08lx\n", xi.hXact, coi.ci.pxiOut);
        coi.ci.pxiOut = xi.next;
    }
    Print("    dmqIn             = 0x%08lx\n", coi.ci.dmqIn);
    Print("    dmqOut            = 0x%08lx\n", coi.ci.dmqOut);
    Print("    aLinks            = 0x%08lx\n", coi.ci.aLinks);
    Print("    cLinks            = 0x%08lx\n", coi.ci.cLinks);
    SAFEWHILE (coi.ci.cLinks--) {
        move(al, coi.ci.aLinks++);
        Print("      pLinkCount = 0x%08x\n", al.pLinkCount);
        Print("      wType      = 0x%08x\n", al.wType);
        Print("      state      = 0x%08x\n", al.state);
        if (coi.ci.cLinks) {
            Print("      ---\n");
        }
    }
    if (coi.ci.state & ST_CLIENT) {
        Print("    hwndReconnect     = 0x%08lx\n", coi.hwndReconnect);
        Print("    hConvList         = 0x%08lx\n", coi.hConvList);
    }

    return TRUE;
}
#endif // !KERNEL




#ifndef KERNEL
/************************************************************************\
* Procedure: GetTargetTEB
*
* Description: Retrieves the target thread's TEB
*
* Returns: fSuccess
*
* 6/15/1995 Created SanfordS
*
\************************************************************************/
BOOL
GetTargetTEB(
PTEB pteb,
PTEB *ppteb) // OPTIONAL
{
    NTSTATUS Status;
    THREAD_BASIC_INFORMATION ThreadInformation;

    Status = NtQueryInformationThread( hCurrentThread,
                                       ThreadBasicInformation,
                                       &ThreadInformation,
                                       sizeof( ThreadInformation ),
                                       NULL);
    if (NT_SUCCESS( Status )) {
        if (ppteb != NULL) {
            *ppteb = (PTEB)ThreadInformation.TebBaseAddress;
        }
        return tryMove(*pteb, (LPVOID)ThreadInformation.TebBaseAddress);
    }
    return FALSE;
}
#endif // !KERNEL



#ifndef KERNEL
/************************************************************************\
* Procedure: FixKernelPointer
*
* Description: Used to convert a kernel object pointer into its client-
* side equivalent.  Client pointers and NULL are unchanged.
*
* Returns: pClient
*
* 6/15/1995 Created SanfordS
*
\************************************************************************/
PVOID
FixKernelPointer(
PVOID pKernel)
{
    static TEB teb;
    static PTEB pteb = NULL;
    static PVOID HighestUserAddress;

    if (pKernel == NULL) {
        return NULL;
    }
    if (HighestUserAddress == 0) {
        SYSTEM_BASIC_INFORMATION SystemInformation;

        if (NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation,
                                                 &SystemInformation,
                                                 sizeof(SystemInformation),
                                                 NULL))) {
            HighestUserAddress = (PVOID)SystemInformation.MaximumUserModeAddress;
        } else {
            // Query failed.  Assume usermode is the low half of the address
            // space.
            HighestUserAddress = (PVOID)MAXINT_PTR;
        }
    }

    if (pKernel <= HighestUserAddress) {
        return pKernel;
    }
    if (pteb == NULL) {
        GetTargetTEB(&teb, &pteb);
    }
    return (PVOID)(((PBYTE)pKernel) - ((PCLIENTINFO)(&teb.Win32ClientInfo[0]))->ulClientDelta);
}

void *
RebaseSharedPtr(void * p)
{
    PSHAREDINFO     pshi;
    ULONG_PTR       ulSharedDelta;

    moveExp(&pshi, VAR(gSharedInfo));
    move(ulSharedDelta, &pshi->ulSharedDelta);

    return (p) ? (PVOID)((PBYTE)(p) - ulSharedDelta) : NULL;
}

#endif // !KERNEL

/************************************************************************\
* Procedure: userexts_bsearch
*
* 08/09/98 HiroYama     Ported from VC5 CRT
*
\************************************************************************/
void* userexts_bsearch (
        const void *key,
        const void *base,
        size_t num,
        size_t width,
        int (*compare)(const void*, const void*))
{
    char* lo = (char*)base;
    char* hi = (char*)base + (num - 1) * width;
    char* mid;
    unsigned int half;
    int result;

    while (lo <= hi) {
        if (half = num / 2) {
            mid = lo + ((num & 1) ? half : (half - 1)) * width;
            if (!(result = compare(key, mid)))
                return mid;
            else if (result < 0) {
                hi = mid - width;
                num = (num & 1) ? half : half-1;
            }
            else {
                lo = mid + width;
                num = half;
            }
        }
        else if (num) {
            return compare(key, lo) ? NULL : lo;
        }
        else {
            break;
        }
    }

    return NULL;
}


/************************************************************************\
* Procedure: GetVKeyName
*
* 08/09/98 HiroYama     Created
*
\************************************************************************/

typedef struct {
    DWORD dwVKey;
    const char* name;
} VKeyDef;

int compareVKey(const VKeyDef* a, const VKeyDef* b)
{
    return a->dwVKey - b->dwVKey;
}

#define VKEY_ITEM(x) { x, #x }

const VKeyDef gVKeyDef[] = {
#include "../vktbl.txt"
};

const char* _GetVKeyName(DWORD dwVKey, int n)
{
    int i;

    /*
     * If dwVKey is one of alphabets or numerics, there's no VK_ macro defined.
     */
    if ((dwVKey >= 'A' && dwVKey <= 'Z') || (dwVKey >= '0' && dwVKey <= '9')) {
        static char buffer[] = "VK_*";

        if (n != 0) {
            return "";
        }
        buffer[ARRAY_SIZE(buffer) - 2] = (BYTE)dwVKey;
        return buffer;
    }

    /*
     * Search the VKEY table.
     */
    for (i = 0; i < ARRAY_SIZE(gVKeyDef); ++i) {
        const VKeyDef* result = gVKeyDef + i;

        if (result->dwVKey == dwVKey) {
            for (; i < ARRAY_SIZE(gVKeyDef); ++i) {
                if (gVKeyDef[i].dwVKey != dwVKey) {
                    return "";
                }
                if (&gVKeyDef[i] - result == n) {
                    return gVKeyDef[i].name;
                }
            }
        }
    }

    /*
     * VKey name is not found.
     */
    return "";
}

const char* GetVKeyName(DWORD dwVKey)
{
    static char buf[256];
    const char* delim = "";
    int n = 0;

    buf[0] = 0;

    for (n = 0; n < ARRAY_SIZE(gVKeyDef); ++n) {
        const char* name = _GetVKeyName(dwVKey, n);
        if (*name) {
            strcat(buf, delim);
            strcat(buf, name);
            delim = " / ";
        }
        else {
            break;
        }
    }
    return buf;
}

#undef VKEY_ITEM





#ifdef KERNEL
/************************************************************************\
* Procedure: DumpClassList
*
*
* 05/18/98 GerardoB     Extracted from Idcls
\************************************************************************/
void DumpClassList (DWORD opts, PCLS pcls, BOOL fPrivate)
{
    PCLS pclsClone;
    CLS cls, clsClone;

    SAFEWHILE (pcls != NULL) {
        if (!tryMove(cls, pcls)) {
            Print("  Private class\t\tPCLS @ 0x%lx - inaccessible, skipping...\n", pcls);
            break;
        }
        Print("  %s class\t\t", fPrivate ? "Private" : "Public ");
        Idcls(opts, pcls);

        if (cls.pclsClone != NULL) {
            pclsClone = cls.pclsClone;
            SAFEWHILE (pclsClone != NULL) {
                if (!tryMove(clsClone, pclsClone)) {
                    Print("Could not access clone class at %x, skipping clones...\n", pclsClone);
                    break;
                }
                Print("  %s class clone\t", fPrivate ? "Private" : "Public ");
                Idcls(opts, pclsClone);
                pclsClone = clsClone.pclsNext;
            }
        }

        pcls = cls.pclsNext;
    }
}
/************************************************************************\
* Procedure: Idcls
*
* Description: Dumps window class structures
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idcls(
    DWORD opts,
    PVOID param1)
{
    char ach[120];
    DWORD_PTR dwOffset;
    CLS localCLS;
    PCLS pcls = param1;
    PROCESSINFO pi;
    PPROCESSINFO ppi;

    if (param1 == 0) {

        FOREACHPPI(ppi)
            Print("\nClasses for process %x:\n", ppi);
            move(pi, ppi);
            DumpClassList(opts, pi.pclsPrivateList, TRUE);
            DumpClassList(opts, pi.pclsPublicList, FALSE);
        NEXTEACHPPI()

        Print("\nGlobal Classes:\n");
        moveExpValuePtr(&pcls, VAR(gpclsList));
        SAFEWHILE (pcls) {
            Print("  Global Class\t\t");
            Idcls(opts, pcls);
            move(pcls, &pcls->pclsNext);
        }
        return TRUE;
    }

    /*
     * Dump class list for a process
     */
    if (opts & OFLAG(p)) {
        opts &= ~OFLAG(p);
        Print("\nClasses for process %x:\n", param1);
        move(pi, param1);
        DumpClassList(opts, pi.pclsPrivateList, TRUE);
        DumpClassList(opts, pi.pclsPublicList, FALSE);
        return TRUE;
    }

    move(localCLS, pcls);

    DebugGetClassNameA(localCLS.lpszAnsiClassName, ach);
    Print("PCLS @ 0x%lx \t(%s)\n", pcls, ach);
    if (opts & OFLAG(v)) {
        Print("\t pclsNext          @0x%p\n"
              "\t atomClassNameAtom  0x%04x\n"
              "\t fnid               0x%04x\n"
              "\t pDCE              @0x%p\n"
              "\t cWndReferenceCount 0x%08lx\n"
              "\t flags              %s\n",

              localCLS.pclsNext,
              localCLS.atomClassName,
              localCLS.fnid,
              localCLS.pdce,
              localCLS.cWndReferenceCount,
              GetFlags(GF_CSF, (WORD)localCLS.CSF_flags, NULL, TRUE));

        if (localCLS.lpszClientAnsiMenuName) {
            move(ach, localCLS.lpszClientAnsiMenuName);
            ach[sizeof(ach) - 1] = '\0';
        } else {
            ach[0] = '\0';
        }
        Print("\t lpszClientMenu    @0x%p (%s)\n",
              localCLS.lpszClientUnicodeMenuName,
              ach);

        Print("\t hTaskWow           0x%08lx\n"
              "\t spcpdFirst        @0x%p\n"
              "\t pclsBase          @0x%p\n"
              "\t pclsClone         @0x%p\n",
              localCLS.hTaskWow,
              localCLS.spcpdFirst,
              localCLS.pclsBase,
              localCLS.pclsClone);

        GetSym((LPVOID)localCLS.lpfnWndProc, ach, &dwOffset);
        Print("\t style              %s\n"
              "\t lpfnWndProc       @0x%p = \"%s\" \n"
              "\t cbclsExtra         0x%08lx\n"
              "\t cbwndExtra         0x%08lx\n"
              "\t hModule            0x%08lx\n"
              "\t spicn             @0x%p\n"
              "\t spcur             @0x%p\n"
              "\t hbrBackground      0x%08lx\n"
              "\t spicnSm           @0x%p\n",
              GetFlags(GF_CS, localCLS.style, NULL, TRUE),
              localCLS.lpfnWndProc, ach,
              localCLS.cbclsExtra,
              localCLS.cbwndExtra,
              localCLS.hModule,
              localCLS.spicn,
              localCLS.spcur,
              localCLS.hbrBackground,
              localCLS.spicnSm);
    }

    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL



LPSTR ProcessName(
    PPROCESSINFO ppi)
{
    W32PROCESS w32p;
    static UCHAR ImageFileName[16];

    move(w32p, ppi);
    GetEProcessData(w32p.Process, PROCESS_IMAGEFILENAME, ImageFileName);
    if (ImageFileName[0]) {
        return ImageFileName;
    } else {
        return "System";
    }
}



VOID PrintCurHeader()
{
    Print("P = Process Owned.\n");
    Print("P .pcursor flg rt ..lpName aMod bpp ..cx ..cy xHot yHot .hbmMask hbmColor\n");
}


VOID PrintCurData(
PCURSOR pcur,
DWORD opts)
{
    CURSOR cur;

    move(cur, pcur);

    if ((opts & OFLAG(x)) &&
            cur.CURSORF_flags & (CURSORF_ACONFRAME | CURSORF_LINKED)) {
        return; // skip acon frame or linked objects.
    }
    if (cur.CURSORF_flags & CURSORF_ACON) {
        ACON acon;

        if (opts & OFLAG(a)) {
            Print("--------------\n");
        }
        if (opts & OFLAG(o)) {
            Print("Owner:%#p(%s)\n", cur.head.ppi, ProcessName(cur.head.ppi));
        }
        move(acon, pcur);
        if (opts & OFLAG(v)) {
            Print("\nACON @%x:\n", pcur);
            Print("  ppiOwner       = %#p\n", (DWORD_PTR)cur.head.ppi);
            Print("  CURSORF_flags  = %s\n", GetFlags(GF_CURSORF, cur.CURSORF_flags, NULL, TRUE));
            Print("  strName        = %#p\n", (DWORD_PTR)cur.strName.Buffer);
            Print("  atomModName    = %x\n", cur.atomModName);
            Print("  rt             = %x\n", cur.rt);
        } else {
            Print("%c %8x %3x %2x %8x %4x --- ACON (%d frames)\n",
                cur.head.ppi ? 'P' : ' ',
                pcur,
                cur.CURSORF_flags,
                cur.rt,
                cur.strName.Buffer,
                cur.atomModName,
                acon.cpcur);
        }
        if (opts & OFLAG(a)) {
            Print("%d animation sequences, currently at step %d.\n",
                    acon.cicur,
                    acon.iicur);
            while (acon.cpcur--) {
                move(pcur, acon.aspcur++);
                PrintCurData(pcur, opts & ~(OFLAG(x) | OFLAG(o)));
            }
            Print("--------------\n");
        }
    } else {
        if (opts & OFLAG(v)) {
            Print("\nCursor/Icon @%x:\n", pcur);
            Print("  ppiOwner       = %#p(%s)\n",
                  (DWORD_PTR)cur.head.ppi,
                  ProcessName(cur.head.ppi));
            Print("  pcurNext       = %x\n", cur.pcurNext);
            Print("  CURSORF_flags  = %s\n", GetFlags(GF_CURSORF, cur.CURSORF_flags, NULL, TRUE));
            Print("  strName        = %#p\n", (DWORD_PTR)cur.strName.Buffer);
            Print("  atomModName    = %x\n", cur.atomModName);
            Print("  rt             = %x\n", cur.rt);
            Print("  bpp            = %x\n", cur.bpp);
            Print("  cx             = %x\n", cur.cx);
            Print("  cy             = %x\n", cur.cy);
            Print("  xHotspot       = %x\n", cur.xHotspot);
            Print("  yHotspot       = %x\n", cur.yHotspot);
            Print("  hbmMask        = %x\n", cur.hbmMask);
            Print("  hbmColor       = %x\n", cur.hbmColor);
        } else {
            if (opts & OFLAG(o)) {
                Print("Owner:%x(%s)\n", cur.head.ppi, ProcessName(cur.head.ppi));
            }
            Print("%c %8x %3x %2x %8x %4x %3x %4x %4x %4x %4x %8x %8x\n",
                cur.head.ppi ? 'P' : ' ',
                pcur,
                cur.CURSORF_flags,
                cur.rt,
                cur.strName.Buffer,
                cur.atomModName,
                cur.bpp,
                cur.cx,
                cur.cy,
                cur.xHotspot,
                cur.yHotspot,
                cur.hbmMask,
                cur.hbmColor);
        }
    }
}


/************************************************************************\
* Procedure: Idcur
*
* Description: Dump cursor structures
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idcur(
    DWORD opts,
    PVOID param1)
{
    PROCESSINFO pi, *ppi, *ppiDesired = NULL;
    CURSOR cur, *pcur;
    int cCursors = 0;
    INT_PTR idDesired = 0;
    HANDLEENTRY he, *phe;
    int i;

    if (OFLAG(p) & opts) {
        ppiDesired = (PPROCESSINFO)param1;
        param1 = NULL;
    } else if (OFLAG(i) & opts) {
        idDesired = (INT_PTR)param1;
        param1 = NULL;
    }
    if (param1 == NULL) {
        if (!(OFLAG(v) & opts)) {
            PrintCurHeader();
        }
        moveExpValuePtr(&pcur, VAR(gpcurFirst));
        if (pcur != NULL && ppiDesired == NULL) {
            Print("Global cache:\n");
            while (pcur) {
                move(cur, pcur);
                if (!idDesired || ((INT_PTR)cur.strName.Buffer == idDesired)) {
                    if (cur.head.ppi != NULL) {
                        Print("Wrong cache! Owned by %x! --v\n", cur.head.ppi);
                    }
                    PrintCurData((PCURSOR)pcur, opts);
                }
                pcur = cur.pcurNext;
            }
        }
        FOREACHPPI(ppi)
            if (ppiDesired == NULL || ppiDesired == ppi) {
                if (tryMove(pi, ppi)) {
                    if (pi.pCursorCache) {
                        Print("Cache for process %x(%s):\n", ppi, ProcessName(ppi));
                        pcur = pi.pCursorCache;
                        while (pcur) {
                            if (tryMove(cur, pcur)) {
                                if (!idDesired || ((INT_PTR)cur.strName.Buffer == idDesired)) {
                                    if (cur.head.ppi != ppi) {
                                        Print("Wrong cache! Owned by %x! --v\n", cur.head.ppi);
                                    }
                                    PrintCurData((PCURSOR)pcur, opts);
                                }
                                pcur = cur.pcurNext;
                            } else {
                                Print("Could not access %x.\n", pcur);
                                break;
                            }
                        }
                    }
                } else {
                    Print("Failed to access ppi %x.\n", ppi);
                }
            }
        NEXTEACHPPI();
        Print("Non-cached cursor objects:\n");
        FOREACHHANDLEENTRY(phe, he, i)
            if (he.bType == TYPE_CURSOR) {
                CURSOR cur;

                if (tryMove(cur, he.phead)) {
                    if (!(cur.CURSORF_flags & (CURSORF_LINKED | CURSORF_ACONFRAME)) &&
                            (!idDesired || (INT_PTR)cur.strName.Buffer == idDesired) &&
                            (ppiDesired == NULL || ppiDesired == cur.head.ppi)) {
                        Print("%x:", i);
                        PrintCurData((PCURSOR)he.phead, opts | OFLAG(x) | OFLAG(o));
                    }
                } else {
                    Print("Could not access phead(%x) of handle %x.\n", he.phead, i);
                }
                cCursors++;
            }
        NEXTEACHHANDLEENTRY()
        Print("%d Cursors/Icons Total.\n", cCursors);
        return TRUE;
    }

    pcur = HorPtoP(param1, TYPE_CURSOR);
    if (pcur == NULL) {
        Print("%8x : Invalid cursor handle or pointer.\n", param1);
        return FALSE;
    }

    if (!(OFLAG(v) & opts)) {
        PrintCurHeader();
    }
    PrintCurData(pcur, opts);
    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/************************************************************************\
* Procedure: ddeexact
*
* Description: Dumps DDEML transaction structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL dddexact(
    DWORD_PTR pOrg,
    PXSTATE pxs,
    DWORD opts)
{
    if (opts & OFLAG(v)) {
        Print("    XACT:0x%p\n", pOrg);
        Print("      snext = 0x%08lx\n", pxs->snext);
        Print("      fnResponse = 0x%08lx\n", pxs->fnResponse);
        Print("      hClient = 0x%08lx\n", pxs->hClient);
        Print("      hServer = 0x%08lx\n", pxs->hServer);
        Print("      pIntDdeInfo = 0x%08lx\n", pxs->pIntDdeInfo);
    } else {
        Print("0x%p(0x%08lx) ", pOrg, pxs->flags);
    }
    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/************************************************************************\
* Procedure: ddeconv
*
* Description: Dumps DDE tracking layer conversation structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL dddeconv(
    DWORD_PTR pOrg,
    PDDECONV pddeconv,
    DWORD opts)
{
    DDEIMP ddei;
    XSTATE xs;
    PXSTATE pxs;
    int cX;


    Print("  CONVERSATION-PAIR(0x%p:0x%08lx)\n", pOrg, pddeconv->spartnerConv);
    if (opts & OFLAG(v)) {
        Print("    snext        = 0x%08lx\n", pddeconv->snext);
        Print("    spwnd        = 0x%08lx\n", pddeconv->spwnd);
        Print("    spwndPartner = 0x%08lx\n", pddeconv->spwndPartner);
    }
    if (opts & (OFLAG(v) | OFLAG(r))) {
        if (pddeconv->spxsOut) {
            pxs = pddeconv->spxsOut;
            cX = 0;
            SAFEWHILE (pxs) {
                move(xs, pxs);
                if ((opts & OFLAG(r)) && !cX++) {
                    Print("    Transaction chain:");
                } else {
                    Print("    ");
                }
                dddexact((DWORD_PTR)pxs, &xs, opts);
                if (opts & OFLAG(r)) {
                    pxs = xs.snext;
                } else {
                    pxs = NULL;
                }
                if (!pxs) {
                    Print("\n");
                }
            }
        }
    }
    if (opts & OFLAG(v)) {
        Print("    pfl          = 0x%08lx\n", pddeconv->pfl);
        Print("    flags        = 0x%08lx\n", pddeconv->flags);
        if ((opts & OFLAG(v)) && (opts & OFLAG(r)) && pddeconv->pddei) {
            Print("    pddei    = 0x%08lx\n", pddeconv->pddei);
            move(ddei, pddeconv->pddei);
            Print("    Impersonation info:\n");
            Print("      qos.Length                 = 0x%08lx\n", ddei.qos.Length);
            Print("      qos.ImpersonationLevel     = 0x%08lx\n", ddei.qos.ImpersonationLevel);
            Print("      qos.ContextTrackingMode    = 0x%08lx\n", ddei.qos.ContextTrackingMode);
            Print("      qos.EffectiveOnly          = 0x%08lx\n", ddei.qos.EffectiveOnly);
            Print("      ClientContext              = 0x%08lx\n", &ddei.ClientContext);
            Print("      cRefInit                   = 0x%08lx\n", ddei.cRefInit);
            Print("      cRefConv                   = 0x%08lx\n", ddei.cRefConv);
        }
    }
    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/************************************************************************\
* Procedure: Idde
*
* Description: Dumps DDE tracking layer state and structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idde(
DWORD opts,
PVOID param1)
{
    HEAD head;
    DDECONV ddeconv;
    PSHAREDINFO pshi;
    SHAREDINFO shi;
    SERVERINFO si;
    HANDLEENTRY he;
    DWORD_PTR cHandleEntries;
    HANDLE h;
    WND wnd;
    UINT i;
    PVOID pObj = NULL;
    PHE pheList;
    PROP prop;
    PROPLIST propList;
    DWORD atomDdeTrack;
    XSTATE xs;

    moveExpValue(&atomDdeTrack, VAR(atomDDETrack));
    GETSHAREDINFO(pshi);
    move(shi, pshi);
    move(si, shi.psi);
    cHandleEntries = si.cHandleEntries;
    pheList = shi.aheList;

    if (param1) {
        /*
         * get object param.
         */
        h = (HANDLE)param1;
        i = HMIndexFromHandle(h);
        if (i >= cHandleEntries) {
            move(head, h);
            i = HMIndexFromHandle(head.h);
        }
        if (i >= cHandleEntries) {
            Print("0x%08lx is not a valid object.\n", h);
            return FALSE;
        }
        move(he, &pheList[i]);
        pObj = FIXKP(he.phead);
        /*
         * verify type.
         */
        switch (he.bType) {
        case TYPE_WINDOW:
            move(wnd, pObj);
            move(propList, wnd.ppropList);
            for (i = 0; i < propList.iFirstFree; i++) {
                if (i == 0) {
                    Print("Window 0x%08lx conversations:\n", h);
                }
                move(prop, &propList.aprop[i]);
                if (prop.atomKey == (ATOM)MAKEINTATOM(atomDdeTrack)) {
                    move(ddeconv, (PDDECONV)prop.hData);
                    Print("  ");
                    dddeconv((DWORD_PTR)prop.hData,
                            &ddeconv,
                            opts);
                }
            }
            return TRUE;

        case TYPE_DDECONV:
        case TYPE_DDEXACT:
            break;

        default:
            Print("0x%08lx is not a valid window, conversation or transaction object.\n", h);
            return FALSE;
        }
    }

    /*
     * look for all qualifying objects in the object table.
     */

    Print("DDE objects:\n");
    for (i = 0; i < cHandleEntries; i++) {
        move(he, &pheList[i]);
        if (he.bType == TYPE_DDECONV && (pObj == FIXKP(he.phead) || pObj == NULL)) {
            move(ddeconv, FIXKP(he.phead));
            dddeconv((DWORD_PTR)FIXKP(he.phead),
                    (PDDECONV)&ddeconv,
                    opts);
        }

        if (he.bType == TYPE_DDEXACT && (pObj == NULL || pObj == FIXKP(he.phead))) {
            move(xs, FIXKP(he.phead));
            if (!(opts & OFLAG(v))) {
                Print("  XACT:");
            }
            dddexact((DWORD_PTR)FIXKP(he.phead),
                     (PXSTATE)&xs,
                     opts);
            Print("\n");
        }
    }
    return TRUE;
}
#endif // KERNEL



#ifndef KERNEL
/************************************************************************\
* Procedure: Iddeml
*
* Description: Dumps the DDEML state for this client process.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Iddeml(
    DWORD opts,
    LPSTR lpas)
{
    CHANDLEENTRY he, *phe;
    int cHandles, ch, i;
    DWORD Type;
    DWORD_PTR Instance, Object, Pointer;
    CL_INSTANCE_INFO cii, *pcii;
    ATOM ns;
    SERVER_LOOKUP sl;
    LINK_COUNT lc;
    CL_CONV_INFO cci;
    PCL_CONV_INFO pcci;
    CONVLIST cl;
    HWND hwnd, *phwnd;
    XACT_INFO xi;
    DDEMLDATA dd;
    CONV_INFO ci;

    moveExpValue(&cHandles, "user32!cHandlesAllocated");

    Instance = 0;
    Type = 0;
    Object = 0;
    Pointer = 0;
    SAFEWHILE (*lpas) {
        SAFEWHILE (*lpas == ' ')
            lpas++;

        if (*lpas == 'i') {
            lpas++;
            Instance = (DWORD_PTR)EvalExp(lpas);
            SAFEWHILE (*lpas != ' ' && *lpas != 0)
                lpas++;
            continue;
        }
        if (*lpas == 't') {
            lpas++;
            Type = (DWORD)(DWORD_PTR)EvalExp(lpas);
            SAFEWHILE (*lpas != ' ' && *lpas != 0)
                lpas++;
            continue;
        }
        if (*lpas) {
            Object = Pointer = (DWORD_PTR)EvalExp(lpas);
            SAFEWHILE (*lpas != ' ' && *lpas != 0)
                lpas++;
        }
    }

    /*
     * for each instance for this process...
     */

    moveExpValuePtr(&pcii, "user32!pciiList");
    if (pcii == NULL) {
        Print("No Instances exist.\n");
        return TRUE;
    }
    move(cii, pcii);
    SAFEWHILE(pcii != NULL) {
        pcii = cii.next;
        if (Instance == 0 || (Instance == (DWORD_PTR)cii.hInstClient)) {
            Print("Objects for instance 0x%p:\n", cii.hInstClient);
            ch = cHandles;
            moveExpValuePtr(&phe, "user32!aHandleEntry");
            SAFEWHILE (ch--) {
                move(he, phe++);
                if (he.handle == 0) {
                    continue;
                }
                if (InstFromHandle(cii.hInstClient) != InstFromHandle(he.handle)) {
                    continue;
                }
                if (Type && TypeFromHandle(he.handle) != Type) {
                    continue;
                }
                if (Object && (he.handle != (HANDLE)Object) &&
                    Pointer && he.dwData != Pointer) {
                    continue;
                }
                Print("  (0x%08lx)->0x%08lx ", he.handle, he.dwData);
                switch (TypeFromHandle(he.handle)) {
                case HTYPE_INSTANCE:
                    Print("Instance\n");
                    if (opts & OFLAG(v)) {
                        Print("    next               = 0x%08lx\n", cii.next);
                        Print("    hInstServer        = 0x%08lx\n", cii.hInstServer);
                        Print("    hInstClient        = 0x%08lx\n", cii.hInstClient);
                        Print("    MonitorFlags       = 0x%08lx\n", cii.MonitorFlags);
                        Print("    hwndMother         = 0x%08lx\n", cii.hwndMother);
                        Print("    hwndEvent          = 0x%08lx\n", cii.hwndEvent);
                        Print("    hwndTimeout        = 0x%08lx\n", cii.hwndTimeout);
                        Print("    afCmd              = 0x%08lx\n", cii.afCmd);
                        Print("    pfnCallback        = 0x%08lx\n", cii.pfnCallback);
                        Print("    LastError          = 0x%08lx\n", cii.LastError);
                        Print("    tid                = 0x%08lx\n", cii.tid);
                        Print("    plaNameService     = 0x%08lx\n", cii.plaNameService);
                        Print("    cNameServiceAlloc  = 0x%08lx\n", cii.cNameServiceAlloc);
                        SAFEWHILE (cii.cNameServiceAlloc--) {
                            move(ns, cii.plaNameService++);
                            Print("      0x%04lx\n", ns);
                        }
                        Print("    aServerLookup      = 0x%08lx\n", cii.aServerLookup);
                        Print("    cServerLookupAlloc = 0x%08lx\n", cii.cServerLookupAlloc);
                        SAFEWHILE (cii.cServerLookupAlloc--) {
                            move(sl, cii.aServerLookup++);
                            Print("      laService  = 0x%04x\n", sl.laService);
                            Print("      laTopic    = 0x%04x\n", sl.laTopic);
                            Print("      hwndServer = 0x%08lx\n", sl.hwndServer);
                            if (cii.cServerLookupAlloc) {
                                Print("      ---\n");
                            }
                        }
                        Print("    ConvStartupState   = 0x%08lx\n", cii.ConvStartupState);
                        Print("    flags              = %s\n",
                                GetFlags(GF_IIF, cii.flags, NULL, TRUE));
                        Print("    cInDDEMLCallback   = 0x%08lx\n", cii.cInDDEMLCallback);
                        Print("    pLinkCount         = 0x%08lx\n", cii.pLinkCount);
                        SAFEWHILE (cii.pLinkCount) {
                            move(lc, cii.pLinkCount);
                            cii.pLinkCount = lc.next;
                            Print("      next    = 0x%08lx\n", lc.next);
                            Print("      laTopic = 0x%04x\n", lc.laTopic);
                            Print("      gaItem  = 0x%04x\n", lc.gaItem);
                            Print("      laItem  = 0x%04x\n", lc.laItem);
                            Print("      wFmt    = 0x%04x\n", lc.wFmt);
                            Print("      Total   = 0x%04x\n", lc.Total);
                            Print("      Count   = 0x%04x\n", lc.Count);
                            if (cii.pLinkCount != NULL) {
                                Print("      ---\n");
                            }
                        }
                    }
                    break;

                case HTYPE_ZOMBIE_CONVERSATION:
                    Print("Zombie Conversation\n");
                    if (opts & OFLAG(v)) {
                        DumpConvInfo((PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_SERVER_CONVERSATION:
                    Print("Server Conversation\n");
                    if (opts & OFLAG(v)) {
                        DumpConvInfo((PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_CLIENT_CONVERSATION:
                    Print("Client Conversation\n");
                    if (opts & OFLAG(v)) {
                        DumpConvInfo((PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_CONVERSATION_LIST:

                    if (IsRemoteSession()) {
                        Print("!ddeml for Conversation List doesn't work on HYDRA systems\n");
                    } else {
                        Print("Conversation List\n");
                        if (opts & OFLAG(v)) {
                            move(cl, (PVOID)he.dwData);
                            Print("    pcl   = 0x%08lx\n", he.dwData);
                            Print("    chwnd = 0x%08lx\n", cl.chwnd);
                            i = 0;
                            phwnd = (HWND *)&((PCONVLIST)he.dwData)->ahwnd;
                            SAFEWHILE(cl.chwnd--) {
                                move(hwnd, phwnd++);
                                Print("    ahwnd[%d] = 0x%08lx\n", i, hwnd);
                                pcci = (PCL_CONV_INFO)GetWindowLongPtr(hwnd, GWLP_PCI);
                                SAFEWHILE (pcci) {
                                    move(cci, pcci);
                                    pcci = (PCL_CONV_INFO)cci.ci.next;
                                    Print("      hConv = 0x%08lx\n", cci.ci.hConv);
                                }
                                i++;
                            }
                        }
                    }
                    break;

                case HTYPE_TRANSACTION:
                    Print("Transaction\n");
                    if (opts & OFLAG(v)) {
                        move(xi, (PVOID)he.dwData);
                        Print("    next         = 0x%08lx\n", xi.next);
                        Print("    pcoi         = 0x%08lx\n", xi.pcoi);
                        move(ci, xi.pcoi);
                        Print("      hConv      = 0x%08lx\n", ci.hConv);
                        Print("    hUser        = 0x%08lx\n", xi.hUser);
                        Print("    hXact        = 0x%08lx\n", xi.hXact);
                        Print("    pfnResponse  = 0x%08lx\n", xi.pfnResponse);
                        Print("    gaItem       = 0x%04x\n",  xi.gaItem);
                        Print("    wFmt         = 0x%04x\n",  xi.wFmt);
                        Print("    wType;       = 0x%04x\n",  xi.wType);
                        Print("    wStatus;     = 0x%04x\n",  xi.wStatus);
                        Print("    flags;       = %s\n",
                                GetFlags(GF_XI, xi.flags, NULL, TRUE));
                        Print("    state;       = 0x%04x\n",  xi.state);
                        Print("    hDDESent     = 0x%08lx\n", xi.hDDESent);
                        Print("    hDDEResult   = 0x%08lx\n", xi.hDDEResult);
                    }
                    break;

                case HTYPE_DATA_HANDLE:
                    Print("Data Handle\n");
                    if (opts & OFLAG(v)) {
                        move(dd, (PVOID)he.dwData);
                        Print("    hDDE     = 0x%08lx\n", dd.hDDE);
                        Print("    flags    = %s\n",
                                GetFlags(GF_HDATA, (WORD)dd.flags, NULL, TRUE));
                    }
                    break;
                }
            }
        }
        if (pcii != NULL) {
            move(cii, pcii);
        }
    }
    return TRUE;
}
#endif // !KERNEL


#ifdef KERNEL
/***************************************************************************\
* ddesk           - dumps list of desktops
* ddesk address   - dumps simple statistics for desktop
* ddesk v address - dumps verbose statistics for desktop
* ddesk h address - dumps statistics for desktop plus handle list
*
* Dump handle table statistics.
*
* 02-21-92 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Iddesk(
DWORD opts,
PVOID param1)
{

    PWINDOWSTATION pwinsta = NULL;
    WINDOWSTATION winsta;
    PDESKTOP pdesk;
    DESKTOP desk;
    WND wnd;
    MENU menu;
    CALLPROCDATA cpd;
    HOOK hook;
    DESKTOPINFO di;
    DWORD cClasses = 0;
    DWORD acHandles[TYPE_CTYPES];
    BOOL abTrack[TYPE_CTYPES];
    HANDLEENTRY he;
    PHE phe;
    DWORD i;
    WCHAR ach[80];
    OBJECT_HEADER Head;
    OBJECT_HEADER_NAME_INFO NameInfo;
    BOOL fMatch;

    /*
     * If there is no address, list all desktops on all terminals.
     */
    if (!param1) {

        FOREACHWINDOWSTATION(pwinsta)

            DEBUGPRINT("WINSTA @ %x\n", pwinsta);
            move(winsta, pwinsta);
            move(Head, OBJECT_TO_OBJECT_HEADER(pwinsta));
            move(NameInfo, ((PCHAR)(OBJECT_TO_OBJECT_HEADER(pwinsta)) - Head.NameInfoOffset));
            move(ach, NameInfo.Name.Buffer);
            ach[NameInfo.Name.Length / sizeof(WCHAR)] = 0;
            Print("Windowstation: %ws\n", ach);
            Print("Other desktops:\n");
            pdesk = winsta.rpdeskList;

            SAFEWHILE (pdesk) {
                Print("Desktop at %x\n", pdesk);
                Iddesk(opts & OFLAG(v) | OFLAG(h), pdesk);
                move(desk, pdesk);
                pdesk = desk.rpdeskNext;
            }

            Print("\n");

        NEXTEACHWINDOWSTATION(pwinsta)

        return TRUE;
    }

    pdesk = (PDESKTOP)param1;
    move(desk, pdesk);

    move(Head, OBJECT_TO_OBJECT_HEADER(pdesk));
    move(NameInfo, ((PCHAR)(OBJECT_TO_OBJECT_HEADER(pdesk)) - Head.NameInfoOffset));
    move(ach, NameInfo.Name.Buffer);
    ach[NameInfo.Name.Length / sizeof(WCHAR)] = 0;
    Print("Name: %ws\n", ach);

    move(Head, OBJECT_TO_OBJECT_HEADER(pdesk));
    Print("# Opens         = %d\n", Head.HandleCount);
    Print("Heap            = %08x\n", desk.pheapDesktop);
    Print("Windowstation   = %08x\n", desk.rpwinstaParent);
    Print("Message pwnd    = %08x\n", desk.spwndMessage);
    Print("Menu pwnd       = %08x\n", desk.spwndMenu);
    Print("System pmenu    = %08x\n", desk.spmenuSys);
    Print("Console thread  = %x\n", desk.dwConsoleThreadId);
    Print("PtiList.Flink   = %08x\n", desk.PtiList.Flink);
    if (!tryMove(di, desk.pDeskInfo)) {
        Print("Unable to get DESKTOPINFO at %x\n", desk.pDeskInfo);
    } else {
        Print("Desktop pwnd    = %08x\n", di.spwnd);
        Print("\tfsHooks     = 0x%08lx\n"
              "\taphkStart\n",
            di.fsHooks);

        DUMPHOOKS("WH_MSGFILTER", WH_MSGFILTER);
        DUMPHOOKS("WH_JOURNALRECORD", WH_JOURNALRECORD);
        DUMPHOOKS("WH_JOURNALPLAYBACK", WH_JOURNALPLAYBACK);
        DUMPHOOKS("WH_KEYBOARD", WH_KEYBOARD);
        DUMPHOOKS("WH_GETMESSAGE", WH_GETMESSAGE);
        DUMPHOOKS("WH_CALLWNDPROC", WH_CALLWNDPROC);
        DUMPHOOKS("WH_CALLWNDPROCRET", WH_CALLWNDPROCRET);
        DUMPHOOKS("WH_CBT", WH_CBT);
        DUMPHOOKS("WH_SYSMSGFILTER", WH_SYSMSGFILTER);
        DUMPHOOKS("WH_MOUSE", WH_MOUSE);
        DUMPHOOKS("WH_HARDWARE", WH_HARDWARE);
        DUMPHOOKS("WH_DEBUG", WH_DEBUG);
        DUMPHOOKS("WH_SHELL", WH_SHELL);
        DUMPHOOKS("WH_FOREGROUNDIDLE", WH_FOREGROUNDIDLE);
        DUMPHOOKS("WH_KEYBOARD_LL", WH_KEYBOARD_LL);
        DUMPHOOKS("WH_MOUSE_LL", WH_MOUSE_LL);
    }

    /*
     * Find all objects allocated from the desktop.
     */
    for (i = 0; i < TYPE_CTYPES; i++) {
        abTrack[i] = FALSE;
        acHandles[i] = 0;
    }
    abTrack[TYPE_WINDOW] = abTrack[TYPE_MENU] =
            abTrack[TYPE_CALLPROC] =
            abTrack[TYPE_HOOK] = TRUE;

    if (opts & OFLAG(v)) {
        Print("Handle          Type\n");
        Print("--------------------\n");
    }

    FOREACHHANDLEENTRY(phe, he, i)
        fMatch = FALSE;
        try {
            switch (he.bType) {
                case TYPE_WINDOW:
                    move(wnd, FIXKP(he.phead));
                    if (wnd.head.rpdesk == pdesk)
                        fMatch = TRUE;
                    break;
                case TYPE_MENU:
                    move(menu, FIXKP(he.phead));
                    if (menu.head.rpdesk == pdesk)
                        fMatch = TRUE;
                    break;
                case TYPE_CALLPROC:
                    move(cpd, FIXKP(he.phead));
                    if (cpd.head.rpdesk == pdesk)
                        fMatch = TRUE;
                    break;
                case TYPE_HOOK:
                    move(hook, FIXKP(he.phead));
                    if (hook.head.rpdesk == pdesk)
                        fMatch = TRUE;
                    break;
                default:
                    break;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ;
        }

        if (!fMatch)
            continue;

        acHandles[he.bType]++;

        if (opts & OFLAG(v)) {
            Print("0x%08lx %c    %s\n",
                    i,
                    (he.bFlags & HANDLEF_DESTROY) ? '*' : ' ',
                    aszTypeNames[he.bType]);
        }
    NEXTEACHHANDLEENTRY()

    if (!(opts & OFLAG(v))) {
        Print("Count           Type\n");
        Print("--------------------\n");
        Print("0x%08lx      Class\n", cClasses);
        for (i = 0; i < TYPE_CTYPES; i++) {
            if (abTrack[i])
                Print("0x%08lx      %s\n", acHandles[i], aszTypeNames[i]);
        }
    }

    Print("\n");
    return TRUE;
}
#endif // KERNEL


BOOL IsNumChar(int c, int base)
{
    return ('0' <= c && c <= '9') ||
           (base == 16 && (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')));
}


NTSTATUS
GetInteger(LPSTR psz, int base, int * pi, LPSTR * ppsz)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;

    for (;;) {
        if (IsNumChar(*psz, base)) {
            status = RtlCharToInteger(psz, base, pi);
            if (ppsz && NT_SUCCESS(status)) {
                while (IsNumChar(*psz++, base))
                    ;

                *ppsz = psz;
            }

            break;
        }

        if (*psz != ' ' && *psz != '\t') {
            break;
        }

        psz++;
    }

    return status;
}

BOOL Idf(DWORD opts, LPSTR pszName)
{
    static char *szLevels[8] = {
        "<none>",
        "Errors",
        "Warnings",
        "Errors and Warnings",
        "Verbose",
        "Errors and Verbose",
        "Warnings and Verbose",
        "Errors, Warnings, and Verbose"
    };

    NTSTATUS    status;
    ULONG       ulFlags;
    PSERVERINFO psi;
    WORD        wRipFlags;
    DWORD       wPID;


    if (opts & OFLAG(x)) {
        /*
         * !df -x foo
         * This is an undocumented way to start a remote CMD session named
         * "foo" on the machine that the debugger is running on.
         * Sometimes useful to assist in debugging.
         * Sometimes useful when trying to do dev work from home: if you don't
         * already have a remote cmd.exe session to connect to, you probably
         * have a remote debug session.  You can use this debug extension to
         * start a remote cmd session.
         */
        BOOL bRet;
        char ach[80];
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFOA StartupInfoA;

        if (pszName[0] == '\0') {
            Print("must provide a name.  eg; \"!df -x fred\"\n");
            return TRUE;
        }
        sprintf(ach, "remote.exe /s cmd.exe %s", pszName);

        // GetStartupInfoA( &StartupInfoA );

        RtlZeroMemory(&StartupInfoA, sizeof(STARTUPINFOA));
        StartupInfoA.cb = sizeof(STARTUPINFOA);
        StartupInfoA.lpTitle = pszName;
        StartupInfoA.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfoA.wShowWindow = SW_SHOWMINIMIZED; // SW_HIDE is *too* sneaky
#if 0
        Print("StartupInfoA:\n");
        Print(" cb = %d\n", StartupInfoA.cb);
        Print(" lpReserved = %s\n", StartupInfoA.lpReserved);
        Print(" lpDesktop = %s\n", StartupInfoA.lpDesktop);
        Print(" lpTitle = %s\n", StartupInfoA.lpTitle);
        Print(" dwX, dwY = %d,%d\n", StartupInfoA.dwX, StartupInfoA.dwY);
        Print(" dwXSize, dwYSize = %d,%d\n", StartupInfoA.dwXSize, StartupInfoA.dwYSize);
        Print(" dwXCountChars, dwYCountChars = %d,%d\n", StartupInfoA.dwXCountChars, StartupInfoA.dwYCountChars);
        Print(" dwFillAttribute = 0x%x\n", StartupInfoA.dwFillAttribute);
        Print(" dwFlags = 0x%x\n", StartupInfoA.dwFlags);
        Print(" wShowWindow = 0x%x\n", StartupInfoA.wShowWindow);
        Print(" hStdInput = 0x%x\n", StartupInfoA.hStdInput);
        Print(" hStdOutput = 0x%x\n", StartupInfoA.hStdOutput);
        Print(" hStdError = 0x%x\n", StartupInfoA.hStdError);
#endif
        bRet = CreateProcessA(
                NULL,            // get executable name from command line
                ach,             // CommandLine
                NULL,            // Process Attr's
                NULL,            // Thread Attr's
                FALSE,           // Inherit Handle
                CREATE_NEW_CONSOLE, // Creation Flags
                NULL,            // Environ
                NULL,            // Cur Dir
                &StartupInfoA,   // StartupInfo
                &ProcessInfo );  // ProcessInfo

        if (bRet) {
            Print("Successfully created a minimized remote cmd process\n");
            Print("use \"remote /c <machine> %s\" to connect\n", pszName);
            Print("use \"exit\" to kill the remote cmd process\n");
        }
#if 0
        Print("CreateProcessA returns %x\n", bRet);
        Print("Process handle %x\n"
              "        hThread %x\n"
              "        dwProcessId %x\n"
              "        dwThreadId %x\n",
              ProcessInfo.hProcess,
              ProcessInfo.hThread,
              ProcessInfo.dwProcessId,
              ProcessInfo.dwThreadId);
#endif
        NtClose(ProcessInfo.hProcess);
        NtClose(ProcessInfo.hThread);
        return TRUE;
    }

    moveExpValuePtr(&psi, VAR(gpsi));

    if (opts & OFLAG(p)) {

        status = GetInteger(pszName, 16, &wPID, NULL);

#ifdef KERNEL

        (lpExtensionApis->lpWriteProcessMemoryRoutine)(
                (ULONG_PTR) (&psi->wRIPPID),
                (void *) &wPID,
                sizeof(wPID),
                NULL);
#else
        if (!IsRemoteSession()) {
            PrivateSetRipFlags(-1, (DWORD)wPID);
        } else {
            Print("!df -p doesn't work on HYDRA systems\n");
        }
#endif

        move(wPID, &psi->wRIPPID);
        Print("Set Rip process to %ld (0x%lX)\n", (DWORD)wPID, (DWORD)wPID);

        return TRUE;
    }



    move(wRipFlags, &psi->wRIPFlags);
    move(wPID, &psi->wRIPPID);

    status = GetInteger(pszName, 16, &ulFlags, NULL);


    if (NT_SUCCESS(status) && !(ulFlags & ~RIPF_VALIDUSERFLAGS)) {
#ifdef KERNEL
        wRipFlags = (WORD)((wRipFlags & ~RIPF_VALIDUSERFLAGS) | ulFlags);


        (lpExtensionApis->lpWriteProcessMemoryRoutine)(
                (ULONG_PTR) (&psi->wRIPFlags),
                (void *) &wRipFlags,
                sizeof(wRipFlags),
                NULL);
#else
        PrivateSetRipFlags(ulFlags, -1);
#endif

        move(wRipFlags, &psi->wRIPFlags);
    }

    Print("Flags = %.3x\n", wRipFlags & RIPF_VALIDUSERFLAGS);
    Print("  Print Process/Component %sabled\n", (wRipFlags & RIPF_HIDEPID) ? "dis" : "en");
    Print("  Print File/Line %sabled\n", (wRipFlags & RIPF_PRINTFILELINE) ? "en" : "dis");
    Print("  Print on %s\n",  szLevels[(wRipFlags & RIPF_PRINT_MASK)  >> RIPF_PRINT_SHIFT]);
    Print("  Prompt on %s\n", szLevels[(wRipFlags & RIPF_PROMPT_MASK) >> RIPF_PROMPT_SHIFT]);

    if (wPID == 0)
        Print("  All process RIPs are shown\n");
    else
        Print("  RIPs shown for process %ld (0x%lX)\n", (DWORD)wPID, (DWORD)wPID);

    return TRUE;
}


#if DEBUGTAGS || DBG
BOOL Itag(DWORD opts, LPSTR pszName)
{
    NTSTATUS    status;
    PSERVERINFO psi;
    int         tag = -1;
    DWORD       dwDBGTAGFlagsNew;
    DWORD       dwDBGTAGFlags;
    int         i;
    int         iStart, iEnd;
    char        szT[100];
    DBGTAG *    pdbgtag;
    DBGTAG      dbgtag;

    UNREFERENCED_PARAMETER(opts);

    /*
     * Get the tag index.
     */
    moveExpValuePtr(&psi, VAR(gpsi));
    status = GetInteger(pszName, 10, &tag, &pszName);

    if (!NT_SUCCESS(status) || tag < 0 || DBGTAG_Max <= tag) {
        tag = -1;
    } else {
        /*
         * Get the flag value.
         */
        status = GetInteger(pszName, 16, &dwDBGTAGFlagsNew, NULL);
        if (NT_SUCCESS(status) && !(dwDBGTAGFlagsNew & ~DBGTAG_VALIDUSERFLAGS)) {

            /*
             * Set the flag value.
             */
#ifdef KERNEL
            move(dwDBGTAGFlags, &psi->adwDBGTAGFlags[tag]);
            COPY_FLAG(dwDBGTAGFlags, dwDBGTAGFlagsNew, DBGTAG_VALIDUSERFLAGS);
            (lpExtensionApis->lpWriteProcessMemoryRoutine)(
                    (DWORD_PTR) (&psi->adwDBGTAGFlags[tag]),
                    (void *) &dwDBGTAGFlags,
                    sizeof(dwDBGTAGFlags),
                    NULL);
#else
            extern void PrivateSetDbgTag(int tag, DWORD dwDBGTAGFlags);

            if (!IsRemoteSession()) {
                PrivateSetDbgTag(tag, dwDBGTAGFlagsNew);
            } else {
                Print("!tag doesn't work on HYDRA systems\n");
            }
#endif

        }
    }

    /*
     * Print the header.
     */
    Print(  "%-5s%-7s%-*s%-*s\n",
            "Tag",
            "Flags",
            DBGTAG_NAMELENGTH,
            "Name",
            DBGTAG_DESCRIPTIONLENGTH,
            "Description");

    for (i = 0; i < 12 + DBGTAG_NAMELENGTH + DBGTAG_DESCRIPTIONLENGTH; i++) {
        szT[i] = '-';
    }
    szT[i++] = '\n';
    szT[i] = 0;
    Print(szT);

    if (tag != -1) {
        iStart = iEnd = tag;
    } else {
        iStart = 0;
        iEnd = DBGTAG_Max - 1;
    }

    for (i = iStart; i <= iEnd; i++) {
        move(dwDBGTAGFlags, &(psi->adwDBGTAGFlags[i]));

        pdbgtag = EvalExp(VAR(gadbgtag));
        move(dbgtag, &pdbgtag[i]);

        Print(  "%-5d%-7d%-*s%-*s\n",
                i,
                dwDBGTAGFlags & DBGTAG_VALIDUSERFLAGS,
                DBGTAG_NAMELENGTH,
                dbgtag.achName,
                DBGTAG_DESCRIPTIONLENGTH,
                dbgtag.achDescription);
    }

    return TRUE;
}

#endif // if DEBUGTAGS

/************************************************************************\
* Procedure: Idhe
*
* Description: Dump Handle Entry
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idhe(
    DWORD opts,
    PVOID param1,
    PVOID param2)
{
                            /* 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 */
    static char szHeader [] = "Phe         Handle      phead       pOwner      cLockObj   Type           Flags\n";
    static char szFormat [] = "%#010lx  %#010lx  %#010lx  %#010lx  %#010lx %-15s %#4lx\n";
    THROBJHEAD head;
    DWORD_PTR dw;
    PHE pheT;
    HANDLEENTRY he, *phe;
    int i;
    UINT uHandleCount = 0;
#ifdef KERNEL
    PHANDLETYPEINFO pahti = NULL;
    HANDLETYPEINFO ahti[TYPE_CTYPES];
    PPROCESSINFO ppi;
#endif // KERNEL

    /*
     * If only the owner was provided, copy it to param2 so it's always in the same place
     */
    if (!(opts & OFLAG(t)) && (opts & OFLAG(o))) {
        param2 = param1;
    }
    /*
     * If not a recursive call, print what we're dumping
     */
    if (!(opts & OFLAG(r)) && (opts & (OFLAG(t) | OFLAG(o) | OFLAG(p)))) {
        Print("Dumping handles ");
        if (opts & OFLAG(t)) {
            if ((UINT_PTR)param1 >= TYPE_CTYPES) {
                Print("Invalid Type: %#lx\n", param1);
                return FALSE;
            }
            Print("of type %d (%s)", param1, aszTypeNames[(UINT_PTR)param1]);
        }
        if (opts & OFLAG(o)) {
            Print(" owned by %#lx", param2);
        }
        if (opts & OFLAG(p)) {
            Print(" or any thread on this process");
        }
        Print("\n");
    }

#ifdef KERNEL
    /*
     * If dumping handles for any thread in the process, we need the type flags
     */
    if (opts & OFLAG(p)) {
        pahti = EvalExp(VAR(gahti));
        move(ahti, pahti);
    }


#endif // KERNEL
    /*
     * If a handle/phe was provided, just dump it.
     */
    if (!(opts & ~OFLAG(r)) && (param1 != NULL)) {
        dw = (DWORD_PTR)HorPtoP(param1, -2);
        if (dw == 0) {
            Print("0x%p is not a valid object or handle.\n", param1);
            return FALSE;
        }
    } else {
        /*
         * Walk the handle table
         */
        Print(szHeader);
        FOREACHHANDLEENTRY(phe, he, i)
            /* Skip free handles */
            if (he.bType == TYPE_FREE) {
                continue;
            }
            /* Type check */
            if ((opts & OFLAG(t)) && (he.bType != (BYTE)param1)) {
                continue;
            }
            /* thread check */
            if ((opts & OFLAG(o)) && (FIXKP(he.pOwner) != param2)) {
                /* check for thread owned objects owned by the requested process */
                if (opts & OFLAG(p)) {
                    #ifndef KERNEL
                        continue;
                    #else
                        if (ahti[he.bType].bObjectCreateFlags & OCF_PROCESSOWNED) {
                            continue;
                        }
                        if (!(ahti[he.bType].bObjectCreateFlags & OCF_THREADOWNED)) {
                            continue;
                        }
                        tryMove(ppi, &((PTHREADINFO)he.pOwner)->ppi);
                        if (ppi != param2) {
                            continue;
                        }
                    #endif
                } else {
                    continue;
                }
            }

            Idhe(OFLAG(r), he.phead, NULL);
            uHandleCount++;
        NEXTEACHHANDLEENTRY()
        Print("%d handle(s)\n", uHandleCount);
        return TRUE;
    }

    if (!getHEfromP(&pheT, &he, (PVOID)dw)) {
        Print("%x is not a USER handle manager object.\n", param1);
        return FALSE;
    }

#ifdef KERNEL
    /*
     * If printing only one entry, print info about the owner
     */
    if (!(opts & OFLAG(r))) {
        if (he.pOwner != NULL) {
            if (!(opts & OFLAG(p))) {
                pahti = EvalExp(VAR(gahti));
                move(ahti, pahti);
            }
            if ((ahti[he.bType].bObjectCreateFlags & OCF_PROCESSOWNED)) {
                Idp(OFLAG(p), (PVOID)he.pOwner);
            } else if ((ahti[he.bType].bObjectCreateFlags & OCF_THREADOWNED)) {
                Idt(OFLAG(p), (PVOID)he.pOwner);
            }
        }
    }
#endif // KERNEL

    move(head, (PVOID)dw);

    /*
     * If only dumping one, use !dso like format. Otherwise, print a table.
     */
    if (!(opts & OFLAG(r))) {
        PRTVDW2(phe, pheT, cLockObj, head.cLockObj);
        PRTVDW2(wUniq, he.wUniq, handle, head.h);
        PRTVDW2(phead, FIXKP(he.phead), pOwner, FIXKP(he.pOwner));
        Print(DWSTR1 " - %s\n", he.bType, "bType", aszTypeNames[he.bType]);
        Print(DWSTR1 " - %s\n", he.bFlags,"bFlags",  GetFlags(GF_HE, he.bFlags, NULL, TRUE));
    } else {
        Print(szFormat,
              pheT, head.h, FIXKP(he.phead), FIXKP(he.pOwner),
              head.cLockObj, aszTypeNames[(DWORD)he.bType], (DWORD)he.bFlags);
    }

    return TRUE;
}

#ifdef KERNEL
/***************************************************************************\
* dhk - dump hooks
*
* dhk           - dumps local hooks on the foreground thread
* dhk g         - dumps global hooks
* dhk address   - dumps local hooks on THREADINFO at address
* dhk g address - dumps global hooks and local hooks on THREADINFO at address
* dhk *         - dumps local hooks for all threads
* dhk g *       - dumps global hooks and local hooks for all threads
*
* 10/21/94 IanJa        Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idhk(
DWORD opts,
PVOID param1)
{

    DWORD dwFlags;
    PTHREADINFO pti;
    THREADINFO ti;
    HOOK hook;

#define DHKF_GLOBAL_HOOKS   1
#define DHKF_PTI_GIVEN      2

    dwFlags = 0;

    pti = NULL;
    if (opts & OFLAG(g)) { // global hooks
        dwFlags |= DHKF_GLOBAL_HOOKS;
    }

#ifdef LATER
    if (opts & OFLAG(a)) {
        moveExpValuePtr(&pti, VAR(gptiFirst));
        SAFEWHILE (pti != NULL) {
            char ach[80];

            sprintf(ach, "%lx", pti);
            dhk(hCurrentProcess, hCurrentThread, dwCurPc,
                    dwCurrentPc, ach);
            move(pti, &(pti->ptiNext));
        }
        if (dwFlags & DHKF_GLOBAL_HOOKS) {
            dhk(hCurrentProcess, hCurrentThread, dwCurPc,
                    dwCurrentPc, "g");
        }
        return TRUE;
    }
#endif
    if (param1 == NULL) {
        PQ pq;
        Q q;
        moveExpValuePtr(&pq, VAR(gpqForeground));
        if (pq == NULL) {
            // Happens during winlogon
            Print("No foreground queue!\n");
            return TRUE;
        }
        move(q, pq);
        pti = q.ptiKeyboard;
    } else {
        dwFlags |= DHKF_PTI_GIVEN;
        pti = (PTHREADINFO)param1;
    }

    move(ti, pti);

    if (dwFlags & DHKF_PTI_GIVEN || !(dwFlags & DHKF_GLOBAL_HOOKS)) {
        Print("Local hooks on PTHREADINFO @ 0x%p%s:\n", pti,
            (dwFlags & DHKF_PTI_GIVEN ? "" : " (foreground thread)"));

        DUMPLHOOKS("WH_MSGFILTER", WH_MSGFILTER);
        DUMPLHOOKS("WH_JOURNALRECORD", WH_JOURNALRECORD);
        DUMPLHOOKS("WH_JOURNALPLAYBACK", WH_JOURNALPLAYBACK);
        DUMPLHOOKS("WH_KEYBOARD", WH_KEYBOARD);
        DUMPLHOOKS("WH_GETMESSAGE", WH_GETMESSAGE);
        DUMPLHOOKS("WH_CALLWNDPROC", WH_CALLWNDPROC);
        DUMPLHOOKS("WH_CALLWNDPROCRET", WH_CALLWNDPROCRET);
        DUMPLHOOKS("WH_CBT", WH_CBT);
        DUMPLHOOKS("WH_SYSMSGFILTER", WH_SYSMSGFILTER);
        DUMPLHOOKS("WH_MOUSE", WH_MOUSE);
        DUMPLHOOKS("WH_HARDWARE", WH_HARDWARE);
        DUMPLHOOKS("WH_DEBUG", WH_DEBUG);
        DUMPLHOOKS("WH_SHELL", WH_SHELL);
        DUMPLHOOKS("WH_FOREGROUNDIDLE", WH_FOREGROUNDIDLE);
        DUMPLHOOKS("WH_KEYBOARD_LL", WH_KEYBOARD_LL);
        DUMPLHOOKS("WH_MOUSE_LL", WH_MOUSE_LL);
    }

    if (dwFlags & DHKF_GLOBAL_HOOKS) {
        DESKTOPINFO di;

        move(di, ti.pDeskInfo);

        Print("Global hooks for Desktop @ %lx:\n", ti.rpdesk);
        Print("\tfsHooks            0x%08lx\n"
              "\taphkStart\n", di.fsHooks);

        DUMPHOOKS("WH_MSGFILTER", WH_MSGFILTER);
        DUMPHOOKS("WH_JOURNALRECORD", WH_JOURNALRECORD);
        DUMPHOOKS("WH_JOURNALPLAYBACK", WH_JOURNALPLAYBACK);
        DUMPHOOKS("WH_KEYBOARD", WH_KEYBOARD);
        DUMPHOOKS("WH_GETMESSAGE", WH_GETMESSAGE);
        DUMPHOOKS("WH_CALLWNDPROC", WH_CALLWNDPROC);
        DUMPHOOKS("WH_CALLWNDPROCRET", WH_CALLWNDPROCRET);
        DUMPHOOKS("WH_CBT", WH_CBT);
        DUMPHOOKS("WH_SYSMSGFILTER", WH_SYSMSGFILTER);
        DUMPHOOKS("WH_MOUSE", WH_MOUSE);
        DUMPHOOKS("WH_HARDWARE", WH_HARDWARE);
        DUMPHOOKS("WH_DEBUG", WH_DEBUG);
        DUMPHOOKS("WH_SHELL", WH_SHELL);
        DUMPHOOKS("WH_FOREGROUNDIDLE", WH_FOREGROUNDIDLE);
        DUMPHOOKS("WH_KEYBOARD_LL", WH_KEYBOARD_LL);
        DUMPHOOKS("WH_MOUSE_LL", WH_MOUSE_LL);
    }
    return TRUE;
}
#endif // KERNEL


#ifdef KERNEL
/***************************************************************************\
* dhot - dump hotkeys
*
* dhot       - dumps all hotkeys
*
* 10/21/94 IanJa        Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idhot()
{
    PHOTKEY         phk;
    HOTKEY          hk;

    moveExpValuePtr(&phk, VAR(gphkFirst));
    SAFEWHILE (phk != NULL) {
        move(hk, phk);
        Print("fsModifiers %lx, vk %x - ",
            hk.fsModifiers, hk.vk);
        Print("%s%s%s%sVK:%x %s",
            hk.fsModifiers & MOD_SHIFT   ? "Shift + " : "",
            hk.fsModifiers & MOD_ALT     ? "Alt + "   : "",
            hk.fsModifiers & MOD_CONTROL ? "Ctrl + "  : "",
            hk.fsModifiers & MOD_WIN     ? "Win + "   : "",
            hk.vk, GetVKeyName(hk.vk));

        Print("\n  id   %x\n", hk.id);
        Print("  pti  %lx\n", hk.pti);
        Print("  pwnd %lx = ", hk.spwnd);
        if (hk.spwnd == PWND_FOCUS) {
            Print("PWND_FOCUS\n");
        } else if (hk.spwnd == PWND_INPUTOWNER) {
            Print("PWND_INPUTOWNER\n");
        } else {
            CHAR ach[80];
            /*
             * Print title string.
             */
            DebugGetWindowTextA(hk.spwnd,ach);
            Print("\"%s\"\n", ach);
        }
        Print("\n");

        phk = hk.phkNext;
    }
    return TRUE;
}
#endif // KERNEL

#ifdef KERNEL
/***************************************************************************\
* dhs           - dumps simple statistics for whole table
* dhs t id      - dumps simple statistics for objects created by thread id
* dhs p id      - dumps simple statistics for objects created by process id
* dhs v         - dumps verbose statistics for whole table
* dhs v t id    - dumps verbose statistics for objects created by thread id.
* dhs v p id    - dumps verbose statistics for objects created by process id.
* dhs y type    - just dumps that type
*
* Dump handle table statistics.
*
* 02-21-92 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idhs(
DWORD opts,
PVOID param1)
{
    HANDLEENTRY *phe, he;
    DWORD dwT;
    DWORD acHandles[TYPE_CTYPES];
    DWORD cHandlesUsed, cHandlesSkipped;
    DWORD idThread, idProcess;
    DWORD i;
    int Type, cHandleEntries = 0;
    PROCESSINFO pi;
    THREADINFO ti;

    PHANDLETYPEINFO pahti = NULL;
    HANDLETYPEINFO ahti[TYPE_CTYPES];

    pahti = EvalExp(VAR(gahti));
    if (!pahti) {
        return TRUE;;
    }
    move(ahti, pahti);

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    if (opts & OFLAG(y)) {
        Type = PtrToUlong(param1);
    } else if (opts & (OFLAG(t) | OFLAG(p))) {
        dwT = PtrToUlong(param1);
    }

    cHandlesSkipped = 0;
    cHandlesUsed = 0;
    for (i = 0; i < TYPE_CTYPES; i++)
        acHandles[i] = 0;

    if (param1) {
        if (opts & OFLAG(p)) {
            Print("Handle dump for client process id 0x%lx only:\n\n", dwT);
        } else if (opts & OFLAG(t)) {
            Print("Handle dump for client thread id 0x%lx only:\n\n", dwT);
        } else if (opts & OFLAG(y)) {
            Print("Handle dump for %s objects:\n\n", aszTypeNames[Type]);
        }
    } else {
        Print("Handle dump for all processes and threads:\n\n");
    }

    if (opts & OFLAG(v)) {
        Print("Handle          Type\n");
        Print("--------------------\n");
    }

    FOREACHHANDLEENTRY(phe, he, i)
        cHandleEntries++;

        if ((opts & OFLAG(y)) && he.bType != Type) {
            continue;
        }

        if (opts & OFLAG(p) &&
                (ahti[he.bType].bObjectCreateFlags & OCF_PROCESSOWNED)) {

            if (he.pOwner == NULL) {
                continue;
            }

            move(pi, he.pOwner);
            if (GetEProcessData(pi.Process, PROCESS_PROCESSID, &idProcess) == NULL) {
                Print("Unable to read _EPROCESS at %lx\n",pi.Process);
                continue;
            }
            if (idProcess != dwT) {
                continue;
            }

        } else if ((opts & OFLAG(t)) &&
                !(ahti[he.bType].bObjectCreateFlags & OCF_PROCESSOWNED)) {

            if (he.pOwner == NULL) {
                continue;
            }

            move(ti, he.pOwner);
            move(idThread, &(ti.pEThread->Cid.UniqueThread));
            if (idThread != dwT) {
                continue;
            }
        }

        acHandles[he.bType]++;

        if (he.bType == TYPE_FREE) {
            continue;
        }

        cHandlesUsed++;

        if (opts & OFLAG(v)) {
            Print("0x%08lx %c    %s\n",
                    i,
                    (he.bFlags & HANDLEF_DESTROY) ? '*' : ' ',
                    aszTypeNames[he.bType]);
        }

    NEXTEACHHANDLEENTRY()

    if (!(opts & OFLAG(v))) {
        Print("Count           Type\n");
        Print("--------------------\n");
        for (i = 0; i < TYPE_CTYPES; i++) {
            if ((opts & OFLAG(y)) && Type != (int)i) {
                continue;
            }
            Print("0x%08lx      (%d) %s\n", acHandles[i], i, aszTypeNames[i]);
        }
    }

    if (!(opts & OFLAG(y))) {
        Print("\nTotal Accessible Handles: 0x%lx\n", cHandleEntries);
        Print("Used Accessible Handles: 0x%lx\n", cHandlesUsed);
        Print("Free Accessible Handles: 0x%lx\n", cHandleEntries - cHandlesUsed);
    }
    return TRUE;
}
#endif // KERNEL

#ifdef KERNEL
/***************************************************************************\
* di - dumps interesting globals in USER related to input.
*
*
* 11-14-91 DavidPe      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/

BOOL Idi()
{
    char            ach[80];
    PQ              pq;
    Q               q;
    DWORD           dw, dw1;
    PSERVERINFO     psi;
    SERVERINFO      si;
    PDESKTOP        pdesk;
    DESKTOP         desk;
    PWND            pwnd;


    PRTGDW2(gptiCurrent, grpdeskRitInput);
    PRTGDW2(gpqForeground, gpqForegroundPrev);
    PRTGDW2(gptiForeground, gpqCursor);

    sprintf(ach, "win32k!glinp + %#lx", FIELD_OFFSET(LASTINPUT, timeLastInputMessage));
    moveExpValue(&dw, ach);
    sprintf(ach, "win32k!glinp + %#lx", FIELD_OFFSET(LASTINPUT, ptiLastWoken));
    moveExpValue(&dw1, ach);
    PRTVDW2(glinp.timeLastInputMessage, dw, glinp.ptiLastWoken, dw1);
    PRTGDW1(gwMouseOwnerButton);

    moveExpValuePtr(&psi, VAR(gpsi));
    move(si, psi);
    PRTVPT(gpsi->ptCursor, si.ptCursor);

    moveExpValuePtr(&pdesk, VAR(grpdeskRitInput));
    if (pdesk != NULL) {
        move(desk, pdesk);
        move(pwnd, &(desk.pDeskInfo->spwnd));
        PRTWND(Desktop window, pwnd);
    }

    moveExpValuePtr(&pq, VAR(gpqForeground));
    if (pq != NULL) {
        move(q, pq);
        PRTWND(gpqForeground->spwndFocus, q.spwndFocus);
        PRTWND(gpqForeground->spwndActive, q.spwndActive);
    }

    PRTGWND(gspwndScreenCapture);
    PRTGWND(gspwndInternalCapture);
    PRTGWND(gspwndMouseOwner);

    return TRUE;
}
#endif // KERNEL



/************************************************************************\
* Procedure: Idll
*
* Description: Dump Linked Lists
*
* Returns: fSuccess
*
* ???????? Scottlu  Created
* 6/9/1995 SanfordS made to fit stdexts motif
*
\************************************************************************/
BOOL Idll(
    DWORD opts,
    LPSTR lpas)
{
    static DWORD iOffset;
    static DWORD cStructs;
    static DWORD cDwords;
    static DWORD cDwordsBack;
    static DWORD_PTR dw, dwHalfSpeed;
    DWORD_PTR dwT;
    DWORD cBytesBack;
    DWORD i, j;
    BOOL fIndirectFirst;
    BOOL fTestAndCountOnly;
    DWORD_PTR dwFind;
    DWORD adw[CDWORDS];
    NTSTATUS Status;
    DWORD dwValue;

    UNREFERENCED_PARAMETER(opts);

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    while (*lpas == ' ')
        lpas++;

    /*
     * If there are no arguments, keep walking from the last
     * pointer.
     */
    if (*lpas != 0) {

        /*
         * If the address has a '*' in front of it, it means start with the
         * pointer stored at that address.
         */
        fIndirectFirst = FALSE;
        if (*lpas == '*') {
            lpas++;
            fIndirectFirst = TRUE;
        }

        /*
         * Scan past the address.
         */
        Status = GetInteger(lpas, 16, &dwValue, &lpas);
        dw = dwValue;
        if (!NT_SUCCESS(Status)) {
            dw = (DWORD_PTR)EvalExp(lpas);
        }
        if (fIndirectFirst)
            move(dw, (PVOID)dw);
        dwHalfSpeed = dw;

        iOffset = 0;
        cStructs = (DWORD)25;
        cDwords = 8;
        cDwordsBack = 0;
        fTestAndCountOnly = FALSE;
        dwFind = 0;

        SAFEWHILE (TRUE) {
            while (*lpas == ' ')
                lpas++;

            switch(*lpas) {
            case 'l':
                /*
                 * length of each structure.
                 */
                lpas++;
                cDwords = (DWORD)(DWORD_PTR)EvalExp(lpas);
                if (cDwords > CDWORDS) {
                    Print("\nl%d? - %d DWORDs maximum\n\n", cDwords, CDWORDS);
                    cDwords = CDWORDS;
                }
                break;

            case 'b':
                /*
                 * go back cDwordsBack and dump cDwords from there
                 * (useful for LIST_ENTRYs, where Flink doesn't point to
                 * the start of the struct)
                 */
                lpas++;
                cDwordsBack = (DWORD)(DWORD_PTR)EvalExp(lpas);
                if (cDwordsBack >= CDWORDS) {
                    Print("\nb%d? - %d DWORDs maximum\n\n", cDwordsBack, CDWORDS - 1);
                    cDwordsBack = CDWORDS - 1;
                }
                break;

            case 'o':
                /*
                 * Offset of 'next' pointer.
                 */
                lpas++;
                iOffset = (DWORD)(DWORD_PTR)EvalExp(lpas);
                break;

            case 'c':
                /*
                 * Count of structures to dump
                 */
                lpas++;
                cStructs = (DWORD)(DWORD_PTR)EvalExp(lpas);
                break;

            case 'f':
                /*
                 * Find element at given address
                 */
                lpas++;
                dwFind = (DWORD_PTR)EvalExp(lpas);
                break;

            case 't':
                /*
                 * Test list for loop, and count
                 */
                fTestAndCountOnly = TRUE;
                cStructs = 0x100000;

            default:
                break;
            }

            while (*lpas && *lpas != ' ')
                lpas++;

            if (*lpas == 0)
                break;
        }

        if (cDwordsBack > cDwords) {
            Print("backing up %d DWORDS per struct (b%d): ",
                    cDwordsBack, cDwordsBack);
            Print("increasing l%d to l%d so next link is included\n",
                    cDwords, cDwordsBack + 1);
            cDwords = cDwordsBack + 1;
        }

        for (i = 0; i < CDWORDS; i++)
            adw[i] = 0;
    }

    cBytesBack = cDwordsBack * sizeof(DWORD);

    for (i = 0; i < cStructs; i++) {
        moveBlock(adw, (PVOID)(dw - cBytesBack), sizeof(DWORD) * cDwords);

        if (!fTestAndCountOnly) {
            Print("---- 0x%lx:\n", i);
            for (j = 0; j < cDwords; j += 4) {
                switch (cDwords - j) {
                case 1:
                    Print("%08lx:  %08lx\n",
                            dw + j * sizeof(DWORD),
                            adw[j + 0]);
                    break;

                case 2:
                    Print("%08lx:  %08lx %08lx\n",
                            dw + j * sizeof(DWORD),
                            adw[j + 0], adw[j + 1]);
                    break;

                case 3:
                    Print("%08lx:  %08lx %08lx %08lx\n",
                            dw + j * sizeof(DWORD),
                            adw[j + 0], adw[j + 1], adw[j + 2]);
                    break;

                default:
                    Print("%08lx:  %08lx %08lx %08lx %08lx\n",
                            dw + j * sizeof(DWORD),
                            adw[j + 0], adw[j + 1], adw[j + 2], adw[j + 3]);
                }
            }
        } else if ((i & 0xff) == 0xff) {
            Print("item 0x%lx at %lx...\n", i+1, dw);
        }

        if (dwFind == dw) {
            Print("====== FOUND ITEM ======\n");
            break;
        }

        /*
         * Give a chance to break out every 16 items
         */
        if ((i & 0xf) == 0xf) {
            if (IsCtrlCHit()) {
                Print("terminated by Ctrl-C on item 0x%lx at %lx...\n", i, dw);
                break;
            }
        }

        /*
         * Advance to next item.
         */
        dwT = dw + iOffset * sizeof(DWORD);
        move(dw, (PVOID)dwT);

        if (fTestAndCountOnly) {
            /*
             * Advance dwHalfSpeed every other time round the loop: if
             * dw ever catches up to dwHalfSpeed, then we have a loop!
             */
            if (i & 1) {
                dwT = dwHalfSpeed + iOffset * sizeof(DWORD);
                move(dwHalfSpeed, (PVOID)dwT);
            }
            if (dw == dwHalfSpeed) {
                Print("!!! Loop Detected on item 0x%lx at %lx...\n", i, dw);
                break;
            }
        }

        if (dw == 0)
            break;
    }
    Print("---- Total 0x%lx items ----\n", i+1);
    return TRUE;
}

/************************************************************************\
* Procedure: Ifind
*
* Description: Find Linked List Element
*
* Returns: fSuccess
*
* 11/22/95 JimA         Created.
\************************************************************************/
BOOL Ifind(
    DWORD opts,
    LPSTR lpas)
{
    DWORD iOffset = 0;
    LPDWORD adw;
    DWORD cbDwords;
    DWORD_PTR dwBase;
    DWORD_PTR dwLast = 0;
    DWORD_PTR dwAddr;
    DWORD_PTR dwTest;
    DWORD_PTR dwT;

    UNREFERENCED_PARAMETER(opts);

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    while (*lpas == ' ')
        lpas++;

    /*
     * If there are no arguments, keep walking from the last
     * pointer.
     */
    if (*lpas != 0) {

        /*
         * Scan past the addresses.
         */
        dwBase = (DWORD_PTR)EvalExp(lpas);
        while (*lpas && *lpas != ' ')
            lpas++;
        dwAddr = (DWORD_PTR)EvalExp(lpas);
        while (*lpas && *lpas != ' ')
            lpas++;

        iOffset = 0;

        SAFEWHILE (TRUE) {
            if (IsCtrlCHit())
                return TRUE;
            while (*lpas == ' ')
                lpas++;

            switch(*lpas) {
            case 'o':
                /*
                 * Offset of 'next' pointer.
                 */
                lpas++;
                iOffset = (DWORD)(DWORD_PTR)EvalExp(lpas);
                break;

            default:
                break;
            }

            while (*lpas && *lpas != ' ')
                lpas++;

            if (*lpas == 0)
                break;
        }
    }

    cbDwords = (iOffset + 1) * sizeof(DWORD);
    adw = LocalAlloc(LPTR, cbDwords);
    dwTest = dwBase;

    while (dwTest && dwTest != dwAddr) {
        moveBlock(adw, (PVOID)dwTest, cbDwords);

        dwLast = dwTest;
        dwT = dwTest + iOffset * sizeof(DWORD);
        move(dwTest, (PVOID)dwT);
    }
    if (dwTest == 0)
        Print("Address %#p not found\n", dwAddr);
    else
        Print("Address %#p found, previous = %#p\n", dwAddr, dwLast);
    LocalFree(adw);
    return TRUE;
}


#ifdef KERNEL
/***************************************************************************\
* dlr handle|pointer
*
* Dumps lock list for object
*
* 02-27-92 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idlr(
DWORD opts,
PVOID param1)
{
    HANDLEENTRY he;
#if DBG
    PLR plrT;
#endif

#if DEBUGTAGS
    PSERVERINFO psi;
    DWORD       dwDBGTAGFlags;
    moveExpValuePtr(&psi, VAR(gpsi));
    move(dwDBGTAGFlags, &(psi->adwDBGTAGFlags[DBGTAG_TrackLocks]));
    dwDBGTAGFlags &= DBGTAG_VALIDUSERFLAGS;
    if (dwDBGTAGFlags == DBGTAG_DISABLED) {
        Print("dlr works better if tag TrackLocks is enabled.\n");
        return(TRUE);
    }

#endif

    UNREFERENCED_PARAMETER(opts);

    if (!GetAndDumpHE((ULONG_PTR)param1, &he, FALSE)) {
        Print("!dlr: GetAndDumpHE failed\n");
        return FALSE;
    }

    /*
     * We have the handle entry: 'he' is filled in.  Now dump the
     * lock records. Remember the 1st record is the last transaction!!
     */
#if DBG
    plrT = he.plr;

    if (plrT != NULL) {
        Print("phe %x Dumping the lock records\n"
              "----------------------------------------------\n"
              "address  cLock\n"
              "----------------------------------------------\n");
    }

    SAFEWHILE (plrT != NULL) {
        DWORD_PTR   dw;
        LOCKRECORD  lr;
        int         i;
        char        ach[80];

        move(lr, plrT);

        Print("%08x %08d\n", lr.ppobj, lr.cLockObj);

        for (i = 0; i < LOCKRECORD_STACK; i++) {
            GetSym((LPVOID)lr.trace[i], ach, &dw);
            Print("                  %s+%x\n",
                  ach, dw);
        }

        plrT = lr.plrNext;
    }
#endif // DBG

    return TRUE;
}
#endif // KERNEL




/************************************************************************\
* Procedure: Idm
*
* Description: Dumps Menu structures
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
void DumpMenu( UINT uIndent, DWORD opts, PMENU pMenu )
{
    MENU    localMenu;
    ITEM    localItem;
    PITEM   pitem;
    LPDWORD lpdw;
    DWORD_PTR localDW;
    UINT     i;
    WCHAR   szBufW[128];
    char    szIndent[256];

    /*
     * Compute our indent
     */
    for (i=0; i < uIndent; szIndent[i++]=' ');
    szIndent[i] = '\0';

    /*
     * Print the menu header
     */
    if (!(opts & OFLAG(v))) {
        Print("0x%p  %s", pMenu, szIndent);
    } else {
        Print("%sPMENU @0x%p:\n", szIndent, pMenu);
    }

    /*
     * Try and get the menu
     */
    if (!tryMove(localMenu, pMenu)) {
        return;
    }


    /*
     * Print the information for this menu
     */
    if (!(opts & OFLAG(v))) {
        Print("PMENU: fFlags=0x%lX, cItems=%lu, iItem=%lu, spwndNotify=0x%p\n",
              localMenu.fFlags, localMenu.cItems, localMenu.iItem, localMenu.spwndNotify);
    } else {
        Print("%s     fFlags............ %s\n"
              "%s     location.......... (%lu, %lu)\n",
              szIndent, GetFlags(GF_MF, (WORD)localMenu.fFlags, NULL, TRUE),
              szIndent, localMenu.cxMenu, localMenu.cyMenu);
        Print("%s     spwndNotify....... 0x%p\n"
              "%s     dwContextHelpId... 0x%08lX\n"
              "%s     items............. %lu items in block of %lu\n",
              szIndent, localMenu.spwndNotify,
              szIndent, localMenu.dwContextHelpId,
              szIndent, localMenu.cItems, localMenu.cAlloced);
    }

    lpdw = (LPDWORD)(((PBYTE)pMenu) + FIELD_OFFSET(MENU, rgItems));
    if (tryMove(localDW, FIXKP(lpdw))) {
        pitem = (PITEM)localDW;
        i = 0;
        SAFEWHILE (i < localMenu.cItems) {
            /*
             * Get the menu item
             */
            if (tryMove(localItem, FIXKP(pitem))) {
                if (!(opts & OFLAG(i))) {
                    /*
                     * Print the info for this item.
                     */
                    if (!(opts & OFLAG(v))) {
                        Print("0x%p      %s%lu: ID=0x%08lX hbmp=0x%08lX", pitem, szIndent, i, localItem.wID, localItem.hbmp);
                        if (localItem.cch && tryMoveBlock(szBufW, FIXKP(localItem.lpstr), (localItem.cch*sizeof(WCHAR)))) {
                            szBufW[localItem.cch] = 0;
                            Print("  %ws%\n", szBufW);
                        } else {
                            Print(", fType=%s",GetFlags(GF_MENUTYPE, (WORD)localItem.fType, NULL, TRUE));
                            if (! (localItem.fType & MF_SEPARATOR)) {
                                 Print(", lpstr=0x%p", localItem.lpstr);
                            }
                            Print("\n");
                        }
                    } else {
                        Print("%s   Item #%d @0x%p:\n", szIndent, i, pitem);
                        /*
                         * Print the details for this item.
                         */
                        Print("%s         ID........... 0x%08lX (%lu)\n"
                              "%s         lpstr.... 0x%p",
                              szIndent, localItem.wID, localItem.wID,
                              szIndent, localItem.lpstr);
                        if (localItem.cch && tryMoveBlock(szBufW, FIXKP(localItem.lpstr), (localItem.cch*sizeof(WCHAR)))) {
                            szBufW[localItem.cch] = 0;
                            Print("  %ws%\n", szBufW);
                        } else {
                            Print("\n");
                        }
                        Print("%s         fType........ %s\n"
                              "%s         fState....... %s\n"
                              "%s         dwItemData... 0x%p\n",
                              szIndent, GetFlags(GF_MENUTYPE, (WORD)localItem.fType, NULL, TRUE),
                              szIndent, GetFlags(GF_MENUSTATE, (WORD)localItem.fState, NULL, TRUE),
                              szIndent, localItem.dwItemData);
                        Print("%s         checks....... on=0x%08lX, off=0x%08lX\n"
                              "%s         location..... @(%lu,%lu) size=(%lu,%lu)\n",
                              szIndent, localItem.hbmpChecked, localItem.hbmpUnchecked,
                              szIndent, localItem.xItem, localItem.yItem, localItem.cxItem, localItem.cyItem);
                        Print("%s         underline.... x=%lu, width=%lu\n"
                              "%s         dxTab........ %lu\n"
                              "%s         spSubMenu.... 0x%p\n",
                              szIndent, localItem.ulX, localItem.ulWidth,
                              szIndent, localItem.dxTab,
                              szIndent, localItem.spSubMenu);
                    }
                }

                /*
                 * If requested, traverse through sub-menus
                 */
                if (opts & OFLAG(r)) {
                    pMenu = HorPtoP(localItem.spSubMenu, TYPE_MENU);
                    if (pMenu && tryMove(localMenu, pMenu)) {
                        DumpMenu(uIndent+8, opts, pMenu);
                    }
                }
            }
            pitem++;
            i++;
        }
    }
}


BOOL Idm(
    DWORD opts,
    PVOID param1)
{
    HANDLEENTRY he;
    PVOID pvObject;

    if (param1 == NULL)
        return FALSE;

    pvObject = HorPtoP(FIXKP(param1), -1);
    if (pvObject == NULL) {
        Print("dm: Could not convert 0x%p to an object.\n", pvObject);
        return TRUE;
    }

    if (!getHEfromP(NULL, &he, pvObject)) {
        Print("dm: Could not get header for object 0x%p.\n", pvObject);
        return TRUE;
    }

    switch (he.bType) {
    case TYPE_WINDOW:
        {
            WND wnd;

            Print("--- Dump Menu for %s object @0x%p ---\n", pszObjStr[he.bType], FIXKP(pvObject));
            if (!tryMove(wnd, pvObject)) {
                Print("dm: Could not get copy of object 0x%p.\n", pvObject);
                return TRUE;
            }

            if (opts & OFLAG(s)) {
                /*
                 * Display window's system menu
                 */
                if ((pvObject = FIXKP(wnd.spmenuSys)) == NULL) {
                    Print("dm: This window does not have a system menu.\n");
                    return TRUE;
                }

            } else {
                if (wnd.style & WS_CHILD) {
                    /*
                     * Child windows don't have menus
                     */
                    Print("dm: Child windows do not have menus.\n");
                    return TRUE;
                }

                if ((pvObject = FIXKP(wnd.spmenu)) == NULL) {
                    Print("dm: This window does not have a menu.\n");
                    return TRUE;
                }
            }
        }

        /* >>>>  F A L L   T H R O U G H   <<<< */

    case TYPE_MENU:
        DumpMenu(0, opts, (PMENU)pvObject);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}





#ifdef KERNEL
/***************************************************************************\
* dmq - dump messages on queue
*
* dmq address - dumps messages in queue structure at address.
* dmq -a      - dump messages for all queues
* dmq -c      - count messages for all queues
*
* 11-13-91 DavidPe      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/

BOOL Idmq(
DWORD opts,
PVOID param1)
{
    THREADINFO ti;
    PQ pq;
    Q q;
    static DWORD dwPosted;
    static DWORD dwInput;
    static DWORD dwQueues, dwThreads;
    static PTHREADINFO ptiStatic;
    BOOL bMsgsPresent = FALSE;

    if (opts & OFLAG(c)) {
        if (param1 == 0) {
            Print("Summary of all message queues (\"age\" is newest time - oldest time)\n");
        } else {
            Print("Summary of message queue (\"age\" is newest time - oldest time)\n");
        }
        Print("Queue      # Input    age   \t Thread  # Posted    age\n");
        Print("========  ========  ========\t======== ========  ========\n");
        dwInput = dwPosted = 0;
        dwThreads = dwQueues = 0;
    }

    if (param1 == 0) {
        if ((opts & OFLAG(a)) || (opts & OFLAG(c))) {

            FOREACHPTI(ptiStatic);
                move(pq, &ptiStatic->pq);
                if (pq == 0) {
                    Print("Thread %lx has no queue\n", ptiStatic);
                } else {
                    Idmq(opts & OFLAG(c), pq);
                }
                move(pq, &ptiStatic->pqAttach);
                if (pq) {
                    Print("->");
                    Idmq(opts & OFLAG(c), pq);
                }
                dwThreads++;
            NEXTEACHPTI(ptiStatic);

            if (opts & OFLAG(c)) {
                Print("Queues     # Input          \t Threads # Posted\n");
                Print("========  ========  ========\t======== ========\n");
                Print("%8x  %8x          \t%8x %8x\n",
                      dwQueues, dwPosted, dwThreads, dwPosted);
            }
            return TRUE;
        }
    }

    pq = (PQ)FIXKP(param1);
    if (!tryMove(q, pq)) {
        if ((opts & OFLAG(c)) == 0) {
            Print("* Cannot get queue 0x%p: (skipped)\n", pq);
            return FALSE;
        } else {
            Print("%lx - cannot get queue \t%lx", pq, ptiStatic);
            move(ti, FIXKP(q.ptiKeyboard));
            goto ShowThreadCount;
        }
    }
    if ((opts & OFLAG(c)) == 0) {
        Print("Messages for queue 0x%p\n", pq);
    }

    if ((q.ptiKeyboard == ptiStatic) || (q.ptiMouse == ptiStatic)) {
        dwQueues++;
    }

    if (q.ptiKeyboard != NULL) {
        move(ti, FIXKP(q.ptiKeyboard));

        if (!(opts & OFLAG(c)) && ti.mlPost.pqmsgRead) {
            bMsgsPresent = TRUE;
            Print("==== PostMessage queue ====\n");
            if (ti.mlPost.pqmsgRead != NULL) {
                PrintMessages(FIXKP(ti.mlPost.pqmsgRead));
            }
        }
    }

    if (!(opts & OFLAG(c)) && q.mlInput.pqmsgRead) {
        bMsgsPresent = TRUE;
        Print(    "==== Input queue ==========\n");
        if (q.mlInput.pqmsgRead != NULL) {
            PrintMessages(FIXKP(q.mlInput.pqmsgRead));
        }
    }

    if (opts & OFLAG(c)) {
        DWORD dwTimePosted;
        DWORD dwTimeInput = 0;
        DWORD dwOldest, dwNewest;

        if (q.mlInput.cMsgs) {
            dwInput += q.mlInput.cMsgs;
            move(dwOldest, FIXKP(&q.mlInput.pqmsgRead->msg.time));
            move(dwNewest, FIXKP(&q.mlInput.pqmsgWriteLast->msg.time));
            dwTimeInput = dwNewest - dwOldest;
        }
        Print("%08x%c %8x  %8x\t%08x",
              pq,
              ((q.ptiKeyboard != ptiStatic) && (q.ptiMouse != ptiStatic)) ? '*' : ' ',
              q.mlInput.cMsgs, dwTimeInput,
              q.ptiKeyboard);
        // it would be good to print the ptiStatic too, maybe like this:
        // e1b978a8         0         0    e1ba3368        0         0
        // e1b9aca8*        0         0    e1b8b2e8        0         0
        //   (thread who's queue this is : e1a3ca28        0         0)
        //
ShowThreadCount:
        dwTimePosted = 0;
        if (ti.mlPost.cMsgs) {
            dwPosted += ti.mlPost.cMsgs;
            move(dwOldest, FIXKP(&ti.mlPost.pqmsgRead->msg.time));
            move(dwNewest, FIXKP(&ti.mlPost.pqmsgWriteLast->msg.time));
            dwTimePosted = dwNewest - dwOldest;
        }
        Print(" %8x  %8x\n", ti.mlPost.cMsgs, dwTimePosted);
    } else {
        if (bMsgsPresent) {
            Print("\n");
        }
    }

    return TRUE;
}
#endif KERNEL




#ifdef KERNEL
/***************************************************************************\
* dwe - dump winevents
*
* dwe           - dumps all EVENTHOOKs.
* dwe <addr>    - dumps EVENTHOOK at address.
* dwe -n        - dumps all NOTIFYs.
* dwe -n <addr> - dumps NOTIFY at address.
*
* 1997-07-10 IanJa      Created.
\***************************************************************************/

BOOL Idwe(
DWORD opts,
PVOID param1)
{
    EVENTHOOK EventHook, *pEventHook;
    NOTIFY Notify, *pNotify;
    PVOID pobj;
    char ach[100];

    pobj = FIXKP(param1);

    if (opts & OFLAG(n)) {
        if (pobj) {
            move(Notify, pobj);
            sprintf(ach, "NOTIFY 0x%p\n", pobj);
            Idso(0, ach);
            return 1;
        }
        moveExpValuePtr(&pNotify, VAR(gpPendingNotifies));
        Print("Pending Notifications:\n");
        gnIndent += 2;
        SAFEWHILE (pNotify != NULL) {
            sprintf(ach, "NOTIFY  0x%p\n", pNotify);
            Idso(0, ach);
            move(pNotify, &pNotify->pNotifyNext);
        }
        gnIndent -= 2;
        return TRUE;
    }

    if (pobj) {
        move(EventHook, pobj);
        sprintf(ach, "EVENTHOOK 0x%p\n", pobj);
        Idso(0, ach);
        return 1;
    }
    moveExpValuePtr(&pEventHook, VAR(gpWinEventHooks));
    Print("WinEvent hooks:\n");
    gnIndent += 2;
    SAFEWHILE (pEventHook != NULL) {
        sprintf(ach, "EVENTHOOK  0x%p\n", pEventHook);
        Idso(0, ach);
        move(pEventHook, &pEventHook->pehNext);
    }
    gnIndent -= 2;
    Print("\n");
    return TRUE;
}
#endif KERNEL



#ifndef KERNEL
/************************************************************************\
* Procedure: Idped
*
* Description: Dumps Edit Control Structures (PEDs)
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idped(
    DWORD opts,
    PVOID param1)
{
    PED   ped;
    ED    ed;
    DWORD pText;

    UNREFERENCED_PARAMETER(opts);

    ped = param1;

    move(ed, ped);
    move(pText, ed.hText);


    Print("PED Handle: %lX\n", ped);
    Print("hText      %lX (%lX)\n", ed.hText, pText);
    PRTFDW2(ed., cchAlloc, cchTextMax);
    PRTFDW2(ed., cch, cLines);
    PRTFDW2(ed., ichMinSel, ichMaxSel);
    PRTFDW2(ed., ichCaret, iCaretLine);
    PRTFDW2(ed., ichScreenStart, ichLinesOnScreen);
    PRTFDW2(ed., xOffset, charPasswordChar);
    PRTFDWDWP(ed., cPasswordCharWidth, hwnd);
    PRTFDWP1(ed., pwnd);
    PRTFRC(ed., rcFmt);
    PRTFDWP1(ed., hwndParent);
    PRTFPT(ed., ptPrevMouse);
    PRTFDW1(ed., prevKeys);

    BEGIN_PRTFFLG();
    PRTFFLG(ed, fSingle);
    PRTFFLG(ed, fNoRedraw);
    PRTFFLG(ed, fMouseDown);
    PRTFFLG(ed, fFocus);
    PRTFFLG(ed, fDirty);
    PRTFFLG(ed, fDisabled);
    PRTFFLG(ed, fNonPropFont);
    PRTFFLG(ed, fNonPropDBCS);
    PRTFFLG(ed, fBorder);
    PRTFFLG(ed, fAutoVScroll);
    PRTFFLG(ed, fAutoHScroll);
    PRTFFLG(ed, fNoHideSel);
    PRTFFLG(ed, fDBCS);
    PRTFFLG(ed, fFmtLines);
    PRTFFLG(ed, fWrap);
    PRTFFLG(ed, fCalcLines);
    PRTFFLG(ed, fEatNextChar);
    PRTFFLG(ed, fStripCRCRLF);
    PRTFFLG(ed, fInDialogBox);
    PRTFFLG(ed, fReadOnly);
    PRTFFLG(ed, fCaretHidden);
    PRTFFLG(ed, fTrueType);
    PRTFFLG(ed, fAnsi);
    PRTFFLG(ed, fWin31Compat);
    PRTFFLG(ed, f40Compat);
    PRTFFLG(ed, fFlatBorder);
    PRTFFLG(ed, fSawRButtonDown);
    PRTFFLG(ed, fInitialized);
    PRTFFLG(ed, fSwapRoOnUp);
    PRTFFLG(ed, fAllowRTL);
    PRTFFLG(ed, fDisplayCtrl);
    PRTFFLG(ed, fRtoLReading);
    PRTFFLG(ed, fInsertCompChr);
    PRTFFLG(ed, fReplaceCompChr);
    PRTFFLG(ed, fNoMoveCaret);
    PRTFFLG(ed, fResultProcess);
    PRTFFLG(ed, fKorea);
    PRTFFLG(ed, fInReconversion);
    END_PRTFFLG();

    PRTFDWDWP(ed., cbChar, chLines);
    PRTFDWDWP(ed., format, lpfnNextWord);
    PRTFDW1(ed., maxPixelWidth);

    {
        const char* p = "**INVALID**";

        if (ed.undoType < UNDO_DELETE) {
            p = GetFlags(GF_EDUNDO, 0, NULL, TRUE);
        }
        Print(DWSTR2 "\t" "%08x undoType (%s)\n", ed.hDeletedText, "hDeleteText", ed.undoType, p);
    }

    PRTFDW2(ed., ichDeleted, cchDeleted);
    PRTFDW2(ed., ichInsStart, ichInsEnd);

    PRTFDWPDW(ed., hFont, aveCharWidth);
    PRTFDW2(ed., lineHeight, charOverhang);
    PRTFDW2(ed., cxSysCharWidth, cySysCharHeight);
    PRTFDWP2(ed., listboxHwnd, pTabStops);
    PRTFDWP1(ed., charWidthBuffer);
//    PRTFDW2(ed., hkl, wMaxNegA);
    PRTFDW1(ed., wMaxNegA);
    PRTFDW2(ed., wMaxNegAcharPos, wMaxNegC);
    PRTFDW2(ed., wMaxNegCcharPos, wLeftMargin);
    PRTFDW2(ed., wRightMargin, ichStartMinSel);
    PRTFDWDWP(ed., ichStartMaxSel, pLpkEditCallout);
    PRTFDWP2(ed., hCaretBitmap, hInstance);
    PRTFDW2(ed., seed, fEncoded);
    PRTFDW2(ed., iLockLevel, wImeStatus);
    return TRUE;
}
#endif // !KERNEL


#ifndef KERNEL
/************************************************************************\
* Procedure: Idci
*
* Description: Dumps Client Info
*
* Returns: fSuccess
*
* 6/15/1995 Created SanfordS
*
\************************************************************************/
BOOL Idci()
{
    TEB teb, *pteb;
    PCLIENTINFO pci;

    if (GetTargetTEB(&teb, &pteb)) {
        pci = (PCLIENTINFO)&teb.Win32ClientInfo[0];

        Print("PCLIENTINFO @ %p:\n", &pteb->Win32ClientInfo[0]);
        // DWORD dwExpWinVer;
        Print("\tdwExpWinVer            %08lx\n", pci->dwExpWinVer);
        // DWORD dwCompatFlags;
        Print("\tdwCompatFlags          %08lx\n", pci->dwCompatFlags);
        // DWORD dwTIFlags;
        Print("\tdwTIFlags              %08lx\n", pci->dwTIFlags);
        // PDESKTOPINFO pDeskInfo;
        Print("\tpDeskInfo              %p\n", pci->pDeskInfo);
        // ULONG ulClientDelta;
        Print("\tulClientDelta          %p\n", pci->ulClientDelta);
        // struct tagHOOK *phkCurrent;
        Print("\tphkCurrent             %p\n", pci->phkCurrent);
        // DWORD fsHooks;
        Print("\tfsHooks                %08lx\n", pci->fsHooks);
        // CALLBACKWND CallbackWnd;
        Print("\tCallbackWnd            %08lx\n", pci->CallbackWnd);
        // DWORD cSpins;
        Print("\tcSpins                 %08lx\n", pci->cSpins);
        Print("\tCodePage               %d\n",    pci->CodePage);

    } else {
        Print("Unable to get TEB info.\n");
    }
    return TRUE;
}
#endif // !KERNEL



#ifdef KERNEL
/************************************************************************\
* Procedure: Idpi
*
* Description: Dumps ProcessInfo structs
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idpi(
DWORD opts,
PVOID param1)
{
    PW32PROCESS pW32Process;
    PPROCESSINFO ppi;
    PROCESSINFO pi;
    SERVERINFO si;
    SHAREDINFO shi;
    PSHAREDINFO pshi;
    DESKTOPVIEW dv;
    DWORD idProcess;

    /*
     * If he just wants the current process, located it.
     */
    if (opts & OFLAG(c)) {
        Print("Current Process:\n");
        param1 = (PVOID)GetCurrentProcessAddress(
                (USHORT)dwProcessor, hCurrentThread, NULL );

        if (param1 == 0) {
            Print("Unable to get current process pointer.\n");
            return FALSE;
        }

        if (GetEProcessData(param1, PROCESS_WIN32PROCESS, &pW32Process) == NULL) {
            Print("Unable to read _EPROCESS at %lx\n", param1);
            return FALSE;
        }
        param1 = pW32Process;
    } else if (param1 == 0) {
        Print("**** NT ACTIVE WIN32 PROCESSINFO DUMP ****\n");
        FOREACHPPI(ppi)
            Idpi(0, ppi);
            Print("\n");
        NEXTEACHPPI()
        return TRUE;
    }

    ppi = FIXKP(param1);

    if (!tryMove(pi, ppi)) {
        Print("Can't get PROCESSINFO from %x.\n", ppi);
        return FALSE;
    }

    if (GetEProcessData(pi.Process, PROCESS_PROCESSID, &idProcess) == NULL) {
        Print("Unable to read _EPROCESS at %lx\n",pi.Process);
        return FALSE;
    }

    Print("---PPROCESSINFO @ 0x%p for process %x(%s):\n",
            ppi,
            idProcess,
            ProcessName(ppi));
    Print("\tppiNext           @0x%p\n", pi.ppiNext);
    Print("\trpwinsta          @0x%p\n", pi.rpwinsta);
    Print("\thwinsta            0x%08lx\n", pi.hwinsta);
    Print("\tamwinsta           0x%08lx\n", pi.amwinsta);
    Print("\tptiMainThread     @0x%p\n", pi.ptiMainThread);
    Print("\tcThreads           0x%08lx\n", pi.cThreads);
    Print("\trpdeskStartup     @0x%p\n", pi.rpdeskStartup);
    Print("\thdeskStartup       0x%08lx\n", pi.hdeskStartup);
    Print("\tpclsPrivateList   @0x%p\n", pi.pclsPrivateList);
    Print("\tpclsPublicList    @0x%p\n", pi.pclsPublicList);
    Print("\tflags              %s\n",
            GetFlags(GF_W32PF, pi.W32PF_Flags, NULL, TRUE));
    Print("\tdwHotkey           0x%08lx\n", pi.dwHotkey);
    Print("\tpWowProcessInfo   @0x%p\n", pi.pwpi);
    Print("\tluidSession        0x%08lx:0x%08lx\n", pi.luidSession.HighPart,
            pi.luidSession.LowPart);
    Print("\tdwX,dwY            (0x%x,0x%x)\n", pi.usi.dwX, pi.usi.dwY);
    Print("\tdwXSize,dwYSize    (0x%x,0x%x)\n", pi.usi.dwXSize, pi.usi.dwYSize);
    Print("\tdwFlags            0x%08x\n", pi.usi.dwFlags);
    Print("\twShowWindow        0x%04x\n", pi.usi.wShowWindow);
    Print("\tpCursorCache       0x%08x\n", pi.pCursorCache);
    Print("\tdwLpkEntryPoints   %s\n",
            GetFlags(GF_LPK, pi.dwLpkEntryPoints, NULL, TRUE));

    /*
     * List desktop views
     */
    dv.pdvNext = pi.pdvList;
    Print("Desktop views:\n");
    while (dv.pdvNext != NULL) {
        if (!tryMove(dv, dv.pdvNext))
            break;
        Print("\tpdesk = %08x, ulClientDelta = %08x\n", dv.pdesk, dv.ulClientDelta);
    }

    /*
     * List all the open objects for this process.
     */
    GETSHAREDINFO(pshi);
    move(shi, pshi);
    move(si, shi.psi);

    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/***************************************************************************\
* dpm - dump popupmenu
*
* dpm address    - dumps menu info for menu at address
*                 (takes handle too)
*
* 13-Feb-1995  johnc      Created.
* 6/9/1995 SanfordS made to fit stdexts motif
\***************************************************************************/
BOOL Idpm(
DWORD opts,
PVOID param1)
{

    PPOPUPMENU ppopupmenu;
    POPUPMENU localPopupMenu;

    UNREFERENCED_PARAMETER(opts);

    ppopupmenu = (PPOPUPMENU)FIXKP(param1);
    move(localPopupMenu, ppopupmenu);

    Print("PPOPUPMENU @ 0x%lX\n", ppopupmenu);

    BEGIN_PRTFFLG();
    PRTFFLG(localPopupMenu, fIsMenuBar);
    PRTFFLG(localPopupMenu, fHasMenuBar);
    PRTFFLG(localPopupMenu, fIsSysMenu);
    PRTFFLG(localPopupMenu, fIsTrackPopup);
    PRTFFLG(localPopupMenu, fDroppedLeft);
    PRTFFLG(localPopupMenu, fHierarchyDropped);
    PRTFFLG(localPopupMenu, fRightButton);
    PRTFFLG(localPopupMenu, fToggle);
    PRTFFLG(localPopupMenu, fSynchronous);
    PRTFFLG(localPopupMenu, fFirstClick);
    PRTFFLG(localPopupMenu, fDropNextPopup);
    PRTFFLG(localPopupMenu, fNoNotify);
    PRTFFLG(localPopupMenu, fAboutToHide);
    PRTFFLG(localPopupMenu, fShowTimer);
    PRTFFLG(localPopupMenu, fHideTimer);
    PRTFFLG(localPopupMenu, fDestroyed);
    PRTFFLG(localPopupMenu, fDelayedFree);
    PRTFFLG(localPopupMenu, fFlushDelayedFree);
    PRTFFLG(localPopupMenu, fFreed);
    PRTFFLG(localPopupMenu, fInCancel);
    PRTFFLG(localPopupMenu, fTrackMouseEvent);
    PRTFFLG(localPopupMenu, fSendUninit);
    END_PRTFFLG();

    PRTFDWP2(localPopupMenu., spwndNotify, spwndPopupMenu);
    PRTFDWP2(localPopupMenu., spwndNextPopup, spwndPrevPopup);
    PRTFDWP2(localPopupMenu., spmenu, spmenuAlternate);
    PRTFDWP2(localPopupMenu., spwndActivePopup, ppopupmenuRoot);
    PRTFDWPDW(localPopupMenu., ppmDelayedFree, posSelectedItem);
    PRTFDW1(localPopupMenu., posDropped);

    return TRUE;
}
#endif // KERNEL

#ifdef KERNEL
/***************************************************************************\
* dms - dump pMenuState
*
* dms address
*
* 05-15-96 Created GerardoB
\***************************************************************************/
BOOL Idms(
DWORD opts,
PVOID param1)
{

    MENUSTATE *pms;
    MENUSTATE localms;

    UNREFERENCED_PARAMETER(opts);

    pms = (PMENUSTATE)FIXKP(param1);
    move(localms, pms);

    Print("PMENUSTATE @ 0x%lX\n", pms);

    BEGIN_PRTFFLG();
    PRTFFLG(localms, fMenuStarted);
    PRTFFLG(localms, fIsSysMenu);
    PRTFFLG(localms, fInsideMenuLoop);
    PRTFFLG(localms, fButtonDown);
    PRTFFLG(localms, fInEndMenu);
    PRTFFLG(localms, fUnderline);
    PRTFFLG(localms, fButtonAlwaysDown);
    PRTFFLG(localms, fDragging);
    PRTFFLG(localms, fModelessMenu);
    PRTFFLG(localms, fInCallHandleMenuMessages);
    PRTFFLG(localms, fDragAndDrop);
    PRTFFLG(localms, fAutoDismiss);
    PRTFFLG(localms, fIgnoreButtonUp);
    PRTFFLG(localms, fMouseOffMenu);
    PRTFFLG(localms, fInDoDragDrop);
    PRTFFLG(localms, fActiveNoForeground);
    PRTFFLG(localms, fNotifyByPos);
    END_PRTFFLG();

    PRTFDWP1(localms., pGlobalPopupMenu);
    PRTFPT(localms., ptMouseLast);
    PRTFDW2(localms., mnFocus, cmdLast);
    PRTFDWP1(localms., ptiMenuStateOwner);

    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/***************************************************************************\
* dq - dump queue
*
* dq address   - dumps queue structure at address
* dq t address - dumps queue structure at address plus THREADINFO
*
* 06-20-91 ScottLu      Created.
* 11-14-91 DavidPe      Added THREADINFO option.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idq(
DWORD opts,
PVOID param1)
{

    char ach[80];
    PQ pq;
    Q q;

    if (opts & OFLAG(a)) {
        PTHREADINFO pti;
        Print("Dumping all queues:\n");

        FOREACHPTI(pti);
            move(pq, &pti->pq);
            Idq(opts & ~OFLAG(a), pq);
        NEXTEACHPTI(pti);
        return TRUE;
#ifdef SOME_OTHER_DELUSION
        HANDLEENTRY he, *phe;
        int i;


        FOREACHHANDLEENTRY(phe, he, i)
            if (he.bType == TYPE_INPUTQUEUE) {
                Idq(opts & ~OFLAG(a), FIXKP(he.phead));
                Print("\n");
            }
        NEXTEACHHANDLEENTRY()
        return TRUE;
#endif
    }

    if (param1 == 0) {
        Print("Dumping foreground queue:\n");
        moveExpValuePtr(&pq, VAR(gpqForeground));
        if (pq == NULL) {
            Print("no foreground queue (gpqForeground == NULL)!\n");
            return TRUE;
        }
    } else {
        pq = (PQ)FIXKP(param1);
    }

    /*
     * Print out simple thread info for pq->ptiKeyboard
     */
    move(q, pq);
    if (q.ptiKeyboard) {
        Idt(OFLAG(p), q.ptiKeyboard);
    }

    /*
     * Don't Print() with more than 16 arguments at once because it'll blow
     * up.
     */
    Print("PQ @ 0x%p\n", pq);
    Print(
          "\tmlInput.pqmsgRead      0x%08lx\n"
          "\tmlInput.pqmsgWriteLast 0x%08lx\n"
          "\tmlInput.cMsgs          0x%08lx\n",
          q.mlInput.pqmsgRead,
          q.mlInput.pqmsgWriteLast,
          q.mlInput.cMsgs);

    Print("\tptiSysLock             0x%08lx\n"
          "\tidSysLock              0x%08lx\n"
          "\tidSysPeek              0x%08lx\n",
          q.ptiSysLock,
          q.idSysLock,
          q.idSysPeek);

    Print("\tptiMouse               0x%08lx\n"
          "\tptiKeyboard            0x%08lx\n",
          q.ptiMouse,
          q.ptiKeyboard);

    Print("\tspcurCurrent           0x%08lx\n"
          "\tiCursorLevel           0x%08lx\n",
          q.spcurCurrent,
          q.iCursorLevel);

    DebugGetWindowTextA(q.spwndCapture, ach);
    Print("\tspwndCapture           0x%08lx     \"%s\"\n",
          q.spwndCapture, ach);
    DebugGetWindowTextA(q.spwndFocus, ach);
    Print("\tspwndFocus             0x%08lx     \"%s\"\n",
          q.spwndFocus, ach);
    DebugGetWindowTextA(q.spwndActive, ach);
    Print("\tspwndActive            0x%08lx     \"%s\"\n",
          q.spwndActive, ach);
    DebugGetWindowTextA(q.spwndActivePrev, ach);
    Print("\tspwndActivePrev        0x%08lx     \"%s\"\n",
          q.spwndActivePrev, ach);

    Print("\tcodeCapture            0x%04lx\n"
          "\tmsgDblClk              0x%04lx\n"
          "\ttimeDblClk             0x%08lx\n",
          q.codeCapture,
          q.msgDblClk,
          q.timeDblClk);

    Print("\thwndDblClk             0x%08lx\n",
          q.hwndDblClk);

    Print("\tptDblClk               { %d, %d }\n",
          q.ptDblClk.x,
          q.ptDblClk.y);

    Print("\tQF_flags               0x%08lx %s\n"
          "\tcThreads               0x%08lx\n"
          "\tcLockCount             0x%08lx\n",
          q.QF_flags, GetFlags(GF_QF, q.QF_flags, NULL, FALSE),
          (DWORD) q.cThreads,
          (DWORD) q.cLockCount);

    Print("\tmsgJournal             0x%08lx\n"
          "\tExtraInfo              0x%08lx\n",
          q.msgJournal,
          q.ExtraInfo);

    /*
     * Dump THREADINFO if user specified 't'.
     */
    if (opts & OFLAG(t)) {
        Idti(0, q.ptiKeyboard);
    }
    return TRUE;
}
#endif // KERNEL

/************************************************************************\
* Procedure: Idsbt
*
* Description: Dumps Scrollbar track structures.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idsbt(
DWORD opts,
PVOID param1)
{
    SBTRACK sbt, *psbt;
    SBCALC  sbc;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == 0) {
        Print("Expected pSBTrack address\n");
        return FALSE;
    }
    psbt = (PSBTRACK)param1;
    move(sbt, psbt);

    Print("SBTrack:\n");
    Print("  fHitOld          %d\n",       sbt.fHitOld);
    Print("  fTrackVert       %d\n",       sbt.fTrackVert);
    Print("  fCtlSB           %d\n",       sbt.fCtlSB);
    Print("  fTrackRecalc     %d\n",       sbt.fTrackRecalc);
    Print("  spwndSB          0x%08lx\n",  sbt.spwndSB);
    Print("  spwndSBNotify    0x%08lx\n",  sbt.spwndSBNotify);
    Print("  spwndTrack       0x%08lx\n",  sbt.spwndTrack);
    Print("  cmdSB            0x%08lx\n",  sbt.cmdSB);
    Print("  dpxThumb         0x%08lx\n",  sbt.dpxThumb);
    Print("  posOld           0x%08lx\n",  sbt.posOld);
    Print("  posNew           0x%08lx\n",  sbt.posNew);
    Print("  pxOld            0x%08lx\n",  sbt.pxOld );
    Print("  rcTrack          (0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",
            sbt.rcTrack.left,
            sbt.rcTrack.top,
            sbt.rcTrack.right,
            sbt.rcTrack.bottom);
    Print("  hTimerSB         0x%08lx\n",  sbt.hTimerSB     );
    Print("  xxxpfnSB         0x%08lx\n",  sbt.xxxpfnSB     );
    Print("  nBar             %d\n",       sbt.nBar         );
    Print("  pSBCalc            0x%08lx\n",  sbt.pSBCalc        );
    move(sbc, sbt.pSBCalc);
    Print("  pxTop            0x%08lx\n",  sbc.pxTop        );
    Print("  pxBottom         0x%08lx\n",  sbc.pxBottom);
    Print("  pxLeft           0x%08lx\n",  sbc.pxLeft);
    Print("  pxRight          0x%08lx\n",  sbc.pxRight);
    Print("  cpxThumb         0x%08lx\n",  sbc.cpxThumb     );
    Print("  pxUpArrow        0x%08lx\n",  sbc.pxUpArrow    );
    Print("  pxDownArrow      0x%08lx\n",  sbc.pxDownArrow);
    Print("  pxStart          0x%08lx\n",  sbc.pxStart);
    Print("  pxThumbBottom    0x%08lx\n",  sbc.pxThumbBottom);
    Print("  pxThumbTop       0x%08lx\n",  sbc.pxThumbTop   );
    Print("  cpx              0x%08lx\n",  sbc.cpx          );
    Print("  pxMin            0x%08lx\n",  sbc.pxMin        );
    Print("  pos              0x%08lx\n",  sbc.pos          );
    Print("  posMin           0x%08lx\n",  sbc.posMin       );
    Print("  posMax           0x%08lx\n",  sbc.posMax       );
    Print("  page             0x%08lx\n",  sbc.page         );


    return TRUE;
}




/************************************************************************\
* Procedure: Idsbwnd
*
* Description: Dumps Scrollbar windows struct extra fields
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idsbwnd(
DWORD opts,
PVOID param1)
{
    SBWND sbw, *psbw;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == 0) {
        Print("Expected SB pwnd address\n");
        return FALSE;
    }
    psbw = (PSBWND)param1;
    move(sbw, psbw);

    Print("SBWnd:\n");
    Print("  min           %d\n", sbw.SBCalc.posMin);
    Print("  max           %d\n", sbw.SBCalc.posMax);
    Print("  page          %d\n", sbw.SBCalc.page);
    Print("  pos           %d\n", sbw.SBCalc.pos);
    Print("  fVert         %d\n", sbw.fVert);
    Print("  wDisableFlags %d\n", sbw.wDisableFlags);
    Print("  pxTop            0x%08lx\n",  sbw.SBCalc.pxTop        );
    Print("  pxBottom         0x%08lx\n",  sbw.SBCalc.pxBottom);
    Print("  pxLeft           0x%08lx\n",  sbw.SBCalc.pxLeft);
    Print("  pxRight          0x%08lx\n",  sbw.SBCalc.pxRight);
    Print("  cpxThumb         0x%08lx\n",  sbw.SBCalc.cpxThumb     );
    Print("  pxUpArrow        0x%08lx\n",  sbw.SBCalc.pxUpArrow    );
    Print("  pxDownArrow      0x%08lx\n",  sbw.SBCalc.pxDownArrow);
    Print("  pxStart          0x%08lx\n",  sbw.SBCalc.pxStart);
    Print("  pxThumbBottom    0x%08lx\n",  sbw.SBCalc.pxThumbBottom);
    Print("  pxThumbTop       0x%08lx\n",  sbw.SBCalc.pxThumbTop   );
    Print("  cpx              0x%08lx\n",  sbw.SBCalc.cpx          );
    Print("  pxMin            0x%08lx\n",  sbw.SBCalc.pxMin        );
    Print("  pos              0x%08lx\n",  sbw.SBCalc.pos          );
    Print("  posMin           0x%08lx\n",  sbw.SBCalc.posMin       );
    Print("  posMax           0x%08lx\n",  sbw.SBCalc.posMax       );
    Print("  page             0x%08lx\n",  sbw.SBCalc.page         );
    return TRUE;
}

/***************************************************************************\
* dsi dump serverinfo struct
*
* 02-27-92 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idsi(DWORD opts)
{

    SERVERINFO si;
    PSERVERINFO psi;
    int i;

    moveExpValuePtr(&psi, VAR(gpsi));

    Print("PSERVERINFO @ 0x%p\n", psi);

    move(si, psi);

    Print(  "\tRIPFlags                 0x%04lx %s\n", si.wRIPFlags,  GetFlags(GF_RIP, si.wRIPFlags, NULL, FALSE));
    Print(  "\tSRVIFlags                0x%04lx %s\n", si.wSRVIFlags, GetFlags(GF_SRVI, si.wSRVIFlags, NULL, FALSE));
    Print(  "\tPUSIFlags                0x%08lx %s\n", si.PUSIFlags,  GetFlags(GF_SI, si.PUSIFlags, NULL, FALSE));

    Print(  "\tcHandleEntries           0x%08lx\n"
            "\tcbHandleTable            0x%08lx\n"
            "\tnEvents                  0x%08lx\n",
            si.cHandleEntries,
            si.cbHandleTable,
            si.nEvents);

    if (opts & OFLAG(p)) {
        Print("\tmpFnidPfn:\n");
        for (i = 0; i < FNID_ARRAY_SIZE; i++) {
            Print("\t\t[%d] = %08lx\n", i, si.mpFnidPfn[i]);
        }
    }

    if (opts & OFLAG(w)) {
        Print("\taStoCidPfn:\n");
        for (i = 0; i < FNID_WNDPROCEND - FNID_START + 1; i++) {
            Print("\t\t[%d] = %08lx\n", i, si.aStoCidPfn[i]);
        }
    }

    if (opts & OFLAG(b)) {
        Print("\tmpFnid_serverCBWndProc:\n");
        for (i = 0; i < FNID_END - FNID_START + 1; i++) {
            Print("\t\t[%d] = %08lx\n", i, si.mpFnid_serverCBWndProc[i]);
        }
    }

    if (opts & OFLAG(m)) {

        /*
         * Add entries to this table in alphabetical order with
         * the prefix removed.
         */
        static SYSMET_ENTRY aSysMet[SM_CMETRICS] = {
            SMENTRY(ARRANGE),
            SMENTRY(CXBORDER),
            SMENTRY(CYBORDER),
            SMENTRY(CYCAPTION),
            SMENTRY(CLEANBOOT),
            SMENTRY(CXCURSOR),
            SMENTRY(CYCURSOR),
            SMENTRY(DBCSENABLED),
            SMENTRY(DEBUG),
            SMENTRY(CXDLGFRAME),
            SMENTRY(CYDLGFRAME),
            SMENTRY(CXDOUBLECLK),
            SMENTRY(CYDOUBLECLK),
            SMENTRY(CXDRAG),
            SMENTRY(CYDRAG),
            SMENTRY(CXEDGE),
            SMENTRY(CYEDGE),
            SMENTRY(CXFRAME),
            SMENTRY(CYFRAME),
            SMENTRY(CXFULLSCREEN),
            SMENTRY(CYFULLSCREEN),
            SMENTRY(CXICON),
            SMENTRY(CYICON),
            SMENTRY(CXICONSPACING),
            SMENTRY(CYICONSPACING),
            SMENTRY(IMMENABLED),
            SMENTRY(CYKANJIWINDOW),
            SMENTRY(CXMAXIMIZED),
            SMENTRY(CYMAXIMIZED),
            SMENTRY(CXMAXTRACK),
            SMENTRY(CYMAXTRACK),
            SMENTRY(CYMENU),
            SMENTRY(CXMENUCHECK),
            SMENTRY(CYMENUCHECK),
            SMENTRY(MENUDROPALIGNMENT),
            SMENTRY(CXMENUSIZE),
            SMENTRY(CYMENUSIZE),
            SMENTRY(MIDEASTENABLED),
            SMENTRY(CXMIN),
            SMENTRY(CYMIN),
            SMENTRY(CXMINIMIZED),
            SMENTRY(CYMINIMIZED),
            SMENTRY(CXMINSPACING),
            SMENTRY(CYMINSPACING),
            SMENTRY(CXMINTRACK),
            SMENTRY(CYMINTRACK),
            SMENTRY(CMONITORS),
            SMENTRY(CMOUSEBUTTONS),
            SMENTRY(MOUSEPRESENT),
            SMENTRY(MOUSEWHEELPRESENT),
            SMENTRY(NETWORK),
            SMENTRY(PENWINDOWS),
            SMENTRY(RESERVED1),
            SMENTRY(RESERVED2),
            SMENTRY(RESERVED3),
            SMENTRY(RESERVED4),
            SMENTRY(SAMEDISPLAYFORMAT),
            SMENTRY(CXSCREEN),
            SMENTRY(CYSCREEN),
            SMENTRY(CXVSCROLL),
            SMENTRY(CYHSCROLL),
            SMENTRY(CYVSCROLL),
            SMENTRY(CXHSCROLL),
            SMENTRY(SECURE),
            SMENTRY(SHOWSOUNDS),
            SMENTRY(CXSIZE),
            SMENTRY(CYSIZE),
            SMENTRY(SLOWMACHINE),
            SMENTRY(CYSMCAPTION),
            SMENTRY(CXSMICON),
            SMENTRY(CYSMICON),
            SMENTRY(CXSMSIZE),
            SMENTRY(CYSMSIZE),
            SMENTRY(SWAPBUTTON),
            SMENTRY(CYVTHUMB),
            SMENTRY(CXHTHUMB),
            SMENTRY(UNUSED_64),
            SMENTRY(UNUSED_65),
            SMENTRY(UNUSED_66),
            SMENTRY(XVIRTUALSCREEN),
            SMENTRY(YVIRTUALSCREEN),
            SMENTRY(CXVIRTUALSCREEN),
            SMENTRY(CYVIRTUALSCREEN),
        };

        Print("\taiSysMet:\n");
        for (i = 0; i < SM_CMETRICS; i++) {
            Print(  "\t\tSM_%-18s = 0x%08lx = %d\n",
                    aSysMet[i].pstrMetric,
                    si.aiSysMet[aSysMet[i].iMetric],
                    si.aiSysMet[aSysMet[i].iMetric]);
        }
    }

    if (opts & OFLAG(c)) {
        static LPSTR aszSysColor[COLOR_MAX] = {
          //012345678901234567890
            "SCROLLBAR",
            "BACKGROUND",
            "ACTIVECAPTION",
            "INACTIVECAPTION",
            "MENU",
            "WINDOW",
            "WINDOWFRAME",
            "MENUTEXT",
            "WINDOWTEXT",
            "CAPTIONTEXT",
            "ACTIVEBORDER",
            "INACTIVEBORDER",
            "APPWORKSPACE",
            "HIGHLIGHT",
            "HIGHLIGHTTEXT",
            "BTNFACE",
            "BTNSHADOW",
            "GRAYTEXT",
            "BTNTEXT",
            "INACTIVECAPTIONTEXT",
            "BTNHIGHLIGHT",
            "3DDKSHADOW",
            "3DLIGHT",
            "INFOTEXT",
            "INFOBK",
            "3DALTFACE",
            "HOTLIGHT",
        };
        Print("\targbSystem:\n\t\tCOLOR%24sSYSRGB\tSYSHBR\n", "");
        for (i = 0; i < COLOR_MAX; i++) {
            Print("\t\tCOLOR_%-21s: 0x%08lx\t0x%08lx\n",
                aszSysColor[i], si.argbSystem[i], si.ahbrSystem[i]);
        }
    }

    if (opts & OFLAG(o)) {
        Print("\toembmi @ 0x%p:\n\t\tx       \ty       \tcx       \tcy\n", &psi->oembmi);
        for (i = 0; i < OBI_COUNT; i++) {
            Print("\tbm[%d]:\t%08x\t%08x\t%08x\t%08x\n",
                    i,
                    si.oembmi[i].x ,
                    si.oembmi[i].y ,
                    si.oembmi[i].cx,
                    si.oembmi[i].cy);
        }
        Print(
                "\t\tPlanes             = %d\n"
                "\t\tBitsPixel          = %d\n"
                "\t\tBitCount           = %d\n"
                "\t\tdmLogPixels        = %d\n"
                "\t\trcScreen           = (%d,%d)-(%d,%d) %dx%d\n"
                ,
                si.Planes           ,
                si.BitsPixel        ,
                si.BitCount         ,
                (UINT) si.dmLogPixels,
                si.rcScreen.left,si.rcScreen.top,si.rcScreen.right,si.rcScreen.bottom,
                si.rcScreen.right-si.rcScreen.left, si.rcScreen.bottom-si.rcScreen.top);

    }

    if (opts & OFLAG(v)) {
        Print(
                "\tptCursor                 {%d, %d}\n"
                "\tgclBorder                0x%08lx\n"
                "\tdtScroll                 0x%08lx\n"
                "\tdtLBSearch               0x%08lx\n"
                "\tdtCaretBlink             0x%08lx\n"
                "\tdwDefaultHeapBase        0x%08lx\n"
                "\tdwDefaultHeapSize        0x%08lx\n"
                "\twMaxLeftOverlapChars     0x%08lx\n"
                "\twMaxRightOverlapchars    0x%08lx\n"
                "\tuiShellMsg               0x%08lx\n"
                "\tcxSysFontChar            0x%08lx\n"
                "\tcySysFontChar            0x%08lx\n"
                "\tcxMsgFontChar            0x%08lx\n"
                "\tcyMsgFontChar            0x%08lx\n"
                "\ttmSysFont              @ 0x%p\n"
                "\tatomIconSmProp           0x%04lx\n"
                "\tatomIconProp             0x%04lx\n"
                "\thIconSmWindows           0x%08lx\n"
                "\thIcoWindows              0x%08lx\n"
                "\thCaptionFont             0x%08lx\n"
                "\thMsgFont                 0x%08lx\n"
                "\tatomContextHelpIdProp    0x%08lx\n",
                si.ptCursor.x,
                si.ptCursor.y,
                si.gclBorder,
                si.dtScroll,
                si.dtLBSearch,
                si.dtCaretBlink,
                si.dwDefaultHeapBase,
                si.dwDefaultHeapSize,
                si.wMaxLeftOverlapChars,
                si.wMaxRightOverlapChars,
                si.uiShellMsg,
                si.cxSysFontChar,
                si.cySysFontChar,
                si.cxMsgFontChar,
                si.cyMsgFontChar,
                &psi->tmSysFont,
                si.atomIconSmProp,
                si.atomIconProp,
                si.hIconSmWindows,
                si.hIcoWindows,
                si.hCaptionFont,
                si.hMsgFont,
                si.atomContextHelpIdProp);
    }

    if (opts & OFLAG(h)) {
        SHAREDINFO shi;
        PSHAREDINFO pshi;

        GETSHAREDINFO(pshi);
        move(shi, pshi);
        Print("\nSHAREDINFO @ 0x%p:\n", pshi);
        Print(
                "\taheList                  0x%08lx\n",
                shi.aheList);
    }

    return TRUE;
}


#ifdef KERNEL
/***************************************************************************\
* dsms - dump send message structures
*
* dsms           - dumps all send message structures
* dsms v         - dumps all verbose
* dsms address   - dumps specific sms
* dsms v address - dumps verbose
* dsms l [address] - dumps sendlist of sms
*
*
* 06-20-91 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idsms(
DWORD opts,
PVOID param1)
{

    SMS sms;
    PSMS psms;
    int c = 0;
    int cm = 0;

    if ((opts & OFLAG(m)) || (param1 == 0)) {
        moveExpValuePtr(&psms, VAR(gpsmsList));

        if (psms == NULL) {
            Print("No send messages currently in the list.\n");
            return TRUE;
        }

        if (opts & OFLAG(c)) {
            // just count the messages
            SAFEWHILE (psms != NULL) {
                if ((c % 400) == 0) {
                    Print("%d (0x%lx)...\n", c, c);
                }
                c++;
                move(psms, &psms->psmsNext);
            }
        } else if (opts & OFLAG(m)) {
            // show messages with msg == param1
            SAFEWHILE (psms != NULL) {
                c++;
                move(sms, psms);
                if (sms.message == PtrToUlong(param1)) {
                    cm++;
                    Idsms(opts & ~OFLAG(m), psms);
                }
                psms = sms.psmsNext;
            }
            Print("%d messages == 0x%lx (out of a total of %d).\n", cm, param1, c);
            return TRUE;
        } else {
            SAFEWHILE (psms != NULL) {
                c++;
                if (!Idsms(opts, psms)) {
                    Print("%d (0x%lx) messages.\n", c, c);
                    return FALSE;
                }
                move(psms, &psms->psmsNext);
            }
        }
        Print("%d (0x%lx) messages.\n", c, c);
        return TRUE;
    }

    psms = (PSMS)param1;

    Print("PSMS @ 0x%p\n", psms);
    move(sms, psms);

    Print("SEND: ");
    if (sms.ptiSender != NULL) {
        Idt(OFLAG(p), sms.ptiSender);
    } else {
        Print("NULL\n");
    }

    if (sms.ptiReceiver != NULL) {
        Print("RECV: ");
        Idt(OFLAG(p), sms.ptiReceiver);
    } else {
        Print("NULL\n");
    }

    if (opts & OFLAG(v)) {
        char ach[80];

        Print("\tpsmsNext           0x%08lx\n"
#if DBG
              "\tpsmsSendList       0x%08lx\n"
              "\tpsmsSendNext       0x%08lx\n"
#endif
              "\tpsmsReceiveNext    0x%08lx\n"
              "\ttSent              0x%08lx\n"
              "\tptiSender          0x%08lx\n"
              "\tptiReceiver        0x%08lx\n"
              "\tlRet               0x%08lx\n"
              "\tflags              %s\n"
              "\twParam             0x%08lx\n"
              "\tlParam             0x%08lx\n"
              "\tmessage            0x%08lx\n",
              sms.psmsNext,
#if DBG
              sms.psmsSendList,
              sms.psmsSendNext,
#endif
              sms.psmsReceiveNext,
              sms.tSent,
              sms.ptiSender,
              sms.ptiReceiver,
              sms.lRet,
              GetFlags(GF_SMS, (WORD)sms.flags, NULL, TRUE),
              sms.wParam,
              sms.lParam,
              sms.message);
        DebugGetWindowTextA(sms.spwnd, ach);
        Print("\tspwnd              0x%08lx     \"%s\"\n", sms.spwnd, ach);
    }

#if DBG
    if (opts & OFLAG(l)) {
        DWORD idThread;
        PSMS psmsList;
        DWORD idThreadSender, idThreadReceiver;
        THREADINFO ti;

        psmsList = sms.psmsSendList;
        if (psmsList == NULL) {
            Print("%x : Empty List\n", psms);
        } else {
            Print("%x : [tidSender](msg)[tidReceiver]\n", psms);
        }
        SAFEWHILE (psmsList != NULL) {
            move(sms, psmsList);
            if (sms.ptiSender == NULL) {
                idThread = 0;
            } else {
                move(ti, sms.ptiSender);
                move(idThreadSender, &(ti.pEThread->Cid.UniqueThread));
            }
            if (sms.ptiReceiver == NULL) {
                idThread = 0;
            } else {
                move(ti, sms.ptiReceiver);
                move(idThreadReceiver, &(ti.pEThread->Cid.UniqueThread));
            }
            Print("%x : [%x](%x)[%x]\n", psmsList, idThreadSender, sms.message,
                    idThreadReceiver);

            if (psmsList == sms.psmsSendNext) {
                Print("Loop in list?\n");
                return FALSE;
            }

            psmsList = sms.psmsSendNext;
        }
        Print("\n");
    }
#endif
    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/***************************************************************************\
* dt - dump thread
*
* dt            - dumps simple thread info of all threads which have queues
*                 on server
* dt v          - dumps verbose thread info of all threads which have queues
*                 on server
* dt id         - dumps simple thread info of single server thread id
* dt v id       - dumps verbose thread info of single server thread id
*
* 06-20-91 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL DumpThread(
    DWORD opts,
    PETHREAD pEThread)
{
    ETHREAD EThread;
    WCHAR ach[256];
    THREADINFO ti;
    PTHREADINFO pti;
    CLIENTTHREADINFO cti;
    SMS sms;

    if (!tryMove(EThread, pEThread)) {
        Print("Unable to read _ETHREAD at %lx\n",pEThread);
        return FALSE;
    }

    pti = EThread.Tcb.Win32Thread;
    if (!tryMove(ti, pti)) {
        if (!(opts & OFLAG(g))) {
            Print("  et 0x%08lx t 0x???????? q 0x???????? i %2x.%-3lx <unknown name>\n",
                pEThread,
                EThread.Cid.UniqueProcess,
                EThread.Cid.UniqueThread);
        }
        return TRUE;
    }

    if (ti.pEThread != pEThread || pti == NULL) {
        return FALSE;
    } else { // Good thread

        /*
         * Print out simple thread info if this is in simple mode. Print
         * out queue info if in verbose mode (printing out queue info
         * also prints out simple thread info).
         */
        if (!(opts & OFLAG(v))) {
            PWCHAR pwch;

            GetAppName(&EThread, &ti, ach, sizeof(ach));
            pwch = wcsrchr(ach, L'\\');
            if (pwch == NULL) {
                pwch = ach;
            } else {
                pwch++;
            }

            Print("  et 0x%08lx t 0x%08lx q 0x%08lx i %2x.%-3lx %ws\n",
                    pEThread,
                    pti,
                    ti.pq,
                    EThread.Cid.UniqueProcess,
                    EThread.Cid.UniqueThread,
                    pwch);

            /*
             * Dump thread input state if required
             */
            if (opts & OFLAG(s)) {
                #define DT_INDENT "\t"
                move(cti, ti.pcti);

                if (cti.fsWakeMask == 0) {
                    Print(DT_INDENT "Not waiting for USER input events.\n");
                } else if ((cti.fsWakeMask & (QS_ALLINPUT | QS_EVENT)) == (QS_ALLINPUT | QS_EVENT)) {
                    Print(DT_INDENT "Waiting for any USER input event (== in GetMessage).\n");
                } else if ((cti.fsWakeMask == (QS_SMSREPLY | QS_SENDMESSAGE))
                            || (cti.fsWakeMask == QS_SMSREPLY)) {
                    move(sms, ti.psmsSent);
                    Print(DT_INDENT "Waiting on thread %#lx to reply to this SendMessage:\n", sms.ptiReceiver);
                    Print(DT_INDENT "pwnd:%#lx message:%#lx wParam:%#lx lParam:%#lx\n",
                            sms.spwnd, sms.message, sms.wParam, sms.lParam);
                    if (cti.fsChangeBits & QS_SMSREPLY) {
                        Print(DT_INDENT "The receiver thread has replied to the message.\n");
                    }
                } else {
                    Print(DT_INDENT "Waiting for: %s\n",
                                      GetFlags(GF_QS, (WORD)cti.fsWakeMask, NULL, TRUE));
                }
            }

        } else {
            Idti(0, pti);
            Print("--------\n");
        }
    }
    return TRUE;
}

void DumpProcessThreads(
    DWORD opts,
    PEPROCESS pEProcess,
    ULONG_PTR ThreadToDump)
{
    PW32PROCESS pW32Process;
    PETHREAD pEThread;
    ETHREAD EThread;
    LIST_ENTRY ThreadList;
    PLIST_ENTRY ThreadListHead;
    PLIST_ENTRY NextThread;

    /*
     * Dump threads of Win32 Processes only
     */
    if ((GetEProcessData(pEProcess, PROCESS_WIN32PROCESS,&pW32Process) == NULL)
            || (pW32Process == NULL)) {
        return;
    }

    ThreadListHead = GetEProcessData(pEProcess, PROCESS_THREADLIST, &ThreadList);
    if (ThreadListHead == NULL) {
        return;
    }

    NextThread = ThreadList.Flink;

    SAFEWHILE ( NextThread != ThreadListHead) {
        pEThread = (PETHREAD)(CONTAINING_RECORD(NextThread, KTHREAD, ThreadListEntry));

        if (!tryMove(EThread, pEThread)) {
            Print("Unable to read _ETHREAD at %#p\n",pEThread);
            break;
        }
        NextThread = ((PKTHREAD)&EThread)->ThreadListEntry.Flink;

        /*
         * ThreadToDump is either 0 (all windows threads) or its
         * a TID ( < UserProbeAddress or its a pEThread.
         */
        if (ThreadToDump == 0 ||

                (ThreadToDump < UserProbeAddress &&
                    ThreadToDump == (ULONG_PTR)EThread.Cid.UniqueThread) ||

                (ThreadToDump >= UserProbeAddress &&
                    ThreadToDump == (ULONG_PTR)pEThread)) {

            if (!DumpThread(opts, pEThread) && ThreadToDump != 0) {
                Print("Sorry, EThread %#p is not a Win32 thread.\n",
                        pEThread);
            }

            if (ThreadToDump != 0) {
                return;
            }

        } // Chosen Thread
    } // NextThread
}

BOOL Idt(
DWORD opts,
PVOID param1)
{
    ULONG_PTR ThreadToDump;
    LIST_ENTRY List;
    PLIST_ENTRY NextProcess;
    PLIST_ENTRY ProcessHead;
    PEPROCESS pEProcess;
    PETHREAD pEThread;
    THREADINFO ti;
    PTHREADINFO pti;

    ThreadToDump = (ULONG_PTR)param1;

    /*
     * If its a pti, validate it, and turn it into and idThread.
     */
    if (opts & OFLAG(p)) {
        if (!param1) {
            Print("Expected a pti parameter.\n");
            return FALSE;
        }

        pti = FIXKP(param1);

        if (pti == NULL) {
            Print("WARNING: bad pti given!\n");
            pti = param1;
        } else {
            move(ti, pti);
            if (!DumpThread(opts, ti.pEThread)) {
                /*
                 * This thread either doesn't have a pti or something
                 * is whacked out.  Just skip it if we want all
                 * threads.
                 */
                Print("Sorry, EThread %x is not a Win32 thread.\n",
                        ti.pEThread);
                return FALSE;
            }
            return TRUE;
        }
    }

    /*
     * If he just wants the current thread, located it.
     */
    if (opts & OFLAG(c)) {
        Print("Current Thread:");
        ThreadToDump = (ULONG_PTR)GetCurrentThreadAddress(
                (USHORT)dwProcessor, hCurrentThread );

        if (ThreadToDump == 0) {
            Print("Unable to get current thread pointer.\n");
            return FALSE;
        }
        pEThread = (PETHREAD)ThreadToDump;
        if (!DumpThread(opts, pEThread)) {
            /*
             * This thread either doesn't have a pti or something
             * is whacked out.  Just skip it if we want all
             * threads.
             */
            Print("Sorry, EThread %x is not a Win32 thread.\n",
                    pEThread);
            return FALSE;
        }
        return TRUE;
    /*
     * else he must want all window threads.
     */
    } else if (ThreadToDump == 0) {
        Print("**** NT ACTIVE WIN32 THREADINFO DUMP ****\n");
    }

    ProcessHead = EvalExp( "PsActiveProcessHead" );
    if (!ProcessHead) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }

    if (!tryMove(List, ProcessHead)) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }
    NextProcess = List.Flink;
    if (NextProcess == NULL) {
        Print("PsActiveProcessHead->Flink is NULL!\n");
        return FALSE;
    }

    SAFEWHILE(NextProcess != ProcessHead) {
        pEProcess = GetEProcessData((PEPROCESS)NextProcess,
                                    PROCESS_PROCESSHEAD,
                                    NULL);

        if (GetEProcessData(pEProcess, PROCESS_PROCESSLINK,
                &List) == NULL) {
            Print("Unable to read _EPROCESS at %lx\n",pEProcess);
            break;
        }
        NextProcess = List.Flink;

        DumpProcessThreads(opts, pEProcess, ThreadToDump);

    } // NextProcess

    if (opts & OFLAG(c)) {
        Print("%x is not a windows thread.\n", ThreadToDump);
    }

    return TRUE;
}
#endif // KERNEL

#ifdef KERNEL
/***************************************************************************\
* dp - dump process
*
*
* 06-27-97 GerardoB     Created.
\***************************************************************************/
BOOL DumpProcess(
    DWORD opts,
    PEPROCESS pEProcess)
{
    EPROCESS EProcess;
    WCHAR ach[256];
    PROCESSINFO pi;
    PPROCESSINFO ppi;

    if (!tryMove(EProcess, pEProcess)) {
        Print("Unable to read _EPROCESS at %lx\n",pEProcess);
        return FALSE;
    }

    ppi = PpiFromProcess(&EProcess);

    if (!tryMove(pi, ppi)) {
        Print("sid %2d ep 0x%08lx p 0x???????? f 0x???????? i %2x <unknown name>\n",
                EProcess.SessionId,
                pEProcess,
                EProcess.UniqueProcessId);
        return TRUE;
    }

    if (pi.Process != pEProcess || ppi == NULL) {
        return FALSE;
    } else { // Good process

        /*
         * Print out simple process info if this is in simple mode.
         */
        if (!(opts & OFLAG(v))) {
            PWCHAR pwch;

            GetProcessName(pEProcess, ach);
            pwch = wcsrchr(ach, L'\\');
            if (pwch == NULL) {
                pwch = ach;
            } else {
                pwch++;
            }

            Print("sid %2d ep 0x%08lx p 0x%08lx f 0x%08lx i %2x %ws\n",
                    EProcess.SessionId,
                    pEProcess,
                    ppi,
                    pi.W32PF_Flags,
                    EProcess.UniqueProcessId,
                    pwch);
        } else {
            Idpi(0, ppi);
            Print("--------\n");
        }

        /*
         * Dump all threads if required
         */
        if (opts & OFLAG(t)) {
            DumpProcessThreads(opts, pEProcess, 0);
        }
    }
    return TRUE;
}

BOOL Idp(
DWORD opts,
PVOID param1)
{
    ULONG_PTR ProcessToDump;
    PROCESSINFO pi;
    PPROCESSINFO ppi;
    PEPROCESS pEProcess;
    EPROCESS EProcess;
    LIST_ENTRY List;
    PLIST_ENTRY NextProcess;
    PLIST_ENTRY ProcessHead;
    PW32PROCESS pW32Process;

    ProcessToDump = (ULONG_PTR)param1;

    /*
     * If its a ppi, validate it
     */
    if (opts & OFLAG(p)) {
        if (!param1) {
            Print("Expected a ppi parameter.\n");
            return FALSE;
        }

        ppi = FIXKP(param1);

        if (ppi == NULL) {
            Print("WARNING: bad ppi given!\n");
            ppi = param1;
        } else {
            move(pi, ppi);
            if (!DumpProcess(opts, pi.Process)) {
                Print("Sorry, EProcess %x is not a Win32 process.\n",
                        pi.Process);
                return FALSE;
            }
            return TRUE;
        }
    }

    /*
     * If he just wants the current process, located it.
     */
    if (opts & OFLAG(c)) {
        Print("Current Process:");
        ProcessToDump = (ULONG_PTR)GetCurrentProcessAddress(
                (USHORT)dwProcessor, hCurrentThread, NULL);

        if (ProcessToDump == 0) {
            Print("Unable to get current process pointer.\n");
            return FALSE;
        }
        pEProcess = (PEPROCESS)ProcessToDump;
        if (!DumpProcess(opts, pEProcess)) {
            Print("Sorry, EProcess %x is not a Win32 process.\n", pEProcess);
            return FALSE;
        }
        return TRUE;
    /*
     * else he must want all window threads.
     */
    } else if (ProcessToDump == 0) {
        Print("**** NT ACTIVE WIN32 PROCESSINFO DUMP ****\n");
    }

    ProcessHead = EvalExp( "PsActiveProcessHead" );
    if (!ProcessHead) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }

    if (!tryMove(List, ProcessHead)) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }
    NextProcess = List.Flink;
    if (NextProcess == NULL) {
        Print("PsActiveProcessHead->Flink is NULL!\n");
        return FALSE;
    }

    SAFEWHILE(NextProcess != ProcessHead) {
        pEProcess = GetEProcessData((PEPROCESS)NextProcess,
                                    PROCESS_PROCESSHEAD,
                                    NULL);

        if (GetEProcessData(pEProcess, PROCESS_PROCESSLINK,
                &List) == NULL) {
            Print("Unable to read _EPROCESS at %#p\n",pEProcess);
            break;
        }
        NextProcess = List.Flink;

        /*
         * Dump threads of Win32 Processes only
         */
        if (GetEProcessData(pEProcess, PROCESS_WIN32PROCESS,
                &pW32Process) == NULL || pW32Process == NULL) {
            continue;
        }

        if (!tryMove(EProcess, pEProcess)) {
            Print("Unable to read _EPROCESS at %lx\n",pEProcess);
            break;
        }
        /*
         * ProcessToDump is either 0 (all windows processes) or its
         * a TID ( < UserProbeAddress or its a pEPRocess.
         */
        if (ProcessToDump == 0 ||

                (ProcessToDump < UserProbeAddress &&
                    ProcessToDump == (ULONG_PTR)EProcess.UniqueProcessId) ||

                (ProcessToDump >= UserProbeAddress &&
                    ProcessToDump == (ULONG_PTR)pEProcess)) {

            if (!DumpProcess(opts, pEProcess)
                    && (ProcessToDump != 0)) {
                Print("Sorry, EProcess %#p is not a Win32 process.\n",
                        pEProcess);
            }

            if (ProcessToDump != 0) {
                return TRUE;
            }
        }

    } // NextProcess

    if (opts & OFLAG(c)) {
        Print("%#p is not a windows process.\n", ProcessToDump);
    }

    return TRUE;
}
#endif // KERNEL



#ifdef KERNEL
/***************************************************************************\
* dtdb - dump TDB
*
* dtdb address - dumps TDB structure at address
*
* 14-Sep-1993 DaveHart  Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idtdb(
DWORD opts,
PVOID param1)
{

    PTDB ptdb;
    TDB tdb;
    PTHREADINFO pti;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == 0) {
        Print("Dumping all ptdbs:\n");
        FOREACHPTI(pti)
            move(ptdb, &pti->ptdb);
            SAFEWHILE (ptdb) {
                Idtdb(0, ptdb);
                move(ptdb, &ptdb->ptdbNext);
            }
        NEXTEACHPTI(pti)
        return TRUE;
    }

    ptdb = (PTDB)param1;

    if (ptdb == NULL) {
        Print("Must supply a TDB address.\n");
        return FALSE;
    }

    move(tdb, ptdb);

    Print("TDB (non preemptive scheduler task database) @ 0x%p\n", ptdb);
    Print("\tptdbNext          @0x%p\n", tdb.ptdbNext);
    Print("\tnEvents            0x%08lx\n", tdb.nEvents);
    Print("\tnPriority          0x%08lx\n", tdb.nPriority);
    Print("\tpti               @0x%p\n", tdb.pti);
    return TRUE;
}
#endif // KERNEL

#ifndef KERNEL
/************************************************************************\
* Procedure: Ikbp
*
* Description: Breaks into the kernel debugger
*
* Returns: fSuccess
*
* 7/2/96 Fritz Sands
*
\************************************************************************/
BOOL Ikbp()
{
    static HANDLE ghToken = NULL;
    BOOL fFirstTry = FALSE;
    NTSTATUS Status;

#if 0 /* NTSD itself sets this privilege */
    /*
     * Enable SeDebugPrivilege the first time around
     */
    if (ghToken == NULL) {
        LUID luid;
        TOKEN_PRIVILEGES tp, tpPrevious;
        DWORD cbSizePrevious, dwGLE;

        fFirstTry = TRUE;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &ghToken)) {
            Print("!kbp failed to open process token. GLE:%d\n", GetLastError());
            goto TryItAnyway;
        }
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
            Print("!kbp failed to lookup debug privilege. GLE:%d\n", GetLastError());
            goto TryItAnyway;
        }
        /*
         * Get the current setting attributes
         */
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = 0;
        AdjustTokenPrivileges(ghToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious,&cbSizePrevious);
        dwGLE = GetLastError();
        if (dwGLE != ERROR_SUCCESS) {
            Print("!kbp failed to get current privilege setting. GLE:%d\n", dwGLE);
            goto TryItAnyway;
        }
        /*
         * Enable it
         */
        tpPrevious.PrivilegeCount = 1;
        tpPrevious.Privileges[0].Luid   = luid;
        tpPrevious.Privileges[0].Attributes |= SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(ghToken, FALSE, &tpPrevious,cbSizePrevious, NULL, NULL);
        dwGLE = GetLastError();
        if (dwGLE != ERROR_SUCCESS) {
            Print("!kbp failed to get adjust privilege setting. GLE:%d\n", dwGLE);
            goto TryItAnyway;
        }

        CloseHandle(ghToken);
        fFirstTry = FALSE;
    } /* if (ghToken == NULL) */


TryItAnyway:
    if (fFirstTry && (ghToken != NULL)) {
        CloseHandle(ghToken);
    }
#endif

    Status = NtSystemDebugControl(SysDbgBreakPoint, NULL, 0, NULL, 0, NULL);
    if (!NT_SUCCESS(Status)) {
        Print("!kbp NtSystemDebugControl failed. Status:%#lx\n", Status);
        if (fFirstTry) {
            ghToken = NULL;
        }
        return FALSE;
    } else {
        return TRUE;
    }
}

#endif // !KERNEL

#ifndef KERNEL
/************************************************************************\
* Procedure: Icbp
*
* Description: Breaks into the debugger in context of csrss.exe.
*
* Returns: fSuccess
*
* 6/1/98 JerrySh
*
\************************************************************************/
BOOL Icbp(VOID)
{
    DWORD dwProcessId;
    DWORD dwThreadId;
    BOOL fServerProcess;
    USER_API_MSG m;
    PACTIVATEDEBUGGERMSG a = &m.u.ActivateDebugger;

    moveExpValue(&fServerProcess, VAR(gfServerProcess));

    if (fServerProcess) {
        Print("Already debugging server process!\n");
    } else {
        /*
         * Get the process and thread ID of a CSRSS thread.
         */
        dwThreadId = GetWindowThreadProcessId(GetDesktopWindow(), &dwProcessId);
        a->ClientId.UniqueProcess = (HANDLE)LongToHandle(dwProcessId);
        a->ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);

        /*
         * Tell CSRSS to break on itself.
         */
        CsrClientCallServer( (PCSR_API_MSG)&m,
                             NULL,
                             CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                                  UserpActivateDebugger
                                                ),
                             sizeof(*a)
                           );
    }

    return TRUE;
}

#endif // !KERNEL

#ifndef KERNEL
/************************************************************************\
* Procedure: Idteb
*
* Description: Dumps the target process's TEB
*
* Returns: fSuccess
*
* 6/15/1995 Created SanfordS
*
\************************************************************************/
BOOL Idteb()
{
    TEB teb, *pteb;

    if (GetTargetTEB(&teb, &pteb)) {
        Print("TEB @ 0x%p:\n", pteb);
        // NT_TIB NtTib;
        //     struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
        //     PVOID StackBase;
        //     PVOID StackLimit;
        //     PVOID SubSystemTib;
        //     ULONG Version;
        //     PVOID ArbitraryUserPointer;
        //     struct _NT_TIB *Self;
        // PVOID  EnvironmentPointer;
        // CLIENT_ID ClientId;
        Print("\tClientId                       %08lx\n", teb.ClientId);
        // PVOID ActiveRpcHandle;
        // PVOID ThreadLocalStoragePointer;
        // PPEB ProcessEnvironmentBlock;
        // ULONG LastErrorValue;
        Print("\tLastErrorValue                 %08lx\n", teb.LastErrorValue);
        // ULONG CountOfOwnedCriticalSections;
        Print("\tCountOfOwnedCriticalSections   %08lx\n", teb.CountOfOwnedCriticalSections);
        // PVOID Win32ThreadInfo;          // PtiCurrent
        Print("\tWin32ThreadInfo(pti)           %p\n", teb.Win32ThreadInfo);
        // PVOID CsrQlpcStack;
        // UCHAR SpareBytes[124];
        // LCID CurrentLocale;
        // ULONG FpSoftwareStatusRegister;
        // PVOID Win32ClientInfo[54];
        Print("\tWin32ClientInfo[0](pci)        %08lx\n", teb.Win32ClientInfo[0]);
        // PVOID Spare1;                   // User Debug info
        // NTSTATUS ExceptionCode;         // for RaiseUserException
        // PVOID CsrQlpcTeb[QLPC_TEB_LENGTH];
        // PVOID Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH];
        Print("\tWin32ClientInfo(pcti)         @%p\n", &pteb->Win32ClientInfo[0]);
        // PVOID SystemReserved2[322];
        // ULONG GdiClientPID;
        Print("\tGdiClientPID                   %08lx\n", teb.GdiClientPID);
        // ULONG GdiClientTID;
        Print("\tGdiClientTID                   %08lx\n", teb.GdiClientTID);
        // PVOID GdiThreadLocalInfo;
        Print("\tGdiThreadLocalInfo             %08lx\n", teb.GdiThreadLocalInfo);
        // PVOID User32Reserved0;          // User app spin count
        // PVOID User32Reserved1;
        // PVOID glDispatchTable[307];     // OpenGL
        // PVOID glSectionInfo;            // OpenGL
        // PVOID glSection;                // OpenGL
        // PVOID glTable;                  // OpenGL
        // PVOID glCurrentRC;              // OpenGL
        // PVOID glContext;                // OpenGL
        // ULONG LastStatusValue;
        // UNICODE_STRING StaticUnicodeString;
        // WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
        // PVOID DeallocationStack;
        // PVOID TlsSlots[TLS_MINIMUM_AVAILABLE];
        // LIST_ENTRY TlsLinks;
        // PVOID Vdm;
        // PVOID ReservedForNtRpc;
        // PVOID DbgSsReserved[2];
    } else {
        Print("Unable to get TEB info.\n");
    }
    return TRUE;
}
#endif // !KERNEL


#ifdef KERNEL
typedef struct _tagBASECHARSET {
    LPSTR pstrCS;
    DWORD dwValue;
} BASECHARSET;

BASECHARSET CrackCS[] = {
    {"ANSI_CHARSET"            ,0   },
    {"DEFAULT_CHARSET"         ,1   },
    {"SYMBOL_CHARSET"          ,2   },
    {"SHIFTJIS_CHARSET"        ,128 },
    {"HANGEUL_CHARSET"         ,129 },
    {"GB2312_CHARSET"          ,134 },
    {"CHINESEBIG5_CHARSET"     ,136 },
    {"OEM_CHARSET"             ,255 },
    {"JOHAB_CHARSET"           ,130 },
    {"HEBREW_CHARSET"          ,177 },
    {"ARABIC_CHARSET"          ,178 },
    {"GREEK_CHARSET"           ,161 },
    {"TURKISH_CHARSET"         ,162 },
    {"THAI_CHARSET"            ,222 },
    {"EASTEUROPE_CHARSET"      ,238 },
    {"RUSSIAN_CHARSET"         ,204 },
    {"MAC_CHARSET"             ,77  }};

/***************************************************************************\
* dkl - dump keyboard layout
*
* dkl address      - dumps keyboard layout structure at address
*
* 05/21/95 GregoryW        Created.
\***************************************************************************/

BOOL Idkl(
DWORD opts,
PVOID param1)
{
    LIST_ENTRY List;
    LIST_ENTRY ThreadList;
    PLIST_ENTRY NextProcess;
    PLIST_ENTRY NextThread;
    PLIST_ENTRY ProcessHead;
    PLIST_ENTRY ThreadListHead;
    PEPROCESS pEProcess;
    PW32PROCESS pW32Process;
    PETHREAD pEThread;
    ETHREAD EThread;
    WCHAR ach[256];
    THREADINFO ti;
    PTHREADINFO pti;
    KL kl, *pkl, *pklAnchor;
    KBDFILE kf;
    KBDTABLES KbdTbl;
    int i;
    int nThread;


    if (opts & OFLAG(k)) {
        goto display_layouts;
    }

    if (param1 == 0) {
        Print("Using gspklBaseLayout\n");
        moveExpValuePtr(&pkl, VAR(gspklBaseLayout));
        if (!pkl) {
            return FALSE;
        }
    } else {
        pkl = (PKL)FIXKP(param1);
    }

    if (pkl == NULL) {
        return FALSE;
    }

    move(kl, pkl);

    Print("KL @ 0x%p (cLockObj = %d)\n", pkl, kl.head.cLockObj);
    Print("  pklNext       @0x%p\n", kl.pklNext);
    Print("  pklPrev       @0x%p\n", kl.pklPrev);
    Print("  dwKL_Flags     0x%08lx\n", kl.dwKL_Flags);
    Print("  hkl            0x%08lx\n", kl.hkl);
    Print("  piiex         @0x%p\n", kl.piiex);

    if (kl.spkf == NULL) {
        Print("  spkf          @0x%p (NONE!)\n", kl.spkf);
    }else {
        move(kf, kl.spkf);

        move(KbdTbl, kf.pKbdTbl);

        Print("  spkf          @0x%p (cLockObj = %d)\n", kl.spkf, kf.head.cLockObj);
        Print("     pkfNext       @0x%p\n", kf.pkfNext);
        Print("     awchKF[]      L\"%ws\"\n", &kf.awchKF[0]);
        Print("     hBase          0x%08lx\n", kf.hBase);
        Print("     pKbdTbl       @0x%p\n", kf.pKbdTbl);
        Print("        fLocaleFlags 0x%08lx\n", KbdTbl.fLocaleFlags);
        Print("  dwFontSigs        %s\n", GetFlags(GF_CHARSETS, kl.dwFontSigs, NULL, TRUE));
    }

    for (i = 0; i < (sizeof(CrackCS) / sizeof(BASECHARSET)); i++) {
        if (CrackCS[i].dwValue == kl.iBaseCharset) {
            break;
        }
    }
    Print("  iBaseCharset   %s\n",
          (i < (sizeof(CrackCS) / sizeof(BASECHARSET))) ? CrackCS[i].pstrCS : "ILLEGAL VALUE");
    Print("  Codepage       %d\n", kl.CodePage);

    if (opts & OFLAG(a)) {
        pklAnchor = pkl;
        SAFEWHILE (kl.pklNext != pklAnchor) {
            pkl = kl.pklNext;
            if (!Idkl(0, pkl)) {
                return FALSE;
            }
            move(kl, pkl);
        }
    }
    return TRUE;

display_layouts:

    ProcessHead = EvalExp( "PsActiveProcessHead" );
    if (!ProcessHead) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }
    Print("ProcessHead = %lx\n", ProcessHead);

    if (!tryMove(List, ProcessHead)) {
        Print("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }
    NextProcess = List.Flink;
    if (NextProcess == NULL) {
        Print("PsActiveProcessHead->Flink is NULL!\n");
        return FALSE;
    }
    Print("NextProcess = %lx\n", NextProcess);

    nThread = 0;
    SAFEWHILE(NextProcess != ProcessHead) {
        pEProcess = GetEProcessData((PEPROCESS)NextProcess,
                                    PROCESS_PROCESSHEAD,
                                    NULL);
        Print("pEProcess = %lx\n", pEProcess);

        if (GetEProcessData(pEProcess, PROCESS_PROCESSLINK,
                &List) == NULL) {
            Print("Unable to read _EPROCESS at %lx\n",pEProcess);
            break;
        }
        NextProcess = List.Flink;

        /*
         * Dump threads of Win32 Processes only
         */
        if (GetEProcessData(pEProcess, PROCESS_WIN32PROCESS,
                &pW32Process) == NULL || pW32Process == NULL) {
            continue;
        }

        ThreadListHead = GetEProcessData(pEProcess, PROCESS_THREADLIST, &ThreadList);
        if (ThreadListHead == NULL)
            continue;
        NextThread = ThreadList.Flink;

        SAFEWHILE ( NextThread != ThreadListHead) {
            pEThread = (PETHREAD)(CONTAINING_RECORD(NextThread, KTHREAD, ThreadListEntry));

            if (!tryMove(EThread, pEThread)) {
                Print("Unable to read _ETHREAD at %lx\n",pEThread);
                break;
            }
            NextThread = ((PKTHREAD)&EThread)->ThreadListEntry.Flink;

            pti = EThread.Tcb.Win32Thread;
            if (pti == NULL) {
                Print("Cid %lx:%lx has NULL pti\n",
                       EThread.Cid.UniqueProcess,
                       EThread.Cid.UniqueThread);
            } else if (!tryMove(ti, pti)) {
                Print("Idt: Unable to move pti from %x.\n",
                        pti);
                return FALSE;
            }

            if (ti.pEThread != pEThread || pti == NULL) {
                /*
                 * This thread either doesn't have a pti or something
                 * is whacked out.  Just skip it if we want all
                 * threads.
                 */
            } else { // Good thread

                PWCHAR pwch;

                if (!GetAppName(&EThread, &ti, ach, sizeof(ach))) {
                    Print("Idt: Unable to get app name for ETHREAD %x.\n",
                            pEThread);
                    return FALSE;
                }
                pwch = wcsrchr(ach, L'\\');
                if (pwch == NULL) {
                    pwch = ach;
                } else {
                    pwch++;
                }

                nThread++;
                Print("t 0x%08lx i %2x.%-3lx k 0x%08lx  %ws\n",
                        pti,
                        EThread.Cid.UniqueProcess,
                        EThread.Cid.UniqueThread,
                        ti.spklActive,
                        pwch);
                if (opts & OFLAG(v)) {
                    Idkl(0, ti.spklActive);
                }

            } // Good Thread
        } // NextThread
    } // NextProcess
    Print("  %d threads total.\n", nThread);
    moveExpValuePtr(&pkl, VAR(gspklBaseLayout));
    Print("  gspklBaseLayout = %#p\n", pkl);
    if (opts & OFLAG(v)) {
        Idkl(0, pkl);
    }

    return TRUE;
}

/***************************************************************************\
* ddk - dump deadkey table
*
* ddk address      - dumps deadkey table address
*
* 09/28/95 GregoryW        Created.
\***************************************************************************/

BOOL Iddk(
DWORD opts,
PVOID param1)
{
   KBDTABLES KbdTbl;
   PKBDTABLES pKbdTbl;
   DEADKEY DeadKey;
   PDEADKEY pDeadKey;

    UNREFERENCED_PARAMETER(opts);

   if (param1 == 0) {
       Print("Expected address\n");
       return FALSE;
   } else {
       pKbdTbl = (PKBDTABLES)FIXKP(param1);
   }

   move(KbdTbl, pKbdTbl);

   pDeadKey = KbdTbl.pDeadKey;

   if (!pDeadKey) {
       Print("No deadkey table for this layout\n");
       return TRUE;
   }

   do {
       move(DeadKey, pDeadKey);
       if (DeadKey.dwBoth == 0) {
           break;
       }
       Print("d 0x%04x  ch 0x%04x  => 0x%04x\n", HIWORD(DeadKey.dwBoth), LOWORD(DeadKey.dwBoth), DeadKey.wchComposed);
       pDeadKey++;
   } while (TRUE);

   return TRUE;
}

/***************************************************************************\
* dii - dump IMEINFOEX
*
* dii address      - dumps extended IME information at address
*
* 01/30/96 WKwok        Created.
\***************************************************************************/
BOOL Idii(
DWORD opts,
PVOID param1)
{
    IMEINFOEX iiex, *piiex;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        Print("Expected address\n");
        return FALSE;
    }

    piiex = (PIMEINFOEX)FIXKP(param1);
    move(iiex, piiex);

    Print("IIEX @ 0x%p\n", piiex);
    Print("  hKL            0x%08lx\n", iiex.hkl);
    Print("  ImeInfo\n");
    Print("     dwPrivateDataSize   0x%08lx\n", iiex.ImeInfo.dwPrivateDataSize);
    Print("     fdwProperty         0x%08lx\n", iiex.ImeInfo.fdwProperty);
    Print("     fdwConversionCaps   0x%08lx\n", iiex.ImeInfo.fdwConversionCaps);
    Print("     fdwSentenceCaps     0x%08lx\n", iiex.ImeInfo.fdwSentenceCaps);
    Print("     fdwUICaps           0x%08lx\n", iiex.ImeInfo.fdwUICaps);
    Print("     fdwSCSCaps          0x%08lx\n", iiex.ImeInfo.fdwSCSCaps);
    Print("     fdwSelectCaps       0x%08lx\n", iiex.ImeInfo.fdwSelectCaps);
    Print("  wszImeDescription[]   L\"%ws\"\n", &iiex.wszImeDescription[0]);
    Print("  wszImeFile[]          L\"%ws\"\n", &iiex.wszImeFile[0]);

    return TRUE;
}

/***************************************************************************\
* dti - dump THREADINFO
*
* dti address - dumps THREADINFO structure at address
*
* 11-13-91 DavidPe      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idti(
DWORD opts,
PVOID param1)
{
    PTHREADINFO pti;
    THREADINFO ti;
    CLIENTTHREADINFO cti;
    PETHREAD pThread;
    ETHREAD Thread;
    UCHAR PriorityClass;

    UNREFERENCED_PARAMETER(opts);


    if (param1 == NULL) {
        PQ pq;
        Q q;
        Print("No pti specified: using foreground thread\n");
        moveExpValuePtr(&pq, VAR(gpqForeground));
        if (pq == NULL) {
            Print("No foreground queue!\n");
            return FALSE;
        }
        move(q, FIXKP(pq));
        pti = FIXKP(q.ptiKeyboard);
    } else {
        pti = (PTHREADINFO)FIXKP(param1);
    }

    if (pti == NULL) {
        return FALSE;
    }

    Idt(OFLAG(p), pti);

    move(ti, pti);
    move(cti, ti.pcti);

    Print("PTHREADINFO @ 0x%p\n", pti);

    Print("\tPtiLink.Flink         @0x%p\n"
          "\tptl                   @0x%p\n"
          "\tptlW32                @0x%p\n"
          "\tppi                   @0x%p\n"
          "\tpq                    @0x%p\n"
          "\tspklActive            @0x%p\n"
          "\tmlPost.pqmsgRead      @0x%p\n"
          "\tmlPost.pqmsgWriteLast @0x%p\n"
          "\tmlPost.cMsgs           0x%08lx\n",
          ti.PtiLink.Flink,
          ti.ptl,
          ti.ptlW32,
          ti.ppi,
          ti.pq,
          ti.spklActive,
          ti.mlPost.pqmsgRead,
          ti.mlPost.pqmsgWriteLast,
          ti.mlPost.cMsgs);

    Print("\tspwndDefaultIme       @0x%p\n"
          "\tspDefaultImc          @0x%p\n"
          "\thklPrev                0x%08lx\n",
          ti.spwndDefaultIme,
          ti.spDefaultImc,
          ti.hklPrev);

    Print("\trpdesk                @0x%p\n",
          ti.rpdesk);
    Print("\thdesk                  0x%08lx\n",
          ti.hdesk);
    Print("\tamdesk                 0x%08lx\n",
          ti.amdesk);

    Print("\tpDeskInfo             @0x%p\n"
          "\tpClientInfo           @0x%p\n",
          ti.pDeskInfo,
          ti.pClientInfo);

    Print("\tTIF_flags              %s\n",
          GetFlags(GF_TIF, ti.TIF_flags, NULL, TRUE));
    Print("\tsphkCurrent           @0x%p\n"
          "\tpEventQueueServer     @0x%p\n"
          "\thEventQueueClient      0x%08lx\n",
          ti.sphkCurrent,
          ti.pEventQueueServer,
          ti.hEventQueueClient);

    Print("\tfsChangeBits           %s\n",
            GetFlags(GF_QS, (WORD)cti.fsChangeBits, NULL, TRUE));
    Print("\tfsChangeBitsRemovd     %s\n",
            GetFlags(GF_QS, (WORD)ti.fsChangeBitsRemoved, NULL, TRUE));
    Print("\tfsWakeBits             %s\n",
            GetFlags(GF_QS, (WORD)cti.fsWakeBits, NULL, TRUE));
    Print("\tfsWakeMask             %s\n",
            GetFlags(GF_QS, (WORD)cti.fsWakeMask, NULL, TRUE));

    Print("\tcPaintsReady           0x%04x\n"
          "\tcTimersReady           0x%04x\n"
          "\ttimeLast               0x%08lx\n"
          "\tptLast.x               0x%08lx\n"
          "\tptLast.y               0x%08lx\n"
          "\tidLast                 0x%08lx\n"
          "\tcQuit                  0x%08lx\n"
          "\texitCode               0x%08lx\n"
          "\tpSBTrack               0x%08lx\n"
          "\tpsmsSent              @0x%p\n"
          "\tpsmsCurrent           @0x%p\n",
          ti.cPaintsReady,
          ti.cTimersReady,
          ti.timeLast,
          ti.ptLast.x,
          ti.ptLast.y,
          ti.idLast,
          ti.cQuit,
          ti.exitCode,
          ti.pSBTrack,
          ti.psmsSent,
          ti.psmsCurrent);

    Print("\tfsHooks                0x%08lx\n"
          "\taphkStart             @0x%p l%ld\n"
          "\tsphkCurrent           @0x%p\n",
          ti.fsHooks,
          &(pti->aphkStart), CWINHOOKS,
          ti.sphkCurrent);
    Print("\tpsmsReceiveList       @0x%p\n",
          ti.psmsReceiveList);
    Print("\tptdb                  @0x%p\n"
          "\tThread                @0x%p\n",
          ti.ptdb, ti.pEThread);

    pThread = (PETHREAD)ti.pEThread;
    move(Thread, pThread);
    GetEProcessData(Thread.ThreadsProcess, PROCESS_PRIORITYCLASS, &PriorityClass);
    Print("\t  PriorityClass %d\n",
          PriorityClass);

    Print("\tcWindows               0x%08lx\n"
          "\tcVisWindows            0x%08lx\n"
          "\tpqAttach              @0x%p\n"
          "\tiCursorLevel           0x%08lx\n",
          ti.cWindows,
          ti.cVisWindows,
          ti.pqAttach,
          ti.iCursorLevel);

    Print("\tpMenuState            @0x%p\n",
          ti.pMenuState);

    return TRUE;
}
#endif // KERNEL


#ifdef KERNEL

BOOL ScanThreadlocks(PTHREADINFO pti, char chType, PVOID pvSearch);

/***************************************************************************\
* dtl handle|pointer
*
* !dtl <addr>       Dumps all THREAD locks for object at <addr>
* !dtl -t <pti>     Dumps all THREAD locks made by thread <pti>
* !dtl              Dumps all THREAD locks made by all threads
*
* 02-27-92 ScottLu      Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
BOOL Idtl(
DWORD opts,
PVOID param1)
{
    PTHREADINFO pti;

    if (param1 == 0) {
        Print("Dumping all thread locks:\n");
        Print("pti        pObj     Caller\n");
        FOREACHPTI(pti)
            Idtl(OFLAG(t) | OFLAG(x), pti);
        NEXTEACHPTI(pti);
        return TRUE;
    }

    if (opts & OFLAG(t)) {
        pti = (PTHREADINFO)FIXKP(param1);
        if (pti == NULL) {
            return FALSE;
        }

        /*
         * Regular thread-locked objects
         */
        if (!(opts & OFLAG(x))) { // x is not legal from user - internal only
            Print("pti        pObj     Caller\n");
        }
        ScanThreadlocks(pti, 'o', NULL);
        ScanThreadlocks(pti, 'k', NULL);
        return TRUE;
    }


    if (!param1) {
        return FALSE;
    }

    Print("Thread Locks for object %x:\n", param1);
    Print("pti        pObj     Caller\n");

    FOREACHPTI(pti)
    ScanThreadlocks(pti, 'o', param1);
    ScanThreadlocks(pti, 'k', param1);
    NEXTEACHPTI(pti);

    Print("--- End Thread Lock List ---\n");

    return TRUE;
}

/*
 * Scans all threadlocked objects belonging to thread pti.
 * of type chType (o == regular objects, k == kernel objects, p == pool
 * Display each threadlock, or if pvSearch is non-NULL, just those locks on the
 * object at pvSearch.
 */
BOOL
ScanThreadlocks(
    PTHREADINFO pti,
    char        chType,
    PVOID       pvSearch)
{
    TL         tl;
    THREADINFO ti;
    PTL        ptl;

    if (!tryMove(ti, pti)) {
        Print("Idtl: Can't get pti data from %x.\n", pti);
        return FALSE;
    }
    switch (chType) {
    case 'o':
        ptl = ti.ptl;
        break;
    case 'k':
        ptl = ti.ptlW32;
        break;
    default:
        Print("Internal error, bad chType in ScanThreadlocks\n");
        return FALSE;
    }
    SAFEWHILE(ptl) {
        char ach[80];
        DWORD_PTR dwOffset;

        if (!tryMove(tl, ptl)) {
            Print("Idtl: Can't get ptl data from %x.\n", ptl);
            return FALSE;
        }

        if (!pvSearch || (tl.pobj == pvSearch)) {
            Print("%p %c %p", pti, chType, tl.pobj);
#if DBG
            GetSym(tl.pfnCaller, ach, &dwOffset);
            Print(" %s", ach);
            if (dwOffset) {
                Print("+%lx", dwOffset);
            }
#endif // !DBG
            if (chType == 'k') {
                GetSym(tl.pfnFree, ach, &dwOffset);
                Print(" (%s)", ach);
                if (dwOffset) {
                    Print("+%x", dwOffset);
                }
            }
            Print("\n");
        }
        ptl = tl.next;
    }
    return TRUE;
}

#endif // KERNEL



#ifdef KERNEL
/************************************************************************\
* Procedure: Idtmr
*
* Description: Dumps timer structures
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idtmr(
DWORD opts,
PVOID param1)
{
    PTIMER ptmr;
    TIMER tmr;
    THREADINFO ti;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == 0) {
        moveExpValuePtr(&ptmr, VAR(gptmrFirst));
        SAFEWHILE (ptmr) {
            Idtmr(0, ptmr);
            Print("\n");
            move(ptmr, &ptmr->ptmrNext);
        }
        return TRUE;
    }

    ptmr = (PTIMER)param1;

    if (ptmr == NULL) {
        Print("Expected ptmr address.\n");
        return FALSE;
    }

    move(tmr, ptmr);


    Print("Timer %08x:\n"
          "  ptmrNext       = %x\n"
          "  pti            = %x",
          ptmr,
          tmr.ptmrNext,
          tmr.pti);
    if (tryMove(ti, tmr.pti)) {
        WCHAR awch[80];
        ETHREAD EThread;

        if (tryMove(EThread, ti.pEThread)) {
            if (GetAppName(&EThread, &ti, awch, sizeof(awch))) {
                PWCHAR pwch = wcsrchr(awch, L'\\');
                if (pwch == NULL) {
                    pwch = awch;
                } else {
                    pwch++;
                }
                Print("  q 0x%08lx i %2x.%-3lx %ws",
                        ti.pq,
                        EThread.Cid.UniqueProcess,
                        EThread.Cid.UniqueThread,
                        pwch);
            }
        }
    }
    Print("\n"
          "  spwnd          = %x",
          tmr.spwnd);
    if (tmr.spwnd) {
        char ach[80];
        DebugGetWindowTextA(tmr.spwnd, ach);
        Print("  \"%s\"", ach);
    }
    Print("\n"
          "  nID            = %x\n"
          "  cmsCountdown   = %x\n"
          "  cmsRate        = %x\n"
          "  flags          = %s\n"
          "  pfn            = %x\n"
          "  ptiOptCreator  = %x\n",
          tmr.nID,
          tmr.cmsCountdown,
          tmr.cmsRate,
          GetFlags(GF_TMRF, (WORD)tmr.flags, NULL, TRUE),
          tmr.pfn,
          tmr.ptiOptCreator);

    return TRUE;
}
#endif // KERNEL




/************************************************************************\
* Procedure: Idu
*
* Description: Dump unknown object.  Does what it can figure out.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idu(
DWORD opts,
PVOID param1)
{
    HANDLEENTRY he, *phe;
    int i;
    DWORD dw;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        FOREACHHANDLEENTRY(phe, he, i)
            if (he.bType != TYPE_FREE && tryDword(&dw, FIXKP(he.phead))) {
                Idu(OFLAG(x), he.phead);
            }
        NEXTEACHHANDLEENTRY()
        return TRUE;
    }

    param1 = HorPtoP(FIXKP(param1), -1);
    if (param1 == NULL) {
        return FALSE;
    }

    if (!getHEfromP(NULL, &he, param1)) {
        return FALSE;
    }

    Print("--- %s object @%x ---\n", pszObjStr[he.bType], FIXKP(param1));
    switch (he.bType) {
    case TYPE_WINDOW:
        return Idw(0, param1);

    case TYPE_MENU:
        return Idm(0, param1);

#ifdef KERNEL
    case TYPE_CURSOR:
        return Idcur(0, param1);

#ifdef LATER
    case TYPE_INPUTQUEUE:
        return Idq(0, param1);
#endif

    case TYPE_HOOK:
        return Idhk(OFLAG(a) | OFLAG(g), NULL);

    case TYPE_DDECONV:
    case TYPE_DDEXACT:
        return Idde(0, param1);
#endif // KERNEL

    case TYPE_MONITOR:
        // LATER: - add dmon command
    case TYPE_CALLPROC:
    case TYPE_ACCELTABLE:
    case TYPE_SETWINDOWPOS:
    case TYPE_DDEACCESS:
    default:
        Print("not supported.\n", pszObjStr[he.bType]);
    }
    return TRUE;
}



#ifdef KERNEL
/***************************************************************************\
* dumphmgr - dumps object allocation counts for handle-table.
*
* 10-18-94 ChrisWil     Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
* 06-18-97 MCostea      made it work
\***************************************************************************/

BOOL Idumphmgr(
DWORD opts)
{
    PERFHANDLEINFO aLocalHti[TYPE_CTYPES], aLocalPrevHti[TYPE_CTYPES];
    PPERFHANDLEINFO pgahti;

    LONG lTotalAlloc, lTotalMax, lTotalCurrent;
    LONG lPrevTotalAlloc, lPrevTotalMax, lPrevTotalCurrent;
    SIZE_T lTotalSize, lPrevTotalSize;

    int idx;

    pgahti = EvalExp(VAR(gaPerfhti));
    if (!pgahti) {
        Print("\n!dumphmgr works only with debug versions of win32k.sys\n\n");
        return TRUE;
    }
    move(aLocalHti, pgahti);

    pgahti = EvalExp(VAR(gaPrevhti));
    if (!pgahti) {
        return TRUE;
    }

    move(aLocalPrevHti, pgahti);

    lTotalSize = lTotalAlloc = lTotalMax = lTotalCurrent = 0;
    lPrevTotalSize = lPrevTotalAlloc = lPrevTotalMax = lPrevTotalCurrent = 0;

    if (aLocalPrevHti[TYPE_WINDOW].lTotalCount) {
        Print("\nThe snapshot values come under the current ones\n");

        Print("Type             Allocated         Maximum       Count             Size\n");
        Print("______________________________________________________________________________");
        for(idx=1; idx < TYPE_CTYPES; idx++) {
            Print("\n%-15s  %8d %-+6d %7d %-+5d %6d %-+5d  %9d %-+d",
                  aszTypeNames[idx],
                  aLocalHti[idx].lTotalCount, aLocalHti[idx].lTotalCount - aLocalPrevHti[idx].lTotalCount,
                  aLocalHti[idx].lMaxCount, aLocalHti[idx].lMaxCount - aLocalPrevHti[idx].lMaxCount,
                  aLocalHti[idx].lCount, aLocalHti[idx].lCount - aLocalPrevHti[idx].lCount,
                  aLocalHti[idx].lSize, aLocalHti[idx].lSize - aLocalPrevHti[idx].lSize);
            if (aLocalPrevHti[TYPE_WINDOW].lTotalCount) {
                Print("\n                 %8d        %7d       %6d        %9d",
                      aLocalPrevHti[idx].lTotalCount,
                      aLocalPrevHti[idx].lMaxCount,
                      aLocalPrevHti[idx].lCount,
                      aLocalPrevHti[idx].lSize);
                lPrevTotalAlloc   += aLocalPrevHti[idx].lTotalCount;
                lPrevTotalMax     += aLocalPrevHti[idx].lMaxCount;
                lPrevTotalCurrent += aLocalPrevHti[idx].lCount;
                lPrevTotalSize    += aLocalPrevHti[idx].lSize;

            }
            lTotalAlloc   += aLocalHti[idx].lTotalCount;
            lTotalMax     += aLocalHti[idx].lMaxCount;
            lTotalCurrent += aLocalHti[idx].lCount;
            lTotalSize    += aLocalHti[idx].lSize;
        }
        Print("\n______________________________________________________________________________\n");
        Print("Totals           %8d %-+6d %7d %-+5d %6d %+-5d  %9d %-+d\n",
              lTotalAlloc, lTotalAlloc - lPrevTotalAlloc,
              lTotalMax, lTotalMax - lPrevTotalMax,
              lTotalCurrent, lTotalCurrent - lPrevTotalCurrent,
              lTotalSize, lTotalSize - lPrevTotalSize);
        Print("                 %8d        %7d       %6d        %9d\n",
              lPrevTotalAlloc, lPrevTotalMax, lPrevTotalCurrent, lPrevTotalSize);

    }
    else {
        Print("Type               Allocated  Maximum  Count   Size\n");
        Print("______________________________________________________");
        for(idx=1; idx < TYPE_CTYPES; idx++) {
            Print("\n%-17s  %9d  %7d  %6d  %d",
                  aszTypeNames[idx],
                  aLocalHti[idx].lTotalCount,
                  aLocalHti[idx].lMaxCount,
                  aLocalHti[idx].lCount,
                  aLocalHti[idx].lSize);
            lTotalAlloc   += aLocalHti[idx].lTotalCount;
            lTotalMax     += aLocalHti[idx].lMaxCount;
            lTotalCurrent += aLocalHti[idx].lCount;
            lTotalSize    += aLocalHti[idx].lSize;
        }
        Print("\n______________________________________________________\n");
        Print("Current totals     %9d  %7d  %6d  %d\n",
              lTotalAlloc, lTotalMax, lTotalCurrent, lTotalSize);
    }

    /*
     * If the argument-list contains the Snap option,
     * then copy the current counts to the previous ones
     */
    if (opts & OFLAG(s)) {
        (lpExtensionApis->lpWriteProcessMemoryRoutine)(
                (ULONG_PTR)(&(pgahti[0])),
                (void*)(aLocalHti),
                sizeof(aLocalHti),
                NULL);
    }

    return TRUE;
}
#endif // KERNEL



/************************************************************************\
* Procedure: dwrWorker
*
* Description: Dumps pwnd structures compactly to show relationships.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL dwrWorker(
    PWND pwnd,
    int tab)
{
    WND wnd;

    if (pwnd == 0) {
        return FALSE;
    }

    do {
        pwnd = FIXKP(pwnd);
        move(wnd, pwnd);
        DebugGetWindowTextA(pwnd, gach1);
        move(gcls, FIXKP(wnd.pcls));
        if (gcls.atomClassName < 0xC000) {
            switch (gcls.atomClassName) {
            case WC_DIALOG:
                strcpy(gach1, "WC_DIALOG");
                break;

            case DESKTOPCLASS:
                strcpy(gach1, "DESKTOP");
                break;

            case SWITCHWNDCLASS:
                strcpy(gach1, "SWITCHWND");
                break;

            case ICONTITLECLASS:
                strcpy(gach1, "ICONTITLE");
                break;

            default:
                if (gcls.atomClassName == 0) {
                    move(gach1, FIXKP(gcls.lpszAnsiClassName));
                } else {
                    sprintf(gach2, "0x%04x", gcls.atomClassName);
                }
            }
        } else {
            DebugGetClassNameA(gcls.lpszAnsiClassName, gach2);
        }
        Print("%08x%*s [%s|%s]", pwnd, tab, "", gach1, gach2);
        if (wnd.spwndOwner != NULL) {
            Print(" <- Owned by:%08x", FIXKP(wnd.spwndOwner));
        }
        Print("\n");
        if (wnd.spwndChild != NULL) {
            dwrWorker(wnd.spwndChild, tab + 2);
        }
    } SAFEWHILE ((pwnd = wnd.spwndNext) && tab > 0);
    return TRUE;
}


/************************************************************************\
* Procedure: Idw
*
* Description: Dumps pwnd structures
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idw(
    DWORD opts,
    PVOID param1)
{
    WND wnd;
    CLS cls;
    PWND pwnd = param1;
    char ach[80];
    DWORD_PTR dwOffset;
    int  ix;
    DWORD tempDWord;
    DWORD dwWOW;

    if (opts & OFLAG(a)) {
#ifdef KERNEL
        DESKTOP desk, *pdesk;
        PWND pwnd;
        WCHAR wach[80];

        if (param1 != 0) {
            Print("window parameter ignored with -a option.\n");
        }
        FOREACHDESKTOP(pdesk)
            if (tryMove(desk, pdesk)) {
                OBJECT_HEADER_NAME_INFO NameInfo;
                OBJECT_HEADER Head;

                move(Head, OBJECT_TO_OBJECT_HEADER(pdesk));
                move(NameInfo, ((PCHAR)(OBJECT_TO_OBJECT_HEADER(pdesk)) - Head.NameInfoOffset));
                moveBlock(wach, FIXKP(NameInfo.Name.Buffer), NameInfo.Name.Length);
                wach[NameInfo.Name.Length / sizeof(WCHAR)] = L'\0';
                Print("\n----Windows for %ws desktop @%08lx:\n\n", wach, pdesk);
                move(pwnd, &(desk.pDeskInfo->spwnd));
                if (!Idw((opts & ~OFLAG(a)) | OFLAG(p), pwnd)) {
                    return FALSE;
                }
            }
        NEXTEACHDESKTOP(pdesk)
#else // !KERNEL
        TEB teb;

        if (GetTargetTEB(&teb, NULL)) {
            PDESKTOPINFO pdi;
            DESKTOPINFO di;

            pdi = ((PCLIENTINFO)&teb.Win32ClientInfo[0])->pDeskInfo;
            move(di, pdi);
            return Idw(opts & ~OFLAG(a) | OFLAG(p), FIXKP(di.spwnd));
        }
#endif // !KERNEL
        return TRUE;
    }

    /*
     * t is like EnumThreadWindows.
     */
    if (opts & OFLAG(t)) {
#ifdef KERNEL
        PTHREADINFO pti, ptiWnd;
        PDESKTOP pdesk;
        PDESKTOPINFO pdi;
        /*
         * Get the desktop's first child window
         */
        pti = (PTHREADINFO)param1;
        if (!tryMove(pdesk, &pti->rpdesk)
                || !tryMove(pdi, &pdesk->pDeskInfo)
                || !tryMove(pwnd, &pdi->spwnd)
                || !tryMove(pwnd, &pwnd->spwndChild)) {
            return FALSE;
        }
        /*
         * Walk the sibling chain looking for pwnd owned by pti.
         */
        SAFEWHILE (pwnd) {
            if (tryMove(ptiWnd, &(pwnd->head.pti)) && (ptiWnd == pti)) {
                if (!Idw(opts & ~OFLAG(t), pwnd)) {
                    return FALSE;
                }
            }
            if (!tryMove(pwnd, &pwnd->spwndNext)) {
                return FALSE;
            }
        }
        return TRUE;

#else // !KERNEL
        Print("t parameter not supported for NTSD at this point\n");
#endif // !KERNEL
    }

    /*
     * See if the user wants all top level windows.
     */
    if (param1 == NULL || opts & (OFLAG(p) | OFLAG(s))) {
        /*
         * Make sure there was also a window argument if p or s.
         */

        if (param1 == NULL && (opts & (OFLAG(p) | OFLAG(s)))) {
            Print("Must specify window with '-p' or '-s' options.\n");
            return FALSE;
        }

        if (param1 && (pwnd = HorPtoP(pwnd, TYPE_WINDOW)) == NULL) {
            return FALSE;
        }

        if (opts & OFLAG(p)) {
            Print("pwndParent = 0x%p\n", pwnd);
            if (!tryMove(pwnd, FIXKP(&pwnd->spwndChild))) {
                Print("<< Can't get WND >>\n");
                return TRUE; // we don't need to have the flags explained!
            }
            SAFEWHILE (pwnd) {
                if (!Idw(opts & ~OFLAG(p), pwnd)) {
                    return FALSE;
                }
                move(pwnd, FIXKP(&pwnd->spwndNext));
            }
            return TRUE;

        } else if (opts & OFLAG(s)) {
            move(pwnd, FIXKP(&pwnd->spwndParent));
            return Idw((opts | OFLAG(p)) & ~OFLAG(s), pwnd);

        } else {    // pwnd == NULL & !p & !s
#ifdef KERNEL
            Q q;
            PQ pq;
            THREADINFO ti;
            PWND pwnd;

            moveExpValuePtr(&pq, VAR(gpqForeground));
            move(q, pq);
            move(ti, q.ptiKeyboard);
            if (ti.rpdesk == NULL) {
                Print("Foreground thread doesn't have a desktop.\n");
                return FALSE;
            }
            move(pwnd, &(ti.pDeskInfo->spwnd));
            Print("pwndDesktop = 0x%p\n", pwnd);
            return Idw(opts | OFLAG(p), pwnd);
#else  // !KERNEL
            return Idw(opts | OFLAG(a), 0);
#endif // !KERNEL
        }
    }

    if (param1 && (pwnd = HorPtoP(param1, TYPE_WINDOW)) == NULL) {
        Print("Idw: 0x%p is not a pwnd.\n", param1);
        return FALSE;
    }

    if (opts &  OFLAG(r)) {
        dwrWorker(FIXKP(pwnd), 0);
        return TRUE;
    }

    move(wnd, FIXKP(pwnd));

#ifdef KERNEL
    /*
     * Print simple thread info.
     */
    if (wnd.head.pti) {
        Idt(OFLAG(p), (PVOID)wnd.head.pti);
    }
#endif // KERNEL
    /*
     * Print pwnd.
     */
    Print("pwnd    = 0x%p", pwnd);
    /*
     * Show z-ordering/activation revelant info
     */
    if (opts & OFLAG(z)) {
        if (wnd.ExStyle & WS_EX_TOPMOST) {
            Print(" TOPMOST");
        }
        if (!(wnd.style & WS_VISIBLE)) {
            Print(" HIDDEN");
        }
        if (wnd.style & WS_DISABLED) {
            Print(" DISABLED");
        }
        if (wnd.spwndOwner != NULL) {
            DebugGetWindowTextA(wnd.spwndOwner, ach);
            Print(" OWNER:0x%p \"%s\"", wnd.spwndOwner, ach);
        }
    }
    Print("\n");

    if (!(opts & OFLAG(v))) {

        /*
         * Print title string.
         */
        DebugGetWindowTextA(pwnd, ach);
        Print("title   = \"%s\"\n", ach);

        /*
         * Print wndproc symbol string.
         */
        if (opts & OFLAG(h)) {
            if (IsWOWProc (wnd.lpfnWndProc)) {
            UnMarkWOWProc(wnd.lpfnWndProc,dwWOW);
            Print("wndproc = %04lx:%04lx (WOW) (%s)",
                    HIWORD(dwWOW),LOWORD(dwWOW),
                    TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
            }
            else {
                GetSym((LPVOID)wnd.lpfnWndProc, ach, &dwOffset);
                Print("wndproc = 0x%p = \"%s\" (%s)", wnd.lpfnWndProc, ach,
                    TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
            }
        } else {
            if (IsWOWProc (wnd.lpfnWndProc)) {
                UnMarkWOWProc(wnd.lpfnWndProc,dwWOW);
                Print("wndproc = %04lx:%04lx (WOW) (%s)",
                        HIWORD(dwWOW),LOWORD(dwWOW),
                        TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
            }
            else {
                Print("wndproc = 0x%p (%s)", wnd.lpfnWndProc,
                    TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
            }
        }

        /*
         * Display the class name/atom
         */
        move(cls, FIXKP(wnd.pcls));
        if (cls.atomClassName < 0xC000) {
            sprintf(ach, "0x%04x", cls.atomClassName);
        } else {
            DebugGetClassNameA(cls.lpszAnsiClassName, ach);
        }
        Print(" Class:\"%s\"\n", ach);
    } else {
        /*
         * Get the PWND structure.  Ignore class-specific data for now.
         */
        Print("\tpti               @0x%p\n", FIXKP(wnd.head.pti));
        Print("\thandle             0x%p\n", wnd.head.h);

        DebugGetWindowTextA(wnd.spwndNext, ach);
        Print("\tspwndNext         @0x%p     \"%s\"\n", wnd.spwndNext, ach);
        DebugGetWindowTextA(wnd.spwndParent, ach);
        Print("\tspwndParent       @0x%p     \"%s\"\n", wnd.spwndParent, ach);
        DebugGetWindowTextA(wnd.spwndChild, ach);
        Print("\tspwndChild        @0x%p     \"%s\"\n", wnd.spwndChild, ach);
        DebugGetWindowTextA(wnd.spwndOwner, ach);
        Print("\tspwndOwner        @0x%p     \"%s\"\n", wnd.spwndOwner, ach);

        Print("\trcWindow           (%d,%d)-(%d,%d) %dx%d\n",
                wnd.rcWindow.left, wnd.rcWindow.top,
                wnd.rcWindow.right, wnd.rcWindow.bottom,
                wnd.rcWindow.right - wnd.rcWindow.left,
                wnd.rcWindow.bottom - wnd.rcWindow.top);

        Print("\trcClient           (%d,%d)-(%d,%d) %dx%d\n",
                wnd.rcClient.left, wnd.rcClient.top,
                wnd.rcClient.right, wnd.rcClient.bottom,
                wnd.rcClient.right - wnd.rcClient.left,
                wnd.rcClient.bottom - wnd.rcClient.top);

        if (IsWOWProc (wnd.lpfnWndProc)) {
            UnMarkWOWProc(wnd.lpfnWndProc,dwWOW);
            Print("\tlpfnWndProc       @%04lx:%04lx (WOW) (%s)\n",
                    HIWORD(dwWOW),LOWORD(dwWOW),
                    TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
        }
        else {
            GetSym((LPVOID)wnd.lpfnWndProc, ach, &dwOffset);
            Print("\tlpfnWndProc       @0x%p     (%s) %s\n", wnd.lpfnWndProc, ach,
                TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
        }
        move(cls, FIXKP(wnd.pcls));
        if (cls.atomClassName < 0xC000) {
            sprintf(ach, "0x%04x", cls.atomClassName);
        } else {
            DebugGetClassNameA(cls.lpszAnsiClassName, ach);
        }
        Print("\tpcls              @0x%p     (%s)\n",
                wnd.pcls, ach);

        Print("\thrgnUpdate         0x%p\n",
                wnd.hrgnUpdate);
        DebugGetWindowTextA(wnd.spwndLastActive, ach);
        Print("\tspwndLastActive   @0x%p     \"%s\"\n",
              wnd.spwndLastActive, ach);
        Print("\tppropList         @0x%p\n"
              "\tpSBInfo           @0x%p\n",
              wnd.ppropList,
              wnd.pSBInfo);
        if (wnd.pSBInfo) {
            SBINFO asb;

            moveBlock(&asb, FIXKP(wnd.pSBInfo), sizeof(asb));
            Print("\t  SBO_FLAGS =      %s\n"
                  "\t  SBO_HMIN  =      %d\n"
                  "\t  SBO_HMAX  =      %d\n"
                  "\t  SBO_HPAGE =      %d\n"
                  "\t  SBO_HPOS  =      %d\n"
                  "\t  SBO_VMIN  =      %d\n"
                  "\t  SBO_VMAX  =      %d\n"
                  "\t  SBO_VPAGE =      %d\n"
                  "\t  SBO_VPOS  =      %d\n",
                    GetFlags(GF_SB, (WORD)asb.WSBflags, NULL, TRUE),
                    asb.Horz.posMin,
                    asb.Horz.posMax,
                    asb.Horz.page,
                    asb.Horz.pos,
                    asb.Vert.posMin,
                    asb.Vert.posMax,
                    asb.Vert.page,
                    asb.Vert.pos);
        }
        Print("\tspmenuSys         @0x%p\n"
              "\tspmenu/id         @0x%p\n",
              wnd.spmenuSys,
              wnd.spmenu);
        Print("\thrgnClip           0x%p\n",
              wnd.hrgnClip);


        /*
         * Print title string.
         */
        DebugGetWindowTextA(pwnd, ach);
        Print("\tpName              \"%s\"\n",
              ach);
        Print("\tdwUserData         0x%p\n",
              wnd.dwUserData);
        Print("\tstate              0x%08lx\n"
              "\tstate2             0x%08lx\n"
              "\tExStyle            0x%08lx\n"
              "\tstyle              0x%08lx\n"
              "\tfnid               0x%08lx\n"
              "\thImc               0x%08lx\n"
              "\tbFullScreen        %d\n"
              "\thModule            0x%08lx\n",
              wnd.state,
              wnd.state2,
              wnd.ExStyle,
              wnd.style,
              (DWORD)wnd.fnid,
              wnd.hImc,
              GetFullScreen(&wnd),
              wnd.hModule);
    }
    /*
     * Print out all the flags
     */
    if (opts & OFLAG(f)) {
        int i;
        WORD wFlag;
        PBYTE pbyte = (PBYTE)(&(wnd.state));

        for (i = 0; i < ARRAY_SIZE(aWindowFlags); i++) {
            wFlag = aWindowFlags[i].wFlag;
            if (pbyte[HIBYTE(wFlag)] & LOBYTE(wFlag)) {
                Print("\t%-18s\t%lx:%02lx\n",
                        aWindowFlags[i].pszText,
                        (PBYTE)&(pwnd->state) + HIBYTE(wFlag),
                        LOBYTE(wFlag));
            }
        }
    }

    if (opts & OFLAG(w)) {
        Print("\t%d window bytes: ", wnd.cbwndExtra);
        if (wnd.cbwndExtra) {
            for (ix=0; ix < wnd.cbwndExtra; ix += 4) {
                 DWORD UNALIGNED *pdw;

                 pdw = (DWORD UNALIGNED *) ((BYTE *) (pwnd+1) + ix);
                 move(tempDWord, pdw);
                 Print("%08x ", tempDWord);
            }
        }
        Print("\n");
    }

    /*
     * Print window properties.
     */
    if (opts & OFLAG(o)) {
        PSERVERINFO psi;
        PPROPLIST   ppropList;
        PROPLIST    propList;
        PROP        prop;
        UINT        i, j;

        struct {
            LPSTR   pstrName;
            ATOM    atom;
            LPSTR   pstrSymbol;
        } apropatom[] =
        {
            "Icon",         0,  MAKEINTRESOURCEA(FIELD_OFFSET(SERVERINFO, atomIconProp)),
            "IconSM",       0,  MAKEINTRESOURCEA(FIELD_OFFSET(SERVERINFO, atomIconSmProp)),
            "ContextHelpID",0,  MAKEINTRESOURCEA(FIELD_OFFSET(SERVERINFO, atomContextHelpIdProp)),
            "Checkpoint",   0,  VAR(atomCheckpointProp),
            "Flash State",  0,  VAR(gaFlashWState),
            "DDETrack",     0,  VAR(atomDDETrack),
            "QOS",          0,  VAR(atomQOS),
            "DDEImp",       0,  VAR(atomDDEImp),
            "WNDOBJ",       0,  VAR(atomWndObj),
            "IMELevel",     0,  VAR(atomImeLevel),
        };


        /*
         * Get the atom values for internal properties and put them in apropatom.atom
         */
        moveExpValuePtr(&psi, VAR(gpsi));
        for (i = 0; i < ARRAY_SIZE(apropatom); i++) {
            if (!IS_PTR(apropatom[i].pstrSymbol)) {

                /*
                 * The atom is stored in psi.
                 */
                move(apropatom[0].atom, (ATOM *) ((BYTE *)psi + LOWORD(apropatom[i].pstrSymbol)));
            } else {

                /*
                 * The atom is a global.
                 */
                moveExpValue(&apropatom[i].atom, apropatom[i].pstrSymbol);
            }
        }

        /*
         * Print the property list structure.
         */
        ppropList = wnd.ppropList;
        if (!ppropList) {
            Print("\tNULL Property List\n");
        } else {
            move(propList, ppropList);
            Print("\tProperty List @0x%08x : %d Properties, %d total entries, %d free entries\n",
                    ppropList,
                    propList.iFirstFree,
                    propList.cEntries,
                    propList.cEntries - propList.iFirstFree);

            /*
             * Print each property.
             */
            for (i = 0; !IsCtrlCHit() && i < propList.iFirstFree; i++) {
                LPSTR pstrInternal;

                move(prop, &ppropList->aprop[i]);

                /*
                 * Find name for internal property.
                 */
                pstrInternal = "";
                if (prop.fs & PROPF_INTERNAL) {
                    for (j = 0; j < ARRAY_SIZE(apropatom); j++) {
                        if (prop.atomKey == apropatom[j].atom) {
                            pstrInternal = apropatom[j].pstrName;
                            break;
                        }
                    }
                }

                Print("\tProperty %d\n", i);
                Print("\t\tatomKey     0x%04x %s\n", prop.atomKey, pstrInternal);
                Print("\t\tfs          0x%04x %s\n", prop.fs, GetFlags(GF_PROP, prop.fs, NULL, FALSE));
                Print("\t\thData       0x%08x (%d)\n", prop.hData, prop.hData);

        #ifdef KERNEL
                if (prop.fs & PROPF_INTERNAL) {
                    if (j == 3) {
                        CHECKPOINT  cp;
                        PCHECKPOINT pcp = (PCHECKPOINT)prop.hData;
                        move(cp, pcp);
                        Print("\t\tCheckPoint:\n");
                        Print("\t\trcNormal (%d,%d),(%d,%d) %dx%d\n",
                                cp.rcNormal.left,
                                cp.rcNormal.top,
                                cp.rcNormal.right,
                                cp.rcNormal.bottom,
                                cp.rcNormal.right - cp.rcNormal.left,
                                cp.rcNormal.bottom - cp.rcNormal.top);

                        Print("\t\tptMin    (%d,%d)\n",                cp.ptMin.x, cp.ptMin.y);
                        Print("\t\tptMax    (%d,%d)\n",                cp.ptMax.x, cp.ptMax.y);
                        Print("\t\tfDragged:%d\n",                     cp.fDragged);
                        Print("\t\tfWasMaximizedBeforeMinimized:%d\n", cp.fWasMaximizedBeforeMinimized);
                        Print("\t\tfWasMinimizedBeforeMaximized:%d\n", cp.fWasMinimizedBeforeMaximized);
                        Print("\t\tfMinInitialized:%d\n",              cp.fMinInitialized);
                        Print("\t\tfMaxInitiailized:%d\n",             cp.fMaxInitialized);
                    }
                }
        #endif // ifdef KERNEL

                Print("\n");
            }
        }
    }

    Print("---\n");

    return TRUE;
}


#ifdef KERNEL
/************************************************************************\
* Procedure: Idwpi
*
* Description: Dumps WOWPROCESSINFO structs
*
* Returns:  fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Idwpi(
DWORD opts,
PVOID param1)
{

    PWOWPROCESSINFO pwpi;
    WOWPROCESSINFO wpi;
    PPROCESSINFO ppi;

    if (param1 == 0) {
        FOREACHPPI(ppi)
            Print("Process %x.\n", FIXKP(ppi));
            move(pwpi, FIXKP(&ppi->pwpi));
            SAFEWHILE (pwpi) {
                Idwpi(0, pwpi);
                Print("\n");
                move(pwpi, FIXKP(&pwpi->pwpiNext));
            }
        NEXTEACHPPI()
        return TRUE;
    }

    if (opts & OFLAG(p)) {
        ppi = (PPROCESSINFO)FIXKP(param1);
        move(pwpi, &ppi->pwpi);
        if (pwpi == NULL) {
            Print("No pwpis for this process.\n");
            return TRUE;
        }
        SAFEWHILE (pwpi) {
            Idwpi(0, pwpi);
            Print("\n");
            move(pwpi, &pwpi->pwpiNext);
        }
        return TRUE;
    }

    pwpi = (PWOWPROCESSINFO)FIXKP(param1);
    move(wpi, pwpi);

    Print("PWOWPROCESSINFO @ 0x%p\n", pwpi);
    Print("\tpwpiNext             0x%08lx\n", wpi.pwpiNext);
    Print("\tptiScheduled         0x%08lx\n", wpi.ptiScheduled);
    Print("\tnTaskLock            0x%08lx\n", wpi.nTaskLock);
    Print("\tptdbHead             0x%08lx\n", wpi.ptdbHead);
    Print("\tlpfnWowExitTask      0x%08lx\n", wpi.lpfnWowExitTask);
    Print("\tpEventWowExec        0x%08lx\n", wpi.pEventWowExec);
    Print("\thEventWowExecClient  0x%08lx\n", wpi.hEventWowExecClient);
    Print("\tnSendLock            0x%08lx\n", wpi.nSendLock);
    Print("\tnRecvLock            0x%08lx\n", wpi.nRecvLock);
    Print("\tCSOwningThread       0x%08lx\n", wpi.CSOwningThread);
    Print("\tCSLockCount          0x%08lx\n", wpi.CSLockCount);
    return TRUE;
}
#endif // KERNEL

/************************************************************************\
* Procedure: Idha
*
* Description: Walks the global array of heaps gWin32Heaps looking for an
* allocation that contain the address passed in.  Then does a sanity check
* on the entire heap the allocations belongs to.
* DHAVerifyHeap is a helper procedure.  Note that the passed in parameter is
* already mapped
*
* Returns:
*
* 12/7/1998 Created MCostea
*
\************************************************************************/

#ifdef KERNEL

BOOL DHAVerifyHeap(
PWIN32HEAP pHeap,
BOOL bVerbose)
{
    DbgHeapHead Alloc, *pAlloc = pHeap->pFirstAlloc;
    int sizeHead, counter = 0;
    char    szHeadOrTail[HEAP_CHECK_SIZE];

    if (pAlloc == NULL)
        return FALSE;

    sizeHead = pHeap->dwFlags & WIN32_HEAP_USE_GUARDS ?
               sizeof(pHeap->szHead) : 0;

    do {
        if (!tryMove(Alloc, pAlloc)) {
            Print("Failed to read pAlloc from %p\n", pAlloc);
            return FALSE;
        }
        /*
         * Check the mark, header and tail
         */
        if (Alloc.mark != HEAP_ALLOC_MARK) {
            Print("!!! Bad mark found in allocation use !dso DbgHeapHead %p\n", pAlloc);
        }
        if (sizeHead) {
            if (!tryMove(szHeadOrTail, (PBYTE)pAlloc-sizeof(szHeadOrTail))) {
                Print("Failed to read szHead from %p\n", (PBYTE)pAlloc-sizeof(szHeadOrTail));
                return FALSE;
            }
            if (!RtlEqualMemory(szHeadOrTail, pHeap->szHead, sizeHead)) {
                Print("Head pattern corrupted for allocation %#p\n", pAlloc);
            }
            if (!tryMove(szHeadOrTail, (PBYTE)pAlloc + sizeof(DbgHeapHead) + Alloc.size)) {
                Print("Failed to read szHead from %p\n", (PBYTE)pAlloc + sizeof(DbgHeapHead) + Alloc.size);
                return FALSE;
            }
            if (!RtlEqualMemory(szHeadOrTail, pHeap->szTail, sizeHead)) {
                Print("Tail pattern corrupted for allocation %#p\n", pAlloc);
            }
        }
        if (bVerbose) {
            Print("Allocation %#p, tag %04d size %08d\n", pAlloc, Alloc.tag, Alloc.size);
        }
        if (counter++ > 100) {
            Print(".");
            counter = 0;
        }
        pAlloc = Alloc.pNext;
    } while (pAlloc != NULL);
    if (bVerbose) {
        Print("To dump an allocation use !dso DBGHEAPHEAD address\n");
    }

    return TRUE;
}

BOOL Idha(
DWORD opts,
PVOID pointer)
{
    int ind, counter;
    SIZE_T sizeHead;
    WIN32HEAP localgWin32Heaps[MAX_HEAPS];

    if (pointer == 0 && (opts & OFLAG(a) ==0)) {
        Print("Wrong usage: dha takes a pointer as a parameter\n");
        return FALSE;
    }

    if (!tryMoveBlock(localgWin32Heaps, EvalExp(VAR(gWin32Heaps)), sizeof(localgWin32Heaps)))
    {
        Print("Can't read the heap globals fix symbols and !reload win32k.sys\n");
        return TRUE;
    }

    /*
     * Walk gWin32Heaps array and look for an allocation containing this pointer
     */
    for (ind = counter = 0; ind < MAX_HEAPS; ind++) {

        /*
         * Is the address in this heap?
         */
        if ((opts & OFLAG(a)) == 0) {
            if ((PVOID)localgWin32Heaps[ind].heap > pointer ||
                (PBYTE)localgWin32Heaps[ind].heap + localgWin32Heaps[ind].heapReserveSize < (PBYTE)pointer) {

                continue;
            }
        }

        Print("\nHeap number %d ", ind);
        Print("at address %p, flags %d is ", localgWin32Heaps[ind].heap, localgWin32Heaps[ind].dwFlags);


        if (localgWin32Heaps[ind].dwFlags & WIN32_HEAP_INUSE) {

            DbgHeapHead  Alloc, *pAlloc;

            Print("in use\n");
            if (localgWin32Heaps[ind].pFirstAlloc == NULL)
                continue;

            pAlloc = localgWin32Heaps[ind].pFirstAlloc;
            if (localgWin32Heaps[ind].dwFlags & WIN32_HEAP_USE_GUARDS) {
                sizeHead =  sizeof(localgWin32Heaps[0].szHead);
                Print("    has string quards szHead %s, szTail %s\n",
                        localgWin32Heaps[0].szHead,
                        localgWin32Heaps[0].szTail);
            } else {
                sizeHead =  0;
                Print("no string quards\n");
            }

            if (opts & OFLAG(a)) {
                if (DHAVerifyHeap(&localgWin32Heaps[ind], opts & OFLAG(v))) {
                    Print("WIN32HEAP at %p is healthy\n", localgWin32Heaps[ind].heap);
                }
                continue;
            }
            do {
                if (!tryMove(Alloc, pAlloc)) {
                    Print("Failed to read pAlloc %p\n", pAlloc);
                    return TRUE;
                }
                if ((PBYTE)pAlloc - sizeHead < (PBYTE)pointer &&
                    (PBYTE)pAlloc + sizeof(DbgHeapHead) + Alloc.size + sizeHead > (PBYTE)pointer) {

                    /*
                     * Found the allocation
                     */
                    Print("Found allocation %p ", pAlloc);
                    if (pointer == (PBYTE)pAlloc + sizeof(DbgHeapHead)) {
                        Print("as the begining of a heap allocated block\n");
                    } else {
                        Print("inside a heap allocated block\n");
                    }
                    Print("tag %04d size %08d now verify the heap\n", Alloc.tag, Alloc.size);
                    /*
                     * Verify the entire heap for corruption
                     */
                    if (DHAVerifyHeap(&localgWin32Heaps[ind], opts & OFLAG(v))) {
                        Print("WIN32HEAP at %p is healthy\n", localgWin32Heaps[ind].heap);
                    }
                    return TRUE;
                } else {
                    pAlloc = Alloc.pNext;
                    if (counter++ > 100) {
                        counter = 0;
                        Print(".");
                    }
                }

            } while (pAlloc != NULL);
        } else {
            Print("NOT in use\n");
        }
    }
    Print("No heap contains this pointer %p\n", pointer);
    return TRUE;
}

/***************************************************************************\
* dws   - dump windows stations
* dws h - dump windows stations plus handle list
*
* Dump WindowStation
*
* 8-11-94 SanfordS  Created
* 6/9/1995 SanfordS made to fit stdexts motif
\***************************************************************************/
BOOL Idws(
DWORD opts,
PVOID param1)
{
    WINDOWSTATION           winsta, *pwinsta;
    WCHAR                   ach[80];
    OBJECT_HEADER           Head;
    OBJECT_HEADER_NAME_INFO NameInfo;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == 0) {

        FOREACHWINDOWSTATION(pwinsta)

            Idws(0, pwinsta);
            Print("\n");

        NEXTEACHWINDOWSTATION(pwinsta)

        return TRUE;
    }

    pwinsta = param1;
    move(winsta, pwinsta);
    move(Head, OBJECT_TO_OBJECT_HEADER(pwinsta));
    move(NameInfo, ((PCHAR)(OBJECT_TO_OBJECT_HEADER(pwinsta)) - Head.NameInfoOffset));
    move(ach, NameInfo.Name.Buffer);
    ach[NameInfo.Name.Length / sizeof(WCHAR)] = 0;
    Print("Windowstation: %ws @%0lx\n", ach, pwinsta);
    Print("  # Opens            = %d\n", Head.HandleCount);
    Print("  pTerm              = %0lx\n", winsta.pTerm);
    Print("  rpdeskList         = %0lx\n", winsta.rpdeskList);
    Print("  dwFlags            = %0lx\n", winsta.dwWSF_Flags);
    Print("  spklList           = %0lx\n", winsta.spklList);
    Print("  ptiClipLock        = %0lx\n", winsta.ptiClipLock);
    Print("  spwndClipOpen      = %0lx\n", winsta.spwndClipOpen);
    Print("  spwndClipViewer    = %0lx\n", winsta.spwndClipViewer);
    Print("  spwndClipOwner     = %0lx\n", winsta.spwndClipOwner);
    Print("  pClipBase          = %0lx\n", winsta.pClipBase);
    Print("  cNumClipFormats    = %0lx\n", winsta.cNumClipFormats);
    Print("  ptiDrawingClipboard= %0lx\n", winsta.ptiDrawingClipboard);
    Print("  fClipboardChanged  = %d\n",   winsta.fClipboardChanged);
    Print("  pGlobalAtomTable   = %0lx\n", winsta.pGlobalAtomTable);
    Print("  luidUser           = %0lx.%lx\n", winsta.luidUser.HighPart,
            winsta.luidUser.LowPart);

    return TRUE;
}

/***************************************************************************\
* ddl   - dump desktop log
*
* 12-03-97 CLupu  Created
\***************************************************************************/

BOOL Iddl(
DWORD opts,
PVOID param1)
{
#ifdef LOGDESKTOPLOCKS
    PDESKTOP                pdesk;
    DESKTOP                 desk;
    OBJECT_HEADER           Head;
    LogD                    log;
    PLogD                   pLog;
    BOOL                    bExtra = FALSE;
    int                     i, ind;
    DWORD_PTR               dwOffset;
    CHAR                    symbol[160];

    if (param1 == 0) {
        Print("Use !ddl pdesk\n");
        return TRUE;
    }

    pdesk = (PDESKTOP)param1;
    move(desk, pdesk);
    move(Head, OBJECT_TO_OBJECT_HEADER(pdesk));

    Print("Desktop locks:\n\n");
    Print("# HandleCount      = %d\n", Head.HandleCount);
    Print("# PointerCount     = %d\n", Head.PointerCount);
    Print("# Log PointerCount = %d\n\n", desk.nLockCount);

    pLog = desk.pLog;

    if (opts & OFLAG(v)) {
        bExtra = TRUE;
    }

    for (i = 0; i < desk.nLogCrt; i++) {
        move(log, pLog);

        Print("%s Tag %6d Extra %8lx\n",
              (log.type ? "LOCK  " : "UNLOCK"),
               log.tag, log.extra);

        if (bExtra) {
            Print("----------------------------------------------\n");

            for (ind = 0; ind < 6; ind++) {
                if (log.trace[ind] == 0) {
                    break;
                }

                GetSym((PVOID)log.trace[ind], symbol, &dwOffset);
                if (*symbol) {
                    Print("\t%s", symbol);
                    if (dwOffset) {
                        Print(" + %x\n", dwOffset);
                    }
                }
            }
            Print("\n");
        }

        pLog++;
    }
    return TRUE;
#else
    Print("!ddl is available only on LOGDESKTOPLOCKS enabled builds of win32k.sys\n");
    return FALSE;
    UNREFERENCED_PARAMETER(opts);
    UNREFERENCED_PARAMETER(param1);
#endif // LOGDESKTOPLOCKS
}

/***************************************************************************\
* dcss   - dump critical section stack
*
* Dump critical section stack
*
* 12-27-96 CLupu  Created
\***************************************************************************/
BOOL Idcss(
DWORD opts,
PVOID param1)
{
    int        trace;
    DWORD_PTR  off;
    PCRITSTACK pstack;
    CRITSTACK  stack;
    CHAR       symbol[160];

    /*
     * Set up globals for speed.
     */
    moveExp(&pstack, VAR(gCritStack));
    if (!tryMove(stack, pstack)) {
        Print("dcss: Could not get CRITSTACK.\n");
        return FALSE;
    }

    if (stack.nFrames > 0) {
        Print("--- Critical section stack trace ---\n");

        Print("\nthread : %8lx\n", stack.thread);

        Print("\n\tRetAddr   Call\n");

        for (trace = 0; trace < stack.nFrames; trace++) {
            GetSym((PVOID)stack.trace[trace], symbol, &off);
            Print("\t%#p  %s+%x\n", stack.trace[trace], symbol, off);
        }
    }

    return TRUE;

    UNREFERENCED_PARAMETER(opts);
    UNREFERENCED_PARAMETER(param1);
}

void PrintStackTrace(
    PVOID* parrTrace,
    int    tracesCount)
{
    int       traceInd;
    DWORD_PTR dwOffset;
    CHAR      symbol[160];

    for (traceInd = 0; traceInd < tracesCount; traceInd++) {

        if (parrTrace[traceInd] == 0) {
            break;
        }

        GetSym(parrTrace[traceInd], symbol, &dwOffset);
        if (*symbol) {
            Print("\t%s", symbol);
            if (dwOffset) {
                Print("+%x\n", dwOffset);
            }
        }
    }
    Print("\n");
}

BOOL Idvs(
DWORD opts,
PVOID param1)
{
    PWin32Section   pSection;
    Win32Section    Section;
    PWin32MapView   pView;
    Win32MapView    View;
    BOOL            bIncludeStackTrace = FALSE;

    if (EvalExp(VAR(gpSections)) == NULL) {
        Print("!dvs is available if TRACE_MAP_VIEWS is defined\n");
        return FALSE;
    }

    moveExpValuePtr(&pSection, VAR(gpSections));

    if (opts & OFLAG(s)) {
        bIncludeStackTrace = TRUE;
    }

    while (pSection != NULL) {
        if (!tryMove(Section, pSection)) {
            Print("!dvs: Could not get pSection structure for %#p\n", pSection);
            return FALSE;
        }

        Print(">>--------------------------------------\n");

        Print("Section          %#p\n"
              "   pFirstView    %#p\n"
              "   SectionObject %#p\n"
              "   SectionSize   0x%x\n"
              "   SectionTag    0x%x\n",
              pSection,
              Section.pFirstView,
              Section.SectionObject,
              Section.SectionSize,
              Section.SectionTag);

        if (bIncludeStackTrace) {
#ifdef MAP_VIEW_STACK_TRACE
            DWORD_PTR dwOffset;
            CHAR  symbol[160];
            int   ind;

            for (ind = 0; ind < MAP_VIEW_STACK_TRACE_SIZE; ind++) {
                if (Section.trace[ind] == 0) {
                    break;
                }

                GetSym((PVOID)Section.trace[ind], symbol, &dwOffset);
                if (*symbol) {
                    Print("   %s", symbol);
                    if (dwOffset) {
                        Print("+%x\n", dwOffset);
                    }
                }
            }
            Print("\n");
#endif // MAP_VIEW_STACK_TRACE
        }

        pView = Section.pFirstView;

        while (pView != NULL) {
            if (!tryMove(View, pView)) {
                Print("!dvs: Could not get pView structure for %#p\n", pView);
                return FALSE;
            }

            Print("Views: ---------------------------------\n"
                  " View          %#p\n"
                  "    pViewBase  %#p\n"
                  "    ViewSize   %#p\n",
                  pView,
                  View.pViewBase,
                  View.ViewSize);

            if (bIncludeStackTrace) {
#ifdef MAP_VIEW_STACK_TRACE
                DWORD_PTR dwOffset;
                CHAR  symbol[160];
                int   ind;

                for (ind = 0; ind < MAP_VIEW_STACK_TRACE_SIZE; ind++) {
                    if (View.trace[ind] == 0) {
                        break;
                    }

                    GetSym((PVOID)View.trace[ind], symbol, &dwOffset);
                    if (*symbol) {
                        Print("    %s", symbol);
                        if (dwOffset) {
                            Print("+%x\n", dwOffset);
                        }
                    }
                }
                Print("\n");
#endif // MAP_VIEW_STACK_TRACE
            }

            pView = View.pNext;
        }

        pSection = Section.pNext;
    }
    return TRUE;
    UNREFERENCED_PARAMETER(opts);
    UNREFERENCED_PARAMETER(param1);
}

BOOL Idfa(
DWORD opts,
PVOID param1)
{
#if DBG
    DWORD_PTR  dwOffset;
    CHAR       symbol[160];
    int        ind;
    PVOID*     pTrace;
    PVOID      trace;
    DWORD      dwAllocFailIndex;
    DWORD*     pdwAllocFailIndex;
    PEPROCESS  pep;
    PEPROCESS* ppep;
    PETHREAD   pet;
    PETHREAD*  ppet;

    if (EvalExp(VAR(gdwAllocFailIndex)) == NULL) {
        Print("!dfa is available only in debug versions of win32k.sys\n");
        return FALSE;
    }

    moveExp(&pdwAllocFailIndex, VAR(gdwAllocFailIndex));
    if (!tryMove(dwAllocFailIndex, pdwAllocFailIndex)) {
        Print("dfa failure");
        return FALSE;
    }

    moveExp(&ppep, VAR(gpepRecorded));
    if (!tryMove(pep, ppep)) {
        Print("dfa failure");
        return FALSE;
    }
    moveExp(&ppet, VAR(gpetRecorded));
    if (!tryMove(pet, ppet)) {
        Print("dfa failure");
        return FALSE;
    }

    Print("Fail allocation index %d 0x%04x\n", dwAllocFailIndex, dwAllocFailIndex);
    Print("pEProcess %#p pEThread %#p\n\n", pep, pet);

    moveExp(&pTrace, VAR(gRecordedStackTrace));

    for (ind = 0; ind < 12; ind++) {

        if (!tryMove(trace, pTrace)) {
            Print("dfa failure");
            return FALSE;
        }

        if (trace == 0) {
            break;
        }

        GetSym((PVOID)trace, symbol, &dwOffset);
        if (*symbol) {
            Print("\t%s", symbol);
            if (dwOffset) {
                Print("+%x\n", dwOffset);
            }
        }

        pTrace++;
    }
    Print("\n");
#endif // DBG
    return TRUE;
    UNREFERENCED_PARAMETER(opts);
    UNREFERENCED_PARAMETER(param1);
}

/***************************************************************************\
* dpa   - dump pool allocations
*
* Dump pool allocations
*
* 12-27-96 CLupu  Created
\***************************************************************************/


BOOL Idpa(
DWORD opts,
PVOID param1)
{
    PWin32AllocStats pAllocList;
    Win32AllocStats  AllocList;
    DWORD*           pdwPoolFlags;
    DWORD            dwPoolFlags;
    BOOL             bIncludeStackTrace = FALSE;

    moveExp(&pdwPoolFlags, VAR(gdwPoolFlags));
    if (!tryMove(dwPoolFlags, pdwPoolFlags)) {
        Print("Couldn't evaluate win32k!gdwPoolFlags. The symbols may be wrong !\n");
        return FALSE;
    }

    if (!(dwPoolFlags & POOL_HEAVY_ALLOCS)) {
        Print("win32k.sys doesn't have pool instrumentation !\n");
        return FALSE;
    }

    if (opts & OFLAG(s)) {
        if (dwPoolFlags & POOL_CAPTURE_STACK) {
            bIncludeStackTrace = TRUE;
        } else {
            Print("win32k.sys doesn't have stack traces enabled for pool allocations\n");
        }
    }

    moveExp(&pAllocList, VAR(gAllocList));
    if (!tryMove(AllocList, pAllocList)) {
        Print("Could not get Win32AllocStats structure win32k!gAllocList !\n");
        return FALSE;
    }

    /*
     * Handle !dpa -c
     */
    if (opts & OFLAG(c)) {

        Print("- pool instrumentation enabled for win32k.sys\n");
        if (dwPoolFlags & POOL_CAPTURE_STACK) {
            Print("- stack traces enabled for pool allocations\n");
        } else {
            Print("- stack traces disabled for pool allocations\n");
        }

        if (dwPoolFlags & POOL_KEEP_FAIL_RECORD) {
            Print("- records of failed allocations enabled\n");
        } else {
            Print("- records of failed allocations disabled\n");
        }

        if (dwPoolFlags & POOL_KEEP_FREE_RECORD) {
            Print("- records of free pool enabled\n");
        } else {
            Print("- records of free pool disabled\n");
        }

        Print("\n");

        Print("    CrtM         CrtA         MaxM         MaxA       Head\n");
        Print("------------|------------|------------|------------|------------|\n");
        Print(" 0x%08x   0x%08x   0x%08x   0x%08x   0x%08x\n",
              AllocList.dwCrtMem,
              AllocList.dwCrtAlloc,
              AllocList.dwMaxMem,
              AllocList.dwMaxAlloc,
              AllocList.pHead);

        return TRUE;
    }

    if (opts & OFLAG(f)) {

        DWORD        dwFailRecordCrtIndex;
        DWORD*       pdwFailRecordCrtIndex;
        DWORD        dwFailRecordTotalFailures;
        DWORD*       pdwFailRecordTotalFailures;
        DWORD        dwFailRecords;
        DWORD*       pdwFailRecords;
        DWORD        Ind, dwFailuresToDump;
        PPOOLRECORD* ppFailRecord;
        PPOOLRECORD  pFailRecord;
        PPOOLRECORD  pFailRecordOrg;

        if (!(dwPoolFlags & POOL_KEEP_FAIL_RECORD)) {
            Print("win32k.sys doesn't have records of failed allocations !\n");
            return TRUE;
        }

        moveExp(&pdwFailRecordTotalFailures, VAR(gdwFailRecordTotalFailures));
        if (!tryMove(dwFailRecordTotalFailures, pdwFailRecordTotalFailures)) {
            Print("Could not get win32k!gdwFailRecordTotalFailures !\n");
            return FALSE;
        }

        if (dwFailRecordTotalFailures == 0) {
            Print("No allocation failure in win32k.sys !\n");
            return TRUE;
        }

        moveExp(&pdwFailRecordCrtIndex, VAR(gdwFailRecordCrtIndex));
        if (!tryMove(dwFailRecordCrtIndex, pdwFailRecordCrtIndex)) {
            Print("Could not get win32k!gdwFailRecordCrtIndex !\n");
            return FALSE;
        }

        moveExp(&pdwFailRecords, VAR(gdwFailRecords));
        if (!tryMove(dwFailRecords, pdwFailRecords)) {
            Print("Could not get win32k!gdwFailRecords !\n");
            return FALSE;
        }

        if (dwFailRecordTotalFailures < dwFailRecords) {
            dwFailuresToDump = dwFailRecordTotalFailures;
        } else {
            dwFailuresToDump = dwFailRecords;
        }

        moveExp(&ppFailRecord, VAR(gparrFailRecord));

        if (!tryMove(pFailRecord, ppFailRecord)) {
            Print("\nCould not move from %#p !\n\n", ppFailRecord);
            return FALSE;
        }

        pFailRecordOrg = pFailRecord;

        Print("\nFailures to dump : %d\n\n", dwFailuresToDump);

        for (Ind = 0; Ind < dwFailuresToDump; Ind++) {

            POOLRECORD FailRecord;
            DWORD      tag[2] = {0, 0};

            if (dwFailRecordCrtIndex == 0) {
                dwFailRecordCrtIndex = dwFailRecords - 1;
            } else {
                dwFailRecordCrtIndex--;
            }

            pFailRecord = pFailRecordOrg + dwFailRecordCrtIndex;

            /*
             * Dump
             */
            if (!tryMove(FailRecord, pFailRecord)) {
                Print("\nCould not move from %#p !\n\n", pFailRecord);
                break;
            }

            tag[0] = PtrToUlong(FailRecord.ExtraData);

            Print("Allocation for tag '%s' size 0x%x failed\n",
                  &tag,
                  FailRecord.size);

            PrintStackTrace(FailRecord.trace, RECORD_STACK_TRACE_SIZE);
        }
    }
    if (opts & OFLAG(r)) {

        DWORD        dwFreeRecordCrtIndex;
        DWORD*       pdwFreeRecordCrtIndex;
        DWORD        dwFreeRecordTotalFrees;
        DWORD*       pdwFreeRecordTotalFrees;
        DWORD        dwFreeRecords;
        DWORD*       pdwFreeRecords;
        DWORD        Ind, dwFreesToDump;
        PPOOLRECORD* ppFreeRecord;
        PPOOLRECORD  pFreeRecord;
        PPOOLRECORD  pFreeRecordOrg;

        if (!(dwPoolFlags & POOL_KEEP_FREE_RECORD)) {
            Print("win32k.sys doesn't have records of free pool !\n");
            return FALSE;
        }

        moveExp(&pdwFreeRecordTotalFrees, VAR(gdwFreeRecordTotalFrees));
        if (!tryMove(dwFreeRecordTotalFrees, pdwFreeRecordTotalFrees)) {
            Print("Could not get win32k!gdwFreeRecordTotalFrees !\n");
            return FALSE;
        }

        if (dwFreeRecordTotalFrees == 0) {
            Print("No free pool in win32k.sys !\n");
            return FALSE;
        }

        moveExp(&pdwFreeRecordCrtIndex, VAR(gdwFreeRecordCrtIndex));
        if (!tryMove(dwFreeRecordCrtIndex, pdwFreeRecordCrtIndex)) {
            Print("Could not get win32k!gdwFreeRecordCrtIndex !\n");
            return FALSE;
        }

        moveExp(&pdwFreeRecords, VAR(gdwFreeRecords));
        if (!tryMove(dwFreeRecords, pdwFreeRecords)) {
            Print("Could not get win32k!gdwFreeRecords !\n");
            return FALSE;
        }

        if (dwFreeRecordTotalFrees < dwFreeRecords) {
            dwFreesToDump = dwFreeRecordTotalFrees;
        } else {
            dwFreesToDump = dwFreeRecords;
        }

        moveExp(&ppFreeRecord, VAR(gparrFreeRecord));

        if (!tryMove(pFreeRecord, ppFreeRecord)) {
            Print("\nCould not move from %#p !\n\n", ppFreeRecord);
            return FALSE;
        }

        pFreeRecordOrg = pFreeRecord;

        Print("\nFrees to dump : %d\n\n", dwFreesToDump);

        for (Ind = 0; Ind < dwFreesToDump; Ind++) {

            POOLRECORD FreeRecord;

            if (dwFreeRecordCrtIndex == 0) {
                dwFreeRecordCrtIndex = dwFreeRecords - 1;
            } else {
                dwFreeRecordCrtIndex--;
            }

            pFreeRecord = pFreeRecordOrg + dwFreeRecordCrtIndex;

            /*
             * Dump
             */
            if (!tryMove(FreeRecord, pFreeRecord)) {
                Print("\nCould not move from %#p !\n\n", pFreeRecord);
                break;
            }

            Print("Free pool for p %#p size 0x%x\n",
                  FreeRecord.ExtraData,
                  FreeRecord.size);

            PrintStackTrace(FreeRecord.trace, RECORD_STACK_TRACE_SIZE);
        }
    }

    if (opts & OFLAG(v)) {

        PWin32PoolHead ph;
        Win32PoolHead  h;


        ph = AllocList.pHead;

        while (ph != NULL) {

            if (!tryMove(h, ph)) {
                Print("\nCould not move from %#p !\n\n", ph);
                break;
            }

            Print("p %#p pHead %#p size %x\n",
                  ph + 1, ph, h.size);

            if (bIncludeStackTrace) {

                DWORD_PTR dwOffset;
                CHAR      symbol[160];
                int       ind;
                PVOID     trace;
                PVOID*    pTrace;

                pTrace = h.pTrace;

                for (ind = 0; ind < POOL_ALLOC_TRACE_SIZE; ind++) {

                    if (!tryMove(trace, pTrace)) {
                        Print("dpa failure\n");
                        return FALSE;
                    }

                    if (trace == 0) {
                        break;
                    }

                    GetSym((PVOID)trace, symbol, &dwOffset);
                    if (*symbol) {
                        Print("\t%s", symbol);
                        if (dwOffset) {
                            Print("+%x\n", dwOffset);
                        }
                    }

                    pTrace++;
                }
                Print("\n");
            }

            ph = h.pNext;
        }
        return TRUE;
    }

    if (opts & OFLAG(p)) {

        PWin32PoolHead ph;
        Win32PoolHead  h;
        BYTE*          p;

        if (param1 == 0) {
            return TRUE;
        }

        p = (BYTE*)param1;

        ph = AllocList.pHead;

        while (ph != NULL) {

            if (!tryMove(h, ph)) {
                Print("\nCould not move from %#p !\n\n", ph);
                break;
            }

            if (p >= (BYTE*)ph && p <= (BYTE*)ph + h.size + sizeof(Win32PoolHead)) {

                PVOID Trace[RECORD_STACK_TRACE_SIZE];

                Print("p %#p pHead %#p size %x\n",
                      ph + 1, ph, h.size);

                moveBlock(Trace, h.pTrace, sizeof(PVOID) * RECORD_STACK_TRACE_SIZE);


                PrintStackTrace(Trace, RECORD_STACK_TRACE_SIZE);
                return TRUE;
            }

            ph = h.pNext;
        }
        return TRUE;
    }

    return TRUE;
}
#endif // KERNEL



/************************************************************************\
* Procedure: Ifno
*
* Description: Find Nearest Objects - helps in figureing out references
*   to freed objects or stale pointers.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Ifno(
    DWORD opts,
    PVOID param1)
{
    HANDLEENTRY he, heBest, heAfter, *phe;
    DWORD i;
    DWORD hBest, hAfter;
    DWORD_PTR dw;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        Print("Expected an address.\n");
        return FALSE;
    }

    dw = (DWORD_PTR)FIXKP(param1);
    heBest.phead = NULL;
    heAfter.phead = (PVOID)-1;

    if (dw != (DWORD_PTR)param1) {
        /*
         * no fixups needed - he's looking the kernel address range.
         */
        FOREACHHANDLEENTRY(phe, he, i)
            if ((DWORD_PTR)he.phead <= dw &&
                    heBest.phead < he.phead &&
                    he.bType != TYPE_FREE) {
                heBest = he;
                hBest = i;
            }
            if ((DWORD_PTR)he.phead > dw &&
                    heAfter.phead > he.phead &&
                    he.bType != TYPE_FREE) {
                heAfter = he;
                hAfter = i;
            }
        NEXTEACHHANDLEENTRY()

        if (heBest.phead != NULL) {
            Print("Nearest guy before %#p is a %s object located at %#p (i=%x).\n",
                    dw, aszTypeNames[heBest.bType], heBest.phead, hBest);
        }
        if (heAfter.phead != (PVOID)-1) {
            Print("Nearest guy after %#p is a %s object located at %#p. (i=%x)\n",
                    dw, aszTypeNames[heAfter.bType], heAfter.phead, hAfter);
        }
    } else {
        /*
         * fixups are needed.
         */
        FOREACHHANDLEENTRY(phe, he, i)
            if ((DWORD_PTR)FIXKP(he.phead) <= dw &&
                    heBest.phead < he.phead &&
                    he.bType != TYPE_FREE) {
                heBest = he;
                hBest = i;
            }
            if ((DWORD_PTR)FIXKP(he.phead) > dw &&
                    heAfter.phead > he.phead &&
                    he.bType != TYPE_FREE) {
                heAfter = he;
                hAfter = i;
            }
        NEXTEACHHANDLEENTRY()

        if (heBest.phead != NULL) {
            Print("Nearest guy before %#p is a %s object located at %#p (i=%x).\n",
                    dw, aszTypeNames[heBest.bType], FIXKP(heBest.phead), hBest);
        }
        if (heAfter.phead != (PVOID)-1) {
            Print("Nearest guy after %#p is a %s object located at %#p. (i=%x)\n",
                    dw, aszTypeNames[heAfter.bType], FIXKP(heAfter.phead), hAfter);
        }
    }
    return TRUE;
}




/************************************************************************\
* Procedure: Ifrr
*
* Description: Finds Range References - helpfull for finding stale
*   pointers.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Ifrr(
    DWORD opts,
    PVOID param1,
    PVOID param2,
    PVOID param3,
    PVOID param4)
{
    DWORD_PTR pSrc1 = (DWORD_PTR)param1;
    DWORD_PTR pSrc2 = (DWORD_PTR)param2;
    DWORD_PTR pRef1 = (DWORD_PTR)param3;
    DWORD_PTR pRef2 = (DWORD_PTR)param4;
    DWORD_PTR dw;
    DWORD_PTR buffer[PAGE_SIZE / sizeof(DWORD_PTR)];

    UNREFERENCED_PARAMETER(opts);

    if (pSrc2 < pSrc1) {
        Print("Source range improper.  Values reversed.\n");
        dw = pSrc1;
        pSrc1 = pSrc2;
        pSrc2 = dw;
    }
    if (pRef2 == 0) {
        pRef2 = pRef1;
    }
    if (pRef2 < pRef1) {
        Print("Reference range improper.  Values reversed.\n");
        dw = pRef1;
        pRef1 = pRef2;
        pRef2 = dw;
    }

    pSrc1 &= MAXULONG_PTR - PAGE_SIZE + 1;  // PAGE aligned
    pSrc2 = (pSrc2 + (sizeof(DWORD_PTR)-1)) & (MAXULONG_PTR - (sizeof(DWORD_PTR)-1));   // dword_ptr aligned

    Print("Searching range (%#p-%#p) for references to (%#p-%#p)...",
            pSrc1, pSrc2, pRef1, pRef2);

    for (; pSrc1 < pSrc2; pSrc1 += PAGE_SIZE) {
        BOOL fSuccess;

        if (!(pSrc1 & 0xFFFFFF)) {
            Print("\nSearching %#p...", pSrc1);
        }
        fSuccess = tryMoveBlock(buffer, (PVOID)pSrc1, sizeof(buffer));
        if (!fSuccess) {
            /*
             * Skip to next page
             */
        } else {
            for (dw = 0; dw < ARRAY_SIZE(buffer); dw++) {
                if (buffer[dw] >= pRef1 && buffer[dw] <= pRef2) {
                    Print("\n[%#p] = %#p ",
                            pSrc1 + dw * sizeof(DWORD_PTR),
                            buffer[dw]);
                }
            }
        }
        if (IsCtrlCHit()) {
            Print("\nSearch aborted.\n");
            return TRUE;
        }
    }
    Print("\nSearch complete.\n");
    return TRUE;
}


#ifdef KERNEL
//PGDI_DEVICE undefined
#if 0
VOID ddGdiDevice(
PGDI_DEVICE pGdiDevice)
{
    Print("\t\tGDI_DEVICE\n");
    Print("\t\tcRefCount       = %d\n",     pGdiDevice->cRefCount);
    Print("\t\thDevInfo        = 0x%.8x\n", pGdiDevice->hDevInfo);
    Print("\t\thDev            = 0x%.8x\n", pGdiDevice->hDev);
    Print("\t\trcScreen        = (%d,%d)-(%d,%d) %dx%d\n",
            pGdiDevice->rcScreen.left, pGdiDevice->rcScreen.top,
            pGdiDevice->rcScreen.right, pGdiDevice->rcScreen.bottom,
            pGdiDevice->rcScreen.right - pGdiDevice->rcScreen.left,
            pGdiDevice->rcScreen.bottom - pGdiDevice->rcScreen.top);
    Print("\t\tDEVMODEW\n");
}
#endif
#endif


void
DumpMonitor(PMONITOR pMonitor, LPSTR pstrPrefix)
{
    Print("%shead.h             = 0x%.8x\n", pstrPrefix, pMonitor->head.h);
    Print("%shead.cLockObj      = 0x%.8x\n", pstrPrefix, pMonitor->head.cLockObj);
    Print("%spMonitorNext       = 0x%.8x\n", pstrPrefix, pMonitor->pMonitorNext);
    Print("%sdwMONFlags         = 0x%.8x %s\n", pstrPrefix, pMonitor->dwMONFlags, GetFlags(GF_MON, pMonitor->dwMONFlags, NULL, FALSE));

    Print("%srcMonitor          = (%d,%d)-(%d,%d) %dx%d\n",
            pstrPrefix,
            pMonitor->rcMonitor.left, pMonitor->rcMonitor.top,
            pMonitor->rcMonitor.right, pMonitor->rcMonitor.bottom,
            pMonitor->rcMonitor.right - pMonitor->rcMonitor.left,
            pMonitor->rcMonitor.bottom - pMonitor->rcMonitor.top);

    Print("%srcWork             = (%d,%d)-(%d,%d) %dx%d\n",
            pstrPrefix,
            pMonitor->rcWork.left, pMonitor->rcWork.top,
            pMonitor->rcWork.right, pMonitor->rcWork.bottom,
            pMonitor->rcWork.right - pMonitor->rcWork.left,
            pMonitor->rcWork.bottom - pMonitor->rcWork.top);

    Print("%shrgnMonitor        = 0x%.8x\n", pstrPrefix, pMonitor->hrgnMonitor);
    Print("%scFullScreen        = %d\n",     pstrPrefix, pMonitor->cFullScreen);
    Print("%scwndStack          = %d\n",     pstrPrefix, pMonitor->cWndStack);

#if 0
    Print("%spMonGdiDevice      = 0x%.8x\n", pstrPrefix, pMonitor->pMonGdiDevice);

    if (pMonitor->pMonGdiDevice) {
        move(gdiDevice, pMonitor->pMonGdiDevice);
        ddGdiDevice(&gdiDevice);
    } else {
        Print("\n\n");
        Print("ERROR - pMonGdiDevice missing\n");
        Print("\n\n");
    }
#endif
}



BOOL
Idmon(DWORD opts, PVOID param1)
{
    MONITOR         monitor;

    UNREFERENCED_PARAMETER(opts);

    if (!param1) {
        return FALSE;
    }

    Print("Dumping MONITOR at 0x%.8x\n", param1);
    move(monitor, (PMONITOR)param1);
    DumpMonitor(&monitor, "\t");

    return TRUE;
}

BOOL Idy(
DWORD opts,
PVOID param1)
{
    PDISPLAYINFO    pdi, gpdi;
    DISPLAYINFO     di;
    PMONITOR        pMonitor;
    MONITOR         monitor;
    PSERVERINFO     psi;
    SERVERINFO      si;
    int             i;
    PSHAREDINFO     pshi;

#if 0
    GDI_DEVICE      gdiDevice;
#endif

    UNREFERENCED_PARAMETER(opts);

    moveExp(&pshi, VAR(gSharedInfo));
    move(gpdi, &pshi->pDispInfo);

    if (param1) {
        pdi = (DISPLAYINFO *)param1;
    } else {
        pdi = gpdi;
    }

    move(di, pdi);

    moveExpValuePtr(&psi, VAR(gpsi));
    move(si, psi);

    Print("Dumping DISPLAYINFO at 0x%.8x\n", pdi);
    Print("\thDev                  = 0x%.8x\n", di.hDev);
    Print("\thdcScreen             = 0x%.8x\n", di.hdcScreen);

    Print("\thdcBits               = 0x%.8x\n", di.hdcBits);

    Print("\thdcGray               = 0x%.8x\n", di.hdcGray);
    Print("\thbmGray               = 0x%.8x\n", di.hbmGray);
    Print("\tcxGray                = %d\n",     di.cxGray);
    Print("\tcyGray                = %d\n",     di.cyGray);
    Print("\tpdceFirst             = 0x%.8x\n", di.pdceFirst);
    Print("\tpspbFirst             = 0x%.8x\n", di.pspbFirst);
    Print("\tcMonitors (visible)   = %d\n",     di.cMonitors);
    Print("\tpMonitorPrimary       = 0x%.8x\n", RebaseSharedPtr(di.pMonitorPrimary));
    Print("\tpMonitorFirst         = 0x%.8x\n", RebaseSharedPtr(di.pMonitorFirst));

    Print("\trcScreen              = (%d,%d)-(%d,%d) %dx%d\n",

                                     si.rcScreen.left, si.rcScreen.top,
                                     si.rcScreen.right, si.rcScreen.bottom,
                                     si.rcScreen.right - si.rcScreen.left,
                                     si.rcScreen.bottom - si.rcScreen.top);

    Print("\thrgnScreen            = 0x%.8x\n", di.hrgnScreen);
    Print("\tdmLogPixels           = %d\n",     di.dmLogPixels);
    Print("\tBitCountMax           = %d\n",     di.BitCountMax);
    Print("\tfDesktopIsRect        = %d\n",     di.fDesktopIsRect);

#if 0
    if (di.pDispGdiDevice) {
        move(gdiDevice, di.pDispGdiDevice);
        ddGdiDevice(&gdiDevice);
    } else {
        Print("\n\n");
        Print("ERROR - pDispGdiDevice missing\n");
        Print("\n\n");
    }
#endif

    Print("\n");

    if (pdi == gpdi) {
        if (si.dmLogPixels != di.dmLogPixels) {
            Print("\n\n");
            Print("ERROR - dmLogPixels doesn't match in gpsi (%d) and gpDispInfo (%d)\n",
                  si.dmLogPixels, di.dmLogPixels);
            Print("\n\n");
        }

#if 0
        if (di.pDispGdiDevice) {
            if (!EqualRect(&si.rcScreen, &gdiDevice.rcScreen)) {
                Print("\n\n");
                Print("ERROR - rcScreen doesn't match in gpsi (%d,%d)-(%d,%d) and gpDispInfo (%d,%d)-(%d,%d)\n",
                      si.rcScreen.left,  si.rcScreen.top,
                      si.rcScreen.right, si.rcScreen.bottom,
                      gdiDevice.rcScreen.left,  gdiDevice.rcScreen.top,
                      gdiDevice.rcScreen.right, gdiDevice.rcScreen.bottom);
                Print("\n\n");
            }
        }
#endif
    }


    for (   pMonitor = RebaseSharedPtr(di.pMonitorFirst), i = 1;
            pMonitor;
            pMonitor = RebaseSharedPtr(monitor.pMonitorNext), i++) {

        Print("\tMonitor %d, pMonitor = 0x%.8x\n", i, pMonitor);
        move(monitor, pMonitor);
        DumpMonitor(&monitor, "\t\t");
        Print("\n");
    }

    return TRUE;
}


#ifdef KERNEL
/***************************************************************************\
* kbd [queue]
*
* Loads a DLL containing more debugging extensions
*
* 10/27/92 IanJa        Created.
* 6/9/1995 SanfordS     made to fit stdexts motif
\***************************************************************************/
typedef struct {
    int iVK;
    LPSTR pszVK;
} VK, *PVK;

VK aVK[] = {
    { VK_SHIFT,    "SHIFT"    },
    { VK_LSHIFT,   "LSHIFT"   },
    { VK_RSHIFT,   "RSHIFT"   },
    { VK_CONTROL,  "CONTROL"  },
    { VK_LCONTROL, "LCONTROL" },
    { VK_RCONTROL, "RCONTROL" },
    { VK_MENU,     "MENU"     },
    { VK_LMENU,    "LMENU"    },
    { VK_RMENU,    "RMENU"    },
    { VK_NUMLOCK,  "NUMLOCK"  },
    { VK_CAPITAL,  "CAPITAL"  },
    { VK_LBUTTON,  "LBUTTON"  },
    { VK_MBUTTON,  "MBUTTON"  },
    { VK_RBUTTON,  "RBUTTON"  },
    { VK_XBUTTON1, "XBUTTON1" },
    { VK_XBUTTON2, "XBUTTON2" },
    { VK_RETURN ,  "ENTER"    },
    { VK_KANA,     "KANA/HANJA" },
    { VK_OEM_8,    "OEM_8"    },
    // { 0x52      ,  "R"        },  // your key goes here
    { 0,           NULL       }
};

BOOL Ikbd(
DWORD opts,
PVOID param1)
{
    PQ pq;
    Q q;
    PBYTE pb, pbr;
    int i;
    BYTE afUpdateKeyState[CBKEYSTATE + CBKEYSTATERECENTDOWN];
    PBYTE pgafAsyncKeyState;
    BYTE  afAsyncKeyState[CBKEYSTATE];
    PBYTE pgafRawKeyState;
    BYTE  afRawKeyState[CBKEYSTATE];
    UINT  PhysModifiers;

    if (opts & OFLAG(a)) {
        PTHREADINFO pti;

        opts &= ~OFLAG(a);
        FOREACHPTI(pti);
            move(pq, &pti->pq);
            Ikbd(opts, pq);
        NEXTEACHPTI(pti);
        return TRUE;
    }


    /*
     * If 'u' was specified, make sure there was also an address
     */
    if (opts & OFLAG(u)) {
        if (param1 == NULL) {
            Print("provide arg 2 of ProcessUpdateKeyEvent(), or WM_UPDATEKEYSTATE wParam\n");
            return FALSE;
        }
        pb = (PBYTE)param1;
        move(afUpdateKeyState, pb);
        pb = afUpdateKeyState;
        pbr = afUpdateKeyState + CBKEYSTATE;
        Print("Key State:   === NEW STATE ====   Asynchronous    Physical\n");

    } else {
        if (param1) {
            pq = (PQ)param1;
        } else {
            moveExpValuePtr(&pq, VAR(gpqForeground));
        }

        /*
         * Print out simple thread info for pq->ptiLock.
         */
        move(q, pq);
        if (q.ptiKeyboard) {
            Idt(OFLAG(p), q.ptiKeyboard);
        }

        pb = (PBYTE)&(q.afKeyState);
        pbr = (PBYTE)&(q.afKeyRecentDown);
        Print("Key State:   QUEUE %lx       Asynchronous    Raw\n", pq);
    }

    moveExp(&pgafAsyncKeyState, VAR(gafAsyncKeyState));
    tryMove(afAsyncKeyState, pgafAsyncKeyState);

    moveExp(&pgafRawKeyState, VAR(gafRawKeyState));
    tryMove(afRawKeyState, pgafRawKeyState);

    Print("             Down Toggle Recent   Down Toggle     Down Toggle\n");
    for (i = 0; aVK[i].pszVK != NULL; i++) {
        Print("VK_%s:\t%d    %d     %d        %d     %d         %d     %d\n",
            aVK[i].pszVK,
            TestKeyDownBit(pb, aVK[i].iVK) != 0,
            TestKeyToggleBit(pb, aVK[i].iVK) != 0,
            TestKeyRecentDownBit(pbr, aVK[i].iVK) != 0,
            TestKeyDownBit(afAsyncKeyState, aVK[i].iVK) != 0,
            TestKeyToggleBit(afAsyncKeyState, aVK[i].iVK) != 0,
            TestKeyDownBit(afRawKeyState, aVK[i].iVK) != 0,
            TestKeyToggleBit(afRawKeyState, aVK[i].iVK) != 0);
    }

    moveExpValuePtr(&PhysModifiers, VAR(gfsSASModifiersDown));
    Print("PhysModifiers = %x\n");
    return TRUE;
}
#endif // KERNEL





/************************************************************************\
* Procedure: Itest
*
* Description: Tests the basic stdexts macros and functions - a good check
*   on the debugger extensions in general before you waste time debuging
*   entensions.
*
* Returns: fSuccess
*
* 6/9/1995 Created SanfordS
*
\************************************************************************/
BOOL Itest()
{
    PVOID p;
    DWORD_PTR cch;
    CHAR ach[80];

    Print("Print test!\n");
    SAFEWHILE(TRUE) {
        Print("SAFEWHILE test...  Hit Ctrl-C NOW!\n");
    }
    p = EvalExp(VAR(gpsi));
    Print("EvalExp(%s) = %#p\n", VAR(gpsi), p);
    GetSym(p, ach, &cch);
    Print("GetSym(%#p) = %s\n", p, ach);
    if (IsWinDbg()) {
        Print("I think windbg is calling me.\n");
    } else {
        Print("I don't think windbg is calling me.\n");
    }
    Print("MoveBlock test...\n");
    moveBlock(&p, EvalExp(VAR(gpsi)), sizeof(PVOID));
    Print("MoveBlock(%#p) = %#p.\n", EvalExp(VAR(gpsi)), p);

    Print("moveExp test...\n");
    moveExp(&p, VAR(gpsi));
    Print("moveExp(%s) = %x.\n", VAR(gpsi), p);

    Print("moveExpValue test...\n");
    moveExpValuePtr(&p, VAR(gpsi));
    Print("moveExpValue(%s) = %#p.\n", VAR(gpsi), p);

    Print("Basic tests complete.\n");
    return TRUE;
}



/************************************************************************\
* Procedure: Iuver
*
* Description: Dumps versions of extensions and winsrv/win32k
*
* Returns: fSuccess
*
* 6/15/1995 Created SanfordS
*
\************************************************************************/
BOOL Iuver()
{
    PSERVERINFO psi;
    WORD        wSRVIFlags;

#if DBG
    Print("USEREXTS version: Checked.\n"
          "WIN32K.SYS version: ");
#else
    Print("USEREXTS version: Free.\n"
          "WIN32K.SYS version: ");
#endif

    moveExpValuePtr(&psi, VAR(gpsi));
    move(wSRVIFlags, &psi->wSRVIFlags);

    Print((wSRVIFlags & SRVIF_CHECKED) ? "Checked" : "Free");
    Print(".\n");
    return TRUE;
}

#define DUMPSTATUS(status) if (tryMoveExpValue(&Status, VAR(g ## status))) { \
                               Print("g%s = %lx\n", #status, Status);        \
                           }
#define DUMPTIME(time)     if (tryMoveExpValue(&Time, VAR(g ## time))) {     \
                               Print("g%s = %lx\n", #time, Time);            \
                           }


#ifdef KERNEL
/***************************************************************************\
* dinp - dump input diagnostics
* dinp -v   verbose
* dinp -i   show input records
*
* 04/13/98  IanJa       Created.
\***************************************************************************/
BOOL Idinp(
    DWORD opts,
    PVOID param1)
{
    DWORD    Time;
    NTSTATUS Status;
    PDEVICEINFO pDeviceInfo, *ppDeviceInfo;
    int i = 0;
    char ach[100];
    DWORD nKbd;
    BOOL bVerbose = FALSE;

    DUMPSTATUS(KbdIoctlLEDSStatus);

    DUMPTIME(MouseProcessMiceInputTime);
    DUMPTIME(MouseQueueMouseEventTime);
    DUMPTIME(MouseUnqueueMouseEventTime);

    if (opts & OFLAG(v)) {
        bVerbose = TRUE;
    }


    ppDeviceInfo = EvalExp(VAR(gpDeviceInfoList));
    while (tryMove(pDeviceInfo, ppDeviceInfo) && pDeviceInfo) {

        if (param1 && (param1 != pDeviceInfo)) {
            ; // skip it
        } else if (pDeviceInfo != 0) {
            DEVICEINFO DeviceInfo;
            WCHAR awchBuffer[100];
            DWORD cbBuffer;

            Print("#%d: %x ", i, pDeviceInfo);
            if (tryMove(DeviceInfo, pDeviceInfo)) {
                if (DeviceInfo.type == DEVICE_TYPE_MOUSE) {
                    Print("MOU", i);
                } else if (DeviceInfo.type == DEVICE_TYPE_KEYBOARD) {
                    Print("KBD");
                } else {
                    Print("%2d?", DeviceInfo.type);
                }
                if (DeviceInfo.usActions) {
                    Print(" Pending action %x %s", DeviceInfo.usActions,
                            GetFlags(GF_DIAF, DeviceInfo.usActions, NULL, TRUE));
                }
                cbBuffer = min(DeviceInfo.ustrName.Length, sizeof(awchBuffer)-sizeof(WCHAR));
                if (tryMoveBlock(awchBuffer, DeviceInfo.ustrName.Buffer, cbBuffer)) {
                    awchBuffer[cbBuffer / sizeof(WCHAR)] = 0;
                    Print("\n    %ws\n", awchBuffer);
                } else {
                    Print("\n");
                }
            } else {
                DeviceInfo.type = 0xFF;
            }
            if (bVerbose || (param1 == pDeviceInfo)) {
                sprintf(ach, "GENERIC_DEVICE_INFO 0x%p", pDeviceInfo);
                gnIndent += 2;
                Idso(0, ach);
                gnIndent += 2;
                sprintf(ach, "IO_STATUS_BLOCK 0x%p",
                        (PBYTE)pDeviceInfo + FIELD_OFFSET(DEVICEINFO, iosb));
                Idso(0, ach);
                gnIndent -= 2;
                if (DeviceInfo.type == DEVICE_TYPE_MOUSE) {
                    sprintf(ach, "MOUSE_DEVICE_INFO 0x%p",
                            (PBYTE)pDeviceInfo + FIELD_OFFSET(DEVICEINFO, mouse));
                    Idso(0, ach);

                    gnIndent += 2;
                    sprintf(ach, "MOUSE_ATTRIBUTES 0x%p",
                            (PBYTE)pDeviceInfo + FIELD_OFFSET(DEVICEINFO, mouse)
                                    + FIELD_OFFSET(MOUSE_DEVICE_INFO, Attr));
                    Idso(0, ach);
                    gnIndent -= 2;
                } else if (DeviceInfo.type == DEVICE_TYPE_KEYBOARD) {
                    sprintf(ach, "KEYBOARD_DEVICE_INFO 0x%p",
                            (PBYTE)pDeviceInfo + FIELD_OFFSET(DEVICEINFO, keyboard));
                    Idso(0, ach);
                    gnIndent += 2;
                    sprintf(ach, "KEYBOARD_ATTRIBUTES 0x%p",
                            (PBYTE)pDeviceInfo + FIELD_OFFSET(DEVICEINFO, keyboard)
                                    + FIELD_OFFSET(KEYBOARD_DEVICE_INFO, Attr));
                    Idso(0, ach);
                    gnIndent -= 2;
                } else {
                    Print("Unknown device type %d\n", DeviceInfo.type);
                }
                if (opts & OFLAG(i)) {
                    Print("  Input Records:");
                    if (DeviceInfo.iosb.Information == 0) {
                        Print(" NONE\n");
                    } else {
                        Print("\n");
                        gnIndent += 2;
                        sprintf(ach, "KEYBOARD_INPUT_DATA %p *%x",
                                &(pDeviceInfo->keyboard.Data[0]),
                                DeviceInfo.iosb.Information / sizeof(DeviceInfo.keyboard.Data[0]));
                        Idso(0, ach);
                        gnIndent -= 2;
                    }
                }
                gnIndent -= 2;
            } // bVerbose
        }
        ppDeviceInfo = FIXKP(&pDeviceInfo->pNext);
        i++;
    }

    // Now display input related sytem metrics
    {
        SERVERINFO si;
        PSERVERINFO psi;

// #define SMENTRY(sm) {SM_##sm, #sm}  (see above

        /*
         * Add mouse- and keyboard- related entries to this table
         * with the prefix removed, in whatever order you think is rational
         */
        static SYSMET_ENTRY aSysMet[] = {
            SMENTRY(MOUSEPRESENT),
            SMENTRY(MOUSEWHEELPRESENT),
            SMENTRY(CMOUSEBUTTONS),
        };

        moveExpValuePtr(&psi, VAR(gpsi));
        move(si, psi);

        for (i = 0; i < NELEM(aSysMet); i++) {
            Print(  "SM_%-18s = 0x%08lx = %d\n",
                    aSysMet[i].pstrMetric,
                    si.aiSysMet[aSysMet[i].iMetric],
                    si.aiSysMet[aSysMet[i].iMetric]);
        }

        moveExpValuePtr(&nKbd, VAR(gnKeyboards));
        Print("gnKeyboards = %d\n", nKbd);
        moveExpValuePtr(&nKbd, VAR(gnMice));
        Print("gnMice = %d\n", nKbd);
    }
    return TRUE;
}

/***************************************************************************\
* hh - dump the flags in gdwHydraHint
*
* 05/20/98  MCostea       Created.
\***************************************************************************/
BOOL Ihh(
    DWORD opts,
    PVOID param1)
{
    DWORD dwHHint, *pdwHH;
    ULONG ulSessionId, *pulASessionId;
    int i, maxFlags;

    char * aHHstrings[] = {
        "HH_DRIVERENTRY            0x00000001",
        "HH_USERINITIALIZE         0x00000002",
        "HH_INITVIDEO              0x00000004",
        "HH_REMOTECONNECT          0x00000008",
        "HH_REMOTEDISCONNECT       0x00000010",
        "HH_REMOTERECONNECT        0x00000020",
        "HH_REMOTELOGOFF           0x00000040",
        "HH_DRIVERUNLOAD           0x00000080",
        "HH_GRECLEANUP             0x00000100",
        "HH_USERKCLEANUP           0x00000200",
        "HH_INITIATEWIN32KCLEANUP  0x00000400",
        "HH_ALLDTGONE              0x00000800",
        "HH_RITGONE                0x00001000",
        "HH_RITCREATED             0x00002000",
        "HH_LOADCURSORS            0x00004000",
        "HH_KBDLYOUTGLOBALCLEANUP  0x00008000",
        "HH_KBDLYOUTFREEWINSTA     0x00010000",
        "HH_CLEANUPRESOURCES       0x00020000",
        "HH_DISCONNECTDESKTOP      0x00040000"
    };

    if (param1) {
        dwHHint = (DWORD)((DWORD_PTR)param1);
        Print("gdwHydraHint is 0x%x:\n", dwHHint);
    } else {
        pdwHH = EvalExp(VAR(gdwHydraHint));
        if(!tryMove(dwHHint, pdwHH)) {
            Print("Can't get value of gdwHydraHint");
            return FALSE;
        }

        pulASessionId = EvalExp(VAR(gSessionId));
        if (!tryMove(ulSessionId, pulASessionId)) {
            Print("Can't get value of gSessionId");
            return FALSE;
        }

        Print("Session 0x%x \n  gdwHydraHint is 0x%x:\n", ulSessionId, dwHHint);
    }

    i = 0;
    maxFlags = sizeof(aHHstrings)/sizeof(aHHstrings[0]);

    while (dwHHint) {

        if (dwHHint & 0x01) {

            if (i >= maxFlags) {
                Print("\n Error: Unknown flags: userkdx.dll might be outdated\n");
                return TRUE;
            }
            Print("    %s\n", aHHstrings[i]);
        }
        i++;
        dwHHint >>= 1;
    }

    return TRUE;
    UNREFERENCED_PARAMETER(opts);
}

#endif // KERNEL

/************************************************************************\
* Procedure: Idupm
*
* 04/29/98 GerardoB     Created
*
\************************************************************************/
#ifdef KERNEL
BOOL Idupm (void)
{
    char ach[80];
    DWORD dwMask;
    int i;
    WORD w = GF_UPM0;

    Print("UserPreferencesMask:\n");
    for (i = 0; i < SPI_BOOLMASKDWORDSIZE; i++) {
        sprintf(ach, "win32k!gpdwCPUserPreferencesMask + %#lx", i * sizeof(DWORD));
        moveExpValue(&dwMask, ach);
        w = GF_UPM0 + i;
        Print("Offset: %d - %#lx: %s\n",
              i, dwMask, GetFlags(w, dwMask, NULL, TRUE));
    }
    return TRUE;
}
#endif // KERNEL
/************************************************************************\
* Procedure: Idup
*
* 04/29/98 GerardoB     Created header
*
\************************************************************************/
#ifdef KERNEL
BOOL Idup (void)
{
    Print("Bummer, not implemented yet!");
    return TRUE;
}
#endif // KERNEL


/************************************************************************\
* Procedure: Idimc
*
* HiroYama     Created
*
\************************************************************************/

static struct /*NoName*/ {
    const char* terse;
    const char* verbose;
} gaIMCAttr[] = {
    "IN", "INPUT",
    "TC", "TARGET_CONVERTED",
    "CV", "CONVERTED",
    "TN", "TARGET_NOTCONVERTED",
    "IE", "INPUT_ERROR",
    "FC", "FIXEDCONVERTED",
};

const char* GetInxAttr(BYTE bAttr)
{
    if (bAttr < ARRAY_SIZE(gaIMCAttr)) {
        return gaIMCAttr[bAttr].terse;
    }
    return "**";
}

void _PrintInxAttr(const char* title, PVOID pCompStr, DWORD offset, DWORD len)
{
    DWORD i;
    PBYTE pAttr = (PBYTE)pCompStr + offset;

    if (title == NULL) {
        // Print a legend
        Print("  ");
        for (i = 0; i < ARRAY_SIZE(gaIMCAttr); ++i) {
            if (i && i % 4 == 0) {
                Print("\n");
                Print("  ");
            }
            Print("%s:%s ", gaIMCAttr[i].terse, gaIMCAttr[i].verbose);
        }
        Print("\n");
        return;
    }

    if (offset == 0 || len == 0) {
        return;
    }

    Print("  %-12s (@%p) off:0x%x len:0x%x (in byte)\n",
          title, pAttr, offset, len);
    Print("   ");
    for (i = 0; i < len; ++i) {
        BYTE bAttr;

        move(bAttr, pAttr + i);
        Print("|%s", GetInxAttr(bAttr));
    }
    Print("|\n");
}

#define PrintInxAttr(name) \
    _PrintInxAttr(#name, pCompStr, CompStr.dw ## name ## Offset, CompStr.dw ## name ## Len)

void _PrintInxClause(const char* title, PVOID pCompStr, DWORD offset, DWORD len)
{
    PDWORD pClause = (PDWORD)((PBYTE)pCompStr + offset);
    DWORD i;

    if (offset == 0 || len == 0) {
        return;
    }

    Print("  %-12s (@%p) off:0x%x len:0x%x (0x%x dwords)\n",
          title, pClause, offset, len, len / sizeof(DWORD));

    Print("   ");
    len /= sizeof(DWORD);
    for (i = 0; i < len; ++i) {
        DWORD dwData;

        move(dwData,  pClause + i);
        Print("|0x%x", dwData);
    }
    Print("|\n");
}

#define PrintInxClause(name) \
    _PrintInxClause(#name, pCompStr, CompStr.dw ## name ## Offset, CompStr.dw ## name ## Len)

const char* GetInxStr(WCHAR wchar, BOOLEAN fUnicode)
{
    static char ach[32];

    if (wchar >= 0x20 && wchar <= 0x7e) {
        sprintf(ach, "'%c'", wchar);
    }
    else if (fUnicode) {
        sprintf(ach, "U+%04x", wchar);
    }
    else {
        sprintf(ach, "%02x", (BYTE)wchar);
    }

    return ach;
}

void _PrintInxStr(const char* title, PVOID pCompStr, DWORD offset, DWORD len, BOOLEAN fUnicode)
{
    DWORD i;

    if (offset == 0 || len == 0) {
        return;
    }

    Print("  %-12s (@%p) off:0x%x len:0x%x (0x%x cch)\n",
        title, (PBYTE)pCompStr + offset, offset, len, len / (fUnicode + 1));

    Print("   ");
    for (i = 0; i < len; ++i) {
        WCHAR wchar;
        if (fUnicode) {
            move(wchar, (PWCHAR)((PBYTE)pCompStr + offset) + i);
        }
        else {
            BYTE bchar;

            move(bchar, (PBYTE)pCompStr + offset + i);
            wchar = bchar;
        }
        Print("|%s", GetInxStr(wchar, fUnicode));
    }
    Print("|\n");
}

#define PrintInxStr(name) \
    _PrintInxStr(#name, pCompStr, CompStr.dw ## name ## Offset, CompStr.dw ## name ## Len, fUnicode)


#define PrintInxElementA(name) \
    do { \
        PrintInxAttr(name ## Attr); \
        PrintInxClause(name ## Clause); \
        PrintInxStr(name ## Str); \
    } while (0)

#define PrintInxElementB(name) \
    do { \
        PrintInxClause(name ## Clause); \
        PrintInxStr(name ## Str); \
    } while (0)


void _PrintInxFriendlyStr(
    const char* title,
    PBYTE pCompStr,
    DWORD dwAttrOffset,
    DWORD dwAttrLen,
    DWORD dwClauseOffset,
    DWORD dwClauseLen,
    DWORD dwStrOffset,
    DWORD dwStrLen,
    BOOLEAN fUnicode)
{
    DWORD i;
    DWORD n;
    DWORD dwClause;

    Print("  %-11s", title);
    if (dwStrOffset == 0 || dwStrLen == 0) {
        Print("\n");
        return;
    }

    for (i = 0, n = 0; i < dwStrLen; ++i) {
        BYTE bAttr;
        WCHAR wchar;

        move(dwClause, (PDWORD)(pCompStr + dwClauseOffset) + n);
        if (dwClause == i) {
            ++n;
            if (i) {
                Print("| ");
            }
        }

        if (fUnicode) {
            move(wchar, (PWCHAR)((PBYTE)pCompStr + dwStrOffset) + i);
        }
        else {
            BYTE bchar;
            move(bchar, (PBYTE)pCompStr + dwStrOffset + i);
            wchar = bchar;
        }

        if (dwAttrOffset != ~0) {
            move(bAttr, pCompStr + dwAttrOffset + i);
            Print("|%s:%s", GetInxAttr(bAttr), GetInxStr(wchar, fUnicode));
        }
        else {
            Print("|%s", GetInxStr(wchar, fUnicode));
        }
    }
    Print("|\n");
    if (dwClauseLen / sizeof(DWORD) != (n + 1)) {
        Print("  ** dwClauseLen (0x%x) doesn't match to n (0x%x) **\n", dwClauseLen, (n + 1) * sizeof(DWORD));
    }
    if (dwAttrOffset != ~0 && dwAttrLen != dwStrLen) {
        Print("  ** dwAttrLen (0x%x) doesn't match to dwStrLen (0x%x) **\n", dwAttrLen, dwStrLen);
    }
}

#define PrintInxFriendlyStrA(name) \
    _PrintInxFriendlyStr(#name, \
                         (PBYTE)pCompStr, \
                         CompStr.dw ## name ## AttrOffset, \
                         CompStr.dw ## name ## AttrLen, \
                         CompStr.dw ## name ## ClauseOffset, \
                         CompStr.dw ## name ## ClauseLen, \
                         CompStr.dw ## name ## StrOffset, \
                         CompStr.dw ## name ## StrLen, \
                         fUnicode)

#define PrintInxFriendlyStrB(name) \
    _PrintInxFriendlyStr(#name, \
                         (PBYTE)pCompStr, \
                         ~0, \
                         0, \
                         CompStr.dw ## name ## ClauseOffset, \
                         CompStr.dw ## name ## ClauseLen, \
                         CompStr.dw ## name ## StrOffset, \
                         CompStr.dw ## name ## StrLen, \
                         fUnicode)



BOOL Idimc(DWORD opts, PVOID param1)
{
    IMC imc;
    PIMC pImc;
    PCLIENTIMC pClientImc;
    CLIENTIMC ClientImc;
    PINPUTCONTEXT pInputContext;
    INPUTCONTEXT InputContext;
    HANDLE hInputContext;
    BOOLEAN fUnicode = FALSE;
    BOOLEAN fVerbose, fDumpInputContext, fShowIMCMinInfo, fShowModeSaver, fShowCompStrRaw;
    char ach[32];

    if (param1 == NULL) {
        Print("!dimc -? for help\n");
        return FALSE;
    }

    //
    // If "All" option is specified, set all bits in opts
    // except type specifiers.
    //
    if (opts & OFLAG(a)) {
        opts |= ~(OFLAG(w) | OFLAG(c) | OFLAG(i) | OFLAG(u));
    }

    fVerbose = !!(opts & OFLAG(v));
    fShowCompStrRaw = (opts & OFLAG(s)) || fVerbose;
    fDumpInputContext = (opts & OFLAG(d)) || fShowCompStrRaw || fVerbose;
    fShowIMCMinInfo = (opts & OFLAG(h)) || fDumpInputContext || fVerbose;
    fShowModeSaver = (opts & OFLAG(r)) || fVerbose;

    if (opts & OFLAG(w)) {
        //
        // Arg is hwnd or pwnd.
        //
        PWND pwnd;

        if ((pwnd = HorPtoP(param1, TYPE_WINDOW)) == NULL) {
            return FALSE;
        }
        Print("pwnd=%p\n", pwnd);
        if (!tryMove(param1, &pwnd->hImc)) {
            Print("Could not read pwnd->hImc.\n");
            return FALSE;
        }
    }

    if (opts & OFLAG(c)) {
        //
        // Arg is client side IMC
        //
        pClientImc = param1;
        goto LClientImc;
    }

    if (opts & OFLAG(i)) {
        //
        // Arg is pInputContext.
        //
        pInputContext = param1;
        opts |= OFLAG(h);   // otherwise, nothing will be displayed !
        hInputContext = 0;
        if (opts & OFLAG(u)) {
            Print("Assuming Input Context is UNICODE.\n");
        }
        else {
            Print("Assuming Input Context is ANSI.\n");
        }
        goto LInputContext;
    }

    //
    // Otherwise, Arg is hImc.
    //
    if ((pImc = HorPtoP(param1, TYPE_INPUTCONTEXT)) == NULL) {
        Print("Idimc: %x is not an input context.\n", param1);
        return FALSE;
    }
    //move(imc, FIXKP(pImc));
    move(imc, pImc);

#ifdef KERNEL
    // Print simple thread info.
    if (imc.head.pti) {
        Idt(OFLAG(p), (PVOID)imc.head.pti);
    }
#endif

    //
    // Basic information
    //
    Print("pImc = %08lx  pti:%08lx\n", pImc, FIXKP(imc.head.pti));
    Print("  handle      %08lx\n", imc.head.h);
    Print("  dwClientImc %08lx\n", imc.dwClientImcData);
    Print("  hImeWnd     %08lx\n", imc.hImeWnd);

    //
    // Show client IMC
    //
    pClientImc = (PVOID)imc.dwClientImcData;
LClientImc:
    if (pClientImc == NULL) {
        Print("pClientImc is NULL.\n");
        return TRUE;
    }
    move(ClientImc, pClientImc);

    if (fVerbose) {
        sprintf(ach, "CLIENTIMC %p", pClientImc);
        Idso(0, ach);
    }
    else {
        Print("pClientImc @%p  cLockObj:%x\n", imc.dwClientImcData, ClientImc.cLockObj);
    }
    Print("  dwFlags %s\n", GetFlags(GF_CLIENTIMC, ClientImc.dwFlags, NULL, TRUE));
    fUnicode = !!(ClientImc.dwFlags & IMCF_UNICODE);

    //
    // Show InputContext
    //
    hInputContext = ClientImc.hInputContext;
    if (hInputContext) {
        move(pInputContext, hInputContext);
    }
    else {
        pInputContext = NULL;
    }
LInputContext:
    Print("InputContext %08lx (@%p)", hInputContext, pInputContext);

    if (pInputContext == NULL) {
        Print("\n");
        return TRUE;
    }

    //
    // if UNICODE specified by the option,
    // set the flag accordingly
    //
    if (opts & OFLAG(u)) {
        fUnicode = TRUE;
    }

    move(InputContext, pInputContext);
    Print("   hwnd=%p\n", InputContext.hWnd);

    if (fVerbose) {
        sprintf(ach, "INPUTCONTEXT %p", pInputContext);
        Idso(0, ach);
    }


    //
    // Decipher InputContext.
    //
    if (fShowIMCMinInfo) {
        PCOMPOSITIONSTRING pCompStr = NULL;
        PCANDIDATEINFO pCandInfo = NULL;
        PGUIDELINE pGuideLine = NULL;
        PTRANSMSGLIST pMsgBuf = NULL;
        DWORD i;

        Print("  dwRefCount: 0x%x      fdwDirty: %s\n",
              InputContext.dwRefCount, GetFlags(GF_IMEDIRTY, InputContext.fdwDirty, NULL, TRUE));
        Print("  Conversion: %s\n", GetFlags(GF_CONVERSION, InputContext.fdwConversion, NULL, TRUE));
        Print("  Sentence:   %s\n", GetFlags(GF_SENTENCE, InputContext.fdwSentence, NULL, TRUE));
        Print("  fChgMsg:    %d     uSaveVKey:   %02x %s\n",
              InputContext.fChgMsg,
              InputContext.uSavedVKey, GetVKeyName(InputContext.uSavedVKey));
        Print("  StatusWnd:  (0x%x,0x%x)   SoftKbd: (0x%x,0x%x)\n",
              InputContext.ptStatusWndPos.x, InputContext.ptStatusWndPos.y,
              InputContext.ptSoftKbdPos.x, InputContext.ptSoftKbdPos.y);
        Print("  fdwInit:    %s\n", GetFlags(GF_IMEINIT, InputContext.fdwInit, NULL, TRUE));
        // Font
        {
            LPCSTR fmt = "  Font:       '%s' %dpt wt:%d charset: %s\n";
            if (fUnicode) {
                fmt = "  Font:       '%S' %dpt wt:%d charset: %s\n";
            }
            Print(fmt,
                  InputContext.lfFont.A.lfFaceName,
                  InputContext.lfFont.A.lfHeight,
                  InputContext.lfFont.A.lfWeight,
                  GetMaskedEnum(EI_CHARSETTYPE, InputContext.lfFont.A.lfCharSet, NULL));
        }

        // COMPOSITIONFORM
        Print("  cfCompForm: %s pos:(0x%x,0x%x) rc:(0x%x,0x%x)-(0x%x,0x%x)\n",
              GetFlags(GF_IMECOMPFORM, InputContext.cfCompForm.dwStyle, NULL, TRUE),
              InputContext.cfCompForm.ptCurrentPos.x, InputContext.cfCompForm.ptCurrentPos.y,
              InputContext.cfCompForm.rcArea.left, InputContext.cfCompForm.rcArea.top,
              InputContext.cfCompForm.rcArea.right, InputContext.cfCompForm.rcArea.bottom);

        if (InputContext.hCompStr) {
            if (!tryMove(pCompStr, InputContext.hCompStr)) {
                Print("Could not get hCompStr=%08x\n", InputContext.hCompStr);
                //return FALSE;
            }
        }
        if (pCompStr && fVerbose) {
            sprintf(ach, "COMPOSITIONSTRING %p", pCompStr);
            Idso(0, ach);
        }

        if (InputContext.hCandInfo) {
            if (!tryMove(pCandInfo, InputContext.hCandInfo)) {
                Print("Could not get hCandInfo=%08x\n", InputContext.hCandInfo);
                //return FALSE;
            }
        }
        if (pCandInfo && fVerbose) {
            sprintf(ach, "CANDIDATEINFO %p", pCandInfo);
            Idso(0, ach);
        }

        if (InputContext.hGuideLine) {
            if (!tryMove(pGuideLine, InputContext.hGuideLine)) {
                Print("Could not get hGuideLine=%08x\n", InputContext.hGuideLine);
                //return FALSE;
            }
        }
        if (pGuideLine && fVerbose) {
            sprintf(ach, "GUIDELINE %p", pGuideLine);
            Idso(0, ach);
        }

        if (InputContext.hMsgBuf) {
            if (!tryMove(pMsgBuf, InputContext.hMsgBuf)) {
                Print("Could not get hMsgBuf=%08x\n", InputContext.hMsgBuf);
                //return FALSE;
            }
        }
        if (pMsgBuf && fVerbose) {
            sprintf(ach, "TRANSMSGLIST %p", pMsgBuf);
            Idso(0, ach);
        }

        if (!fDumpInputContext && !fVerbose) {
            Print("  CompStr @%p  CandInfo @%p  GuideL @%p  \n",
                pCompStr, pCandInfo, pGuideLine);
            Print("  MsgBuf @%p (0x%x)\n", pMsgBuf, InputContext.dwNumMsgBuf);
        }

        if (fDumpInputContext) {
            //
            // Composition String
            //
            if (pCompStr) {
                COMPOSITIONSTRING CompStr;

                move(CompStr, pCompStr);
                Print(" hCompositionString: %p (@%p) dwSize=0x%x\n",
                    InputContext.hCompStr, pCompStr, CompStr.dwSize);

                if (fShowCompStrRaw) {
                    _PrintInxAttr(NULL, NULL, 0, 0);
                    PrintInxElementA(CompRead);
                    PrintInxElementA(Comp);
                    PrintInxElementB(ResultRead);
                    PrintInxElementB(Result);
                }

                Print("  CursorPos=0x%x  DeltaStart=0x%x\n",
                    CompStr.dwCursorPos, CompStr.dwDeltaStart);
                Print("  Private: (@%p) off:0x%x len:0x%x\n",
                    (PBYTE)pCompStr + CompStr.dwPrivateOffset,
                    CompStr.dwPrivateOffset, CompStr.dwPrivateSize);

                Print("\n");

                PrintInxFriendlyStrA(CompRead);
                PrintInxFriendlyStrA(Comp);
                PrintInxFriendlyStrB(ResultRead);
                PrintInxFriendlyStrB(Result);

                Print("\n");
            }
            else {
                Print(" pCompStr is NULL\n");
            }

            //
            // Candidate Info
            //
            if (pCandInfo) {
                CANDIDATEINFO CandInfo;

                move(CandInfo, pCandInfo);
                Print(" hCandidateInfo: %p (@%p) dwSize=0x%x dwCount=0x%x PrivOffset=0x%x PrivSize=0x%x\n",
                      InputContext.hCandInfo, pCandInfo, CandInfo.dwSize, CandInfo.dwCount,
                      CandInfo.dwPrivateOffset, CandInfo.dwPrivateSize);

                for (i = 0; i < CandInfo.dwCount; ++i) {
                    PCANDIDATELIST pCandList;
                    CANDIDATELIST CandList;
                    DWORD j;

                    pCandList = (PCANDIDATELIST)((PBYTE)pCandInfo + CandInfo.dwOffset[i]);
                    move(CandList, pCandList);

                    Print("   CandList[%02x] (@%p) %s count=%x sel=%x pgStart=%x pgSize=%x\n",
                          i, pCandList, GetMaskedEnum(EI_IMECANDIDATESTYLE, CandList.dwStyle, NULL),
                          CandList.dwCount,
                          CandList.dwSelection, CandList.dwPageStart, CandList.dwPageSize);

                    if (CandList.dwStyle == IME_CAND_CODE && CandList.dwCount == 1) {
                        // Special case
                        Print("     Special case: DBCS char = %04x", CandList.dwOffset[0]);
                    }
                    else if (CandList.dwCount > 1) {
                        for (j = 0; j < CandList.dwCount; ++j) {
                            DWORD k;
                            DWORD dwOffset;

                            move(dwOffset, pCandList->dwOffset + j);

                            Print("    %c%c[%02x]@%p ",
                                  j == CandList.dwSelection ? '*' : ' ',
                                  (j >= CandList.dwPageStart && j < CandList.dwPageStart + CandList.dwPageSize) ? '+' : ' ',
                                  j, (PBYTE)pCandList + dwOffset);
                            for (k = 0; k < 0x100; ++k) {   // limit upto 0xff cch
                                WCHAR wchar;

                                if (fUnicode) {
                                    move(wchar, (PWCHAR)((PBYTE)pCandList + dwOffset) + k);
                                }
                                else {
                                    BYTE bchar;
                                    move(bchar, (PBYTE)pCandList + dwOffset + k);
                                    wchar = bchar;
                                }
                                if (wchar == 0) {
                                    break;
                                }
                                Print("|%s", GetInxStr(wchar, fUnicode));
                            }
                            Print("|\n");
                        }
                    }
                }
            }

            if (pGuideLine) {
                GUIDELINE GuideLine;

                move(GuideLine, pGuideLine);
                Print(" hGuideLine: %p (@%p) dwSize=0x%x\n",
                    InputContext.hGuideLine, pGuideLine, GuideLine.dwSize);

                Print("   level:%x index;%x privOffset:%x privSize:%x\n",
                      GuideLine.dwLevel, GuideLine.dwIndex,
                      GuideLine.dwPrivateSize, GuideLine.dwPrivateOffset);

                if (GuideLine.dwStrOffset && GuideLine.dwStrLen) {
                    // String
                    Print("   str @%p  ", (PBYTE)pGuideLine + GuideLine.dwStrOffset);
                    for (i = 0; i < GuideLine.dwStrLen; ++i) {
                        WCHAR wchar;

                        if (fUnicode) {
                            move(wchar, (PWCHAR)((PBYTE)pGuideLine + GuideLine.dwStrOffset) + i);
                        }
                        else {
                            BYTE bchar;
                            move(bchar, (PBYTE)pGuideLine + GuideLine.dwStrOffset + i);
                            wchar = bchar;
                        }
                        Print("|%s", GetInxStr(wchar, fUnicode));
                    }
                    Print("|\n");
                }
            }

            if (pMsgBuf) {
                TRANSMSGLIST TransMsgList;

                move(TransMsgList, pMsgBuf);
                Print(" hMsgBuf: %p (@%p) dwNumMsgBuf=0x%x uMsgCount=0x%x\n",
                    InputContext.hMsgBuf, pMsgBuf, InputContext.dwNumMsgBuf,
                    TransMsgList.uMsgCount);

                if (InputContext.dwNumMsgBuf) {
                    PTRANSMSG pTransMsg = pMsgBuf->TransMsg;

                    Print("  | ## |msg | wParam | lParam |\n");
                    Print("  +----+----+--------+--------+\n");

                    for (i = 0; i < InputContext.dwNumMsgBuf; ++i, ++pTransMsg) {
                        const char* pszMsg = "";
                        TRANSMSG TransMsg;
                        DWORD j;

                        move(TransMsg, pTransMsg);

                        // Try to find a readable name of the window message
                        for (j = 0; j < ARRAY_SIZE(gaMsgs); ++j) {
                            if (gaMsgs[i].msg == TransMsg.message) {
                                pszMsg = gaMsgs[j].pszMsg;
                                break;
                            }
                        }

                        Print("   | %02x |%04x|%08x|%08x| %s\n",
                            i,
                            TransMsg.message, TransMsg.wParam, TransMsg.lParam, pszMsg);
                    }
                    Print("  +----+----+--------+--------+\n");
                }
            }

        }
    }

    //
    // Recursively display Mode Savers.
    //
    if (fShowModeSaver) {
        PIMEMODESAVER pModeSaver = InputContext.pImeModeSaver;

        //
        // Private Mode Savers.
        //
        while (pModeSaver) {
            IMEMODESAVER ImeModeSaver;

            move(ImeModeSaver, pModeSaver);
            Print("ImeModeSaver @ %p -- LangId=%04x fOpen=%d\n",
                  pModeSaver, ImeModeSaver.langId, ImeModeSaver.fOpen);
            Print("    fdwInit      %s\n", GetFlags(GF_IMEINIT, ImeModeSaver.fdwInit, NULL, TRUE));
            Print("    Conversion   %s\n", GetFlags(GF_CONVERSION, ImeModeSaver.fdwConversion, NULL, TRUE));
            Print("    Sentence     %s\n", GetFlags(GF_SENTENCE, ImeModeSaver.fdwSentence, NULL, TRUE));
            move(pModeSaver, &pModeSaver->next);
        }
    }

    return TRUE;
}

#ifndef KERNEL
/************************************************************************\
* Procedure: Ikc
*
* Dumps keyboard cues state for the window, and pertinent info on the
* parent KC state and the system settings related to this
*
* 06/11/98 MCostea     Created
*
\************************************************************************/
BOOL Ikc(
    DWORD opts,
    PVOID param1)
{
    WND wnd;
    PWND pwnd, pwndParent;
    char ach[80];
    BOOL bHideFocus, bHideAccel;
    SERVERINFO si;
    PSERVERINFO psi;

    if (param1 && (pwnd = HorPtoP(param1, TYPE_WINDOW)) == NULL) {
        Print("Idw: %x is not a pwnd.\n", param1);
        return FALSE;
    }
    moveExpValuePtr(&psi, VAR(gpsi));
    move(si, psi);

    if (si.bKeyboardPref) {
        Print("gpsi->bKeyboardPref ON, KC mechanism is turned off\n");
    }
    if (!(si.PUSIFlags & (PUSIF_KEYBOARDCUES | PUSIF_UIEFFECTS) == PUSIF_KEYBOARDCUES | PUSIF_UIEFFECTS)) {
        Print("Either the UI effects or PUSIF_KEYBOARDCUES are off\n");
    }

    if (!param1) {
        return FALSE;
    }
    move(wnd, FIXKP(pwnd));
    /*
     * Print pwnd and title string.
     */
    DebugGetWindowTextA(pwnd, ach);
    Print("pwnd = %08lx  \"%s\"\n", pwnd, ach);
    bHideAccel = TestWF(pwnd, WEFPUIACCELHIDDEN);
    bHideFocus = TestWF(pwnd, WEFPUIFOCUSHIDDEN);

    switch(wnd.fnid) {
    case FNID_BUTTON :
        {
            Print("FNID_BUTTON");
        }
        goto printCues;
    case FNID_LISTBOX :
        {
            Print("FNID_LISTBOX");
        }
        goto printCues;
    case FNID_DIALOG :
        {
            Print("FNID_DIALOG");
        }
        goto printCues;
    case FNID_STATIC :
        {
            Print("FNID_STATIC");
        }
printCues:
        Print(bHideAccel ? " Hide Accel" : " Show Accel");
        Print(bHideFocus ? " Hide Focus" : " Show Focus");
        break;

    default:
        Print("Not KC interesting FNID 0x%x", wnd.fnid);
        break;
    }
    Print("\n");

    pwndParent = wnd.spwndParent;
    move(wnd, FIXKP(wnd.spwndParent));
    if (wnd.fnid == FNID_DIALOG) {
        Print("The parent is a dialog:\n");
        Ikc(opts, pwndParent);
    } else {
        Print("The parent is not a dialog\n");
    }
    return TRUE;
}
#endif

#ifdef KERNEL

/************************************************************************\
* Procedure: Idimk -- dump IME Hotkeys
*
* 08/09/98 HiroYama     Created
*
\************************************************************************/

#define IHK_ITEM(x) { x, #x }

BOOL Idimk(DWORD opts, PVOID param1)
{
    PIMEHOTKEYOBJ pObj;
    IMEHOTKEYOBJ hotkeyObj;
    static const struct {
        DWORD mask;
        const char* name;
    } masks[] = {
        IHK_ITEM(MOD_IGNORE_ALL_MODIFIER),
        IHK_ITEM(MOD_ON_KEYUP),
        IHK_ITEM(MOD_RIGHT),
        IHK_ITEM(MOD_LEFT),
        IHK_ITEM(MOD_SHIFT),
        IHK_ITEM(MOD_CONTROL),
        IHK_ITEM(MOD_ALT),
    };
    int nHotKeys = 0;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        moveExpValuePtr(&pObj, VAR(gpImeHotKeyListHeader));
        if (!pObj) {
            Print("No IME HotKeys. win32k!gpImeHotKeyListHeader is NULL.\n\n");
            return FALSE;
        }
        Print("using win32k!gpImeHotKeyListHeader (@0x%p)\n", pObj);
    }
    else {
        pObj = (PIMEHOTKEYOBJ)FIXKP(param1);
    }

    while (pObj) {
        int i, n;

        move(hotkeyObj, pObj);

        Print("ImeHotKeyObj @ 0x%p\n", pObj);
        Print("    pNext          0x%p\n", hotkeyObj.pNext);
#if 0
        Print("IMEHOTKEY @ 0x%p\n", &pObj->hk);
#endif
        Print("    dwHotKeyID     0x%04x    ", hotkeyObj.hk.dwHotKeyID);

        //
        // Show hotkey ID by name
        //
        if (hotkeyObj.hk.dwHotKeyID >= IME_HOTKEY_DSWITCH_FIRST && hotkeyObj.hk.dwHotKeyID <= IME_HOTKEY_DSWITCH_LAST) {
            Print(" Direct Switch to HKL 0x%p", hotkeyObj.hk.hKL);
        }
        else {
            Print(" %s", GetMaskedEnum(EI_IMEHOTKEYTYPE, hotkeyObj.hk.dwHotKeyID, NULL));
        }

        //
        // Show VKey value by name
        //
        Print("\n    uVKey          0x%02x       %s\n", hotkeyObj.hk.uVKey, GetVKeyName(hotkeyObj.hk.uVKey));

        //
        // Show bit mask by name
        //
        Print(  "    Modifiers      0x%04x     ", hotkeyObj.hk.uModifiers);
        n = 0;
        for (i = 0; i < ARRAY_SIZE(masks); ++i) {
            if (masks[i].mask & hotkeyObj.hk.uModifiers) {
                Print("%s%s", n ? " | " : "", masks[i].name);
                ++n;
            }
        }

        //
        // Target HKL
        //
        Print("\n    hKL            0x%p\n\n", hotkeyObj.hk.hKL);

        pObj = hotkeyObj.pNext;

        //
        // If address is specified as an argument, just display one instance.
        //
        if (param1 != NULL) {
            break;
        }

        ++nHotKeys;
    }
    if (nHotKeys) {
        Print("Number of IME HotKeys: 0x%04x\n", nHotKeys);
    }

    return TRUE;
}

#undef IHK_ITEM

#endif  // KERNEL

#ifdef KERNEL
/************************************************************************\
* Procedure: Igflags
*
* Dumps NT Global Flags
*
* 08/11/98 Hiroyama     Created
*
\************************************************************************/

#define DGF_ITEM(x) { x, #x }

BOOL Igflags(DWORD opts)
{
    static const struct {
        DWORD dwFlag;
        const char* name;
    } names[] = {
        DGF_ITEM(FLG_STOP_ON_EXCEPTION),
        DGF_ITEM(FLG_SHOW_LDR_SNAPS),
        DGF_ITEM(FLG_DEBUG_INITIAL_COMMAND),
        DGF_ITEM(FLG_STOP_ON_HUNG_GUI),
        DGF_ITEM(FLG_HEAP_ENABLE_TAIL_CHECK),
        DGF_ITEM(FLG_HEAP_ENABLE_FREE_CHECK),
        DGF_ITEM(FLG_HEAP_VALIDATE_PARAMETERS),
        DGF_ITEM(FLG_HEAP_VALIDATE_ALL),
        DGF_ITEM(FLG_POOL_ENABLE_TAGGING),
        DGF_ITEM(FLG_HEAP_ENABLE_TAGGING),
        DGF_ITEM(FLG_USER_STACK_TRACE_DB),
        DGF_ITEM(FLG_KERNEL_STACK_TRACE_DB),
        DGF_ITEM(FLG_MAINTAIN_OBJECT_TYPELIST),
        DGF_ITEM(FLG_HEAP_ENABLE_TAG_BY_DLL),
        DGF_ITEM(FLG_ENABLE_CSRDEBUG),
        DGF_ITEM(FLG_ENABLE_KDEBUG_SYMBOL_LOAD),
        DGF_ITEM(FLG_DISABLE_PAGE_KERNEL_STACKS),
        DGF_ITEM(FLG_HEAP_DISABLE_COALESCING),
        DGF_ITEM(FLG_ENABLE_CLOSE_EXCEPTIONS),
        DGF_ITEM(FLG_ENABLE_EXCEPTION_LOGGING),
        DGF_ITEM(FLG_ENABLE_HANDLE_TYPE_TAGGING),
        DGF_ITEM(FLG_HEAP_PAGE_ALLOCS),
        DGF_ITEM(FLG_DEBUG_INITIAL_COMMAND_EX),
        DGF_ITEM(FLG_DISABLE_DBGPRINT),
    };
    DWORD dwFlags;
    int i, n = 0;

    moveExpValue(&dwFlags, "NT!NtGlobalFlag");
    if (opts & OFLAG(v)) {
        Print("NT!NtGlobalFlag                         %08lx\n\n", dwFlags);
    }
    else {
        Print("NT!NtGlobalFlag 0x%lx\n", dwFlags);
    }

    dwFlags &= FLG_VALID_BITS;

    for (i = 0; i < ARRAY_SIZE(names); ++i) {
        BOOLEAN on = (dwFlags & names[i].dwFlag) != 0;

        if (opts & OFLAG(v)) {
            Print("  %c%-34s %c(%08x)\n", on ? '*' : ' ', names[i].name, on ? '*' : ' ', names[i].dwFlag);
        }
        else {
            if (n++ % 2 == 0) {
                Print("\n");
            }
            Print(" %c%-29s ", on ? '*' : ' ', names[i].name + sizeof "FLG_" - 1);
        }
    }
    if (!(opts & OFLAG(v))) {
        Print("\n");
    }

    return TRUE;
}

#undef DGF_ITEM

#endif  // KERNEL

/************************************************************************\
* Procedure: Ivkey
*
* Dumps virtual keys
*
* 08/11/98 Hiroyama     Created
*
\************************************************************************/

void PrintVKey(int i)
{
    Print("  %02x %s\n", gVKeyDef[i].dwVKey, gVKeyDef[i].name);
}

BOOL Ivkey(DWORD opts, LPSTR pszName)
{
    int i;

    if ((opts & OFLAG(a)) || (opts & OFLAG(o))) {
        //
        // List all virtual keys.
        //
        int n = 0;

        for (i = 0; i < 0x100; ++i) {
            const char* name = GetVKeyName(i);
            if (*name) {
                char buf[128];
                int len;

                sprintf(buf, " %02x %-35s", i, name);

                if (opts & OFLAG(a)) {
                    //
                    // If it exceeds the second column width, begin new line.
                    //
                    if ((len = strlen(buf)) >= 40 && n % 2 == 1) {
                        Print("\n");
                        n = 0;
                    }
                    Print(buf);
                    //
                    // If it's in the second column, begin new line.
                    //
                    if (++n % 2 == 0 || len >= 40) {
                        Print("\n");
                        n = 0;
                    }
                }
                else {
                    Print("%s\n", buf);
                }
            }
        }
        Print("\n");
    }
    else if (*pszName == 'V' || *pszName == 'v') {
        //
        // Search by VK name.
        //
        int nFound = 0;
        int len = strlen(pszName);

        if (len == 4) {
            int ch = pszName[3];

            if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
                Print("  %02x %s\n", ch, pszName);
                ++nFound;
            }
        }

        for (i = 0; i < ARRAY_SIZE(gVKeyDef); ++i) {
            if (_strnicmp(gVKeyDef[i].name, pszName, len) == 0) {
                Print("  %02x %s\n", gVKeyDef[i].dwVKey, gVKeyDef[i].name);
                ++nFound;
            }
        }
        if (nFound == 0) {
            Print("Could not find it.\n");
        }
    }
    else {
        //
        // Search by VK value.
        //
        NTSTATUS status;
        DWORD dwVKey;
        const char* name;

        status = GetInteger(pszName, 16, &dwVKey, NULL);
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
        name = GetVKeyName(dwVKey);
        if (*name) {
            Print("  %02x %s\n", dwVKey, name);
        }
        else {
            Print("Could not find it.\n");
        }
    }

    return TRUE;
}

/************************************************************************\
* Procedure: Idisi
*
* Dumps event injection union
*
* 09/??/98 Hiroyama     Created
*
\************************************************************************/

BOOL Idisi(DWORD opts, PVOID param1)
{
    PINPUT pObj;
    INPUT input;

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        return FALSE;
    }

    pObj = (PINPUT)FIXKP(param1);
    move(input, pObj);

    Print("INPUT @%p - size: 0x%x\n", pObj, sizeof input);

    switch (input.type) {
    case INPUT_MOUSE:
        {
            MOUSEINPUT* pmi = &input.mi;

            Print("type: Mouse Input(%x)\n", input.type);
            Print("     dx          %lx (%ld in dec.)\n", pmi->dx, pmi->dx);
            Print("     dy          %lx (%ld in dec.)\n", pmi->dx, pmi->dx);
            Print("     mouseData   %lx (%ld in dec.)\n", pmi->mouseData, pmi->mouseData);
            Print("     dwFlags     %lx (%s)\n", pmi->dwFlags, GetFlags(GF_MI, pmi->dwFlags, NULL, TRUE));
            Print("     time        %lx\n", pmi->time);
            Print("     dwExtraInfo %lx\n", pmi->dwExtraInfo);
        }
        break;

    case INPUT_KEYBOARD:
        {
            KEYBDINPUT* pki = &input.ki;
            const char* name;

            Print("type: Keyboard Input(%x)\n", input.type);
            //
            // Print Vkey
            //
            Print("     wVk         %lx", pki->wVk);
            name = GetVKeyName(pki->wVk);
            if (*name) {
                Print(" (%s)\n", name);
            } else {
                Print("\n");
            }
            //
            // Print scan code: if KEYEVENTF_UNICODE, it's UNICODE value.
            //
            if (pki->dwFlags & KEYEVENTF_UNICODE) {
                Print("     UNICODE     %lx\n", pki->wScan);
            } else {
                Print("     wScan       %lx\n", pki->wScan);
            }
            //
            // Print and decrypt dwFlags
            //
            Print("     dwFlags     %lx (%s)\n", pki->dwFlags, GetFlags(GF_KI, pki->dwFlags, NULL, TRUE));
            Print("     time        %lx\n", pki->time);
            Print("     dwExtraInfo %lx\n", pki->dwExtraInfo);
        }
        break;

    case INPUT_HARDWARE:
        Print("type: HardwareEvent(%x)\n", input.type);
        Print("         uMsg            %lx\n", input.hi.uMsg);
        Print("         wParamH:wParamL %x:%x\n", input.hi.wParamH, input.hi.wParamL);
        break;

    default:
        Print("Invalid type information(0x%lx)\n", input.type);
        break;
    }

    return TRUE;
}


/************************************************************************\
* Procedure: Iwm
*
* Decrypt window message number
*
* 09/??/98 Hiroyama     Created
*
\************************************************************************/

BOOL Iwm(DWORD opts, LPSTR pszName)
{
    int i;
    DWORD value;

    UNREFERENCED_PARAMETER(opts);

    if (!(opts & OFLAG(a)) && *pszName == 0) {
        return FALSE;
    }

    if (!NT_SUCCESS(GetInteger(pszName, 16, &value, NULL)) || strchr(pszName, '_')) {
        //
        // Search by WM name.
        //
        int nFound = 0;
        int len = strlen(pszName);

        for (i = 0; i < ARRAY_SIZE(gaMsgs); ++i) {
            if (_strnicmp(gaMsgs[i].pszMsg, pszName, len) == 0) {
                Print("  %04x %s\n", gaMsgs[i].msg, gaMsgs[i].pszMsg);
                ++nFound;
            }
        }
        if (nFound == 0) {
            Print("Could not find it.\n");
        }
    }
    else {
        //
        // Search by WM value.
        //
        int i;

        for (i = 0; i < ARRAY_SIZE(gaMsgs); ++i) {
            if (gaMsgs[i].msg == value) {
                Print("  %04x %s\n", gaMsgs[i].msg, gaMsgs[i].pszMsg);
                break;
            }
        }
    }

    return TRUE;
}

//
// Dump Dialog Template
//
//

PBYTE SkipSz(UTCHAR *lpsz, UTCHAR* lpszCopy, UINT len)
{
    UTCHAR c;
    UINT n = 0;

    lpszCopy[len - 1] = 0;

    move(c, lpsz);
    if (c == 0xFF) {
        if (lpszCopy) {
            *lpszCopy = 0;
        }
        return (PBYTE)lpsz + 4;
    }

    do {
        move(c, lpsz);
        ++lpsz;
        if (++n < len) {
            if (lpszCopy) {
                *lpszCopy ++ = c;
            }
        }
    } while (c != 0);

    return (PBYTE)lpsz;
}


#ifndef NextWordBoundary
#define NextWordBoundary(p)     ((PBYTE)(p) + ((ULONG_PTR)(p) & 1))
#endif
#ifndef NextDWordBoundary
#define NextDWordBoundary(p)    ((PBYTE)(p) + ((ULONG_PTR)(-(LONG_PTR)(p)) & 3))
#endif

PBYTE WordSkipSz(UTCHAR *lpsz, UTCHAR* lpszCopy, UINT len)
{
    PBYTE pb = SkipSz(lpsz, lpszCopy, len);
    return NextWordBoundary(pb);
}

PBYTE DWordSkipSz(UTCHAR *lpsz, UTCHAR* lpszCopy, UINT len)
{
    PBYTE pb = SkipSz(lpsz, lpszCopy, len);
    return NextDWordBoundary(pb);
}

LPCSTR GetCharSetName(BYTE charset)
{
    return GetMaskedEnum(EI_CHARSETTYPE, charset, NULL);
}

VOID ParseDialogFont(LPWORD* lplpstr, LPDLGTEMPLATE2 lpdt)
{
    LOGFONT     LogFont;
    short       tmp;
    int         fontheight, fheight;
    PSERVERINFO gpsi;
    BOOL fDesktopCharset = FALSE;
    WORD   dmLogPixels;

//
//  fheight = fontheight = (SHORT)(*((WORD *) *lplpstr)++);
//
    move(tmp, *lplpstr);
    ++*lplpstr;
    fontheight = fheight = tmp;

    if (fontheight == 0x7FFF) {
        // a 0x7FFF height is our special code meaning use the message box font
        Print("\
    Font    System Font (Messagebox font)\n");
        return;
    }


    //
    // The dialog template contains a font description! Use it.
    //
    // Fill the LogFont with default values
    RtlZeroMemory(&LogFont, sizeof(LOGFONT));

    moveExpValue(&gpsi, VAR(gpsi));
    move(dmLogPixels, &gpsi->dmLogPixels);
    LogFont.lfHeight = -MultDiv(fontheight, dmLogPixels, 72);

    if (lpdt->wDlgVer) {
        WORD w;
        BYTE b;
//
//      LogFont.lfWeight  = *((WORD FAR *) *lplpstr)++;
//
        move(w, *lplpstr);
        ++*lplpstr;
        LogFont.lfWeight = w;
//
//      LogFont.lfItalic  = *((BYTE FAR *) *lplpstr)++;
//
        move(b, *lplpstr);
        ++((BYTE*)*lplpstr);
        LogFont.lfItalic = b;

//
//      LogFont.lfCharSet = *((BYTE FAR *) *lplpstr)++;
//
        move(b, *lplpstr);
        ++((BYTE*)*lplpstr);
        LogFont.lfCharSet = b;
    }
    else {
        // DIALOG statement, which only has a facename.
        // The new applications are not supposed to use DIALOG statement,
        // they should use DIALOGEX instead.
        LogFont.lfWeight  = FW_BOLD;
        LogFont.lfCharSet = 0;  //(BYTE)GET_DESKTOP_CHARSET();
        fDesktopCharset = TRUE;
    }

    *lplpstr = (WORD*)DWordSkipSz(*lplpstr, LogFont.lfFaceName, ARRAY_SIZE(LogFont.lfFaceName));

    Print("\
    Font    %dpt (%d), Weight: %d, %s Italic, %s,\n\
            \"%ls\"\n",
        fontheight, LogFont.lfHeight,
        LogFont.lfWeight,
        LogFont.lfItalic ? "" : "Not",
        fDesktopCharset ? "DESKTOP_CHARSET" : GetCharSetName(LogFont.lfCharSet),
        LogFont.lfFaceName);
}

LPCSTR GetCtrlStyle(WORD iClass, DWORD style)
{
    WORD type = GF_WS;

    switch (iClass) {
    case ICLS_DIALOG:
        type = GF_DS;
        break;
    case ICLS_STATIC:
        type = GF_SS;
        break;
    case ICLS_EDIT:
        type = GF_ES;
        break;
    case ICLS_BUTTON:
        type = GF_BS;
        break;
    case ICLS_COMBOBOX:
        type = GF_CBS;
        break;
    case ICLS_LISTBOX:
        type = GF_LBS;
        break;
    case ICLS_SCROLLBAR:
        type = GF_SBS;
        break;
    default:
        break;
    }
    return GetFlags(type, style, NULL, FALSE);
}

BOOL Iddlgt(DWORD opts, LPVOID param1)
{
    LPDLGTEMPLATE lpdt = (LPDLGTEMPLATE)FIXKP(param1);
    DLGTEMPLATE2 dt;
    LPDLGTEMPLATE2 lpdt2 = &dt;
    BOOLEAN fNewDialogTemplate = FALSE;
    UTCHAR* lpszMenu;
    UTCHAR* lpszClass;
    UTCHAR* lpszText;
    UTCHAR* lpStr;
    UTCHAR* lpCreateParams;
    LPCSTR  lpszIClassName;
    WORD w;
    DLGITEMTEMPLATE2    dit;
    LPDLGITEMTEMPLATE   lpdit;
    UTCHAR menuName[64];
    UTCHAR className[64];
    UTCHAR text[64];
    PSERVERINFO gpsi;

    UNREFERENCED_PARAMETER(opts);

    if (opts == 0 && param1 == NULL) {
        return FALSE;
    }

    move(w, &((LPDLGTEMPLATE2)lpdt)->wSignature);

    if (w == 0xffff) {
        move(dt, lpdt);
        fNewDialogTemplate = TRUE;
    }
    else {
        dt.wDlgVer = 0;
        dt.wSignature = 0;
        dt.dwHelpID = 0;
        move(dt.dwExStyle, &lpdt->dwExtendedStyle);
        move(dt.style, &lpdt->style);
        move(dt.cDlgItems, &lpdt->cdit);
        move(dt.x, &lpdt->x);
        move(dt.y, &lpdt->y);
        move(dt.cx, &lpdt->cx);
        move(dt.cy, &lpdt->cy);
    }


    Print("DlgTemplate%s @%p version %d\n", dt.wDlgVer ? "2" : "", lpdt, dt.wDlgVer);

    if (!(opts & OFLAG(v))) {
        Print("\
    (%d, %d)-(%d,%d) [%d, %d](dec)\n\
    Style   %08lx   ExStyle %08lx   items 0x%x\n",
              dt.x, dt.y, dt.x + dt.cx, dt.y + dt.cy, dt.cx, dt.cy,
              dt.style, dt.dwExStyle, dt.cDlgItems);
    }
    else {
        Print("\
    (%d,%d)-(%d,%d) [%d,%d] (dec)  item: 0x%lx\n",
              dt.x, dt.y, dt.x + dt.cx, dt.y + dt.cy,
              dt.cx, dt.cy,
              dt.cDlgItems);
        Print("\
    Style   %08lx %s", dt.style, OFLAG(v) ? GetFlags(GF_DS, dt.style, NULL, FALSE) : "");
        if ((dt.style & DS_SHELLFONT) == DS_SHELLFONT) {
            Print(" [DS_SHELLFONT]");
        }
        Print("\n");
        Print("\
    ExStyle %08lx %s\n", dt.dwExStyle, GetFlags(GF_WSEX, dt.dwExStyle, NULL, FALSE));
    }

    // If there's a menu name string, load it.
    lpszMenu = (LPWSTR)(((PBYTE)(lpdt)) + (dt.wDlgVer ? sizeof(DLGTEMPLATE2):sizeof(DLGTEMPLATE)));

    /*
     * If the menu id is expressed as an ordinal and not a string,
     * skip all 4 bytes to get to the class string.
     */
    move(w, (WORD*)lpszMenu);

    /*
     * If there's a menu name string, load it.
     */
    if (w != 0) {
        if (w == 0xffff) {
            LPWORD lpwMenu = (LPWORD)((LPBYTE)lpszMenu + 2);
            move(w, lpwMenu);
            Print("\
    menu id     %lx\n", w);
        }
    }

    if (w == 0xFFFF) {
        lpszClass = (LPWSTR)((LPBYTE)lpszMenu + 4);
    } else {
        lpszClass = (UTCHAR *)WordSkipSz(lpszMenu, menuName, ARRAY_SIZE(menuName));
        Print("\
    menu   @%p \"%ls\"\n", lpszMenu, menuName);
    }

    //
    // Class name
    //
    lpszText = (UTCHAR *)WordSkipSz(lpszClass, className, ARRAY_SIZE(className));
    Print("\
    class  @%p \"%ls\"\n", lpszClass, className);

    //
    // Window text
    //
    lpStr = (UTCHAR *)WordSkipSz(lpszText, text, ARRAY_SIZE(text));
    Print("\
    text   @%p \"%ls\"\n", lpszText, text);

    //
    // Font
    //
    if (dt.style & DS_SETFONT) {
        ParseDialogFont(&lpStr, &dt);
    }

    lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(lpStr);


    ///////////////////////////////////////////////////
    // if "-r" option is not specified, bail out.
    ///////////////////////////////////////////////////
    if (!(opts & OFLAG(r))) {
        return TRUE;
    }

    Print("\n");

    /*
     * Loop through the dialog controls, doing a CreateWindowEx() for each of
     * them.
     */
    while (dt.cDlgItems-- != 0) {
        WORD iClass = 0;
        //
        // Retrieve basic information.
        //

        if (dt.wDlgVer) {
            move(dit, lpdit);
        } else {
            dit.dwHelpID = 0;
            move(dit.dwExStyle, &lpdit->dwExtendedStyle);
            move(dit.style, &lpdit->style);
            move(dit.x, &lpdit->x);
            move(dit.y, &lpdit->y);
            move(dit.cx, &lpdit->cx);
            move(dit.cy, &lpdit->cy);
            move(w, &lpdit->id);
            dit.dwID = w;
        }

        Print("\
#ID:0x%04x @%p HelpID:0x%04x (%d,%d)-(%d,%d) [%d,%d] (dec)\n",
            dit.dwID,
            lpdit,
            dit.dwHelpID,
            dit.x, dit.y, dit.x + dit.cx, dit.y + dit.cy,
            dit.cx, dit.cy);

        //
        // Skip DLGITEMTEMPLATE or DLGITEMTEMPLATE2
        //
        lpszClass = (LPWSTR)(((PBYTE)(lpdit)) + (dt.wDlgVer ? sizeof(DLGITEMTEMPLATE2) : sizeof(DLGITEMTEMPLATE)));

        /*
         * If the first WORD is 0xFFFF the second word is the encoded class name index.
         * Use it to look up the class name string.
         */
        move(w, lpszClass);
        if (w == 0xFFFF) {
            WORD wAtom;

            lpszText = lpszClass + 2;
#ifdef ORG
            lpszClass = (LPWSTR)(gpsi->atomSysClass[*(((LPWORD)lpszClass)+1) & ~CODEBIT]);
#endif
            moveExpValue(&gpsi, VAR(gpsi));
            move(iClass, lpszClass + 1);
            iClass &= ~CODEBIT;
            if (*(lpszIClassName = GetMaskedEnum(EI_CLSTYPE, iClass, NULL)) == '\0') {
                lpszIClassName = NULL;
            }
            move(wAtom, &gpsi->atomSysClass[iClass]);
            swprintf(className, L"#%lx", wAtom);
        } else {
            lpszText = (UTCHAR*)SkipSz(lpszClass, className, ARRAY_SIZE(className));
            lpszIClassName = NULL;
        }

        Print("\
    class  @%p \"%ls\" ", lpszClass, className);
        if (lpszIClassName) {
            Print("= %s", lpszIClassName);
        }
        Print("\n");

        lpszText = (UTCHAR*)NextWordBoundary(lpszText); // UINT align lpszText

//      Our code in InternalCreateDialog does this.
//        dit.dwExStyle |= WS_EX_NOPARENTNOTIFY;
//
        /*
         * Get pointer to additional data.  lpszText can point to an encoded
         * ordinal number for some controls (e.g.  static icon control) so
         * we check for that here.
         */

        move(w, lpszText);
        if (w == 0xFFFF) {
            swprintf(text, L"#%lx", w);
            lpCreateParams = (LPWSTR)((PBYTE)lpszText + 4);
        } else {
            lpCreateParams = (LPWSTR)(PBYTE)WordSkipSz(lpszText, text, ARRAY_SIZE(text));
        }

        Print("\
    text   @%p \"%ls\"\n", lpszText, text);

        Print("\
    style   %08lx %s%s", dit.style,
                (opts & OFLAG(v)) ? GetCtrlStyle(iClass, dit.style) : "",
                (opts & OFLAG(v)) ? "\n" : "");
        Print("\
    ExStyle %08lx %s\n", dit.dwExStyle, (opts & OFLAG(v)) ? GetFlags(GF_WSEX, dit.dwExStyle, NULL, FALSE) : "");

        /*
         * Point at next item template
         */
        move(w, lpCreateParams);
        lpdit = (LPDLGITEMTEMPLATE)NextDWordBoundary(
                (LPBYTE)(lpCreateParams + 1) + w);
        Print("\n");
    }


    return TRUE;
}

#if !defined(KERNEL)
BOOL Idimedpi(DWORD opts, PVOID param1)
{
    IMEDPI ImeDpi;
    PIMEDPI pImeDpi;
    char ach[32];

    UNREFERENCED_PARAMETER(opts);

    if (param1 == NULL) {
        moveExpValue(&pImeDpi, "imm32!gpImeDpi");
    }
    else {
        pImeDpi = (PIMEDPI)FIXKP(param1);
    }

    while (pImeDpi) {
        move(ImeDpi, pImeDpi);

        Print("IMEDPI @ %08p  hInst: %08p cLock:%3d", pImeDpi, ImeDpi.hInst, ImeDpi.cLock);
        Print("  CodePage:%4d UI Class: \"%S\"\n", ImeDpi.dwCodePage, ImeDpi.wszUIClass);
        if (opts & OFLAG(i)) {
            Print(" ImeInfo: @ %08p   dwPrivateDataSize: %08x\n", &pImeDpi->ImeInfo,
                  ImeDpi.ImeInfo.dwPrivateDataSize);
        }

        if (opts & OFLAG(v)) {
            sprintf(ach, "IMEDPI %p", pImeDpi);
            Idso(0, ach);
        }

        Print("\n");

        pImeDpi = ImeDpi.pNext;
    }

    return TRUE;
}

#endif
