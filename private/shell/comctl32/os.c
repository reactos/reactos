#include "ctlspriv.h"
#include <krnlcmn.h>

//========== OS Dependent Code =============================================

/*----------------------------------------------------------
Purpose: This export exists so SHDOCVW can call Kernel32's GetProcessDword,
         which is only exported on Win95.  In addition, it is exported
         by ordinal only.  Since GetProcAddress fails for ordinals
         to KERNEL32 directly, we have SHELL32 implicitly link to
         this export and SHDOCVW calls thru this private API.

Returns: 0 on failure
Cond:    --
*/

DWORD
SHGetProcessDword(
    IN DWORD idProcess,
    IN LONG  iIndex)
{
#ifdef WINNT
    return 0;
#else
    return GetProcessDword(idProcess, iIndex);
#endif
}


