

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/icm.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL
STDCALL
W32kCheckColorsInGamut(HDC  hDC,
                             LPVOID  RGBTriples,
                             LPVOID  Buffer,
                             UINT  Count)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget, 
                             DWORD  Action)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kCreateColorSpace(LPLOGCOLORSPACE  LogColorSpace)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kDeleteColorSpace(HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEnumICMProfiles(HDC  hDC,  
                         ICMENUMPROC  EnumICMProfilesFunc,
                         LPARAM lParam)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kGetColorSpace(HDC  hDC)
{
  /* FIXME: Need to to whatever GetColorSpace actually does */
  return  0;
}

BOOL
STDCALL
W32kGetDeviceGammaRamp(HDC  hDC,  
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetICMProfile(HDC  hDC,  
                        LPDWORD  NameSize,  
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACE  Buffer,  
                           DWORD  Size)
{
  UNIMPLEMENTED;
}

HCOLORSPACE
STDCALL
W32kSetColorSpace(HDC  hDC,
                               HCOLORSPACE  hColorSpace)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kSetICMMode(HDC  hDC,
                    INT  EnableICM)
{
  /* FIXME: this should be coded someday  */
  if (EnableICM == ICM_OFF) 
    {
      return  ICM_OFF;
    }
  if (EnableICM == ICM_ON) 
    {
      return  0;
    }
  if (EnableICM == ICM_QUERY) 
    {
      return  ICM_OFF;
    }
  
  return  0;
}

BOOL
STDCALL
W32kSetICMProfile(HDC  hDC,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kUpdateICMRegKey(DWORD  Reserved,  
                          LPWSTR  CMID, 
                          LPWSTR  Filename,
                          UINT  Command)
{
  UNIMPLEMENTED;
}


