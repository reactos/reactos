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

#define PACKED __attribute__((packed))

#define EX_MAXIMUM_WAIT_OBJECTS (64)

#ifndef __USE_W32API

#define ANYSIZE_ARRAY	(1)

#define DELETE		(0x00010000L)
#define READ_CONTROL	(0x00020000L)
#define SYNCHRONIZE	(0x00100000L)

#define DUPLICATE_CLOSE_SOURCE	(1)
#define DUPLICATE_SAME_ACCESS	(2)

#define INVALID_HANDLE_VALUE	((HANDLE)-1)

#endif /* !__USE_W32API */

#endif /* __INCLUDE_NTDEF_H */
