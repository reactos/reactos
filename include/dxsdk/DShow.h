
#ifndef __DSHOW_INCLUDED__
#define __DSHOW_INCLUDED__

#ifdef  _MSC_VER
  #pragma warning(disable:4100)
  #pragma warning(disable:4201)
  #pragma warning(disable:4511)
  #pragma warning(disable:4512)
  #pragma warning(disable:4514)
  #if _MSC_VER>=1100
    #define AM_NOVTABLE __declspec(novtable)
  #else
    #define AM_NOVTABLE
  #endif
#endif

#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <ddraw.h>
#include <mmsystem.h>

#ifndef NO_DSHOW_STRSAFE
  #define NO_SHLWAPI_STRFCNS
#include <strsafe.h>
#endif

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#include <strmif.h>
#include <amvideo.h>
#include <amaudio.h>
#include <control.h>
#include <evcode.h>
#include <uuids.h>
#include <errors.h>
#include <edevdefs.h>
#include <audevcod.h>
#include <dvdevcod.h>

#ifndef InterlockedExchangePointer
  #define InterlockedExchangePointer(Target, Value) (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))
#endif

#ifndef OATRUE
  #define OATRUE (-1)
#endif

#ifndef OAFALSE
  #define OAFALSE (0)
#endif

#endif

