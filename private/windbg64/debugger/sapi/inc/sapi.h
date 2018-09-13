//  SAPI.H - Private header file for SAPI
//
//  DESCRIPTION
//      This file contains types that are private to the SAPI project.


#ifndef _SAPI_INCLUDED_
#define _SAPI_INCLUDED_

typedef HIND    HEXR;               // A handle to an EXR (exe reference)
typedef HIND    HEXG;               // A handle to an EXG (exe structure global)
typedef HVOID   HST;                // A handle to source module table
typedef HVOID   HSL;                // A handle to source line table
typedef HVOID   HFL;                // A handle to an instance of a file

#define hmodNull ((HMOD)NULL)
#define hexgNull ((HEXG)NULL)
#define hexrNull ((HEXR)NULL)
#define hexeNull ((HEXE)NULL)
#define hpdsNull ((HPDS)NULL)

#define MDS_INDEX   1L


// The following are defs stolen from CVDEF

#include <stdlib.h>

#define _MAX_CVPATH     _MAX_PATH   // max. length of full pathname
#define _MAX_CVDRIVE    _MAX_DRIVE  // max. length of drive component
#define _MAX_CVDIR      _MAX_DIR    // max. length of path component
#define _MAX_CVFNAME    _MAX_FNAME  // max. length of file name component
#define _MAX_CVEXT      _MAX_EXT    // max. length of extension component

typedef SYMTYPE         *SYMPTR;
typedef CFLAGSYM        *CFLAGPTR;
typedef CONSTSYM        *CONSTPTR;
typedef REGSYM          *REGPTR;
typedef UDTSYM          *UDTPTR;
typedef SEARCHSYM       *SEARCHPTR;
typedef BLOCKSYM16      *BLOCKPTR16;
typedef DATASYM16       *DATAPTR16;
typedef PUBSYM16        *PUBPTR16;
typedef LABELSYM16      *LABELPTR16;
typedef BPRELSYM16      *BPRELPTR16;
typedef PROCSYM16       *PROCPTR16;
typedef THUNKSYM16      *THUNKPTR16;
typedef CEXMSYM16       *CEXMPTR16;
typedef VPATHSYM16      *VPATHPTR16;
typedef WITHSYM16       *WITHPTR16;

typedef BLOCKSYM32      *BLOCKPTR32;
typedef DATASYM32       *DATAPTR32;
typedef PUBSYM32        *PUBPTR32;
typedef LABELSYM32      *LABELPTR32;
typedef BPRELSYM32      *BPRELPTR32;
typedef PROCSYM32       *PROCPTR32;
typedef PROCSYMMIPS     *PROCPTRMIPS;
typedef PROCSYMIA64     *PROCPTRIA64;
typedef THUNKSYM32      *THUNKPTR32;
typedef CEXMSYM32       *CEXMPTR32;
typedef WITHSYM32       *WITHPTR32;
typedef VPATHSYM32      *VPATHPTR32;

typedef BLOCKSYM        *BLOCKPTR;
typedef PROCSYM         *PROCPTR;
typedef THUNKSYM        *THUNKPTR;
typedef WITHSYM         *WITHPTR;

// End of CVDEF defines

typedef struct _PDS {
    HLLI    hlliExe;
    HPID    hpid;
} PDS;  // PiD Struct
typedef PDS *       PPDS;
typedef PDS *   LPPDS;

typedef union _UFOP {
    DWORD   lfo;
    PVOID   lpv;
} UFOP; // Union of long File Offset & Pointer
typedef UFOP *LPUFOP;

typedef struct _ALM {
    BOOL    fSeq;
    WORD    btAlign;
    WORD    cbBlock;
    DWORD   cb;
    LPB     pbData;
    UFOP    rgufop [1];
//  WORD    cbLast;             // After a null terminator, there is a field
                                //  containing the cb of the last align block
} ALM; // ALigned Map
typedef ALM *LPALM;

typedef struct _ULP {
    DWORD   ib;                 // Byte offset into the symbol table
    DWORD   ulId;               // Identified (either a uoff or a checksum)
} ULP;  // ULong Pair
typedef ULP *LPULP;

typedef struct _SHT {
    WORD    HashIndex;
    WORD    ccib;               // count of buckets [0..n)
    DWORD * rgcib;              // count of ULPs in chain i, 0 <= i < n
    DWORD * rgib;               // offset of start of chain i, 0 <= i < n
    LPALM   lpalm;              // block of ULPs
} SHT;  // Symbol Hash Table
typedef SHT *LPSHT;

typedef struct _GST {
    LPALM   lpalm;
    SHT     shtName;
    SHT     shtAddr;
#if CC_CVTYPE32
    SymConvertInfo  sci;
#endif
} GST;  // Global Symbol Table -- Globals, Publics, or Statics
typedef GST *LPGST;

typedef struct _SGC {
    WORD    seg;
    DWORD   off;
    DWORD   cb;
} SGC;  // SeGment Contributer
typedef SGC *LPSGC;

typedef struct _SGE {
    SGC     sgc;
    HMOD    hmod;
} SGE;  // SeGment directory Entry
typedef SGE *LPSGE;

typedef struct _SGD {
    WORD    csge;
    LPSGE   lpsge;
} SGD;  // SeGment Directory
typedef SGD *LPSGD;

typedef struct _MDS {
    HEXG    hexg;               // EXG parent of MDS list
    WORD    imds;

    DWORD   cbSymbols;

    LPB     symbols;
    HST     hst;
    LSZ     name;

    DWORD   ulhst;
    DWORD   cbhst;

    DWORD   ulsym;

    WORD    csgc;
    LPSGC   lpsgc;
    Mod*    pmod;                 // NB10

#if CC_CVTYPE32
    SymConvertInfo  sci;        // necessary info to keep around while
                                // loading symbols that need to be
                                // converted.
#endif
} MDS;  // MoDule Information
typedef MDS *PMDS;
typedef MDS *LPMDS;

struct STAB;
typedef struct STAB STAB;

typedef struct _EXG {
    BOOL    fOmfLoaded:1;
    BOOL    fOmfMissing:1;
    BOOL    fOmfSkipped:1;
    BOOL    fOmfDefered:1;
    BOOL    fOmfLoading:1;
    BOOL    fIsPE:1;
    BOOL    fIsRisc:1;
    BOOL    fSymConverted:1;
    SHE     sheLoadStatus;      // Load status: defered, startup, etc
    SHE     sheLoadError;       // Load error
    LONGLONG llUnload;

    LSZ     lszName;            // File name of exe
    LSZ     lszModule;          // Module name of exe
    LSZ     lszDebug;           // File name for debug info
    LSZ     lszPdbName;         // File name of pdb
    LSZ     lszAltName;         // Alternate name (for KD debugging)
    LPB     lpbData;            // Pointer to raw data for this image (non-PDB)
    LPB     lpgsi;              // GSN Info table
    PVOID   pvSymMappedBase;

    PDB   * ppdb;
    DBI   * pdbi;
    TPI   * ptpi;
    GSI   * pgsiPubs;           // public symbols
    GSI   * pgsiGlobs;          // globals
    LPALM   lpalmTypes;         // Types table
    DWORD   citd;               // Number of types
    DWORD * rgitd;              // Array of pointers to types
    GST     gstPublics;
    GST     gstGlobals;
    GST     gstStatics;
    STAB  * pstabUDTSym;
    WORD    csgd;               // Segment Directory
    USHORT  machine;            // Original Architecture
    LPSGD   lpsgd;
    LPSGE   lpsge;
    DWORD   cMod;               // Count of modules (count of sstModule should = sstFileIndex.cmodules)
    LPB     lpefi;              // Pointer to raw file index (for freeing)
    WORD  * rgiulFile;          // List of beginning index of module
                                //  file lists.
    WORD  * rgculFile;          // List of counts of module file lists
    DWORD * rgichFile;          // Index into string table of file names
    DWORD   cbFileNames;        // Number of bytes in string table of file
                                // names (lpchFileNames)
    LPCH    lpchFileNames;      // String table of file names
    DEBUGDATA   debugData;      // OSDEBUG4 information pdata/omap/fpo
    WORD    cRef;               // Reference count on this image
    LPMDS   rgMod;              // Array of module contributions.
    UOFFSET LoadAddress;        // Bass address for this image
    DWORD   ulTimeStamp;        // Timestamp from the image
    DWORD   ulCheckSum;         // Checksum from the image
    DWORD   ulImageSize;        // Size of image
    WidenTi *   pwti;           // for converting all of this exe's types
                                // and syms to 32-bit type indices
} EXG; // EXe structure Global
typedef EXG *PEXG;
typedef EXG * LPEXG;

typedef struct _EXE {
    HPDS    hpds;               // PID of process
    HEXG    hexg;
    DWORD   timestamp;
    DWORD   TargetMachine;
    BOOL    fIsLoaded;
    UOFFSET   LoadAddress;
    LPDEBUGDATA pDebugData;
} EXE;   // EXE struct
typedef EXE *PEXE;
typedef EXE *LPEXE;

typedef struct _LBS  {
    ADDR        addr;
    HMOD        tagMod;
    SYMPTR      tagLoc;
    SYMPTR      tagLab;
    SYMPTR      tagProc;
    SYMPTR      tagThunk;
    CEXMPTR16   tagModelMin;
    CEXMPTR16   tagModelMax;
} LBS; // LaBel Structure ???
typedef LBS *PLBS;
typedef LBS *LPLBS;

#define NEXTSYM(a,b)    ((a) (((LPB) (b)) + ((SYMPTR) (b))->reclen + 2))

// New Source Line table handling and maintenance

typedef struct _OFP {
    UOFF32  offStart;
    UOFF32  offEnd;
} OFP;  // OFset Pair -- used to maintain start/end offset pairs
typedef OFP *LPOFP;

typedef struct OPT {
    UOFFSET offStart;
    LPOFP   lpofp;
} OPT;  // Offset Pair Table -- used to maintain start/end offset pairs
typedef OPT *LPOPT;

typedef  char *  (* CONVERTPROC) (HANDLE, char *);

typedef struct _LINECACHE {
    HSF   hsf;
    DWORD wLine;
    ADDR  addr;
    SHOFF cbLn;
    DWORD rgw[2];
    BOOL  fRet;
    WORD  rgiLn[2];
} LINECACHE;
extern LINECACHE LineCache;

// hexe <--> hmod map cache
typedef struct _MODCACHE {
    HMOD hmod;
    HEXE hexe;
    HPDS hpds;
} MODCACHE;
extern MODCACHE ModCache;

typedef struct _HSFCACHE {
    HSF     Hsf;
    HMOD    Hmod;
} HSFCACHE;
extern HSFCACHE HsfCache;

typedef struct _CXTCACHE {
    HMOD    hmod;
    HGRP    hgrp;
    HEXE    hexe;
    HPDS    hpds;
    WORD    seg;
    UOFFSET uoffBase;
    UOFFSET uoffLim;
} CXTCACHE;
extern CXTCACHE CxtCache;

typedef struct _SLCACHE {
    char    szFile[ _MAX_CVPATH ];
    HEXE    hexe;
    HMOD    hmod;
    WORD    line;
    LPSLP   lpslp;
    int     cslp;
} SLCACHE;
extern SLCACHE SlCache;

typedef struct _ADDRCACHE {
    ADDR  addr;
    DWORD wLine;
    int   cb;
} ADDRCACHE;
extern ADDRCACHE AddrCache;

//
//  Matching criteria for DLL list
//
typedef enum _MATCH_CRIT {
    MATCH_FULLPATH,             //  Match full path
    MATCH_FILENAME,             //  Match filename
    MATCH_BASENAME              //  Match base name (no extension)
} MATCH_CRIT;

#if 0
INT     FHOpen (LSZ);
#define FHRead(fh,lpb,cb) (SYReadFar(fh, lpb, cb))
#define FHClose(fh)
#define FHSeek(fh,ib) (SYSeek(fh, ib, SEEK_SET))
#define SYError() assert(FALSE)
#endif

#define cbAlign     0x1000
#define cbAlignType 0xC000

#endif      // _SAPI_INCLUDED_
