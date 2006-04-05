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
  otCallProc
  
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

#define ASSERT_LAST_REF(_obj_) \
{ \
   PW32THREAD t; \
   PSINGLE_LIST_ENTRY e; \
   PUSER_REFERENCE_ENTRY ref; \
   \
   ASSERT(_obj_); \
   t = PsGetWin32Thread(); \
   ASSERT(t); \
   e = t->ReferencesList.Next; \
   ASSERT(e); \
   ref = CONTAINING_RECORD(e, USER_REFERENCE_ENTRY, Entry); \
   ASSERT(ref); \
   \
   ASSERT(_obj_ == ref->obj); \
   \
}
#define UserRefObjectCo(_obj_, _ref_) \
{ \
   PW32THREAD t; \
   \
   ASSERT(_obj_); \
   t = PsGetWin32Thread(); \
   ASSERT(t); \
   ASSERT(_ref_); \
   (_ref_)->obj = _obj_; \
   ObmReferenceObject(_obj_); \
 \
   PushEntryList(&t->ReferencesList, &(_ref_)->Entry); \
   \
}


#define UserDerefObjectCo(_obj_) \
{ \
   PW32THREAD t; \
   PSINGLE_LIST_ENTRY e; \
   PUSER_REFERENCE_ENTRY ref; \
   \
   ASSERT(_obj_); \
   t = PsGetWin32Thread(); \
   ASSERT(t); \
   e = PopEntryList(&t->ReferencesList); \
   ASSERT(e); \
   ref = CONTAINING_RECORD(e, USER_REFERENCE_ENTRY, Entry); \
   ASSERT(ref); \
   \
   ASSERT(_obj_ == ref->obj); \
   ObmDereferenceObject(_obj_); \
   \
}

HANDLE FASTCALL ObmObjectToHandle(PVOID obj);

VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

#endif /* _WIN32K_OBJECT_H */

/* EOF */
