//
// bsc.h
//
// interface to browser information in a .PDB file
//

#ifndef __BSC_INCLUDED__
#define __BSC_INCLUDED__

#include <pdb.h>

typedef ULONG  IINST;
typedef ULONG  IREF;
typedef ULONG  IDEF;
typedef USHORT IMOD;

#define irefNil  ((IREF)-1)
#define idefNil  ((IDEF)-1)
#define iinstNil ((IINST)-1)

#define HTARGET ULONG

// The constant IINST value for the "Globals" class
#define IINST_GLOBALS   0xFFFF0001

typedef USHORT LINE;
typedef BYTE   TYP;
typedef USHORT ATR;
typedef ULONG MBF;

enum OPERATION
{
    addOp,
    delOp,
    changeOp,
    refreshAllOp,
    noOp
};

struct IinstInfo
{
    IINST m_iinst;
    SZ_CONST m_szName; // REVIEW: must be deleted (use Ni instead)!
    NI  m_ni; 
};

struct NiQ
{
    IINST m_iinstOld;
    IinstInfo m_iInfoNew;
    OPERATION m_op;
    TYP     m_typ;
};

typedef BOOL (*pfnNotifyChange) (NiQ * rgQ, ULONG cQ, HTARGET hTarget);

#define OUT

interface Bsc
{
    // open by name or by .pdb 
    static  PDBAPI(BOOL) open(PDB* ppdb, OUT Bsc** ppbsc);
    static  PDBAPI(BOOL) open(SZ szName, OUT Bsc** ppbsc);
    virtual BOOL close() pure;

    // primitives for getting the information that underlies a handle
    virtual BOOL iinstInfo(IINST iinst, OUT SZ *psz, OUT TYP *ptyp, OUT ATR *patr) pure;
    virtual BOOL irefInfo(IREF iref, OUT SZ *pszModule, OUT LINE *piline) pure;
    virtual BOOL idefInfo(IDEF idef, OUT SZ *pszModule, OUT LINE *piline) pure;
    virtual BOOL imodInfo(IMOD imod, OUT SZ *pszModule) pure;
    virtual SZ   szFrTyp(TYP typ) pure;
    virtual SZ   szFrAtr(ATR atr) pure;

    // primitives for managing object instances (iinst)
    virtual BOOL getIinstByvalue(SZ sz, TYP typ, ATR atr, OUT IINST *piinst) pure;
    virtual BOOL getOverloadArray(SZ sz, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;    
    virtual BOOL getUsedByArray(IINST iinst, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
    virtual BOOL getUsesArray(IINST iinst, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
    virtual BOOL getBaseArray(IINST iinst, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
    virtual BOOL getDervArray(IINST iinst, OUT IINST **ppiinst, OUT ULONG *pciinst)  pure;
    virtual BOOL getMembersArray(IINST iinst, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;

    // primitives for getting definition and reference information  
    virtual BOOL getDefArray(IINST iinst, OUT IREF **ppidef, OUT ULONG *pciidef) pure;
    virtual BOOL getRefArray(IINST iinst, OUT IREF **ppiref, OUT ULONG *pciiref) pure;

    // primitives for managing source module contents
    virtual BOOL getModuleContents(IMOD imod, MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
    virtual BOOL getModuleByName(SZ sz, OUT IMOD *pimod) pure;
    virtual BOOL getAllModulesArray(OUT IMOD **ppimod, OUT ULONG *pcimod) pure;
    
    // call this when a computed array is no longer required
    virtual void disposeArray(void *pAnyArray) pure;

    // call this to get a pretty form of a decorated name   
    virtual SZ  formatDname(SZ szDecor) pure;
    
    // call this to do category testing on instances
    virtual BOOL fInstFilter(IINST iinst, MBF mbf) pure;

    // primitives for converting index types
    virtual IINST iinstFrIref(IREF) pure;
    virtual IINST iinstFrIdef(IDEF) pure;
    virtual IINST iinstContextIref(IREF) pure;

    // general size information
    virtual BOOL getStatistics(struct BSC_STAT *) pure;
    virtual BOOL getModuleStatistics(IMOD, struct BSC_STAT *) pure;

    // case sensitivity functions
    virtual BOOL fCaseSensitive() pure;
    virtual BOOL setCaseSensitivity(BOOL) pure;

    // handy common queries which can be optimized
    virtual BOOL getAllGlobalsArray(MBF mbf, OUT IINST **ppiinst, OUT ULONG *pciinst) pure;
    virtual BOOL getAllGlobalsArray(MBF mbf, OUT IinstInfo **ppiinstinfo, OUT ULONG *pciinst) pure;

    // needed for no compile browser
    // get parameters (iinst must be a function type)
    virtual SZ  getParams (IINST iinst) pure;
    virtual USHORT getNumParam (IINST iinst) pure;
    virtual SZ getParam (IINST iinst, USHORT index) pure;
    // get return type/variable type
    virtual SZ  getType (IINST iinst) pure;
    // register call back for notification
    // THIS SHOULD BE DELETED SOON!
    virtual BOOL regNotify (pfnNotifyChange pNotify) pure;
    // register to make sure that NCB will create change queue
    virtual BOOL regNotify () pure;
    virtual BOOL getQ (OUT NiQ ** ppQ, OUT ULONG * pcQ) pure;
    virtual BOOL checkParams (IINST iinst, SZ * pszParam, ULONG cParam) pure;
    virtual BOOL fHasMembers (IINST iinst, MBF mbf) pure;
    
    // needed for class view for optimization
    virtual SZ szFrNi (NI ni) pure;
    virtual BOOL niFrIinst (IINST iinst, NI *ni) pure;
    virtual BOOL lock() pure;
    virtual BOOL unlock() pure;
};

struct BSC_STAT
{
    ULONG   cDef;
    ULONG   cRef;
    ULONG   cInst;
    ULONG   cMod;
    ULONG   cUseLink;
    ULONG   cBaseLink;
};

// these are the bit values for Bsc::instInfo()

// this is the type part of the result, it describes what sort of object
// we are talking about.  Note the values are sequential -- the item will
// be exactly one of these things
//

#define INST_TYP_FUNCTION     0x01
#define INST_TYP_LABEL        0x02
#define INST_TYP_PARAMETER    0x03
#define INST_TYP_VARIABLE     0x04
#define INST_TYP_CONSTANT     0x05
#define INST_TYP_MACRO        0x06
#define INST_TYP_TYPEDEF      0x07
#define INST_TYP_STRUCNAM     0x08
#define INST_TYP_ENUMNAM      0x09
#define INST_TYP_ENUMMEM      0x0A
#define INST_TYP_UNIONNAM     0x0B
#define INST_TYP_SEGMENT      0x0C
#define INST_TYP_GROUP        0x0D
#define INST_TYP_PROGRAM      0x0E
#define INST_TYP_CLASSNAM     0x0F
#define INST_TYP_MEMFUNC      0x10
#define INST_TYP_MEMVAR       0x11
#define INST_TYP_INCL         0x12
#define INST_TYP_MSGMAP       0x13
#define INST_TYP_MSGITEM      0x14
#define INST_TYP_DIALOGID     0x15  // dialog ID for MFC
// idl stuff
#define INST_TYP_IDL_ATTR     0x16  // idl attributes are stored as iinst
#define INST_TYP_IDL_COCLASS  0x17
#define INST_TYP_IDL_IFACE    0x18
#define INST_TYP_IDL_DISPIFACE  0x19
#define INST_TYP_IDL_LIBRARY  0x1A
#define INST_TYP_IDL_MODULE   0x1B
#define INST_TYP_IDL_IMPORT   0x1C
#define INST_TYP_IDL_IMPORTLIB  0x1D
#define INST_TYP_IDL_MFCCOMMENT 0x1E // idl interface/dispinterface can have mfc comment
// java stuff
#define INST_TYP_JAVA_IFACE     0x1F // java (NOT COM) interfaces

// these are the attributes values, they describes the storage
// class and/or scope of the instance.  Any combination of the bits
// might be set by some language compiler, but there are some combinations
// that don't make sense.

#define INST_ATR_LOCAL       0x001
#define INST_ATR_STATIC      0x002
#define INST_ATR_SHARED      0x004
#define INST_ATR_NEAR        0x008
#define INST_ATR_COMMON      0x010
#define INST_ATR_DECL_ONLY   0x020
#define INST_ATR_PUBLIC      0x040
#define INST_ATR_NAMED       0x080
#define INST_ATR_MODULE      0x100
#define INST_ATR_VIRTUAL     0x200
#define INST_ATR_PRIVATE     0x400
#define INST_ATR_PROTECT     0x800

#define IMODE_VIRTUAL        0x001
#define IMODE_PRIVATE        0x002
#define IMODE_PUBLIC         0x004
#define IMODE_PROTECT        0x008

#define mbfNil    0
#define mbfVars   1
#define mbfFuncs  2
#define mbfMacros 4
#define mbfTypes  8
#define mbfClass  16
#define mbfIncl   32
#define mbfMsgMap 64
#define mbfDialogID 128
#define mbfLibrary 256
#define mbfAll    1023

// BOB = browser object, general index holder 

typedef ULONG BOB;

#define bobNil 0L

typedef USHORT CLS;

#define clsMod  1
#define clsInst 2
#define clsRef  3
#define clsDef  4

#define BobFrClsIdx(cls, idx)  ((((ULONG)(cls)) << 28) | (idx))
#define ClsOfBob(bob)   (CLS)((bob) >> 28)

#define ImodFrBob(bob)  ((IMOD) ((bob) & 0xfffffffL))
#define IinstFrBob(bob) ((IINST)((bob) & 0xfffffffL))
#define IrefFrBob(bob)  ((IREF) ((bob) & 0xfffffffL))
#define IdefFrBob(bob)  ((IDEF) ((bob) & 0xfffffffL))

#define BobFrMod(x)  (BobFrClsIdx(clsMod,  (x)))
#define BobFrInst(x) (BobFrClsIdx(clsInst, (x)))
#define BobFrRef(x)  (BobFrClsIdx(clsRef,  (x)))
#define BobFrDef(x)  (BobFrClsIdx(clsDef,  (x)))

#endif
