#ifndef __INCLUDE_DDK_NTDEF_H
#define __INCLUDE_DDK_NTDEF_H

struct _KTHREAD;
struct _ETHREAD;
struct _EPROCESS;

#ifndef NTKERNELAPI
#define NTKERNELAPI STDCALL
#endif

#ifndef NTSYSAPI
#define NTSYSAPI STDCALL
#endif

#ifndef NTAPI
#define NTAPI STDCALL
#endif

#define MINCHAR   (0x80)
#define MAXCHAR   (0x7F)
#define MINSHORT  (0x8000)
#define MAXSHORT  (0x7FFF)
#define MINLONG   (0x80000000)
#define MAXLONG   (0x7FFFFFFF)
#define MAXUCHAR  (0xFF)
#define MAXUSHORT (0xFFFF)
#define MAXULONG  (0xFFFFFFFF)

#endif
