/* $Id: crtmain.c,v 1.5 2003/07/11 21:58:09 royce Exp $
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 */
#include <windows.h>
//#include <msvcrt/stdlib.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

#ifndef __GNUC__

/* GLOBAL VARIABLES *******************************************************/

int _fltused;


/* FUNCTIONS **************************************************************/

/*
 * @unimplemented
 */
int 
STDCALL
_except_handler3(void)
{
    return 0;
}

/*
 * @unimplemented
 */
int
STDCALL
_local_unwind2(void)
{
    return 0;
}

#else /*__GNUC__*/

#endif /*__GNUC__*/


/*
int __cdecl _allmul(void)
{
    return 0;
}

int __cdecl _allshl(void)
{
    return 0;
}

void __cdecl _chkesp(int value1, int value2)
{
}

int __cdecl _alloca_probe(void)
{
    return 0;
}

int STDCALL _abnormal_termination(void)
{
    return 0;
}

int STDCALL _setjmp(void)
{
    return 0;
}
*/
/*
BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved);

int STDCALL _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    BOOL result;

    //__fileno_init();
    //result = DllMain(hInst, ul_reason_for_call, lpReserved);

    result = DllMain(hInst, DLL_PROCESS_ATTACH, lpReserved);
    

    return (result ? 1 : 0);
}
 */

/* EOF */
