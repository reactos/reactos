/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMTBL32.H
 *  WOW32 32-bit Message Thunkers
 *
 *  History:
 *  Created 23-Feb-1992 by Chandan Chauhan (ChandanC)
--*/


#define THUNKMSG    1
#define UNTHUNKMSG  0


/* Message dispatch table
 */

extern M32 aw32Msg[];


#ifdef DEBUG_OR_WOWPROFILE
extern INT iMsgMax;
#endif


#ifdef DEBUG
#define WM32UNDOCUMENTED WM32Undocumented
#else
#define WM32UNDOCUMENTED WM32NoThunking
#endif
