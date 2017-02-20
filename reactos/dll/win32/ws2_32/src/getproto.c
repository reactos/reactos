/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/getproto.c
 * PURPOSE:     GetProtoByY Functions.
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

HANDLE
WSAAPI
GetProtoOpenNetworkDatabase(PCHAR Name)
{
    PCHAR ExpandedPath;
    PCHAR DatabasePath;
    INT ErrorCode;
    HKEY DatabaseKey;
    DWORD RegType;
    DWORD RegSize = 0;
    HANDLE ret;

    ExpandedPath = HeapAlloc(WsSockHeap, 0, MAX_PATH);
    if (!ExpandedPath)
        return INVALID_HANDLE_VALUE;

    /* Open the database path key */
    ErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             "System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                             0,
                             KEY_READ,
                             &DatabaseKey);
    if (ErrorCode == NO_ERROR)
    {
        /* Read the actual path */
        ErrorCode = RegQueryValueEx(DatabaseKey,
                                    "DatabasePath",
                                    NULL,
                                    &RegType,
                                    NULL,
                                    &RegSize);

        DatabasePath = HeapAlloc(WsSockHeap, 0, RegSize);
        if (!DatabasePath)
        {
            HeapFree(WsSockHeap, 0, ExpandedPath);
            return INVALID_HANDLE_VALUE;
        }

        /* Read the actual path */
        ErrorCode = RegQueryValueEx(DatabaseKey,
                                    "DatabasePath",
                                    NULL,
                                    &RegType,
                                    (LPBYTE)DatabasePath,
                                    &RegSize);


        /* Close the key */
        RegCloseKey(DatabaseKey);

        /* Expand the name */
        ExpandEnvironmentStrings(DatabasePath, ExpandedPath, MAX_PATH);

        HeapFree(WsSockHeap, 0, DatabasePath);
    }
    else
    {
        /* Use default path */
        GetSystemDirectory(ExpandedPath, MAX_PATH);
        if (ExpandedPath[strlen(ExpandedPath) - 1] != '\\')
        {
            /* It isn't, so add it ourselves */
            strncat(ExpandedPath, "\\", MAX_PATH);
        }
        strncat(ExpandedPath, "DRIVERS\\ETC\\", MAX_PATH);
    }

    /* Make sure that the path is backslash-terminated */
    if (ExpandedPath[strlen(ExpandedPath) - 1] != '\\')
    {
        /* It isn't, so add it ourselves */
        strncat(ExpandedPath, "\\", MAX_PATH);
    }

    /* Add the database name */
    strncat(ExpandedPath, Name, MAX_PATH);

    /* Return a handle to the file */
    ret = CreateFile(ExpandedPath,
                      FILE_READ_ACCESS,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    HeapFree(WsSockHeap, 0, ExpandedPath);
    return ret;
}

PCHAR
WSAAPI
GetProtoPatternMatch(IN PCHAR Buffer,
                     IN PCHAR Lookup)
{
    CHAR ScanChar;

    /* Loop as long as we have data */
    while ((ScanChar = *Buffer))
    {
        /* Check for a match and return its pointer if found */
        if (strchr(Lookup, ScanChar)) return Buffer;

        /* Keep going */
        Buffer++;
    }

    /* Nothing found */
    return NULL;
}

PPROTOENT
WSAAPI
GetProtoGetNextEnt(IN HANDLE DbHandle,
                   IN PWSPROTO_BUFFER Buffer)
{
    DWORD Read;
    LONG n;
    PCHAR p, p1, Entry, *Aliases;
    PPROTOENT ReturnedProtoent;

    /* Find out where we currently are in the file */
    n = SetFilePointer(DbHandle, 0, 0, FILE_CURRENT);

    while (TRUE)
    {
        /* Read 512 bytes */
        if (!ReadFile(DbHandle,
                      Buffer->LineBuffer,
                      sizeof(Buffer->LineBuffer) - 1,
                      &Read,
                      NULL)) return NULL;

        /* Find out where the line ends */
        p1 = Buffer->LineBuffer;
        Buffer->LineBuffer[Read] = ANSI_NULL;
        p = strchr(Buffer->LineBuffer, '\n');

        /* Bail out if the file is parsed */
        if (!p) return NULL;

        /* Calculate our new position */
        n += (LONG)(p - p1) + 1;

        /* Make it so we read from there next time */
        SetFilePointer(DbHandle, n, 0, FILE_BEGIN);

        /* Null-terminate the buffer so it only contains this line */
        *p = ANSI_NULL;

        /* If this line is a comment, skip it */
        if (*p1 == '#') continue;

        /* Get the entry in this line and null-terminate it */
        Entry = GetProtoPatternMatch(p1, "#\n");
        if (!Entry) continue;
        *Entry = ANSI_NULL;

        /* Start with the name */
        Buffer->Protoent.p_name = p1;

        /* Get the first tab and null-terminate */
        Entry = GetProtoPatternMatch(p1, " \t");
        if (!Entry) continue;
        *Entry++ = ANSI_NULL;

        /* Handle remaining tabs or spaces */
        while (*Entry == ' ' || *Entry == '\t') Entry++;

        /* Now move our read pointer */
        p1 = GetProtoPatternMatch(Entry, " \t");
        if (p1) *p1++ = ANSI_NULL;

        /* This is where the address is */
        Buffer->Protoent.p_proto = (short)atoi(Entry);

        /* Setup the alias buffer */
        Buffer->Protoent.p_aliases = Buffer->Aliases;
        Aliases = Buffer->Protoent.p_aliases;

        /* Check if the pointer is stil valid */
        if (p1)
        {
            /* The first entry is here */
            Entry = p1;

            /* Loop while there are non-null entries */
            while (Entry && *Entry)
            {
                /* Handle tabs and spaces */
                while (*Entry == ' ' || *Entry == '\t') Entry++;

                /* Make sure we don't go over the buffer */
                if (Aliases < &Buffer->Protoent.p_aliases[MAXALIASES - 1])
                {
                    /* Write the alias */
                    *Aliases++ = Entry;
                }

                /* Get to the next entry */
                Entry = GetProtoPatternMatch(Entry, " \t");
                if (Entry) *Entry++ = ANSI_NULL;
            }
        }

        /* Terminate the list */
        *Aliases = NULL;

        /* Return to caller */
        ReturnedProtoent = &Buffer->Protoent;
        break;
    }

    /* Return whatever we got */
    return ReturnedProtoent;
}

/*
 * @implemented
 */
LPPROTOENT
WSAAPI
getprotobynumber(IN INT number)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PPROTOENT Protoent;
    PVOID GetProtoBuffer;
    HANDLE DbHandle;
    DPRINT("getprotobynumber: %lx\n", number);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Get our buffer */
    GetProtoBuffer = WsThreadGetProtoBuffer(Thread);
    if (!GetProtoBuffer)
    {
        /* Fail */
        SetLastError(WSANO_DATA);
        return NULL;
    }

    /* Open the network database */
    DbHandle = GetProtoOpenNetworkDatabase("protocol");
    if (DbHandle == INVALID_HANDLE_VALUE)
    {
        /* Couldn't open the DB; fail */
        SetLastError(WSANO_DATA);
        return NULL;
    }

    /* Start the scan loop */
    while (TRUE)
    {
        /* Get a protoent entry */
        Protoent = GetProtoGetNextEnt(DbHandle, GetProtoBuffer);

        /* Break if we didn't get any new one */
        if (!Protoent) break;

        /* Break if we have a match */
        if (Protoent->p_proto == number) break;
    }

    /* Close the network database */
    CloseHandle(DbHandle);

    /* Set error if we don't have a protoent */
    if (!Protoent) SetLastError(WSANO_DATA);

    /* Return it */
    return Protoent;
}

/*
 * @implemented
 */
LPPROTOENT
WSAAPI
getprotobyname(IN CONST CHAR FAR *name)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PPROTOENT Protoent;
    PVOID GetProtoBuffer;
    HANDLE DbHandle;
    DPRINT("getprotobyname: %s\n", name);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return NULL;
    }

    /* Get our buffer */
    GetProtoBuffer = WsThreadGetProtoBuffer(Thread);
    if (!GetProtoBuffer)
    {
        /* Fail */
        SetLastError(WSANO_DATA);
        return NULL;
    }

    /* Open the network database */
    DbHandle = GetProtoOpenNetworkDatabase("protocol");
    if (DbHandle == INVALID_HANDLE_VALUE)
    {
        /* Couldn't open the DB; fail */
        SetLastError(WSANO_DATA);
        return NULL;
    }

    /* Start the scan loop */
    while (TRUE)
    {
        /* Get a protoent entry */
        Protoent = GetProtoGetNextEnt(DbHandle, GetProtoBuffer);

        /* Break if we didn't get any new one */
        if (!Protoent) break;

        /* Break if we have a match */
        if (!_stricmp(Protoent->p_name, name)) break;
    }

    /* Close the network database */
    CloseHandle(DbHandle);

    /* Set error if we don't have a protoent */
    if (!Protoent) SetLastError(WSANO_DATA);

    /* Return it */
    return Protoent;
}
