/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMSGSBM.H
 *  WOW32 16-bit message thunks for SCROLLBARs
 *
 *  History:
 *  Created 10-Jun-1992 by Bob Day (bobday)
--*/



/* Function prototypes
 */
PSZ     GetSBMMsgName(WORD wMsg);

BOOL FASTCALL   ThunkSBMMsg16(LPMSGPARAMEX lpmpex); 
VOID FASTCALL   UnThunkSBMMsg16(LPMSGPARAMEX lpmpex); 
