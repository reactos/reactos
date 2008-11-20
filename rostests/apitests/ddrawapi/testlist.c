#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawapi.h"

#define MIX_BOTH_CAPS(a,b)  (  (a + b - (a & b))  )

/* Simple position the dump output bit better */
#define MY_MAX_SPACE_LEN 70

#define MY_POS1_SPACE_LEN  30
#define MY_POS2_SPACE_LEN 0
char space_buffer[MY_MAX_SPACE_LEN+1];

#define MY_DUMP_STR(buffer,mStr,str,lpStr) sprintf(buffer,"%s%s%s%s%s", mStr,&space_buffer[ MY_POS1_SPACE_LEN + strlen(mStr) ], str,lpStr, &space_buffer[ MY_POS2_SPACE_LEN + (strlen(lpStr) + strlen(str))  ]);

/* DirectDraw dump*/
void dump_DDRAWI_DIRECTDRAW_INT(char *str, LPDDRAWI_DIRECTDRAW_INT lpDraw_int, DWORD offset);
void dump_DDRAWI_DIRECTDRAW_LCL(char *str, LPDDRAWI_DIRECTDRAW_LCL lpDraw_lcl, DWORD offset);
void dump_DDRAWI_DIRECTDRAW_GBL(char *str, LPDDRAWI_DIRECTDRAW_GBL lpDraw_gbl, DWORD offset);
void dump_DDRAWI_DDRAWSURFACE_LCL(char *str, LPDDRAWI_DDRAWSURFACE_LCL lpDdrawSurface, DWORD offset);

/* DirectDraw Surface dump*/
void dump_DDRAWI_DDRAWSURFACE_INT(char *str, LPDDRAWI_DDRAWSURFACE_INT lpDdrawSurface, DWORD offset);

/* DirectDraw misc dump*/
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

    RtlFillMemory(space_buffer,MY_MAX_SPACE_LEN,32);
    space_buffer[MY_MAX_SPACE_LEN] = 0;


    MY_DUMP_STR(buffer,"LPVOID\0",str,"lpVtbl\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpVtbl)+offset, buffer, lpDraw_int->lpVtbl);

    MY_DUMP_STR(buffer,"DWORD\0",str,"lpLcl\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLcl) + offset, buffer, lpDraw_int->lpLcl );

    MY_DUMP_STR(buffer,"DWORD\0",str,"lpLink\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLink) + offset, buffer, lpDraw_int->lpLink );

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwIntRefCnt\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, dwIntRefCnt) + offset, buffer, lpDraw_int->dwIntRefCnt );

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

    MY_DUMP_STR(buffer,"DWORD\0",str,"lpDDMore\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDMore)+offset, buffer, lpDraw_lcl->lpDDMore);

    MY_DUMP_STR(buffer,"LPDDRAWI_DIRECTDRAW_GBL\0",str,"lpGbl\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGbl)+offset, buffer, lpDraw_lcl->lpGbl);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused0\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwUnused0)+offset, buffer, lpDraw_lcl->dwUnused0);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwLocalFlags\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalFlags)+offset, buffer, lpDraw_lcl->dwLocalFlags);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwLocalRefCnt\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwLocalRefCnt)+offset, buffer, lpDraw_lcl->dwLocalRefCnt);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwProcessId\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwProcessId)+offset, buffer, lpDraw_lcl->dwProcessId);

    MY_DUMP_STR(buffer,"PVOID\0",str,"pUnkOuter\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pUnkOuter)+offset, buffer, lpDraw_lcl->pUnkOuter);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwObsolete1\0");
    printf("%08lx %s: 0x%08lx\n",  FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwObsolete1)+offset, buffer, lpDraw_lcl->dwObsolete1);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hWnd\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWnd)+offset, buffer, (PVOID)lpDraw_lcl->hWnd);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDC\0");
    printf("%08lx %shDC: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDC)+offset, str, (PVOID) lpDraw_lcl->hDC);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwErrorMode\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwErrorMode)+offset, buffer, lpDraw_lcl->dwErrorMode);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"lpPrimary\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpPrimary)+offset, buffer, lpDraw_lcl->lpPrimary);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"lpCB\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpCB)+offset, buffer, lpDraw_lcl->lpCB);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwPreferredMode\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwPreferredMode)+offset, buffer, lpDraw_lcl->dwPreferredMode);




    MY_DUMP_STR(buffer,"HINSTANCE\0",str,"hD3DInstance\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hD3DInstance)+offset, buffer, lpDraw_lcl->hD3DInstance);

    MY_DUMP_STR(buffer,"PVOID\0",str,"pD3DIUnknown\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, pD3DIUnknown)+offset, buffer, (PVOID) lpDraw_lcl->pD3DIUnknown);

    MY_DUMP_STR(buffer,"LPDDHAL_CALLBACKS\0",str,"lpDDCB\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpDDCB)+offset, buffer, lpDraw_lcl->lpDDCB);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDDVxd\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDDVxd)+offset, buffer, (PVOID) lpDraw_lcl->hDDVxd);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAppHackFlags\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwAppHackFlags)+offset, buffer, lpDraw_lcl->dwAppHackFlags);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hFocusWnd\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hFocusWnd)+offset, buffer, (PVOID) lpDraw_lcl->hFocusWnd);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwHotTracking\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwHotTracking)+offset, buffer, lpDraw_lcl->dwHotTracking);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwIMEState\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, dwIMEState)+offset, buffer, lpDraw_lcl->dwIMEState);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hWndPopu\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hWndPopup)+offset, buffer, (PVOID) lpDraw_lcl->hWndPopup);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDD\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hDD)+offset, buffer, (PVOID) lpDraw_lcl->hDD);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hGammaCalibrator\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, hGammaCalibrator)+offset, buffer, (PVOID) lpDraw_lcl->hGammaCalibrator);

    MY_DUMP_STR(buffer,"LPDDGAMMACALIBRATORPROC\0",str,"lpGammaCalibrator\0");
    printf("%08lx %s: 0x%p \n",    FIELD_OFFSET(DDRAWI_DIRECTDRAW_LCL, lpGammaCalibrator)+offset, buffer, lpDraw_lcl->lpGammaCalibrator);



    printf("\n");
    sprintf(buffer,"%slpGbl->",str);
    dump_DDRAWI_DIRECTDRAW_GBL(buffer, lpDraw_lcl->lpGbl, 0);
    sprintf(buffer,"%slpPrimary->",str);
    dump_DDRAWI_DDRAWSURFACE_INT(buffer, lpDraw_lcl->lpPrimary, 0);
    sprintf(buffer,"%slpCB->",str);
    dump_DDRAWI_DDRAWSURFACE_INT(buffer, lpDraw_lcl->lpCB, 0);
}

void dump_DDRAWI_DIRECTDRAW_GBL(char * str, LPDDRAWI_DIRECTDRAW_GBL lpDraw_gbl, DWORD offset)
{
    char buffer[2048];
    if (lpDraw_gbl == NULL)
        return ;

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwRefCnt\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwRefCnt) + offset, buffer, lpDraw_gbl->dwRefCnt);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwFlags\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwFlags) + offset, buffer, lpDraw_gbl->dwFlags);

    MY_DUMP_STR(buffer,"FLATPTR\0",str,"fpPrimaryOrig\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, fpPrimaryOrig) + offset, buffer, (LPVOID)lpDraw_gbl->fpPrimaryOrig);

    sprintf(buffer,"%sddCaps.",str);
    dump_DDCORECAPS(buffer, &lpDraw_gbl->ddCaps, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddCaps) + offset );

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwInternal1\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwInternal1) + offset, buffer, lpDraw_gbl->dwInternal1);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[0]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[0]) + offset, buffer, lpDraw_gbl->dwUnused1[0]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[1]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[1]) + offset, buffer, lpDraw_gbl->dwUnused1[1]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[2]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[2]) + offset, buffer, lpDraw_gbl->dwUnused1[2]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[3]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[3]) + offset, buffer, lpDraw_gbl->dwUnused1[3]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[4]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[4]) + offset, buffer, lpDraw_gbl->dwUnused1[4]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[5]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[5]) + offset, buffer, lpDraw_gbl->dwUnused1[5]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[6]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[6]) + offset, buffer, lpDraw_gbl->dwUnused1[6]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[7]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[7]) + offset, buffer, lpDraw_gbl->dwUnused1[7]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused1[8]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused1[8]) + offset, buffer, lpDraw_gbl->dwUnused1[8]);

    MY_DUMP_STR(buffer,"LPDDHAL_CALLBACKS\0",str,"lpDDCBtmp[8]\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDDCBtmp) + offset, buffer, lpDraw_gbl->lpDDCBtmp);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"dsList\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dsList) + offset, buffer, lpDraw_gbl->dsList);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWPALETTE_INT\0",str,"palList\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, palList) + offset, buffer, lpDraw_gbl->palList);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWCLIPPER_INT\0",str,"clipperList\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, clipperList) + offset, buffer, lpDraw_gbl->clipperList);

    MY_DUMP_STR(buffer,"LPDDRAWI_DIRECTDRAW_GBL\0",str,"lp16DD\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lp16DD) + offset, buffer, lpDraw_gbl->lp16DD);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxOverlays\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwMaxOverlays) + offset, buffer, lpDraw_gbl->dwMaxOverlays);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCurrOverlays\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwCurrOverlays) + offset, buffer, lpDraw_gbl->dwCurrOverlays);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMonitorFrequency\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwMonitorFrequency) + offset, buffer, lpDraw_gbl->dwMonitorFrequency);

    sprintf(buffer,"%sddHELCaps.",str);
    dump_DDCORECAPS(buffer, &lpDraw_gbl->ddHELCaps, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddHELCaps) + offset );

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x00]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[0]) + offset, buffer, lpDraw_gbl->dwUnused2[0]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x01]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[1]) + offset, buffer, lpDraw_gbl->dwUnused2[1]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x02]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[2]) + offset, buffer, lpDraw_gbl->dwUnused2[2]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x03]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[3]) + offset, buffer, lpDraw_gbl->dwUnused2[3]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x04]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[4]) + offset, buffer, lpDraw_gbl->dwUnused2[4]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x05]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[5]) + offset, buffer, lpDraw_gbl->dwUnused2[5]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x06]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[6]) + offset, buffer, lpDraw_gbl->dwUnused2[6]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x07]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[7]) + offset, buffer, lpDraw_gbl->dwUnused2[7]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x08]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[8]) + offset, buffer, lpDraw_gbl->dwUnused2[8]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x09]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[9]) + offset, buffer, lpDraw_gbl->dwUnused2[9]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0A]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[10]) + offset, buffer, lpDraw_gbl->dwUnused2[10]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0B]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[11]) + offset, buffer, lpDraw_gbl->dwUnused2[11]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0C]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[12]) + offset, buffer, lpDraw_gbl->dwUnused2[12]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0D]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[13]) + offset, buffer, lpDraw_gbl->dwUnused2[13]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0E]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[14]) + offset, buffer, lpDraw_gbl->dwUnused2[14]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x0F]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[15]) + offset, buffer, lpDraw_gbl->dwUnused2[15]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x10]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[16]) + offset, buffer, lpDraw_gbl->dwUnused2[16]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x11]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[17]) + offset, buffer, lpDraw_gbl->dwUnused2[17]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x12]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[18]) + offset, buffer, lpDraw_gbl->dwUnused2[18]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x13]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[19]) + offset, buffer, lpDraw_gbl->dwUnused2[19]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x14]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[20]) + offset, buffer, lpDraw_gbl->dwUnused2[20]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x15]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[21]) + offset, buffer, lpDraw_gbl->dwUnused2[21]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x16]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[22]) + offset, buffer, lpDraw_gbl->dwUnused2[22]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x17]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[23]) + offset, buffer, lpDraw_gbl->dwUnused2[23]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x18]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[24]) + offset, buffer, lpDraw_gbl->dwUnused2[24]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x19]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[25]) + offset, buffer, lpDraw_gbl->dwUnused2[25]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1A]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[26]) + offset, buffer, lpDraw_gbl->dwUnused2[26]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1B]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[27]) + offset, buffer, lpDraw_gbl->dwUnused2[27]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1C]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[28]) + offset, buffer, lpDraw_gbl->dwUnused2[28]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1D]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[29]) + offset, buffer, lpDraw_gbl->dwUnused2[29]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1E]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[30]) + offset, buffer, lpDraw_gbl->dwUnused2[30]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x1F]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[31]) + offset, buffer, lpDraw_gbl->dwUnused2[31]);





    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x20]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[32]) + offset, buffer, lpDraw_gbl->dwUnused2[32]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x21]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[33]) + offset, buffer, lpDraw_gbl->dwUnused2[33]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x22]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[34]) + offset, buffer, lpDraw_gbl->dwUnused2[34]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x23]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[35]) + offset, buffer, lpDraw_gbl->dwUnused2[35]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x24]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[36]) + offset, buffer, lpDraw_gbl->dwUnused2[36]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x25]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[37]) + offset, buffer, lpDraw_gbl->dwUnused2[37]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x26]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[38]) + offset, buffer, lpDraw_gbl->dwUnused2[38]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x27]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[39]) + offset, buffer, lpDraw_gbl->dwUnused2[39]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x28]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[40]) + offset, buffer, lpDraw_gbl->dwUnused2[40]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x29]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[41]) + offset, buffer, lpDraw_gbl->dwUnused2[41]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2A]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[42]) + offset, buffer, lpDraw_gbl->dwUnused2[42]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2B]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[43]) + offset, buffer, lpDraw_gbl->dwUnused2[43]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2C]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[44]) + offset, buffer, lpDraw_gbl->dwUnused2[44]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2D]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[45]) + offset, buffer, lpDraw_gbl->dwUnused2[45]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2E]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[46]) + offset, buffer, lpDraw_gbl->dwUnused2[46]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x2F]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[47]) + offset, buffer, lpDraw_gbl->dwUnused2[47]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x30]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[46]) + offset, buffer, lpDraw_gbl->dwUnused2[48]);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwUnused2[0x31]\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwUnused2[47]) + offset, buffer, lpDraw_gbl->dwUnused2[49]);


    //printf("%08lx DDCOLORKEY                    lpGbl->ddckCKDestOverlay                : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddckCKDestOverlay) + offset, str, lpDraw_gbl->ddckCKDestOverlay);
    //printf("%08lx DDCOLORKEY                    lpGbl->ddckCKSrcOverlay                 : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, ddckCKSrcOverlay) + offset, str, lpDraw_gbl->ddckCKSrcOverlay);

    sprintf(buffer,"%svmiData.",str);
    dump_VIDMEMINFO(buffer, &lpDraw_gbl->vmiData, FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, vmiData) + offset);

    MY_DUMP_STR(buffer,"LPVOID\0",str,"lpDriverHandle\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpDriverHandle) + offset, buffer, lpDraw_gbl->lpDriverHandle);

    MY_DUMP_STR(buffer,"LPDDRAWI_DIRECTDRAW_LCL\0",str,"lpExclusiveOwner\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpExclusiveOwner) + offset, buffer, lpDraw_gbl->lpExclusiveOwner);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwModeIndex\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwModeIndex) + offset, buffer, lpDraw_gbl->dwModeIndex);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwModeIndexOrig\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwModeIndexOrig) + offset, buffer, lpDraw_gbl->dwModeIndexOrig);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwNumFourCC\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwNumFourCC) + offset, buffer, lpDraw_gbl->dwNumFourCC);


    MY_DUMP_STR(buffer,"LPDWORD\0",str,"lpdwFourCC\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpdwFourCC) + offset, buffer, lpDraw_gbl->lpdwFourCC);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwNumModes\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwNumModes) + offset, buffer, lpDraw_gbl->dwNumModes);

    MY_DUMP_STR(buffer,"LPDDHALMODEINFO\0",str,"lpModeInfo\0");
    printf("%08lx %s: 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, lpModeInfo) + offset, buffer, lpDraw_gbl->lpModeInfo);
    //printf("%08lx PROCESS_LIST                            lpGbl->plProcessList                    : 0x%p \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, plProcessList) + offset, str, lpDraw_gbl->plProcessList);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSurfaceLockCount\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwSurfaceLockCount) + offset, buffer, lpDraw_gbl->dwSurfaceLockCount);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAliasedLockCnt\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwAliasedLockCnt) + offset, buffer, lpDraw_gbl->dwAliasedLockCnt);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved3\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved3) + offset, buffer, lpDraw_gbl->dwReserved3);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDD\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, hDD) + offset, buffer, lpDraw_gbl->hDD);

    MY_DUMP_STR(buffer,"char\0",str,"cObsolete\0");
    printf("%08lx %s: %s \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, cObsolete[0]) + offset, buffer, lpDraw_gbl->cObsolete);


    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved1\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved1)+ offset, buffer, lpDraw_gbl->dwReserved1);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved2\0");
    printf("%08lx %s: 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_GBL, dwReserved2)+ offset, buffer, lpDraw_gbl->dwReserved2);

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
    char buffer[2048];
    if (lpDblnode == NULL)
        return ;

    MY_DUMP_STR(buffer,"struct _DBLNODE *\0",str,"next\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DBLNODE, next)+offset, buffer, lpDblnode->next);

    MY_DUMP_STR(buffer,"struct _DBLNODE *\0",str,"prev\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DBLNODE, prev)+offset, buffer, lpDblnode->prev);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_LCL\0",str,"object\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DBLNODE, object)+offset, buffer, lpDblnode->object);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"object_int\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DBLNODE, object_int)+offset, buffer, lpDblnode->object_int);

    printf("\n");
    sprintf(buffer,"%sobject_int->",str);
    dump_DDRAWI_DDRAWSURFACE_INT(buffer, lpDblnode->object_int, 0);
}

void dump_DDCORECAPS(char *str, LPDDCORECAPS lpDdcorecaps, DWORD offset)
{
    char buffer[2048];
    if (lpDdcorecaps == NULL)
        return ;

    int c;

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSize\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSize)+offset, buffer, lpDdcorecaps->dwSize);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCaps)+offset, buffer, lpDdcorecaps->dwCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCaps2\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCaps2)+offset, buffer, lpDdcorecaps->dwCaps2);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCKeyCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCKeyCaps)+offset, buffer, lpDdcorecaps->dwCKeyCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwFXCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwFXCaps)+offset, buffer, lpDdcorecaps->dwFXCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwFXAlphaCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwFXAlphaCaps)+offset, buffer, lpDdcorecaps->dwFXAlphaCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwPalCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwPalCaps)+offset, buffer, lpDdcorecaps->dwPalCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSVCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVCaps)+offset, buffer, lpDdcorecaps->dwSVCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaBltConstBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltConstBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaBltConstBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaBltPixelBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltPixelBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaBltPixelBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaBltSurfaceBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaBltSurfaceBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaBltSurfaceBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaOverlayConstBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlayConstBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaOverlayConstBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaOverlayPixelBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlayPixelBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaOverlayPixelBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaOverlaySurfaceBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlphaOverlaySurfaceBitDepths)+offset, buffer, lpDdcorecaps->dwAlphaOverlaySurfaceBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwZBufferBitDepths\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwZBufferBitDepths)+offset, buffer, lpDdcorecaps->dwZBufferBitDepths);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwVidMemTotal\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVidMemTotal)+offset, buffer, lpDdcorecaps->dwVidMemTotal);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwVidMemFree\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVidMemFree)+offset, buffer, lpDdcorecaps->dwVidMemFree);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxVisibleOverlays\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxVisibleOverlays)+offset, buffer, lpDdcorecaps->dwMaxVisibleOverlays);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCurrVisibleOverlays\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCurrVisibleOverlays)+offset, buffer, lpDdcorecaps->dwCurrVisibleOverlays);
    
    MY_DUMP_STR(buffer,"DWORD\0",str,"dwNumFourCCCodes\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwNumFourCCCodes)+offset, buffer, lpDdcorecaps->dwNumFourCCCodes);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlignBoundarySrc\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignBoundarySrc)+offset, buffer, lpDdcorecaps->dwAlignBoundarySrc);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlignSizeSrc\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignSizeSrc)+offset, buffer, lpDdcorecaps->dwAlignSizeSrc);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlignBoundaryDest\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignBoundaryDest)+offset, buffer, lpDdcorecaps->dwAlignBoundaryDest);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlignSizeDest\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignSizeDest)+offset, buffer, lpDdcorecaps->dwAlignSizeDest);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlignStrideAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwAlignStrideAlign)+offset, buffer, lpDdcorecaps->dwAlignStrideAlign);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwRops[0x%02x]                     : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwRops[c])+offset, str, c, lpDdcorecaps->dwRops[c]);
    }

    MY_DUMP_STR(buffer,"DWORD\0",str,"ddsCaps.dwCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, ddsCaps)+offset, buffer, lpDdcorecaps->ddsCaps.dwCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMinOverlayStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinOverlayStretch)+offset, buffer, lpDdcorecaps->dwMinOverlayStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxOverlayStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxOverlayStretch)+offset, buffer, lpDdcorecaps->dwMaxOverlayStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMinLiveVideoStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinLiveVideoStretch)+offset, buffer, lpDdcorecaps->dwMinLiveVideoStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxLiveVideoStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxLiveVideoStretch)+offset, buffer, lpDdcorecaps->dwMaxLiveVideoStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMinHwCodecStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMinHwCodecStretch)+offset, buffer, lpDdcorecaps->dwMinHwCodecStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxHwCodecStretch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxHwCodecStretch)+offset, buffer, lpDdcorecaps->dwMaxHwCodecStretch);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved1\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved1)+offset, buffer, lpDdcorecaps->dwReserved1);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved2\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved2)+offset, buffer, lpDdcorecaps->dwReserved2);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwReserved3\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwReserved3)+offset, buffer, lpDdcorecaps->dwReserved3);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSVBCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBCaps)+offset, buffer, lpDdcorecaps->dwSVBCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSVBCKeyCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBCKeyCaps)+offset, buffer, lpDdcorecaps->dwSVBCKeyCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSVBFXCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBFXCaps)+offset, buffer, lpDdcorecaps->dwSVBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwSVBRops[0x%02x]                  : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSVBRops[c])+offset, str, c, lpDdcorecaps->dwSVBRops[c]);
    }

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwVSBCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBCaps)+offset, buffer, lpDdcorecaps->dwVSBCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwVSBCKeyCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBCKeyCaps)+offset, buffer, lpDdcorecaps->dwVSBCKeyCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwVSBFXCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwVSBFXCaps)+offset, buffer, lpDdcorecaps->dwVSBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwVSBRops[0x%02x]                  : 0x%08lx\n",  FIELD_OFFSET(DDCORECAPS, dwVSBRops[c])+offset, str, c, lpDdcorecaps->dwVSBRops[c]);
    }

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSSBCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBCaps)+offset, buffer, lpDdcorecaps->dwSSBCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSSBCKeyCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBCKeyCaps)+offset, buffer, lpDdcorecaps->dwSSBCKeyCaps);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSSBFXCaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBFXCaps)+offset, buffer, lpDdcorecaps->dwSSBFXCaps);

    for (c=0;c<DD_ROP_SPACE;c++)
    {
        printf("%08lx DWORD                         %sdwSSBRops[0x%02x]                  : 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwSSBRops[c])+offset, str, c, lpDdcorecaps->dwSSBRops[c]);
    }

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwMaxVideoPorts\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwMaxVideoPorts)+offset, buffer, lpDdcorecaps->dwMaxVideoPorts);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwCurrVideoPorts\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwCurrVideoPorts)+offset, buffer, lpDdcorecaps->dwCurrVideoPorts);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwSVBCaps2\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDCORECAPS, dwZBufferBitDepths)+offset, buffer, lpDdcorecaps->dwSVBCaps2);
}

void dump_VIDMEMINFO(char *str, LPVIDMEMINFO lpVidmeminfo, DWORD offset)
{
    char buffer[2048];
    if (lpVidmeminfo == NULL)
        return ;

    MY_DUMP_STR(buffer,"FLATPTR\0",str,"fpPrimary\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, fpPrimary)+offset, buffer, lpVidmeminfo->fpPrimary);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwFlags\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwFlags)+offset, buffer, lpVidmeminfo->dwFlags);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwDisplayWidth\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, fpPrimary)+offset, buffer, lpVidmeminfo->dwDisplayWidth);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwDisplayHeight\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwDisplayHeight)+offset, buffer, lpVidmeminfo->dwDisplayHeight);

    MY_DUMP_STR(buffer,"LONG\0",str,"lDisplayPitch\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, lDisplayPitch)+offset, buffer, lpVidmeminfo->lDisplayPitch);
    //printf("%08lx DDPIXELFORMAT                       %sddpfDisplay          : 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, ddpfDisplay)+offset, str, lpVidmeminfo->ddpfDisplay);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwOffscreenAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwOffscreenAlign)+offset, buffer, lpVidmeminfo->dwOffscreenAlign);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwOverlayAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwOverlayAlign)+offset, buffer, lpVidmeminfo->dwOverlayAlign);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwTextureAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwTextureAlign)+offset, buffer, lpVidmeminfo->dwTextureAlign);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwZBufferAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwZBufferAlign)+offset, buffer, lpVidmeminfo->dwZBufferAlign);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwAlphaAlign\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwDisplayWidth)+offset, buffer, lpVidmeminfo->dwAlphaAlign);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwNumHeaps\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(VIDMEMINFO, dwNumHeaps)+offset, buffer, lpVidmeminfo->dwNumHeaps);

    MY_DUMP_STR(buffer,"LPVIDMEM\0",str,"pvmList\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(VIDMEMINFO, pvmList)+offset, buffer, lpVidmeminfo->pvmList);
}



/* Surface dump */

void dump_DDRAWI_DDRAWSURFACE_INT(char *str, LPDDRAWI_DDRAWSURFACE_INT lpDdrawSurface, DWORD offset)
{
    char buffer[2048];
    if (lpDdrawSurface == NULL)
        return ;

    MY_DUMP_STR(buffer,"LPVOID\0",str,"lpVtbl\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_INT, lpVtbl)+offset, buffer, lpDdrawSurface->lpVtbl);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_LCL\0",str,"lpLcl\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_INT, lpLcl)+offset, buffer, lpDdrawSurface->lpLcl);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"lpLink\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_INT, lpLink)+offset, str, lpDdrawSurface->lpLink);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwIntRefCnt\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_INT, dwIntRefCnt)+offset, str, lpDdrawSurface->dwIntRefCnt);

    printf("\n");
    sprintf(buffer,"%slpLcl->",str);
    dump_DDRAWI_DDRAWSURFACE_LCL(buffer, lpDdrawSurface->lpLcl, 0);

    sprintf(buffer,"%slpLcl->lpLink",str);
    dump_DDRAWI_DDRAWSURFACE_INT(buffer, lpDdrawSurface->lpLink, 0);
}

void dump_DDRAWI_DDRAWSURFACE_LCL(char *str, LPDDRAWI_DDRAWSURFACE_LCL lpDdrawSurface, DWORD offset)
{

    char buffer[2048];
    if (lpDdrawSurface == NULL)
        return ;

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_MORE\0",str,"lpSurfMore\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpSurfMore)+offset, buffer, lpDdrawSurface->lpSurfMore);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_GBL\0",str,"lpGbl\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpGbl)+offset, buffer, lpDdrawSurface->lpGbl);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDDSurface\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, hDDSurface)+offset, buffer, lpDdrawSurface->hDDSurface);

    MY_DUMP_STR(buffer,"LPATTACHLIST\0",str,"lpAttachList\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpAttachList)+offset, buffer, lpDdrawSurface->lpAttachList);

    MY_DUMP_STR(buffer,"LPATTACHLIST\0",str,"lpAttachListFrom\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpAttachListFrom)+offset, buffer, lpDdrawSurface->lpAttachListFrom);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwLocalRefCnt\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwLocalRefCnt)+offset, buffer, lpDdrawSurface->dwLocalRefCnt);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwProcessId\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwProcessId)+offset, buffer, lpDdrawSurface->dwProcessId);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwFlags\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwFlags)+offset, buffer, lpDdrawSurface->dwFlags);
    //printf("%08lx DDSCAPS                     %sddsCaps                 : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, ddsCaps)+offset, str, lpDdrawSurface->ddsCaps);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWPALETTE_INT\0",str,"lpDDPalette\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpDDPalette)+offset, buffer, lpDdrawSurface->lpDDPalette);
    /* note slpDDClipper have union to LPDDRAWI_DDRAWCLIPPER_INT, we need figout how to detect which are set */

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWCLIPPER_LCL\0",str,"lpDDClipper\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpDDClipper)+offset, buffer, lpDdrawSurface->lpDDClipper);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwModeCreatedIn\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwModeCreatedIn)+offset, buffer, lpDdrawSurface->dwModeCreatedIn);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwBackBufferCount\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwBackBufferCount)+offset, buffer, lpDdrawSurface->dwBackBufferCount);

    // printf("%08lx DDCOLORKEY                         %sddckCKDestBlt         : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, ddckCKDestBlt)+offset, str, lpDdrawSurface->ddckCKDestBlt);
    // printf("%08lx DDCOLORKEY                         %sddckCKSrcBlt       : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcBlt)+offset, str, lpDdrawSurface->ddckCKSrcBlt);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"hDC\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, hDC)+offset, buffer, lpDdrawSurface->hDC);

    MY_DUMP_STR(buffer,"ULONG_PTR\0",str,"dwReserved1\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwReserved1)+offset, buffer, lpDdrawSurface->dwReserved1);

    
    //printf("%08lx DDCOLORKEY                         %sddckCKSrcOverlay       : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcOverlay)+offset, str, lpDdrawSurface->ddckCKSrcOverlay);
    //printf("%08lx DDCOLORKEY                         %sddckCKDestOverlay       : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, ddckCKDestOverlay)+offset, str, lpDdrawSurface->ddckCKDestOverlay);

    MY_DUMP_STR(buffer,"LPDDRAWI_DDRAWSURFACE_INT\0",str,"lpSurfaceOverlaying\0");
    printf("%08lx %s: 0x%p\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lpSurfaceOverlaying)+offset, buffer, lpDdrawSurface->lpSurfaceOverlaying);

   // printf("%08lx DBLNODE                        %sdbnOverlayNode          : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dbnOverlayNode)+offset, str, lpDdrawSurface->dbnOverlayNode);
    //printf("%08lx RECT                        %srcOverlaySrc          : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, rcOverlaySrc)+offset, str, lpDdrawSurface->rcOverlaySrc);
   // printf("%08lx RECT                        %srcOverlayDest          : 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, rcOverlayDest)+offset, str, lpDdrawSurface->rcOverlayDest);

    MY_DUMP_STR(buffer,"DWORD\0",str,"dwClrXparent\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, dwClrXparent)+offset, buffer, lpDdrawSurface->dwClrXparent);

    MY_DUMP_STR(buffer,"LONG\0",str,"lOverlayX\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lOverlayX)+offset, buffer, lpDdrawSurface->lOverlayX);

    MY_DUMP_STR(buffer,"LONG\0",str,"lOverlayY\0");
    printf("%08lx %s: 0x%08lx\n", FIELD_OFFSET(DDRAWI_DDRAWSURFACE_LCL, lOverlayY)+offset, buffer, lpDdrawSurface->lOverlayY);
}





#endif

/* EOF */
