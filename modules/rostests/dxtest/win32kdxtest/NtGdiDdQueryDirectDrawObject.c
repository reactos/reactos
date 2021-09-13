#include <stdio.h>
/* SDK/DDK/NDK Headers. */
#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <dll/directx/d3d8thk.h>
#include "test.h"

extern BOOL dumping_on;

/* my struct */
struct
{
    DWORD pos;
} *mytest;

/*
 * Test see if we can setup DirectDrawObject
 *
 */

/*
 * ToDO
 * 1. add more testcase it is only some, but we do not test for all case
 *    that happen only some
 *
 * 2.Fixed the false alaret for drivers only support 2d dx interface
 *
 * 3. fixed the dumping of d3d struct.
 *
 */

void
test_NtGdiDdQueryDirectDrawObject( HANDLE hDirectDrawLocal)
{
    int fails=0;
    BOOL retValue=FALSE;

    DD_HALINFO *pHalInfo = NULL;
    DWORD *pCallBackFlags = NULL;
    LPD3DNTHAL_CALLBACKS puD3dCallbacks = NULL;
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData = NULL;
    PDD_D3DBUFCALLBACKS puD3dBufferCallbacks = NULL;
    LPDDSURFACEDESC puD3dTextureFormats = NULL;
    DWORD *puNumHeaps = NULL;
    VIDEOMEMORY *puvmList = NULL;
    DWORD *puNumFourCC = NULL;
    DWORD *puFourCC = NULL;

    DD_HALINFO HalInfo;
    DWORD CallBackFlags[4];
    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DDSURFACEDESC2 D3dTextureFormats[100];
    VIDEOMEMORY vmList;

    /* clear data */
    memset(&vmList,0,sizeof(VIDEOMEMORY));
    memset(&D3dTextureFormats,0,sizeof(DDSURFACEDESC));
    memset(&D3dBufferCallbacks,0,sizeof(DD_D3DBUFCALLBACKS));
    memset(&D3dDriverData,0,sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    memset(&D3dCallbacks,0,sizeof(D3DNTHAL_CALLBACKS));
    memset(&HalInfo,0,sizeof(DD_HALINFO));
    memset(CallBackFlags,0,sizeof(DWORD)*3);

    printf("Start testing of NtGdiDdQueryDirectDrawObject\n");

    /* testing  OsThunkDdQueryDirectDrawObject( NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( NULL, ....)\n");

    retValue = OsThunkDdQueryDirectDrawObject( NULL, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"1. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"2. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"3. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"4. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"5. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"6. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"7. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"8. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"9. NtGdiDdQueryDirectDrawObject(NULL, ...);\0");

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, NULL, ....)\n");

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"10. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pHalInfo,NULL,fails,"11. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"12. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"13. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"14. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"15. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"16. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"17. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"18. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, NULL, ...);\0");

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, NULL, ....)\n");

    pHalInfo = &HalInfo;
    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"19. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"20. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(pCallBackFlags,NULL,fails,"21. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"22. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"23. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"24. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"25. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"26. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"27. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\0");

    /*
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("28. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...);\n");
        fails++;
    }
    */

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, NULL, ...)");
    }













    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"29. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"30. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"31. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dCallbacks,NULL,fails,"32. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dDriverData,NULL,fails,"33. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"34. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"35. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"36. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"37. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\0");
    /*
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("38. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...);\n");
        fails++;
    }
    */

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, NULL, ...)");
    }

    /* testing  OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo, pCallBackFlags, D3dCallbacks, ....  */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"39. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"40. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"41. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");


    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"42. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");

    testing_noteq(puD3dDriverData,NULL,fails,"43. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puD3dBufferCallbacks,NULL,fails,"44. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"45. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"46. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"47. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");
    /*
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("48. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\n");
        fails++;
    }
    */

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...)");
        dump_D3dCallbacks(puD3dCallbacks,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...)");
    }

   /* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"49. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"50. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"51. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"52. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"53. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, NULL, ...);\0");

    testing_noteq(puD3dBufferCallbacks,NULL,fails,"54. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puD3dTextureFormats,NULL,fails,"55. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"56. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"57. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\0");
    /*
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("58. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...);\n");
        fails++;
    }
    */

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...)");
        dump_D3dCallbacks(puD3dCallbacks,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...)");
        dump_D3dDriverData(puD3dDriverData,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, NULL, ...)");
    }

/* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    RtlZeroMemory(pHalInfo,sizeof(DD_HALINFO));
    RtlZeroMemory(pCallBackFlags,sizeof(DWORD)*3);
    RtlZeroMemory(puD3dCallbacks,sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(puD3dDriverData,sizeof(D3DNTHAL_CALLBACKS));

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);

    testing_noteq(retValue,FALSE,fails,"59. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"60. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"61. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"62. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"63. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dBufferCallbacks, NULL, ...);\0");

    testing_noteq(puD3dTextureFormats,NULL,fails,"64. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal,  pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puNumFourCC,NULL,fails,"65. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"66. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\0");
    /*
    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
    {
        printf("67. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...);\n");
        fails++;
    }
    */

    if (puD3dBufferCallbacks)
    {
        testing_noteq(puD3dBufferCallbacks->dwSize,sizeof(DD_D3DBUFCALLBACKS),fails,"68. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL...);\0");
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...)");
        dump_D3dCallbacks(puD3dCallbacks,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...)");
        dump_D3dDriverData(puD3dDriverData, "NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...)");
        dump_D3dBufferCallbacks(puD3dBufferCallbacks, "NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, NULL, ...)");
    }

/* testing  OsThunkDdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, puD3dTextureFormats, NULL, */
    printf("testing  DdQueryDirectDrawObject( hDD, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, D3dBufferCallbacks, puD3dTextureFormats, NULL, ....)\n");

    pHalInfo = &HalInfo;
    pCallBackFlags = CallBackFlags;
    puD3dCallbacks = &D3dCallbacks;
    puD3dDriverData = &D3dDriverData;
    puD3dBufferCallbacks = &D3dBufferCallbacks;

    if (puD3dDriverData)
    {
          puD3dTextureFormats = malloc (puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
          if (!puD3dTextureFormats)
            printf("Waring Out of memory\n");

          RtlZeroMemory(puD3dTextureFormats, puD3dDriverData->dwNumTextureFormats * sizeof(DDSURFACEDESC2));
    }

    retValue = OsThunkDdQueryDirectDrawObject( hDirectDrawLocal, pHalInfo,
                                                pCallBackFlags, puD3dCallbacks,
                                                puD3dDriverData, puD3dBufferCallbacks,
                                                puD3dTextureFormats, puNumHeaps,
                                                puvmList, puNumFourCC,
                                                puFourCC);





    testing_noteq(retValue,FALSE,fails,"69. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_eq(pHalInfo,NULL,fails,"70. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_eq(pCallBackFlags,NULL,fails,"71. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

    /* does not work nice in xp */
    // testing_noteq(puD3dCallbacks->dwSize,sizeof(D3DNTHAL_CALLBACKS),fails,"72. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

    testing_noteq(puD3dDriverData->dwSize,sizeof(D3DNTHAL_GLOBALDRIVERDATA),fails,"73. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

    testing_noteq(puNumFourCC,NULL,fails,"74. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");
    testing_noteq(puFourCC,NULL,fails,"75. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\0");

//    /*
//    if ((pHalInfo->dwSize != sizeof(DD_HALINFO)) &&
//        (pHalInfo->dwSize != sizeof(DD_HALINFO_V4)))
//    {
//        printf("8. if this show for NT 2000/XP/2003 ignore it, NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...);\n");
//        fails++;
//    }
//    */
//
    if (puD3dBufferCallbacks)
    {

        testing_noteq(puD3dBufferCallbacks->dwSize,sizeof(DD_D3DBUFCALLBACKS),fails,"76. NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL...);\0");
    }

    if (puD3dTextureFormats)
    {
        /* fixme test case for it */
    }

    if (dumping_on == TRUE)
    {
        dump_halinfo(pHalInfo,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
        dump_CallBackFlags(pCallBackFlags,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
        dump_D3dCallbacks(puD3dCallbacks,"NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
        dump_D3dDriverData(puD3dDriverData, "NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
        dump_D3dBufferCallbacks(puD3dBufferCallbacks, "NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
        dump_D3dTextureFormats(puD3dTextureFormats, puD3dDriverData->dwNumTextureFormats, "NtGdiDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, puD3dBufferCallbacks, puD3dTextureFormats, NULL, ...)");
    }





    if (puD3dTextureFormats)
        free (puD3dTextureFormats);
    show_status(fails, "NtGdiDdQueryDirectDrawObject\0");
}
