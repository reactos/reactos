/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WRES16.H
 *  WOW32 16-bit resource support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


/* Resource table entries
 */
#define RES_ALIASPTR        0x0001  // pbResData is 32-bit alias ptr

#pragma pack(2)
typedef struct _RES {       /* res */
    struct _RES *presNext;  // pointer to next res entry
    HMOD16  hmod16;         // 16-bit handle of owning task
    WORD    wExeVer;        // exe version
    ULONG   flState;        // misc. flags (see RES_*)
    HRESI16 hresinfo16;     // 16-bit handle of resource info
    HRESD16 hresdata16;     // 16-bit handle of resource data
    LPSZ    lpszResType;    // type of resource
    PBYTE   pbResData;      // pointer to copy of converted resource data
} RES, *PRES, **PPRES;
#pragma pack()


/* Function prototypes
 */
PRES    AddRes16(HMOD16 hmod16, WORD wExeVer, HRESI16 hresinfo16, LPSZ lpszType);
VOID    FreeRes16(PRES pres);
VOID    DestroyRes16(HMOD16 hmod16);

PRES    FindResource16(HMOD16 hmod16, LPSZ lpszName, LPSZ lpszType);
PRES    LoadResource16(HMOD16 hmod16, PRES pres);
BOOL    FreeResource16(PRES pres);
LPBYTE  LockResource16(PRES pres);
BOOL    UnlockResource16(PRES pres);
DWORD   SizeofResource16(HMOD16 hmod16, PRES pres);

DWORD   ConvertMenu16(WORD wExeVer, PBYTE pmenu32, VPBYTE vpmenu16, DWORD cb, DWORD cb16);
DWORD   ConvertMenuItems16(WORD wExeVer, PPBYTE ppmenu32, PPBYTE ppmenu16, VPBYTE vpmenu16);
DWORD   ConvertDialog16(PBYTE pdlg32, VPBYTE vpdlg16, DWORD cb, DWORD cb16);
