/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/synch.h
 * PURPOSE:      Synchronization declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_SYNCH_H
#define __INCLUDE_SYNCH_H

#define MUTEX_ALL_ACCESS	(0x1f0001L)
#define MUTEX_MODIFY_STATE	(1)
#define SEMAPHORE_ALL_ACCESS	(0x1f0003L)
#define SEMAPHORE_MODIFY_STATE	(2)
#define EVENT_ALL_ACCESS	(0x1f0003L)
#define EVENT_MODIFY_STATE	(2)


#endif /* __INCLUDE_PS_H */
