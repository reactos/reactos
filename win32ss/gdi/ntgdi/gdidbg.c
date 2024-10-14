/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/gdidbg.c
 * PURPOSE:         Special debugging functions for GDI
 * PROGRAMMERS:     Timo Kreuzer
 */

/** INCLUDES ******************************************************************/

#if DBG
#include <win32k.h>
#define NDEBUG
#include <debug.h>

extern ULONG gulFirstFree;
extern ULONG gulFirstUnused;
extern PENTRY gpentHmgr;

ULONG gulLogUnique = 0;

/* Note: the following values need to be sorted */
DBG_CHANNEL DbgChannels[DbgChCount] = {
    {L"EngBlt", DbgChEngBlt},
    {L"EngBrush", DbgChEngBrush},
    {L"EngClip", DbgChEngClip},
    {L"EngCursor", DbgChEngCursor},
    {L"EngDev", DbgChEngDev},
    {L"EngErr", DbgChEngErr},
    {L"EngEvent", DbgChEngEvent},
    {L"EngGrad", DbgChEngGrad},
    {L"EngLDev", DbgChEngLDev},
    {L"EngLine", DbgChEngLine},
    {L"EngMapping", DbgChEngMapping},
    {L"EngMDev", DbgChEngMDev},
    {L"EngPDev", DbgChEngPDev},
    {L"EngSurface", DbgChEngSurface},
    {L"EngWnd", DbgChEngWnd},
    {L"EngXlate", DbgChEngXlate},
    {L"GdiBitmap", DbgChGdiBitmap},
    {L"GdiBlt", DbgChGdiBlt},
    {L"GdiBrush", DbgChGdiBrush},
    {L"GdiClipRgn", DbgChGdiClipRgn},
    {L"GdiCoord", DbgChGdiCoord},
    {L"GdiDC", DbgChGdiDC},
    {L"GdiDCAttr", DbgChGdiDCAttr},
    {L"GdiDCState", DbgChGdiDCState},
    {L"GdiDev", DbgChGdiDev},
    {L"GdiDib", DbgChGdiDib},
    {L"GdiFont", DbgChGdiFont},
    {L"GdiLine", DbgChGdiLine},
    {L"GdiObj", DbgChGdiObj},
    {L"GdiPalette", DbgChGdiPalette},
    {L"GdiPath", DbgChGdiPath},
    {L"GdiPen", DbgChGdiPen},
    {L"GdiPool", DbgChGdiPool},
    {L"GdiRgn", DbgChGdiRgn},
    {L"GdiText", DbgChGdiText},
    {L"GdiXFormObj", DbgChGdiXFormObj},
    {L"UserAccel", DbgChUserAccel},
    {L"UserCallback", DbgChUserCallback},
    {L"UserCallProc", DbgChUserCallProc},
    {L"UserCaret", DbgChUserCaret},
    {L"UserClass", DbgChUserClass},
    {L"UserClipbrd", DbgChUserClipbrd},
    {L"UserCsr", DbgChUserCsr},
    {L"UserDce", DbgChUserDce},
    {L"UserDefwnd", DbgChUserDefwnd},
    {L"UserDesktop", DbgChUserDesktop},
    {L"UserDisplay",DbgChUserDisplay},
    {L"UserEvent", DbgChUserEvent},
    {L"UserFocus", DbgChUserFocus},
    {L"UserHook", DbgChUserHook},
    {L"UserHotkey", DbgChUserHotkey},
    {L"UserIcon", DbgChUserIcon},
    {L"UserInput", DbgChUserInput},
    {L"UserKbd", DbgChUserKbd},
    {L"UserKbdLayout", DbgChUserKbdLayout},
    {L"UserMenu", DbgChUserMenu},
    {L"UserMetric", DbgChUserMetric},
    {L"UserMisc", DbgChUserMisc},
    {L"UserMonitor", DbgChUserMonitor},
    {L"UserMsg", DbgChUserMsg},
    {L"UserMsgQ", DbgChUserMsgQ},
    {L"UserObj", DbgChUserObj},
    {L"UserPainting", DbgChUserPainting},
    {L"UserProcess", DbgChUserProcess},
    {L"UserProp", DbgChUserProp},
    {L"UserScrollbar", DbgChUserScrollbar},
    {L"UserSecurity", DbgChUserSecurity},
    {L"UserShutdown", DbgChUserShutdown},
    {L"UserSysparams", DbgChUserSysparams},
    {L"UserTimer", DbgChUserTimer},
    {L"UserThread", DbgChUserThread},
    {L"UserWinpos", DbgChUserWinpos},
    {L"UserWinsta", DbgChUserWinsta},
    {L"UserWnd", DbgChUserWnd}
};

ULONG
NTAPI
DbgCaptureStackBackTace(
    _Out_writes_(cFramesToCapture) PVOID* ppvFrames,
    _In_ ULONG cFramesToSkip,
    _In_ ULONG cFramesToCapture)
{
    ULONG cFrameCount;
    PVOID apvTemp[30];
    NT_ASSERT(cFramesToCapture <= _countof(apvTemp));

    /* Zero it out */
    RtlZeroMemory(ppvFrames, cFramesToCapture * sizeof(PVOID));

    /* Capture kernel stack */
    cFrameCount = RtlWalkFrameChain(apvTemp, _countof(apvTemp), 0);

    /* If we should skip more than we have, we are done */
    if (cFramesToSkip > cFrameCount)
        return 0;

    /* Copy, but skip frames */
    cFrameCount -= cFramesToSkip;
    cFrameCount = min(cFrameCount, cFramesToCapture);
    RtlCopyMemory(ppvFrames, &apvTemp[cFramesToSkip], cFrameCount * sizeof(PVOID));

    /* Check if there is still space left */
    if (cFrameCount < cFramesToCapture)
    {
        /* Capture user stack */
        cFrameCount += RtlWalkFrameChain(&ppvFrames[cFrameCount],
                                         cFramesToCapture - cFrameCount,
                                         1);
    }

    return cFrameCount;
}

#if DBG_ENABLE_GDIOBJ_BACKTRACES

static
BOOL
CompareBacktraces(
    USHORT idx1,
    USHORT idx2)
{
    POBJ pobj1, pobj2;
    ULONG iLevel;

    /* Get the objects */
    pobj1 = gpentHmgr[idx1].einfo.pobj;
    pobj2 = gpentHmgr[idx2].einfo.pobj;

    /* Loop all stack levels */
    for (iLevel = 0; iLevel < GDI_OBJECT_STACK_LEVELS; iLevel++)
    {
        /* If one level doesn't match we are done */
        if (pobj1->apvBackTrace[iLevel] != pobj2->apvBackTrace[iLevel])
        {
            return FALSE;
        }
    }

    return TRUE;
}

typedef struct
{
    USHORT idx;
    USHORT iCount;
} GDI_DBG_HANDLE_BT;

VOID
NTAPI
DbgDumpGdiHandleTableWithBT(void)
{
    static BOOL bLeakReported = FALSE;
    ULONG idx, j;
    BOOL bAlreadyPresent;
    GDI_DBG_HANDLE_BT aBacktraceTable[GDI_DBG_MAX_BTS];
    USHORT iCount;
    KIRQL OldIrql;
    POBJ pobj;
    ULONG iLevel, ulObj;

    /* Only report once */
    if (bLeakReported)
    {
        DPRINT1("GDI handle abusers already reported!\n");
        return;
    }

    bLeakReported = TRUE;
    DPRINT1("Reporting GDI handle abusers:\n");

    /* Zero out the table */
    RtlZeroMemory(aBacktraceTable, sizeof(aBacktraceTable));

    /* We've got serious business to do */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Step through GDI handle table and find out who our culprit is... */
    for (idx = RESERVE_ENTRIES_COUNT; idx < GDI_HANDLE_COUNT; idx++)
    {
        /* If the handle is free, continue */
        if (gpentHmgr[idx].einfo.pobj == 0) continue;

        /* Check if this backtrace is already covered */
        bAlreadyPresent = FALSE;
        for (j = RESERVE_ENTRIES_COUNT; j < idx; j++)
        {
            if (CompareBacktraces(idx, j))
            {
                bAlreadyPresent = TRUE;
                break;
            }
        }

        if (bAlreadyPresent) continue;

        /* We don't have this BT yet, count how often it is present */
        iCount = 1;
        for (j = idx + 1; j < GDI_HANDLE_COUNT; j++)
        {
            if (CompareBacktraces(idx, j))
            {
                iCount++;
            }
        }

        /* Now add this backtrace */
        for (j = 0; j < GDI_DBG_MAX_BTS; j++)
        {
            /* Insert it below the next smaller count */
            if (aBacktraceTable[j].iCount < iCount)
            {
                /* Check if there are entries above */
                if (j < GDI_DBG_MAX_BTS - 1)
                {
                    /* Move the following entries up by 1 */
                    RtlMoveMemory(&aBacktraceTable[j],
                                  &aBacktraceTable[j + 1],
                                  GDI_DBG_MAX_BTS - j - 1);
                }

                /* Set this entry */
                aBacktraceTable[j].idx = idx;
                aBacktraceTable[j].iCount = iCount;

                /* We are done here */
                break;
            }
        }
    }

    /* Print the worst offenders... */
    DbgPrint("Count Handle   Backtrace\n");
    DbgPrint("------------------------------------------------\n");
    for (j = 0; j < GDI_DBG_MAX_BTS; j++)
    {
        idx = aBacktraceTable[j].idx;
        if (idx == 0)
            break;

        ulObj = ((ULONG)gpentHmgr[idx].FullUnique << 16) | idx;
        pobj = gpentHmgr[idx].einfo.pobj;

        DbgPrint("%5d %08lx ", aBacktraceTable[j].iCount, ulObj);
        for (iLevel = 0; iLevel < GDI_OBJECT_STACK_LEVELS; iLevel++)
        {
            DbgPrint("%p,", pobj->apvBackTrace[iLevel]);
        }
        DbgPrint("\n");
    }

    __debugbreak();

    KeLowerIrql(OldIrql);
}

#endif /* DBG_ENABLE_GDIOBJ_BACKTRACES */

#if DBG

BOOL
NTAPI
DbgGdiHTIntegrityCheck(VOID)
{
	ULONG i, nDeleted = 0, nFree = 0, nUsed = 0;
	PGDI_TABLE_ENTRY pEntry;
	BOOL r = 1;

	KeEnterCriticalRegion();

	/* FIXME: Check reserved entries */

	/* Now go through the deleted objects */
	i = gulFirstFree & 0xffff;
	while (i)
	{
		pEntry = &GdiHandleTable->Entries[i];
		if (i >= GDI_HANDLE_COUNT)
		{
		    DPRINT1("nDeleted=%lu\n", nDeleted);
		    ASSERT(FALSE);
		}

        nDeleted++;

        /* Check the entry */
        if ((pEntry->Type & GDI_ENTRY_BASETYPE_MASK) != 0)
        {
            r = 0;
            DPRINT1("Deleted Entry has a type != 0\n");
        }
        if ((ULONG_PTR)pEntry->KernelData >= GDI_HANDLE_COUNT)
        {
            r = 0;
            DPRINT1("Deleted entries KernelPointer too big\n");
        }
        if (pEntry->UserData != NULL)
        {
            r = 0;
            DPRINT1("Deleted entry has UserData != 0\n");
        }
        if (pEntry->ProcessId != 0)
        {
            r = 0;
            DPRINT1("Deleted entry has ProcessId != 0\n");
        }

        i = (ULONG_PTR)pEntry->KernelData & 0xffff;
	};

	for (i = gulFirstUnused;
	     i < GDI_HANDLE_COUNT;
	     i++)
	{
		pEntry = &GdiHandleTable->Entries[i];

		if ((pEntry->Type) != 0)
		{
			r = 0;
			DPRINT1("Free Entry has a type != 0\n");
		}
		if ((ULONG_PTR)pEntry->KernelData != 0)
		{
			r = 0;
			DPRINT1("Free entries KernelPointer != 0\n");
		}
		if (pEntry->UserData != NULL)
		{
			r = 0;
			DPRINT1("Free entry has UserData != 0\n");
		}
		if (pEntry->ProcessId != 0)
		{
			r = 0;
			DPRINT1("Free entry has ProcessId != 0\n");
		}
		nFree++;
	}

	for (i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++)
	{
		HGDIOBJ Handle;
		ULONG Type;

		pEntry = &GdiHandleTable->Entries[i];
		Type = pEntry->Type;
		Handle = (HGDIOBJ)(ULONG_PTR)((Type << GDI_ENTRY_UPPER_SHIFT) + i);

		if (Type & GDI_ENTRY_BASETYPE_MASK)
		{
			if (pEntry->KernelData == NULL)
			{
				r = 0;
				DPRINT1("Used entry has KernelData == 0\n");
			}
			else if (pEntry->KernelData <= MmHighestUserAddress)
			{
				r = 0;
				DPRINT1("Used entry invalid KernelData\n");
			}
			else if (((POBJ)(pEntry->KernelData))->hHmgr != Handle)
			{
				r = 0;
				DPRINT1("Used entry %lu, has invalid hHmg %p (expected: %p)\n",
				        i, ((POBJ)(pEntry->KernelData))->hHmgr, Handle);
			}
			nUsed++;
		}
	}

	if (RESERVE_ENTRIES_COUNT + nDeleted + nFree + nUsed != GDI_HANDLE_COUNT)
	{
		r = 0;
		DPRINT1("Number of all entries incorrect: RESERVE_ENTRIES_COUNT = %lu, nDeleted = %lu, nFree = %lu, nUsed = %lu\n",
		        RESERVE_ENTRIES_COUNT, nDeleted, nFree, nUsed);
	}

	KeLeaveCriticalRegion();

	return r;
}

#endif /* DBG */


#if DBG_ENABLE_EVENT_LOGGING

VOID
NTAPI
DbgLogEvent(PSLIST_HEADER pslh, LOG_EVENT_TYPE nEventType, LPARAM lParam)
{
    PLOGENTRY pLogEntry;

    /* Log a maximum of 100 events */
    if (QueryDepthSList(pslh) >= 1000) return;

    /* Allocate a logentry */
    pLogEntry = EngAllocMem(0, sizeof(LOGENTRY), 'golG');
    if (!pLogEntry) return;

    /* Set type */
    pLogEntry->nEventType = nEventType;
    pLogEntry->ulUnique = InterlockedIncrement((LONG*)&gulLogUnique);
    pLogEntry->dwProcessId = HandleToUlong(PsGetCurrentProcessId());
    pLogEntry->dwThreadId = HandleToUlong(PsGetCurrentThreadId());
    pLogEntry->lParam = lParam;

    /* Capture a backtrace */
    DbgCaptureStackBackTace(pLogEntry->apvBackTrace, 1, 20);

    switch (nEventType)
    {
        case EVENT_ALLOCATE:
        case EVENT_CREATE_HANDLE:
        case EVENT_REFERENCE:
        case EVENT_DEREFERENCE:
        case EVENT_LOCK:
        case EVENT_UNLOCK:
        case EVENT_DELETE:
        case EVENT_FREE:
        case EVENT_SET_OWNER:
        default:
            break;
    }

    /* Push it on the list */
    InterlockedPushEntrySList(pslh, &pLogEntry->sleLink);
}

#define REL_ADDR(va) ((ULONG_PTR)va - (ULONG_PTR)&__ImageBase)

VOID
NTAPI
DbgPrintEvent(PLOGENTRY pLogEntry)
{
    PSTR pstr;

    switch (pLogEntry->nEventType)
    {
        case EVENT_ALLOCATE: pstr = "Allocate"; break;
        case EVENT_CREATE_HANDLE: pstr = "CreatHdl"; break;
        case EVENT_REFERENCE: pstr = "Ref"; break;
        case EVENT_DEREFERENCE: pstr = "Deref"; break;
        case EVENT_LOCK: pstr = "Lock"; break;
        case EVENT_UNLOCK: pstr = "Unlock"; break;
        case EVENT_DELETE: pstr = "Delete"; break;
        case EVENT_FREE: pstr = "Free"; break;
        case EVENT_SET_OWNER: pstr = "SetOwner"; break;
        default: pstr = "Unknown"; break;
    }

    DbgPrint("[%lu] %03x:%03x %.8s val=%p <%lx,%lx,%lx,%lx>\n",
             pLogEntry->ulUnique,
             pLogEntry->dwProcessId,
             pLogEntry->dwThreadId,
             pstr,
             (PVOID)pLogEntry->lParam,
             REL_ADDR(pLogEntry->apvBackTrace[2]),
             REL_ADDR(pLogEntry->apvBackTrace[3]),
             REL_ADDR(pLogEntry->apvBackTrace[4]),
             REL_ADDR(pLogEntry->apvBackTrace[5]));
}

VOID
NTAPI
DbgDumpEventList(PSLIST_HEADER pslh)
{
    PSLIST_ENTRY psle;
    PLOGENTRY pLogEntry;

    while ((psle = InterlockedPopEntrySList(pslh)))
    {
        pLogEntry = CONTAINING_RECORD(psle, LOGENTRY, sleLink);
        DbgPrintEvent(pLogEntry);
    }
}

VOID
NTAPI
DbgCleanupEventList(PSLIST_HEADER pslh)
{
    PSLIST_ENTRY psle;
    PLOGENTRY pLogEntry;

    while ((psle = InterlockedPopEntrySList(pslh)))
    {
        pLogEntry = CONTAINING_RECORD(psle, LOGENTRY, sleLink);
        EngFreeMem(pLogEntry);
    }
}

#endif /* DBG_ENABLE_EVENT_LOGGING */

#if 1 || DBG_ENABLE_SERVICE_HOOKS

VOID
NTAPI
DbgDumpLockedGdiHandles(VOID)
{
    ULONG i;

    for (i = RESERVE_ENTRIES_COUNT; i < GDI_HANDLE_COUNT; i++)
    {
        PENTRY pentry = &gpentHmgr[i];

        if (pentry->Objt)
        {
            POBJ pobj = pentry->einfo.pobj;
            if (pobj->cExclusiveLock > 0)
            {
                DPRINT1("Locked object: %lx, type = %lx. allocated from:\n",
                        i, pentry->Objt);
                DBG_DUMP_EVENT_LIST(&pobj->slhLog);
            }
        }
    }
}

void
NTAPI
GdiDbgPreServiceHook(ULONG ulSyscallId, PULONG_PTR pulArguments)
{
    PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (pti && pti->cExclusiveLocks != 0)
    {
        DbgPrint("FATAL: Win32DbgPreServiceHook(0x%lx): There are %lu exclusive locks!\n",
                 ulSyscallId, pti->cExclusiveLocks);
        DbgDumpLockedGdiHandles();
        ASSERT(FALSE);
    }

}

ULONG_PTR
NTAPI
GdiDbgPostServiceHook(ULONG ulSyscallId, ULONG_PTR ulResult)
{
    PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
    if (pti && pti->cExclusiveLocks != 0)
    {
        DbgPrint("FATAL: Win32DbgPostServiceHook(0x%lx): There are %lu exclusive locks!\n",
                 ulSyscallId, pti->cExclusiveLocks);
        DbgDumpLockedGdiHandles();
        ASSERT(FALSE);
    }
    return ulResult;
}

#endif /* DBG_ENABLE_SERVICE_HOOKS */


NTSTATUS NTAPI
QueryEnvironmentVariable(PUNICODE_STRING Name,
                         PUNICODE_STRING Value)
{
   NTSTATUS Status;
   PWSTR wcs;
   UNICODE_STRING var;
   PWSTR val;
   PPEB Peb;
   PWSTR Environment;

   /* Ugly HACK for ReactOS system threads */
   if(!NtCurrentTeb())
   {
       return(STATUS_VARIABLE_NOT_FOUND);
   }

   Peb = NtCurrentPeb();

   if (Peb == NULL)
   {
       return(STATUS_VARIABLE_NOT_FOUND);
   }

   Environment = Peb->ProcessParameters->Environment;

   if (Environment == NULL)
   {
      return(STATUS_VARIABLE_NOT_FOUND);
   }

   Value->Length = 0;

   wcs = Environment;
   while (*wcs)
   {
      var.Buffer = wcs++;
      wcs = wcschr(wcs, L'=');
      if (wcs == NULL)
      {
         wcs = var.Buffer + wcslen(var.Buffer);
      }
      if (*wcs)
      {
         var.Length = var.MaximumLength = (wcs - var.Buffer) * sizeof(WCHAR);
         val = ++wcs;
         wcs += wcslen(wcs);

         if (RtlEqualUnicodeString(&var, Name, TRUE))
         {
            Value->Length = (wcs - val) * sizeof(WCHAR);
            if (Value->Length <= Value->MaximumLength)
            {
               memcpy(Value->Buffer, val,
                      min(Value->Length + sizeof(WCHAR), Value->MaximumLength));
               Status = STATUS_SUCCESS;
            }
            else
            {
               Status = STATUS_BUFFER_TOO_SMALL;
            }

            return(Status);
         }
      }
      wcs++;
   }

   return(STATUS_VARIABLE_NOT_FOUND);
}

static int __cdecl
DbgCompareChannels(const void * a, const void * b)
{
    return wcscmp((WCHAR*)a, ((DBG_CHANNEL*)b)->Name);
}

static BOOL
DbgAddDebugChannel(PPROCESSINFO ppi, WCHAR* channel, WCHAR* level, WCHAR op)
{
    DBG_CHANNEL *ChannelEntry;
    UINT iLevel, iChannel;

    /* Special treatment for the "all" channel */
    if (wcscmp(channel, L"all") == 0)
    {
        for (iChannel = 0; iChannel < DbgChCount; iChannel++)
        {
            DbgAddDebugChannel(ppi, DbgChannels[iChannel].Name, level, op);
        }
        return TRUE;
    }

    ChannelEntry = (DBG_CHANNEL*)bsearch(channel,
                                         DbgChannels,
                                         DbgChCount,
                                         sizeof(DBG_CHANNEL),
                                         DbgCompareChannels);
    if(ChannelEntry == NULL)
    {
        return FALSE;
    }

    iChannel = ChannelEntry->Id;
    ASSERT(iChannel < DbgChCount);

    if(level == NULL || *level == L'\0' ||wcslen(level) == 0 )
        iLevel = MAX_LEVEL;
    else if(wcsncmp(level, L"err", 3) == 0)
        iLevel = ERR_LEVEL;
    else if(wcsncmp(level, L"fixme", 5) == 0)
        iLevel = FIXME_LEVEL;
    else if(wcsncmp(level, L"warn", 4) == 0)
        iLevel = WARN_LEVEL;
    else if (wcsncmp(level, L"trace", 4) == 0)
        iLevel = TRACE_LEVEL;
    else
        return FALSE;

    if(op==L'+')
    {
        DBG_ENABLE_CHANNEL(ppi, iChannel, iLevel);
    }
    else
    {
        DBG_DISABLE_CHANNEL(ppi, iChannel, iLevel);
    }

    return TRUE;
}

static BOOL
DbgParseDebugChannels(PPROCESSINFO ppi, PUNICODE_STRING Value)
{
    WCHAR *str, *separator, *c, op;

    str = Value->Buffer;

    do
    {
        separator = wcschr(str, L',');
        if(separator != NULL)
            *separator = L'\0';

        c = wcschr(str, L'+');
        if(c == NULL)
            c = wcschr(str, L'-');

        if(c != NULL)
        {
            op = *c;
            *c = L'\0';
            c++;

            DbgAddDebugChannel(ppi, c, str, op);
        }

        str = separator + 1;
    }while(separator != NULL);

    return TRUE;
}

BOOL DbgInitDebugChannels(VOID)
{
    WCHAR valBuffer[100];
    UNICODE_STRING Value;
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"DEBUGCHANNEL");
    NTSTATUS Status;
    PPROCESSINFO ppi;
    BOOL ret;

    /* Initialize all channels to ERROR */
    ppi = PsGetCurrentProcessWin32Process();
    RtlFillMemory( ppi->DbgChannelLevel,
                   sizeof(ppi->DbgChannelLevel),
                   ERR_LEVEL);

    /* Find DEBUGCHANNEL env var */
    Value.Buffer = valBuffer;
    Value.Length = 0;
    Value.MaximumLength = sizeof(valBuffer);
    Status = QueryEnvironmentVariable(&Name, &Value);

    /* It does not exist */
    if(Status == STATUS_VARIABLE_NOT_FOUND)
    {
        /* There is nothing more to do */
        return TRUE;
    }

    /* If the buffer in the stack is not enough allocate it */
    if(Status == STATUS_BUFFER_TOO_SMALL)
    {
        Value.Buffer = ExAllocatePool(PagedPool, Value.MaximumLength);
        if(Value.Buffer == NULL)
        {
            return FALSE;
        }

        /* Get the env var again */
        Status = QueryEnvironmentVariable(&Name, &Value);
    }

   /* Check for error */
    if(!NT_SUCCESS(Status))
    {
        if(Value.Buffer != valBuffer)
        {
            ExFreePool(Value.Buffer);
        }

        return FALSE;
    }

    /* Parse the variable */
    ret = DbgParseDebugChannels(ppi, &Value);

    /* Clean up */
    if(Value.Buffer != valBuffer)
    {
        ExFreePool(Value.Buffer);
    }

    return ret;
}


#endif // DBG

/* EOF */
