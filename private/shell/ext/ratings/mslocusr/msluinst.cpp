#include "mslocusr.h"
#include "msluglob.h"
#include "resource.h"

/* InstallLogonDialog - check if there is a primary logon provider already on
 * the system, and if not, install MSLOCUSR as a net provider and make it the
 * primary logon.  Returns TRUE if the NP was installed.
 *
 * This chunk of hideous registry code exists because NETDI.DLL (the win95
 * network setup engine) (a) has no programmatic interface, it just assumes
 * it's being driven by NETCPL.CPL;  (b) is 16-bit code, so even if it had
 * a programmatic interface, we'd have to thunk;  and (c) if everything's
 * not consistent in his database of what network components are installed
 * and which are bound to which, then the next time the user brings up the
 * network CPL, any components which don't make sense just get silently
 * deinstalled.
 *
 * The set of registry keys and values which need to be added, changed, or
 * updated was gleaned from a registry diff done after using the real network
 * CPL to install this logon provider from an INF.  A similar registry diff
 * and similar code could be created to programmatically install a transport.
 * Don't ask me to do it for you, though...
 *
 * Note that in case of registry errors, we just bail out.  It would require
 * a huge amount of extra code to keep track of everything that had been done
 * up to that point and undo it.  The worst that happens if we do strange
 * things to the net component database is that NETDI will deinstall our
 * component the next time the user brings up the network control panel.  It
 * shouldn't actually cause any crashes or anything like that.
 */

BOOL InstallLogonDialog(void)
{
    HKEY hkey;          /* used for various work */
    LONG err;
    TCHAR szBuf[MAX_PATH];
    DWORD dwType;
    DWORD cbData;
    DWORD dwTemp;
    DWORD dwDisp;

    NLS_STR nlsNPName(MAX_RES_STR_LEN);
    if (nlsNPName.LoadString(IDS_NP_NAME) != ERROR_SUCCESS)
        return FALSE;

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\Logon", 0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE, &hkey);
    if (err != ERROR_SUCCESS)
        return FALSE;           /* big problems if we can't get this guy */

    /* Get the PrimaryProvider value, which is the name of the net provider
     * that's handling the main logon dialog.  If it's there and not blank,
     * then presumably the user's on a LAN or something, so we don't want
     * to replace the logon dialog.
     */
    cbData = sizeof(szBuf);
    err = RegQueryValueEx(hkey, "PrimaryProvider", NULL, &dwType,
                          (LPBYTE)szBuf, &cbData);
    if (err == ERROR_SUCCESS && szBuf[0] != '\0') {
        RegCloseKey(hkey);
        return FALSE;
    }

    /* Make us the primary logon provider, as far as MPR and the logon code
     * are concerned.
     */
    err = RegSetValueEx(hkey, "PrimaryProvider", 0, REG_SZ,
                        (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);
    RegCloseKey(hkey);
    if (err != ERROR_SUCCESS)
        return FALSE;

    /* Under HKLM\SW\MS\W\CV\Network\Real Mode Net, preferredredir=null string,
     * since we will now be the primary network in all respects.  NETDI needs
     * this to avoid getting confused.
     */
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Network\\Real Mode Net",
                       0, KEY_QUERY_VALUE, &hkey);
    if (err == ERROR_SUCCESS) {
        err = RegSetValueEx(hkey, "preferredredir", 0, REG_SZ, (LPBYTE)TEXT(""), sizeof(TCHAR));
        RegCloseKey(hkey);
    }

    /* Add new keys under HKLM\System\CurrentControlSet which will actually
     * make MPR load our DLL as a net provider.
     */
    HKEY hkeyFamilyClient;
    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\NPSTUB\\NetworkProvider", 
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_SET_VALUE,
                         NULL, &hkeyFamilyClient, &dwDisp);
    if (err == ERROR_SUCCESS) {
        RegSetValueEx(hkeyFamilyClient, "Name", 0, REG_SZ,
                      (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);
        RegSetValueEx(hkeyFamilyClient, "ProviderPath", 0, REG_SZ,
                      (LPBYTE)TEXT("ienpstub.dll"), 11 * sizeof(TCHAR));
        RegSetValueEx(hkeyFamilyClient, "RealDLL", 0, REG_SZ,
                      (LPBYTE)TEXT("mslocusr.dll"), 13 * sizeof(TCHAR));
        RegSetValueEx(hkeyFamilyClient, "Description", 0, REG_SZ,
                      (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);

        dwTemp = WNNC_NET_MSNET;
        RegSetValueEx(hkeyFamilyClient, "NetID", 0, REG_DWORD,
                      (LPBYTE)&dwTemp, sizeof(dwTemp));
        dwTemp = 0x40000000;
        RegSetValueEx(hkeyFamilyClient, "CallOrder", 0, REG_DWORD,
                      (LPBYTE)&dwTemp, sizeof(dwTemp));

        RegCloseKey(hkeyFamilyClient);
    }

    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\NetworkProvider\\Order", 
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_SET_VALUE,
                         NULL, &hkeyFamilyClient, &dwDisp);
    if (err == ERROR_SUCCESS) {
        cbData = sizeof(szBuf);
        if (RegQueryValueEx(hkeyFamilyClient, "NPSTUB", NULL, &dwType, 
                            (LPBYTE)szBuf, &cbData) == ERROR_SUCCESS) {
            /* Our provider is already installed!  Better not do anything
             * more than just making it default, which we've already done.
             */
            RegCloseKey(hkeyFamilyClient);
            return FALSE;
        }
        RegSetValueEx(hkeyFamilyClient, "NPSTUB", 0, REG_SZ,
                      (LPBYTE)TEXT(""), sizeof(TCHAR));
        RegCloseKey(hkeyFamilyClient);
    }

    /* We've now installed our NP in the registry, and to see it appear we
     * need a reboot.  So from here on, if we bail out, we return TRUE.
     */

    /* First big chunk of network component database management.  Under
     * HKLM\System\CurrentControlSet\Services\Class\NetClient there is a
     * four-digit numeric subkey (e.g., "0000") for each network client.
     * One of them will be the default network client as far as NETDI's
     * database is concerned;  this is indicated by the existence of the
     * "Ndi\Default" subkey under the number key.  If we find one of those
     * guys, we save away the DeviceID value from the Ndi subkey so we can
     * tweak some configuration flags later in another part of the database.
     *
     * While enumerating the keys, we keep track of the highest number we've
     * seen so far.  When we're done, we add 1 to that and use that as the
     * subkey name for our client.  The number is kept separate from the
     * RegEnumKey index because the numbers are not necessarily packed (nor
     * will RegEnumKey necessarily return them in numeric order!).
     */

    HKEY hkeyNetClient;
    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\NetClient",
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
                         NULL, &hkeyNetClient, &dwDisp);
    if (err != ERROR_SUCCESS)
        return TRUE;

    UINT nFamilyNum;
    TCHAR szFamilyNumString[5]; /* four digits plus null */
    TCHAR szDefaultDeviceID[MAX_PATH] = "";

    if (dwDisp == REG_OPENED_EXISTING_KEY) {
        NLS_STR nlsSubKey(20);       /* enough for four digits, plus some just in case */
        DWORD iSubKey = 0;
        UINT maxSubKey = 0;

        for (;;) {
            err = RegEnumKey(hkeyNetClient, iSubKey, nlsSubKey.Party(), nlsSubKey.QueryAllocSize());
            nlsSubKey.DonePartying();
            if (err != ERROR_SUCCESS)
                break;

            NLS_STR nls2(nlsSubKey.strlen() + 12);
            if (nls2.QueryError() == ERROR_SUCCESS) {
                nls2 = nlsSubKey;
                nls2.strcat("\\Ndi\\Default");
                cbData = sizeof(szBuf);
                err = RegQueryValue(hkeyNetClient, nls2.QueryPch(), szBuf, (PLONG)&cbData);
                if (err == ERROR_SUCCESS) {
                    if (!lstrcmpi(szBuf, "True")) {
                        HKEY hkeyNdi;

                        NLS_STR nls3(nlsSubKey.strlen() + 5);
                        if (nls3.QueryError() == ERROR_SUCCESS) {
                            nls3 = nlsSubKey;
                            nls3.strcat("\\Ndi");

                            err = RegOpenKeyEx(hkeyNetClient, nls3.QueryPch(), 0, KEY_QUERY_VALUE, &hkeyNdi);
                            if (err == ERROR_SUCCESS) {
                                cbData = sizeof(szDefaultDeviceID);
                                RegQueryValueEx(hkeyNdi, "DeviceID", NULL, &dwType,
                                                (LPBYTE)szDefaultDeviceID, &cbData);
                                RegCloseKey(hkeyNdi);
                            }
                        }
                    }
                    RegDeleteKey(hkeyNetClient, nls2.QueryPch());
                }
            }

            UINT nSubKey = nlsSubKey.atoi();
            if (nSubKey > maxSubKey)
                maxSubKey = nSubKey;

            iSubKey++;
        }
        nFamilyNum = maxSubKey+1;
    }
    else
        nFamilyNum = 0;

    wsprintf(szFamilyNumString, "%04d", nFamilyNum);
    err = RegCreateKeyEx(hkeyNetClient, szFamilyNumString, 
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                         NULL, &hkeyFamilyClient, &dwDisp);
    if (err == ERROR_SUCCESS) {
        RegSetValueEx(hkeyFamilyClient, "DriverDesc", 0, REG_SZ,
                      (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);
        RegSetValueEx(hkeyFamilyClient, "InfPath", 0, REG_SZ,
                      (LPBYTE)TEXT("NETFAM.INF"), 11 * sizeof(TCHAR));
        RegSetValueEx(hkeyFamilyClient, "DriverDate", 0, REG_SZ,
                      (LPBYTE)TEXT(" 5-21-1997"), 11 * sizeof(TCHAR));
        err = RegCreateKeyEx(hkeyFamilyClient, "Ndi", 
                             0, "", REG_OPTION_NON_VOLATILE,
                             KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                             NULL, &hkey, &dwDisp);
        if (err == ERROR_SUCCESS) {
            RegSetValueEx(hkey, "DeviceID", 0, REG_SZ,
                          (LPBYTE)TEXT("FAMILY"), 7 * sizeof(TCHAR));
            RegSetValueEx(hkey, "NetworkProvider", 0, REG_SZ,
                          (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);
            RegSetValueEx(hkey, "InstallInf", 0, REG_SZ,
                          (LPBYTE)TEXT(""), sizeof(TCHAR));
            RegSetValueEx(hkey, "InfSection", 0, REG_SZ,
                          (LPBYTE)TEXT("FAMILY.ndi"), 11 * sizeof(TCHAR));

            {
                NLS_STR nlsHelpText(MAX_RES_STR_LEN);
                if (nlsHelpText.LoadString(IDS_NETFAM_HELP_TEXT) == ERROR_SUCCESS) {
                    RegSetValueEx(hkey, "HelpText", 0, REG_SZ,
                                  (LPBYTE)nlsHelpText.QueryPch(), nlsHelpText.strlen() + 1);
                }
            }

            HKEY hkeyInterfaces;
            err = RegCreateKeyEx(hkey, "Interfaces", 
                                 0, "", REG_OPTION_NON_VOLATILE,
                                 KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                                 NULL, &hkeyInterfaces, &dwDisp);
            if (err == ERROR_SUCCESS) {
                RegSetValueEx(hkeyInterfaces, "DefLower", 0, REG_SZ,
                              (LPBYTE)TEXT("netbios,ipxDHost"), 13 * sizeof(TCHAR));
                RegSetValueEx(hkeyInterfaces, "LowerRange", 0, REG_SZ,
                              (LPBYTE)TEXT("netbios,ipxDHost"), 13 * sizeof(TCHAR));
                RegSetValueEx(hkeyInterfaces, "Lower", 0, REG_SZ,
                              (LPBYTE)TEXT("netbios,ipxDHost"), 13 * sizeof(TCHAR));
                RegSetValueEx(hkeyInterfaces, "Upper", 0, REG_SZ,
                              (LPBYTE)TEXT(""), sizeof(TCHAR));
                RegCloseKey(hkeyInterfaces);
            }
            if (err == ERROR_SUCCESS)
                err = RegSetValue(hkey, "Install", REG_SZ, "FAMILY.Install", 14);
            if (err == ERROR_SUCCESS)
                err = RegSetValue(hkey, "Remove", REG_SZ, "FAMILY.Remove", 13);
            if (err == ERROR_SUCCESS)
                err = RegSetValue(hkey, "Default", REG_SZ, "True", 5);

            RegCloseKey(hkey);
        }
        RegCloseKey(hkeyFamilyClient);
    }
    RegCloseKey(hkeyNetClient);

    if (err != ERROR_SUCCESS)
        return TRUE;

    /* Now the other half of the database, under HKLM\Enum\Network.  This has
     * a subkey (named by DeviceID, as seen above) for each network component.
     * Under each such subkey, there's a numbered subkey for each instance.
     * We have three tasks here:  First of all, for each instance of the client
     * that used to be the default, we have to mask out bit 0x00000010 from
     * the ConfigFlags value, to make it no longer the default client.  Then
     * we have to create a new branch for our own client, which mainly points
     * back to the other section of the database which we just finished with.
     * Finally, we must find MSTCP and add a binding between it and our client,
     * because NETDI assumes that a client that's not bound to any transports
     * must be screwed up, so it deletes it.
     */

    HKEY hkeyEnum;
    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Enum\\Network", 
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
                         NULL, &hkeyEnum, &dwDisp);
    if (err != ERROR_SUCCESS)
        return TRUE;

    /* Un-default the default client. */
    if (szDefaultDeviceID[0] != '\0') {
        HKEY hkeyDefaultDevice;
        err = RegOpenKeyEx(hkeyEnum, szDefaultDeviceID, 0,
                           KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
                           &hkeyDefaultDevice);
        if (err == ERROR_SUCCESS) {
            NLS_STR nlsSubKey(20);       /* enough for four digits, plus some just in case */
            DWORD iSubKey = 0;

            for (;;) {
                err = RegEnumKey(hkeyDefaultDevice, iSubKey, nlsSubKey.Party(), nlsSubKey.QueryAllocSize());
                nlsSubKey.DonePartying();
                if (err != ERROR_SUCCESS)
                    break;

                HKEY hkeyInstance;
                err = RegOpenKeyEx(hkeyDefaultDevice, nlsSubKey.QueryPch(),
                                   0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyInstance);
                if (err == ERROR_SUCCESS) {
                    DWORD dwConfigFlags;
                    cbData = sizeof(dwConfigFlags);
                    err = RegQueryValueEx(hkeyInstance, "ConfigFlags", NULL,
                                          &dwType, (LPBYTE)&dwConfigFlags,
                                          &cbData);
                    if (err == ERROR_SUCCESS &&
                        (dwType == REG_DWORD || dwType == REG_BINARY) &&
                        (dwConfigFlags & 0x10)) {
                        dwConfigFlags &= ~0x10;
                        RegSetValueEx(hkeyInstance, "ConfigFlags", 0, dwType,
                                      (LPBYTE)&dwConfigFlags, cbData);
                    }
                    RegCloseKey(hkeyInstance);
                }

                iSubKey++;
            }
            RegCloseKey(hkeyDefaultDevice);
        }
    }

    /* Now create a new branch for our client. */

    err = RegCreateKeyEx(hkeyEnum, "FAMILY\\0000", 
                         0, "", REG_OPTION_NON_VOLATILE,
                         KEY_SET_VALUE,
                         NULL, &hkeyFamilyClient, &dwDisp);
    if (err == ERROR_SUCCESS) {
        RegSetValueEx(hkeyFamilyClient, "Class", 0, REG_SZ,
                      (LPBYTE)TEXT("NetClient"), 10 * sizeof(TCHAR));
        lstrcpy(szBuf, "NetClient\\");
        lstrcat(szBuf, szFamilyNumString);
        RegSetValueEx(hkeyFamilyClient, "Driver", 0, REG_SZ, (LPBYTE)szBuf, lstrlen(szBuf)+1);
        RegSetValueEx(hkeyFamilyClient, "MasterCopy", 0, REG_SZ,
                      (LPBYTE)TEXT("Enum\\Network\\FAMILY\\0000"), 25 * sizeof(TCHAR));
        RegSetValueEx(hkeyFamilyClient, "DeviceDesc", 0, REG_SZ,
                      (LPBYTE)nlsNPName.QueryPch(), nlsNPName.strlen()+1);
        RegSetValueEx(hkeyFamilyClient, "CompatibleIDs", 0, REG_SZ,
                      (LPBYTE)TEXT("FAMILY"), 7 * sizeof(TCHAR));
        RegSetValueEx(hkeyFamilyClient, "Mfg", 0, REG_SZ,
                      (LPBYTE)TEXT("Microsoft"), 10 * sizeof(TCHAR));
        dwTemp = 0x00000010;
        RegSetValueEx(hkeyFamilyClient, "ConfigFlags", 0, REG_BINARY,
                      (LPBYTE)&dwTemp, sizeof(dwTemp));

        /* A "Bindings" subkey needs to exist here, with no values in it
         * (since our "client" isn't bound to any higher level components
         * like servers).
         */
        err = RegCreateKeyEx(hkeyFamilyClient, "Bindings", 
                             0, "", REG_OPTION_NON_VOLATILE,
                             KEY_SET_VALUE,
                             NULL, &hkey, &dwDisp);
        if (err == ERROR_SUCCESS)
            RegCloseKey(hkey);

        RegCloseKey(hkeyFamilyClient);
    }

    /* Get MSTCP's enum key, get the first instance, and from it we can find
     * the "master" instance.  We can then add a binding to ourselves there.
     * Can't just assume "0000" as the first one, unfortunately.
     */
    err = RegOpenKeyEx(hkeyEnum, "MSTCP", 0,
                       KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
                       &hkey);
    if (err == ERROR_SUCCESS) {
        NLS_STR nlsSubKey(20);       /* enough for four digits, plus some just in case */
        DWORD iSubKey = 0;

        for (;;) {
            err = RegEnumKey(hkey, iSubKey, nlsSubKey.Party(), nlsSubKey.QueryAllocSize());
            nlsSubKey.DonePartying();
            if (err != ERROR_SUCCESS)
                break;

            HKEY hkeyInstance;
            err = RegOpenKeyEx(hkey, nlsSubKey.QueryPch(),
                               0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyInstance);
            if (err == ERROR_SUCCESS) {
                cbData = sizeof(szBuf);
                err = RegQueryValueEx(hkeyInstance, "MasterCopy", NULL,
                                      &dwType, (LPBYTE)szBuf,
                                      &cbData);
                RegCloseKey(hkeyInstance);

                /* The MasterCopy value is actually a path to a registry key
                 * from HKEY_LOCAL_MACHINE.  We want to deal with its Bindings
                 * subkey.
                 */
                if (err == ERROR_SUCCESS) {
                    HKEY hkeyBindings;
                    lstrcat(szBuf, "\\Bindings");
                    err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf,
                                         0, "", REG_OPTION_NON_VOLATILE,
                                         KEY_SET_VALUE,
                                         NULL, &hkeyBindings, &dwDisp);
                    if (err == ERROR_SUCCESS) {
                        RegSetValueEx(hkeyBindings, "FAMILY\\0000", 0, REG_SZ,
                                      (LPBYTE)TEXT(""), sizeof(TCHAR));
                        RegCloseKey(hkeyBindings);
                    }
                    break;      /* abandon enum loop */
                }

                iSubKey++;
            }
        }
        RegCloseKey(hkey);
    }

    RegCloseKey(hkeyEnum);

    return TRUE;
}


/*
Purpose: Recursively delete the key, including all child values
         and keys.  Mimics what RegDeleteKey does in Win95.

         Snarfed from shlwapi so we don't end up loading him at
         boot time.

Returns: 
Cond:    --
*/
DWORD
DeleteKeyRecursively(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkSubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyEx(hkey, pszSubKey, 0, KEY_ALL_ACCESS, &hkSubKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        CHAR    szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        CHAR    szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKey(hkSubKey,
                                szClass,
                                &cbClass,
                                NULL,
                                &dwIndex, // The # of subkeys -- all we need
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKey(hkSubKey, --dwIndex, szSubKeyName, cchSubKeyName))
            {
                DeleteKeyRecursively(hkSubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkSubKey);

        dwRet = RegDeleteKey(hkey, pszSubKey);
    }

    return dwRet;
}


void DeinstallLogonDialog(void)
{
    RegDeleteKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\NPSTUB\\NetworkProvider");
    RegDeleteKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\NPSTUB");

    HKEY hkey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\NetworkProvider\\Order",
                     0, KEY_WRITE, &hkey) == ERROR_SUCCESS) {
        RegDeleteValue(hkey, "NPSTUB");
        RegCloseKey(hkey);
    }

    char szBuf[MAX_PATH];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Network\\Logon",
                     0, KEY_WRITE, &hkey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(szBuf);
        DWORD dwType;
        LONG err = RegQueryValueEx(hkey, "PrimaryProvider", NULL, &dwType,
                                   (LPBYTE)szBuf, &cbData);
        if (err == ERROR_SUCCESS && szBuf[0] != '\0') {
            NLS_STR nlsNPName(MAX_RES_STR_LEN);
            if (nlsNPName.LoadString(IDS_NP_NAME) == ERROR_SUCCESS) {
                if (!::strcmpf(nlsNPName.QueryPch(), szBuf)) {
                    RegDeleteValue(hkey, "PrimaryProvider");
                }
            }
        }

        RegCloseKey(hkey);
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Enum\\Network\\FAMILY", 0,
                     KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WRITE,
                     &hkey) == ERROR_SUCCESS) {
        UINT i=0;

        /* For each instance of us under the Enum branch, fetch the
         * corresponding key name under the other half of the database
         * and delete it.
         */
        for (;;) {
            DWORD err = RegEnumKey(hkey, i, szBuf, sizeof(szBuf));
            if (err != ERROR_SUCCESS)
                break;

            HKEY hkeyInstance;
            err = RegOpenKeyEx(hkey, szBuf,
                               0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyInstance);
            if (err == ERROR_SUCCESS) {
                strcpyf(szBuf, "System\\CurrentControlSet\\Services\\Class\\");

                DWORD dwType;
                DWORD cbData = sizeof(szBuf) - 40;  /* - length of above string */
                if (RegQueryValueEx(hkeyInstance, "Driver", NULL, &dwType,
                                    (LPBYTE)szBuf + 40, &cbData) == ERROR_SUCCESS) {
                    /* szBuf now equals the other branch we need to kill */
                    DeleteKeyRecursively(HKEY_LOCAL_MACHINE, szBuf);
                }
                RegCloseKey(hkeyInstance);
            }
            i++;
        }

        RegCloseKey(hkey);

        DeleteKeyRecursively(HKEY_LOCAL_MACHINE, "Enum\\Network\\FAMILY");
    }

    /* Now clean up bindings to our client, otherwise PNP will try to install
     * us as a new (unknown) device.  This involves enumerating components
     * under HKLM\Enum\Network;  for each one, enumerate the instances;  for
     * each instance's Bindings key, enumerate the values, and delete all
     * values that begin with FAMILY\.
     */

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Enum\\Network", 0,
                     KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WRITE,
                     &hkey) == ERROR_SUCCESS) {
        UINT iComponent = 0;

        for (;;) {
            DWORD err = RegEnumKey(hkey, iComponent, szBuf, sizeof(szBuf));
            if (err != ERROR_SUCCESS)
                break;

            HKEY hkeyComponent;
            err = RegOpenKeyEx(hkey, szBuf,
                               0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyComponent);
            if (err == ERROR_SUCCESS) {

                /* Opened a component's key.  Enumerate its instances, opening
                 * each one's Bindings subkey.
                 */
                TCHAR szInstance[16];       /* actually only needs to be "nnnn\Bindings" plus null char */

                UINT iInstance = 0;

                for (;;) {
                    err = RegEnumKey(hkeyComponent, iInstance, szInstance, sizeof(szInstance));
                    if (err != ERROR_SUCCESS)
                        break;

                    if (strlenf(szInstance)*sizeof(TCHAR) <= sizeof(szInstance) - sizeof("\\Bindings"))
                        strcatf(szInstance, "\\Bindings");
                    HKEY hkeyInstance;
                    err = RegOpenKeyEx(hkeyComponent, szInstance,
                               0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyInstance);
                    if (err == ERROR_SUCCESS) {

                        /* Opened a Bindings subkey.  For each value under this
                         * key, the value name indicates the instance being
                         * bound to, and the value data is empty.  So we can
                         * enum values, ignoring the value data and type and
                         * just concentrating on the name.
                         */

                        TCHAR szValueName[64];      /* usually "COMPONENT\nnnn" */
                        UINT iValue = 0;
                        for (;;) {
                            DWORD cchValue = ARRAYSIZE(szValueName);
                            err = RegEnumValue(hkeyInstance, iValue, szValueName, 
                                               &cchValue, NULL, NULL, NULL, NULL);
                            if (err != ERROR_SUCCESS)
                                break;

                            /* If this is a binding to our client, delete the
                             * binding and reset (deleting values while enuming
                             * can be unpredictable).
                             */
                            if (!strnicmpf(szValueName, "FAMILY\\", 7)) {
                                RegDeleteValue(hkeyInstance, szValueName);
                                iValue = 0;
                                continue;
                            }

                            iValue++;
                        }

                        RegCloseKey(hkeyInstance);
                    }

                    iInstance++;
                }


                RegCloseKey(hkeyComponent);
            }
            iComponent++;
        }

        RegCloseKey(hkey);
    }
}
