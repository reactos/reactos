#ifndef _WIN32K_OBJECT_H
#define _WIN32K_OBJECT_H

#include "gdiobj.h"
#include "bitmaps.h"
#include "pen.h"

#define FIRST_USER_HANDLE 0x0020  /* first possible value for low word of user handle */
#define LAST_USER_HANDLE  0xffef  /* last possible value for low word of user handle */

#define HANDLEENTRY_INDESTROY 1

typedef struct _USER_HANDLE_ENTRY
{
    void          *ptr;          /* pointer to object */
    union
    {
        PVOID pi;
        PTHREADINFO pti;          // pointer to Win32ThreadInfo
        PPROCESSINFO ppi;         // pointer to W32ProcessInfo
    };
    unsigned char  type;         /* object type (0 if free) */
    unsigned char  flags;
    unsigned short generation;   /* generation counter */
} USER_HANDLE_ENTRY, * PUSER_HANDLE_ENTRY;



typedef struct _USER_HANDLE_TABLE
{
   PUSER_HANDLE_ENTRY handles;
   PUSER_HANDLE_ENTRY freelist;
   int nb_handles;
   int allocated_handles;
} USER_HANDLE_TABLE, * PUSER_HANDLE_TABLE;



typedef enum _USER_OBJECT_TYPE
{
  otFree = 0,
  otWindow,
  otMenu,
  otCursorIcon,
  otSMWP,
  otHook,
  otClipBoardData,
  otCallProc,
  otAccel,
  otDDEaccess,
  otDDEconv,
  otDDExact,
  otMonitor,
  otKBDlayout,
  otKBDfile,
  otEvent,
  otTimer,
  otInputContext,
  otHidData,
  otDeviceInfo,
  otTouchInput,
  otGestureInfo
} USER_OBJECT_TYPE;

typedef struct _USER_REFERENCE_ENTRY
{
   SINGLE_LIST_ENTRY Entry;
   PVOID obj;
} USER_REFERENCE_ENTRY, *PUSER_REFERENCE_ENTRY;

#include <malloc.h>

#define USER_ASSERT(exp,file,line) \
    if (!(exp)) {RtlAssert(#exp,(PVOID)file,line,"");}

static __inline VOID
UserAssertLastRef(PVOID obj, const char *file, int line)
{
    PTHREADINFO W32Thread;
    PSINGLE_LIST_ENTRY ReferenceEntry;
    PUSER_REFERENCE_ENTRY UserReferenceEntry;

    USER_ASSERT(obj != NULL, file, line);
    W32Thread = PsGetCurrentThreadWin32Thread();
    USER_ASSERT(W32Thread != NULL, file, line);
    ReferenceEntry = W32Thread->ReferencesList.Next;
    USER_ASSERT(ReferenceEntry != NULL, file, line);
    UserReferenceEntry = CONTAINING_RECORD(ReferenceEntry, USER_REFERENCE_ENTRY, Entry);
    USER_ASSERT(UserReferenceEntry != NULL, file, line);
    USER_ASSERT(obj == UserReferenceEntry->obj, file, line);
}
#define ASSERT_LAST_REF(_obj_) UserAssertLastRef(_obj,__FILE__,__LINE__)

#undef USER_ASSERT

extern PUSER_HANDLE_TABLE gHandleTable;
VOID FASTCALL UserReferenceObject(PVOID obj);
PVOID FASTCALL UserReferenceObjectByHandle(HANDLE handle, USER_OBJECT_TYPE type);
BOOL FASTCALL UserDereferenceObject(PVOID obj);
PVOID FASTCALL UserCreateObject(PUSER_HANDLE_TABLE ht, struct _DESKTOP* pDesktop, HANDLE* h,USER_OBJECT_TYPE type , ULONG size);
BOOL FASTCALL UserDeleteObject(HANDLE h, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type );
BOOL FASTCALL UserFreeHandle(PUSER_HANDLE_TABLE ht, HANDLE handle );
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type );
PUSER_HANDLE_ENTRY handle_to_entry(PUSER_HANDLE_TABLE ht, HANDLE handle );
BOOL FASTCALL UserCreateHandleTable(VOID);
VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes);


static __inline VOID
UserRefObjectCo(PVOID obj, PUSER_REFERENCE_ENTRY UserReferenceEntry)
{
    PTHREADINFO W32Thread;

    W32Thread = PsGetCurrentThreadWin32Thread();
    ASSERT(W32Thread != NULL);
    ASSERT(UserReferenceEntry != NULL);
    UserReferenceEntry->obj = obj;
    UserReferenceObject(obj);
    PushEntryList(&W32Thread->ReferencesList, &UserReferenceEntry->Entry);
}

static __inline VOID
UserDerefObjectCo(PVOID obj)
{
    PTHREADINFO W32Thread;
    PSINGLE_LIST_ENTRY ReferenceEntry;
    PUSER_REFERENCE_ENTRY UserReferenceEntry;

    ASSERT(obj != NULL);
    W32Thread = PsGetCurrentThreadWin32Thread();
    ASSERT(W32Thread != NULL);
    ReferenceEntry = PopEntryList(&W32Thread->ReferencesList);
    ASSERT(ReferenceEntry != NULL);
    UserReferenceEntry = CONTAINING_RECORD(ReferenceEntry, USER_REFERENCE_ENTRY, Entry);
    ASSERT(UserReferenceEntry != NULL);

    ASSERT(obj == UserReferenceEntry->obj);
    UserDereferenceObject(obj);
}

VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

#endif /* _WIN32K_OBJECT_H */

/* EOF */
