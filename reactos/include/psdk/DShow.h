
#ifndef __DSHOW_INCLUDED__
#define __DSHOW_INCLUDED__

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#if _MSC_VER>=1100
#define AM_NOVTABLE __declspec(novtable)
#else
#define AM_NOVTABLE
#endif
	

#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <ddraw.h>
#include <mmsystem.h>

#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#include <strmif.h>
#include <amvideo.h>
#include <amaudio.h>
//#include <control.h>
#include <evcode.h> 
#include <uuids.h>  
#include <errors.h> 
#include <edevdefs.h> 
#include <audevcod.h> 
#include <dvdevcod.h> 

#ifndef OATRUE
#define OATRUE (-1)
#endif 
#ifndef OAFALSE
#define OAFALSE (0)
#endif 


#ifndef InterlockedExchangePointer
#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))
#endif 


#endif 
