/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/battery/battc/battc.h
* PURPOSE:         Battery Class Driver
* PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
*/

#pragma once

#include <ntddk.h>
#include <initguid.h>
#include <batclass.h>

typedef struct _BATTERY_CLASS_DATA {
  BATTERY_MINIPORT_INFO MiniportInfo;
  KEVENT WaitEvent;
  BOOLEAN Waiting;
  FAST_MUTEX Mutex;
  UCHAR EventTrigger;
  PVOID EventTriggerContext;
  UNICODE_STRING InterfaceName;
} BATTERY_CLASS_DATA, *PBATTERY_CLASS_DATA;

/* Memory tags */
#define BATTERY_CLASS_DATA_TAG 'CtaB'

/* Event triggers */
#define EVENT_BATTERY_TAG    0x01
#define EVENT_BATTERY_STATUS 0x02
