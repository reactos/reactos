/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGCB.H
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/



/* Function prototypes
 */
PSZ GetCBMsgName(WORD wMsg);

BOOL FASTCALL    ThunkCBMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL    UnThunkCBMsg16(LPMSGPARAMEX lpmpex);
