/*----------------------------------------------------------------------------
/ Title;
/   cabstate.c => cabinet state i/o
/
/ Purpose:
/   Provides a clean API to fill out the cabinet state from the registry, if the
/   relevent keys cannot be found then we set the relevant defaults.  This is
/   called by the explorer.
/
/ History:
/   23apr96 daviddv New API which passes the structure size in
/   18mar96 daviddv Bug fix; Added colour state to FALSE when state structure not read
/    7feb96 daviddv Tweeked cabinet state writing
/   30jan96 daviddv Created
/
/----------------------------------------------------------------------------*/
#include "shellprv.h"
#include "regstr.h"
#include "cstrings.h"
#pragma hdrstop


TCHAR const c_szCabinetState[] = REGSTR_PATH_EXPLORER TEXT( "\\CabinetState");
TCHAR const c_szSettings[]     = TEXT("Settings");
TCHAR const c_szFullPath[]     = TEXT("FullPath");

/*----------------------------------------------------------------------------
/   Read in the CABINETSTATE structure from the registry and attempt to validate it.
/
/ Notes:
/   -
/
/ In:
/   lpCabinetState => pointer to CABINETSTATE structure to be filled.
/   cLength = size of structure to be filled
/
/ Out:
/   [lpState] filled in with data
/   fReadFromRegistry == indicates if the structure was actually read from the registry
/                        or if we are giviing the client a default one.
/----------------------------------------------------------------------------*/
STDAPI_(BOOL) ReadCabinetState( CABINETSTATE *lpState, int cLength )
{
    DWORD cbData = SIZEOF(CABINETSTATE);
    BOOL fReadFromRegistry = FALSE;
    CABINETSTATE state;
    SHELLSTATE ss;
    DWORD dwType;
    HKEY hKey;

    ASSERT( lpState );

    SHGetSetSettings(&ss, SSF_WIN95CLASSIC, FALSE);

    if ( lpState && cLength )
    {
        BOOL fReadFullPath = FALSE;
        //
        // Setup the default state of the structure and read in the current state
        // from the registry (over our freshly initialised structure).
        //

        state.cLength                   = SIZEOF(CABINETSTATE);
        state.nVersion                  = CABINETSTATE_VERSION;

        state.fSimpleDefault            = TRUE;
        state.fFullPathTitle            = FALSE;
        state.fSaveLocalView            = TRUE;
        state.fNotShell                 = FALSE;
        state.fNewWindowMode            = BOOLIFY(ss.fWin95Classic);
        state.fShowCompColor            = FALSE;
        state.fDontPrettyNames          = FALSE;
        state.fAdminsCreateCommonGroups = TRUE;
        state.fUnusedFlags              = 0;
        state.fMenuEnumFilter           = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

        if ( !GetSystemMetrics( SM_CLEANBOOT ) &&
             ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, c_szCabinetState, 0L, KEY_READ, &hKey ) )
        {
            DWORD dwFullPath=0;
            DWORD cbFullPath=SIZEOF(dwFullPath);
            
            fReadFromRegistry = ( ERROR_SUCCESS == SHQueryValueEx( hKey,
                                                                    c_szSettings,
                                                                    NULL,
                                                                    &dwType,
                                                                    (PVOID) &state, &cbData ) );
            
            if (ERROR_SUCCESS == SHQueryValueEx(hKey, c_szFullPath, NULL, NULL, (LPVOID)&dwFullPath, &cbFullPath))
            {
                fReadFullPath = TRUE;
                state.fFullPathTitle = (BOOL)dwFullPath ? TRUE : FALSE;
            }
            
            RegCloseKey( hKey );
        }

        //
        // Fix the structure if it is an early version and write back into the registry
        // to avoid having to do it again.
        //

        if ( fReadFromRegistry && state.nVersion < CABINETSTATE_VERSION )
        {
            // NT4 and IE4x had the same value for state.nVersion (1), and we have to stomp some of the flags
            // depending on whether we are pre-IE4x or not. To differentiate, we see if c_szFullPath was present.
            // This reg key was introduced only in IE40.
            if ( (state.nVersion < 1) || ((state.nVersion == 1) && !fReadFullPath) )
            {
                state.fNewWindowMode            = BOOLIFY(ss.fWin95Classic);
                state.fAdminsCreateCommonGroups = TRUE;              // Moved post BETA 2 SUR!
                state.fUnusedFlags              = 0;
                state.fMenuEnumFilter           = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
            }

            state.cLength = SIZEOF(CABINETSTATE);
            state.nVersion = CABINETSTATE_VERSION;

            WriteCabinetState( &state );
        }

        //
        // Copy only the requested data back to the caller.
        //

        state.cLength = (int) min( SIZEOF(CABINETSTATE), cLength );
        memcpy( lpState, &state, cLength );
    }

    return fReadFromRegistry;
}

// old export
STDAPI_(BOOL) OldReadCabinetState( LPCABINETSTATE lpState, int cLength )
{
   return ReadCabinetState(lpState, sizeof(CABINETSTATE));
}



/*----------------------------------------------------------------------------
/   Writes a CABINETSTATE structure back into the registry.
/
/ Notes:
/   Attempt to do the right thing when given a small structure to write
/   back so that we don't bugger the users settings.
/
/ In:
/   lpState -> structure to be written
/
/ Out:
/    fSuccess = TRUE / FALSE indicating if state has been seralised
/----------------------------------------------------------------------------*/
STDAPI_(BOOL) WriteCabinetState(CABINETSTATE *lpState)
{
    BOOL fSuccess = FALSE;
    if (lpState)
    {
        CABINETSTATE state;
        HKEY hKey;

        // Check to see if the structure is the right size, if its too small
        // then we must merge it with a real one before writing back!
        if (lpState->cLength < SIZEOF(CABINETSTATE))
        {
            ReadCabinetState(&state, SIZEOF(state));

            memcpy(&state, lpState, lpState->cLength);
            state.cLength = SIZEOF(CABINETSTATE);
            lpState = &state;
        }

        if ( ERROR_SUCCESS == RegCreateKey( HKEY_CURRENT_USER, c_szCabinetState, &hKey ) )
        {
            DWORD dwFullPath = lpState->fFullPathTitle ? TRUE : FALSE;

            fSuccess = ERROR_SUCCESS == RegSetValueEx( hKey,
                                                       c_szSettings,
                                                       0,
                                                       REG_BINARY,
                                                       (LPVOID)lpState, (DWORD)SIZEOF(CABINETSTATE) );

            // NOTE: We have to continue writing this key. One of the uses for it is to decide
            // whether we are pre-IE4 or not. See ReadCabinetState()...
            RegSetValueEx(hKey, c_szFullPath, 0, REG_DWORD, (LPVOID)&dwFullPath, sizeof(dwFullPath));
            RegCloseKey( hKey );
        }
    }

    if (fSuccess) 
    {
        // Notify anybody who is listening
        HANDLE hChange = SHGlobalCounterCreate(&GUID_FolderSettingsChange);
        SHGlobalCounterIncrement(hChange);
        SHGlobalCounterDestroy(hChange);
    }

    return fSuccess;
}
