#ifndef _INC_OSCALLS
#define _INC_OSCALLS

#include <_mingw.h>

#ifdef NULL
#undef NULL
#endif

#define NOMINMAX

#define _WIN32_FUSION 0x0100
#include <windows.h>

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

typedef struct _FTIME
{
  unsigned short twosecs : 5;
  unsigned short minutes : 6;
  unsigned short hours : 5;
} FTIME;

typedef FTIME *PFTIME;

typedef struct _FDATE
{
  unsigned short day : 5;
  unsigned short month : 4;
  unsigned short year : 7;
} FDATE;

typedef FDATE *PFDATE;

#endif
