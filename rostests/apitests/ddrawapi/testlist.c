#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawapi.h"
void dump_ddrawi_directdraw_int(LPDDRAWI_DIRECTDRAW_INT lpDraw_int);
void dump_ddrawi_directdraw_lcl(LPDDRAWI_DIRECTDRAW_LCL lpDraw_lcl);

/* dump all data struct when this is trun onm usefull when u debug ddraw.dll */
#define DUMP_ON 1

/* include the tests */
#include "tests/Test_DirectDrawCreateEx.c"








/* The List of tests */
TESTENTRY TestList[] =
{
    { L"DirectDrawCreateEx", Test_DirectDrawCreateEx }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
    return sizeof(TestList) / sizeof(TESTENTRY);
}

/* old debug macro and dump data */


void dump_ddrawi_directdraw_int(LPDDRAWI_DIRECTDRAW_INT lpDraw_int)
{
    printf("%08lx LPVOID                        pDirectDraw->lpVtbl      : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpVtbl), lpDraw_int->lpVtbl);
    printf("%08lx LPDDRAWI_DIRECTDRAW_LCL       pDirectDraw->lpLcl       : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLcl), lpDraw_int->lpLcl );
    printf("%08lx LPDDRAWI_DIRECTDRAW_INT       pDirectDraw->lpLink      : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLink), lpDraw_int->lpLink );
    printf("%08lx DWORD                         pDirectDraw->dwIntRefCnt : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, dwIntRefCnt), lpDraw_int->dwIntRefCnt );
}


void dump_ddrawi_directdraw_lcl(LPDDRAWI_DIRECTDRAW_LCL lpDraw_lcl)
{
    printf("%08lx DWORD                         lpLcl->lpDDMore          : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDMore), lpDraw_lcl->lpDDMore);
    printf("%08lx LPDDRAWI_DIRECTDRAW_GBL       lpLcl->lpGbl             : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGbl), lpDraw_lcl->lpGbl);
    printf("%08lx DWORD                         lpLcl->dwUnused0         : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwUnused0), lpDraw_lcl->dwUnused0);
    printf("%08lx DWORD                         lpLcl->dwLocalFlags      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalFlags), lpDraw_lcl->dwLocalFlags);
    printf("%08lx DWORD                         lpLcl->dwLocalRefCnt     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalRefCnt), lpDraw_lcl->dwLocalRefCnt);
    printf("%08lx DWORD                         lpLcl->dwProcessId       : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwProcessId), lpDraw_lcl->dwProcessId);
    printf("%08lx PVOID                         lpLcl->pUnkOuter         : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pUnkOuter), lpDraw_lcl->pUnkOuter);
    printf("%08lx DWORD                         lpLcl->dwObsolete1       : 0x%08lx\n",  FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwObsolete1), lpDraw_lcl->dwObsolete1);   
    printf("%08lx ULONG_PTR                     lpLcl->hWnd              : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWnd), (PVOID)lpDraw_lcl->hWnd);
    printf("%08lx ULONG_PTR                     lpLcl->hDC               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDC), (PVOID) lpDraw_lcl->hDC);
    printf("%08lx DWORD                         lpLcl->dwErrorMode       : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwErrorMode), lpDraw_lcl->dwErrorMode);    
    printf("%08lx LPDDRAWI_DDRAWSURFACE_INT     lpLcl->lpPrimary         : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpPrimary), lpDraw_lcl->lpPrimary);
    printf("%08lx LPDDRAWI_DDRAWSURFACE_INT     lpLcl->lpCB              : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpCB), lpDraw_lcl->lpCB);
    printf("%08lx DWORD                         lpLcl->dwPreferredMode   : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwPreferredMode), lpDraw_lcl->dwPreferredMode);
    printf("%08lx HINSTANCE                     lpLcl->hD3DInstance      : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hD3DInstance), lpDraw_lcl->hD3DInstance);
    printf("%08lx PVOID                         lpLcl->pD3DIUnknown      : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pD3DIUnknown), (PVOID) lpDraw_lcl->pD3DIUnknown);
    printf("%08lx LPDDHAL_CALLBACKS             lpLcl->lpDDCB            : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDCB), lpDraw_lcl->lpDDCB);
    printf("%08lx ULONG_PTR                     lpLcl->hDDVxd            : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDDVxd), (PVOID) lpDraw_lcl->hDDVxd);
    printf("%08lx DWORD                         lpLcl->dwAppHackFlags    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwAppHackFlags), lpDraw_lcl->dwAppHackFlags);
    printf("%08lx ULONG_PTR                     lpLcl->hFocusWnd         : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hFocusWnd), (PVOID) lpDraw_lcl->hFocusWnd);
    printf("%08lx DWORD                         lpLcl->dwHotTracking     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwHotTracking), lpDraw_lcl->dwHotTracking);
    printf("%08lx DWORD                         lpLcl->dwIMEState        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwIMEState), lpDraw_lcl->dwIMEState);
    printf("%08lx ULONG_PTR                     lpLcl->hWndPopup         : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWndPopup), (PVOID) lpDraw_lcl->hWndPopup);
    printf("%08lx ULONG_PTR                     lpLcl->hDD               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDD), (PVOID) lpDraw_lcl->hDD);
    printf("%08lx ULONG_PTR                     lpLcl->hGammaCalibrator  : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hGammaCalibrator), (PVOID) lpDraw_lcl->hGammaCalibrator);
    printf("%08lx LPDDGAMMACALIBRATORPROC       lpLcl->lpGammaCalibrator : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGammaCalibrator), lpDraw_lcl->lpGammaCalibrator);
}



#endif

/* EOF */
