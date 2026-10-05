/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common declarations for ucx01000
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <ntstrsafe.h>
#include <wdmguid.h>
#include <fxldr.h>
#include <wdf.h>
#include <ucxclass.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <wmistr.h>
#include <drivers/usb3/hubucx.h>
#include <drivers/usb3/usbdclient.h>

#include "ucxutil.h"

class UcxController;
class UcxRootHub;
class UcxUsbDevice;
class UcxEndpoint;
class UcxStaticStreams;
class UcxUsbdHandle;

#include "crsm.h"
#include "epsm.h"
#include "hostctl.h"
#include "rhub.h"
#include "pipe.h"
#include "devobj.h"
#include "urbroute.h"
#include "clientxrb.h"
#include "clientif.h"
#include "userreq.h"

#define UCX_CLASS_MAJOR_VERSION 1
#define UCX_CLASS_MINOR_VERSION 7

extern "C" WDF_CLASS_LIBRARY_INFO UcxClassLibraryInfo;


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClassLoad(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClassUnload(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClientBind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS* ComponentGlobals);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClientUnbind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS* ComponentGlobals);

extern PFN_UCXFUNC UcxExportTable[UcxFunctionTableNumEntries];

VOID
NTAPI
UcxBuildExportTable(VOID);

/* Reference and dereference handed out with every query interface */
VOID
NTAPI
UcxInterfaceNoOp(
    _In_ PVOID Context);
