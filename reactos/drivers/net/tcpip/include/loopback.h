/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/loopback.h
 * PURPOSE:     Loopback adapter definitions
 */
#ifndef __LOOPBACK_H
#define __LOOPBACK_H

#include <lan.h>


extern PIP_INTERFACE Loopback;


NDIS_STATUS LoopRegisterAdapter(
    PNDIS_STRING AdapterName,
    PLAN_ADAPTER *Adapter);

NDIS_STATUS LoopUnregisterAdapter(
    PLAN_ADAPTER Adapter);

#endif /* __LOOPBACK_H */

/* EOF */
