/* register.c - Handles the Win 3.1 registration library.
 *
 * Created by Microsoft Corporation.
 */

#include "packager.h"

TCHAR gszAppName[] = "packager.exe";

/* RegInit() - Prepare the registration database for calls.
 */
VOID
RegInit(
    VOID
    )
{
    CHAR sz[CBMESSAGEMAX];
    CHAR szVerb[CBMESSAGEMAX];
    DWORD dwBytes = CBMESSAGEMAX;
    static TCHAR szAppClassID[] = TEXT("{0003000C-0000-0000-C000-000000000046}");



    // If the server is not in the registration database, add it
    if (RegQueryValue(HKEY_CLASSES_ROOT, gszAppClassName, sz, &dwBytes))
    {
        HKEY hkey;

        if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hkey))
            return;

        // Add the server name string
        LoadString(ghInst, IDS_FILTER, sz, CBMESSAGEMAX);
        RegSetValue(HKEY_CLASSES_ROOT, gszAppClassName, REG_SZ, sz,
            lstrlen(sz) + 1);

        // Add the server execute string  (don't forget the terminating zero
        // on "packgr32.exe")
        lstrcpy(sz, gszAppClassName);
        lstrcat(sz, "\\protocol\\StdFileEditing\\server");
        RegSetValue(HKEY_CLASSES_ROOT, sz, REG_SZ, gszAppName,
                (lstrlen(gszAppName) + 1));

        // Primary verb
        lstrcpy(sz, gszAppClassName);
        lstrcat(sz, "\\protocol\\StdFileEditing\\verb\\0");
        LoadString(ghInst, IDS_PRIMARY_VERB, szVerb, CBMESSAGEMAX);
        RegSetValue(HKEY_CLASSES_ROOT, sz, REG_SZ, szVerb, sizeof(szVerb));

        // Secondary verb
        lstrcpy(sz, gszAppClassName);
        lstrcat(sz, "\\protocol\\StdFileEditing\\verb\\1");
        LoadString(ghInst, IDS_SECONDARY_VERB, szVerb, CBMESSAGEMAX);
        RegSetValue(HKEY_CLASSES_ROOT, sz, REG_SZ, szVerb, sizeof(szVerb));

        // CLSID
        lstrcpy(sz, gszAppClassName);
        lstrcat(sz, "\\CLSID");
        RegSetValue(HKEY_CLASSES_ROOT, sz, REG_SZ, szAppClassID, sizeof(szAppClassID));

        RegCloseKey(hkey);
    }

    // If the CLSID is not in the registration database, add it
    dwBytes = CBMESSAGEMAX;

    lstrcpy(sz, "CLSID\\");
    lstrcat(sz, szAppClassID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, sz, szVerb, &dwBytes))
    {
        HKEY hkey;

        if (RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hkey))
            return;

        // Add the CLSID name string
        RegSetValue(hkey, szAppClassID, REG_SZ, gszAppClassName, lstrlen(gszAppClassName) + 1);

        // Add the OLE class
        lstrcpy(sz, szAppClassID);
        lstrcat(sz, "\\Ole1Class");
        RegSetValue(hkey, sz, REG_SZ, gszAppClassName, lstrlen(gszAppClassName) + 1);

        // Add the prog id
        lstrcpy(sz, szAppClassID);
        lstrcat(sz, "\\ProgID");
        RegSetValue(hkey, sz, REG_SZ, gszAppClassName, lstrlen(gszAppClassName) + 1);

        RegCloseKey(hkey);
    }
}



/* RegGetClassId() - Retrieves the string name of a class.
 *
 * Note:  Classes are guaranteed to be in ASCII, but should
 *        not be used directly as a rule because they might
 *        be meaningless if running non-English Windows.
 */
VOID
RegGetClassId(
    LPSTR lpstrName,
    LPSTR lpstrClass
    )
{
    DWORD dwSize = KEYNAMESIZE;
    CHAR szName[KEYNAMESIZE];

    if (HKEY_CLASSES_ROOT
        && !RegQueryValue(HKEY_CLASSES_ROOT, lpstrClass, szName, &dwSize))
        lstrcpy(lpstrName, szName);
    else
        lstrcpy(lpstrName, lpstrClass);
}



/* RegMakeFilterSpec() - Retrieves class-associated default extensions.
 *
 * This function returns a filter spec, to be used in the "Change Link"
 * standard dialog box, which contains all the default extensions which
 * are associated with the given class name.  Again, the class names are
 * guaranteed to be in ASCII.
 *
 * Returns:  The index nFilterIndex stating which filter item matches the
 *           extension, or 0 if none is found.
 */
INT
RegMakeFilterSpec(
    LPSTR lpstrClass,
    LPSTR lpstrExt,
    LPSTR lpstrFilterSpec
    )
{
    DWORD dwSize;
    CHAR szClass[KEYNAMESIZE];
    CHAR szName[KEYNAMESIZE];
    CHAR szString[KEYNAMESIZE];
    UINT i;
    INT idWhich = 0;
    INT idFilterIndex = 0;

    if (HKEY_CLASSES_ROOT)
    {
        for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szName, KEYNAMESIZE);)
        {
            if (*szName == '.'              /* Default Extension... */  /* ... so, get the class name */
                && (dwSize = KEYNAMESIZE)
                && !RegQueryValue(HKEY_CLASSES_ROOT, szName, szClass, &dwSize)
                /* ... and if the class name matches (null class is wildcard) */
                && (!lpstrClass || !lstrcmpi(lpstrClass, szClass))
                /* ... get the class name string */
                && (dwSize = KEYNAMESIZE)
                && !RegQueryValue(HKEY_CLASSES_ROOT, szClass, szString, &dwSize))
            {
                idWhich++;      /* Which item of the combo box is it? */

                // If the extension matches, save the filter index
                if (lpstrExt && !lstrcmpi(lpstrExt, szName))
                    idFilterIndex = idWhich;

                //
                // Copy over "<Class Name String> (*<Default Extension>)"
                // e.g. "Server Picture (*.PIC)"
                //
                lstrcpy(lpstrFilterSpec, szString);
                lstrcat(lpstrFilterSpec, " (*");
                lstrcat(lpstrFilterSpec, szName);
                lstrcat(lpstrFilterSpec, ")");
                lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;

                // Copy over "*<Default Extension>" (e.g. "*.PIC")
                lstrcpy(lpstrFilterSpec, "*");
                lstrcat(lpstrFilterSpec, szName);
                lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
            }
        }
    }
    else
    {
        // Default Handling
        LoadString(ghInst, IDS_ALLFILTER, lpstrFilterSpec, CBMESSAGEMAX);
        lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
        lstrcpy(lpstrFilterSpec, "*.*");
        lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
    }

    // Add another NULL at the end of the spec
    *lpstrFilterSpec = 0;

    return idFilterIndex;
}



VOID
RegGetExeName(
    LPSTR lpstrExe,
    LPSTR lpstrClass,
    DWORD dwBytes
    )
{
    CHAR szServer[KEYNAMESIZE];

    // Add the server execute string
    lstrcpy(szServer, lpstrClass);
    lstrcat(szServer, "\\protocol\\StdFileEditing\\server");
    RegQueryValue(HKEY_CLASSES_ROOT, szServer, lpstrExe, &dwBytes);
}
