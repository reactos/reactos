/*
 *  SETLOCAL.C - setlocal and endlocal internal batch commands.
 *
 *  History:
 *
 *    1 Feb 2008 (Christoph von Wittich)
 *        started.
*/

#include "precomp.h"

#include <direct.h> // For _getdrive().

typedef struct _SETLOCAL
{
    struct _SETLOCAL *Prev;
    LPTSTR Environment;
    INT CurDrive;
    BOOL EnableExtensions;
    BOOL DelayedExpansion;
} SETLOCAL, *PSETLOCAL;

/* Create a copy of the current environment */
LPTSTR
DuplicateEnvironment(VOID)
{
    LPTSTR Environ = GetEnvironmentStrings();
    LPTSTR End, EnvironCopy;

    if (!Environ) return NULL;

    for (End = Environ; *End; End += _tcslen(End) + 1) ;
    EnvironCopy = cmd_alloc((End + 1 - Environ) * sizeof(TCHAR));

    if (EnvironCopy)
        memcpy(EnvironCopy, Environ, (End + 1 - Environ) * sizeof(TCHAR));

    FreeEnvironmentStrings(Environ);
    return EnvironCopy;
}

INT cmd_setlocal(LPTSTR param)
{
    PSETLOCAL Saved;
    LPTSTR* arg;
    INT argc, i;

    if (!_tcscmp(param, _T("/?")))
    {
        // FIXME
        ConOutPuts(_T("SETLOCAL help not implemented yet!\n"));
        return 0;
    }

    /* SETLOCAL only works inside a batch context */
    if (!bc)
        return 0;

    Saved = cmd_alloc(sizeof(SETLOCAL));
    if (!Saved)
    {
        WARN("Cannot allocate memory for Saved!\n");
        error_out_of_memory();
        return 1;
    }

    Saved->Environment = DuplicateEnvironment();
    if (!Saved->Environment)
    {
        error_out_of_memory();
        cmd_free(Saved);
        return 1;
    }
    /*
     * Save the current drive; the duplicated environment
     * contains the corresponding current directory.
     */
    Saved->CurDrive = _getdrive();

    Saved->EnableExtensions = bEnableExtensions;
    Saved->DelayedExpansion = bDelayedExpansion;

    Saved->Prev = bc->setlocal;
    bc->setlocal = Saved;

    nErrorLevel = 0;

    arg = splitspace(param, &argc);
    for (i = 0; i < argc; i++)
    {
        if (!_tcsicmp(arg[i], _T("ENABLEEXTENSIONS")))
            bEnableExtensions = TRUE;
        else if (!_tcsicmp(arg[i], _T("DISABLEEXTENSIONS")))
            bEnableExtensions = FALSE;
        else if (!_tcsicmp(arg[i], _T("ENABLEDELAYEDEXPANSION")))
            bDelayedExpansion = TRUE;
        else if (!_tcsicmp(arg[i], _T("DISABLEDELAYEDEXPANSION")))
            bDelayedExpansion = FALSE;
        else
        {
            error_invalid_parameter_format(arg[i]);
            break;
        }
    }
    freep(arg);

    return nErrorLevel;
}

INT cmd_endlocal(LPTSTR param)
{
    LPTSTR Environ, Name, Value;
    PSETLOCAL Saved;
    TCHAR drvEnvVar[] = _T("=?:");
    TCHAR szCurrent[MAX_PATH];

    if (!_tcscmp(param, _T("/?")))
    {
        // FIXME
        ConOutPuts(_T("ENDLOCAL help not implemented yet!\n"));
        return 0;
    }

    /* Pop a SETLOCAL struct off of this batch context's stack */
    if (!bc || !(Saved = bc->setlocal))
        return 0;
    bc->setlocal = Saved->Prev;

    bEnableExtensions = Saved->EnableExtensions;
    bDelayedExpansion = Saved->DelayedExpansion;

    /* First, clear out the environment. Since making any changes to the
     * environment invalidates pointers obtained from GetEnvironmentStrings(),
     * we must make a copy of it and get the variable names from that. */
    Environ = DuplicateEnvironment();
    if (Environ)
    {
        for (Name = Environ; *Name; Name += _tcslen(Name) + 1)
        {
            if (!(Value = _tcschr(Name + 1, _T('='))))
                continue;
            *Value++ = _T('\0');
            SetEnvironmentVariable(Name, NULL);
            Name = Value;
        }
        cmd_free(Environ);
    }

    /* Now, restore variables from the copy saved by cmd_setlocal() */
    for (Name = Saved->Environment; *Name; Name += _tcslen(Name) + 1)
    {
        if (!(Value = _tcschr(Name + 1, _T('='))))
            continue;
        *Value++ = _T('\0');
        SetEnvironmentVariable(Name, Value);
        Name = Value;
    }

    /* Restore the current drive and its current directory from the environment */
    drvEnvVar[1] = _T('A') + Saved->CurDrive - 1;
    if (!GetEnvironmentVariable(drvEnvVar, szCurrent, ARRAYSIZE(szCurrent)))
    {
        _stprintf(szCurrent, _T("%C:\\"), _T('A') + Saved->CurDrive - 1);
    }
    _tchdir(szCurrent); // SetRootPath(NULL, szCurrent);

    cmd_free(Saved->Environment);
    cmd_free(Saved);
    return 0;
}
