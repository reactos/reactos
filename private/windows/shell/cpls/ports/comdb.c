/*++


  Author:

  Doron J. Holan (doronh), 1-22-1998
  --*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <msports.h>
#include <tchar.h>

#define GROWTH_VALUE         1024

#define BITS_INA_BYTE        8

typedef struct _DB_INFO {

    HANDLE  RegChangedEvent;
    HANDLE  AccessMutex;

    HKEY    DBKey;

    PBYTE   Ports;
    ULONG   PortsLength;
} DB_INFO, * PDB_INFO;

#define HandleToDBInfo(h) ((PDB_INFO) (h))
#define IsEventSignalled(hevent) (WaitForSingleObject(hevent, 0) == WAIT_OBJECT_0)
#define SanityCheckComNumber(num) { if (num > COMDB_MAX_PORTS_ARBITRATED) return ERROR_INVALID_PARAMETER; }
#define SanityCheckDBInfo(dbi) { if ((HANDLE) dbi == INVALID_HANDLE_VALUE) return ERROR_INVALID_PARAMETER; }


const TCHAR szMutexName[] = _T("ComPortNumberDatabaseMutexObject");
const TCHAR szComDBName[] = _T("ComDB");
const TCHAR szComDBMerge[] = _T("ComDB Merge");
const TCHAR szComDBPath[] = _T("System\\CurrentControlSet\\Control\\COM Name Arbiter");
const TCHAR szComDBPathOld[] = _T("System\\CurrentControlSet\\Services\\Serial");

#ifdef malloc
#undef malloc
#endif
#define malloc(size) LocalAlloc(LPTR, (size))

#ifdef free
#undef free
#endif 
#define free LocalFree

VOID
DestroyDBInfo(
     PDB_INFO DBInfo
     )
{
    if (DBInfo->AccessMutex && 
        DBInfo->AccessMutex != INVALID_HANDLE_VALUE) {
        CloseHandle(DBInfo->AccessMutex);
    }

    if (DBInfo->RegChangedEvent && 
        DBInfo->RegChangedEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(DBInfo->RegChangedEvent);
    }

    if (DBInfo->DBKey && 
        DBInfo->DBKey != (HKEY) INVALID_HANDLE_VALUE) {
        RegCloseKey(DBInfo->DBKey);     
    }

    if (DBInfo->Ports) {
        free(DBInfo->Ports);
    }

    free(DBInfo);
}

LONG
CreationFailure (
     PHCOMDB  PHComDB,
     PDB_INFO DBInfo
     )
{
    if (DBInfo->AccessMutex != 0) 
        ReleaseMutex(DBInfo->AccessMutex);
    DestroyDBInfo(DBInfo);
    *PHComDB = (HCOMDB) INVALID_HANDLE_VALUE;
    return ERROR_ACCESS_DENIED;
}

VOID
RegisterForNotification(
    PDB_INFO DBInfo
    )
{
    ResetEvent(DBInfo->RegChangedEvent);
    if (RegNotifyChangeKeyValue(DBInfo->DBKey,
                                FALSE,
                                REG_NOTIFY_CHANGE_LAST_SET,
                                DBInfo->RegChangedEvent,
                                TRUE) != ERROR_SUCCESS) {
        //
        // Can't get a notification of when the DB is changed so close the handle
        // and we must update the DB at every access no matter what
        //
        CloseHandle(DBInfo->RegChangedEvent);
        DBInfo->RegChangedEvent = INVALID_HANDLE_VALUE;
    }
}

BOOL
ResizeDatabase(
    PDB_INFO DBInfo,
    ULONG    NumberPorts
    )
{
    PBYTE newPorts = NULL;
    ULONG newPortsLength;

    if (DBInfo->Ports) {
        newPortsLength = NumberPorts / BITS_INA_BYTE;
        newPorts = (PBYTE) malloc(newPortsLength * sizeof(BYTE));

        if (newPorts) {
            memcpy(newPorts, DBInfo->Ports, DBInfo->PortsLength);
            free(DBInfo->Ports);
            DBInfo->Ports = newPorts;
            DBInfo->PortsLength = newPortsLength;

            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        //
        // Just alloc and be done with it
        //
        DBInfo->PortsLength = NumberPorts / BITS_INA_BYTE;
        DBInfo->Ports = (PBYTE) malloc(DBInfo->PortsLength * sizeof(BYTE));

        return DBInfo->Ports ? TRUE : FALSE;
    }
}

LONG
WINAPI
ComDBOpen (
    PHCOMDB PHComDB
    )
/*++

Routine Description:

    Opens name data base, and returns a handle to be used in future calls.

Arguments:

    None.

Return Value:

    INVALID_HANDLE_VALUE if the call fails, otherwise a valid handle

    If INVALID_HANDLE_VALUE, call GetLastError() to get details (??)

--*/
{
    PDB_INFO dbInfo = malloc(sizeof(DB_INFO));
    DWORD    type, size, disposition = 0x0;
    BOOLEAN  migrated = FALSE;
    LONG     res;
    BYTE     merge[COMDB_MIN_PORTS_ARBITRATED / BITS_INA_BYTE /* 32 */]; 

    if (dbInfo == 0) {
        *PHComDB = (HCOMDB) INVALID_HANDLE_VALUE;
        return ERROR_ACCESS_DENIED;
    }

    dbInfo->AccessMutex = CreateMutex(NULL, FALSE, szMutexName);

    if (dbInfo->AccessMutex == 0) {
        return CreationFailure(PHComDB, dbInfo);
    }

    //
    // Enter the mutex so we can guarantee only one thread pounding on the reg
    // key at once
    //
    WaitForSingleObject(dbInfo->AccessMutex, INFINITE);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       szComDBPath, 
                       0,
                       (TCHAR *) NULL, 
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS | KEY_NOTIFY,
                       (LPSECURITY_ATTRIBUTES) NULL,
                       &dbInfo->DBKey,
                       &disposition) != ERROR_SUCCESS) {
        //
        // Try again w/out notification caps
        //
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           szComDBPath,
                           0,
                           (TCHAR *) NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           (LPSECURITY_ATTRIBUTES) NULL,
                           &dbInfo->DBKey, 
                           &disposition) != ERROR_SUCCESS) {
            return CreationFailure(PHComDB, dbInfo);
        }

        dbInfo->RegChangedEvent = INVALID_HANDLE_VALUE;
    }
    else {
        dbInfo->RegChangedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (dbInfo->RegChangedEvent == 0) {
            dbInfo->RegChangedEvent = INVALID_HANDLE_VALUE;
        }
    }

    if (disposition == REG_CREATED_NEW_KEY) {
        //
        // Must migrate the previous values from the old com db path
        //
        HKEY hOldDB;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szComDBPathOld, 
                         0,
                         KEY_ALL_ACCESS,
                         &hOldDB) == ERROR_SUCCESS &&
            RegQueryValueEx(hOldDB,
                            szComDBName,
                            0,
                            &type,
                            NULL,
                            &dbInfo->PortsLength) == ERROR_SUCCESS) {

            //
            // The old value is still there, get its contents, copy it to the 
            // new location and delete the old value
            //
            migrated = TRUE;
            ResizeDatabase(dbInfo, dbInfo->PortsLength * BITS_INA_BYTE);
    
            size = dbInfo->PortsLength;

            res = RegQueryValueEx(hOldDB,
                                  szComDBName,
                                  0,
                                  &type,
                                  (PBYTE) dbInfo->Ports,
                                  &size);

            RegDeleteValue(hOldDB, szComDBName);

            //
            // The value does not exist, write it out
            //
            if (RegSetValueEx(dbInfo->DBKey,
                              szComDBName,
                              0,
                              REG_BINARY,
                              dbInfo->Ports,
                              dbInfo->PortsLength) != ERROR_SUCCESS) {

                RegCloseKey(hOldDB);
                return CreationFailure(PHComDB, dbInfo);
            }

            RegCloseKey(hOldDB);
        }

    }

    //
    // If we haven't migrated values from the old path, then either create a 
    // new chunk or read in values previously written
    //
    if (!migrated) {
        res = RegQueryValueEx(dbInfo->DBKey,
                              szComDBName,
                              0,
                              &type,
                              NULL,
                              &dbInfo->PortsLength);
    
        if (res == ERROR_FILE_NOT_FOUND) {
            ResizeDatabase(dbInfo, COMDB_MIN_PORTS_ARBITRATED); 
    
            //
            // The value does not exist, write it out
            //
            res = RegSetValueEx(dbInfo->DBKey,
                                szComDBName,
                                0,
                                REG_BINARY,
                                dbInfo->Ports,
                                dbInfo->PortsLength);
                                
            if (res != ERROR_SUCCESS) {
                return CreationFailure(PHComDB, dbInfo);
            }
        }
        else if (res == ERROR_MORE_DATA || res != ERROR_SUCCESS || type != REG_BINARY) {
            return CreationFailure(PHComDB, dbInfo);
        }
        else if (res == ERROR_SUCCESS) {
            ResizeDatabase(dbInfo, dbInfo->PortsLength * BITS_INA_BYTE);
    
            size = dbInfo->PortsLength;
            res = RegQueryValueEx(dbInfo->DBKey,
                                  szComDBName,
                                  0,
                                  &type,
                                  (PBYTE) dbInfo->Ports,
                                  &size);
        }
    }
    
    size = sizeof(merge);
    if (RegQueryValueEx(dbInfo->DBKey,
                        szComDBMerge,
                        0,
                        &type,
                        (PBYTE) merge,
                        &size) == ERROR_SUCCESS &&
        size <= dbInfo->PortsLength) {

        int i;

        for (i = 0 ; i < COMDB_MIN_PORTS_ARBITRATED / BITS_INA_BYTE; i++) {
            dbInfo->Ports[i] |= merge[i];
        }
        
        RegDeleteValue(dbInfo->DBKey, szComDBMerge);

        RegSetValueEx(dbInfo->DBKey,
                      szComDBName,
                      0,
                      REG_BINARY,
                      dbInfo->Ports,
                      dbInfo->PortsLength);
    }

    if (dbInfo->RegChangedEvent != INVALID_HANDLE_VALUE) {
        RegisterForNotification(dbInfo);
    }

    ReleaseMutex(dbInfo->AccessMutex);

    //
    // All done!  phew...
    //
    *PHComDB = (HCOMDB) dbInfo;
    return ERROR_SUCCESS;

}

LONG
WINAPI
ComDBClose (
    HCOMDB HComDB
    )
/*++

Routine Description:

    frees a handle to the database returned from OpenComPortDataBase

Arguments:

    Handle returned from OpenComPortDataBase.

Return Value:

    None

--*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);

    SanityCheckDBInfo(dbInfo);
    DestroyDBInfo(dbInfo);

    return ERROR_SUCCESS;
}

BOOL
EnterDB(
    PDB_INFO DBInfo
    )
{
    BOOL eventSignalled;
    LONG res;
    DWORD type, size;

    WaitForSingleObject(DBInfo->AccessMutex, INFINITE);
    
    if (DBInfo->RegChangedEvent == INVALID_HANDLE_VALUE ||
        (eventSignalled = IsEventSignalled(DBInfo->RegChangedEvent))) {

        size = 0;
        res = RegQueryValueEx(DBInfo->DBKey,
                              szComDBName,
                              0,
                              &type,
                              0,
                              &size);

        //
        // Couldn't update the DB ... fail
        // 
        if (res != ERROR_SUCCESS || type != REG_BINARY) {
            ReleaseMutex(DBInfo->AccessMutex);
            return FALSE;
        }

        if (size != DBInfo->PortsLength) {
            ResizeDatabase(DBInfo, size * BITS_INA_BYTE);
        }

        RegQueryValueEx(DBInfo->DBKey,
                        szComDBName,
                        0,
                        &type,
                        DBInfo->Ports,
                        &size);

        //
        // Reregister the notification with the registry
        // 
        if (eventSignalled) {
            RegisterForNotification(DBInfo);
        }
    }

    return TRUE;
}

LONG
LeaveDB(
    PDB_INFO DBInfo,
    BOOL     CommitChanges
    )
{
    LONG retVal = ERROR_SUCCESS;

    if (CommitChanges) {
        if (RegSetValueEx(DBInfo->DBKey,
                          szComDBName,
                          0,
                          REG_BINARY,
                          DBInfo->Ports,
                          DBInfo->PortsLength) != ERROR_SUCCESS) {
            retVal = ERROR_CANTWRITE;
        }

        //
        // The setting of the value in the reg signals the event...but we don't 
        // need to resync w/the reg off of this change b/c it is our own!  Instead
        // reset the event and rereg for the event
        //
        if (DBInfo->RegChangedEvent != INVALID_HANDLE_VALUE) {
            RegisterForNotification(DBInfo);
        }
    }

    ReleaseMutex(DBInfo->AccessMutex);
    return retVal;
}

VOID
GetByteAndMask(
    PDB_INFO DBInfo,
    DWORD    ComNumber,
    PBYTE    *Byte,
    PBYTE    Mask
    )
{
    ComNumber--;
    *Byte = DBInfo->Ports + (ComNumber / BITS_INA_BYTE);
    *Mask = 1 << (ComNumber % BITS_INA_BYTE);
}

LONG
WINAPI
ComDBGetCurrentPortUsage (
    HCOMDB   HComDB,
    PBYTE    Buffer,
    DWORD    BufferSize,
    ULONG    ReportType, 
    LPDWORD  MaxPortsReported
    )
/*++
    
    Handle requests that require no synch w/DB first.

  --*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);
    PBYTE    curSrc, curDest, endDest;
    BYTE     mask;

    SanityCheckDBInfo(dbInfo);

    if (!EnterDB(dbInfo))  {
        return ERROR_NOT_CONNECTED;
    }

    if (Buffer == 0) {
        if (!MaxPortsReported) {
            LeaveDB(dbInfo, FALSE);
            return ERROR_INVALID_PARAMETER;
        }
        else {
            *MaxPortsReported = dbInfo->PortsLength * BITS_INA_BYTE;
            return LeaveDB(dbInfo, FALSE);
        }
    }

    if (ReportType == CDB_REPORT_BITS) {
        if (BufferSize > dbInfo->PortsLength) {
            BufferSize = dbInfo->PortsLength;
        }
        memcpy(Buffer, dbInfo->Ports, BufferSize);
        if (MaxPortsReported) {
            *MaxPortsReported = BufferSize * BITS_INA_BYTE;
        }
    }
    else if (ReportType == CDB_REPORT_BYTES) {
        if (BufferSize > dbInfo->PortsLength * BITS_INA_BYTE) {
            BufferSize = dbInfo->PortsLength * BITS_INA_BYTE;
        }

        curSrc = dbInfo->Ports;
        endDest = Buffer + BufferSize;
        curDest = Buffer;

        for (mask = 1; curDest != endDest; curDest++) {
            *curDest = (*curSrc & mask) ? 1 : 0;
            if (mask & 0x80) {
                mask = 0x1;
                curSrc++;
            }
            else
                mask <<= 1;
        }
    }
    else {
        LeaveDB(dbInfo, FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    return LeaveDB(dbInfo, FALSE);
}

LONG
WINAPI
ComDBClaimNextFreePort (
    HCOMDB   HComDB,
    LPDWORD  ComNumber
    )
/*++

Routine Description:

    returns the first free COMx value

Arguments:

    Handle returned from OpenComPortDataBase.

Return Value:


    returns ERROR_SUCCESS if successful. or other ERROR_ if not

    if successful, then ComNumber will be that next free com value and claims it in the database


--*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);
    DWORD    num;
    BOOL     commit = FALSE;
    PBYTE    curSrc, srcEnd;
    BYTE     mask;
    LONG     ret;

    SanityCheckDBInfo(dbInfo);

    if (!EnterDB(dbInfo)) {
        return ERROR_NOT_CONNECTED;
    }

    curSrc = dbInfo->Ports;
    srcEnd = curSrc + dbInfo->PortsLength;

    for (num = 3, mask = 0x4; curSrc != srcEnd; num++) {
        if (!(*curSrc & mask)) {
            *ComNumber = num;
            *curSrc |= mask;
            commit = TRUE;
            break;
        }
        else if (mask & 0x80) {
            mask = 0x1;
            curSrc++;
        }
        else {
            mask <<= 1;
        }
    }

    if (curSrc == srcEnd && !commit && num < COMDB_MAX_PORTS_ARBITRATED) {
        // DB entirely full
        ResizeDatabase(dbInfo, ((num / GROWTH_VALUE) + 1) * GROWTH_VALUE);
        *ComNumber = num;

        GetByteAndMask(dbInfo, num, &curSrc, &mask);
        *curSrc |= mask;    
        commit = TRUE;
    }

    ret = LeaveDB(dbInfo, commit);
    if (!commit) {
        ret = ERROR_NO_LOG_SPACE;
    }

    return ret;
}

LONG
WINAPI
ComDBClaimPort (
    HCOMDB   HComDB,
    DWORD    ComNumber,
    BOOL     ForceClaim,
    PBOOL    Forced
    )
/*++

Routine Description:

    Attempts to claim a com name in the database

Arguments:

    DataBaseHandle - returned from OpenComPortDataBase.

    ComNumber      - The port value to be claimed

    Force          - If TRUE, will force the port to be claimed even if in use already



Return Value:


    returns ERROR_SUCCESS if port name was not already claimed, or if it was claimed
                          and Force was TRUE.

    ERROR_SHARING_VIOLATION  if port name is use and Force is false


--*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);
    PBYTE    curByte;
    BYTE     mask;
    BOOL     commit = TRUE;
    LONG     res;
    ULONG    newSize;
    
    BOOL f;
    if (!(Forced)) {
        Forced = &f;
    }
    SanityCheckComNumber(ComNumber);
    SanityCheckDBInfo(dbInfo);

    if (!EnterDB(dbInfo)) {
        return ERROR_NOT_CONNECTED;
    }

    if (ComNumber > dbInfo->PortsLength * BITS_INA_BYTE) {
        ResizeDatabase(dbInfo, ((ComNumber / GROWTH_VALUE) + 1) * GROWTH_VALUE);
    }

    GetByteAndMask(dbInfo, ComNumber, &curByte, &mask);

    if (*curByte & mask) {
        commit = FALSE;
        if (ForceClaim) {
            if (Forced)
                *Forced = TRUE;
        }
        else {
            res = LeaveDB(dbInfo, commit);
            if (res == ERROR_SUCCESS) {
                return ERROR_SHARING_VIOLATION;
            }
            else {
                return res;
            }
        }   
    }
    else {
        if (Forced)
            *Forced = FALSE;
        *curByte |= mask;
    }

    return LeaveDB(dbInfo, commit);
}

LONG
WINAPI
ComDBReleasePort (
    HCOMDB   HComDB, 
    DWORD    ComNumber
    )
/*++

Routine Description:

    un-claims the port in the database

Arguments:

    DatabaseHandle - returned from OpenComPortDataBase.

    ComNumber      - port to be unclaimed in database

Return Value:


    returns ERROR_SUCCESS if successful. or other ERROR_ if not


--*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);
    PBYTE    byte;
    BYTE     mask;

    SanityCheckDBInfo(dbInfo);

    if (!EnterDB(dbInfo)) {
        return ERROR_NOT_CONNECTED;
    }

    if (ComNumber > dbInfo->PortsLength * BITS_INA_BYTE) {
        LeaveDB(dbInfo, FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    GetByteAndMask(dbInfo, ComNumber, &byte, &mask);
    *byte &= ~mask;

    return LeaveDB(dbInfo, TRUE);
}

LONG
WINAPI
ComDBResizeDatabase (
    HCOMDB   HComDB, 
    DWORD    NewSize
    )
/*++

Routine Description:

    Resizes the database to the new size.  To get the current size, call
    ComDBGetCurrentPortUsage with a Buffer == NULL.

Arguments:

    DatabaseHandle - returned from OpenComPortDataBase.

    NewSize        - must be a multiple of 1024, with a max of 4096
    
Return Value:

    returns ERROR_SUCCESS if successful
            ERROR_BAD_LENGTH if NewSize is not greater than the current size or
                             NewSize is greater than COMDB_MAX_PORTS_ARBITRATED

--*/
{
    PDB_INFO dbInfo = HandleToDBInfo(HComDB);
    BOOL     commit = FALSE;

    SanityCheckDBInfo(dbInfo);

    if (NewSize % GROWTH_VALUE) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!EnterDB(dbInfo)) {
        return ERROR_NOT_CONNECTED;
    }

    if (NewSize > COMDB_MAX_PORTS_ARBITRATED ||
        dbInfo->PortsLength * BITS_INA_BYTE >= NewSize) {
        LeaveDB(dbInfo, FALSE);
        return ERROR_BAD_LENGTH;
    }

    ResizeDatabase(dbInfo, NewSize);

    return LeaveDB(dbInfo, TRUE);
}

