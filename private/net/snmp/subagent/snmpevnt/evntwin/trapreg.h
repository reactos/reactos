#ifndef _trapreg_h
#define _trapreg_h

#include "regkey.h"
#include "utils.h"

#define MAX_TRAP_SIZE       4096
#define THRESHOLD_COUNT     500
#define THRESHOLD_TIME      300


// Values for the SNMP_EVENTS\Parameters\Threshold flag
#define THROTTLE_RESET 0
#define THROTTLE_TRIPPED 1

// Values for the SNMP_EVENTS\Parameters\ThresholdEnabled flag
#define THROTTLE_DISABLED 0
#define THROTTLE_ENABLED 1

//***************************************************************************
// REGISTRY KEYS
//
// The following strings are registry keys. They should not be internationalized,
// so they are not in the string table.
//
//****************************************************************************
#define SZ_REGKEY_MICROSOFT _T("SOFTWARE\\Microsoft")
#define SZ_REGKEY_SOURCE_EVENTLOG _T("SYSTEM\\CurrentControlSet\\Services\\EventLog")
#define SZ_REGKEY_SNMP_EVENTS  _T("SOFTWARE\\Microsoft\\SNMP_EVENTS")

// These are subkeys under \SOFTWARE\Microsoft\SNMP_EVENTS
#define SZ_REGKEY_EVENTLOG _T("EventLog")
#define SZ_REGKEY_SOURCES _T("EventLog\\Sources")
#define SZ_REGKEY_CURRENTLY_OPEN _T("CurrentlyOpen")

#define SZ_REGKEY_SOURCE_ENTERPRISE_OID _T("EnterpriseOID")
#define SZ_REGKEY_SOURCE_APPEND _T("Append")

#define SZ_REGKEY_PARAMETERS _T("EventLog\\Parameters")
#define SZ_REGKEY_PARAMS _T("Parameters")
#define SZ_REGKEY_PARAMS_BASE_ENTERPRISE_OID _T("BaseEnterpriseOID")
#define SZ_REGKEY_PARAMS_TRIMFLAG _T("TrimFlag")
#define SZ_REGKEY_PARAMS_MAXTRAP_SIZE _T("MaxTrapSize")
#define SZ_REGKEY_PARAMS_TRIM_MESSAGE _T("TrimMessage")
#define SZ_REGKEY_PARAMS_THRESHOLD _T("Threshold")
#define SZ_REGKEY_PARAMS_THRESHOLDENABLED _T("ThresholdEnabled")
#define SZ_REGKEY_PARAMS_THRESHOLDCOUNT _T("ThresholdCount")
#define SZ_REGKEY_PARAMS_THRESHOLDTIME _T("ThresholdTime")

#define SZ_REGKEY_EVENT_COUNT _T("Count")
#define SZ_REGKEY_EVENT_TIME _T("Time")
#define SZ_REGKEY_EVENT_FULLID _T("FullID")
#define SZ_REGKEY_SOURCE_EVENT_MESSAGE_FILE _T("EventMessageFile")
#define SZ_NAME_REGVAL_TRANSLATOR_ENABLED _T("TranslatorEnabled")
#define SZ_NAME_REGVAL_REVISIONCOUNT _T("RevisionCount")
#define SZ_NAME_REGVAL_CONFIGTYPE _T("ConfigurationType")

#define SZ_REGVAL_YES "YES"
#define SZ_REGVAL_NO "NO"

//**********************************************************************
// The number of load steps that are performed in CTrapDlg::OnInitDialog
// This is the number of steps that will be reserved in the progress
// indicator for OnInitDialog
//*********************************************************************
#define LOAD_STEPS_IN_TRAPDLG 5

// 11 setup steps plus 10 steps for the four known event logs
#define LOAD_SETUP_STEP_COUNT 11
#define LOAD_LOG_ARRAY_STEP_COUNT 40
#define LOAD_STEP_COUNT (LOAD_SETUP_STEP_COUNT + LOAD_LOG_ARRAY_STEP_COUNT + LOAD_STEPS_IN_TRAPDLG)

class CDlgSaveProgress;
class CRegistryKey;
class CXEventSource;
class CBaseArray : public CObArray
{
public:
    CBaseArray() {}
    ~CBaseArray() {}
    void DeleteAll();
};

class CXEventSource;
class CXEventLog;

//The registry is made up of logs, that contain sources, that contain events
class CXMessage : public CObject
{
public:
    CXMessage(CXEventSource* pEventSource);
    CXEventSource*  m_pEventSource;
    DWORD           m_dwId;
    CString         m_sText;

    CXMessage& operator=(CXMessage& message);
    DWORD GetShortId() {return LOWORD(m_dwId); }
    void GetShortId(CString& sText);
    void GetSeverity(CString& sSeverity);
    void IsTrapping(CString& sIsTrapping);
    void SetAndCleanText(PMESSAGE_RESOURCE_ENTRY pEntry);
};



class CXMessageArray : private CBaseArray
{
public:
    CXMessageArray();
    ~CXMessageArray() {}
    void Initialize(CXEventSource* pEventSource) {m_pEventSource = pEventSource; }
	CXMessage* GetAt(int nIndex) {return (CXMessage*) CBaseArray::GetAt(nIndex); }
	CXMessage* operator[](int nIndex) {return (CXMessage*) CBaseArray::GetAt(nIndex); }
	void Add(CXMessage* pMessage) { CBaseArray::Add(pMessage); }	
	void RemoveAll() {CBaseArray::RemoveAll(); }
	void DeleteAll() {CBaseArray::DeleteAll(); }
    LONG GetSize() {return (LONG)CBaseArray::GetSize(); }
    SCODE LoadMessages();

    CXMessage* FindMessage(DWORD dwId);
    CXEventSource*  m_pEventSource;

private:
    BOOL m_bDidLoadMessages;
    SCODE GetNextPath(CString& sPathlist, CString& sPath);
};


class CXEvent : public CObject
{
public:
    CXEvent(CXEventSource* pEventSource);
    CXEvent(CXMessage* pMessage);
    ~CXEvent();
    CXEventSource*  m_pEventSource;
    DWORD           m_dwCount;
    DWORD           m_dwTimeInterval;
    CXMessage       m_message;

    SCODE Deserialize(CRegistryKey& regkeyParent, CString& sName);
    SCODE Serialize(CRegistryKey& regkeyParent);

    void GetName(CString& sText) {DecString(sText, m_message.m_dwId); }
    void GetCount(CString& sText);
    void GetTimeInterval(CString& sText);
};


class CXEventArray : public CBaseArray
{
public:
    ~CXEventArray() {}
	CXEvent* GetAt(int nIndex) {return (CXEvent*) CBaseArray::GetAt(nIndex); }
	CXEvent* operator[](int nIndex) {return (CXEvent*) CBaseArray::GetAt(nIndex); }
//	void Add(CXEvent* pEvent) { CBaseArray::Add(pEvent); }	
	void Add(CXEvent* pEvent);
	void RemoveAll() {CBaseArray::RemoveAll(); }
	void DeleteAll() {CBaseArray::DeleteAll(); }
    LONG GetSize() {return (LONG)CBaseArray::GetSize(); }

    SCODE Deserialize(CXEventSource* pEventSource);
    SCODE Serialize(CRegistryKey& regkeyParent);

    CXEvent* FindEvent(DWORD dwId);
    SCODE RemoveEvent(CXEvent* pEvent);
};


class CXEventSource  : public CObject
{
public:
    CXEventSource(CXEventLog* pEventLog, CString& sName);
    ~CXEventSource();

    // Public data members
    CXEventLog*      m_pEventLog;
	CString 	     m_sName;
    CXEventArray     m_aEvents;
    CXMessageArray   m_aMessages;
    CString          m_sLibPath;

    CXMessage* FindMessage(DWORD dwId) {return m_aMessages.FindMessage(dwId); }
    CXEvent* FindEvent(DWORD dwId) {return m_aEvents.FindEvent(dwId); }
    CXEventSource* FindEventSource(CString& sEventSource);

    SCODE Deserialize(CRegistryKey& regkeyParent);
    SCODE Serialize(CRegistryKey& regkeyParent);
    void GetEnterpriseOID(CString& sEnterpriseOID, BOOL bGetFullID=FALSE);
    SCODE LoadMessages() {return m_aMessages.LoadMessages(); }

private:
    SCODE GetLibPath(CRegistryKey& regkey);
};

// CObArray is declared a private base type to ensure strong typing.
class CXEventSourceArray : private CBaseArray
{
public:
    // Base array functionality public member functions.
    ~CXEventSourceArray() {DeleteAll(); }
	CXEventSource* GetAt(int nIndex) {return (CXEventSource*) CBaseArray::GetAt(nIndex); }
	CXEventSource* operator[](int nIndex) {return (CXEventSource*) CBaseArray::GetAt(nIndex); }
	void Add(CXEventSource* pEventSource) { CBaseArray::Add(pEventSource); }	
	void RemoveAll() {CBaseArray::RemoveAll(); }
	void DeleteAll() {CBaseArray::DeleteAll(); }
    LONG GetSize() {return (LONG)CBaseArray::GetSize(); }


    // Public members specific to CXEventSourceArray
    CXEventSource* FindEventSource(CString& sSource);
    LONG FindEvent(CString& sLog, CString& sSource, DWORD dwEventId);


    SCODE Deserialize(CXEventLog* pEventLog);
    SCODE Serialize(CRegistryKey& regkey);
};




class CXEventLog : public CObject
{
public:
    CXEventLog(CString& sName) {m_sName = sName;}
    CXEventSourceArray m_aEventSources;
    CString m_sName;


    SCODE Deserialize();
    SCODE Serialize(CRegistryKey& regkey);
    CXEventSource* FindEventSource(CString& sEventSource);
};

inline CXEventSource* CXEventLog::FindEventSource(CString& sEventSource)
{
    return m_aEventSources.FindEventSource(sEventSource);
}


class CXEventLogArray : private CBaseArray
{
public:
    // Base array functionality public member functions.
    CXEventLogArray() {}
    ~CXEventLogArray() {DeleteAll(); }
	CXEventLog* GetAt(int nIndex) {return (CXEventLog*) CBaseArray::GetAt(nIndex); }
	CXEventLog* operator[](int nIndex) {return (CXEventLog*) CBaseArray::GetAt(nIndex); }
	void Add(CXEventLog* pEventLog) { CBaseArray::Add(pEventLog); }	
	void RemoveAll() {CBaseArray::RemoveAll(); }
	void DeleteAll() {CBaseArray::DeleteAll(); }
    LONG GetSize() {return (LONG)CBaseArray::GetSize(); }


    SCODE Deserialize();
    SCODE Serialize();

    CXEventSource* FindEventSource(CString& sLog, CString& sEventSource);
};



class CTraps
{
public:
    CXEventLogArray m_aEventLogs;
    SCODE Serialize();
    SCODE Deserialize();
};


class CTrapParams
{
public:
    CTrapParams();
    SCODE Serialize();
    SCODE Deserialize();
    SCODE ResetExtensionAgent();
    BOOL ThrottleIsTripped();

    CString m_sBaseEnterpriseOID;
    CString m_sSupportedView;
    CString m_sTracefileName;
    DWORD   m_dwTraceLevel;

    // Data members for the "limit" section of the settings dialog
    struct {
        BOOL    m_bTrimFlag;            // Limit trap length
        BOOL    m_bTrimMessages;        // Trim messages first
        DWORD   m_dwMaxTrapSize;        // Trap length (bytes)
    }m_trapsize;

    // Data members for the "throttle" section of the settings dialog
    struct {
        long m_nTraps;
        long m_nSeconds;
        BOOL m_bIsEnabled;
    }m_throttle;
};

class CTrapReg
{
public:
    CTrapReg();
    ~CTrapReg();
    SCODE Connect(LPCTSTR pszComputerName, BOOL bIsReconnecting = FALSE);
    SCODE Serialize();
    SCODE Deserialize();
    SCODE LockRegistry();
    void UnlockRegistry();
    SCODE SetConfigType(DWORD dwConfigType);
    DWORD GetConfigType() {return m_dwConfigType; }
    void SetApplyButton(CButton *pbtnApply) { m_pbtnApply = pbtnApply; }
    void SetDirty(BOOL bDirty);

    inline BOOL SourceHasTraps(CString& sSource);

    // Public data members.
    CRegistryKey m_regkeySource;        // SYSTEM\CurrentControlSet\Services\EventLogs
    CRegistryKey m_regkeySnmp;          // SOFTWARE\Microsoft\SNMP_EVENTS
    CRegistryKey m_regkeyEventLog;
    CXEventLogArray m_aEventLogs;
    CTrapParams m_params;
    CDlgSaveProgress* m_pdlgSaveProgress;
    CDlgSaveProgress* m_pdlgLoadProgress;
    LONG m_nLoadStepsPerSource;
    LONG m_nLoadStepsPerLog;
    LONG m_nLoadSteps;
    BOOL m_bShowConfigTypeBox;
    BOOL m_bRegIsReadOnly;
    BOOL m_bIsDirty;
    BOOL m_bSomeMessageWasNotFound;
    CString m_sComputerName;
    CButton *m_pbtnApply;

private:
    LONG GetSaveProgressStepCount();
    SCODE BuildSourceHasTrapsMap();
    BOOL m_bNeedToCloseKeys;
    CMapStringToPtr m_mapSourceHasTraps;
    BOOL m_bDidLockRegistry;
    DWORD m_dwConfigType;
};




extern CTrapReg g_trapreg;

enum {
    CONFIG_TYPE_DEFAULT = 0,
    CONFIG_TYPE_CUSTOM,
    CONFIG_TYPE_DEFAULT_PENDING
};


// Error failure values.
enum
{
    E_REGKEY_NOT_FOUND = -1000,
    E_REG_CANT_CONNECT,
    E_REGKEY_NOT_INSTALLED,
    E_REGKEY_CANT_OPEN,
    E_REGKEY_NO_CREATE,
    E_REG_NOT_INSTALLED,
    E_REGKEY_LOST_CONNECTION,
    E_ACCESS_DENIED,
    E_MESSAGE_NOT_FOUND


};

// Success status codes
enum
{
    S_NO_EVENTS = 1000,
    S_NO_SOURCES,
    S_SAVE_CANCELED,
    S_LOAD_CANCELED
};


//*******************************************************************
// CTrapReg::SourceHasTraps
//
// Check to see if traps have been configured for the specified event
// source.
//
// Parameters:
//      CString& sEventSource
//          The name of the event source.
//
// Returns:
//      TRUE if the event source has traps, FALSE otherwise.
//
//********************************************************************
inline BOOL CTrapReg::SourceHasTraps(CString& sEventSource)
{
    LPVOID pVoid;
	CString tmp(sEventSource);
	tmp.MakeUpper();
    return m_mapSourceHasTraps.Lookup(tmp, pVoid);
}


#endif //_trapreg_h
