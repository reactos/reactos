// ===========================================================================
// File: DBGLOG.CXX
//       CDLDebugLog: Class for Debug Logs
//       DebugLogElement: Node class for debug log messages
//

#include <cdlpch.h>
#include <stdio.h>
extern HINSTANCE g_hInst;

// Initialize static variables
CList<CDLDebugLog *, CDLDebugLog *> CDLDebugLog::s_dlogList;
TCHAR                               CDLDebugLog::s_szMessage[MAX_DEBUG_STRING_LENGTH];
CMutexSem                           CDLDebugLog::s_mxsDLogList;
BOOL                                CDLDebugLog::s_bMessage(FALSE);
CMutexSem                           CDLDebugLog::s_mxsMessage;

CDLDebugLog::CDLDebugLog()
:m_DebugLogList(),
 m_fAddedDebugLogHead(FALSE),
 m_iRefCount(0)
{
    m_szFileName[0] = '\0';
    m_szUrlName[0] = '\0';
    m_szMainClsid[0] = '\0';
    m_szMainType[0] = '\0';
    m_szMainExt[0] = '\0';
    m_szMainUrl[0] = '\0';
}

CDLDebugLog::~CDLDebugLog()
{
    Clear();
}

int CDLDebugLog::AddRef()
{
    return ++m_iRefCount;
}

int CDLDebugLog::Release()
{
    ASSERT(m_iRefCount > 0);
    m_iRefCount--;
    if(m_iRefCount <= 0)
    {
        delete this;
        return 0;
    }
    else
        return m_iRefCount;
}

CDLDebugLog * CDLDebugLog::MakeDebugLog()
{
    return new CDLDebugLog();
}

// Delete all new'd data and start a new log
void CDLDebugLog::Clear()
{
    DebugLogElement            *pMsg = NULL;
    LISTPOSITION                pos;
    int                         iNumMessages;
    int                         i;

    // delete new'd messages
    iNumMessages = m_DebugLogList.GetCount();
    pos = m_DebugLogList.GetHeadPosition();
    for (i = 0; i < iNumMessages; i++) {
        pMsg = m_DebugLogList.GetNext(pos);
        delete pMsg;
    }
    m_DebugLogList.RemoveAll();
    m_fAddedDebugLogHead = FALSE;

    // commit any lingering CacheEntry
    if(m_szUrlName[0])
    {
        FILETIME     ftExpireTime;
        FILETIME     ftTime;

        GetSystemTimeAsFileTime(&ftTime);
        ftExpireTime.dwLowDateTime = (DWORD)0;
        ftExpireTime.dwHighDateTime = (DWORD)0;
        CommitUrlCacheEntry(m_szUrlName, m_szFileName, ftExpireTime,
                            ftTime, NORMAL_CACHE_ENTRY,
                            NULL, 0, NULL, 0);
    }
    m_szUrlName[0] = 0;
    m_szFileName[0] = 0;
}

// Initializes the main clsid, type, ext, and codebase of the debuglog from the
// corresponding values in the CCodeDownload
// If any of the CCodeDownload's parameters are NULL, they are ignored
// (An assertion is thrown if they are all NULL)
// for Retail: returns false if they are all NULL
BOOL CDLDebugLog::Init(CCodeDownload * pcdl)
{
    int iRes = 0;

    if(pcdl->GetMainDistUnit())
    {       
        iRes = WideCharToMultiByte(CP_ACP, 0, pcdl->GetMainDistUnit(), -1, m_szMainClsid,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }
    if(pcdl->GetMainType())
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, pcdl->GetMainType(), -1, m_szMainType,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }                                    
    if(pcdl->GetMainExt())
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, pcdl->GetMainExt(), -1, m_szMainExt,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }
    if(pcdl->GetMainURL())
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, pcdl->GetMainURL(), -1, m_szMainUrl,
                                   INTERNET_MAX_URL_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }

    ASSERT(iRes != 0);

    return(iRes != 0);
}

// Initializes the main clsid, type, ext, and codebase of the debuglog
// If any of the parameters are NULL, they are ignored
// (An assertion is thrown if they are all NULL)
// for Retail: returns false if they are all NULL
BOOL CDLDebugLog::Init(LPCWSTR wszMainClsid, LPCWSTR wszMainType, LPCWSTR wszMainExt, LPCWSTR wszMainUrl)
{
    int iRes = 0;

    if(wszMainClsid)
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, wszMainClsid, -1, m_szMainClsid,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }
    if(wszMainType)
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, wszMainType, -1, m_szMainType,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);                           
        ASSERT(iRes != 0);
    }                                    
    if(wszMainExt)
    {
        WideCharToMultiByte(CP_ACP, 0, wszMainExt, -1, m_szMainExt,
                            MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }
    if(wszMainUrl)
    {
        WideCharToMultiByte(CP_ACP, 0, wszMainUrl, -1, m_szMainUrl,
                            INTERNET_MAX_URL_LENGTH, NULL, NULL);
        ASSERT(iRes != 0);
    }

    ASSERT(iRes != 0);
    return(iRes != 0);
}

// Make the cache file for the debug log using the current main clsid, type, ext, 
// and url data.  The previous values in m_szUrlName and m_szFileName are cleaned 
// up and written over.
void CDLDebugLog::MakeFile()
{
    TCHAR szExtension[] = TEXT("HTM");

    if(m_szFileName[0])
    {
        return; // Only make the file if we don't already have one
    }

    m_szUrlName[0]  = 0;
    m_szFileName[0] = 0;

    if(m_szMainClsid[0])
    {
        wsprintf(m_szUrlName,"?CodeDownloadErrorLog!name=%s", m_szMainClsid);
    }
    else if(m_szMainType[0])
    {
        wsprintf(m_szUrlName,"?CodeDownloadErrorLog!type=%s", m_szMainType);
    }
    else if(m_szMainExt[0])
    {
        wsprintf(m_szUrlName,"?CodeDownloadErrorLog!ext=%s", m_szMainExt);
    }
    else
    {
        wsprintf(m_szUrlName,"?CodeDownloadErrorLog!");
    }

    CreateUrlCacheEntry(m_szUrlName, 0, szExtension, m_szFileName, 0);
}

// ---------------------------------------------------------------------------
// %%Function: CDLDebugLog::DebugOut(int iOption, const char *pscFormat, ...)
//   Replacement for UrlMkDebugOut() and CCodeDownload::CodeDownloadDebugOut
//   calls to log code download debug/error messages
// ---------------------------------------------------------------------------

void CDLDebugLog::DebugOut(int iOption, BOOL fOperationFailed,
                                         UINT iResId, ...)
{
    static char             szDebugString[MAX_DEBUG_STRING_LENGTH*5];
    static char             szFormatString[MAX_DEBUG_FORMAT_STRING_LENGTH];
    va_list                 args;

    LoadString(g_hInst, iResId, szFormatString, MAX_DEBUG_FORMAT_STRING_LENGTH);
    va_start(args, iResId);
    vsprintf(szDebugString, szFormatString, args);
    va_end(args);

    DebugOutPreFormatted(iOption, fOperationFailed, szDebugString);
}


// Debug out taken a preformatted string instead of a resid format address and
// an arbitrary list of arguments.  szDebugString is the string which will be outputted
// as the debug statement
void CDLDebugLog::DebugOutPreFormatted(int iOption, BOOL fOperationFailed,
                                       LPTSTR szDebugString)
{
    static TCHAR           szUrlMkDebugOutString[MAX_DEBUG_STRING_LENGTH];
    DebugLogElement        *pdbglog = NULL;
    DebugLogElement        *pdbglogHead = NULL;


    pdbglog = new DebugLogElement(szDebugString);
    if(! pdbglog)
        return;
    wnsprintf(szUrlMkDebugOutString, MAX_DEBUG_STRING_LENGTH-1, "CODE DL:%s", szDebugString);
    UrlMkDebugOut((iOption,szUrlMkDebugOutString));

    if (pdbglog != NULL) {
        m_DebugLogList.AddTail(pdbglog);
        if (fOperationFailed && !m_fAddedDebugLogHead) {
            m_fAddedDebugLogHead = TRUE;
            pdbglogHead = new DebugLogElement("--- Detailed Error Log Follows ---\n");
            if(! pdbglogHead)
                return;
            m_DebugLogList.AddHead(pdbglogHead);
            pdbglogHead = new DebugLogElement(*pdbglog);
            if(! pdbglogHead)
                return;
            m_DebugLogList.AddHead(pdbglogHead);
        }
    }
}

#define DEBUG_LOG_HTML_START            TEXT("<html><pre>\n")
#define DEBUG_LOG_HTML_END              TEXT("\n</pre></html>")
#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

// ---------------------------------------------------------------------------
// %%Function: CDLDebugLog::DumpDebugLog()
//   Output the debug error log. This log is written as a cache entry.
// pszCacheFileName is an outparam for the name of the file in the cache,
// cbBufLen is the length of the buffer
// szErrorMsg is the error message causing the dump, and hrError the hr causing
//   the dump
// ---------------------------------------------------------------------------

void CDLDebugLog::DumpDebugLog(LPTSTR pszCacheFileName, int cbBufLen, LPTSTR szErrorMsg,
                                 HRESULT hrError)
{
    DebugLogElement     *pMsg = NULL;
    LPCSTR               pszStr = NULL;
    LISTPOSITION         pos = NULL;
    int                  iNumMessages = 0;
    int                  i = 0;
    HANDLE               hFile = INVALID_HANDLE_VALUE;
    FILETIME             ftTime;
    FILETIME             ftExpireTime;
    DWORD                dwBytes = 0;
    SYSTEMTIME           systime;
    static TCHAR         pszHeader[MAX_DEBUG_STRING_LENGTH];
    static const TCHAR  *ppszMonths[] = {TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), 
                                         TEXT("Apr"), TEXT("May"), TEXT("Jun"),
                                         TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), 
                                         TEXT("Oct"), TEXT("Nov"), TEXT("Dec")};
    
    // Get the filename and put it in the out LPTSTR
    if(!m_szFileName[0])
        MakeFile();
    if(pszCacheFileName)
        lstrcpyn(pszCacheFileName,m_szFileName, cbBufLen);

    iNumMessages = m_DebugLogList.GetCount();
    pos = m_DebugLogList.GetHeadPosition();
    if (pos) {
        pMsg = m_DebugLogList.GetAt(pos);
        if (pMsg != NULL) {
            hFile = CreateFile(m_szFileName, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                               NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                // Write the header
                WriteFile(hFile, DEBUG_LOG_HTML_START, strlen(DEBUG_LOG_HTML_START),
                          &dwBytes, NULL);
                GetLocalTime(&systime);
                wsprintf(pszHeader, "*** Code Download Log entry (%s%d %s %d @ %s%d:%s%d:%s%d) ***\n",
                         PAD_DIGITS_FOR_STRING(systime.wDay), systime.wDay,
                         ppszMonths[systime.wMonth - 1],
                         systime.wYear,
                         PAD_DIGITS_FOR_STRING(systime.wHour), systime.wHour,
                         PAD_DIGITS_FOR_STRING(systime.wMinute), systime.wMinute,
                         PAD_DIGITS_FOR_STRING(systime.wSecond), systime.wSecond);
                WriteFile(hFile, pszHeader, strlen(pszHeader), &dwBytes, NULL);                     

                // Write what error caused this dump
                wsprintf(pszHeader, "Code Download Error: (hr = %lx) %s\n",
                         hrError,
                         (szErrorMsg == NULL) ? ("(null)") : (szErrorMsg));
                WriteFile(hFile, pszHeader, strlen(pszHeader), &dwBytes, NULL);

                wsprintf(pszHeader, "Operation failed. Detailed Information:\n"
                                    "     CodeBase: %s\n"
                                    "     CLSID: %s\n"
                                    "     Extension: %s\n"
                                    "     Type: %s\n\n",
                                    m_szMainUrl,
                                    m_szMainClsid,
                                    m_szMainExt,
                                    m_szMainType);
                WriteFile(hFile, pszHeader, strlen(pszHeader), &dwBytes, NULL);

                // Write the Debug Log
                pos = m_DebugLogList.GetHeadPosition();
                iNumMessages = m_DebugLogList.GetCount();
                for (i = 0; i < iNumMessages; i++) {
                    pMsg = m_DebugLogList.GetNext(pos);
                    pszStr = pMsg->GetLogMessage();
                    WriteFile(hFile, pszStr, strlen(pszStr), &dwBytes, NULL);
                }

                // Close and clean
                WriteFile(hFile, DEBUG_LOG_HTML_END, strlen(DEBUG_LOG_HTML_END),
                          &dwBytes, NULL);
                CloseHandle(hFile);
                GetSystemTimeAsFileTime(&ftTime);
                ftExpireTime.dwLowDateTime = (DWORD)0;
                ftExpireTime.dwHighDateTime = (DWORD)0;
                CommitUrlCacheEntry(m_szUrlName, m_szFileName, ftExpireTime,
                                    ftTime, NORMAL_CACHE_ENTRY,
                                    NULL, 0, NULL, 0);
                m_fAddedDebugLogHead = FALSE;
                m_szUrlName[0] = NULL;
                m_szFileName[0] = NULL;
            }
        }
    }
}


// returns TRUE if there is already a message saved
// (If there is a message saved already, it will not be written over
// unless bOverwrite is true)
BOOL CDLDebugLog::SetSavedMessage(LPCTSTR szMessage, BOOL bOverwrite) 
{
    CLock lck(s_mxsMessage);
    BOOL bRet = FALSE;

    if(s_bMessage)
        bRet = TRUE;

    if((!s_bMessage) || bOverwrite)
    {
        lstrcpyn(s_szMessage, szMessage, MAX_DEBUG_STRING_LENGTH); 
        s_bMessage=TRUE;
    }

    return bRet;
}

LPCTSTR CDLDebugLog::GetSavedMessage() 
{
    CLock lck(s_mxsMessage);
    if(!s_bMessage) // Make sure to return an empty string if one has not been set
    {
        s_szMessage[0] = '\0';
    } 
    return s_szMessage;
}

// add a debug log to the global list
void CDLDebugLog::AddDebugLog(CDLDebugLog * dlog)
{
    CLock lck(s_mxsDLogList);
    if(dlog)
        s_dlogList.AddTail(dlog);
}

// Remove a given debug log from the global list
void CDLDebugLog::RemoveDebugLog(CDLDebugLog * dlog)
{
    CLock lck(s_mxsDLogList);
    if(dlog)
    {
        POSITION pos = s_dlogList.Find(dlog);
        if(pos != NULL)
            s_dlogList.RemoveAt(pos);
    }
}


// Gets the debug log with the given clsid, mime type, file extension or url code base.
// Naming priority is the same as at debug file urlname creation: clsid, then type, then ext, then url
// The names are checked in this order (so two logs with the same extension but different classids will never
// be confused, since clsids are checked before extensions)
// If an earlier name is NULL or doesn't produce a match, the next name is checked.
// If all names fails to produce a match, the return value is NULL
CDLDebugLog * CDLDebugLog::GetDebugLog(LPCWSTR wszMainClsid, LPCWSTR wszMainType, LPCWSTR wszMainExt, LPCWSTR wszMainUrl)
{
    TCHAR szComparer[MAX_DEBUG_STRING_LENGTH];
    POSITION pos = NULL;
    szComparer[0] = NULL;
    CDLDebugLog * dlogCur = NULL;
    const int iSwitchMax = 4;
    LPCWSTR ppwszMains[] = {wszMainClsid, wszMainType, wszMainExt, wszMainUrl};
    int iSwitch = iSwitchMax;
    int iRes = 0;

    // Find the first non-empty name
    for(iSwitch = 0; iSwitch < iSwitchMax; iSwitch++)
    {
        if((ppwszMains[iSwitch]) && (ppwszMains[iSwitch])[0])
            break;
    }

    if(iSwitch >= iSwitchMax)
        return NULL;


    // Grab mutex for accesing the list
    CLock lck(s_mxsDLogList);

    // Loop over the names; a higher array value means a lower priority name;
    // don't check a lower priority name unless all higher priority names have
    // failed
    while(iSwitch < iSwitchMax)
    {
        iRes = WideCharToMultiByte(CP_ACP, 0, ppwszMains[iSwitch], -1, szComparer,
                                   MAX_DEBUG_STRING_LENGTH, NULL, NULL);
        if(iRes == 0)
            return NULL;

        // Look through the entire list for a debuglog whose name (clsid, type, ext, or url)
        // matches the given one
        for(pos = s_dlogList.GetHeadPosition(); pos != NULL; )
        {
            dlogCur = s_dlogList.GetNext(pos);
            switch(iSwitch)
            {
            case 0:
                if(!lstrcmp(szComparer, dlogCur->GetMainClsid()))
                    return dlogCur;
                break;
            case 1:
                if(!lstrcmp(szComparer, dlogCur->GetMainType()))
                    return dlogCur;
                break;
            case 2:
                if(!lstrcmp(szComparer, dlogCur->GetMainExt()))
                    return dlogCur;
                break;
            case 3:
                if(!lstrcmp(szComparer, dlogCur->GetMainUrl()))
                    return dlogCur;
                break;
            default:
                break;
            }
        }
        iSwitch++;
    }

    // No match of any of the non-NULL passed in names was found
    return NULL;
}

DebugLogElement::DebugLogElement(const DebugLogElement &ref)
{
    SetLogMessage(ref.m_szMessage);
}

DebugLogElement::DebugLogElement(LPSTR szMessage)
: m_szMessage(NULL)
{
    SetLogMessage(szMessage);
}

DebugLogElement::~DebugLogElement()
{
    if (m_szMessage != NULL)
    {
        delete [] m_szMessage;
    }
}

HRESULT DebugLogElement::SetLogMessage(LPSTR szMessage)
{
    HRESULT             hr = S_OK;

    if (m_szMessage != NULL)
    {
        delete [] m_szMessage;
        m_szMessage = NULL;
    }
    m_szMessage = new char[strlen(szMessage) + 1];
    if (m_szMessage != NULL)
    {
        strcpy(m_szMessage, szMessage);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

