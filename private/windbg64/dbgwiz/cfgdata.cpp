#include "precomp.hxx"
#pragma hdrstop


CFGDATA::
CFGDATA()
{
    m_DebugMode = enumLOCAL_USERMODE;
    
    m_bUsingCfgFile = FALSE;
    
    m_bRunningOnHost = TRUE;
    m_bDisplaySummaryInfo = FALSE;
    m_bLaunchNewProcess = TRUE;
    m_pszExecutableName = NULL;
    m_bAdvancedInterface = TRUE;
    
    m_dwServicePack = 0;
    
    m_pszDumpFileName = _tcsdup(_T("%systemroot%\\memory.dmp"));
    m_pszShortCutName = NULL;
    
    //
    // Sym stuff
    m_pszDestinationPath = NULL;    
    
    m_bCopyOS_SymPath = FALSE;
    m_pszOS_SymPath = NULL;
    m_pszOS_VolumeLabel_SymPath = NULL;
    m_uOS_DriveType_SymPath = DRIVE_UNKNOWN;

    m_bCopySP_SymPath = FALSE;
    m_pszSP_SymPath = NULL;
    m_pszSP_VolumeLabel_SymPath = NULL;
    m_uSP_DriveType_SymPath = DRIVE_UNKNOWN;

    m_bCopyHotFix_SymPath = FALSE;
    m_pszHotFix_SymPath = NULL;
    m_pszHotFix_VolumeLabel_SymPath = NULL;
    m_uHotFix_DriveType_SymPath = DRIVE_UNKNOWN;

    m_bCopyAdditional_SymPath = FALSE;


   
    m_pszDeskTopShortcut_FullPath = NULL;

    m_pszHotFixes = NULL;
    
    ZeroMemory(m_szInstallSrcPath, sizeof(m_szInstallSrcPath));
    
    //
    // Where does DbgWiz.exe live?
    {
        ZeroMemory(m_szWizInitialDir, sizeof(m_szWizInitialDir));
        
        char szPath[_MAX_PATH] = {0};
        PSTR pszFilePart = NULL;
        
        Assert(SearchPath(NULL, "dbgwiz", ".exe", 
            sizeof(szPath), szPath, &pszFilePart));
        
        Assert(pszFilePart);
        *pszFilePart = NULL;
        
        strcpy(m_szWizInitialDir, szPath); 
    }
    
    m_bSerial = TRUE;
    m_dwBaudRate = 0;
    m_pszCommPortName = NULL;
    m_pszCompName = NULL;
    m_pszPipeName = NULL;
    
    ZeroMemory(&m_si, sizeof(m_si));
    GetSystemInfo(&m_si);
    
    {
        HKEY hkey = NULL;
        PSTR pszKeyPath = NULL;
        PSTR pszRegValue = NULL;
        
        ZeroMemory(&m_osi, sizeof(m_osi));
        m_osi.dwOSVersionInfoSize = sizeof(m_osi);
        Assert(GetVersionEx(&m_osi));
        
        // Detect from where the OS was installed
        if (VER_PLATFORM_WIN32_NT == m_osi.dwPlatformId) {
            // NT
            pszKeyPath = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
            pszRegValue = "SourcePath";
        } else if (VER_PLATFORM_WIN32_WINDOWS == m_osi.dwPlatformId) {
            // Figure out what version of win9x we're using
            if (0 == m_osi.dwMinorVersion) {
                pszKeyPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
                pszRegValue = "AppinstallPath";
            } else if (10 == m_osi.dwMinorVersion) {
                pszKeyPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup";
                pszRegValue = "SourcePath";
            } else {
                WKSP_MsgBox(NULL, IDS_ERROR_UNSUPPORTED_OS);
            }
        } else {
            // Something else
            WKSP_MsgBox(NULL, IDS_ERROR_UNSUPPORTED_OS);
        }
        
        
        Assert(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            pszKeyPath, 0, KEY_READ, &hkey));
        if (hkey) {
            DWORD dwSize = sizeof(m_szInstallSrcPath);
            DWORD dwType = REG_EXPAND_SZ;
            Assert(ERROR_SUCCESS == RegQueryValueEx(hkey, pszRegValue, NULL, &dwType,
                (PUCHAR) m_szInstallSrcPath, &dwSize));
            
            Assert(ERROR_SUCCESS == RegCloseKey(hkey));
        }
        
        //
        // Get the hot fixes. Concat hotfixes into a list that looks like:
        //  "Qxxxx, Qxxxx, Qxxxx, Qxxxx"
        //        
        if (VER_PLATFORM_WIN32_NT == m_osi.dwPlatformId) {
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix", 0, KEY_READ, &hkey)) {

                DWORD dwMaxKeyNameLen = 0;
                DWORD dwNumSubKeys = 0;
                PSTR pszBigBuffer = NULL;
                PSTR pszNameBuffer = NULL;
            
                WKSP_RegKeyValueInfo(hkey, &dwNumSubKeys, NULL, &dwMaxKeyNameLen, NULL);

                // Make it bigger. Bigger is safer.
                dwMaxKeyNameLen += 2;
            
                pszNameBuffer = (PSTR) calloc(dwMaxKeyNameLen, 1);
                pszBigBuffer = (PSTR) calloc(dwMaxKeyNameLen * dwNumSubKeys 
                    // Factor in the space required for each ", " between the hotfixes
                    + (dwNumSubKeys -1) * 2, 1);
            
                if (pszNameBuffer && pszBigBuffer) {
                    // So far so good, get each entry
                    for (DWORD dw=0; dw<dwNumSubKeys; dw++) {
                        DWORD dwSize = dwMaxKeyNameLen;
                        WKSP_RegGetKeyName(hkey, dw, pszNameBuffer, &dwSize);
                    
                        // concat the list
                        strcat(pszBigBuffer, pszNameBuffer);
                        if (dw < dwNumSubKeys-1) {
                            strcat(pszBigBuffer, ", ");
                        }
                    }
                
                    m_pszHotFixes = NT4SafeStrDup(pszBigBuffer);
                }
            
                if (pszNameBuffer) {
                   free(pszNameBuffer);
                }
                if (pszBigBuffer) {
                   free(pszBigBuffer);
                }

                Assert(ERROR_SUCCESS == RegCloseKey(hkey));
            }
        }
    }
    
    //
    // debug: free/checked: 
    if (VER_PLATFORM_WIN32_NT == m_osi.dwPlatformId) {
        HKEY hkey = NULL;
        char sz[_MAX_PATH] = {0};
        DWORD dwType;
        DWORD dwSize = sizeof(sz);
        
        Assert(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hkey));
        
        Assert(ERROR_SUCCESS == RegQueryValueEx(hkey, "CurrentType", NULL, &dwType,
            (PUCHAR) sz, &dwSize));
        
        Assert(ERROR_SUCCESS == RegCloseKey(hkey));
        
        PSTR psz = strtok(sz, " ");
        Assert(psz);
        psz = strtok(NULL, " ");
        
        if (!_stricmp(psz, "Free")) {
            m_pszFreeCheckedBuild = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_FREE);
        } else if (!_stricmp(psz, "Checked")) {
            m_pszFreeCheckedBuild = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_CHECKED);
        } else {
            m_pszFreeCheckedBuild = WKSP_DynaLoadString(g_hInst, IDS_UNKNOWN);
        }
    }
    
    
    DoNotUseCfgFile();    
    GuessAtSymSrc();
}

void 
CFGDATA::
DoNotUseCfgFile()
{
    m_bUsingCfgFile = FALSE;

    if (*m_osi.szCSDVersion) {
        m_dwServicePack = atol(m_osi.szCSDVersion + strlen("Service Pack "));
    } else {
        m_dwServicePack = 0;
    }
}

void 
CFGDATA::
UseCfgFile()
{
    m_bUsingCfgFile = TRUE;
}

