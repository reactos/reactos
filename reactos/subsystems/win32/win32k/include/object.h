#ifndef _WIN32K_OBJECT_H
#define _WIN32K_OBJECT_H

#include "gdiobj.h"
#include "bitmaps.h"
#include "pen.h"

#define FIRST_USER_HANDLE 0x0020  /* first possible value for low word of user handle */
#define LAST_USER_HANDLE  0xffef  /* last possible value for low word of user handle */


#define USER_HEADER_TO_BODY(ObjectHeader) \
  ((PVOID)(((PUSER_OBJECT_HEADER)ObjectHeader) + 1))

#define USER_BODY_TO_HEADER(ObjectBody) \
  ((PUSER_OBJECT_HEADER)(((PUSER_OBJECT_HEADER)ObjectBody) - 1))



typedef struct _USER_HANDLE_ENTRY
{
    void          *ptr;          /* pointer to object */
    unsigned short type;         /* object type (0 if free) */
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
  otAccel,
  otCursorIcon,
  otHook,
  otMonitor,
  otClass //fixme: remove
  
} USER_OBJECT_TYPE;


typedef struct _USER_OBJECT_HEADER
/*
 * Header for user object
 */
{
//  USER_OBJECT_TYPE Type;
  LONG RefCount;
  BOOL destroyed;
  HANDLE hSelf;
//  CSHORT Size;
} USER_OBJECT_HEADER, *PUSER_OBJECT_HEADER;


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
    PW32THREAD W32Thread;
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

VOID FASTCALL ObmReferenceObject(PVOID obj);
BOOL FASTCALL ObmDereferenceObject2(PVOID obj);

static __inline VOID
UserRefObjectCo(PVOID obj, PUSER_REFERENCE_ENTRY UserReferenceEntry)
{
    PW32THREAD W32Thread;

    W32Thread = PsGetCurrentThreadWin32Thread();
    ASSERT(W32Thread != NULL);
    ASSERT(UserReferenceEntry != NULL);
    UserReferenceEntry->obj = obj;
    ObmReferenceObject(obj);
    PushEntryList(&W32Thread->ReferencesList, &UserReferenceEntry->Entry);
}

static __inline VOID
UserDerefObjectCo(PVOID obj)
{
    PW32THREAD W32Thread;
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
    ObmDereferenceObject2(obj);
}

VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

#endif /* _WIN32K_OBJECT_H */

/* EOF */
