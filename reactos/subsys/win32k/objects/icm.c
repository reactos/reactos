

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/icm.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action)
{
  UNIMPLEMENTED;
}

HCOLORSPACE  W32kCreateColorSpace(LPLOGCOLORSPACE  LogColorSpace)
{
  UNIMPLEMENTED;
}

BOOL  W32kDeleteColorSpace(HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

INT  W32kEnumICMProfiles(HDC  hDC,  
                         ICMENUMPROC  EnumICMProfilesFunc,
                         LPARAM lParam)
{
  UNIMPLEMENTED;
}

HCOLORSPACE  W32kGetColorSpace(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetDeviceGammaRamp(HDC  hDC,  
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,  
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACE  Buffer,  
                           DWORD  Size)
{
  UNIMPLEMENTED;
}

HCOLORSPACE  W32kSetColorSpace(HDC  hDC,
                               HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

INT  W32kSetICMMode(HDC  hDC,
                    INT  EnableICM)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetICMProfile(HDC  hDC,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL  W32kUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command)
{
  UNIMPLEMENTED;
}


