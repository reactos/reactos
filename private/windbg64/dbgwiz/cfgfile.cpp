#include "precomp.hxx"
#pragma hdrstop


const char * const TARGET_CFG_INFO::m_pszKERNEL = "KERNEL";
const char * const TARGET_CFG_INFO::m_pszUSER = "USER";
const char * const TARGET_CFG_INFO::m_pszYES = "Y";
const char * const TARGET_CFG_INFO::m_pszNO = "N";

#define FPRINTF(pf, psz) fprintf(pf, "%s=%s\n", #psz, psz)

BOOL
ScanString(
    FILE * pfile, 
    PSTR pszName,
    PSTR pszBuffer, 
    UINT uBufSize, 
    PSTR & pszDest
    )
{
    PSTR pszEqual = NULL;
    fgets(pszBuffer, uBufSize, pfile);
    Assert(*pszBuffer);

    // remove any whitespace from the end (such as linefeeds, etc...)
    for (pszEqual = pszBuffer + strlen(pszBuffer) -1; 
        pszBuffer <= pszEqual && isspace(*pszEqual); pszEqual--) {

        *pszEqual = NULL;
    }

    if (_strnicmp(pszBuffer, pszName, strlen(pszName))) {
        return FALSE;
    }
    pszEqual = pszBuffer + strlen(pszName);
    if ('=' == *pszEqual) {
        pszEqual++;
    } else {
        return FALSE;
    }
    if (pszDest) {
        free(pszDest);
    }
    pszDest = NT4SafeStrDup(pszEqual);

    return TRUE;
}

#define FSCANF(pf, szBuffer, pszDest) ScanString(pf, #pszDest, szBuffer, sizeof(szBuffer), pszDest)

void
FreeString(
    PSTR & p
    )
{
    if (p) {
        free(p);
        p = NULL;
    }
}



TARGET_CFG_INFO::
TARGET_CFG_INFO()
{
    m_pszDebugMode = NULL;
    m_pszUseSerial = NULL;
    m_pszBaudRate = NULL;
    m_pszComputerName = NULL;
    m_pszPipeName = NULL;

    //m_pszVersion = NULL;
    //m_pszArchitecture = NULL;
    //m_pszType = NULL;
    //m_pszDebugType = NULL;
    m_pszServicePack = NULL;
    m_pszHotFixes = NULL;
}

void
TARGET_CFG_INFO::
FreeAllData()
{
    FreeString(m_pszDebugMode);
    FreeString(m_pszUseSerial);
    FreeString(m_pszBaudRate);
    FreeString(m_pszComputerName);
    FreeString(m_pszPipeName);

    //FreeString(m_pszVersion);
    //FreeString(m_pszArchitecture);
    //FreeString(m_pszType);
    //FreeString(m_pszDebugType);
    FreeString(m_pszServicePack);
    FreeString(m_pszHotFixes);
}

TARGET_CFG_INFO::
~TARGET_CFG_INFO()
{
    FreeAllData();
}

BOOL
TARGET_CFG_INFO::
Write(
    PSTR pszFileName
    )
{
    Assert(pszFileName);

    FreeAllData();

    if (enumKERNELMODE == g_CfgData.m_DebugMode) {
        m_pszDebugMode = NT4SafeStrDup(m_pszKERNEL);
    } else { 
        m_pszDebugMode = NT4SafeStrDup(m_pszUSER);
    }

    if (g_CfgData.m_bSerial) {
        m_pszUseSerial = NT4SafeStrDup(m_pszYES);
    } else {
        m_pszUseSerial = NT4SafeStrDup(m_pszNO);
    }

    // Convert baud rate to a string
    {
        char sz[20] = {0};
        m_pszBaudRate = NT4SafeStrDup(BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));
    }
    m_pszComputerName = NT4SafeStrDup(g_CfgData.m_pszCompName);
    m_pszPipeName = NT4SafeStrDup(g_CfgData.m_pszPipeName);

    {
        if (*g_CfgData.m_osi.szCSDVersion) {            
            m_pszServicePack = NT4SafeStrDup(g_CfgData.m_osi.szCSDVersion + strlen("Service Pack "));
        } else {
            m_pszServicePack = NT4SafeStrDup("");
        }
    }

    if (g_CfgData.m_pszHotFixes) {
        m_pszHotFixes = NT4SafeStrDup(g_CfgData.m_pszHotFixes);
    } else {
        m_pszHotFixes = NT4SafeStrDup("");
    }


    FILE * pfile = NULL;

    do {
        pfile = fopen(pszFileName, "wt");
        if (!pfile && IDRETRY != WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_RETRYCANCEL, 
            NULL, IDS_ERROR_COULD_NOT_CREATE_CFG_FILE, pszFileName)) {

            return FALSE;
        }
    } while (!pfile);

    FPRINTF(pfile, m_pszDebugMode);
    FPRINTF(pfile, m_pszUseSerial);
    FPRINTF(pfile, m_pszBaudRate);
    FPRINTF(pfile, m_pszComputerName);
    FPRINTF(pfile, m_pszPipeName);
    FPRINTF(pfile, m_pszServicePack);
    FPRINTF(pfile, m_pszHotFixes);

    fclose(pfile);

    return TRUE;
}


BOOL 
TARGET_CFG_INFO::
Read(
    PSTR pszFileName
    )
{
    Assert(pszFileName);

    char szLineOfTextBuffer[_MAX_PATH * 4];
    FILE * pfile = NULL;

    do {
        pfile = fopen(pszFileName, "rt");
        if (!pfile && IDRETRY != WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_RETRYCANCEL, 
            NULL, IDS_ERROR_COULD_NOT_OPEN_CFG_FILE, pszFileName)) {

            return FALSE;
        }
    } while (!pfile);

    BOOL bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszDebugMode);
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszUseSerial);
    }
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszBaudRate);
    }    
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszComputerName);
    }    
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszPipeName);
    }    
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszServicePack);
    }    
    if (bSuccess) {
        bSuccess = FSCANF(pfile, szLineOfTextBuffer, m_pszHotFixes);
    }    

    fclose(pfile);
    if (!bSuccess) {
        return FALSE;
    }

    if (!strcmp(m_pszDebugMode, m_pszKERNEL)) {
        g_CfgData.m_DebugMode = enumKERNELMODE;
    } else if (!strcmp(m_pszDebugMode, m_pszUSER)) {
        g_CfgData.m_DebugMode = enumREMOTE_USERMODE;
    } else {
        return FALSE;
    }
    if (!strcmp(m_pszUseSerial, m_pszYES)) {
        g_CfgData.m_bSerial = TRUE;
    } else if (!strcmp(m_pszUseSerial, m_pszNO)) {
        g_CfgData.m_bSerial = FALSE;
    } else {
        return FALSE;
    }

    if (m_pszBaudRate && *m_pszBaudRate) {
        g_CfgData.m_dwBaudRate = BaudRateTextToBitFlag(m_pszBaudRate);
    }
    if (m_pszComputerName && *m_pszComputerName) {
        FreeString(g_CfgData.m_pszCompName);
        g_CfgData.m_pszCompName = NT4SafeStrDup(m_pszComputerName);
    }
    if (m_pszPipeName && *m_pszPipeName) {
        FreeString(g_CfgData.m_pszPipeName);
        g_CfgData.m_pszPipeName = NT4SafeStrDup(m_pszPipeName);
    }
    if (m_pszServicePack && *m_pszServicePack) {        
        g_CfgData.m_dwServicePack = atol(m_pszServicePack);
    }
    if (m_pszHotFixes && *m_pszHotFixes) {
        FreeString(g_CfgData.m_pszHotFixes);
        g_CfgData.m_pszHotFixes = NT4SafeStrDup(m_pszHotFixes);
    }
    
    return TRUE;
}
