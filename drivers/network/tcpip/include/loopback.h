/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/loopback.h
 * PURPOSE:     Loopback adapter definitions
 */

#pragma once

#include <lan.h>

extern PIP_INTERFACE Loopback;

NDIS_STATUS LoopRegisterAdapter(
    PNDIS_STRING AdapterName,
    PLAN_ADAPTER *Adapter);

NDIS_STATUS LoopUnregisterAdapter(
    PLAN_ADAPTER Adapter);

/* EOF */
