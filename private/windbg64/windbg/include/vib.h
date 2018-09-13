/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/

#ifndef _LCLWN_H
#define _LCLWN_H 1

/************************ Structures And Types *****************************/

typedef enum {
    vibUndefined =  0,          // We don't know what this vib is
    vibSymbol =     1,          // vib is a local
    vibWatch =      2,          // vib is a watch
    vibChild =      3,          // vib is the root of a tree
    vibGeneric =    4,          // vib is a array/structure entry
    vibPointer =    5,          // vib is a pointer
    vibType =       6           // vib is a type
} EVIB;

typedef enum {
    OK = 0,
    INITLOCSTATUS = 100,
    INITWATCHSTATUS = 200,
    CODEVIEWMSG = 1,
    OUTOFMEMORY = 10,
    UNABLETOEXPAND = 20,
    UNABLETOADDWATCH = 30,
    UNABLETODELETEWATCH = 40,
    WATCHNOTOPEN = 50,
    UNABLETOOPEN = 60,
    NONEDITFIELD = 70,
    WATCHSYNTAXERROR = 80
} LTS;

/*
#define OK          0
#define INITLOCSTATUS       ((LTS) 100)
#define INITWATCHSTATUS     ((LTS) 200)
#define CODEVIEWMSG     ((LTS) 1)
#define OUTOFMEMORY     ((LTS) 10)
#define UNABLETOEXPAND      ((LTS) 20)
#define UNABLETOADDWATCH    ((LTS) 30)
#define UNABLETODELETEWATCH ((LTS) 40)
#define WATCHNOTOPEN        ((LTS) 50)
#define UNABLETOOPEN        ((LTS) 60)
#define NONEDITFIELD        ((LTS) 70)
#define WATCHSYNTAXERROR    ((LTS) 80)
*/

#define EXPAND1ST           0x01;

/*
**
*/

#define CLREISACT   0xF0
#define CLREISVAL   0x0F


typedef UCHAR   OC;         // offset of a character within a line
typedef ULONG   VPI;        // vib packet index

typedef struct vib *    PTRVIB;     // a pointer to a vib

/*
**
*/

typedef struct woj {
    int     evalLang;   // language of the expression, not used yet
    USHORT  iFormSpec;  // offset in szExpStr of the format specifer
    USHORT  ErrNbr;
    USHORT  cbLen;
    WORD    hprc;
    char    szExpStr[1];
} WOJ;

typedef WOJ *   PTRWOJ;

/*
**  CIF - Children information block
*/

typedef struct cif {
    PTRVIB      pvibChild;  //  pointer to its children
    HTM             hTMBd;  //  the TM to be expanded.
    } CIF;                  // an expansion info packet

typedef CIF *       PTRCIF; // a pointer to a cif

/*
**  VIBFLAGS
*/

typedef struct vibflags {
    WORD NoData   :1;       // Vib contains no data
    WORD NoBind   :1;       // Vib not bound (Out of Context)
    WORD FuncEval :1;       // Vib uses Function Evaluation
    WORD ExpandMe :1;       // Needs to be expand at next paint
    WORD DlgAdd   :1;       // Dialog Box has added but not done
    WORD DlgDel   :1;       // Dialog Box has deleted but not done
    WORD DlgOrig  :1;       // Dialog Box tracking but not done
} VFLAGS;

/*
**  VIBTEXT
*/

typedef struct vibtext {
    PSZ  pszValueC;         // Value (Current Right Pane)
    PSZ  pszValueP;         // Value (Current Left Pane)
    PSZ  pszFormat;         // Format Override
    HTM  htm;
    BOOL fChanged;          // TRUE if changed on prev step
} VTEXT;

typedef VTEXT * PVTEXT;

/*
**  Variable Information Block (VIB)
**  This structure describes the information about each
**  expression in the watch window.  There is one VIB for every
**  filled line in the window.
*/

typedef struct vib {
    short   cln;        //  nbr lines taken by this an all children
    PTRVIB  pvibParent; //  pointer to the parent
    PTRCIF  pcif;       //  pointer to its child info structure
    PTRVIB  pvibSib;    //  pointer to the next siblings
    EVIB    vibPtr;
    UCHAR   level;      //  The nesting level
    VPI     vibIndex;   //  The index into the parents array
    HPROC   hprocCache; //  hproc bound under
    HBLK    hblkCache;  //  hblk  bound under
    HTM     hTMBd;      //  pointer to the bound TM
    HSYM    hSym;       //  The handle the symbols table
    VFLAGS  flags;      //  Flags word
    PTRWOJ  pwoj;       //  Name Information

    PVTEXT  pvtext;     //  Text Descriptions
    ULONG   cText;      //  Count of Text Descriptions

} VIB;                  // variable info block

/*
**  VIT
*/

typedef struct vit *    PTRVIT; // pointer to a vit

typedef struct vit {
    short   cln;            //  Total number of lines in tree
    PTRVIT *    pvitParent; //  pointer to the parent, allways NULL
    PTRVIB  pvibChild;      //  pointer to its children
    CXF     cxf;            //  the context of the vit
    } VIT;                  // variable info block top


/**************************** Prototypes *********************************/

LPVOID AllocMem(int cb);
LPVOID DuplicateMem(PSTR pOld);
VOID   FreeMem(LPVOID lpv);

PTRVIB PASCAL PvibAlloc(PTRVIB, PTRVIB);
VOID   PASCAL FTAgeVibValues(PTRVIB);
void   PASCAL FTclnUpdateParent( PTRVIB pvibParent, int dcln );
PTRVIB PASCAL FTvibGetAtLine( PTRVIT pvit, ULONG oln );
VOID   PASCAL FTEnsureTextExists( PTRVIB pVib );
LTS    PASCAL FTExpand ( PTRVIT  pvit, ULONG oln );
VOID   PASCAL FTExpandOne( PTRVIB pVib);
VOID   PASCAL FTFreeAllSib( PTRVIB pvib );
BOOL   PASCAL FTVerify(PCXF pcxf, PTRVIB pvib);
int    PASCAL FTMakeWatchEntry(void *, void *, char *);
LTS    PASCAL FTError(LTS);
PSTR   PASCAL FTGetVibNames(PTRVIT, BOOL, PLONG);
PSTR   PASCAL FTGetWatchList(PTRVIT);
PSTR   PASCAL FTGetVibResults(PTRVIT, BOOL, PLONG);
PSTR   PASCAL FTGetVibNameString(PTRVIB pVib);
PSTR   PASCAL FTGetVibResultString(PTRVIT pVit, PTRVIB pVib);
PSTR   PASCAL FTGetVibTypeString(PTRVIB pVib);
BOOL   PASCAL FTGetPanelStatus( PTRVIB pVib, UINT PanelNumber);
PSTR   PASCAL FTGetPanelString( PTRVIT pVit, PTRVIB pVib, UINT PanelNumber);
VOID   PASCAL FTSetWatchList(PTRVIT, PSTR);
PTRVIB PASCAL FTvibGet ( PTRVIB, PTRVIB );
PTRVIB PASCAL FTvibInit( PTRVIB, PTRVIB );
PTRVIB PASCAL FTvibGetAtLine( PTRVIT pvit, ULONG oln );
BOOL   PASCAL FTVerifyNew( PTRVIT pvit, ULONG oln);

BOOL   FTAddWatchVariable(PTRVIT * ppVit, PTRVIB * ppVib, LPSTR lpszWatchVar);

#endif /* _LCLWN_H */

