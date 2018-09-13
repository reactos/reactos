/*****************************************************************/
/**				  Microsoft Internet Explorer   				**/
/**		      Copyright (C) Microsoft Corp., 1997   			**/
/*****************************************************************/ 

/* INITSTR -- Initialize string library
 *
 * History:
 *	08/11/97	gregj	Created
 */

#include "npcommon.h"

BOOL fDBCSEnabled = FALSE;

void WINAPI InitStringLibrary(void)
{
    ::fDBCSEnabled = ::GetSystemMetrics(SM_DBCSENABLED);
}
