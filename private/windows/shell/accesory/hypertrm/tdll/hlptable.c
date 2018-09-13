/*	File: \WACKER\TDLL\hlptable.c (Created: 4-30-1998)
 *
 *	Copyright 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 4 $
 *	$Date: 5/25/99 8:55a $
 */

#include <windows.h>
#pragma hdrstop

#include "globals.h"
#include "hlptable.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	doContextHelp
 *
 * DESCRIPTION:
  *
 * ARGUMENTS:
 *
 * RETURNS:
 *	BOOL
 *
 */
void doContextHelp(const DWORD aHlpTable[], WPARAM wPar, LPARAM lPar, BOOL bContext, BOOL bForce)
    {

    if ( !bContext )
		{
		if ( isControlinHelpTable( aHlpTable, ((LPHELPINFO)lPar)->iCtrlId ) || bForce )
	        {
            if ( ((LPHELPINFO)lPar)->iCtrlId == IDOK || ((LPHELPINFO)lPar)->iCtrlId == IDCANCEL )
                {
			    WinHelp(((LPHELPINFO)lPar)->hItemHandle,
				    TEXT("windows.hlp"),
				    HELP_WM_HELP,
				    (DWORD_PTR)(LPTSTR)aHlpTable);
                }
            else
                {
			    WinHelp(((LPHELPINFO)lPar)->hItemHandle,
				    glblQueryHelpFileName(),
				    HELP_WM_HELP,
				    (DWORD_PTR)(LPTSTR)aHlpTable);
                }
			}
		}
	else
		{
		if ( isControlinHelpTable( aHlpTable, GetDlgCtrlID((HWND)wPar)) || bForce )
		    {
			if ( GetDlgCtrlID((HWND)wPar) == IDOK || GetDlgCtrlID((HWND)wPar) == IDCANCEL )
				{
				WinHelp((HWND)wPar,
					TEXT("windows.hlp"),
					HELP_CONTEXTMENU,
					(DWORD_PTR)(LPTSTR)aHlpTable);
				}
			else
				{
				if ( GetDlgCtrlID( (HWND)wPar ) )
					{
					WinHelp((HWND)wPar,
						glblQueryHelpFileName(),
						HELP_CONTEXTMENU,
						(DWORD_PTR)(LPTSTR)aHlpTable);
					}
				}
			}
		}
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	isControlinHelpTable
 *
 * DESCRIPTION:
  * Let's us decide whether or not to call WinHelp (HTMLHelp) based on
  * whether or not the specified control is matched to a help id.
  *
 * ARGUMENTS:
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL isControlinHelpTable(const DWORD aHlpTable[], const INT cntrlID)
	{
    INT nLoop;
    BOOL retval = FALSE;

    for(nLoop = 0; aHlpTable[nLoop] != (DWORD)0; nLoop++)
        {
        if ( aHlpTable[nLoop] == (DWORD)cntrlID )
            {
            retval = TRUE;
            break;
            }
        }

    return retval;
    }
