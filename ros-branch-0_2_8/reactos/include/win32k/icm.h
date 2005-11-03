
#ifndef __WIN32K_ICM_H
#define __WIN32K_ICM_H

BOOL
STDCALL
NtGdiCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count);

BOOL
STDCALL
NtGdiColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action);

HCOLORSPACE
STDCALL
NtGdiCreateColorSpace(LPLOGCOLORSPACEW  LogColorSpace);

BOOL
STDCALL
NtGdiDeleteColorSpace(HCOLORSPACE  hColorSpace);

INT
STDCALL
NtGdiEnumICMProfiles(HDC    hDC,
                    LPWSTR lpstrBuffer,
                    UINT   cch );

HCOLORSPACE
STDCALL
NtGdiGetColorSpace(HDC  hDC);

BOOL
STDCALL
NtGdiGetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp);

BOOL
STDCALL
NtGdiGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,
                        LPWSTR  Filename);

BOOL
STDCALL
NtGdiGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACEW  Buffer,
                           DWORD  Size);

HCOLORSPACE
STDCALL
NtGdiSetColorSpace(HDC  hDC,  
                               HCOLORSPACE  hColorSpace);

BOOL
STDCALL
NtGdiSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp);

INT
STDCALL
NtGdiSetICMMode(HDC  hDC,
                    INT  EnableICM);

BOOL
STDCALL
NtGdiSetICMProfile(HDC  hDC,
                        LPWSTR  Filename);

BOOL
STDCALL
NtGdiUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command);

#endif

