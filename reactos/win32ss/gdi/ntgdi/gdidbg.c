/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/gdidbg.c
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

ULONG gulLogUnique = 0;

/* Note: the following values need to be sorted */
DBG_CHANNEL DbgChannels[DbgChCount]={
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
    {L"UserShutdown", DbgChUserShutdown},
    {L"UserSysparams", DbgChUserSysparams},
    {L"UserTimer", DbgChUserTimer},
    {L"UserThread", DbgChUserThread},
    {L"UserWinpos", DbgChUserWinpos},
    {L"UserWinsta", DbgChUserWinsta},
    {L"UserWnd", DbgChUserWnd}
};

#ifdef GDI_DEBUG
#if 0
static
BOOL
CompareBacktraces(ULONG idx1, ULONG idx2)
{
    ULONG iLevel;

    /* Loop all stack levels */
    for (iLevel = 0; iLevel < GDI_STACK_LEVELS; iLevel++)
    {
        if (GDIHandleAllocator[idx1][iLevel]
                != GDIHandleAllocator[idx2][iLevel])
//        if (GDIHandleShareLocker[idx1][iLevel]
//                != GDIHandleShareLocker[idx2][iLevel])
        {
            return FALSE;
        }
    }

    return TRUE;
}

VOID
NTAPI
DbgDumpGdiHandleTableWithBT(void)
{
    static int leak_reported = 0;
    int i, j, idx, nTraces = 0;
    KIRQL OldIrql;

    if (leak_reported)
    {
        DPRINT1("GDI handle abusers already reported!\n");
        return;
    }

    leak_reported = 1;
    DPRINT1("Reporting GDI handle abusers:\n");

    /* We've got serious business to do */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Step through GDI handle table and find out who our culprit is... */
    for (idx = RESERVE_ENTRIES_COUNT; idx < GDI_HANDLE_COUNT; idx++)
    {
        /* If the handle is free, continue */
        if (!IS_HANDLE_VALID(idx)) continue;

        /* Step through all previous backtraces */
        for (j = 0; j < nTraces; j++)
        {
            /* Check if the backtrace matches */
            if (CompareBacktraces(idx, AllocatorTable[j].idx))
            {
                /* It matches, increment count and break out */
                AllocatorTable[j].count++;
                break;
            }
        }

        /* Did we find a new backtrace? */
        if (j == nTraces)
        {
            /* Break out, if we reached the maximum */
            if (nTraces == MAX_BACKTRACES) break;

            /* Initialize this entry */
            AllocatorTable[j].idx = idx;
            AllocatorTable[j].count = 1;
            nTraces++;
        }
    }

    /* bubble sort time! weeeeee!! */
    for (i = 0; i < nTraces-1; i++)
    {
        if (AllocatorTable[i].count < AllocatorTable[i+1].count)
        {
            struct DbgOpenGDIHandle temp;

            temp = AllocatorTable[i+1];
            AllocatorTable[i+1] = AllocatorTable[i];
            j = i;
            while (j > 0 && AllocatorTable[j-1].count < temp.count)
                j--;
            AllocatorTable[j] = temp;
        }
    }

    /* Print the worst offenders... */
    DbgPrint("Worst GDI Handle leak offenders (out of %i unique locations):\n", nTraces);
    for (i = 0; i < nTraces && AllocatorTable[i].count > 1; i++)
    {
        /* Print out the allocation count */
        DbgPrint(" %i allocs, type = 0x%lx:\n",
                 AllocatorTable[i].count,
                 GdiHandleTable->Entries[AllocatorTable[i].idx].Type);

        /* Dump the frames */
        KeRosDumpStackFrames(GDIHandleAllocator[AllocatorTable[i].idx], GDI_STACK_LEVELS);
        //KeRosDumpStackFrames(GDIHandleShareLocker[AllocatorTable[i].idx], GDI_STACK_LEVELS);

        /* Print new line for better readability */
        DbgPrint("\n");
    }

    if (i < nTraces)
        DbgPrint("(List terminated - the remaining entries have 1 allocation only)\n");

    KeLowerIrql(OldIrql);

    ASSERT(FALSE);
}
#endif

ULONG
NTAPI
DbgCaptureStackBackTace(PVOID* pFrames, ULONG nFramesToCapture)
{
    ULONG nFrameCount;

    memset(pFrames, 0x00, (nFramesToCapture + 1) * sizeof(PVOID));

    nFrameCount = RtlWalkFrameChain(pFrames, nFramesToCapture, 0);

    if (nFrameCount < nFramesToCapture)
    {
        nFrameCount += RtlWalkFrameChain(pFrames + nFrameCount,
                                         nFramesToCapture - nFrameCount,
                                         1);
    }

    return nFrameCount;
}

BOOL
NTAPI
DbgGdiHTIntegrityCheck()
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
		Handle = (HGDIOBJ)((Type << GDI_ENTRY_UPPER_SHIFT) + i);

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

#endif /* GDI_DEBUG */

VOID
NTAPI
DbgDumpLockedGdiHandles()
{
#if 0
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
#endif
}

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
    DbgCaptureStackBackTace(pLogEntry->apvBackTrace, 20);

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

static int
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

BOOL DbgInitDebugChannels()
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
