/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSG16.H
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
 *  Changed 12-May-1992 by Mike Tricker (miketri) Added MultiMedia prototypes
--*/


#define WIN30_MN_MSGMAX      WM_USER+200
#define WIN30_MN_FINDMENUWINDOWFROMPOINT    WIN30_MN_MSGMAX+2   // 0x602

#define WIN30_MN_GETHMENU    WM_USER+2


/* Message number/name association (for debug output only)
 */
#ifdef DEBUG
typedef struct _MSGINFO {   /* mi */
    UINT uMsg;                  // 0x0001 in the high word means "undocumented"
    PSZ  pszMsgName;            // 0x0002 in the high word means "win32-specific"
} MSGINFO, *PMSGINFO;
#endif


/* Function prototypes
 */
#ifdef DEBUG
PSZ     GetWMMsgName(UINT uMsg);
#endif

HWND FASTCALL ThunkMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL UnThunkMsg16(LPMSGPARAMEX lpmpex);
BOOL FASTCALL ThunkWMMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL UnThunkWMMsg16(LPMSGPARAMEX lpmpex);
BOOL FASTCALL ThunkSTMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL UnThunkSTMsg16(LPMSGPARAMEX lpmpex);
BOOL FASTCALL ThunkMNMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL UnThunkMNMsg16(LPMSGPARAMEX lpmpex);


BOOL    ThunkWMGetMinMaxInfo16(VPVOID lParam, LPPOINT *plParamNew);
VOID    UnThunkWMGetMinMaxInfo16(VPVOID lParam, LPPOINT lParamNew);
BOOL    ThunkWMMDICreate16(VPVOID lParam, LPMDICREATESTRUCT *plParamNew );
VOID    UnThunkWMMDICreate16(VPVOID lParam, LPMDICREATESTRUCT lParamNew );
BOOL    FinishThunkingWMCreateMDI16(LONG lParamNew, LPCLIENTCREATESTRUCT lpCCS);
BOOL    FinishThunkingWMCreateMDIChild16(LONG lParamNew, LPMDICREATESTRUCT lpMCS);
#define StartUnThunkingWMCreateMDI16(lParamNew)  

