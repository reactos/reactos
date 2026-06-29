#pragma once 

#include <stdio.h>

#include <acpi.h>
#include <kernel_api.h>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>

#include <initguid.h>
#include <ntddk.h>
#include <ntifs.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>

UINT32
ACPIInitUACPI(void);
