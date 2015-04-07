/*
 * Implementation of the Local Printprovider
 *
 * Copyright 2006-2009 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "localspl_private.h"

#include <shlwapi.h>
#include <ddk/winddiui.h>

/* ############################### */

static CRITICAL_SECTION monitor_handles_cs;
static CRITICAL_SECTION_DEBUG monitor_handles_cs_debug =
{
    0, 0, &monitor_handles_cs,
    { &monitor_handles_cs_debug.ProcessLocksList, &monitor_handles_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": monitor_handles_cs") }
};
static CRITICAL_SECTION monitor_handles_cs = { &monitor_handles_cs_debug, -1, 0, 0, 0, 0 };

/* ############################### */

typedef struct {
    WCHAR   src[MAX_PATH+MAX_PATH];
    WCHAR   dst[MAX_PATH+MAX_PATH];
    DWORD   srclen;
    DWORD   dstlen;
    DWORD   copyflags;
    BOOL    lazy;
} apd_data_t;

typedef struct {
    struct list     entry;
    LPWSTR          name;
    LPWSTR          dllname;
    PMONITORUI      monitorUI;
    LPMONITOR       monitor;
    HMODULE         hdll;
    DWORD           refcount;
    DWORD           dwMonitorSize;
} monitor_t;

typedef struct {
    LPCWSTR  envname;
    LPCWSTR  subdir;
    DWORD    driverversion;
    LPCWSTR  versionregpath;
    LPCWSTR  versionsubdir;
} printenv_t;

typedef struct {
    LPWSTR name;
    LPWSTR printername;
    monitor_t * pm;
    HANDLE hXcv;
} printer_t;

/* ############################### */

static struct list monitor_handles = LIST_INIT( monitor_handles );
static monitor_t * pm_localport;

static const PRINTPROVIDOR * pprovider = NULL;

static const WCHAR backslashW[] = {'\\',0};
static const WCHAR bs_ports_bsW[] = {'\\','P','o','r','t','s','\\',0};
static const WCHAR configuration_fileW[] = {'C','o','n','f','i','g','u','r','a','t','i','o','n',' ','F','i','l','e',0};
static const WCHAR datatypeW[] = {'D','a','t','a','t','y','p','e',0};
static const WCHAR data_fileW[] = {'D','a','t','a',' ','F','i','l','e',0};
static const WCHAR dependent_filesW[] = {'D','e','p','e','n','d','e','n','t',' ','F','i','l','e','s',0};
static const WCHAR driverW[] = {'D','r','i','v','e','r',0};
static const WCHAR emptyW[] = {0};
static const WCHAR fmt_driversW[] = { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'c','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'E','n','v','i','r','o','n','m','e','n','t','s','\\',
                                  '%','s','\\','D','r','i','v','e','r','s','%','s',0 };
static const WCHAR fmt_printprocessorsW[] = { 'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'E','n','v','i','r','o','n','m','e','n','t','s','\\','%','s','\\',
                                  'P','r','i','n','t',' ','P','r','o','c','e','s','s','o','r','s',0 };
static const WCHAR help_fileW[] = {'H','e','l','p',' ','F','i','l','e',0};
static const WCHAR ia64_envnameW[] = {'W','i','n','d','o','w','s',' ','I','A','6','4',0};
static const WCHAR ia64_subdirW[] = {'i','a','6','4',0};
static const WCHAR localportW[] = {'L','o','c','a','l',' ','P','o','r','t',0};
static const WCHAR monitorW[] = {'M','o','n','i','t','o','r',0};
static const WCHAR monitorsW[] = {'S','y','s','t','e','m','\\',
                                'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                'C','o','n','t','r','o','l','\\',
                                'P','r','i','n','t','\\',
                                'M','o','n','i','t','o','r','s','\\',0};
static const WCHAR monitorUIW[] = {'M','o','n','i','t','o','r','U','I',0};
static const WCHAR previous_namesW[] = {'P','r','e','v','i','o','u','s',' ','N','a','m','e','s',0};
static const WCHAR printersW[] = {'S','y','s','t','e','m','\\',
                                  'C','u', 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'P','r','i','n','t','\\',
                                  'P','r','i','n','t','e','r','s',0};
static const WCHAR spoolW[] = {'\\','s','p','o','o','l',0};
static const WCHAR driversW[] = {'\\','d','r','i','v','e','r','s','\\',0};
static const WCHAR spoolprtprocsW[] = {'\\','s','p','o','o','l','\\','p','r','t','p','r','o','c','s','\\',0};
static const WCHAR version0_regpathW[] = {'\\','V','e','r','s','i','o','n','-','0',0};
static const WCHAR version0_subdirW[] = {'\\','0',0};
static const WCHAR version3_regpathW[] = {'\\','V','e','r','s','i','o','n','-','3',0};
static const WCHAR version3_subdirW[] = {'\\','3',0};
static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};
static const WCHAR win40_envnameW[] = {'W','i','n','d','o','w','s',' ','4','.','0',0};
static const WCHAR win40_subdirW[] = {'w','i','n','4','0',0};
static const WCHAR winnt_cv_portsW[] = {'S','o','f','t','w','a','r','e','\\',
                                        'M','i','c','r','o','s','o','f','t','\\',
                                        'W','i','n','d','o','w','s',' ','N','T','\\',
                                        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                        'P','o','r','t','s',0};
static const WCHAR winprintW[] = {'w','i','n','p','r','i','n','t',0};
static const WCHAR x64_envnameW[] = {'W','i','n','d','o','w','s',' ','x','6','4',0};
static const WCHAR x64_subdirW[] = {'x','6','4',0};
static const WCHAR x86_envnameW[] = {'W','i','n','d','o','w','s',' ','N','T',' ','x','8','6',0};
static const WCHAR x86_subdirW[] = {'w','3','2','x','8','6',0};
static const WCHAR XcvMonitorW[] = {',','X','c','v','M','o','n','i','t','o','r',' ',0};
static const WCHAR XcvPortW[] = {',','X','c','v','P','o','r','t',' ',0};


static const printenv_t env_ia64 =  {ia64_envnameW, ia64_subdirW, 3,
                                     version3_regpathW, version3_subdirW};

static const printenv_t env_x86 =   {x86_envnameW, x86_subdirW, 3,
                                     version3_regpathW, version3_subdirW};

static const printenv_t env_x64 =   {x64_envnameW, x64_subdirW, 3,
                                     version3_regpathW, version3_subdirW};

static const printenv_t env_win40 = {win40_envnameW, win40_subdirW, 0,
                                     version0_regpathW, version0_subdirW};

static const printenv_t * const all_printenv[] = {&env_x86, &env_x64, &env_ia64, &env_win40};


static const DWORD di_sizeof[] = {0, sizeof(DRIVER_INFO_1W), sizeof(DRIVER_INFO_2W),
                                     sizeof(DRIVER_INFO_3W), sizeof(DRIVER_INFO_4W),
                                     sizeof(DRIVER_INFO_5W), sizeof(DRIVER_INFO_6W),
                                  0, sizeof(DRIVER_INFO_8W)};


/******************************************************************
 * strdupW [internal]
 *
 * create a copy of a unicode-string
 *
 */
static LPWSTR strdupW(LPCWSTR p)
{
    LPWSTR ret;
    DWORD len;

    if(!p) return NULL;
    len = (lstrlenW(p) + 1) * sizeof(WCHAR);
    ret = heap_alloc(len);
    if (ret) memcpy(ret, p, len);
    return ret;
}

/******************************************************************
 *  apd_copyfile [internal]
 *
 * Copy a file from the driverdirectory to the versioned directory
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL apd_copyfile( WCHAR *pathname, WCHAR *file_part, apd_data_t *apd )
{
    WCHAR *srcname;
    BOOL res;

    apd->src[apd->srclen] = '\0';
    apd->dst[apd->dstlen] = '\0';

    if (!pathname || !pathname[0]) {
        /* nothing to copy */
        return TRUE;
    }

    if (apd->copyflags & APD_COPY_FROM_DIRECTORY)
        srcname = pathname;
    else
    {
        srcname = apd->src;
        strcatW( srcname, file_part );
    }
    strcatW( apd->dst, file_part );

    TRACE("%s => %s\n", debugstr_w(srcname), debugstr_w(apd->dst));

    /* FIXME: handle APD_COPY_NEW_FILES */
    res = CopyFileW(srcname, apd->dst, FALSE);
    TRACE("got %d with %u\n", res, GetLastError());

    return apd->lazy || res;
}

/******************************************************************
 * copy_servername_from_name  (internal)
 *
 * for an external server, the serverpart from the name is copied.
 *
 * RETURNS
 *  the length (in WCHAR) of the serverpart (0 for the local computer)
 *  (-length), when the name is too long
 *
 */
static LONG copy_servername_from_name(LPCWSTR name, LPWSTR target)
{
    LPCWSTR server;
    LPWSTR  ptr;
    WCHAR   buffer[MAX_COMPUTERNAME_LENGTH +1];
    DWORD   len;
    DWORD   serverlen;

    if (target) *target = '\0';

    if (name == NULL) return 0;
    if ((name[0] != '\\') || (name[1] != '\\')) return 0;

    server = &name[2];
    /* skip over both backslash, find separator '\' */
    ptr = strchrW(server, '\\');
    serverlen = (ptr) ? ptr - server : lstrlenW(server);

    /* servername is empty */
    if (serverlen == 0) return 0;

    TRACE("found %s\n", debugstr_wn(server, serverlen));

    if (serverlen > MAX_COMPUTERNAME_LENGTH) return -serverlen;

    if (target) {
        memcpy(target, server, serverlen * sizeof(WCHAR));
        target[serverlen] = '\0';
    }

    len = sizeof(buffer) / sizeof(buffer[0]);
    if (GetComputerNameW(buffer, &len)) {
        if ((serverlen == len) && (strncmpiW(server, buffer, len) == 0)) {
            /* The requested Servername is our computername */
            return 0;
        }
    }
    return serverlen;
}

/******************************************************************
 * get_basename_from_name  (internal)
 *
 * skip over the serverpart from the full name
 *
 */
static LPCWSTR get_basename_from_name(LPCWSTR name)
{
    if (name == NULL)  return NULL;
    if ((name[0] == '\\') && (name[1] == '\\')) {
        /* skip over the servername and search for the following '\'  */
        name = strchrW(&name[2], '\\');
        if ((name) && (name[1])) {
            /* found a separator ('\') followed by a name:
               skip over the separator and return the rest */
            name++;
        }
        else
        {
            /* no basename present (we found only a servername) */
            return NULL;
        }
    }
    return name;
}

/******************************************************************
 * monitor_unload [internal]
 *
 * release a printmonitor and unload it from memory, when needed
 *
 */
static void monitor_unload(monitor_t * pm)
{
    if (pm == NULL) return;
    TRACE("%p (refcount: %d) %s\n", pm, pm->refcount, debugstr_w(pm->name));

    EnterCriticalSection(&monitor_handles_cs);

    if (pm->refcount) pm->refcount--;

    if (pm->refcount == 0) {
        list_remove(&pm->entry);
        FreeLibrary(pm->hdll);
        heap_free(pm->name);
        heap_free(pm->dllname);
        heap_free(pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
}

/******************************************************************
 * monitor_unloadall [internal]
 *
 * release all registered printmonitors and unload them from memory, when needed
 *
 */

static void monitor_unloadall(void)
{
    monitor_t * pm;
    monitor_t * next;

    EnterCriticalSection(&monitor_handles_cs);
    /* iterate through the list, with safety against removal */
    LIST_FOR_EACH_ENTRY_SAFE(pm, next, &monitor_handles, monitor_t, entry)
    {
        /* skip monitorui dlls */
        if (pm->monitor) monitor_unload(pm);
    }
    LeaveCriticalSection(&monitor_handles_cs);
}

/******************************************************************
 * monitor_load [internal]
 *
 * load a printmonitor, get the dllname from the registry, when needed
 * initialize the monitor and dump found function-pointers
 *
 * On failure, SetLastError() is called and NULL is returned
 */

static monitor_t * monitor_load(LPCWSTR name, LPWSTR dllname)
{
    LPMONITOR2  (WINAPI *pInitializePrintMonitor2) (PMONITORINIT, LPHANDLE);
    PMONITORUI  (WINAPI *pInitializePrintMonitorUI)(VOID);
    LPMONITOREX (WINAPI *pInitializePrintMonitor)  (LPWSTR);
    DWORD (WINAPI *pInitializeMonitorEx)(LPWSTR, LPMONITOR);
    DWORD (WINAPI *pInitializeMonitor)  (LPWSTR);

    monitor_t * pm = NULL;
    monitor_t * cursor;
    LPWSTR  regroot = NULL;
    LPWSTR  driver = dllname;

    TRACE("(%s, %s)\n", debugstr_w(name), debugstr_w(dllname));
    /* Is the Monitor already loaded? */
    EnterCriticalSection(&monitor_handles_cs);

    if (name) {
        LIST_FOR_EACH_ENTRY(cursor, &monitor_handles, monitor_t, entry)
        {
            if (cursor->name && (lstrcmpW(name, cursor->name) == 0)) {
                pm = cursor;
                break;
            }
        }
    }

    if (pm == NULL) {
        pm = heap_alloc_zero(sizeof(monitor_t));
        if (pm == NULL) goto cleanup;
        list_add_tail(&monitor_handles, &pm->entry);
    }
    pm->refcount++;

    if (pm->name == NULL) {
        /* Load the monitor */
        LPMONITOREX pmonitorEx;
        DWORD   len;

        if (name) {
            len = lstrlenW(monitorsW) + lstrlenW(name) + 2;
            regroot = heap_alloc(len * sizeof(WCHAR));
        }

        if (regroot) {
            lstrcpyW(regroot, monitorsW);
            lstrcatW(regroot, name);
            /* Get the Driver from the Registry */
            if (driver == NULL) {
                HKEY    hroot;
                DWORD   namesize;
                if (RegOpenKeyW(HKEY_LOCAL_MACHINE, regroot, &hroot) == ERROR_SUCCESS) {
                    if (RegQueryValueExW(hroot, driverW, NULL, NULL, NULL,
                                        &namesize) == ERROR_SUCCESS) {
                        driver = heap_alloc(namesize);
                        RegQueryValueExW(hroot, driverW, NULL, NULL, (LPBYTE) driver, &namesize) ;
                    }
                    RegCloseKey(hroot);
                }
            }
        }

        pm->name = strdupW(name);
        pm->dllname = strdupW(driver);

        if ((name && (!regroot || !pm->name)) || !pm->dllname) {
            monitor_unload(pm);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            pm = NULL;
            goto cleanup;
        }

        pm->hdll = LoadLibraryW(driver);
        TRACE("%p: LoadLibrary(%s) => %d\n", pm->hdll, debugstr_w(driver), GetLastError());

        if (pm->hdll == NULL) {
            monitor_unload(pm);
            SetLastError(ERROR_MOD_NOT_FOUND);
            pm = NULL;
            goto cleanup;
        }

        pInitializePrintMonitor2  = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitor2");
        pInitializePrintMonitorUI = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitorUI");
        pInitializePrintMonitor   = (void *)GetProcAddress(pm->hdll, "InitializePrintMonitor");
        pInitializeMonitorEx = (void *)GetProcAddress(pm->hdll, "InitializeMonitorEx");
        pInitializeMonitor   = (void *)GetProcAddress(pm->hdll, "InitializeMonitor");


        TRACE("%p: %s,pInitializePrintMonitor2\n", pInitializePrintMonitor2, debugstr_w(driver));
        TRACE("%p: %s,pInitializePrintMonitorUI\n", pInitializePrintMonitorUI, debugstr_w(driver));
        TRACE("%p: %s,pInitializePrintMonitor\n", pInitializePrintMonitor, debugstr_w(driver));
        TRACE("%p: %s,pInitializeMonitorEx\n", pInitializeMonitorEx, debugstr_w(driver));
        TRACE("%p: %s,pInitializeMonitor\n", pInitializeMonitor, debugstr_w(driver));

        if (pInitializePrintMonitorUI  != NULL) {
            pm->monitorUI = pInitializePrintMonitorUI();
            TRACE("%p: MONITORUI from %s,InitializePrintMonitorUI()\n", pm->monitorUI, debugstr_w(driver));
            if (pm->monitorUI) {
                TRACE("0x%08x: dwMonitorSize (%d)\n",
                        pm->monitorUI->dwMonitorUISize, pm->monitorUI->dwMonitorUISize);

            }
        }

        if (pInitializePrintMonitor && regroot) {
            pmonitorEx = pInitializePrintMonitor(regroot);
            TRACE("%p: LPMONITOREX from %s,InitializePrintMonitor(%s)\n",
                    pmonitorEx, debugstr_w(driver), debugstr_w(regroot));

            if (pmonitorEx) {
                pm->dwMonitorSize = pmonitorEx->dwMonitorSize;
                pm->monitor = &(pmonitorEx->Monitor);
            }
        }

        if (pm->monitor) {
            TRACE("0x%08x: dwMonitorSize (%d)\n", pm->dwMonitorSize, pm->dwMonitorSize);

        }

        if (!pm->monitor && regroot) {
            if (pInitializePrintMonitor2 != NULL) {
                FIXME("%s,InitializePrintMonitor2 not implemented\n", debugstr_w(driver));
            }
            if (pInitializeMonitorEx != NULL) {
                FIXME("%s,InitializeMonitorEx not implemented\n", debugstr_w(driver));
            }
            if (pInitializeMonitor != NULL) {
                FIXME("%s,InitializeMonitor not implemented\n", debugstr_w(driver));
            }
        }
        if (!pm->monitor && !pm->monitorUI) {
            monitor_unload(pm);
            SetLastError(ERROR_PROC_NOT_FOUND);
            pm = NULL;
        }
    }
cleanup:
    if ((pm_localport ==  NULL) && (pm != NULL) && (lstrcmpW(pm->name, localportW) == 0)) {
        pm->refcount++;
        pm_localport = pm;
    }
    LeaveCriticalSection(&monitor_handles_cs);
    if (driver != dllname) heap_free(driver);
    heap_free(regroot);
    TRACE("=> %p\n", pm);
    return pm;
}

/******************************************************************
 * monitor_loadall [internal]
 *
 * Load all registered monitors
 *
 */
static DWORD monitor_loadall(void)
{
    monitor_t * pm;
    DWORD   registered = 0;
    DWORD   loaded = 0;
    HKEY    hmonitors;
    WCHAR   buffer[MAX_PATH];
    DWORD   id = 0;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hmonitors) == ERROR_SUCCESS) {
        RegQueryInfoKeyW(hmonitors, NULL, NULL, NULL, &registered, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL);

        TRACE("%d monitors registered\n", registered);

        while (id < registered) {
            buffer[0] = '\0';
            RegEnumKeyW(hmonitors, id, buffer, MAX_PATH);
            pm = monitor_load(buffer, NULL);
            if (pm) loaded++;
            id++;
        }
        RegCloseKey(hmonitors);
    }
    TRACE("%d monitors loaded\n", loaded);
    return loaded;
}

/******************************************************************
 * monitor_loadui [internal]
 *
 * load the userinterface-dll for a given portmonitor
 *
 * On failure, NULL is returned
 */
static monitor_t * monitor_loadui(monitor_t * pm)
{
    monitor_t * pui = NULL;
    WCHAR   buffer[MAX_PATH];
    HANDLE  hXcv;
    DWORD   len;
    DWORD   res;

    if (pm == NULL) return NULL;
    TRACE("(%p) => dllname: %s\n", pm, debugstr_w(pm->dllname));

    /* Try the Portmonitor first; works for many monitors */
    if (pm->monitorUI) {
        EnterCriticalSection(&monitor_handles_cs);
        pm->refcount++;
        LeaveCriticalSection(&monitor_handles_cs);
        return pm;
    }

    /* query the userinterface-dllname from the Portmonitor */
    if ((pm->monitor) && (pm->monitor->pfnXcvDataPort)) {
        /* building (",XcvMonitor %s",pm->name) not needed yet */
        res = pm->monitor->pfnXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv);
        TRACE("got %u with %p\n", res, hXcv);
        if (res) {
            res = pm->monitor->pfnXcvDataPort(hXcv, monitorUIW, NULL, 0, (BYTE *) buffer, sizeof(buffer), &len);
            TRACE("got %u with %s\n", res, debugstr_w(buffer));
            if (res == ERROR_SUCCESS) pui = monitor_load(NULL, buffer);
            pm->monitor->pfnXcvClosePort(hXcv);
        }
    }
    return pui;
}

/******************************************************************
 * monitor_load_by_port [internal]
 *
 * load a printmonitor for a given port
 *
 * On failure, NULL is returned
 */

static monitor_t * monitor_load_by_port(LPCWSTR portname)
{
    HKEY    hroot;
    HKEY    hport;
    LPWSTR  buffer;
    monitor_t * pm = NULL;
    DWORD   registered = 0;
    DWORD   id = 0;
    DWORD   len;

    TRACE("(%s)\n", debugstr_w(portname));

    /* Try the Local Monitor first */
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, winnt_cv_portsW, &hroot) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hroot, portname, NULL, NULL, NULL, &len) == ERROR_SUCCESS) {
            /* found the portname */
            RegCloseKey(hroot);
            return monitor_load(localportW, NULL);
        }
        RegCloseKey(hroot);
    }

    len = MAX_PATH + lstrlenW(bs_ports_bsW) + lstrlenW(portname) + 1;
    buffer = heap_alloc(len * sizeof(WCHAR));
    if (buffer == NULL) return NULL;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) == ERROR_SUCCESS) {
        EnterCriticalSection(&monitor_handles_cs);
        RegQueryInfoKeyW(hroot, NULL, NULL, NULL, &registered, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        while ((pm == NULL) && (id < registered)) {
            buffer[0] = '\0';
            RegEnumKeyW(hroot, id, buffer, MAX_PATH);
            TRACE("testing %s\n", debugstr_w(buffer));
            len = lstrlenW(buffer);
            lstrcatW(buffer, bs_ports_bsW);
            lstrcatW(buffer, portname);
            if (RegOpenKeyW(hroot, buffer, &hport) == ERROR_SUCCESS) {
                RegCloseKey(hport);
                buffer[len] = '\0';             /* use only the Monitor-Name */
                pm = monitor_load(buffer, NULL);
            }
            id++;
        }
        LeaveCriticalSection(&monitor_handles_cs);
        RegCloseKey(hroot);
    }
    heap_free(buffer);
    return pm;
}

/******************************************************************
 * Return the number of bytes for an multi_sz string.
 * The result includes all \0s
 * (specifically the extra \0, that is needed as multi_sz terminator).
 */
static int multi_sz_lenW(const WCHAR *str)
{
    const WCHAR *ptr = str;
    if (!str) return 0;
    do
    {
        ptr += lstrlenW(ptr) + 1;
    } while (*ptr);

    return (ptr - str + 1) * sizeof(WCHAR);
}

/******************************************************************
 * validate_envW [internal]
 *
 * validate the user-supplied printing-environment
 *
 * PARAMS
 *  env  [I] PTR to Environment-String or NULL
 *
 * RETURNS
 *  Success:  PTR to printenv_t
 *  Failure:  NULL and ERROR_INVALID_ENVIRONMENT
 *
 * NOTES
 *  An empty string is handled the same way as NULL.
 *
 */

static const  printenv_t * validate_envW(LPCWSTR env)
{
    const printenv_t *result = NULL;
    unsigned int i;

    TRACE("(%s)\n", debugstr_w(env));
    if (env && env[0])
    {
        for (i = 0; i < sizeof(all_printenv)/sizeof(all_printenv[0]); i++)
        {
            if (lstrcmpiW(env, all_printenv[i]->envname) == 0)
            {
                result = all_printenv[i];
                break;
            }
        }
        if (result == NULL) {
            FIXME("unsupported Environment: %s\n", debugstr_w(env));
            SetLastError(ERROR_INVALID_ENVIRONMENT);
        }
        /* on win9x, only "Windows 4.0" is allowed, but we ignore this */
    }
    else
    {
        result = (GetVersion() & 0x80000000) ? &env_win40 : &env_x86;
    }

    TRACE("=> using %p: %s\n", result, debugstr_w(result ? result->envname : NULL));
    return result;
}

/*****************************************************************************
 * enumerate the local monitors (INTERNAL)
 *
 * returns the needed size (in bytes) for pMonitors
 * and  *lpreturned is set to number of entries returned in pMonitors
 *
 * Language-Monitors are also installed in the same Registry-Location but
 * they are filtered in Windows (not returned by EnumMonitors).
 * We do no filtering to simplify our Code.
 *
 */
static DWORD get_local_monitors(DWORD level, LPBYTE pMonitors, DWORD cbBuf, LPDWORD lpreturned)
{
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    LPWSTR  ptr;
    LPMONITOR_INFO_2W mi;
    WCHAR   buffer[MAX_PATH];
    WCHAR   dllname[MAX_PATH];
    DWORD   dllsize;
    DWORD   len;
    DWORD   index = 0;
    DWORD   needed = 0;
    DWORD   numentries;
    DWORD   entrysize;

    entrysize = (level == 1) ? sizeof(MONITOR_INFO_1W) : sizeof(MONITOR_INFO_2W);

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    len = entrysize * numentries;
    ptr = (LPWSTR) &pMonitors[len];

    numentries = 0;
    len = sizeof(buffer)/sizeof(buffer[0]);
    buffer[0] = '\0';

    /* Windows creates the "Monitors"-Key on reboot / start "spooler" */
    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) == ERROR_SUCCESS) {
        /* Scan all Monitor-Registry-Keys */
        while (RegEnumKeyExW(hroot, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            TRACE("Monitor_%d: %s\n", numentries, debugstr_w(buffer));
            dllsize = sizeof(dllname);
            dllname[0] = '\0';

            /* The Monitor must have a Driver-DLL */
            if (RegOpenKeyExW(hroot, buffer, 0, KEY_READ, &hentry) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hentry, driverW, NULL, NULL, (LPBYTE) dllname, &dllsize) == ERROR_SUCCESS) {
                    /* We found a valid DLL for this Monitor. */
                    TRACE("using Driver: %s\n", debugstr_w(dllname));
                }
                RegCloseKey(hentry);
            }

            /* Windows returns only Port-Monitors here, but to simplify our code,
               we do no filtering for Language-Monitors */
            if (dllname[0]) {
                numentries++;
                needed += entrysize;
                needed += (len+1) * sizeof(WCHAR);  /* len is lstrlenW(monitorname) */
                if (level > 1) {
                    /* we install and return only monitors for "Windows NT x86" */
                    needed += (lstrlenW(x86_envnameW) +1) * sizeof(WCHAR);
                    needed += dllsize;
                }

                /* required size is calculated. Now fill the user-buffer */
                if (pMonitors && (cbBuf >= needed)){
                    mi = (LPMONITOR_INFO_2W) pMonitors;
                    pMonitors += entrysize;

                    TRACE("%p: writing MONITOR_INFO_%dW #%d\n", mi, level, numentries);
                    mi->pName = ptr;
                    lstrcpyW(ptr, buffer);      /* Name of the Monitor */
                    ptr += (len+1);               /* len is lstrlenW(monitorname) */
                    if (level > 1) {
                        mi->pEnvironment = ptr;
                        lstrcpyW(ptr, x86_envnameW); /* fixed to "Windows NT x86" */
                        ptr += (lstrlenW(x86_envnameW)+1);

                        mi->pDLLName = ptr;
                        lstrcpyW(ptr, dllname);         /* Name of the Driver-DLL */
                        ptr += (dllsize / sizeof(WCHAR));
                    }
                }
            }
            index++;
            len = sizeof(buffer)/sizeof(buffer[0]);
            buffer[0] = '\0';
        }
        RegCloseKey(hroot);
    }
    *lpreturned = numentries;
    TRACE("need %d byte for %d entries\n", needed, numentries);
    return needed;
}

/*****************************************************************************
 * enumerate the local print processors (INTERNAL)
 *
 * returns the needed size (in bytes) for pPPInfo
 * and  *lpreturned is set to number of entries returned in pPPInfo
 *
 */
static DWORD get_local_printprocessors(LPWSTR regpathW, LPBYTE pPPInfo, DWORD cbBuf, LPDWORD lpreturned)
{
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    LPWSTR  ptr;
    PPRINTPROCESSOR_INFO_1W ppi;
    WCHAR   buffer[MAX_PATH];
    WCHAR   dllname[MAX_PATH];
    DWORD   dllsize;
    DWORD   len;
    DWORD   index = 0;
    DWORD   needed = 0;
    DWORD   numentries;

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    len = numentries * sizeof(PRINTPROCESSOR_INFO_1W);
    ptr = (LPWSTR) &pPPInfo[len];

    numentries = 0;
    len = sizeof(buffer)/sizeof(buffer[0]);
    buffer[0] = '\0';

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, regpathW, &hroot) == ERROR_SUCCESS) {
        /* add "winprint" first */
        numentries++;
        needed = sizeof(PRINTPROCESSOR_INFO_1W) + sizeof(winprintW);
        if (pPPInfo && (cbBuf >= needed)){
            ppi = (PPRINTPROCESSOR_INFO_1W) pPPInfo;
            pPPInfo += sizeof(PRINTPROCESSOR_INFO_1W);

            TRACE("%p: writing PRINTPROCESSOR_INFO_1W #%d\n", ppi, numentries);
            ppi->pName = ptr;
            lstrcpyW(ptr, winprintW);      /* Name of the Print Processor */
            ptr += sizeof(winprintW) / sizeof(WCHAR);
        }

        /* Scan all Printprocessor Keys */
        while ((RegEnumKeyExW(hroot, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) &&
            (lstrcmpiW(buffer, winprintW) != 0)) {
            TRACE("PrintProcessor_%d: %s\n", numentries, debugstr_w(buffer));
            dllsize = sizeof(dllname);
            dllname[0] = '\0';

            /* The Print Processor must have a Driver-DLL */
            if (RegOpenKeyExW(hroot, buffer, 0, KEY_READ, &hentry) == ERROR_SUCCESS) {
                if (RegQueryValueExW(hentry, driverW, NULL, NULL, (LPBYTE) dllname, &dllsize) == ERROR_SUCCESS) {
                    /* We found a valid DLL for this Print Processor */
                    TRACE("using Driver: %s\n", debugstr_w(dllname));
                }
                RegCloseKey(hentry);
            }

            if (dllname[0]) {
                numentries++;
                needed += sizeof(PRINTPROCESSOR_INFO_1W);
                needed += (len+1) * sizeof(WCHAR);  /* len is lstrlenW(printprocessor name) */

                /* required size is calculated. Now fill the user-buffer */
                if (pPPInfo && (cbBuf >= needed)){
                    ppi = (PPRINTPROCESSOR_INFO_1W) pPPInfo;
                    pPPInfo += sizeof(PRINTPROCESSOR_INFO_1W);

                    TRACE("%p: writing PRINTPROCESSOR_INFO_1W #%d\n", ppi, numentries);
                    ppi->pName = ptr;
                    lstrcpyW(ptr, buffer);      /* Name of the Print Processor */
                    ptr += (len+1);             /* len is lstrlenW(printprosessor name) */
                }
            }
            index++;
            len = sizeof(buffer)/sizeof(buffer[0]);
            buffer[0] = '\0';
        }
        RegCloseKey(hroot);
    }
    *lpreturned = numentries;
    TRACE("need %d byte for %d entries\n", needed, numentries);
    return needed;
}

/******************************************************************
 * enumerate the local Ports from all loaded monitors (internal)
 *
 * returns the needed size (in bytes) for pPorts
 * and  *lpreturned is set to number of entries returned in pPorts
 *
 */
static DWORD get_ports_from_all_monitors(DWORD level, LPBYTE pPorts, DWORD cbBuf, LPDWORD lpreturned)
{
    monitor_t * pm;
    LPWSTR      ptr;
    LPPORT_INFO_2W cache;
    LPPORT_INFO_2W out;
    LPBYTE  pi_buffer = NULL;
    DWORD   pi_allocated = 0;
    DWORD   pi_needed;
    DWORD   pi_index;
    DWORD   pi_returned;
    DWORD   res;
    DWORD   outindex = 0;
    DWORD   needed;
    DWORD   numentries;
    DWORD   entrysize;


    TRACE("(%d, %p, %d, %p)\n", level, pPorts, cbBuf, lpreturned);
    entrysize = (level == 1) ? sizeof(PORT_INFO_1W) : sizeof(PORT_INFO_2W);

    numentries = *lpreturned;       /* this is 0, when we scan the registry */
    needed = entrysize * numentries;
    ptr = (LPWSTR) &pPorts[needed];

    numentries = 0;
    needed = 0;

    LIST_FOR_EACH_ENTRY(pm, &monitor_handles, monitor_t, entry)
    {
        if ((pm->monitor) && (pm->monitor->pfnEnumPorts)) {
            pi_needed = 0;
            pi_returned = 0;
            res = pm->monitor->pfnEnumPorts(NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                /* Do not use heap_realloc (we do not need the old data in the buffer) */
                heap_free(pi_buffer);
                pi_buffer = heap_alloc(pi_needed);
                pi_allocated = (pi_buffer) ? pi_needed : 0;
                res = pm->monitor->pfnEnumPorts(NULL, level, pi_buffer, pi_allocated, &pi_needed, &pi_returned);
            }
            TRACE("(%s) got %d with %d (need %d byte for %d entries)\n",
                  debugstr_w(pm->name), res, GetLastError(), pi_needed, pi_returned);

            numentries += pi_returned;
            needed += pi_needed;

            /* fill the output-buffer (pPorts), if we have one */
            if (pPorts && (cbBuf >= needed ) && pi_buffer) {
                pi_index = 0;
                while (pi_returned > pi_index) {
                    cache = (LPPORT_INFO_2W) &pi_buffer[pi_index * entrysize];
                    out = (LPPORT_INFO_2W) &pPorts[outindex * entrysize];
                    out->pPortName = ptr;
                    lstrcpyW(ptr, cache->pPortName);
                    ptr += (lstrlenW(ptr)+1);
                    if (level > 1) {
                        out->pMonitorName = ptr;
                        lstrcpyW(ptr,  cache->pMonitorName);
                        ptr += (lstrlenW(ptr)+1);

                        out->pDescription = ptr;
                        lstrcpyW(ptr,  cache->pDescription);
                        ptr += (lstrlenW(ptr)+1);
                        out->fPortType = cache->fPortType;
                        out->Reserved = cache->Reserved;
                    }
                    pi_index++;
                    outindex++;
                }
            }
        }
    }
    /* the temporary portinfo-buffer is no longer needed */
    heap_free(pi_buffer);

    *lpreturned = numentries;
    TRACE("need %d byte for %d entries\n", needed, numentries);
    return needed;
}


/*****************************************************************************
 * open_driver_reg [internal]
 *
 * opens the registry for the printer drivers depending on the given input
 * variable pEnvironment
 *
 * RETURNS:
 *    Success: the opened hkey
 *    Failure: NULL
 */
static HKEY open_driver_reg(LPCWSTR pEnvironment)
{
    HKEY  retval = NULL;
    LPWSTR buffer;
    const printenv_t * env;

    TRACE("(%s)\n", debugstr_w(pEnvironment));

    env = validate_envW(pEnvironment);
    if (!env) return NULL;

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(fmt_driversW) +
                (lstrlenW(env->envname) + lstrlenW(env->versionregpath)) * sizeof(WCHAR));

    if (buffer) {
        wsprintfW(buffer, fmt_driversW, env->envname, env->versionregpath);
        RegCreateKeyW(HKEY_LOCAL_MACHINE, buffer, &retval);
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    return retval;
}

/*****************************************************************************
 * fpGetPrinterDriverDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Printer-Drivers
 *
 * PARAMS
 *   pName            [I] Servername (NT only) or NULL (local Computer)
 *   pEnvironment     [I] Printing-Environment (see below) or NULL (Default)
 *   Level            [I] Structure-Level (must be 1)
 *   pDriverDirectory [O] PTR to Buffer that receives the Result
 *   cbBuf            [I] Size of Buffer at pDriverDirectory
 *   pcbNeeded        [O] PTR to DWORD that receives the size in Bytes used /
 *                        required for pDriverDirectory
 *
 * RETURNS
 *   Success: TRUE  and in pcbNeeded the Bytes used in pDriverDirectory
 *   Failure: FALSE and in pcbNeeded the Bytes required for pDriverDirectory,
 *            if cbBuf is too small
 *
 *   Native Values returned in pDriverDirectory on Success:
 *|  NT(Windows NT x86):  "%winsysdir%\\spool\\DRIVERS\\w32x86"
 *|  NT(Windows 4.0):     "%winsysdir%\\spool\\DRIVERS\\win40"
 *|  win9x(Windows 4.0):  "%winsysdir%"
 *
 *   "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
static BOOL WINAPI fpGetPrinterDriverDirectory(LPWSTR pName, LPWSTR pEnvironment,
            DWORD Level, LPBYTE pDriverDirectory, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD needed;
    const printenv_t * env;
    WCHAR * const dir = (WCHAR *)pDriverDirectory;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName),
          debugstr_w(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (pName != NULL && pName[0]) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env) return FALSE;  /* pEnvironment invalid or unsupported */


    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spoolW);
    needed += lstrlenW(driversW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (dir == NULL) {
        /* ERROR_INVALID_USER_BUFFER is NT, ERROR_INVALID_PARAMETER is win9x */
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW( dir, cbBuf / sizeof(WCHAR) );
    /* add the Subdirectories */
    lstrcatW( dir, spoolW );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, driversW );
    CreateDirectoryW( dir, NULL );
    lstrcatW( dir, env->subdir );
    CreateDirectoryW( dir, NULL );

    TRACE( "=> %s\n", debugstr_w( dir ) );
    return TRUE;
}

/******************************************************************
 * driver_load [internal]
 *
 * load a driver user interface dll
 *
 * On failure, NULL is returned
 *
 */

static HMODULE driver_load(const printenv_t * env, LPWSTR dllname)
{
    WCHAR fullname[MAX_PATH];
    HMODULE hui;
    DWORD len;

    TRACE("(%p, %s)\n", env, debugstr_w(dllname));

    /* build the driverdir */
    len = sizeof(fullname) -
          (lstrlenW(env->versionsubdir) + 1 + lstrlenW(dllname) + 1) * sizeof(WCHAR);

    if (!fpGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1,
                                     (LPBYTE) fullname, len, &len)) {
        /* Should never fail */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return NULL;
    }

    lstrcatW(fullname, env->versionsubdir);
    lstrcatW(fullname, backslashW);
    lstrcatW(fullname, dllname);

    hui = LoadLibraryW(fullname);
    TRACE("%p: LoadLibrary(%s) %d\n", hui, debugstr_w(fullname), GetLastError());

    return hui;
}

/******************************************************************
 *  printer_free
 *  free the data pointer of an opened printer
 */
static VOID printer_free(printer_t * printer)
{
    if (printer->hXcv)
        printer->pm->monitor->pfnXcvClosePort(printer->hXcv);

    monitor_unload(printer->pm);

    heap_free(printer->printername);
    heap_free(printer->name);
    heap_free(printer);
}

/******************************************************************
 *  printer_alloc_handle
 *  alloc a printer handle and remember the data pointer in the printer handle table
 *
 */
static HANDLE printer_alloc_handle(LPCWSTR name, LPPRINTER_DEFAULTSW pDefault)
{
    WCHAR servername[MAX_COMPUTERNAME_LENGTH + 1];
    printer_t *printer = NULL;
    LPCWSTR printername;
    HKEY    hkeyPrinters;
    HKEY    hkeyPrinter;
    DWORD   len;

    if (copy_servername_from_name(name, servername)) {
        FIXME("server %s not supported\n", debugstr_w(servername));
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return NULL;
    }

    printername = get_basename_from_name(name);
    if (name != printername) TRACE("converted %s to %s\n", debugstr_w(name), debugstr_w(printername));

    /* an empty printername is invalid */
    if (printername && (!printername[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    printer = heap_alloc_zero(sizeof(printer_t));
    if (!printer) goto end;

    /* clone the base name. This is NULL for the printserver */
    printer->printername = strdupW(printername);

    /* clone the full name */
    printer->name = strdupW(name);
    if (name && (!printer->name)) {
        printer_free(printer);
        printer = NULL;
    }
    if (printername) {
        len = sizeof(XcvMonitorW)/sizeof(WCHAR) - 1;
        if (strncmpW(printername, XcvMonitorW, len) == 0) {
            /* OpenPrinter(",XcvMonitor ", ...) detected */
            TRACE(",XcvMonitor: %s\n", debugstr_w(&printername[len]));
            printer->pm = monitor_load(&printername[len], NULL);
            if (printer->pm == NULL) {
                printer_free(printer);
                SetLastError(ERROR_UNKNOWN_PORT);
                printer = NULL;
                goto end;
            }
        }
        else
        {
            len = sizeof(XcvPortW)/sizeof(WCHAR) - 1;
            if (strncmpW( printername, XcvPortW, len) == 0) {
                /* OpenPrinter(",XcvPort ", ...) detected */
                TRACE(",XcvPort: %s\n", debugstr_w(&printername[len]));
                printer->pm = monitor_load_by_port(&printername[len]);
                if (printer->pm == NULL) {
                    printer_free(printer);
                    SetLastError(ERROR_UNKNOWN_PORT);
                    printer = NULL;
                    goto end;
                }
            }
        }

        if (printer->pm) {
            if ((printer->pm->monitor) && (printer->pm->monitor->pfnXcvOpenPort)) {
                printer->pm->monitor->pfnXcvOpenPort(&printername[len],
                                                    pDefault ? pDefault->DesiredAccess : 0,
                                                    &printer->hXcv);
            }
            if (printer->hXcv == NULL) {
                printer_free(printer);
                SetLastError(ERROR_INVALID_PARAMETER);
                printer = NULL;
                goto end;
            }
        }
        else
        {
            /* Does the Printer exist? */
            if (RegCreateKeyW(HKEY_LOCAL_MACHINE, printersW, &hkeyPrinters) != ERROR_SUCCESS) {
                ERR("Can't create Printers key\n");
                printer_free(printer);
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                printer = NULL;
                goto end;
            }
            if (RegOpenKeyW(hkeyPrinters, printername, &hkeyPrinter) != ERROR_SUCCESS) {
                WARN("Printer not found in Registry: %s\n", debugstr_w(printername));
                RegCloseKey(hkeyPrinters);
                printer_free(printer);
                SetLastError(ERROR_INVALID_PRINTER_NAME);
                printer = NULL;
                goto end;
            }
            RegCloseKey(hkeyPrinter);
            RegCloseKey(hkeyPrinters);
        }
    }
    else
    {
        TRACE("using the local printserver\n");
    }

end:

    TRACE("==> %p\n", printer);
    return (HANDLE)printer;
}

static inline WCHAR *get_file_part( WCHAR *name )
{
    WCHAR *ptr = strrchrW( name, '\\' );
    if (ptr) return ptr + 1;
    return name;
}

/******************************************************************************
 *  myAddPrinterDriverEx [internal]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 * and a special mode with lazy error checking.
 *
 */
static BOOL myAddPrinterDriverEx(DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags, BOOL lazy)
{
    const printenv_t *env;
    apd_data_t apd;
    DRIVER_INFO_8W di;
    BOOL    (WINAPI *pDrvDriverEvent)(DWORD, DWORD, LPBYTE, LPARAM);
    HMODULE hui;
    WCHAR *file;
    HKEY    hroot;
    HKEY    hdrv;
    DWORD   disposition;
    DWORD   len;
    LONG    lres;
    BOOL    res;

    /* we need to set all entries in the Registry, independent from the Level of
       DRIVER_INFO, that the caller supplied */

    ZeroMemory(&di, sizeof(di));
    if (pDriverInfo && (level < (sizeof(di_sizeof) / sizeof(di_sizeof[0])))) {
        memcpy(&di, pDriverInfo, di_sizeof[level]);
    }

    /* dump the most used infos */
    TRACE("%p: .cVersion    : 0x%x/%d\n", pDriverInfo, di.cVersion, di.cVersion);
    TRACE("%p: .pName       : %s\n", di.pName, debugstr_w(di.pName));
    TRACE("%p: .pEnvironment: %s\n", di.pEnvironment, debugstr_w(di.pEnvironment));
    TRACE("%p: .pDriverPath : %s\n", di.pDriverPath, debugstr_w(di.pDriverPath));
    TRACE("%p: .pDataFile   : %s\n", di.pDataFile, debugstr_w(di.pDataFile));
    TRACE("%p: .pConfigFile : %s\n", di.pConfigFile, debugstr_w(di.pConfigFile));
    TRACE("%p: .pHelpFile   : %s\n", di.pHelpFile, debugstr_w(di.pHelpFile));
    /* dump only the first of the additional Files */
    TRACE("%p: .pDependentFiles: %s\n", di.pDependentFiles, debugstr_w(di.pDependentFiles));


    /* check environment */
    env = validate_envW(di.pEnvironment);
    if (env == NULL) return FALSE;        /* ERROR_INVALID_ENVIRONMENT */

    /* fill the copy-data / get the driverdir */
    len = sizeof(apd.src) - sizeof(version3_subdirW) - sizeof(WCHAR);
    if (!fpGetPrinterDriverDirectory(NULL, (LPWSTR) env->envname, 1,
                                    (LPBYTE) apd.src, len, &len)) {
        /* Should never fail */
        return FALSE;
    }
    memcpy(apd.dst, apd.src, len);
    lstrcatW(apd.src, backslashW);
    apd.srclen = lstrlenW(apd.src);
    lstrcatW(apd.dst, env->versionsubdir);
    lstrcatW(apd.dst, backslashW);
    apd.dstlen = lstrlenW(apd.dst);
    apd.copyflags = dwFileCopyFlags;
    apd.lazy = lazy;
    CreateDirectoryW(apd.src, NULL);
    CreateDirectoryW(apd.dst, NULL);

    hroot = open_driver_reg(env->envname);
    if (!hroot) {
        ERR("Can't create Drivers key\n");
        return FALSE;
    }

    /* Fill the Registry for the Driver */
    if ((lres = RegCreateKeyExW(hroot, di.pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_QUERY_VALUE, NULL,
                                &hdrv, &disposition)) != ERROR_SUCCESS) {

        ERR("can't create driver %s: %u\n", debugstr_w(di.pName), lres);
        RegCloseKey(hroot);
        SetLastError(lres);
        return FALSE;
    }
    RegCloseKey(hroot);

    /* Verified with the Adobe PS Driver, that w2k does not use di.Version */
    RegSetValueExW(hdrv, versionW, 0, REG_DWORD, (const BYTE*) &env->driverversion,
                   sizeof(DWORD));

    file = get_file_part( di.pDriverPath );
    RegSetValueExW( hdrv, driverW, 0, REG_SZ, (LPBYTE)file, (strlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDriverPath, file, &apd );

    file = get_file_part( di.pDataFile );
    RegSetValueExW( hdrv, data_fileW, 0, REG_SZ, (LPBYTE)file, (strlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pDataFile, file, &apd );

    file = get_file_part( di.pConfigFile );
    RegSetValueExW( hdrv, configuration_fileW, 0, REG_SZ, (LPBYTE)file, (strlenW( file ) + 1) * sizeof(WCHAR) );
    apd_copyfile( di.pConfigFile, file, &apd );

    /* settings for level 3 */
    if (di.pHelpFile)
    {
        file = get_file_part( di.pHelpFile );
        RegSetValueExW( hdrv, help_fileW, 0, REG_SZ, (LPBYTE)file, (strlenW( file ) + 1) * sizeof(WCHAR) );
        apd_copyfile( di.pHelpFile, file, &apd );
    }
    else
        RegSetValueExW( hdrv, help_fileW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW) );

    if (di.pDependentFiles && *di.pDependentFiles)
    {
        WCHAR *reg, *reg_ptr, *in_ptr;
        reg = reg_ptr = HeapAlloc( GetProcessHeap(), 0, multi_sz_lenW( di.pDependentFiles ) );

        for (in_ptr = di.pDependentFiles; *in_ptr; in_ptr += strlenW( in_ptr ) + 1)
        {
            file = get_file_part( in_ptr );
            len = strlenW( file ) + 1;
            memcpy( reg_ptr, file, len * sizeof(WCHAR) );
            reg_ptr += len;
            apd_copyfile( in_ptr, file, &apd );
        }
        *reg_ptr = 0;

        RegSetValueExW( hdrv, dependent_filesW, 0, REG_MULTI_SZ, (LPBYTE)reg, (reg_ptr - reg + 1) * sizeof(WCHAR) );
        HeapFree( GetProcessHeap(), 0, reg );
    }
    else
        RegSetValueExW(hdrv, dependent_filesW, 0, REG_MULTI_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    /* The language-Monitor was already copied by the caller to "%SystemRoot%\system32" */
    if (di.pMonitorName)
        RegSetValueExW(hdrv, monitorW, 0, REG_SZ, (LPBYTE) di.pMonitorName,
                       (lstrlenW(di.pMonitorName)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, monitorW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    if (di.pDefaultDataType)
        RegSetValueExW(hdrv, datatypeW, 0, REG_SZ, (LPBYTE) di.pDefaultDataType,
                       (lstrlenW(di.pDefaultDataType)+1)* sizeof(WCHAR));
    else
        RegSetValueExW(hdrv, datatypeW, 0, REG_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    /* settings for level 4 */
    if (di.pszzPreviousNames)
        RegSetValueExW(hdrv, previous_namesW, 0, REG_MULTI_SZ, (LPBYTE) di.pszzPreviousNames,
                       multi_sz_lenW(di.pszzPreviousNames));
    else
        RegSetValueExW(hdrv, previous_namesW, 0, REG_MULTI_SZ, (const BYTE*)emptyW, sizeof(emptyW));

    if (level > 5) TRACE("level %u for Driver %s is incomplete\n", level, debugstr_w(di.pName));

    RegCloseKey(hdrv);
    hui = driver_load(env, di.pConfigFile);
    pDrvDriverEvent = (void *)GetProcAddress(hui, "DrvDriverEvent");
    if (hui && pDrvDriverEvent) {

        /* Support for DrvDriverEvent is optional */
        TRACE("DRIVER_EVENT_INITIALIZE for %s (%s)\n", debugstr_w(di.pName), debugstr_w(di.pConfigFile));
        /* MSDN: level for DRIVER_INFO is 1 to 3 */
        res = pDrvDriverEvent(DRIVER_EVENT_INITIALIZE, 3, (LPBYTE) &di, 0);
        TRACE("got %d from DRIVER_EVENT_INITIALIZE\n", res);
    }
    FreeLibrary(hui);

    TRACE("=> TRUE with %u\n", GetLastError());
    return TRUE;

}

/******************************************************************************
 * fpAddMonitor [exported through PRINTPROVIDOR]
 *
 * Install a Printmonitor
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  Level       [I] Structure-Level (Must be 2)
 *  pMonitors   [I] PTR to MONITOR_INFO_2
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  All Files for the Monitor must already be copied to %winsysdir% ("%SystemRoot%\system32")
 *
 */
static BOOL WINAPI fpAddMonitor(LPWSTR pName, DWORD Level, LPBYTE pMonitors)
{
    monitor_t * pm = NULL;
    LPMONITOR_INFO_2W mi2w;
    HKEY    hroot = NULL;
    HKEY    hentry = NULL;
    DWORD   disposition;
    BOOL    res = FALSE;

    mi2w = (LPMONITOR_INFO_2W) pMonitors;
    TRACE("(%s, %d, %p): %s %s %s\n", debugstr_w(pName), Level, pMonitors,
            debugstr_w(mi2w ? mi2w->pName : NULL),
            debugstr_w(mi2w ? mi2w->pEnvironment : NULL),
            debugstr_w(mi2w ? mi2w->pDLLName : NULL));

    if (copy_servername_from_name(pName, NULL)) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if (!mi2w->pName || (! mi2w->pName[0])) {
        WARN("pName not valid : %s\n", debugstr_w(mi2w->pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!mi2w->pEnvironment || lstrcmpW(mi2w->pEnvironment, x86_envnameW)) {
        WARN("Environment %s requested (we support only %s)\n",
                debugstr_w(mi2w->pEnvironment), debugstr_w(x86_envnameW));
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

    if (!mi2w->pDLLName || (! mi2w->pDLLName[0])) {
        WARN("pDLLName not valid : %s\n", debugstr_w(mi2w->pDLLName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Load and initialize the monitor. SetLastError() is called on failure */
    if ((pm = monitor_load(mi2w->pName, mi2w->pDLLName)) == NULL) {
        return FALSE;
    }
    monitor_unload(pm);

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(monitorsW));
        return FALSE;
    }

    if (RegCreateKeyExW(hroot, mi2w->pName, 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_QUERY_VALUE, NULL, &hentry,
                        &disposition) == ERROR_SUCCESS) {

        /* Some installers set options for the port before calling AddMonitor.
           We query the "Driver" entry to verify that the monitor is installed,
           before we return an error.
           When a user installs two print monitors at the same time with the
           same name, a race condition is possible but silently ignored. */

        DWORD   namesize = 0;

        if ((disposition == REG_OPENED_EXISTING_KEY) &&
            (RegQueryValueExW(hentry, driverW, NULL, NULL, NULL,
                              &namesize) == ERROR_SUCCESS)) {
            TRACE("monitor %s already exists\n", debugstr_w(mi2w->pName));
            /* 9x use ERROR_ALREADY_EXISTS */
            SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        }
        else
        {
            INT len;
            len = (lstrlenW(mi2w->pDLLName) +1) * sizeof(WCHAR);
            res = (RegSetValueExW(hentry, driverW, 0, REG_SZ,
                    (LPBYTE) mi2w->pDLLName, len) == ERROR_SUCCESS);
        }
        RegCloseKey(hentry);
    }

    RegCloseKey(hroot);
    return (res);
}

/******************************************************************************
 * fpAddPort [exported through PRINTPROVIDOR]
 *
 * Add a Port for a specific Monitor
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  hWnd         [I] Handle to parent Window for the Dialog-Box
 *  pMonitorName [I] Name of the Monitor that manage the Port
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPort(LPWSTR pName, HWND hWnd, LPWSTR pMonitorName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* an empty Monitorname is Invalid */
    if (!pMonitorName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load(pMonitorName, NULL);
    if (pm && pm->monitor && pm->monitor->pfnAddPort) {
        res = pm->monitor->pfnAddPort(pName, hWnd, pMonitorName);
        TRACE("got %d with %u (%s)\n", res, GetLastError(), debugstr_w(pm->dllname));
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnAddPortUI) {
            res = pui->monitorUI->pfnAddPortUI(pName, hWnd, pMonitorName, NULL);
            TRACE("got %d with %u (%s)\n", res, GetLastError(), debugstr_w(pui->dllname));
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pMonitorName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/******************************************************************************
 * fpAddPortEx [exported through PRINTPROVIDOR]
 *
 * Add a Port for a specific Monitor, without presenting a user interface
 *
 * PARAMS
 *  pName         [I] Servername or NULL (local Computer)
 *  level         [I] Structure-Level (1 or 2) for pBuffer
 *  pBuffer       [I] PTR to: PORT_INFO_1 or PORT_INFO_2
 *  pMonitorName  [I] Name of the Monitor that manage the Port
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPortEx(LPWSTR pName, DWORD level, LPBYTE pBuffer, LPWSTR pMonitorName)
{
    PORT_INFO_2W * pi2;
    monitor_t * pm;
    DWORD lres;
    DWORD res;

    pi2 = (PORT_INFO_2W *) pBuffer;

    TRACE("(%s, %d, %p, %s): %s %s %s\n", debugstr_w(pName), level, pBuffer,
            debugstr_w(pMonitorName), debugstr_w(pi2 ? pi2->pPortName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pMonitorName : NULL),
            debugstr_w(((level > 1) && pi2) ? pi2->pDescription : NULL));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((level < 1) || (level > 2)) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ((!pi2) || (!pMonitorName) || (!pMonitorName[0])) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* load the Monitor */
    pm = monitor_load(pMonitorName, NULL);
    if (pm && pm->monitor && pm->monitor->pfnAddPortEx) {
        res = pm->monitor->pfnAddPortEx(pName, level, pBuffer, pMonitorName);
        TRACE("got %d with %u (%s)\n", res, GetLastError(), debugstr_w(pm->dllname));
    }
    else
    {
        FIXME("not implemented for %s (monitor %p: %s)\n",
            debugstr_w(pMonitorName), pm, pm ? debugstr_w(pm->dllname) : "(null)");
            SetLastError(ERROR_INVALID_PARAMETER);
            res = FALSE;
    }
    monitor_unload(pm);
    return res;
}

/******************************************************************************
 * fpAddPrinterDriverEx [exported through PRINTPROVIDOR]
 *
 * Install a Printer Driver with the Option to upgrade / downgrade the Files
 *
 * PARAMS
 *  pName           [I] Servername or NULL (local Computer)
 *  level           [I] Level for the supplied DRIVER_INFO_*W struct
 *  pDriverInfo     [I] PTR to DRIVER_INFO_*W struct with the Driver Parameter
 *  dwFileCopyFlags [I] How to Copy / Upgrade / Downgrade the needed Files
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpAddPrinterDriverEx(LPWSTR pName, DWORD level, LPBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    LONG lres;

    TRACE("(%s, %d, %p, 0x%x)\n", debugstr_w(pName), level, pDriverInfo, dwFileCopyFlags);
    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    if ((dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY) != APD_COPY_ALL_FILES) {
        TRACE("Flags 0x%x ignored (using APD_COPY_ALL_FILES)\n", dwFileCopyFlags & ~APD_COPY_FROM_DIRECTORY);
    }

    return myAddPrinterDriverEx(level, pDriverInfo, dwFileCopyFlags, TRUE);
}

/******************************************************************************
 * fpClosePrinter [exported through PRINTPROVIDOR]
 *
 * Close a printer handle and free associated resources
 *
 * PARAMS
 *  hPrinter [I] Printerhandle to close
 *
 * RESULTS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpClosePrinter(HANDLE hPrinter)
{
    printer_t *printer = (printer_t *) hPrinter;

    TRACE("(%p)\n", hPrinter);

    if (printer) {
        printer_free(printer);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * fpConfigurePort [exported through PRINTPROVIDOR]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpConfigurePort(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* an empty Portname is Invalid, but can popup a Dialog */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && pm->monitor && pm->monitor->pfnConfigurePort) {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor->pfnConfigurePort(pName, hWnd, pPortName);
        TRACE("got %d with %u\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnConfigurePortUI) {
            TRACE("use %s for %s (monitorui %p: %s)\n", debugstr_w(pui->name),
                        debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnConfigurePortUI(pName, hWnd, pPortName);
            TRACE("got %d with %u\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pPortName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/******************************************************************
 * fpDeleteMonitor [exported through PRINTPROVIDOR]
 *
 * Delete a specific Printmonitor from a Printing-Environment
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment of the Monitor or NULL (Default)
 *  pMonitorName [I] Name of the Monitor, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  pEnvironment is ignored in Windows for the local Computer.
 *
 */

static BOOL WINAPI fpDeleteMonitor(LPWSTR pName, LPWSTR pEnvironment, LPWSTR pMonitorName)
{
    HKEY    hroot = NULL;
    LONG    lres;

    TRACE("(%s, %s, %s)\n",debugstr_w(pName),debugstr_w(pEnvironment),
           debugstr_w(pMonitorName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /*  pEnvironment is ignored in Windows for the local Computer */
    if (!pMonitorName || !pMonitorName[0]) {
        TRACE("pMonitorName %s is invalid\n", debugstr_w(pMonitorName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(RegCreateKeyW(HKEY_LOCAL_MACHINE, monitorsW, &hroot) != ERROR_SUCCESS) {
        ERR("unable to create key %s\n", debugstr_w(monitorsW));
        return FALSE;
    }

    if(SHDeleteKeyW(hroot, pMonitorName) == ERROR_SUCCESS) {
        TRACE("%s deleted\n", debugstr_w(pMonitorName));
        RegCloseKey(hroot);
        return TRUE;
    }

    TRACE("%s does not exist\n", debugstr_w(pMonitorName));
    RegCloseKey(hroot);

    /* NT: ERROR_UNKNOWN_PRINT_MONITOR (3000), 9x: ERROR_INVALID_PARAMETER (87) */
    SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
    return FALSE;
}

/*****************************************************************************
 * fpDeletePort [exported through PRINTPROVIDOR]
 *
 * Delete a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box
 *  pPortName [I] Name of the Port, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI fpDeletePort(LPWSTR pName, HWND hWnd, LPWSTR pPortName)
{
    monitor_t * pm;
    monitor_t * pui;
    LONG        lres;
    DWORD       res;

    TRACE("(%s, %p, %s)\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* an empty Portname is Invalid */
    if (!pPortName[0]) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    pm = monitor_load_by_port(pPortName);
    if (pm && pm->monitor && pm->monitor->pfnDeletePort) {
        TRACE("use %s for %s (monitor %p: %s)\n", debugstr_w(pm->name),
                debugstr_w(pPortName), pm, debugstr_w(pm->dllname));
        res = pm->monitor->pfnDeletePort(pName, hWnd, pPortName);
        TRACE("got %d with %u\n", res, GetLastError());
    }
    else
    {
        pui = monitor_loadui(pm);
        if (pui && pui->monitorUI && pui->monitorUI->pfnDeletePortUI) {
            TRACE("use %s for %s (monitorui %p: %s)\n", debugstr_w(pui->name),
                        debugstr_w(pPortName), pui, debugstr_w(pui->dllname));
            res = pui->monitorUI->pfnDeletePortUI(pName, hWnd, pPortName);
            TRACE("got %d with %u\n", res, GetLastError());
        }
        else
        {
            FIXME("not implemented for %s (monitor %p: %s / monitorui %p: %s)\n",
                    debugstr_w(pPortName), pm, debugstr_w(pm ? pm->dllname : NULL),
                    pui, debugstr_w(pui ? pui->dllname : NULL));

            SetLastError(ERROR_NOT_SUPPORTED);
            res = FALSE;
        }
        monitor_unload(pui);
    }
    monitor_unload(pm);

    TRACE("returning %d with %u\n", res, GetLastError());
    return res;
}

/*****************************************************************************
 * fpEnumMonitors [exported through PRINTPROVIDOR]
 *
 * Enumerate available Port-Monitors
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level (1:Win9x+NT or 2:NT only)
 *  pMonitors  [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pMonitors
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pMonitors
 *  pcReturned [O] PTR to DWORD that receives the number of Monitors in pMonitors
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pMonitors, if cbBuf is too small
 *
 * NOTES
 *  Windows reads the Registry once and cache the Results.
 *
 */
static BOOL WINAPI fpEnumMonitors(LPWSTR pName, DWORD Level, LPBYTE pMonitors, DWORD cbBuf,
                                  LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   numentries = 0;
    DWORD   needed = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pMonitors,
          cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto em_cleanup;
    }

    if (!Level || (Level > 2)) {
        WARN("level (%d) is ignored in win9x\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    /* Scan all Monitor-Keys */
    numentries = 0;
    needed = get_local_monitors(Level, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do more error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto em_cleanup;
    }

    /* fill the Buffer with the Monitor-Keys */
    needed = get_local_monitors(Level, pMonitors, cbBuf, &numentries);
    res = TRUE;

em_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = numentries;

    TRACE("returning %d with %d (%d byte for %d entries)\n",
            res, GetLastError(), needed, numentries);

    return (res);
}

/******************************************************************************
 * fpEnumPorts [exported through PRINTPROVIDOR]
 *
 * Enumerate available Ports
 *
 * PARAMS
 *  pName      [I] Servername or NULL (local Computer)
 *  Level      [I] Structure-Level (1 or 2)
 *  pPorts     [O] PTR to Buffer that receives the Result
 *  cbBuf      [I] Size of Buffer at pPorts
 *  pcbNeeded  [O] PTR to DWORD that receives the size in Bytes used / required for pPorts
 *  pcReturned [O] PTR to DWORD that receives the number of Ports in pPorts
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPorts, if cbBuf is too small
 *
 */
static BOOL WINAPI fpEnumPorts(LPWSTR pName, DWORD Level, LPBYTE pPorts, DWORD cbBuf,
                               LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD   needed = 0;
    DWORD   numentries = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), Level, pPorts,
          cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto emP_cleanup;
    }

    if (!Level || (Level > 2)) {
        SetLastError(ERROR_INVALID_LEVEL);
        goto emP_cleanup;
    }

    if (!pcbNeeded || (!pPorts && (cbBuf > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emP_cleanup;
    }

    EnterCriticalSection(&monitor_handles_cs);
    monitor_loadall();

    /* Scan all local Ports */
    numentries = 0;
    needed = get_ports_from_all_monitors(Level, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do the error-checks */
    if (cbBuf < needed) {
        monitor_unloadall();
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto emP_cleanup_cs;
    }
    else if (!pPorts || !pcReturned) {
        monitor_unloadall();
        SetLastError(RPC_X_NULL_REF_POINTER);
        goto emP_cleanup_cs;
    }

    /* Fill the Buffer */
    needed = get_ports_from_all_monitors(Level, pPorts, cbBuf, &numentries);
    res = TRUE;
    monitor_unloadall();

emP_cleanup_cs:
    LeaveCriticalSection(&monitor_handles_cs);

emP_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    TRACE("returning %d with %d (%d byte for %d of %d entries)\n",
          (res), GetLastError(), needed, (res) ? numentries : 0, numentries);

    return (res);
}

/*****************************************************************************
 * fpEnumPrintProcessors [exported through PRINTPROVIDOR]
 *
 * Enumerate available Print Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (local Computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  Level        [I] Structure-Level (Only 1 is allowed)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pMonitors
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *  pcReturned   [O] PTR to DWORD that receives the number of Print Processors in pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 */
static BOOL WINAPI fpEnumPrintProcessors(LPWSTR pName, LPWSTR pEnvironment, DWORD Level,
                            LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    const printenv_t * env;
    LPWSTR  regpathW = NULL;
    DWORD   numentries = 0;
    DWORD   needed = 0;
    LONG    lres;
    BOOL    res = FALSE;

    TRACE("(%s, %s, %d, %p, %d, %p, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
                                Level, pPPInfo, cbBuf, pcbNeeded, pcReturned);

    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(ERROR_INVALID_NAME);
        goto epp_cleanup;
    }

    if (Level != 1) {
        SetLastError(ERROR_INVALID_LEVEL);
        goto epp_cleanup;
    }

    env = validate_envW(pEnvironment);
    if (!env)
        goto epp_cleanup;   /* ERROR_INVALID_ENVIRONMENT */

    regpathW = heap_alloc(sizeof(fmt_printprocessorsW) +
                            (lstrlenW(env->envname) * sizeof(WCHAR)));

    if (!regpathW)
        goto epp_cleanup;

    wsprintfW(regpathW, fmt_printprocessorsW, env->envname);

    /* Scan all Printprocessor-Keys */
    numentries = 0;
    needed = get_local_printprocessors(regpathW, NULL, 0, &numentries);

    /* we calculated the needed buffersize. now do more error-checks */
    if (cbBuf < needed) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto epp_cleanup;
    }

    /* fill the Buffer with the Printprocessor Infos */
    needed = get_local_printprocessors(regpathW, pPPInfo, cbBuf, &numentries);
    res = TRUE;

epp_cleanup:
    heap_free(regpathW);
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = numentries;

    TRACE("returning %d with %d (%d byte for %d entries)\n",
            res, GetLastError(), needed, numentries);

    return (res);
}

/******************************************************************************
 * fpGetPrintProcessorDirectory [exported through PRINTPROVIDOR]
 *
 * Return the PATH for the Print-Processors
 *
 * PARAMS
 *  pName        [I] Servername or NULL (this computer)
 *  pEnvironment [I] Printing-Environment or NULL (Default)
 *  level        [I] Structure-Level (must be 1)
 *  pPPInfo      [O] PTR to Buffer that receives the Result
 *  cbBuf        [I] Size of Buffer at pPPInfo
 *  pcbNeeded    [O] PTR to DWORD that receives the size in Bytes used / required for pPPInfo
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and in pcbNeeded the Bytes required for pPPInfo, if cbBuf is too small
 *
 *  Native Values returned in pPPInfo on Success for this computer:
 *| NT(Windows x64):    "%winsysdir%\\spool\\PRTPROCS\\x64"
 *| NT(Windows NT x86): "%winsysdir%\\spool\\PRTPROCS\\w32x86"
 *| NT(Windows 4.0):    "%winsysdir%\\spool\\PRTPROCS\\win40"
 *
 *  "%winsysdir%" is the Value from GetSystemDirectoryW()
 *
 */
static BOOL WINAPI fpGetPrintProcessorDirectory(LPWSTR pName, LPWSTR pEnvironment, DWORD level,
                                                LPBYTE pPPInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    const printenv_t * env;
    DWORD needed;
    LONG  lres;

    TRACE("(%s, %s, %d, %p, %d, %p)\n", debugstr_w(pName), debugstr_w(pEnvironment),
                                        level, pPPInfo, cbBuf, pcbNeeded);

    *pcbNeeded = 0;
    lres = copy_servername_from_name(pName, NULL);
    if (lres) {
        FIXME("server %s not supported\n", debugstr_w(pName));
        SetLastError(RPC_S_SERVER_UNAVAILABLE);
        return FALSE;
    }

    env = validate_envW(pEnvironment);
    if (!env)
        return FALSE;   /* ERROR_INVALID_ENVIRONMENT */

    /* GetSystemDirectoryW returns number of WCHAR including the '\0' */
    needed = GetSystemDirectoryW(NULL, 0);
    /* add the Size for the Subdirectories */
    needed += lstrlenW(spoolprtprocsW);
    needed += lstrlenW(env->subdir);
    needed *= sizeof(WCHAR);  /* return-value is size in Bytes */

    *pcbNeeded = needed;

    if (needed > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    GetSystemDirectoryW((LPWSTR) pPPInfo, cbBuf/sizeof(WCHAR));
    /* add the Subdirectories */
    lstrcatW((LPWSTR) pPPInfo, spoolprtprocsW);
    lstrcatW((LPWSTR) pPPInfo, env->subdir);
    TRACE("==> %s\n", debugstr_w((LPWSTR) pPPInfo));
    return TRUE;
}

/******************************************************************************
 * fpOpenPrinter [exported through PRINTPROVIDOR]
 *
 * Open a Printer / Printserver or a Printer-Object
 *
 * PARAMS
 *  lpPrinterName [I] Name of Printserver, Printer, or Printer-Object
 *  pPrinter      [O] The resulting Handle is stored here
 *  pDefaults     [I] PTR to Default Printer Settings or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  lpPrinterName is one of:
 *|  Printserver (NT only): "Servername" or NULL for the local Printserver
 *|  Printer: "PrinterName"
 *|  Printer-Object: "PrinterName,Job xxx"
 *|  XcvMonitor: "Servername,XcvMonitor MonitorName"
 *|  XcvPort: "Servername,XcvPort PortName"
 *
 *
 */
static BOOL WINAPI fpOpenPrinter(LPWSTR lpPrinterName, HANDLE *pPrinter,
                                 LPPRINTER_DEFAULTSW pDefaults)
{

    TRACE("(%s, %p, %p)\n", debugstr_w(lpPrinterName), pPrinter, pDefaults);

    *pPrinter = printer_alloc_handle(lpPrinterName, pDefaults);

    return (*pPrinter != 0);
}

/******************************************************************************
 * fpXcvData [exported through PRINTPROVIDOR]
 *
 * Execute commands in the Printmonitor DLL
 *
 * PARAMS
 *  hXcv            [i] Handle from fpOpenPrinter (with XcvMonitor or XcvPort)
 *  pszDataName     [i] Name of the command to execute
 *  pInputData      [i] Buffer for extra Input Data (needed only for some commands)
 *  cbInputData     [i] Size in Bytes of Buffer at pInputData
 *  pOutputData     [o] Buffer to receive additional Data (needed only for some commands)
 *  cbOutputData    [i] Size in Bytes of Buffer at pOutputData
 *  pcbOutputNeeded [o] PTR to receive the minimal Size in Bytes of the Buffer at pOutputData
 *  pdwStatus       [o] PTR to receive the win32 error code from the Printmonitor DLL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Returning "TRUE" does mean, that the Printmonitor DLL was called successful.
 *  The execution of the command can still fail (check pdwStatus for ERROR_SUCCESS).
 *
 *  Minimal List of commands, that a Printmonitor DLL should support:
 *
 *| "MonitorUI" : Return the Name of the Userinterface-DLL as WSTR in pOutputData
 *| "AddPort"   : Add a Port
 *| "DeletePort": Delete a Port
 *
 *  Many Printmonitors support additional commands. Examples for localspl.dll:
 *  "GetDefaultCommConfig", "SetDefaultCommConfig",
 *  "GetTransmissionRetryTimeout", "ConfigureLPTPortCommandOK"
 *
 */
static BOOL WINAPI fpXcvData(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
                    DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
                    PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    printer_t *printer = (printer_t * ) hXcv;

    TRACE("(%p, %s, %p, %d, %p, %d, %p, %p)\n", hXcv, debugstr_w(pszDataName),
          pInputData, cbInputData, pOutputData,
          cbOutputData, pcbOutputNeeded, pdwStatus);

    if (!printer || (!printer->hXcv)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!pcbOutputNeeded) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pszDataName || !pdwStatus || (!pOutputData && (cbOutputData > 0))) {
        SetLastError(RPC_X_NULL_REF_POINTER);
        return FALSE;
    }

    *pcbOutputNeeded = 0;

    *pdwStatus = printer->pm->monitor->pfnXcvDataPort(printer->hXcv, pszDataName,
            pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded);

    return TRUE;
}

/*****************************************************
 *  setup_provider [internal]
 */
void setup_provider(void)
{
    static const PRINTPROVIDOR backend = {
        fpOpenPrinter,
        NULL,   /* fpSetJob */
        NULL,   /* fpGetJob */
        NULL,   /* fpEnumJobs */
        NULL,   /* fpAddPrinter */
        NULL,   /* fpDeletePrinter */
        NULL,   /* fpSetPrinter */
        NULL,   /* fpGetPrinter */
        NULL,   /* fpEnumPrinters */
        NULL,   /* fpAddPrinterDriver */
        NULL,   /* fpEnumPrinterDrivers */
        NULL,   /* fpGetPrinterDriver */
        fpGetPrinterDriverDirectory,
        NULL,   /* fpDeletePrinterDriver */
        NULL,   /* fpAddPrintProcessor */
        fpEnumPrintProcessors,
        fpGetPrintProcessorDirectory,
        NULL,   /* fpDeletePrintProcessor */
        NULL,   /* fpEnumPrintProcessorDatatypes */
        NULL,   /* fpStartDocPrinter */
        NULL,   /* fpStartPagePrinter */
        NULL,   /* fpWritePrinter */
        NULL,   /* fpEndPagePrinter */
        NULL,   /* fpAbortPrinter */
        NULL,   /* fpReadPrinter */
        NULL,   /* fpEndDocPrinter */
        NULL,   /* fpAddJob */
        NULL,   /* fpScheduleJob */
        NULL,   /* fpGetPrinterData */
        NULL,   /* fpSetPrinterData */
        NULL,   /* fpWaitForPrinterChange */
        fpClosePrinter,
        NULL,   /* fpAddForm */
        NULL,   /* fpDeleteForm */
        NULL,   /* fpGetForm */
        NULL,   /* fpSetForm */
        NULL,   /* fpEnumForms */
        fpEnumMonitors,
        fpEnumPorts,
        fpAddPort,
        fpConfigurePort,
        fpDeletePort,
        NULL,   /* fpCreatePrinterIC */
        NULL,   /* fpPlayGdiScriptOnPrinterIC */
        NULL,   /* fpDeletePrinterIC */
        NULL,   /* fpAddPrinterConnection */
        NULL,   /* fpDeletePrinterConnection */
        NULL,   /* fpPrinterMessageBox */
        fpAddMonitor,
        fpDeleteMonitor,
        NULL,   /* fpResetPrinter */
        NULL,   /* fpGetPrinterDriverEx */
        NULL,   /* fpFindFirstPrinterChangeNotification */
        NULL,   /* fpFindClosePrinterChangeNotification */
        fpAddPortEx,
        NULL,   /* fpShutDown */
        NULL,   /* fpRefreshPrinterChangeNotification */
        NULL,   /* fpOpenPrinterEx */
        NULL,   /* fpAddPrinterEx */
        NULL,   /* fpSetPort */
        NULL,   /* fpEnumPrinterData */
        NULL,   /* fpDeletePrinterData */
        NULL,   /* fpClusterSplOpen */
        NULL,   /* fpClusterSplClose */
        NULL,   /* fpClusterSplIsAlive */
        NULL,   /* fpSetPrinterDataEx */
        NULL,   /* fpGetPrinterDataEx */
        NULL,   /* fpEnumPrinterDataEx */
        NULL,   /* fpEnumPrinterKey */
        NULL,   /* fpDeletePrinterDataEx */
        NULL,   /* fpDeletePrinterKey */
        NULL,   /* fpSeekPrinter */
        NULL,   /* fpDeletePrinterDriverEx */
        NULL,   /* fpAddPerMachineConnection */
        NULL,   /* fpDeletePerMachineConnection */
        NULL,   /* fpEnumPerMachineConnections */
        fpXcvData,
        fpAddPrinterDriverEx,
        NULL,   /* fpSplReadPrinter */
        NULL,   /* fpDriverUnloadComplete */
        NULL,   /* fpGetSpoolFileInfo */
        NULL,   /* fpCommitSpoolData */
        NULL,   /* fpCloseSpoolFileHandle */
        NULL,   /* fpFlushPrinter */
        NULL,   /* fpSendRecvBidiData */
        NULL    /* fpAddDriverCatalog */
    };
    pprovider = &backend;

}

/*****************************************************
 * InitializePrintProvidor     (localspl.@)
 *
 * Initialize the Printprovider
 *
 * PARAMS
 *  pPrintProvidor    [I] Buffer to fill with a struct PRINTPROVIDOR
 *  cbPrintProvidor   [I] Size of Buffer in Bytes
 *  pFullRegistryPath [I] Registry-Path for the Printprovidor
 *
 * RETURNS
 *  Success: TRUE and pPrintProvidor filled
 *  Failure: FALSE
 *
 * NOTES
 *  The RegistryPath should be:
 *  "System\CurrentControlSet\Control\Print\Providers\<providername>",
 *  but this Parameter is ignored in "localspl.dll".
 *
 */

BOOL WINAPI InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor,
                                    DWORD cbPrintProvidor, LPWSTR pFullRegistryPath)
{

    TRACE("(%p, %u, %s)\n", pPrintProvidor, cbPrintProvidor, debugstr_w(pFullRegistryPath));
    memcpy(pPrintProvidor, pprovider,
          (cbPrintProvidor < sizeof(PRINTPROVIDOR)) ? cbPrintProvidor : sizeof(PRINTPROVIDOR));

    return TRUE;
}
