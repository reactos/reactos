#ifndef __WIN32K_CLASS_H
#define __WIN32K_CLASS_H

#include <windows.h>
#include <ddk/ntddk.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))


typedef struct _WNDCLASS_OBJECT
{
  WNDCLASSEX Class;
  BOOL Unicode;
  LIST_ENTRY ListEntry;
} WNDCLASS_OBJECT, *PWNDCLASS_OBJECT;


NTSTATUS
InitClassImpl(VOID);

NTSTATUS
CleanupClassImpl(VOID);

NTSTATUS
ClassReferenceClassByName(
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassName);

NTSTATUS
ClassReferenceClassByAtom(
  PWNDCLASS_OBJECT *Class,
  RTL_ATOM ClassAtom);

NTSTATUS
ClassReferenceClassByNameOrAtom(
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassNameOrAtom);

#endif /* __WIN32K_CLASS_H */

/* EOF */
