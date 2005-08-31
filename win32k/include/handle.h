#ifndef _WIN32K_HANDLE_H
#define _WIN32K_HANDLE_H

#define FIRST_USER_HANDLE 0x0020  /* first possible value for low word of user handle */
#define LAST_USER_HANDLE  0xffef  /* last possible value for low word of user handle */


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
  /* 0 = free */
  otWindow = 1,
  otMenu,
  otAccel,
  otCursor,
  otHook,
  otMonitor
  
} USER_OBJECT_TYPE;

#endif /* _WIN32K_HANDLE_H */

/* EOF */
