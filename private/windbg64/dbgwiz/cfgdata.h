
class CFGDATA {
public:
    DEBUG_MODE  m_DebugMode;

protected:
    BOOL        m_bUsingCfgFile;

public:
    BOOL        m_bRunningOnHost;
    BOOL        m_bDisplaySummaryInfo;
    BOOL        m_bLaunchNewProcess;
    PSTR        m_pszExecutableName;
    BOOL        m_bAdvancedInterface;
    char        m_szInstallSrcPath[_MAX_PATH];

    char        m_szWizInitialDir[_MAX_PATH];
    
    DWORD       m_dwServicePack;



    BOOL        m_bSerial;
    DWORD       m_dwBaudRate;
    PSTR        m_pszCommPortName;
    PSTR        m_pszCompName;
    PSTR        m_pszPipeName;

    PSTR        m_pszShortCutName;
    PSTR        m_pszDumpFileName;


    PSTR        m_pszFreeCheckedBuild;


    PSTR        m_pszDestinationPath;

    BOOL        m_bCopyOS_SymPath;
    PSTR        m_pszOS_SymPath;
    PSTR        m_pszOS_VolumeLabel_SymPath;
    DWORD       m_dwOS_SerialNum_SymPath;
    UINT        m_uOS_DriveType_SymPath;

    BOOL        m_bCopySP_SymPath;
    PSTR        m_pszSP_SymPath;
    PSTR        m_pszSP_VolumeLabel_SymPath;
    DWORD       m_dwSP_SerialNum_SymPath;
    UINT        m_uSP_DriveType_SymPath;

    BOOL        m_bCopyHotFix_SymPath;
    PSTR        m_pszHotFix_SymPath;
    PSTR        m_pszHotFix_VolumeLabel_SymPath;
    DWORD       m_dwHotFix_SerialNum_SymPath;
    UINT        m_uHotFix_DriveType_SymPath;

    // Multiple sympaths, stored as NULL terminated strings
    // With the end defined by \0\0
    BOOL            m_bCopyAdditional_SymPath;
    TList<PSTR>     m_list_pszAdditional_SymPath;
    TList<PSTR>     m_list_pszAdditional_VolumeLabel_SymPath;
    // Array of serial numbers and drive types
    TList<DWORD>    m_list_dwAdditional_SerialNum_SymPath;
    TList<UINT>     m_list_uAdditional_DriveType_SymPath;

    PSTR        m_pszHotFixes;

    PSTR        m_pszDeskTopShortcut_FullPath;

    OSVERSIONINFO   m_osi;
    SYSTEM_INFO     m_si;

    CFGDATA();


    void DoNotUseCfgFile();
    void UseCfgFile();

    BOOL AreWeUsingACfgFile() { return m_bUsingCfgFile; }
};


