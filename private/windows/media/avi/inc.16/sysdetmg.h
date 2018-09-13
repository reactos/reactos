/***MD sysdetmg.h - System Detection Manager definitions
 *
 *  This module contains System Detection Manager definitions including
 *  Detection Module Services definitions and Module Function definitions.
 *
 *  Copyright (c) 1992,1993 Microsoft Corporation
 *  Author:	Michael Tsang (MikeTs)
 *  Created	12/10/92
 *
 *  MODIFICATION HISTORY
 */


#ifndef _INC_SYSDETMG
#define _INC_SYSDETMG


/* do not complain about in-line comment and pragma use in windows.h */
#pragma warning(disable:4001 4103 4705)

#include <sdmerror.h>
#ifdef CALLCM	//only do this hack if we need to call CM
#define WINVER	0x030a		//system detection can be run under win31
#include <windows.h>

// Windows.h defines the following set of things for old reg users, whose
// WINVER is less than 0x0400.	Normally this is not a problem, but SYSDETMG
// is a special DLL whose winver is 0x030a, but we really use Win4.0 reg
// error codes, so we need to pick up the defines in WINERROR.H, so this
// prevents macro redef warnings.

#ifdef ERROR_SUCCESS
#undef ERROR_SUCCESS
#endif
#ifdef ERROR_BADDB
#undef ERROR_BADDB
#endif
#ifdef ERROR_BADKEY
#undef ERROR_BADKEY
#endif
#ifdef ERROR_CANTOPEN
#undef ERROR_CANTOPEN
#endif
#ifdef ERROR_CANTREAD
#undef ERROR_CANTREAD
#endif
#ifdef ERROR_CANTWRITE
#undef ERROR_CANTWRITE
#endif
#ifdef ERROR_INSUFFICIENT_MEMORY
#undef ERROR_INSUFFICIENT_MEMORY
#endif
#ifdef ERROR_INVALID_PARAMETER
#undef ERROR_INVALID_PARAMETER
#endif
#ifdef ERROR_ACCESS_DENIED
#undef ERROR_ACCESS_DENIED
#endif
#ifdef HKEY_CLASSES_ROOT
#undef HKEY_CLASSES_ROOT
#endif
#ifndef REG_BINARY
#define REG_BINARY		0x0003
#endif
#ifndef HKEY_CURRENT_CONFIG
#define HKEY_CURRENT_CONFIG	((HKEY)0x80000005)
#endif
#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE	((HKEY)0x80000002)
#endif
#else	//ifdef SYSDETMG
#include <windows.h>
#endif

#include <winerror.h>
#define NOPRSHT 	//do not include prsht.h
#include <setupx.h>


/*** Miscellaneou macros
 */

#define BYTEOF(d,i)	(((BYTE *)&(d))[i])
#define WORDOF(d,i)	(((WORD *)&(d))[i])
#define LOCAL		PASCAL FAR
#define LOCALC		CDECL FAR
#define DLLENTRY	_loadds WINAPI
#define DEREF(x)	((x) = (x))
#define ALLOC(n)	((VOID FAR *)GlobalAllocPtr(GHND, (n)))
#define FREE(p) 	GlobalFreePtr((p))
#ifdef ERRMSG
  #define CATMSG(p)	CatMsg p
  #define CATERR(rc,p)	{if (rc) CatMsg p;}
#else
  #define CATMSG(p)
  #define CATERR(rc,p)
#endif
#ifdef DEBUG
  #define ENTER(p)	EnterProc p
  #define EXIT(p)	ExitProc p
  #define PRINTTRACE(p) PrintTrace p
#else
  #define ENTER(p)
  #define EXIT(p)
  #define PRINTTRACE(p)
#endif
#define CODESEG 	_based(_segname("_CODE"))


/*** Implementation constants
 */

#define MAX_PATHNAME_LEN	63	//max. length of path name
#define MAX_CLASSNAME_LEN	15	//max. length of device class name
#define MAX_FUNCNAME_LEN	31	//max. length of function name
#define MAX_DEVNAME_LEN 	15	//max. length of device name
#define MAX_INSTNAME_LEN	15	//device instance name length
#define MAX_DOSDEVNAME_LEN	8	//dos device name length
#define MAX_PARAMLINE_LEN	63	//TSR parameter line length
#define MAX_DESC_LEN		63	//max. description length


/*** Callback function error
 */

#define DCBERR_NONE		0x00000000	//no error
#define DCBERR_SKIP		0x80000001	//skip detection function
#define DCBERR_ABORT		0x80000002	//abort detection


/*** Other constants
 */

#define STR_INFNAME_MSDETINF	"msdet.inf"	//main detection INF name

//dwfDetOpen flags
#define DOF_CUSTOM		0x00000001	//custom detection
#define DOF_NORISK		0x00000002	//no risk detection mode
#define DOF_CLEANREG		0x00000004	//clean hw from registry
#define DOF_QUIET		0x00000008	//don't show progress bar
#define DOF_VERBOSE		0x00000010	//detection progress dialog
#define DOF_NORECOVER		0x00000020	//no recover from last crash
#define DOF_MAXCALLBACK 	0x00000040	//maximum callback
#define DOF_PROMPTBEFORE	0x00000080	//prompt before detect
#define DOF_PROGRESSCALLBACK	0x00000100	//do progress callback
#define DOF_INSETUP		0x00000200	//called by Setup
#define DOF_LOGPERFORMANCE	0x00000400	//enable performance logging
#define DOF_ERRORPOPUP		0x00008000	//enable error message box

//dwfDetect flags
#define DETF_NORISK		0x00010000	//no risk detection
#define DETF_VERIFY		0x00020000	//verify mode

//dwCallBackContext
#define CBC_DEVDETECTED 	1	//device detected
#define CBC_REPORTERR		2	//report error
#define CBC_QUERYRES		3	//DMSQueryIOMem has been called
#define CBC_DETECTDONE		4	//detection done
#define CBC_VERIFYDANGER	5	//verifying old danger entry
#define CBC_NEWDANGER		6	//creating new danger entry
#define CBC_DISCARDCRASH	7	//discarding a crash entry
#define CBC_VERIFYDONE		8	//finish verifying devices
#define CBC_BEGINVERIFY 	9	//begin verify
#define CBC_VERIFYPROGRESS	10	//verify progress
#define CBC_BEGINDETECT 	11	//begin detection
#define CBC_DETECTPROGRESS	12	//detection progress
#define CBC_DETECTING		13	//just above to detect a device
#define CBC_DISCARDDANGER	14	//discard a danger entry
#define CBC_SKIPCRASHFUNC	15	//skip a crash function
#define CBC_DMSWRITELOG 	16	//detection module log entry
#define CBC_PERFORMANCE 	17	//log detection performance data

//dwfSearch flags
#define MSF_REALADDR		0x00000001	//real mode address
#define MSF_IGNORECASE		0x00000002	//case insensitive search

//dwResType values
#define RESTYPE_IO		1		//I/O resource
#define RESTYPE_MEM		2		//memory resource
#define RESTYPE_IRQ		3		//irq resource
#define RESTYPE_DMA		4		//dma resource

//Return values of DMSQueryIOMem or DMSQueryIRQDMA
#define RES_NOMATCH	0	//resources have no owner
#define RES_OVERLAP	1	//resources overlap with existing owner
#define RES_MATCH	2	//resources match with existing owner
#define RES_SHARED	3	//resources are shareable by the owner
#define RES_SUPERSET	4	//resources are superset of existing owner


/*** Function type definitions
 */

typedef LONG (DLLENTRY *LPFNDET)(HDET, DWORD, DWORD);
typedef LONG (FAR PASCAL _loadds *LPFNDCB)(DWORD, LPSTR, DWORD);
typedef VOID (FAR PASCAL _loadds *LPFNICB)(DWORD);
typedef VOID (FAR PASCAL *LPFNGEN)();
typedef DWORD (FAR PASCAL _loadds *LPFNPROC)();


/*** Structure and related definitions
 */

#define HANDLE_NULL	0	//null handle
typedef DWORD HDET;		//detection handle
typedef DWORD HDEV;		//device handle
typedef union _REGS FAR *LPREGS;
typedef struct _SREGS FAR *LPSREGS;

#define SYSENVF_EISASYSTEM	0x00000001
#define SYSENVF_MCASYSTEM	0x00000002

#define MACHINFO_MCABUS 	0x02000000	//machine has MCA bus
#define MACHINFO_EXTBIOSAREA	0x04000000	//extended BIOS area allocated
#define MACHINFO_WAITEXTEVENT	0x08000000	//wait ext. event supported
#define MACHINFO_INT154FCALLOUT 0x10000000	//int15/4f callout at int09
#define MACHINFO_CMOSRTC	0x20000000	//CMOS/RTC installed
#define MACHINFO_PIC2		0x40000000	//2nd PIC
#define MACHINFO_HDDMA3 	0x80000000	//hard disk BIOS using DMA3

typedef struct sysenv_s
{
    DWORD dwSDMVersion; 			//byte 0,1=build number
						//byte 2=version minor
						//byte 3=version major
    DWORD dwWinVer;				//byte 0=winver minor
						//byte 1=winver major
						//byte 2=dosver minor
						//byte 3=dosver major
    DWORD dwWinFlags;				//WinFlags from GetWinFlags
    DWORD dwMachineInfo;			//byte 0=model
						//byte 1=sub-model
						//byte 2=BIOS revision
						//byte 3=features
    DWORD dwfSysEnv;				//system environment flags
    char szDetPath[MAX_PATHNAME_LEN + 1];	//detection path string
} SYSENV;

typedef SYSENV *PSYSENV;
typedef SYSENV FAR *LPSYSENV;

typedef struct resinfo_s
{
    int icIO;		//number of I/O resource regions
    int ioffsetIO;	//offset of I/O resource array
    int icMem;		//number of memory resource regions
    int ioffsetMem;	//offset of memory resource array
    int icIRQ;		//number of IRQs
    int ioffsetIRQ;	//offset of IRQ resource array
    int icDMA;		//number of DMAs
    int ioffsetDMA;	//offset of DMA resource array
    int icbResBuff;	//resource buffer size that follows
			//  IOMEM and/or IRQDMA array follows here
} RESINFO;

typedef RESINFO *PRESINFO;
typedef RESINFO FAR *LPRESINFO;

typedef struct ownerinfo_s
{
    char szClassName[MAX_CLASSNAME_LEN + 1];	//owner's class name
    char szDevName[MAX_DEVNAME_LEN + 1];	//owner's device name
    HDEV hdevOwner;				//owner's device handle
    LPRESINFO lpresinfo;			//resource info.
} OWNERINFO;

typedef OWNERINFO *POWNERINFO;
typedef OWNERINFO FAR *LPOWNERINFO;

typedef struct iomem_s
{
    DWORD dwStartAddr;		//region starting address
    DWORD dwEndAddr;		//region ending address
    DWORD dwDecodeMask; 	//decode mask (don't care aliases)
    DWORD dwAliasMask;		//alias mask (used aliases)
    DWORD dwResAttr;		//region attributes
} IOMEM;

typedef IOMEM *PIOMEM;
typedef IOMEM FAR *LPIOMEM;

typedef struct irqdma_s
{
    DWORD dwResNum;		//IRQ or DMA number
    DWORD dwResAttr;		//attributes for this IRQ or DMA
} IRQDMA;

typedef IRQDMA *PIRQDMA;
typedef IRQDMA FAR *LPIRQDMA;

//dwfDev flags
#define DEVF_CHARDEV	0x00000001	//lpstrDevName is a char dev name

typedef struct dosdev_s
{
    char szFileName[MAX_DOSDEVNAME_LEN + 1];//driver filename to query
    char szDevName[MAX_DOSDEVNAME_LEN + 1];//to hold device name
    WORD wfDevAttr;			//to hold device attribute
    WORD wcUnits;			//to hold number of block dev units
    WORD wbitIRQs;			//to hold IRQ bit vector used by dev.
    DWORD dwDevHdrPtr;			//to hold pointer to device header
    DWORD dwNextDevHdrPtr;		//to hold pointer to next in chain
} DOSDEV;

typedef DOSDEV *PDOSDEV;
typedef DOSDEV FAR *LPDOSDEV;

typedef struct dostsr_s
{
    char szPathName[MAX_PATH_LEN + 1];	//to hold the TSR full path name
    char szMCBOwner[9];
    WORD segTSRPSP;			//to hold TSR's segment address
    WORD wcparaTSRSize; 		//to hold TSR's size in paragrahs
    WORD segParentPSP;
    WORD wbitIRQs;			//to hold IRQ bit vector used by TSR
    char szParamLine[MAX_PARAMLINE_LEN + 1];//to hold TSR's parameter line
    DWORD dwNextMCBPtr; 		//to hold the seg addr of next MCB
} DOSTSR;

typedef DOSTSR *PDOSTSR;
typedef DOSTSR FAR *LPDOSTSR;

#define MAX_MCA_SLOTS		8

/*** EISA related stuff
 */

#define MAX_EISAID_LEN		7
#define MAX_EISA_SLOTS		16
#define MAX_IOCONFIGS		20
#define MAX_MEMCONFIGS		9
#define MAX_IRQCONFIGS		7
#define MAX_DMACONFIGS		4

#define IDSLOT_DUPID		0x0080
#define IDSLOT_NOREADID 	0x0040
#define IDSLOT_SLOTTYPEMASK	0x0030
#define IDSLOT_EXPANSLOT	0x0000
#define IDSLOT_EMBEDSLOT	0x0010
#define IDSLOT_VIRTSLOT 	0x0020
#define IDSLOT_DUPCFGIDMASK	0x000f
#define IDSLOT_INCOMPLETECONFIG 0x8000
#define IDSLOT_SUPPORTIOCHKERR	0x0200
#define IDSLOT_SUPPORTENABLE	0x0100

#define FUNCINFO_FUNCDISABLED	0x80
#define FUNCINFO_FREEFORMDATA	0x40
#define FUNCINFO_IOINITENTRIES	0x20
#define FUNCINFO_IORANGEENTRIES 0x10
#define FUNCINFO_DMAENTRIES	0x08
#define FUNCINFO_IRQENTRIES	0x04
#define FUNCINFO_MEMENTRIES	0x02
#define FUNCINFO_TYPEENTRY	0x01

#define PORTINFO_MOREENTRIES	0x80
#define PORTINFO_SHARED 	0x40
#define PORTINFO_NUMPORTMASK	0x1f

#define MEMCFG_MOREENTRIES	0x80
#define MEMCFG_SHARED		0x20
#define MEMCFG_MEMTYPEMASK	0x18
#define MEMCFG_CACHED		0x02
#define MEMCFG_READWRITE	0x01

#define MEMSIZ_DECODEMASK	0x0c
#define MEMSIZ_DECODE20BIT	0x00
#define MEMSIZ_DECODE24BIT	0x04
#define MEMSIZ_DECODE32BIT	0x08

#define IRQCFG_MOREENTRIES	0x80
#define IRQCFG_SHARED		0x40
#define IRQCFG_LEVELTRIGGERED	0x20
#define IRQCFG_INTNUMMASK	0x0f

#define DMACFG_MOREENTRIES	0x0080
#define DMACFG_SHARED		0x0040
#define DMACFG_DMANUMMASK	0x0007
#define DMACFG_TIMINGMASK	0x3000
#define DMACFG_XFERSIZEMASK	0x0c00


#pragma pack(1)
typedef struct memconfig_s
{
    BYTE  bMemConfig;
    BYTE  bMemDataSize;
    BYTE  bStartAddrLo; 	//divided by 0x100
    WORD  wStartAddrHi;
    WORD  wMemSize;		//divided by 0x400
} MEMCONFIG;


typedef struct ioconfig_s
{
    BYTE  bPortInfo;
    WORD  wStartPort;
} IOCONFIG;


typedef struct initdata_s
{
    BYTE  bInitType;
    WORD  wPortAddr;
} INITDATA;


typedef struct eisaconfig_s
{
    DWORD dwEISAID;
    WORD  wIDSlotInfo;
    BYTE  bMajorRev;
    BYTE  bMinorRev;
    BYTE  abSelections[26];
    BYTE  bFuncInfo;
    char  achTypeInfo[80];
    MEMCONFIG amemconfig[MAX_MEMCONFIGS];
    WORD  awIRQConfig[MAX_IRQCONFIGS];
    WORD  awDMAConfig[MAX_DMACONFIGS];
    IOCONFIG aioconfig[MAX_IOCONFIGS];
    INITDATA ainitdata[20];
} EISACONFIG;

typedef EISACONFIG FAR *LPEISACONFIG;


/*** DPMI call structure
 */

typedef struct dwregs_s
{
    DWORD   edi;
    DWORD   esi;
    DWORD   ebp;
    DWORD   rmdw1;
    DWORD   ebx;
    DWORD   edx;
    DWORD   ecx;
    DWORD   eax;
} DWREGS;

typedef struct wregs_s
{
    WORD    di;
    WORD    rmw1;
    WORD    si;
    WORD    rmw2;
    WORD    bp;
    WORD    rmw3;
    DWORD   rmw4;
    WORD    bx;
    WORD    rmw5;
    WORD    dx;
    WORD    rmw6;
    WORD    cx;
    WORD    rmw7;
    WORD    ax;
} WREGS;

typedef struct bregs_s
{
    DWORD   rmb1[4];
    BYTE    bl;
    BYTE    bh;
    WORD    rmb2;
    BYTE    dl;
    BYTE    dh;
    WORD    rmb3;
    BYTE    cl;
    BYTE    ch;
    WORD    rmb4;
    BYTE    al;
    BYTE    ah;
} BREGS;

typedef struct rmcs_s
{
    union
    {
	DWREGS	dw;
	WREGS	w;
	BREGS	b;
    }	    regs;
    WORD    flags;
    WORD    es;
    WORD    ds;
    WORD    fs;
    WORD    gs;
    WORD    ip;
    WORD    cs;
    WORD    sp;
    WORD    ss;
} RMCS, FAR *LPRMCS;
#pragma pack()


/*** SDS Services prototypes
 */

LONG DLLENTRY SDSOpen(HWND hwnd, LPCSTR lpstrDetPath, WORD wfDetOpen,
		      LPFNDCB lpfnCallBack, LPSTR lpstrParams);
LONG DLLENTRY SDSClose(VOID);
LONG DLLENTRY SDSDetect(LPSTR lpstrClass, LPSTR lpstrFunc, WORD wfDetect,
			DWORD dwDetParam);
LONG DLLENTRY SDSRegAvoidRes(int icIO, LPIOMEM lpaio,
			     int icMem, LPIOMEM lpamem,
			     int icIRQ, LPIRQDMA lpairq,
			     int icDMA, LPIRQDMA lpadma);
VOID DLLENTRY SDSGetErrMsg(LONG lErr, LPSTR lpstrBuff, int icbLen);


/*** DMS Services prototypes
 */

VOID _loadds FAR CDECL CatMsg(LPCSTR lpstrFormat, ...);
VOID _loadds FAR CDECL EnterProc(int iTraceLevel, LPCSTR lpstrFormat, ...);
VOID _loadds FAR CDECL ExitProc(int iTraceLevel, LPCSTR lpstrFormat, ...);
VOID _loadds FAR CDECL PrintTrace(int iTraceLevel, LPCSTR lpstrFormat, ...);
LONG DLLENTRY DMSQueryIOMem(HDET hdet, int iResType, int icEntries,
			    LPIOMEM lpaiomem, LPOWNERINFO lpownerinfo);
LONG DLLENTRY DMSQueryIRQDMA(HDET hdet, int iResType, int icEntries,
			     LPIRQDMA lpairqdma, LPOWNERINFO lpownerinfo);
LONG DLLENTRY DMSRegHW(HDET hdet, LPSTR lpstrHWName,
		       HKEY FAR *lphkHW, HINF FAR *lphinfHW,
		       int icIO, LPIOMEM lpaio,
		       int icMem, LPIOMEM lpamem,
		       int icIRQ, LPIRQDMA lpairq,
		       int icDMA, LPIRQDMA lpadma,
		       WORD wfRegHW);
int DLLENTRY DMSInp(unsigned uPort);
unsigned DLLENTRY DMSInpw(unsigned uPort);
DWORD DLLENTRY DMSInpdw(unsigned uPort);
int DLLENTRY DMSOutp(unsigned uPort, int iData);
unsigned DLLENTRY DMSOutpw(unsigned uPort, unsigned uData);
DWORD DLLENTRY DMSOutpdw(unsigned uPort, DWORD dwData);
int DLLENTRY DMSDetectIRQ(unsigned uIRQMask, LPFNICB lpfnIntOn,
			  LPFNICB lpfnIntOff, DWORD dwParam);
BOOL DLLENTRY DMSTimeout(DWORD dwcTicks);
VOID DLLENTRY DMSDelayTicks(DWORD dwcTicks);
LPBYTE DLLENTRY DMSGetMemAlias(DWORD dwRealMemAddr, DWORD dwcbSize);
VOID DLLENTRY DMSFreeMemAlias(LPBYTE lpbMemAlias);
LPBYTE DLLENTRY DMSFindMemStr(LPBYTE lpbAddr, DWORD dwcbSize, LPCSTR lpstr,
			      WORD wfSearch);
LONG DLLENTRY DMSQueryDosDev(DWORD dwDevHdrPtr, LPCSTR lpstrDevName,
			     WORD wfDev, LPDOSDEV lpdosdev);
LONG DLLENTRY DMSQueryDosTSR(DWORD dwMCBPtr, LPSTR lpstrTSRName,
			     LPDOSTSR lpdostsr);
VOID DLLENTRY DMSQuerySysEnv(LPSYSENV lpsysenv);
LONG DLLENTRY DMSGetIHVEISADevSlots(LPCSTR lpstrIHVID);
LONG DLLENTRY DMSGetSlotEISAID(int iSlot, LPSTR lpstrDevID);
LONG DLLENTRY DMSGetEISAFuncConfig(int iSlot, int iFunc,
				   LPEISACONFIG lpcfg, LPSTR lpstrEISAID);
BOOL DLLENTRY DMSGetEISACardConfig(int iSlot, LPSTR lpstrDevID,
				   int FAR *lpicIO, LPIOMEM lpaio,
				   int FAR *lpicMem, LPIOMEM lpamem,
				   int FAR *lpicIRQ, LPIRQDMA lpairq,
				   int FAR *lpicDMA, LPIRQDMA lpadma,
				   WORD wcbTypeBuff, LPSTR lpstrTypeBuff);
LONG DLLENTRY DMSGetMCADevSlots(WORD wMCAID);
LONG DLLENTRY DMSGetSlotMCAID(int iSlot);
int DLLENTRY DMSInt86x(int iIntNum, LPREGS lpregsIn, LPREGS lpregsOut,
		       LPSREGS lpsregs);
BOOL DLLENTRY DMSQueryVerifyState(HDET hdet);
LPSTR DLLENTRY DMSCatPath(LPSTR lpstrPath, LPSTR lpstrName);
LPSTR DLLENTRY DMSGetWinDir(LPSTR lpstrWinDir, int icbBuffLen);
LPFNPROC DLLENTRY DMSRegRing0Proc(LPFNPROC lpfnR3Proc, int icwArg);
VOID DLLENTRY DMSFreeRing0Proc(LPFNPROC lpfnR0Proc);
LONG DLLENTRY DMSWriteLog(LPSTR lpstrMsg);


/*** Module function error codes
 */

#define MODERR_NONE		0L		//no error
#define MODERR_SDMERR		0x80008001	//sysdetmg error
#define MODERR_REGERR		0x80008002	//cannot access registry
#define MODERR_UNRECOVERABLE	0x80000003	//unrecoverable error

#endif	//_INC_SYSDETMG
