/* $Id: metric.c,v 1.5 2002/09/28 22:13:28 jfilby Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

ULONG STDCALL
NtUserGetSystemMetrics(ULONG Index)
{
  switch (Index)
    {
    case SM_CXSCREEN:
      return(640);
    case SM_CYSCREEN:
      return(480);
    case SM_CXMINTRACK:
      return(100);
    case SM_CYMINTRACK:
      return(28);
    case SM_CXDLGFRAME:
      return(4);
    case SM_CYDLGFRAME:
      return(4);
    case SM_CXFRAME:
      return(5);
    case SM_CYFRAME:
      return(5);
    case SM_CXBORDER:
      return(1);
    case SM_CYBORDER:
      return(1);
    case SM_CXVSCROLL:
      return(17);
    case SM_CYHSCROLL:
      return(17);
    case SM_CYCAPTION:
      return(20);
    case SM_CXSIZE:
    case SM_CYSIZE:
      return(18);
    case SM_CXSMSIZE:
    case SM_CYSMSIZE:
      return(14);
    default:
      return(0xFFFFFFFF);
    }
}

/* EOF */
