#include <cache.hxx>


// Typedef for GetFileAttributeEx function
typedef BOOL (WINAPI *PFNGETFILEATTREX)(LPCTSTR, GET_FILEEX_INFO_LEVELS, LPVOID);
#ifdef unix
#include <flock.hxx>
#endif /* unix */

//
// global variables definition.
//

CRITICAL_SECTION GlobalCacheCritSect;
BOOL GlobalCacheInitialized = FALSE;
CConMgr *GlobalUrlContainers = NULL;
LONG GlobalScavengerRunning = -1;
DWORD GlobalRetrieveUrlCacheEntryFileCount = 0;

PFNGETFILEATTREX gpfnGetFileAttributesEx = 0;

char       g_szFixup[sizeof(DWORD)];
HINSTANCE  g_hFixup;
PFN_FIXUP  g_pfnFixup;

MEMORY *CacheHeap = NULL;
HNDLMGR HandleMgr;

#ifdef unix
/***********************
 * ReadOnlyCache on Unix
 * *********************
 * When the cache resides on a file system which is shared over NFS
 * and the user can access the same cache from different work-stations,
 * it causes a problem. The fix is made so that, the first process has
 * write access to the cache and any subsequent browser process which
 * is started from a different host will receive a read-only version
 * of the cache and will not be able to get cookies etc. A symbolic
 * link is created in $HOME/.microsoft named ielock. Creation and
 * deletion of this symbolic link should be atomic. The functions
 * CreateAtomicCacheLockFile and DeleteAtomicCacheLockFile implement
 * this behavior. When a readonly cache is used, cache deletion is
 * not allowed (Scavenger thread need not be launched).
 *
 * g_ReadOnlyCaches denotes if a readonly cache is being used.
 * gszLockingHost denotes the host that holds the cache lock.
 */

BOOL g_ReadOnlyCaches = FALSE;
char *gszLockingHost = 0;

extern "C" void unixGetWininetCacheLockStatus(BOOL *pBool, char **pszLockingHost)
{
    if(pBool)
        *pBool = g_ReadOnlyCaches;
    if(pszLockingHost)
        *pszLockingHost = gszLockingHost;
}
#endif /* unix */

#ifdef CHECKLOCK_PARANOID

//  Code to enforce strict ordering on resources to prevent deadlock
//  One cannot attempt to take the critical section for the first time
//  if one holds a container lock
DWORD dwThreadLocked;
DWORD dwLockLevel;

void CheckEnterCritical(CRITICAL_SECTION *_cs)
{
    EnterCriticalSection(_cs);
    if (_cs == &GlobalCacheCritSect && dwLockLevel++ == 0)
    {
        dwThreadLocked = GetCurrentThreadId();
        if (GlobalUrlContainers) GlobalUrlContainers->CheckNoLocks(dwThreadLocked);
    }
}

void CheckLeaveCritical(CRITICAL_SECTION *_cs)
{
    if (_cs == &GlobalCacheCritSect)
    {
        INET_ASSERT(dwLockLevel);
        if (dwLockLevel == 1)
        {
            if (GlobalUrlContainers) GlobalUrlContainers->CheckNoLocks(dwThreadLocked);
            dwThreadLocked = 0;
        }
        dwLockLevel--;
    }
    LeaveCriticalSection(_cs);
}
#endif

//

/*++

--*/

BOOL InitGlobals (void)
{
    if (GlobalCacheInitialized)
        return TRUE;

    LOCK_CACHE();

    if (GlobalCacheInitialized)
        goto done;

    GetWininetUserName();

    // Read registry settings.
    OpenInternetSettingsKey();
    InternetReadRegistryDword(vszSyncMode, &GlobalUrlCacheSyncMode);

    { // Detect a fixup handler.  Open scope to avoid compiler complaint.
    
        REGISTRY_OBJ roCache (HKEY_LOCAL_MACHINE, OLD_CACHE_KEY);

        if (ERROR_SUCCESS == roCache.GetStatus())
        {
            DWORD cbFixup = sizeof(g_szFixup);
            if (ERROR_SUCCESS != roCache.GetValue
                ("FixupKey", (LPBYTE) g_szFixup, &cbFixup))
            {
                g_szFixup[0] = 0;
            }

            if (g_szFixup[0] != 'V' || g_szFixup[3] != 0)
            {
                g_szFixup[0] = 0;
            }                  
        }
    }
    {
        REGISTRY_OBJ roCache (HKEY_LOCAL_MACHINE, CACHE5_KEY);
        if (ERROR_SUCCESS == roCache.GetStatus())
        {
            DWORD dwDefTime;
            if (ERROR_SUCCESS == roCache.GetValue("SessionStartTimeDefaultDeltaSecs", &dwDefTime))
            {
                dwdwSessionStartTimeDefaultDelta = dwDefTime * (LONGLONG)10000000;
                dwdwSessionStartTime -= dwdwSessionStartTimeDefaultDelta;
            }
        }
    }
    
    // Seed the random number generator for random file name generation.
    srand(GetTickCount());

    GlobalUrlContainers = new CConMgr();
    GlobalCacheInitialized =
        GlobalUrlContainers && (GlobalUrlContainers->GetStatus() == ERROR_SUCCESS);

    if( GlobalCacheInitialized )
    {
        DWORD dwError = GlobalUrlContainers->CreateDefaultGroups();
        INET_ASSERT(dwError == ERROR_SUCCESS);
    }
    else
    {
        delete GlobalUrlContainers;
        GlobalUrlContainers = NULL;
    }

done:
    UNLOCK_CACHE();
    return GlobalCacheInitialized;
}


BOOL
DLLUrlCacheEntry(
    IN DWORD Reason
    )
/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/
{
    HMODULE ModuleHandleKernel;

    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
#ifdef CHECKLOCK_PARANOID
            dwThreadLocked = 0;
            dwLockLevel = 0;
#endif
            ModuleHandleKernel = GetModuleHandle("KERNEL32");
            if (ModuleHandleKernel)
            {
                gpfnGetFileAttributesEx = (PFNGETFILEATTREX)
                    GetProcAddress(ModuleHandleKernel, "GetFileAttributesExA");
            }

            InitializeCriticalSection (&GlobalCacheCritSect);
            // RunOnceUrlCache (NULL, NULL, NULL, 0); // test stub
#ifdef unix
            if(CreateAtomicCacheLockFile(&g_ReadOnlyCaches,&gszLockingHost) == FALSE)
                return FALSE;
#endif /* unix */
            break;

        case DLL_PROCESS_DETACH:

            // Clean up containers list.
            if (GlobalUrlContainers != NULL)
                delete GlobalUrlContainers;

            // Unload fixup handler.
            if (g_hFixup)
                FreeLibrary (g_hFixup);
                
            HandleMgr.Destroy();
            
#ifdef unix
        DeleteAtomicCacheLockFile();
#endif /* unix */
        DeleteCriticalSection (&GlobalCacheCritSect);
        break;
    }
    return TRUE;
}
