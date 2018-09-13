/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGEM.H
 *  WOW32 16-bit message thunks
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/



/* Function prototypes
 */
PSZ GetEMMsgName(WORD wMsg);

BOOL FASTCALL   ThunkEMMsg16(LPMSGPARAMEX lpmpex);
VOID FASTCALL   UnThunkEMMsg16(LPMSGPARAMEX lpmpex);
