
#ifndef __WIN32K_ICM_H
#define __WIN32K_ICM_H

BOOL  W32kCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count);

BOOL  W32kColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action);

HCOLORSPACE  W32kCreateColorSpace(LPLOGCOLORSPACE  LogColorSpace);

BOOL  W32kDeleteColorSpace(HCOLORSPACE  hColorSpace);

INT  W32kEnumICMProfiles(HDC  hDC,  
                         ICMENUMPROC  EnumICMProfilesFunc,
                         LPARAM lParam);

HCOLORSPACE  W32kGetColorSpace(HDC  hDC);

BOOL  W32kGetDeviceGammaRamp(HDC  hDC,  
                             LPVOID  Ramp);

BOOL  W32kGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,  
                        LPWSTR  Filename);

BOOL  W32kGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACE  Buffer,  
                           DWORD  Size);

HCOLORSPACE  W32kSetColorSpace(HDC  hDC,  
                               HCOLORSPACE  hColorSpace);

BOOL  W32kSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp);

INT  W32kSetICMMode(HDC  hDC,
                    INT  EnableICM);

BOOL  W32kSetICMProfile(HDC  hDC,
                        LPWSTR  Filename);

BOOL  W32kUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command);

#endif

