#ifndef __INCLUDE_DDK_NTDEF_H
#define __INCLUDE_DDK_NTDEF_H

struct _KTHREAD;
struct _ETHREAD;
struct _EPROCESS;

#ifndef NTKERNELAPI
#define NTKERNELAPI STDCAL
#endif

#ifndef NTSYSAPI
#define NTSYSAPI STDCALL
#endif

#ifndef NTAPI
#define NTAPI STDCALL
#endif

#endif
