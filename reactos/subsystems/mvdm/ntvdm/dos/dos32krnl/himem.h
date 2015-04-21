/*
 * COPYRIGHT:       GPLv2+ - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            himem.h
 * PURPOSE:         DOS XMS Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* FUNCTIONS ******************************************************************/

BOOLEAN XmsGetDriverEntry(PDWORD Pointer);
VOID XmsInitialize(VOID);
VOID XmsCleanup(VOID);
