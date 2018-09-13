#include <excpt.h>
//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include <dde.h>
#include "port1632.h"

/*** this is replaced by the regulat WinMain call

HANDLE APIENTRY MGetInstHandle()
{
    return((HANDLE)NtCurrentPeb()->ImageBaseAddress);
}

*********/


  
/*----------------------------------USER-------------------------------------*/

LPSTR MGetCmdLine() 
{
    LPSTR lpCmdLine, lpT;

    lpCmdLine = GetCommandLine();
        
    // on NT, lpCmdLine's first string includes its own name, remove this
    // to make it exactly like the windows command line.
  
    if (*lpCmdLine) {
        lpT = strchr(lpCmdLine, ' ');   // skip self name
        if (lpT) {
            lpCmdLine = lpT;
            while (*lpCmdLine == ' ') {
                lpCmdLine++;            // skip spaces to end or first cmd
            }
        } else {
            lpCmdLine += strlen(lpCmdLine);   // point to NULL
        }
    }
    return(lpCmdLine);
}

    

BOOL APIENTRY MGetTextExtent(HDC hdc, LPSTR lpstr, INT cnt, INT * pcx, INT * pcy)
{
    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetTextExtentPoint(hdc, lpstr, (DWORD)cnt, & Size);
    if (pcx != NULL)
        *pcx = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);
}

/*-------------------------------------DEV-----------------------------------*/

    
/*-----------------------------------KERNEL----------------------------------*/
