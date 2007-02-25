#include <rpc.h>
#include <rpcndr.h>

#ifndef _PROPIDL_H
#define _PROPIDL_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <objidl.h>

#define PRSPEC_LPWSTR (0)
#define PRSPEC_PROPID (1)

#define PID_DICTIONARY 0x00000000
#define PID_CODEPAGE 0x00000001
#define PID_FIRST_USABLE 0x00000002
#define PID_FIRST_NAME_DEFAULT 0x00000fff
#define PID_LOCALE 0x80000000
#define PID_MODIFY_TIME 0x80000001
#define PID_SECURITY 0x80000002
#define PID_BEHAVIOR 0x80000003
#define PID_ILLEGAL 0xffffffff
#define PID_MIN_READONLY 0x80000000
#define PID_MAX_READONLY 0xbfffffff

#define PROPSETFLAG_DEFAULT 0
#define PROPSETFLAG_NONSIMPLE 1
#define PROPSETFLAG_ANSI 2
#define PROPSETFLAG_UNBUFFERED 4
#define PROPSETFLAG_CASE_SENSITIVE 8

#define CCH_MAX_PROPSTG_NAME 31

/* Macros for dwOSVersion member of STATPROPSETSTG */
#define PROPSETHDR_OSVER_KIND(dwOSVer)  HIWORD((dwOSVer))
#define PROPSETHDR_OSVER_MAJOR(dwOSVer) LOBYTE(LOWORD((dwOSVer)))
#define PROPSETHDR_OSVER_MINOR(dwOSVer) HIBYTE(LOWORD((dwOSVer)))
#define PROPSETHDR_OSVERSION_UNKNOWN    0xffffffff

HRESULT WINAPI FreePropVariantArray(ULONG cVariants, PROPVARIANT *rgvars);
HRESULT WINAPI PropVariantClear(PROPVARIANT*);
HRESULT WINAPI PropVariantCopy(PROPVARIANT*,const PROPVARIANT*);

#define _PROPVARIANT_INIT_DEFINED_
#define PropVariantInit(p) memset((p), 0, sizeof(PROPVARIANT))

HRESULT WINAPI FmtIdToPropStgName(const FMTID *, LPOLESTR);

#endif
