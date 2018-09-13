#ifndef _EMDATA
#define _EMDATA

/*
**  These are handles to structures.  The structures are different in
**  the EM and the DM
*/

typedef HLLE HPRC;
typedef HLLE HTHD;
typedef HLLE HEXD;



#ifdef HOSTDOS32
#define EMENTRY far pascal
#else
#define EMENTRY PASCAL LOADDS
#endif

#define IsSegEqual(seg1,seg2) ( (seg1) == (seg2) )
#if 0 // clash with dolphin types.h
#define FP_SEG(fp) (*((unsigned _far *)&(fp)+1))
#define FP_OFF(fp) (*((unsigned _far *)&(fp)))
#endif

typedef enum {
    statStarted,
    statRunning,
    statStopped,
    statDead
} STAT; // process STATus

typedef enum {
    efpNone,
    efpEmulator,
    efpChip
} EFP; // Existence of Floating Point chip or emulator


// Global Segment Info table
typedef struct _sgf {
    union {
        struct {
            unsigned short  fRead   :1;
            unsigned short  fWrite  :1;
            unsigned short  fExecute:1;
            unsigned short  f32Bit  :1;
            unsigned short  res1    :4;
            unsigned short  fSel    :1;
            unsigned short  fAbs    :1;
            unsigned short  res2    :2;
            unsigned short  fGroup  :1;
            unsigned short  res3    :3;
        } u1;
        struct {
            unsigned short  segAttr :8;
            unsigned short  saAttr  :4;
            unsigned short  misc    :4;
        } u2;
    } u;
} SGF;

typedef struct _sgi {
    SGF                 sgf;        // Segment flags
    unsigned short      iovl;       // Overlay number
    unsigned short      igr;        // Group index
    unsigned short      isgPhy;     // Physical segment index
    unsigned short      isegName;   // Index to segment name
    unsigned short      iclassName; // Index to segment class name
    unsigned long       doffseg;    // Starting offset inside physical segment
    unsigned long       cbSeg;      // Logical segment size
} SGI;

typedef SGI FAR * LPSGI;

typedef struct _GSI {
    unsigned short   csgMax;
    unsigned short   csgLogical;
    SGI              rgsgi[];
} GSI;

typedef GSI FAR * LPGSI;


/*
 *  This structure is used by the EM to describe all of the relevant
 *      information for an executable object.
 *
 */

typedef struct _MDI {
    WORD            mte;        /* Unique handle identifing the DLL      */
    unsigned short  pad0;       /* PAD                                   */
    UOFF64          lpBaseOfDll;/* Base offset of the DLL                */
    DWORD           dwSizeOfDll;
    SEGMENT         StartingSegment;    // Starting segment for real mode

    SEGMENT         CSSel;      /* FLAT CS selector                      */
    SEGMENT         DSSel;      /* FLAT DS selector                      */

    DWORD           cobj;       /* Count of objects (sections) in the DLL */
    OBJD *          rgobjd;     /* Array of object (section) descriptors */

    LPGSI           lpgsi;      /* Symbol handler description of sections */
    LPDEBUGDATA     lpDebug;    /* fpo/pdata/omap info                    */
    HEMI            hemi;       /* Handle to the symbol handle description */
    LPTSTR          lszName;    /* Name of DLL                           */

    UOFF64          lpBaseOfData; /* for adrData */
    NLG             nlg;
    UOFF64          uoffiTls;
    DWORD           isecTLS;
    DWORD           iTls;
    BOOL            fSendNLG:1;
    BOOL            fFlatMode:1; /* Is this dll 0:32 or 16:32 addressing  */
    BOOL            fRealMode:1; /* Is this dll real mode?                */
    BOOL            fOffset32:1; /* Is this dll offset 32?                */
} MDI;  // Module Info
typedef MDI FAR *LPMDI;
typedef HLLE HMDI;

#define hmdiNull ((HMDI) 0)

// cache the tls index in this structure
typedef struct _ITLSCACHE {
    DWORD iTls;
    BOOL  fValid;
    ADDR  addr;
} ITLSCACHE;

typedef struct _PRC {
    HPID hpid;
    PID  pid;

    BOOL fRunning:1;
    BOOL fDmiCache:1;

    DMINFO dmi;                     // debug metric info
    HLLI llthd;                     // list of threads
    HLLI llmdi;                     // list of module info
    UINT cmdlTLS;
    ITLSCACHE iTlsCache;
    HLLI llexc;                     // Exceptions info list
    STAT stat;                      // process status

    SEGMENT     selFlatCs;          // The one and only FLAT code selector
    SEGMENT     selFlatDs;          // The one and only FLAT data selector
} PRC;
typedef PRC  FAR *LPPRC;   // Process information
typedef HPRC FAR *LPHPRC;

#define hprcNull 0
#define hprcInvalid (HPRC)(-1)

/*
 *   The THD structure contains the information which describes the
 *      internal state of a debuggee thread.
 */

//
// For specifying the frame number
// If the frame number is 0, use "regs", not "frameRegs"
//

typedef enum {
    drtNonePresent    =    0,   /* No registers present          */
    drtCntrlPresent   =    1,   /* Control registers are present */
    drtAllPresent     =    2,   /* All registers are present     */
    drtSpecialPresent =    4,   /* Kernel specials are present   */
    drtCntrlDirty     = 0x10,   /* Control registers are dirty   */
    drtAllDirty       = 0x20,   /* Non-control register dirty    */
    drtSpecialDirty   = 0x40,   /* Kernel special regs dirty     */
} DRT; // DiRTy registers

typedef struct _THD {
    HTID    htid;                // WinDbg thread identifier
    TID     tid;                 // System thread identifier
    HPRC    hprc;                // System process identifier
    BOOL    fVirtual:1;          // Thread has been terminated and does
                                 //      not really exist anymore
    BOOL    fFlat:1;             // Current context is in 0:32 bit mode
    BOOL    fOff32:1;            // Current context is 32-bit offset
    BOOL    fReal:1;             // Current context is in real mode
    BOOL    fRunning:1;          // Thread is thought to be executing
    DRT     drt;                 // Thread dirty status flags
    ADDR    addrTls;             // Address of TLS data
    PVOID   regs;                // Last known context of thread

    DWORD   frameNumber;         // frame number
    PVOID   frameRegs;           // Regs for frameNumber

    PKNONVOLATILE_CONTEXT_POINTERS frameRegPtrs;
                                 // Pointers to last place register saved.

    STACKFRAME64 StackFrame;     // Imagehlp stack frame struct

    PVOID   pvSpecial;           // kernel mode data
    DWORD   dwcbSpecial;         // size of special data
    UOFFSET uoffTEB;
    UOFFSET *rguoffTlsBase;      // List of bases (per dll) for tls sections

    PVOID  pvStackRegs;         // Pointer to IA64 stack register data
    DWORD  dwcbStackRegs;       // size of IA64 stack register data
} THD;
typedef THD FAR *LPTHD;          // Thread information
typedef HTHD FAR *LPHTHD;

#define hthdNull    ((HTHD) 0)
#define hthdInvalid ((HTHD)(-1))

/*
 *
 */

#if 0
#pragma pack ( 1 )

typedef struct _EMC {
    WORD        wControl;   // Control word.
    WORD        wStatus;    // Status word.
    WORD        BASstk;     // base of emulator stack
    WORD        CURstk;     // current stack element
    WORD        LIMstk;     // limit of stack
} EMC;  // Emulator control


typedef struct _EME {
    WORD rgwMantissa [ 4 ];
    union {
        WORD wExponent;
        struct {
            WORD wExpPad    : 15;
            WORD bitExpSign : 1;
        } u;
    } u;
    BYTE fSingle : 1;
    BYTE flagpad : 6;
    BYTE fSign   : 1;
    BYTE tag     : 2;
    BYTE tagpad  : 6;
} EME;  // Emulator element

#pragma pack ( )
#endif

typedef enum {
    emdiName,
    emdiEMI,
    emdiMTE,
    emdiBaseAddr,
    emdiNLG
} EMDI;



/*
 *  The following structure is used for doing function evaluation
 */

typedef struct _EXECUTE_OBJECT_EM {
    PVOID       regs;           /* save register area            */
    HTHD        hthd;           /* thread execution is on        */
    HIND        heoDm;          /* execute object from DM        */
} EXECUTE_OBJECT_EM;

typedef EXECUTE_OBJECT_EM FAR * LP_EXECUTE_OBJECT_EM;

enum ModeDisasm
{
    mSpecDisasm,                       // from DM, memory had been read
    mRegularDisasm,                    // from EM, regular
};



//
// flag description structure.  contains a register description
// and the shift count to the rightmost bit in the flag.
//
struct RGFD {
    FD      fd;
    USHORT  iShift;
};


//
// These are the machine-specific functions which must be provided
// in emdp_CPU.cpp to support each CPU.
//

typedef  XOSD   (*PFNGETADDR) (HPID, HTID, ADR, LPADDR);
typedef  XOSD   (*PFNSETADDR) (HPID, HTID, ADR, LPADDR);
typedef  LPVOID (*PFNDOGETREG) (LPVOID, DWORD, LPVOID);
typedef  XOSD   (*PFNGETREGVALUE) (HPID, HTID, DWORD, LPVOID);
typedef  LPVOID (*PFNDOSETREG) (LPVOID, DWORD, LPVOID);
typedef  XOSD   (*PFNSETREGVALUE) (HPID, HTID, DWORD, LPVOID);
typedef  XOSD   (*PFNGETFLAG) (HPID, HTID, DWORD, LPVOID);
typedef  XOSD   (*PFNSETFLAG) (HPID, HTID, DWORD, LPVOID);
typedef  XOSD   (*PFNGETFRAME) (HPID, HTID, DWORD_PTR, DWORD_PTR);
typedef  XOSD   (*PFNGETFRAMEEH) (HPID, HTID, LPEXHDLR *, LPDWORD);
typedef  XOSD   (*PFNUPDATECHILD) (HPID, HTID, DMF);
typedef  VOID   (*PFNADJUSTFORPROLOG) (HPID, HTID, PADDR, CANSTEP *);
typedef  VOID   (*PFNCOPYFRAMEREGS) (LPTHD, LPBPR);
typedef  XOSD   (*PFNGETFUNCTIONINFO) (HPID, LPGFI);

typedef
struct _CPU_POINTERS {

    size_t              SizeOfContext;
    size_t              SizeOfStackRegisters;
    RGFD              * Rgfd;
    RD                * Rgrd;
    DWORD               CRgfd;
    DWORD               CRgrd;

    PFNGETADDR          pfnGetAddr;
    PFNSETADDR          pfnSetAddr;
    PFNDOGETREG         pfnDoGetReg;
    PFNGETREGVALUE      pfnGetRegValue;
    PFNDOSETREG         pfnDoSetReg;
    PFNSETREGVALUE      pfnSetRegValue;
    PFNGETFLAG          pfnGetFlagValue;
    PFNSETFLAG          pfnSetFlagValue;
    PFNGETFRAME         pfnGetFrame;
    PFNGETFRAMEEH       pfnGetFrameEH;
    PFNUPDATECHILD      pfnUpdateChild;
    PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
} CPU_POINTERS;
typedef CPU_POINTERS * PCPU_POINTERS;



extern MASKINFO MaskInfo[];
extern MASKMAP MaskMap;
extern MESSAGEINFO MessageInfo[];
extern MESSAGEMAP MessageMap;

extern LPDBF   lpdbf;
extern LPFNSVC lpfnsvcTL;

extern HLLI llpid;

extern HLLI llprc;

extern HPRC hprcCurr;
extern HPID hpidCurr;
extern PID  pidCurr;
extern PCPU_POINTERS pointersCurr;
extern MPT  mptCurr;

extern HTHD hthdCurr;
extern HTID htidCurr;
extern TID  tidCurr;

#define cbBufferDef 525
extern DWORD cbBuffer;
extern LPDM_MSG LpDmMsg;

extern FNCALLBACKTL CallTL;
extern FNCALLBACKDB CallDB;
extern FNCALLBACKNT CallNT;

#define MAXADDR ((DWORD_PTR)-1)


const LPDEBUGDATA       LpDebugToBeLoaded = (LPDEBUGDATA) -1;
void  VerifyDebugDataLoaded(HPID, HTID, LPMDI);

#endif  // _EMDATA
