/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/gdikdbgext.c
 * PURPOSE:         KDBG extension for GDI
 * PROGRAMMERS:     Timo Kreuzer
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
//#define NDEBUG
//#include <debug.h>

extern PENTRY gpentHmgr;
extern PULONG gpaulRefCount;
extern ULONG gulFirstUnused;


static const char * gpszObjectTypes[] =
{
    "FREE", "DC", "UNUSED1", "UNUSED2", "RGN", "SURF", "CLIENTOBJ", "PATH",
    "PAL", "ICMLCS", "LFONT", "RFONT", "PFE", "PFT", "ICMCXF", "SPRITE",
    "BRUSH", "UMPD", "UNUSED4", "SPACE", "UNUSED5", "META", "EFSTATE",
    "BMFD", "VTFD", "TTFD", "RC", "TEMP", "DRVOBJ", "DCIOBJ", "SPOOL",
    "RESERVED", "ALL"
};


BOOLEAN
KdbIsMemoryValid(PVOID pvBase, ULONG cjSize)
{
    PUCHAR pjAddress;

    pjAddress = ALIGN_DOWN_POINTER_BY(pvBase, PAGE_SIZE);

    while (pjAddress < (PUCHAR)pvBase + cjSize)
    {
        if (!MmIsAddressValid(pjAddress)) return FALSE;
        pjAddress += PAGE_SIZE;
    }

    return TRUE;
}

static
BOOL
KdbGetHexNumber(char *pszNum, ULONG_PTR *pulValue)
{
    char *endptr;

    /* Skip optional '0x' prefix */
    if ((pszNum[0] == '0') && ((pszNum[1] == 'x') || (pszNum[1] == 'X')))
        pszNum += 2;

    /* Make a number from the string (hex) */
    *pulValue = strtoul(pszNum, &endptr, 16);

    return (*endptr == '\0');
}

static
VOID
KdbCommand_Gdi_help(VOID)
{
    DbgPrint("GDI KDBG extension.\nAvailable commands:\n"
             "- help - Displays this screen.\n"
             "- dumpht [<type>] - Dumps all handles of <type> or lists all types\n"
             "- handle <handle> - Displays information about a handle\n"
             "- entry <entry> - Displays an ENTRY, <entry> can be a pointer or index\n"
             "- baseobject <object> - Displays a BASEOBJECT\n"
#if DBG_ENABLE_EVENT_LOGGING
             "- eventlist <object> - Displays the eventlist for an object\n"
#endif
            );
}

static
VOID
KdbCommand_Gdi_dumpht(ULONG argc, char *argv[])
{
    ULONG i;
    UCHAR Objt, jReqestedType;
    PENTRY pentry;
    POBJ pobj;
    KAPC_STATE ApcState;
    ULONG_PTR ulArg;

    /* No CSRSS, no handle table */
    if (!gpepCSRSS) return;
    KeStackAttachProcess(&gpepCSRSS->Pcb, &ApcState);

    if (argc == 0)
    {
        USHORT Counts[GDIObjType_MAX_TYPE + 2] = {0};

        /* Loop all possibly used entries in the handle table */
        for (i = RESERVE_ENTRIES_COUNT; i < gulFirstUnused; i++)
        {
            if (KdbIsMemoryValid(&gpentHmgr[i], sizeof(ENTRY)))
            {
                Objt = gpentHmgr[i].Objt & 0x1F;
                Counts[Objt]++;
            }
        }

        DbgPrint("Type         Count\n");
        DbgPrint("-------------------\n");
        for (i = 0; i <= GDIObjType_MAX_TYPE; i++)
        {
            DbgPrint("%02x %-9s %d\n",
                     i, gpszObjectTypes[i], Counts[i]);
        }
        DbgPrint("\n");
    }
    else
    {
        /* Loop all object types */
        for (i = 0; i <= GDIObjType_MAX_TYPE + 1; i++)
        {
            /* Check if this object type was requested */
            if (_stricmp(argv[0], gpszObjectTypes[i]) == 0) break;
        }

        /* Check if we didn't find it yet */
        if (i > GDIObjType_MAX_TYPE + 1)
        {
            /* Try if it's a number */
            if (!KdbGetHexNumber(argv[0], &ulArg))
            {
                DbgPrint("Invalid parameter: %s\n", argv[0]);
                return;
            }

            /* Check if it's inside the allowed range */
            if (i > GDIObjType_MAX_TYPE)
            {
                DbgPrint("Unknown object type: %s\n", argv[0]);
                goto leave;
            }
        }

        jReqestedType = i;

        /* Print header */
        DbgPrint("Index Handle   Type      pObject    ThreadId cLocks  ulRefCount\n");
        DbgPrint("---------------------------------------------------------------\n");

        /* Loop all possibly used entries in the handle table */
        for (i = RESERVE_ENTRIES_COUNT; i < gulFirstUnused; i++)
        {
            /* Get the entry and the object */
            pentry = &gpentHmgr[i];

            if (!MmIsAddressValid(pentry)) continue;

            pobj = pentry->einfo.pobj;
            Objt = pentry->Objt & 0x1F;

            /* Check if ALL objects are requested, or the object type matches */
            if ((jReqestedType == GDIObjType_MAX_TYPE + 1) ||
                (Objt == jReqestedType))
            {
                DbgPrint("%04lx  %p %-9s 0x%p 0x%06lx %-6ld ",
                         i, pobj->hHmgr, gpszObjectTypes[Objt], pobj,
                         pobj->dwThreadId, pobj->cExclusiveLock);
                if (MmIsAddressValid(&gpaulRefCount[i]))
                    DbgPrint("0x%08lx\n", gpaulRefCount[i]);
                else
                    DbgPrint("??????????\n");
            }
        }
    }

leave:
    KeUnstackDetachProcess(&ApcState);
}

static
VOID
KdbCommand_Gdi_handle(char *argv)
{
    ULONG_PTR ulObject;
    BASEOBJECT *pobj;
    ENTRY *pentry;
    USHORT usIndex;
    KAPC_STATE ApcState;

    /* Convert the parameter into a number */
    if (!KdbGetHexNumber(argv, &ulObject))
    {
        DbgPrint("Invalid parameter: %s\n", argv);
        return;
    }

    /* No CSRSS, no handle table */
    if (!gpepCSRSS) return;
    KeStackAttachProcess(&gpepCSRSS->Pcb, &ApcState);

    usIndex = ulObject & 0xFFFF;
    pentry = &gpentHmgr[usIndex];

    if (MmIsAddressValid(pentry))
    {
        pobj = pentry->einfo.pobj;

        DbgPrint("GDI handle=%p, type=%s, index=0x%lx, pentry=%p.\n",
                 ulObject, gpszObjectTypes[(ulObject >> 16) & 0x1f],
                 usIndex, pentry);
        DbgPrint(" ENTRY = {.pobj = %p, ObjectOwner = 0x%lx, FullUnique = 0x%04x,\n"
                 "  Objt=0x%02x, Flags = 0x%02x, pUser = 0x%p}\n",
                 pentry->einfo.pobj, pentry->ObjectOwner.ulObj, pentry->FullUnique,
                 pentry->Objt, pentry->Flags, pentry->pUser);
        DbgPrint(" BASEOBJECT = {hHmgr = %p, dwThreadId = 0x%lx,\n"
                 "  cExclusiveLock = %ld, BaseFlags = 0x%lx}\n",
                 pobj->hHmgr, pobj->dwThreadId,
                 pobj->cExclusiveLock, pobj->BaseFlags);
        if (MmIsAddressValid(&gpaulRefCount[usIndex]))
            DbgPrint(" gpaulRefCount[idx] = %ld\n", gpaulRefCount[usIndex]);
    }
    else
    {
        DbgPrint("Coudn't access ENTRY. Probably paged out.\n");
    }

    KeUnstackDetachProcess(&ApcState);
}

static
VOID
KdbCommand_Gdi_entry(char *argv)
{
    ULONG_PTR ulValue;
    PENTRY pentry;
    KAPC_STATE ApcState;

    /* Convert the parameter into a number */
    if (!KdbGetHexNumber(argv, &ulValue))
    {
        DbgPrint("Invalid parameter: %s\n", argv);
        return;
    }

    /* No CSRSS, no handle table */
    if (!gpepCSRSS) return;
    KeStackAttachProcess(&gpepCSRSS->Pcb, &ApcState);

    /* If the parameter is smaller than 0x10000, it's an index */
    pentry = (ulValue <= 0xFFFF) ? &gpentHmgr[ulValue] : (PENTRY)ulValue;

    /* Check if the address is readable */
    if (!MmIsAddressValid(pentry))
    {
        DbgPrint("Cannot access entry at %p\n", pentry);
        goto cleanup;
    }

    /* print the entry */
    DbgPrint("Dumping ENTRY #%ld, @%p:\n", (pentry - gpentHmgr), pentry);
    if (pentry->Objt != 0)
        DbgPrint(" pobj = 0x%p\n", pentry->einfo.pobj);
    else
        DbgPrint(" hFree = 0x%p\n", pentry->einfo.hFree);
    DbgPrint(" ObjectOwner = 0x%p\n", pentry->ObjectOwner.ulObj);
    DbgPrint(" FullUnique = 0x%x\n", pentry->FullUnique);
    DbgPrint(" Objt = 0x%x (%s)\n", pentry->Objt,
             pentry->Objt <= 0x1E ? gpszObjectTypes[pentry->Objt] : "invalid");
    DbgPrint(" Flags = 0x%x\n", pentry->Flags);
    DbgPrint(" pUser = 0x%p\n", pentry->pUser);

cleanup:
    KeUnstackDetachProcess(&ApcState);
}

static
VOID
KdbCommand_Gdi_baseobject(char *argv)
{
}

#if DBG_ENABLE_EVENT_LOGGING
static
VOID
KdbCommand_Gdi_eventlist(char *argv)
{
    ULONG_PTR ulValue;
    POBJ pobj;
    PSLIST_ENTRY psle, psleFirst;
    PLOGENTRY pLogEntry;

    /* Convert the parameter into a number */
    if (!KdbGetHexNumber(argv, &ulValue))
    {
        DbgPrint("Invalid parameter: %s\n", argv);
        return;
    }

    pobj = (POBJ)ulValue;

    /* Check if the address is readable */
    if (!KdbIsMemoryValid(pobj, sizeof(BASEOBJECT)))
    {
        DbgPrint("Cannot access BASEOBJECT at %p\n", pobj);
        return;
    }

    /* The kernel doesn't export RtlFirstEntrySList :( */
    psleFirst = InterlockedFlushSList(&pobj->slhLog);

    /* Loop all events, but don't remove them */
    for (psle = psleFirst; psle != NULL; psle = psle->Next)
    {
        pLogEntry = CONTAINING_RECORD(psle, LOGENTRY, sleLink);
        DbgPrintEvent(pLogEntry);
    }

    /* Put the log back in place */
    InterlockedPushEntrySList(&pobj->slhLog, psleFirst);
}
#endif

BOOLEAN
NTAPI
DbgGdiKdbgCliCallback(
    IN PCHAR pszCommand,
    IN ULONG argc,
    IN PCH argv[])
{

    if (_stricmp(argv[0], "!gdi.help") == 0)
    {
        KdbCommand_Gdi_help();
    }
    else if (_stricmp(argv[0], "!gdi.dumpht") == 0)
    {
        KdbCommand_Gdi_dumpht(argc - 1, argv + 1);
    }
    else if (_stricmp(argv[0], "!gdi.handle") == 0)
    {
        KdbCommand_Gdi_handle(argv[1]);
    }
    else if (_stricmp(argv[0], "!gdi.entry") == 0)
    {
        KdbCommand_Gdi_entry(argv[1]);
    }
    else if (_stricmp(argv[0], "!gdi.baseobject") == 0)
    {
        KdbCommand_Gdi_baseobject(argv[1]);
    }
#if DBG_ENABLE_EVENT_LOGGING
    else if (_stricmp(argv[0], "!gdi.eventlist") == 0)
    {
        KdbCommand_Gdi_eventlist(argv[1]);
    }
#endif
    else
    {
        /* Not handled */
        return FALSE;
    }

    return TRUE;
}





