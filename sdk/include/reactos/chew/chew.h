/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/reactos/chew/chew.h
 * PURPOSE:         Common Highlevel Executive Worker
 *
 * PROGRAMMERS:     arty (ayerkes@speakeasy.net)
 */

#ifndef _REACTOS_CHEW_H
#define _REACTOS_CHEW_H

/**
 * Initialize CHEW, given a device object (since IoAllocateWorkItem relies on
 * it).
 */
VOID ChewInit(PDEVICE_OBJECT DeviceObject);

/**
 * Shutdown CHEW, waits for remaining work items.
 */
VOID ChewShutdown(VOID);

/**
 * Creates and queues a work item.
 */
BOOLEAN ChewCreate(VOID (*Worker)(PVOID), PVOID WorkerContext);

#endif/*_REACTOS_CHEW_H*/
