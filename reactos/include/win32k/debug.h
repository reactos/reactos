
#ifndef  H_WIN32K_DEBUG
#define  H_WIN32K_DEBUG

#ifdef DEBUG
#define INTERNAL_CALL STDCALL
#else
#define INTERNAL_CALL FASTCALL
#endif

#endif

