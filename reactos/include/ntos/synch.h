/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/synch.h
 * PURPOSE:      Synchronization declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */

#ifndef __INCLUDE_SYNCH_H
#define __INCLUDE_SYNCH_H


#define EVENT_ALL_ACCESS	(0x1f0003L)
#define EVENT_QUERY_STATE	(1)
#define EVENT_MODIFY_STATE	(2)
#define EVENT_PAIR_ALL_ACCESS	(0x1f0000L)
#define MUTEX_ALL_ACCESS	(0x1f0001L)
#define MUTEX_QUERY_STATE	(1)
#define MUTANT_ALL_ACCESS	(0x1f0001L)
#define MUTANT_QUERY_STATE	(1)
#define SEMAPHORE_ALL_ACCESS	(0x1f0003L)
#define SEMAPHORE_QUERY_STATE	(1)
#define SEMAPHORE_MODIFY_STATE	(2)
#define TIMER_ALL_ACCESS	(0x1f0003L)
#define TIMER_QUERY_STATE	(1)
#define TIMER_MODIFY_STATE	(2)


#endif /* __INCLUDE_SYNCH_H */
