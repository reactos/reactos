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

#define EX_MAXIMUM_WAIT_OBJECTS (64)

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

/* Helpers for easy conversion to system time units (100ns) */
#define ABSOLUTE_TIME(wait) (wait)
#define RELATIVE_TIME(wait) (-(wait))
#define NANOS_TO_100NS(nanos) (((LONGLONG)(nanos)) / 100L)
#define MICROS_TO_100NS(micros) (((LONGLONG)(micros)) * NANOS_TO_100NS(1000L))
#define MILLIS_TO_100NS(milli) (((LONGLONG)(milli)) * MICROS_TO_100NS(1000L))
#define SECONDS_TO_100NS(seconds) (((LONGLONG)(seconds)) * MILLIS_TO_100NS(1000L))


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
