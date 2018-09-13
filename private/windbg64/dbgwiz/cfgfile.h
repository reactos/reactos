
class TARGET_CFG_INFO {
public:
    static const char * const m_pszKERNEL;
    static const char * const m_pszUSER;
    static const char * const m_pszYES;
    static const char * const m_pszNO;

    PSTR m_pszDebugMode;
    PSTR m_pszUseSerial;
    PSTR m_pszBaudRate;
    PSTR m_pszComputerName;
    PSTR m_pszPipeName;
    
    //PSTR m_pszVersion;
    //PSTR m_pszArchitecture;
    //PSTR m_pszType;
    //PSTR m_pszDebugType;
    PSTR m_pszServicePack;
    PSTR m_pszHotFixes;

    TARGET_CFG_INFO();
    ~TARGET_CFG_INFO();

    void FreeAllData();

    BOOL Write(PSTR);
    BOOL Read(PSTR);
};

