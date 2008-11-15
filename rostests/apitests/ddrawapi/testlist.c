#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawapi.h"

#define MIX_BOTH_CAPS(a,b)  (  (a + b - (a & b))  )

void dump_DDRAWI_DIRECTDRAW_INT(char *str, LPDDRAWI_DIRECTDRAW_INT lpDraw_int, DWORD offset);
void dump_DDRAWI_DIRECTDRAW_LCL(char *str, LPDDRAWI_DIRECTDRAW_LCL lpDraw_lcl, DWORD offset);
void dump_DDRAWI_DIRECTDRAW_GBL(char *str, LPDDRAWI_DIRECTDRAW_GBL lpDraw_gbl, DWORD offset);

void dump_DDCORECAPS(char *str, LPDDCORECAPS lpDdcorecaps, DWORD offset);
void dump_VIDMEMINFO(char *str, LPVIDMEMINFO lpVidmeminfo, DWORD offset);
void dump_DBLNODE(char *str, LPDBLNODE lpDblnode, DWORD offset);

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


 void dump_DDRAWI_DIRECTDRAW_INT(char *str, LPDDRAWI_DIRECTDRAW_INT lpDraw_int, DWORD offset)
{
    char buffer[2048];
    if (lpDraw_int == NULL)
        return ;

    printf("%08lx LPVOID                                  %slpVtbl                                         : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpVtbl)+offset, str, lpDraw_int->lpVtbl);
    printf("%08lx DWORD                                   %s->lpLcl                                        : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLcl) + offset, str, lpDraw_int->lpLcl );
    printf("%08lx DWORD                                   %s->lpLink                                       : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLink) + offset, str, lpDraw_int->lpLink );
    printf("%08lx DWORD                                   %s->dwIntRefCnt                                  : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, dwIntRefCnt) + offset, str, lpDraw_int->dwIntRefCnt );

    printf("\n");
    sprintf(buffer,"%slpLcl->",str);
    dump_DDRAWI_DIRECTDRAW_LCL(buffer, lpDraw_int->lpLcl, FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLcl));

    printf("\n");
    sprintf(buffer,"%slpLink->",str);
    dump_DDRAWI_DIRECTDRAW_INT(buffer, lpDraw_int->lpLink, FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLink));
}

void dump_DDRAWI_DIRECTDRAW_LCL(char *str, LPDDRAWI_DIRECTDRAW_LCL lpDraw_lcl, DWORD offset)
{
    char buffer[2048];
    if (lpDraw_lcl == NULL)
        return ;

    printf("%08lx DWORD                                   %slpDDMore                                : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDMore)+offset, str, lpDraw_lcl->lpDDMore);
    printf("%08lx LPDDRAWI_DIRECTDRAW_GBL                 %slpGbl                                   : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGbl)+offset, str, lpDraw_lcl->lpGbl);
    printf("%08lx DWORD                                   %sdwUnused0                               : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwUnused0)+offset, str, lpDraw_lcl->dwUnused0);
    printf("%08lx DWORD                                   %sdwLocalFlags                            : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalFlags)+offset, str, lpDraw_lcl->dwLocalFlags);
    printf("%08lx DWORD                                   %sdwLocalRefCnt                           : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalRefCnt)+offset, str, lpDraw_lcl->dwLocalRefCnt);
    printf("%08lx DWORD                                   %sdwProcessId                             : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwProcessId)+offset, str, lpDraw_lcl->dwProcessId);
    printf("%08lx PVOID                                   %spUnkOuter                               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pUnkOuter)+offset, str, lpDraw_lcl->pUnkOuter);
    printf("%08lx DWORD                                   %sdwObsolete1                             : 0x%08lx\n",  FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwObsolete1)+offset, str, lpDraw_lcl->dwObsolete1);   
    printf("%08lx ULONG_PTR                               %shWnd                                    : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWnd)+offset, str, (PVOID)lpDraw_lcl->hWnd);
    printf("%08lx ULONG_PTR                               %shDC                                     : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDC)+offset, str, (PVOID) lpDraw_lcl->hDC);
    printf("%08lx DWORD                                   %sdwErrorMode                             : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwErrorMode)+offset, str, lpDraw_lcl->dwErrorMode);    
    printf("%08lx LPDDRAWI_DDRAWSURFACE_INT               %slpPrimary                               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpPrimary)+offset, str, lpDraw_lcl->lpPrimary);
    printf("%08lx LPDDRAWI_DDRAWSURFACE_INT               %slpCB                                    : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpCB)+offset, str, lpDraw_lcl->lpCB);
    printf("%08lx DWORD                                   %sdwPreferredMode                         : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwPreferredMode)+offset, str, lpDraw_lcl->dwPreferredMode);
    printf("%08lx HINSTANCE                               %shD3DInstance                            : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hD3DInstance)+offset, str, lpDraw_lcl->hD3DInstance);
    printf("%08lx PVOID                                   %spD3DIUnknown                            : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pD3DIUnknown)+offset, str, (PVOID) lpDraw_lcl->pD3DIUnknown);
    printf("%08lx LPDDHAL_CALLBACKS                       %slpDDCB                                  : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDCB)+offset, str, lpDraw_lcl->lpDDCB);
    printf("%08lx ULONG_PTR                               %shDDVxd                                  : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDDVxd)+offset, str, (PVOID) lpDraw_lcl->hDDVxd);
    printf("%08lx DWORD                                   %sdwAppHackFlags                          : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwAppHackFlags)+offset, str, lpDraw_lcl->dwAppHackFlags);
    printf("%08lx ULONG_PTR                               %shFocusWnd                               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hFocusWnd)+offset, str, (PVOID) lpDraw_lcl->hFocusWnd);
    printf("%08lx DWORD                                   %sdwHotTracking                           : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwHotTracking)+offset, str, lpDraw_lcl->dwHotTracking);
    printf("%08lx DWORD                                   %sdwIMEState                              : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwIMEState)+offset, str, lpDraw_lcl->dwIMEState);
    printf("%08lx ULONG_PTR                               %shWndPopup                               : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWndPopup)+offset, str, (PVOID) lpDraw_lcl->hWndPopup);
    printf("%08lx ULONG_PTR                               %shDD                                     : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDD)+offset, str, (PVOID) lpDraw_lcl->hDD);
    printf("%08lx ULONG_PTR                               %shGammaCalibrator                        : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hGammaCalibrator)+offset, str, (PVOID) lpDraw_lcl->hGammaCalibrator);
    printf("%08lx LPDDGAMMACALIBRATORPROC                 %slpGammaCalibrator                       : 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGammaCalibrator)+offset, str, lpDraw_lcl->lpGammaCalibrator);

    printf("\n");
    sprintf(buffer,"%slpGbl->",str);
    dump_DDRAWI_DIRECTDRAW_GBL(buffer, lpDraw_lcl->lpGbl, 0);
}

void dump_DDRAWI_DIRECTDRAW_GBL(char * str, LPDDRAWI_DIRECTDRAW_GBL lpDraw_gbl, DWORD offset)
{
    char buffer[2048];
    if (lpDraw_gbl == NULL)
        return ;

    printf("%08lx DWORD                                   %sdwRefCnt                         : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwRefCnt) + offset, str, lpDraw_gbl->dwRefCnt);
    printf("%08lx DWORD                                   %sdwFlags                          : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwFlags) + offset, str, lpDraw_gbl->dwFlags);
    printf("%08lx FLATPTR                                 %sfpPrimaryOrig                    : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, fpPrimaryOrig) + offset, str, (LPVOID)lpDraw_gbl->fpPrimaryOrig);

    sprintf(buffer,"%sddCaps.",str);
    dump_DDCORECAPS(buffer, &lpDraw_gbl->ddCaps, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddCaps) + offset );

    printf("%08lx DWORD                                   %sdwInternal1                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwInternal1) + offset, str, lpDraw_gbl->dwInternal1);
    printf("%08lx DWORD                                   %sdwUnused1[0]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[0]) + offset, str, lpDraw_gbl->dwUnused1[0]);
    printf("%08lx DWORD                                   %sdwUnused1[1]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[1]) + offset, str, lpDraw_gbl->dwUnused1[1]);
    printf("%08lx DWORD                                   %sdwUnused1[2]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[2]) + offset, str, lpDraw_gbl->dwUnused1[2]);
    printf("%08lx DWORD                                   %sdwUnused1[3]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[3]) + offset, str, lpDraw_gbl->dwUnused1[3]);
    printf("%08lx DWORD                                   %sdwUnused1[4]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[4]) + offset, str, lpDraw_gbl->dwUnused1[4]);
    printf("%08lx DWORD                                   %sdwUnused1[5]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[5]) + offset, str, lpDraw_gbl->dwUnused1[5]);
    printf("%08lx DWORD                                   %sdwUnused1[6]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[6]) + offset, str, lpDraw_gbl->dwUnused1[6]);
    printf("%08lx DWORD                                   %sdwUnused1[7]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[7]) + offset, str, lpDraw_gbl->dwUnused1[7]);
    printf("%08lx DWORD                                   %sdwUnused1[8]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[8]) + offset, str, lpDraw_gbl->dwUnused1[8]);
    printf("%08lx LPDDHAL_CALLBACKS                       %slpDDCBtmp                        : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDDCBtmp) + offset, str, lpDraw_gbl->lpDDCBtmp);
    printf("%08lx LPDDRAWI_DDRAWSURFACE_INT               %sdsList                           : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dsList) + offset, str, lpDraw_gbl->dsList);
    printf("%08lx LPDDRAWI_DDRAWPALETTE_INT               %spalList                          : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, palList) + offset, str, lpDraw_gbl->palList);
    printf("%08lx LPDDRAWI_DDRAWCLIPPER_INT               %sclipperList                      : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, clipperList) + offset, str, lpDraw_gbl->clipperList);
    printf("%08lx LPDDRAWI_DIRECTDRAW_GBL                 %slp16DD                           : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lp16DD) + offset, str, lpDraw_gbl->lp16DD);
    printf("%08lx DWORD                                   %sdwMaxOverlays                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwMaxOverlays) + offset, str, lpDraw_gbl->dwMaxOverlays);
    printf("%08lx DWORD                                   %sdwCurrOverlays                   : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwCurrOverlays) + offset, str, lpDraw_gbl->dwCurrOverlays);
    printf("%08lx DWORD                                   %sdwMonitorFrequency               : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwMonitorFrequency) + offset, str, lpDraw_gbl->dwMonitorFrequency);

    sprintf(buffer,"%sddHELCaps.",str);
    dump_DDCORECAPS(buffer, &lpDraw_gbl->ddHELCaps, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddHELCaps) + offset );

    printf("%08lx DWORD                                   %sdwUnused2[0]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[0]) + offset, str, lpDraw_gbl->dwUnused2[0]);
    printf("%08lx DWORD                                   %sdwUnused2[1]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[1]) + offset, str, lpDraw_gbl->dwUnused2[1]);
    printf("%08lx DWORD                                   %sdwUnused2[2]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[2]) + offset, str, lpDraw_gbl->dwUnused2[2]);
    printf("%08lx DWORD                                   %sdwUnused2[3]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[3]) + offset, str, lpDraw_gbl->dwUnused2[3]);
    printf("%08lx DWORD                                   %sdwUnused2[4]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[4]) + offset, str, lpDraw_gbl->dwUnused2[4]);
    printf("%08lx DWORD                                   %sdwUnused2[5]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[5]) + offset, str, lpDraw_gbl->dwUnused2[5]);
    printf("%08lx DWORD                                   %sdwUnused2[6]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[6]) + offset, str, lpDraw_gbl->dwUnused2[6]);
    printf("%08lx DWORD                                   %sdwUnused2[7]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[7]) + offset, str, lpDraw_gbl->dwUnused2[7]);
    printf("%08lx DWORD                                   %sdwUnused2[8]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[8]) + offset, str, lpDraw_gbl->dwUnused2[8]);
    printf("%08lx DWORD                                   %sdwUnused2[9]                     : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[9]) + offset, str, lpDraw_gbl->dwUnused2[9]);
    printf("%08lx DWORD                                   %sdwUnused2[10]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[10]) + offset, str, lpDraw_gbl->dwUnused2[10]);
    printf("%08lx DWORD                                   %sdwUnused2[11]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[11]) + offset, str, lpDraw_gbl->dwUnused2[11]);
    printf("%08lx DWORD                                   %sdwUnused2[12]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[12]) + offset, str, lpDraw_gbl->dwUnused2[12]);
    printf("%08lx DWORD                                   %sdwUnused2[13]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[13]) + offset, str, lpDraw_gbl->dwUnused2[13]);
    printf("%08lx DWORD                                   %sdwUnused2[14]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[14]) + offset, str, lpDraw_gbl->dwUnused2[14]);
    printf("%08lx DWORD                                   %sdwUnused2[15]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[15]) + offset, str, lpDraw_gbl->dwUnused2[15]);
    printf("%08lx DWORD                                   %sdwUnused2[16]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[16]) + offset, str, lpDraw_gbl->dwUnused2[16]);
    printf("%08lx DWORD                                   %sdwUnused2[17]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[17]) + offset, str, lpDraw_gbl->dwUnused2[17]);
    printf("%08lx DWORD                                   %sdwUnused2[18]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[18]) + offset, str, lpDraw_gbl->dwUnused2[18]);
    printf("%08lx DWORD                                   %sdwUnused2[19]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[19]) + offset, str, lpDraw_gbl->dwUnused2[19]);
    printf("%08lx DWORD                                   %sdwUnused2[20]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[20]) + offset, str, lpDraw_gbl->dwUnused2[20]);
    printf("%08lx DWORD                                   %sdwUnused2[21]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[21]) + offset, str, lpDraw_gbl->dwUnused2[21]);
    printf("%08lx DWORD                                   %sdwUnused2[22]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[22]) + offset, str, lpDraw_gbl->dwUnused2[22]);
    printf("%08lx DWORD                                   %sdwUnused2[23]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[23]) + offset, str, lpDraw_gbl->dwUnused2[23]);
    printf("%08lx DWORD                                   %sdwUnused2[24]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[24]) + offset, str, lpDraw_gbl->dwUnused2[24]);
    printf("%08lx DWORD                                   %sdwUnused2[25]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[25]) + offset, str, lpDraw_gbl->dwUnused2[25]);
    printf("%08lx DWORD                                   %sdwUnused2[26]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[26]) + offset, str, lpDraw_gbl->dwUnused2[26]);
    printf("%08lx DWORD                                   %sdwUnused2[27]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[27]) + offset, str, lpDraw_gbl->dwUnused2[27]);
    printf("%08lx DWORD                                   %sdwUnused2[28]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[28]) + offset, str, lpDraw_gbl->dwUnused2[28]);
    printf("%08lx DWORD                                   %sdwUnused2[29]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[29]) + offset, str, lpDraw_gbl->dwUnused2[29]);
    printf("%08lx DWORD                                   %sdwUnused2[30]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[30]) + offset, str, lpDraw_gbl->dwUnused2[30]);
    printf("%08lx DWORD                                   %sdwUnused2[31]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[31]) + offset, str, lpDraw_gbl->dwUnused2[31]);
    printf("%08lx DWORD                                   %sdwUnused2[32]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[32]) + offset, str, lpDraw_gbl->dwUnused2[32]);
    printf("%08lx DWORD                                   %sdwUnused2[33]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[33]) + offset, str, lpDraw_gbl->dwUnused2[33]);
    printf("%08lx DWORD                                   %sdwUnused2[34]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[34]) + offset, str, lpDraw_gbl->dwUnused2[34]);
    printf("%08lx DWORD                                   %sdwUnused2[35]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[35]) + offset, str, lpDraw_gbl->dwUnused2[35]);
    printf("%08lx DWORD                                   %sdwUnused2[36]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[36]) + offset, str, lpDraw_gbl->dwUnused2[36]);
    printf("%08lx DWORD                                   %sdwUnused2[37]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[37]) + offset, str, lpDraw_gbl->dwUnused2[37]);
    printf("%08lx DWORD                                   %sdwUnused2[38]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[38]) + offset, str, lpDraw_gbl->dwUnused2[38]);
    printf("%08lx DWORD                                   %sdwUnused2[39]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[39]) + offset, str, lpDraw_gbl->dwUnused2[39]);
    printf("%08lx DWORD                                   %sdwUnused2[40]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[40]) + offset, str, lpDraw_gbl->dwUnused2[40]);
    printf("%08lx DWORD                                   %sdwUnused2[41]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[41]) + offset, str, lpDraw_gbl->dwUnused2[41]);
    printf("%08lx DWORD                                   %sdwUnused2[42]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[42]) + offset, str, lpDraw_gbl->dwUnused2[42]);
    printf("%08lx DWORD                                   %sdwUnused2[43]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[43]) + offset, str, lpDraw_gbl->dwUnused2[43]);
    printf("%08lx DWORD                                   %sdwUnused2[44]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[44]) + offset, str, lpDraw_gbl->dwUnused2[44]);
    printf("%08lx DWORD                                   %sdwUnused2[45]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[45]) + offset, str, lpDraw_gbl->dwUnused2[45]);
    printf("%08lx DWORD                                   %sdwUnused2[46]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[46]) + offset, str, lpDraw_gbl->dwUnused2[46]);
    printf("%08lx DWORD                                   %sdwUnused2[47]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[47]) + offset, str, lpDraw_gbl->dwUnused2[47]);
    printf("%08lx DWORD                                   %sdwUnused2[48]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[48]) + offset, str, lpDraw_gbl->dwUnused2[48]);
    printf("%08lx DWORD                                   %sdwUnused2[49]                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[49]) + offset, str, lpDraw_gbl->dwUnused2[49]);
    //printf("%08lx DDCOLORKEY                    lpGbl->ddckCKDestOverlay                : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddckCKDestOverlay) + offset, str, lpDraw_gbl->ddckCKDestOverlay);
    //printf("%08lx DDCOLORKEY                    lpGbl->ddckCKSrcOverlay                 : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddckCKSrcOverlay) + offset, str, lpDraw_gbl->ddckCKSrcOverlay);

    sprintf(buffer,"%svmiData.",str);
    dump_VIDMEMINFO(buffer, &lpDraw_gbl->vmiData, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, vmiData) + offset);

    printf("%08lx LPVOID                                  %slpDriverHandle                   : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDriverHandle) + offset, str, lpDraw_gbl->lpDriverHandle);
    printf("%08lx LPDDRAWI_DIRECTDRAW_LCL                 %slpExclusiveOwner                 : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpExclusiveOwner) + offset, str, lpDraw_gbl->lpExclusiveOwner);
    printf("%08lx DWORD                                   %sdwModeIndex                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwModeIndex) + offset, str, lpDraw_gbl->dwModeIndex);
    printf("%08lx DWORD                                   %sdwModeIndexOrig                  : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwModeIndexOrig) + offset, str, lpDraw_gbl->dwModeIndexOrig);
    printf("%08lx DWORD                                   %sdwNumFourCC                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwNumFourCC) + offset, str, lpDraw_gbl->dwNumFourCC);
    printf("%08lx LPDWORD                                 %slpdwFourCC                       : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpdwFourCC) + offset, str, lpDraw_gbl->lpdwFourCC);
    printf("%08lx DWORD                                   %sdwNumModes                       : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwNumModes) + offset, str, lpDraw_gbl->dwNumModes);
    printf("%08lx LPDDHALMODEI                            %slpModeInfo                       : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpModeInfo) + offset, str, lpDraw_gbl->lpModeInfo);
    //printf("%08lx PROCESS_LIST                            lpGbl->plProcessList                    : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, plProcessList) + offset, str, lpDraw_gbl->plProcessList);

    printf("%08lx DWORD                                   %sdwSurfaceLockCount               : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwSurfaceLockCount) + offset, str, lpDraw_gbl->dwSurfaceLockCount);
    printf("%08lx DWORD                                   %sdwAliasedLockCnt                 : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwAliasedLockCnt) + offset, str, lpDraw_gbl->dwAliasedLockCnt);
    printf("%08lx DWORD                                   %sdwReserved3                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved3) + offset, str, lpDraw_gbl->dwReserved3);
    printf("%08lx ULONG_PTR                               %shDD                              : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, hDD) + offset, str, lpDraw_gbl->hDD);
    printf("%08lx char                                    %scObsolete                        : %s \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, cObsolete[0]) + offset, str, lpDraw_gbl->cObsolete);
    printf("%08lx DWORD                                   %sdwReserved1                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved1)+ offset, str, lpDraw_gbl->dwReserved1);
    printf("%08lx DWORD                                   %sdwReserved2                      : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved2)+ offset, str, lpDraw_gbl->dwReserved2);

    sprintf(buffer,"%sdbnOverlayRoot.",str);
    dump_DBLNODE(buffer, &lpDraw_gbl->dbnOverlayRoot, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dbnOverlayRoot)+ offset);

    printf("%08lx volatile LPWORD                         %slpwPDeviceFlags                  : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpwPDeviceFlags)+ offset, str, lpDraw_gbl->lpwPDeviceFlags);
    printf("%08lx DWORD                                   %sdwPDevice                        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwPDevice)+ offset, str, lpDraw_gbl->dwPDevice);
    printf("%08lx DWORD                                   %sdwWin16LockCnt                   : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwWin16LockCnt)+ offset, str, lpDraw_gbl->dwWin16LockCnt);
    printf("%08lx DWORD                                   %sdwUnused3                        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused3)+ offset, str, lpDraw_gbl->dwUnused3);
    printf("%08lx DWORD                                   %shInstance                        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, hInstance)+ offset, str, lpDraw_gbl->hInstance);
    printf("%08lx DWORD                                   %sdwEvent16                        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwEvent16)+ offset, str, lpDraw_gbl->dwEvent16);
    printf("%08lx DWORD                                   %sdwSaveNumModes                   : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwSaveNumModes)+ offset, str, lpDraw_gbl->dwSaveNumModes);
    printf("%08lx ULONG_PTR                               %slpD3DGlobalDriverData            : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpD3DGlobalDriverData)+ offset, str, (LPVOID) lpDraw_gbl->lpD3DGlobalDriverData);
    printf("%08lx ULONG_PTR                               %slpD3DHALCallbacks                : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpD3DHALCallbacks)+ offset, str, (LPVOID) lpDraw_gbl->lpD3DHALCallbacks);

    sprintf(buffer,"%sddBothCaps.",str);
    dump_DDCORECAPS(buffer, &lpDraw_gbl->ddBothCaps, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddBothCaps)+ offset );

    printf("%08lx LPDDVIDEOPORTCAPS                       %slpDDVideoPortCaps                : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDDVideoPortCaps)+ offset, str, lpDraw_gbl->lpDDVideoPortCaps);
    printf("%08lx LPDDRAWI_DDVIDEOPORT_INT                %sdvpList                          : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dvpList)+ offset, str, lpDraw_gbl->dvpList);

    printf("%08lx RECT                                    %srectDevice.bottom                : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDevice.bottom)+ offset, str, lpDraw_gbl->rectDevice.bottom);
    printf("%08lx RECT                                    %srectDevice.left                  : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDevice.left)+ offset, str, lpDraw_gbl->rectDevice.left);
    printf("%08lx RECT                                    %srectDevice.right                 : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDevice.right)+ offset, str, lpDraw_gbl->rectDevice.right);
    printf("%08lx RECT                                    %srectDevice.top                   : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDevice.top)+ offset, str, lpDraw_gbl->rectDevice.top);

    printf("%08lx DWORD                                   %scMonitors                        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, cMonitors)+ offset, str, lpDraw_gbl->cMonitors);
    printf("%08lx LPVOID                                  %sgpbmiSrc                         : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, gpbmiSrc)+ offset, str, lpDraw_gbl->gpbmiSrc);
    printf("%08lx LPVOID                                  %sgpbmiDest                        : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, gpbmiDest)+ offset, str, lpDraw_gbl->gpbmiDest);
    printf("%08lx LPHEAPALIASINFO                         %sphaiHeapAliases                  : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, phaiHeapAliases)+ offset, str, lpDraw_gbl->phaiHeapAliases);
    printf("%08lx ULONG_PTR                               %shKernelHandle                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, hKernelHandle)+ offset, str, lpDraw_gbl->hKernelHandle);
    printf("%08lx ULONG_PTR                               %spfnNotifyProc                    : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, pfnNotifyProc)+ offset, str, (LPVOID)lpDraw_gbl->pfnNotifyProc);
    printf("%08lx LPDDKERNELCAPS                          %slpDDKernelCaps                   : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDDKernelCaps)+ offset, str, lpDraw_gbl->lpDDKernelCaps);
    printf("%08lx LPDDNONLOCALVIDMEMCAPS                  %slpddNLVCaps                      : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpddNLVCaps)+ offset, str, lpDraw_gbl->lpddNLVCaps);
    printf("%08lx LPDDNONLOCALVIDMEMCAPS                  %slpddNLVHELCaps                   : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpddNLVHELCaps)+ offset, str, lpDraw_gbl->lpddNLVHELCaps);
    printf("%08lx LPDDNONLOCALVIDMEMCAPS                  %slpddNLVBothCaps                  : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpddNLVBothCaps)+ offset, str, lpDraw_gbl->lpddNLVBothCaps);
    printf("%08lx ULONG_PTR                               %slpD3DExtendedCaps                : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpD3DExtendedCaps)+ offset, str, (LPVOID) lpDraw_gbl->lpD3DExtendedCaps);
    printf("%08lx DWORD                                   %sdwDOSBoxEvent                    : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwDOSBoxEvent)+ offset, str, lpDraw_gbl->dwDOSBoxEvent);

    printf("%08lx RECT                                    %srectDesktop.bottom               : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDesktop.bottom)+ offset, str, lpDraw_gbl->rectDesktop.bottom);
    printf("%08lx RECT                                    %srectDesktop.left                 : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDesktop.left)+ offset, str, lpDraw_gbl->rectDesktop.left);
    printf("%08lx RECT                                    %srectDesktop.right                : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDesktop.right)+ offset, str, lpDraw_gbl->rectDesktop.right);
    printf("%08lx RECT                                    %srectDesktop.top                  : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, rectDesktop.top)+ offset, str, lpDraw_gbl->rectDesktop.top);

    printf("%08lx char                                    %scDriverName[MAX_DRIVER_NAME]     : %s \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, cDriverName)+ offset, str, lpDraw_gbl->cDriverName);
    printf("%08lx ULONG_PTR                               %slpD3DHALCallbacks3               : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpD3DHALCallbacks3)+ offset, str, (LPVOID) lpDraw_gbl->lpD3DHALCallbacks3);
    printf("%08lx DWORD                                   %sdwNumZPixelFormats               : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwNumZPixelFormats)+ offset, str, lpDraw_gbl->dwNumZPixelFormats);
    printf("%08lx LPDDPIXELFORMAT                         %slpZPixelFormats                  : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpZPixelFormats)+ offset, str, lpDraw_gbl->lpZPixelFormats);
    printf("%08lx LPDDRAWI_DDMOTIONCOMP_INT               %smcList                           : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, mcList)+ offset, str, lpDraw_gbl->mcList);
    printf("%08lx DWORD                                   %shDDVxd                           : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, hDDVxd)+ offset, str, lpDraw_gbl->hDDVxd);
    printf("%08lx DWORD                                   %sddsCapsMore.dwCaps2              : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddsCapsMore)+ FIELD_OFFSET(DDSCAPSEX, dwCaps2)+offset, str, lpDraw_gbl->ddsCapsMore.dwCaps2);
    printf("%08lx DWORD                                   %sddsCapsMore.dwCaps3              : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddsCapsMore)+ FIELD_OFFSET(DDSCAPSEX, dwCaps3)+ offset, str, lpDraw_gbl->ddsCapsMore.dwCaps3);
    printf("%08lx DWORD                                   %sddsCapsMore.dwCaps4              : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddsCapsMore)+ FIELD_OFFSET(DDSCAPSEX, dwCaps4)+ offset, str, lpDraw_gbl->ddsCapsMore.dwCaps4);
    printf("%08lx DWORD                                   %sddsCapsMore.dwVolumeDepth        : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddsCapsMore)+ FIELD_OFFSET(DDSCAPSEX, dwVolumeDepth)+ offset, str, lpDraw_gbl->ddsCapsMore.dwVolumeDepth);
    

}

void dump_DBLNODE(char *str, LPDBLNODE lpDblnode, DWORD offset)
{

    if (lpDblnode == NULL)
        return ;

    printf("%08lx struct _DBLNODE *                         %snext      : 0x%p\n", FIELD_OFFSET(DBLNODE, next)+offset, str, lpDblnode->next);
    printf("%08lx struct _DBLNODE *                         %sprev      : 0x%p\n", FIELD_OFFSET(DBLNODE, prev)+offset, str, lpDblnode->prev);
    printf("%08lx struct LPDDRAWI_DDRAWSURFACE_LCL          %sobject    : 0x%p\n", FIELD_OFFSET(DBLNODE, object)+offset, str, lpDblnode->object);
    printf("%08lx struct LPDDRAWI_DDRAWSURFACE_INT          %sobject_int : 0x%p\n", FIELD_OFFSET(DBLNODE, object_int)+offset, str, lpDblnode->object_int);
}

void dump_DDCORECAPS(char *str, LPDDCORECAPS lpDdcorecaps, DWORD offset)
{

    if (lpDdcorecaps == NULL)
        return ;

    int c;
    printf("%08lx DWORD                         %sdwSize                             : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSize)+offset, str, lpDdcorecaps->dwSize);
    printf("%08lx DWORD                         %sdwCaps                             : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCaps)+offset, str, lpDdcorecaps->dwCaps);
    printf("%08lx DWORD                         %sdwCaps2                            : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCaps2)+offset, str, lpDdcorecaps->dwCaps2);
    printf("%08lx DWORD                         %sdwCKeyCaps                         : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCKeyCaps)+offset, str, lpDdcorecaps->dwCKeyCaps);
    printf("%08lx DWORD                         %sdwFXCaps                           : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwFXCaps)+offset, str, lpDdcorecaps->dwFXCaps);
    printf("%08lx DWORD                         %sdwFXAlphaCaps                      : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwFXAlphaCaps)+offset, str, lpDdcorecaps->dwFXAlphaCaps);
    printf("%08lx DWORD                         %sdwPalCaps                          : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwPalCaps)+offset, str, lpDdcorecaps->dwPalCaps);
    printf("%08lx DWORD                         %sdwSVCaps                           : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVCaps)+offset, str, lpDdcorecaps->dwSVCaps);
    printf("%08lx DWORD                         %sdwAlphaBltConstBitDepths           : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltConstBitDepths)+offset, str, lpDdcorecaps->dwAlphaBltConstBitDepths);
    printf("%08lx DWORD                         %sdwAlphaBltPixelBitDepths           : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltPixelBitDepths)+offset, str, lpDdcorecaps->dwAlphaBltPixelBitDepths);
    printf("%08lx DWORD                         %sdwAlphaBltSurfaceBitDepths         : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltSurfaceBitDepths)+offset, str, lpDdcorecaps->dwAlphaBltSurfaceBitDepths);
    printf("%08lx DWORD                         %sdwAlphaOverlayConstBitDepths       : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlayConstBitDepths)+offset, str, lpDdcorecaps->dwAlphaOverlayConstBitDepths);
    printf("%08lx DWORD                         %sdwAlphaOverlayPixelBitDepths       : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlayPixelBitDepths)+offset, str, lpDdcorecaps->dwAlphaOverlayPixelBitDepths);
    printf("%08lx DWORD                         %sdwAlphaOverlaySurfaceBitDepths     : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlaySurfaceBitDepths)+offset, str, lpDdcorecaps->dwAlphaOverlaySurfaceBitDepths);
    printf("%08lx DWORD                         %sdwZBufferBitDepths                 : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwZBufferBitDepths)+offset, str, lpDdcorecaps->dwZBufferBitDepths);
    printf("%08lx DWORD                         %sdwVidMemTotal                      : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVidMemTotal)+offset, str, lpDdcorecaps->dwVidMemTotal);
    printf("%08lx DWORD                         %sdwVidMemFree                       : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVidMemFree)+offset, str, lpDdcorecaps->dwVidMemFree);
    printf("%08lx DWORD                         %sdwMaxVisibleOverlays               : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxVisibleOverlays)+offset, str, lpDdcorecaps->dwMaxVisibleOverlays);
    printf("%08lx DWORD                         %sdwCurrVisibleOverlays              : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCurrVisibleOverlays)+offset, str, lpDdcorecaps->dwCurrVisibleOverlays);
    printf("%08lx DWORD                         %sdwNumFourCCCodes                   : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwNumFourCCCodes)+offset, str, lpDdcorecaps->dwNumFourCCCodes);
    printf("%08lx DWORD                         %sdwAlignBoundarySrc                 : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignBoundarySrc)+offset, str, lpDdcorecaps->dwAlignBoundarySrc);
    printf("%08lx DWORD                         %sdwAlignSizeSrc                     : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignSizeSrc)+offset, str, lpDdcorecaps->dwAlignSizeSrc);
    printf("%08lx DWORD                         %sdwAlignBoundaryDest                : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignBoundaryDest)+offset, str, lpDdcorecaps->dwAlignBoundaryDest);
    printf("%08lx DWORD                         %sdwAlignSizeDest                    : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignSizeDest)+offset, str, lpDdcorecaps->dwAlignSizeDest);
    printf("%08lx DWORD                         %sdwAlignStrideAlign                 : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignStrideAlign)+offset, str, lpDdcorecaps->dwAlignStrideAlign);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwRops[0x%02x]                     : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwRops[c])+offset, str, c, lpDdcorecaps->dwRops[c]);
    }

    printf("%08lx DWORD                         %sddsCaps.dwCaps                     : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, ddsCaps)+offset, str, lpDdcorecaps->ddsCaps.dwCaps);
    printf("%08lx DWORD                         %sdwMinOverlayStretch                : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinOverlayStretch)+offset, str, lpDdcorecaps->dwMinOverlayStretch);
    printf("%08lx DWORD                         %sdwMaxOverlayStretch                : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxOverlayStretch)+offset, str, lpDdcorecaps->dwMaxOverlayStretch);
    printf("%08lx DWORD                         %sdwMinLiveVideoStretch              : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinLiveVideoStretch)+offset, str, lpDdcorecaps->dwMinLiveVideoStretch);
    printf("%08lx DWORD                         %sdwMaxLiveVideoStretch              : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxLiveVideoStretch)+offset, str, lpDdcorecaps->dwMaxLiveVideoStretch);
    printf("%08lx DWORD                         %sdwMinHwCodecStretch                : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinHwCodecStretch)+offset, str, lpDdcorecaps->dwMinHwCodecStretch);
    printf("%08lx DWORD                         %sdwMaxHwCodecStretch                : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxHwCodecStretch)+offset, str, lpDdcorecaps->dwMaxHwCodecStretch);
    printf("%08lx DWORD                         %sdwReserved1                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved1)+offset, str, lpDdcorecaps->dwReserved1);
    printf("%08lx DWORD                         %sdwReserved2                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved2)+offset, str, lpDdcorecaps->dwReserved2);
    printf("%08lx DWORD                         %sdwReserved3                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved3)+offset, str, lpDdcorecaps->dwReserved3);
    printf("%08lx DWORD                         %sdwSVBCaps                          : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBCaps)+offset, str, lpDdcorecaps->dwSVBCaps);
    printf("%08lx DWORD                         %sdwSVBCKeyCaps                      : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBCKeyCaps)+offset, str, lpDdcorecaps->dwSVBCKeyCaps);
    printf("%08lx DWORD                         %sdwSVBFXCaps                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBFXCaps)+offset, str, lpDdcorecaps->dwSVBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwSVBRops[0x%02x]                  : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBRops[c])+offset, str, c, lpDdcorecaps->dwSVBRops[c]);
    }

    printf("%08lx DWORD                         %sdwVSBCaps                          : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBCaps)+offset, str, lpDdcorecaps->dwVSBCaps);
    printf("%08lx DWORD                         %sdwVSBCKeyCaps                      : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBCKeyCaps)+offset, str, lpDdcorecaps->dwVSBCKeyCaps);
    printf("%08lx DWORD                         %sdwVSBFXCaps                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBFXCaps)+offset, str, lpDdcorecaps->dwVSBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwVSBRops[0x%02x]                  : 0x%08lx\n",  FIELD_OFFSET(DDCORECAPS, dwVSBRops[c])+offset, str, c, lpDdcorecaps->dwVSBRops[c]);
    }

    printf("%08lx DWORD                         %sdwSSBCaps                          : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBCaps)+offset, str, lpDdcorecaps->dwSSBCaps);
    printf("%08lx DWORD                         %sdwSSBCKeyCaps                      : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBCKeyCaps)+offset, str, lpDdcorecaps->dwSSBCKeyCaps);
    printf("%08lx DWORD                         %sdwSSBFXCaps                        : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBFXCaps)+offset, str, lpDdcorecaps->dwSSBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwSSBRops[0x%02x]                  : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBRops[c])+offset, str, c, lpDdcorecaps->dwSSBRops[c]);
    }

    printf("%08lx DWORD                         %sdwMaxVideoPorts                    : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxVideoPorts)+offset, str, lpDdcorecaps->dwMaxVideoPorts);
    printf("%08lx DWORD                         %sdwCurrVideoPorts                   : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCurrVideoPorts)+offset, str, lpDdcorecaps->dwCurrVideoPorts);
    printf("%08lx DWORD                         %sdwSVBCaps2                         : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwZBufferBitDepths)+offset, str, lpDdcorecaps->dwSVBCaps2);
}

void dump_VIDMEMINFO(char *str, LPVIDMEMINFO lpVidmeminfo, DWORD offset)
{

    if (lpVidmeminfo == NULL)
        return ;

    printf("%08lx FLATPTR                       %sfpPrimary                : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, fpPrimary)+offset, str, lpVidmeminfo->fpPrimary);
    printf("%08lx DWORD                         %sdwFlags                  : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwFlags)+offset, str, lpVidmeminfo->dwFlags);
    printf("%08lx DWORD                         %sdwDisplayWidth           : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, fpPrimary)+offset, str, lpVidmeminfo->dwDisplayWidth);
    printf("%08lx DWORD                         %sdwDisplayHeight          : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwDisplayHeight)+offset, str, lpVidmeminfo->dwDisplayHeight);
    printf("%08lx LONG                          %slDisplayPitch            : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, lDisplayPitch)+offset, str, lpVidmeminfo->lDisplayPitch);
    //printf("%08lx DDPIXELFORMAT                       %sddpfDisplay          : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, ddpfDisplay)+offset, str, lpVidmeminfo->ddpfDisplay);
    printf("%08lx DWORD                         %sdwOffscreenAlign         : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwOffscreenAlign)+offset, str, lpVidmeminfo->dwOffscreenAlign);
    printf("%08lx DWORD                         %sdwOverlayAlign           : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwOverlayAlign)+offset, str, lpVidmeminfo->dwOverlayAlign);
    printf("%08lx DWORD                         %sdwTextureAlign           : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwTextureAlign)+offset, str, lpVidmeminfo->dwTextureAlign);
    printf("%08lx DWORD                         %sdwZBufferAlign           : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwZBufferAlign)+offset, str, lpVidmeminfo->dwZBufferAlign);
    printf("%08lx DWORD                         %sdwAlphaAlign             : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwDisplayWidth)+offset, str, lpVidmeminfo->dwAlphaAlign);
    printf("%08lx DWORD                         %sdwNumHeaps               : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwNumHeaps)+offset, str, lpVidmeminfo->dwNumHeaps);
    printf("%08lx LPVIDMEM                      %spvmList                  : 0x%p\n", FIELD_OFFSET(VIDMEMINFO, pvmList)+offset, str, lpVidmeminfo->pvmList);
}











#endif

/* EOF */
