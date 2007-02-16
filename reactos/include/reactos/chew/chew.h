/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/chew/chew.h
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
VOID ChewInit( PDEVICE_OBJECT DeviceObject );
/**
 * Shutdown CHEW, including removing remaining work items.
 */
VOID ChewShutdown();
/**
 * Create a work item, or perform the work, based on IRQL.
 * At passive level, Worker is called directly on UserSpace.
 * At greater than passive level, a work item is created with Bytes
 * context area and data copied from UserSpace.
 * If a work item is created, Item contains the address and the function
 * returns true.
 * If the work is performed immediately, Item contains NULL and the
 * function returns true.
 * Else, the function returns false and Item is undefined.
 */
BOOLEAN ChewCreate
( PVOID *Item, SIZE_T Bytes, VOID (*Worker)(PVOID), PVOID UserSpace );
/**
 * Remove a work item, given the pointer returned to Item in ChewCreate.
 */
VOID ChewRemove( PVOID Item );

#endif/*_REACTOS_CHEW_H*/
