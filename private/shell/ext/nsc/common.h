// common.h

#include "debug.h"

// to avoid CRT in debug
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#pragma intrinsic(abs)

#define ARRAYSIZE(a)	(sizeof(a)/sizeof(a[0]))

#define StrToOleStrN(pwsz, cchWideChar,  psz,  cchMultiByte) MultiByteToWideChar(CP_ACP, 0, psz, cchMultiByte, pwsz, cchWideChar)
#define OleStrToStrN(psz,  cchMultiByte, pwsz, cchWideChar)  WideCharToMultiByte(CP_ACP, 0, pwsz, cchWideChar, psz, cchMultiByte, NULL, NULL)

#define QueryInterface(punk, iid, pobj)	(punk)->lpVtbl->QueryInterface(punk, iid, pobj)
#define AddRef(punk)			(punk)->lpVtbl->AddRef(punk)
#define Release(punk)			(punk)->lpVtbl->Release(punk)

#ifndef IToClass

#define _IOffset(class, itf)         ((UINT)(UINT_PTR)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf)))

#endif

#ifdef _DEBUG

#define ReleaseLast(punk)   Assert(Release(punk) == 0)

#else

#define ReleaseLast(punk)   Release(punk)

#endif