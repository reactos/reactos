/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDDE.H
 *  WOW32 DDE worker routines.
 *
 *  History:
 *  WOW DDE support designed and developed by ChandanC
 *
--*/

#include "wowclip.h"

typedef struct _DDENODE {
    HAND16  Initiator;
    struct _DDENODE *Next;
} DDENODE, *LPDDENODE;


/* DDE h16 and h32 object alias structure
 */

typedef struct _HDDE {
    struct _HDDE *pDDENext;    // pointer to next hDDE alias
    HAND16  To_hwnd;           // window that will receive this message
    HAND16  From_hwnd;         // window that sent this message
    HAND16  hMem16;            // handle of WOW app allocated 16 bit object
    HANDLE  hMem32;            // handle of WOW allocated 32 bit object
    WORD    DdeMsg;            // message id
    WORD    DdeFormat;         // message format
    WORD    DdeFlags;          // indicates if it is metafile handle
    HAND16  h16;               // original h16 for bad apps doing EXECUTE
} HDDE, *PHDDE;


typedef struct _DDEINFO {
    WORD    Msg;               // message id
    WORD    Format;            // message format
    WORD    Flags;             // indicates if it is metafile handle
    HAND16  h16;               // original h16 for bad apps doing EXECUTE
} DDEINFO, *PDDEINFO;


typedef struct _CPDATA {
    struct _CPDATA *Next;      // pointer to next CopyData alias
    HAND16  To_hwnd;           // window that will receive this message
    HAND16  From_hwnd;         // window that sent this message
    DWORD   Mem16;             // handle of allocated 16 bit object
    DWORD   Mem32;             // handle of allocated 32 bit object
    DWORD   Flags;             // No real structure is complete without flags
} CPDATA, *PCPDATA;


// This is used by GetMessage to thunk a 32 bit message to the 16 bit
// message.

#define FREEDDEML               0x0001
#define DDE_EXECUTE_FREE_H16    0x0001
#define DDE_EXECUTE_FREE_MEM    0x0002
#define DDE_METAFILE            0x0004
#define DDE_PACKET              0x0008


// This flag is used when a 16 bit app sends data using WM_COPYDATA message
//

#define COPYDATA_16             0x0001

/*----------------------------------------------------------------------------
|       DDEDATA structure
|
|       WM_DDE_DATA parameter structure for hData (LOWORD(lParam)).
|       The actual size of this structure depends on the size of
|       the Value array.
|
----------------------------------------------------------------------------*/

typedef struct {
   unsigned short wStatus;
   short    cfFormat;
   HAND16   Value;
} DDEDATA16;

typedef struct {
   unsigned short wStatus;
   short    cfFormat;
   HANDLE   Value;
} DDEDATA32;


VOID    WI32DDEAddInitiator (HAND16 Initiator);
VOID    WI32DDEDeleteInitiator(HAND16 Initiator);
BOOL    WI32DDEInitiate(HAND16 Initiator);
BOOL    DDEDeletehandle(HAND16 h16, HANDLE h32);
HANDLE  DDEFind32(HAND16 h16);
HAND16  DDEFind16(HANDLE h32);
BOOL    DDEAddhandle(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 hMem16, HANDLE hMem32, PDDEINFO pDdeInfo);
HAND16  DDECopyhData16(HAND16 To_hwnd, HAND16 From_hwnd, HANDLE h32, PDDEINFO pDdeInfo);
HANDLE  DDECopyhData32(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 h16, PDDEINFO pDdeInfo);
VOID    W32MarkDDEHandle (HAND16 hMem16);
VOID    W32UnMarkDDEHandle (HAND16 hMem16);
HANDLE  DDEFindPair32(HAND16 To_hwnd, HAND16 From_hwnd, HAND16 hMem16);
HAND16  DDEFindPair16(HAND16 To_hwnd, HAND16 From_hwnd, HANDLE hMem32);
BOOL    W32DDEFreeGlobalMem32 (HANDLE h32);
ULONG FASTCALL WK32WowDdeFreeHandle (PVDMFRAME pFrame);
BOOL    W32DdeFreeHandle16 (HAND16 h16);
PHDDE   DDEFindNode16 (HAND16 h16);
PHDDE   DDEFindNode32 (HANDLE h32);
PHDDE   DDEFindAckNode (HAND16 To_hwnd, HAND16 From_hwnd, HANDLE hMem32);
BOOL    CopyDataAddNode (HAND16 To_hwnd, HAND16 From_hwnd, DWORD Mem16, DWORD Mem32, DWORD Flags);
VPVOID  CopyDataFindData16 (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem);
PCPDATA CopyDataFindData32 (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem);
BOOL    CopyDataDeleteNode (HWND16 To_hwnd, HWND16 From_hwnd, DWORD Mem);
BOOL    DDEIsTargetMSDraw(HAND16 To_hwnd);
HAND16  Copyh32Toh16 (int cb, LPBYTE lpMem32);
HANDLE  Copyh16Toh32 (int cb, LPBYTE lpMem16);
VOID    FixMetafile32To16 (LPMETAFILEPICT lpMemMeta32, LPMETAFILEPICT16 lpMemMeta16);

