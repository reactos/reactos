
#ifndef __WIN32K_ICM_H
#define __WIN32K_ICM_H

BOOL
STDCALL
W32kCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count);

BOOL
STDCALL
W32kColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action);

HCOLORSPACE
STDCALL
W32kCreateColorSpace(LPLOGCOLORSPACE  LogColorSpace);

BOOL
STDCALL
W32kDeleteColorSpace(HCOLORSPACE  hColorSpace);

INT
STDCALL
W32kEnumICMProfiles(HDC  hDC,  
                         ICMENUMPROC  EnumICMProfilesFunc,
                         LPARAM lParam);

HCOLORSPACE
STDCALL
W32kGetColorSpace(HDC  hDC);

BOOL
STDCALL
W32kGetDeviceGammaRamp(HDC  hDC,  
                             LPVOID  Ramp);

BOOL
STDCALL
W32kGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,  
                        LPWSTR  Filename);

BOOL
STDCALL
W32kGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACE  Buffer,  
                           DWORD  Size);

HCOLORSPACE
STDCALL
W32kSetColorSpace(HDC  hDC,  
                               HCOLORSPACE  hColorSpace);

BOOL
STDCALL
W32kSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp);

INT
STDCALL
W32kSetICMMode(HDC  hDC,
                    INT  EnableICM);

BOOL
STDCALL
W32kSetICMProfile(HDC  hDC,
                        LPWSTR  Filename);

BOOL
STDCALL
W32kUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command);

#endif

