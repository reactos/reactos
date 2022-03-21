/*
 * This file contains debug support procedures, common for NDIS5 and NDIS6
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "ndis56common.h"
#include "stdarg.h"
#include "ntstrsafe.h"

//#define OVERRIDE_DEBUG_BREAK

#ifdef WPP_EVENT_TRACING
#include "ParaNdis-Debug.tmh"
#endif

int virtioDebugLevel = 1;
int nDebugLevel = 1;
int bDebugPrint = 1;

static NDIS_SPIN_LOCK CrashLock;

static KBUGCHECK_REASON_CALLBACK_ROUTINE ParaNdis_OnBugCheck;
static VOID NTAPI ParaNdis_OnBugCheck(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
);
static VOID ParaNdis_PrepareBugCheckData();

typedef BOOLEAN (NTAPI *KeRegisterBugCheckReasonCallbackType) (
    __out PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    __in PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    __in KBUGCHECK_CALLBACK_REASON Reason,
    __in PUCHAR Component
    );

typedef BOOLEAN (NTAPI *KeDeregisterBugCheckReasonCallbackType) (
    __inout PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    );

typedef ULONG (NTAPI *vDbgPrintExType)(
    __in ULONG ComponentId,
    __in ULONG Level,
    __in PCCH Format,
    __in va_list arglist
    );

static ULONG NTAPI DummyPrintProcedure(
    __in ULONG ComponentId,
    __in ULONG Level,
    __in PCCH Format,
    __in va_list arglist
    )
{
    return 0;
}
static BOOLEAN NTAPI KeRegisterBugCheckReasonCallbackDummyProc(
    __out PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    __in PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    __in KBUGCHECK_CALLBACK_REASON Reason,
    __in PUCHAR Component
    )
{
    CallbackRecord->State = 0;
    return FALSE;
}

BOOLEAN NTAPI KeDeregisterBugCheckReasonCallbackDummyProc(
    __inout PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    )
{
    return FALSE;
}

static vDbgPrintExType PrintProcedure = DummyPrintProcedure;
static KeRegisterBugCheckReasonCallbackType BugCheckRegisterCallback = KeRegisterBugCheckReasonCallbackDummyProc;
static KeDeregisterBugCheckReasonCallbackType BugCheckDeregisterCallback = KeDeregisterBugCheckReasonCallbackDummyProc;
KBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord;

#if !defined(WPP_EVENT_TRACING) || defined(WPP_USE_BYPASS)
#if defined(DPFLTR_MASK)

//common case, except Win2K
static void __cdecl DebugPrint(const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    PrintProcedure(DPFLTR_DEFAULT_ID, 9 | DPFLTR_MASK, fmt, list);
#if defined(VIRTIO_DBG_USE_IOPORT)
    {
        NTSTATUS status;
        // use this way of output only for DISPATCH_LEVEL,
        // higher requires more protection
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
        {
            char buf[256];
            size_t len, i;
            buf[0] = 0;
            status = RtlStringCbVPrintfA(buf, sizeof(buf), fmt, list);
            if (status == STATUS_SUCCESS) len = strlen(buf);
            else if (status == STATUS_BUFFER_OVERFLOW) len = sizeof(buf);
            else { memcpy(buf, "Can't print", 11); len = 11; }
            NdisAcquireSpinLock(&CrashLock);
            for (i = 0; i < len; ++i)
            {
                NdisRawWritePortUchar(VIRTIO_DBG_USE_IOPORT, buf[i]);
            }
            NdisRawWritePortUchar(VIRTIO_DBG_USE_IOPORT, '\n');
            NdisReleaseSpinLock(&CrashLock);
        }
    }
#endif
}

DEBUGPRINTFUNC pDebugPrint = DebugPrint;
DEBUGPRINTFUNC VirtioDebugPrintProc = DebugPrint;

#else //DPFLTR_MASK
#pragma message("DebugPrint for Win2K")

DEBUGPRINTFUNC pDebugPrint = DbgPrint;
DEBUGPRINTFUNC VirtioDebugPrintProc = DbgPrint;

#endif //DPFLTR_MASK
#endif //!defined(WPP_EVENT_TRACING) || defined(WPP_USE_BYPASS)



void _LogOutEntry(int level, const char *s)
{
    DPrintf(level, ("[%s]=>", s));
}

void _LogOutExitValue(int level, const char *s, ULONG value)
{
    DPrintf(level, ("[%s]<=0x%X", s, value));
}

void _LogOutString(int level, const char *s)
{
    DPrintf(level, ("[%s]", s));
}

VOID WppEnableCallback(
    __in LPCGUID Guid,
    __in __int64 Logger,
    __in BOOLEAN Enable,
    __in ULONG Flags,
    __in UCHAR Level)
{
#if WPP_USE_BYPASS
    DPrintfBypass(0, ("[%s] %s, flags %X, level %d",
        __FUNCTION__, Enable ? "enabled" : "disabled",
        Flags, (ULONG)Level));
#endif
    nDebugLevel = Level;
    bDebugPrint = Enable;
}


#ifdef OVERRIDE_DEBUG_BREAK
static PUCHAR pDbgBreakPoint;
static UCHAR DbgBreakPointChunk[5];
static void AnotherDbgBreak()
{
    DPrintf(0, ("Somebody tried to break into the debugger!"));
}
#endif

void ParaNdis_DebugInitialize(PVOID DriverObject,PVOID RegistryPath)
{
    NDIS_STRING usRegister, usDeregister, usPrint;
    PVOID pr, pd;
    BOOLEAN res;
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    NdisAllocateSpinLock(&CrashLock);
    KeInitializeCallbackRecord(&CallbackRecord);
    ParaNdis_PrepareBugCheckData();
    NdisInitUnicodeString(&usPrint, L"vDbgPrintEx");
    NdisInitUnicodeString(&usRegister, L"KeRegisterBugCheckReasonCallback");
    NdisInitUnicodeString(&usDeregister, L"KeDeregisterBugCheckReasonCallback");
    pd = MmGetSystemRoutineAddress(&usPrint);
    if (pd) PrintProcedure = (vDbgPrintExType)pd;
    pr = MmGetSystemRoutineAddress(&usRegister);
    pd = MmGetSystemRoutineAddress(&usDeregister);
    if (pr && pd)
    {
        BugCheckRegisterCallback = (KeRegisterBugCheckReasonCallbackType)pr;
        BugCheckDeregisterCallback = (KeDeregisterBugCheckReasonCallbackType)pd;
    }
    res = BugCheckRegisterCallback(&CallbackRecord, ParaNdis_OnBugCheck, KbCallbackSecondaryDumpData, "NetKvm");
    DPrintf(0, ("[%s] Crash callback %sregistered", __FUNCTION__, res ? "" : "NOT "));

#ifdef OVERRIDE_DEBUG_BREAK
    if (sizeof(PVOID) == sizeof(ULONG))
    {
        UCHAR replace[5] = {0xe9,0,0,0,0};
        ULONG replacement;
        NDIS_STRING usDbgBreakPointName;
        NdisInitUnicodeString(&usDbgBreakPointName, L"DbgBreakPoint");
        pDbgBreakPoint = (PUCHAR)MmGetSystemRoutineAddress(&usDbgBreakPointName);
        if (pDbgBreakPoint)
        {
            DPrintf(0, ("Replacing original BP handler at %p", pDbgBreakPoint));
            replacement = RtlPointerToOffset(pDbgBreakPoint + 5, AnotherDbgBreak);
            RtlCopyMemory(replace + 1, &replacement, sizeof(replacement));
            RtlCopyMemory(DbgBreakPointChunk, pDbgBreakPoint, sizeof(DbgBreakPointChunk));
            RtlCopyMemory(pDbgBreakPoint, replace, sizeof(replace));
        }
    }
#endif
}

void ParaNdis_DebugCleanup(PDRIVER_OBJECT  pDriverObject)
{
#ifdef OVERRIDE_DEBUG_BREAK
    if (sizeof(PVOID) == sizeof(ULONG) && pDbgBreakPoint)
    {
        DPrintf(0, ("Restoring original BP handler at %p", pDbgBreakPoint));
        RtlCopyMemory(pDbgBreakPoint, DbgBreakPointChunk, sizeof(DbgBreakPointChunk));
    }
#endif
    BugCheckDeregisterCallback(&CallbackRecord);
    WPP_CLEANUP(pDriverObject);
}


#define MAX_CONTEXTS    4
#if defined(ENABLE_HISTORY_LOG)
#define MAX_HISTORY     0x40000
#else
#define MAX_HISTORY     2
#endif
typedef struct _tagBugCheckStaticData
{
    tBugCheckStaticDataHeader Header;
    tBugCheckPerNicDataContent PerNicData[MAX_CONTEXTS];
    tBugCheckStaticDataContent Data;
    tBugCheckHistoryDataEntry  History[MAX_HISTORY];
}tBugCheckStaticData;


typedef struct _tagBugCheckData
{
    tBugCheckStaticData     StaticData;
    tBugCheckDataLocation   Location;
}tBugCheckData;

static tBugCheckData BugCheckData;
static BOOLEAN bNative = TRUE;

VOID ParaNdis_PrepareBugCheckData()
{
    BugCheckData.StaticData.Header.StaticDataVersion = PARANDIS_DEBUG_STATIC_DATA_VERSION;
    BugCheckData.StaticData.Header.PerNicDataVersion = PARANDIS_DEBUG_PER_NIC_DATA_VERSION;
    BugCheckData.StaticData.Header.ulMaxContexts = MAX_CONTEXTS;
    BugCheckData.StaticData.Header.SizeOfPointer = sizeof(PVOID);
    BugCheckData.StaticData.Header.PerNicData = (UINT_PTR)(PVOID)BugCheckData.StaticData.PerNicData;
    BugCheckData.StaticData.Header.DataArea = (UINT64)&BugCheckData.StaticData.Data;
    BugCheckData.StaticData.Header.DataAreaSize = sizeof(BugCheckData.StaticData.Data);
    BugCheckData.StaticData.Data.HistoryDataVersion = PARANDIS_DEBUG_HISTORY_DATA_VERSION;
    BugCheckData.StaticData.Data.SizeOfHistory = MAX_HISTORY;
    BugCheckData.StaticData.Data.SizeOfHistoryEntry = sizeof(tBugCheckHistoryDataEntry);
    BugCheckData.StaticData.Data.HistoryData = (UINT_PTR)(PVOID)BugCheckData.StaticData.History;
    BugCheckData.Location.Address = (UINT64)&BugCheckData;
    BugCheckData.Location.Size = sizeof(BugCheckData);
}

void ParaNdis_DebugRegisterMiniport(PARANDIS_ADAPTER *pContext, BOOLEAN bRegister)
{
    UINT i;
    NdisAcquireSpinLock(&CrashLock);
    for (i = 0; i < MAX_CONTEXTS; ++i)
    {
        UINT64 val1 = bRegister ? 0 : (UINT_PTR)pContext;
        UINT64 val2 = bRegister ? (UINT_PTR)pContext : 0;
        if (BugCheckData.StaticData.PerNicData[i].Context != val1) continue;
        BugCheckData.StaticData.PerNicData[i].Context = val2;
        break;
    }
    NdisReleaseSpinLock(&CrashLock);
}

static UINT FillDataOnBugCheck()
{
    UINT i, n = 0;
    NdisGetCurrentSystemTime(&BugCheckData.StaticData.Header.qCrashTime);
    for (i = 0; i < MAX_CONTEXTS; ++i)
    {
        tBugCheckPerNicDataContent *pSave = &BugCheckData.StaticData.PerNicData[i];
        PARANDIS_ADAPTER *p = (PARANDIS_ADAPTER *)pSave->Context;
        if (!p) continue;
        pSave->nofPacketsToComplete = p->NetTxPacketsToReturn;
        pSave->nofReadyTxBuffers = p->nofFreeHardwareBuffers;
        pSave->LastInterruptTimeStamp.QuadPart = PARANDIS_GET_LAST_INTERRUPT_TIMESTAMP(p);
        pSave->LastTxCompletionTimeStamp = p->LastTxCompletionTimeStamp;
        ParaNdis_CallOnBugCheck(p);
        ++n;
    }
    return n;
}

VOID NTAPI ParaNdis_OnBugCheck(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    )
{
    KBUGCHECK_SECONDARY_DUMP_DATA *pDump = (KBUGCHECK_SECONDARY_DUMP_DATA *)ReasonSpecificData;
    if (KbCallbackSecondaryDumpData == Reason && ReasonSpecificDataLength >= sizeof(*pDump))
    {
        ULONG dumpSize = sizeof(BugCheckData.Location);
        if (!pDump->OutBuffer)
        {
            UINT nSaved;
            nSaved = FillDataOnBugCheck();
            if (pDump->InBufferLength >= dumpSize)
            {
                pDump->OutBuffer = pDump->InBuffer;
                pDump->OutBufferLength = dumpSize;
            }
            else
            {
                pDump->OutBuffer = &BugCheckData.Location;
                pDump->OutBufferLength = dumpSize;
                bNative = FALSE;
            }
            DPrintf(0, ("[%s] system buffer of %d, saving data for %d NIC", __FUNCTION__,pDump->InBufferLength, nSaved));
            DPrintf(0, ("[%s] using %s buffer", __FUNCTION__, bNative ? "native" : "own"));
        }
        else if (pDump->OutBuffer == pDump->InBuffer)
        {
            RtlCopyMemory(&pDump->Guid, &ParaNdis_CrashGuid, sizeof(pDump->Guid));
            RtlCopyMemory(pDump->InBuffer, &BugCheckData.Location, dumpSize);
            pDump->OutBufferLength = dumpSize;
            DPrintf(0, ("[%s] written %d to %p", __FUNCTION__, (ULONG)BugCheckData.Location.Size, (UINT_PTR)BugCheckData.Location.Address ));
            DPrintf(0, ("[%s] dump data (%d) at %p", __FUNCTION__, pDump->OutBufferLength, pDump->OutBuffer));
        }
    }
}

#if defined(ENABLE_HISTORY_LOG)
void ParaNdis_DebugHistory(
    PARANDIS_ADAPTER *pContext,
    eHistoryLogOperation op,
    PVOID pParam1,
    ULONG lParam2,
    ULONG lParam3,
    ULONG lParam4)
{
    tBugCheckHistoryDataEntry *phe;
    ULONG index = InterlockedIncrement(&BugCheckData.StaticData.Data.CurrentHistoryIndex);
    index = (index - 1) % MAX_HISTORY;
    phe = &BugCheckData.StaticData.History[index];
    phe->Context = (UINT_PTR)pContext;
    phe->operation = op;
    phe->pParam1 = (UINT_PTR)pParam1;
    phe->lParam2 = lParam2;
    phe->lParam3 = lParam3;
    phe->lParam4 = lParam4;
#if (PARANDIS_DEBUG_HISTORY_DATA_VERSION == 1)
    phe->uIRQL = KeGetCurrentIrql();
    phe->uProcessor = KeGetCurrentProcessorNumber();
#endif
    NdisGetCurrentSystemTime(&phe->TimeStamp);
}

#endif
