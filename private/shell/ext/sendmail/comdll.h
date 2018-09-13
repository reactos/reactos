#ifndef _INC_COMDLL
#define _INC_COMDLL

#include <windows.h>

// helper macros...
#define _IOffset(class, itf)         ((UINT_PTR)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf)))

// standard DLL goo...
extern HANDLE g_hinst;
STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

#endif