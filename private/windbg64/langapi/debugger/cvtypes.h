//  CVTYPES.H
//
//  This file contains a common set of base type declarations
//  between multiple CodeView projects.  If you touch this in one
//  project be sure to copy it to all other projects as well.

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef CV_PROJECT_BASE_TYPES
#define CV_PROJECT_BASE_TYPES

#if !defined(DOLPHIN)
#include "dbapiver.h"
#endif

#include "types.h"

typedef HANDLE HDEP;
typedef HANDLE HIND;

#if 0

// Phoey.  A Handle is a handle.  Clean up this later.  BryanT - 2/23/96

//  HDEP is a machine dependent size and passes as a general handle.
//  HIND is a machine independent sized handle and is used for things
//      which are passed between machines

#ifdef STRICT
DECLARE_HANDLE(HDEP);
DECLARE_HANDLE(HIND);
#else
typedef HANDLE  HDEP;
typedef HANDLE  HIND;
#endif

#endif

typedef HDEP FAR *  LPHDEP;
typedef HIND FAR *  LPHIND;

// HMEM should be avoided (HDEP should be used instead), but for now we'll
// define it for backwards compatibility.

typedef HDEP    HMEM;
typedef HMEM FAR *  LPHMEM;

// These values are used in the SegType field of the Expression Evaluator's
// TI structure, and as the third parameter to the Symbol Handler's
// SHGetNearestHsym function.

#define EECODE      0x01
#define EEDATA      0x02
#define EEANYSEG    0xFFFF

#ifdef STRICT
DECLARE_HANDLE(HPID);
DECLARE_HANDLE(HTID);
#else
typedef HIND    HPID;
typedef HIND    HTID;
#endif

typedef HPID FAR *LPHPID;
typedef HTID FAR *LPHTID;

typedef ULONG   SEGMENT;    // 32-bit compiler doesn't like "_segment"
typedef USHORT  SEG16;
typedef ULONG   SEG32;
typedef SEGMENT FAR * LPSEGMENT;
typedef ULONG   UOFF32;
typedef UOFF32 FAR * LPUOFF32;
typedef USHORT  UOFF16;
typedef UOFF16 FAR * LPUOFF16;
typedef LONG    OFF32;
typedef OFF32 FAR * LPOFF32;
typedef SHORT   OFF16;
typedef OFF16 FAR * LPOFF16;
typedef ULONGLONG UOFF64;
typedef LONGLONG OFF64;


typedef OFF64   SOFFSET;
typedef UOFF64  UOFFSET;
typedef UOFF64  OFFSET;
typedef SOFFSET FAR * LPSOFFSET;
typedef UOFFSET FAR * LPUOFFSET;
typedef OFFSET FAR * LPOFFSET;


//
//  This is a serious bug, with hard to detect side-effects.
//  We break or we AV.
// 
#ifdef DEBUGVER
// Checked version

#define CVTypes_assert(exp) if (!(exp)) { *((char *) 0) = 0; }

#else // DEBUGVER
// Free build

#define CVTypes_assert(exp) if (!(exp)) { DebugBreak(); }

#endif // DEBUGVER


//  address definitions
//  the address packet is always a 16:32 address.

typedef struct {
    UOFF64      off;
    SEGMENT     seg;
} address_t;

#define SegAddrT(a)   ((a).seg)
#define OffAddrT(a)   ((a).off)

#ifdef __cplusplus
void
__inline
AddrTInit(
    address_t * paddrT,
    SEGMENT segSet,
    UOFF64 offSet
    )
{
    //
    // The offSet MUST be sign extended!!!!!!
    //
    CVTypes_assert(((offSet < 0x0000000080000000) || (0x100000000 <= offSet)));
    SegAddrT(*(paddrT)) = segSet;
    OffAddrT(*(paddrT)) = offSet;
}
#else
//
// The offSet MUST be sign extended!!!!!!
//
#define AddrTInit(paddrT,segSet,offSet)     \
    {                                       \
        UOFF64 off64 = offSet;              \
                                            \
        CVTypes_assert(((offSet < 0x0000000080000000) || (0x100000000 <= offSet)));  \
        SegAddrT(*(paddrT)) = segSet;       \
        OffAddrT(*(paddrT)) = off64;        \
    }
#endif

typedef struct {
    BYTE    fFlat   :1;         // true if address is flat
    BYTE    fOff32  :1;         // true if offset is 32 bits
    BYTE    fIsLI   :1;         // true if segment is linker index
    BYTE    fReal   :1;         // x86: is segment a real mode address
    BYTE    fSql    :1;         // true if execution context is SQL SP
        BYTE    fJava   :1;                     // true if Java addr
    BYTE    unused  :2;         // unused
} memmode_t;

#define MODE_IS_FLAT(m)     ((m).fFlat)
#define MODE_IS_OFF32(m)    ((m).fOff32)
#define MODE_IS_LI(m)       ((m).fIsLI)
#define MODE_IS_REAL(m)     ((m).fReal)

#ifdef __cplusplus
void
__inline
ModeInit(
    memmode_t *pmode,
    BOOL fFlat,
    BOOL fOff32,
    BOOL fLi,
    BOOL fRealSet
    )
{
    MODE_IS_FLAT(*(pmode))    = (BYTE)fFlat;
    MODE_IS_OFF32(*(pmode))   = (BYTE)fOff32;
    MODE_IS_LI(*(pmode))      = (BYTE)fLi;
    MODE_IS_REAL(*(pmode))    = (BYTE)fRealSet;
    pmode->fSql               = FALSE;
    pmode->fJava              = FALSE;
}
#else
#define ModeInit(pmode,fFlat,fOff32,fLi,fRealSet)   \
        {                                           \
            MODE_IS_FLAT(*(pmode))    = (BYTE)fFlat;      \
            MODE_IS_OFF32(*(pmode))   = (BYTE)fOff32;     \
            MODE_IS_LI(*(pmode))      = (BYTE)fLi;        \
            MODE_IS_REAL(*(pmode))    = (BYTE)fRealSet;   \
            (pmode)->fSql             = FALSE;      \
            (pmode)->fJava            = FALSE;      \
        }
#endif

#ifdef STRICT
DECLARE_HANDLE(HEMI);
#else
typedef HIND    HEMI;           // Executable Module Index
#endif

typedef struct ADDR {
    address_t   addr;
    memmode_t   mode;
    union
    {
        DWORDLONG Alignment;
        HEMI        emi;
    };
} ADDR;                     // An address specifier
typedef ADDR *  PADDR;
typedef ADDR *  LPADDR;

#define addrAddr(a)         ((a).addr)
#define emiAddr(a)          ((a).emi)
#define modeAddr(a)         ((a).mode)

#ifdef __cplusplus
void
__inline
AddrInit(
    LPADDR paddr,
    HEMI emiSet,
    SEGMENT segSet,
    UOFF64 offSet,
    BOOL fFlat,
    BOOL fOff32,
    BOOL fLi,
    BOOL fRealSet
    )
{
    //
    // The offSet MUST be sign extended!!!!!!
    //
    AddrTInit( &(addrAddr(*(paddr))), segSet, offSet );
    emiAddr(*(paddr)) = emiSet;
    ModeInit( &(modeAddr(*(paddr))),fFlat,fOff32,fLi,fRealSet);
}
#else
//
// The offSet MUST be sign extended!!!!!!
//
#define AddrInit(paddr,emiSet,segSet,offSet,fFlat,fOff32,fLi,fRealSet)  \
        {                                                               \
            AddrTInit( &(addrAddr(*(paddr))), segSet, offSet );         \
            emiAddr(*(paddr)) = emiSet;                                 \
            ModeInit( &(modeAddr(*(paddr))),fFlat,fOff32,fLi,fRealSet); \
        }
#endif

#define ADDR_IS_FLAT(a)     (MODE_IS_FLAT(modeAddr(a)))
#define ADDR_IS_OFF32(a)    (MODE_IS_OFF32(modeAddr(a)))
#define ADDR_IS_LI(a)       (MODE_IS_LI(modeAddr(a)))
#define ADDR_IS_REAL(a)     (MODE_IS_REAL(modeAddr(a)))

#define ADDRSEG16(a)        {ADDR_IS_FLAT(a) = FALSE; ADDR_IS_OFF32(a) = FALSE;}
#define ADDRSEG32(a)        {ADDR_IS_FLAT(a) = FALSE; ADDR_IS_OFF32(a) = TRUE;}
#define ADDRLIN32(a)        {ADDR_IS_FLAT(a) = TRUE;  ADDR_IS_OFF32(a) = TRUE;}

#define GetAddrSeg(a)       ((a).addr.seg)
#define GetAddrOff(a)       ((a).addr.off)
#define SetAddrSeg(a,s)     ((a)->addr.seg=s)

//
// If it is not sign extended, then AV or at least break
//
// Safe to use with function calls
//
#ifndef _NDEBUG
// Debug version
#define SetAddrOff(a,o)             \
    {                               \
        DWORD64 dw64 = (o);         \
        if (!Is64PtrSE(dw64)) {     \
            *((char *) 0) = 1;      \
        }                           \
        (a)->addr.off = dw64;       \
    }
#else
// Retial version
#define SetAddrOff(a,o)             \
    {                               \
        DWORD64 dw64 = (o);         \
        if (!Is64PtrSE(dw64)) {     \
            DebugBreak();           \
        }                           \
        (a)->addr.off = dw64;       \
    }
#endif

//
// If it is not a 64 bit pointer, automatically sign extend it.
//
// NOT SAFE to use with function calls
//
#define SE_SetAddrOff(a,o)                  \
    {                                       \
        if (sizeof(DWORD64) == sizeof(o)) { \
            SetAddrOff(a, o);               \
        } else {                            \
            (a)->addr.off = SE32To64(o);    \
        }                                   \
    }


// Because an ADDR has some filler areas (in the mode and the address_t),
// it's bad to use memcmp two ADDRs to see if they're equal.  Use this
// macro instead.  (I deliberately left out the test for fAddr32(), because
// I think it's probably not necessary when comparing.)
#ifdef __cplusplus
BOOL
__inline
FAddrsEq(
    ADDR &a1,
    ADDR &a2
    )
{
    return
    (GetAddrOff(a1) == GetAddrOff(a2)) &&
    (GetAddrSeg(a1) == GetAddrSeg(a2)) &&
    (ADDR_IS_LI(a1) == ADDR_IS_LI(a2)) &&
    (emiAddr(a1)    == emiAddr(a2));
}
#else
#define FAddrsEq(a1, a2)                        \
    (                                           \
    GetAddrOff(a1) == GetAddrOff(a2) &&         \
    GetAddrSeg(a1) == GetAddrSeg(a2) &&         \
    ADDR_IS_LI(a1) == ADDR_IS_LI(a2) &&         \
    emiAddr(a1)    == emiAddr(a2)               \
    )
#endif

#if 0 // JLS
//  address definitions
//  the address packet is always a 16:32 address.

typedef struct FRAME {
    SEG16       SS;
    address_t   BP;
    SEG16       DS;
    memmode_t   mode;
    HPID        PID;
    HTID        TID;
    address_t   SLP;        // Static link pointer
} FRAME;
typedef FRAME *PFRAME;

#define addrFrameSS(a)     ((a).SS)
#define addrFrameBP(a)     ((a).BP)
#define GetFrameBPOff(a)   ((a).BP.off)
#define GetFrameBPSeg(a)   ((a).BP.seg)
#define SetFrameBPOff(a,o) ((a).BP.off = o)
#define SetFrameBPSeg(a,s) ((a).BP.seg = s)
#define GetFrameSLPOff(a)   ((a).SLP.off)
#define GetFrameSLPSeg(a)   ((a).SLP.seg)
#define SetFrameSLPOff(a,o) ((a).SLP.off = o)
#define SetFrameSLPSeg(a,s) ((a).SLP.seg = s)
#define FRAMEMODE(a)       ((a).mode)
#define FRAMEPID(a)        ((a).PID)
#define FRAMETID(a)        ((a).TID)

#define FrameFlat(a)       MODE_IS_FLAT((a).mode)
#define FrameOff32(a)      MODE_IS_OFF32((a).mode)
#define FrameReal(a)       MODE_IS_REAL((a).mode)
#else  // JLS
DECLARE_HANDLE(HFRAME);
#endif // JLS

//  A few public types related to the linked list manager

typedef HDEP    HLLI;       // A handle to a linked list
typedef HDEP    HLLE;       // A handle to a linked list entry

typedef void (FAR PASCAL * LPFNKILLNODE)(LPVOID);
typedef int  (FAR PASCAL * LPFNFCMPNODE)(LPVOID, LPVOID, LONG );

typedef DWORD          LLF;    // Linked List Flags
#define llfNull         (LLF)0x0
#define llfAscending    (LLF)0x1
#define llfDescending   (LLF)0x2
#define fCmpLT              (-1)
#define fCmpEQ              (0)
#define fCmpGT              (1)

//  EXPCALL indicates that a function should use whatever calling
//      convention is preferable for exported functions.

#define EXPCALL         __stdcall

// copied from winnt.h:

#ifndef PAGE_NOACCESS

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD            0x100
#define PAGE_NOCACHE          0x200

#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_FREE            0x10000

#define MEM_PRIVATE         0x20000
#define MEM_MAPPED          0x40000
#define MEM_IMAGE         0x1000000

#endif

typedef struct _MEMINFO {
    ADDR    addr;
    ADDR    addrAllocBase;
    UOFFSET uRegionSize;
    DWORD   dwProtect;
    DWORD   dwState;
    DWORD   dwType;
    DWORD   dwAllocationProtect;
} MEMINFO;
typedef MEMINFO FAR * LPMEMINFO;

//  Return values for mtrcEndian -- big or little endian -- which
//  byte is [0] most or least significat byte

enum _END {
    endBig,
    endLittle
};
typedef DWORD END;

//  Return values for mtrcProcessorType

enum _MPT {
    mptix86  = 1,   // Intel X86
    mptm68k  = 2,   // Mac 68K
    mptdaxp  = 3,   // Alpha AXP
    mptmips  = 4,   // MIPS
    mptmppc  = 5,   // Mac PPC
    mptntppc = 6,   // NT PPC
    mptJavaVM10 = 7,        // Java VM
    mptia64  = 20,   // IA64
    mptUnknown
};
typedef DWORD MPT;

#include <dbgver.h>     // For AVS definition and support functions

#endif  // CV_PROJECT_BASE_TYPES
