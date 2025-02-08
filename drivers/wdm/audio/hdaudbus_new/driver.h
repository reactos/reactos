#if !defined(_SKLHDAUDBUS_H_)
#define _SKLHDAUDBUS_H_

#define POOL_ZERO_DOWN_LEVEL_SUPPORT

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <ntddk.h>
#include <initguid.h>

//SGPC (Win10 1809+ support)
extern "C" {
	#define __CPLUSPLUS

	#include <windef.h>
	#include <winerror.h>

	#include <wingdi.h>
	#include <d3dkmddi.h>
	#include <d3dkmthk.h>
}

#include <wdm.h>
#include <wdmguid.h>
#include <wdf.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <hdaudio.h>
#include <portcls.h>

#include "hda_registers.h"
#include "fdo.h"
#include "buspdo.h"
#include "hdac_controller.h"
#include "hdac_stream.h"
#include "hda_verbs.h"

#define DRIVERNAME "sklhdaudbus.sys: "
#define SKLHDAUDBUS_POOL_TAG 'SADH'

#define VEN_INTEL 0x8086
#define VEN_ATI 0x1002
#define VEN_AMD 0x1022
#define VEN_NVIDIA 0x10DE
#define VEN_VMWARE 0x15AD

#include "regfuncs.h"

HDAUDIO_BUS_INTERFACE HDA_BusInterface(PVOID Context);
HDAUDIO_BUS_INTERFACE_V2 HDA_BusInterfaceV2(PVOID Context);
HDAUDIO_BUS_INTERFACE_V3 HDA_BusInterfaceV3(PVOID Context);

#define IS_BXT(ven, dev) (ven == VEN_INTEL && dev == 0x5a98)

static inline void mdelay(LONG msec) {
	LARGE_INTEGER Interval;
	Interval.QuadPart = -10 * 1000 * (LONGLONG)msec;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

static inline void udelay(LONG usec) {
	LARGE_INTEGER Interval;
	Interval.QuadPart = -10 * (LONGLONG)usec;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

//
// Helper macros
//

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_IOCTL 4

static ULONG SklHdAudBusDebugLevel = 100;
static ULONG SklHdAudBusDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

#if 0
#define SklHdAudBusPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (SklHdAudBusDebugLevel >= dbglevel &&                         \
        (SklHdAudBusDebugCatagories && dbgcatagory))                 \
		    {                                                           \
        DbgPrint(DRIVERNAME);                                   \
        DbgPrint(fmt, __VA_ARGS__);                             \
		    }                                                           \
}
#else
#define SklHdAudBusPrint(dbglevel, fmt, ...) {                       \
}
#endif
#endif