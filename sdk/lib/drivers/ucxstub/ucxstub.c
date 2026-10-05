/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX client binding for drivers built in tree
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/*
 * Windows clients get bound by the KMDF stub walking .kmdfclassbind.
 * wdfdriverentry has no such walk, so clients call in here instead.
 */

#include <ntddk.h>
#include <windef.h>
#include <fxldr.h>
#include <wdf.h>
#include <ucxclass.h>
#include "ucxstub.h"

#define NDEBUG
#include <debug.h>

/* The UCX header set this library is built against */
#define UCX_STUB_MAJOR_VERSION 1
#define UCX_STUB_MINOR_VERSION 6

/* Owned by wdfdriverentry */
extern WDF_BIND_INFO BindInfo;
extern PWDF_DRIVER_GLOBALS WdfDriverGlobals;

PFN_UCXFUNC UcxFunctions[UcxFunctionTableNumEntries];
PUCX_DRIVER_GLOBALS UcxDriverGlobals;

static WDF_CLASS_BIND_INFO UcxStubClassBindInfo;
static BOOLEAN UcxStubBound;

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxStubBindClass(VOID)
{
    PWDF_CLASS_BIND_INFO Info = &UcxStubClassBindInfo;
    NTSTATUS Status;

    PAGED_CODE();

    if (UcxStubBound)
        return STATUS_SUCCESS;

    RtlZeroMemory(Info, sizeof(*Info));
    Info->Size = sizeof(*Info);
    Info->ClassName = (PWCHAR)L"Ucx";
    Info->Version.Major = UCX_STUB_MAJOR_VERSION;
    Info->Version.Minor = UCX_STUB_MINOR_VERSION;
    Info->FunctionTable = (VOID (NTAPI **)(VOID))UcxFunctions;
    Info->FunctionTableCount = UcxFunctionTableNumEntries;
    Info->ClassBindInfo = &UcxDriverGlobals;

    /* The class receives the globals pointer itself, matching what Windows clients pass */
    Status = WdfVersionBindClass(&BindInfo, (PWDF_COMPONENT_GLOBALS *)WdfDriverGlobals, Info);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UCX class bind failed 0x%lx\n", Status);
        return Status;
    }

    UcxStubBound = TRUE;
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxStubUnbindClass(VOID)
{
    PAGED_CODE();

    if (!UcxStubBound)
        return;

    WdfVersionUnbindClass(&BindInfo, (PWDF_COMPONENT_GLOBALS)WdfDriverGlobals, &UcxStubClassBindInfo);

    UcxStubBound = FALSE;
    UcxDriverGlobals = NULL;
    RtlZeroMemory(UcxFunctions, sizeof(UcxFunctions));
}
