/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntdef.h
 * PURPOSE:      Defines used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */

#ifndef __INCLUDE_NTDEF_H
#define __INCLUDE_NTDEF_H

#ifndef __USE_W32API
#define MAXIMUM_WAIT_OBJECTS (64)
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define TYPE_ALIGNMENT(t) __alignof(t)
#elif defined(__GNUC__)
# define TYPE_ALIGNMENT(t) __alignof__(t)
#else
# define TYPE_ALIGNMENT(t) FIELD_OFFSET(struct { char x; t test; }, test)
#endif

#ifdef _WIN64
# define PROBE_ALIGNMENT(_s) \
    (TYPE_ALIGNMENT(_s) > TYPE_ALIGNMENT(DWORD) ? \
    TYPE_ALIGNMENT(_s) : TYPE_ALIGNMENT(DWORD))
# define PROBE_ALIGNMENT32(_s) TYPE_ALIGNMENT(DWORD)
#else
# define PROBE_ALIGNMENT(_s) TYPE_ALIGNMENT(DWORD)
#endif


#if defined(_MSC_VER)
   #define Const64(a) (a##i64)
   static __inline LARGE_INTEGER MK_LARGE_INTEGER(__int64 i)
   {
      LARGE_INTEGER li;
      li.QuadPart = i;
      return li;
   }
   #define INIT_LARGE_INTEGER(a) {{(ULONG)a, (LONG)(a>>32)}}   
#elif defined (__GNUC__)
   #define Const64(a) (a##LL)
   #define MK_LARGE_INTEGER(a) ((LARGE_INTEGER)(LONGLONG)(a))
   #define INIT_LARGE_INTEGER(a) MK_LARGE_INTEGER(Const64(a))    
#endif

#define LargeInteger0 MK_LARGE_INTEGER(0)


/* Compile time asserts */
#define C_ASSERT(e) do{extern char __C_ASSERT__[(e)?1:-1]; (void)__C_ASSERT__;}while(0)


/* Helpers for easy conversion to system time units (100ns) */
#define ABSOLUTE_TIME(wait) (wait)
#define RELATIVE_TIME(wait) (-(wait))
#define NANOS_TO_100NS(nanos) (((LONGLONG)(nanos)) / 100)
#define MICROS_TO_100NS(micros) (((LONGLONG)(micros)) * NANOS_TO_100NS(1000))
#define MILLIS_TO_100NS(milli) (((LONGLONG)(milli)) * MICROS_TO_100NS(1000))
#define SECONDS_TO_100NS(seconds) (((LONGLONG)(seconds)) * MILLIS_TO_100NS(1000))
#define MINUTES_TO_100NS(minutes) (((LONGLONG)(minutes)) * SECONDS_TO_100NS(60))
#define HOURS_TO_100NS(hours) (((LONGLONG)(hours)) * MINUTES_TO_100NS(60))




/* Helpers for enumarating lists */
#define LIST_FOR_EACH(entry, head) \
   for((entry) = (head)->Flink; (entry) != (head); (entry) = (entry)->Flink)

/* 
Safe version which saves pointer to the next entry so the current ptr->field entry
can be unlinked without corrupting the list. NOTE: Never unlink tmp_entry!!!!!!!!!
*/
#define LIST_FOR_EACH_SAFE(tmp_entry, head, ptr, type, field) \
   for ((tmp_entry)=(head)->Flink; (tmp_entry)!=(head) && \
        ((ptr) = CONTAINING_RECORD(tmp_entry,type,field)) && \
        ((tmp_entry) = (tmp_entry)->Flink); )


#ifndef __USE_W32API

#define ANYSIZE_ARRAY	(1)

#define DELETE		(0x00010000L)
#define READ_CONTROL	(0x00020000L)
#define WRITE_DAC	(0x00040000L)
#define WRITE_OWNER	(0x00080000L)
#define SYNCHRONIZE	(0x00100000L)

#define DUPLICATE_CLOSE_SOURCE	(1)
#define DUPLICATE_SAME_ACCESS	(2)

#define INVALID_HANDLE_VALUE	((HANDLE)-1)

#endif /* !__USE_W32API */

#endif /* __INCLUDE_NTDEF_H */
