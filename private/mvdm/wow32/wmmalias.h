/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMALIAS.H
 *  WOW32 16-bit handle alias support
 *
 *  History:
 *  Created Sept-1-1992 by Chandan Chauhan (ChandanC)
 *  Modified 12-May-1992 by Mike Tricker (miketri) to add MultiMedia support
--*/



/* 16-bit handle format
 *
 * Bits 0 and 1 are always zero (potential compatibility requirement).
 * Note however that the macros below treat HASH_BITS as starting at bit
 * 0, for simplicity.  We just shift the alias left two bits when we're
 * done.  The actual number of low bits that are reserved is determined
 * by RES_BITS.
 *
 * Of the remaining 14 bits, the next HASH_BITS bits are the hash slot #
 * (relative to 0), followed by LIST_BITS bits containing the list slot #
 * (relative to 1).  List slot is relative to 1 because some portion of
 * a valid handle must be non-zero;  this is also why LIST_SLOTS contains
 * that extra "-1".
 */
#define RES_BITS                2

#define HASH_BITS       6
#define HASH_SLOTS      (1 << HASH_BITS)
#define HASH_MASK       (HASH_SLOTS-1)
#define MASK32(h32)     ((INT)(h32))
#define HASH32(h32)     (MASK32(h32) & (HASH_SLOTS-1))

#define LIST_BITS               (16-RES_BITS-HASH_BITS)
#define LIST_SLOTS      ((1 << LIST_BITS) - 1)
#define LIST_MASK       (LIST_SLOTS << HASH_BITS)

#define ALIAS_SLOTS             128     // must be a power of 2


/* Class map entry
 */
#pragma pack(2)
typedef struct _WCDM {	     /* wcd */
    struct _WCD *pwcdNext;  // pointer to next wcd entry
    PSZ     pszClass;       // pointer to local copy of class name
    VPSZ    vpszMenu;	    // pointer to original copy of menu name, if any
    HAND16  hModule16;      // handle of owning module
    HAND16  hInst16;        // 16-bit hInstance (wndclass16.hInstance)
    WORD    nWindows;	    // # of windows in existence based on class
    VPWNDPROC vpfnWndProc;  // 16-bit window proc address
    WORD    wStyle;	    // Class Style bits
} WCD, *PWCD, **PPWCD;
#pragma pack()



/* Handle map entry
 */
#pragma pack(2)
typedef struct _HMAP {      /* hm */
    struct _HMAP *phmNext;  // pointer to next hmap entry
    HANDLE  h32;	    // 32-bit handle
    HAND16  h16;            // 16-bit handle
    HTASK16 htask16;        // 16-bit handle of owning task
    INT     iClass;         // WOW class index
    DWORD   dwStyle;        // style flags (if handle to window)
    PWCD    pwcd;           // WOW class data pointer
    VPWNDPROC vpfnWndProc;  // associated 16-bit function address
    VPWNDPROC vpfnDlgProc;  // 16-bit dialog function
} HMAP, *PHMAP, **PPHMAP;
#pragma pack()


/* Handle alias info
 */
typedef struct _HINFO {         /* hi */
    PPHMAP  pphmHash;           // address of hash table
#ifdef NEWALIAS
    PPHMAP  pphmAlias;          // address of alias table
    INT     nAliasEntries;      // size of alias table, in entries
    INT     iAliasHint;         // next (possibly) free slot in alias table
#endif
} HINFO, *PHINFO, **PPHINFO;


PHMAP FindHMap32(HAND32 h32, PHINFO phi, INT iClass);
PHMAP FindHMap16(HAND16 h16, PHINFO phi);
VOID FreeHMap16(HAND16 h16, PHINFO phi);
PSZ GetHMapNameM(PHINFO phi, INT iClass);
