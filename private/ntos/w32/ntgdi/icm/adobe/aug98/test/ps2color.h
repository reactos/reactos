#ifndef PS2COLOR_H
#define PS2COLOR_H
BOOL GetPS2ColorRenderingDictionary(
    CHANDLE     cp,
    DWORD       Intent, 
    MEMPTR      lpMem, 
    LPDWORD     lpcbSize,
    BOOL        AllowBinary);
BOOL    GetPS2ColorSpaceArray(
    CHANDLE     cp, 
    DWORD       InputIntent, 
    DWORD       InpDrvClrSp,  
    MEMPTR      lpBuffer, 
    LPDWORD     lpcbSize, 
    BOOL        AllowBinary);
BOOL    EXTERN GetPS2ColorRenderingIntent(
    CHANDLE     cp, 
    DWORD       Intent,
    MEMPTR      lpMem, 
    LPDWORD     lpcbSize);
#endif
