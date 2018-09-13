#include "ras.h"
#include "raserror.h"

//
// Useful registry constants
//
extern const TCHAR c_szRASKey[];
extern const TCHAR c_szProfile[];

//
// Data for the dialer and property sheet is loaded and saved
// in this format
//
struct _dialpropdata {

    BOOL        fEnabled;
    BOOL        fUnattended;
    TCHAR       pszConnection[RAS_MaxEntryName + 1];
    TCHAR       pszUsername[UNLEN + 1];
    TCHAR       pszPassword[PWLEN + 1];
    TCHAR       pszDomain[DNLEN + 1];
    UINT        uRedialAttempts;
    UINT        uRedialDelay;

};

typedef struct _dialpropdata DIALPROPDATA;

//
// Forward declaration
//
class CConnectionAgent;

// the various states that a client can be in
typedef enum {
    CLIENT_NEW,
    CLIENT_CONNECTING,
    CLIENT_CONNECTED,
    CLIENT_ABORT,
    CLIENT_DISCONNECTED,
} ClientState;


//
// Connection client object
//
class CConnClient : public INotificationSink,
                    public IOleCommandTarget
{
private:
    ~CConnClient() {};
public:    
    CConnClient();

    //
    // IUnknown members
    //
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // INotificationSink members
    //
    STDMETHODIMP OnNotification(
            LPNOTIFICATION         pNotification,
            LPNOTIFICATIONREPORT   pNotificationReport,
            DWORD                  dwReserved
            );

    //
    // IOleCommandTarget members
    //
    STDMETHODIMP QueryStatus(
            const GUID __RPC_FAR *pguidCmdGroup,
            ULONG cCmds,
            OLECMD __RPC_FAR prgCmds[  ],
            OLECMDTEXT __RPC_FAR *pCmdText
            );
        
    STDMETHODIMP Exec( 
            const GUID __RPC_FAR *pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANT __RPC_FAR *pvaIn,
            VARIANT __RPC_FAR *pvaOut
            );

    HRESULT SetConnAgent(IUnknown *punk);

protected:
    long                    m_cRef;
    IOleCommandTarget *     m_poctAgent;
    BSTR                    m_bstrURL;
    ClientState             m_State;
    LPNOTIFICATIONREPORT    m_pReport;
    long                    m_iCookie;

    HRESULT Connect(void);
    HRESULT Disconnect(void);
    HRESULT DeliverProgressReport(SCODE, BSTR);
};

//
// Main dialer agent object
//
class CConnectionAgent : public IOleCommandTarget
{
private:
    ~CConnectionAgent();
public:
    CConnectionAgent();

    //
    // IUnknown members
    //
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IOleCommandTarget members
    //
    STDMETHODIMP QueryStatus(
            const GUID __RPC_FAR *pguidCmdGroup,
            ULONG cCmds,
            OLECMD __RPC_FAR prgCmds[  ],
            OLECMDTEXT __RPC_FAR *pCmdText
            );
        
    STDMETHODIMP Exec( 
            const GUID __RPC_FAR *pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANT __RPC_FAR *pvaIn,
            VARIANT __RPC_FAR *pvaOut
            );


    //
    // Other public members 
    //
    DWORD           m_dwRegisterHandle;

protected:
    long            m_cRef;
    BOOL            IsDialPossible(void);
    BOOL            IsDialExisting(void);
    void            Notify(HRESULT hr, TCHAR *pszErrorText);
    void            Connect(void);
    void            Disconnect(void);
    void            Clean(void);

    INotificationMgr *m_pMgr;
    DIALPROPDATA *  m_pData;
    DWORD           m_dwFlags;
    long            m_lConnectionCount;
    HDPA            m_hdpaClient;
};

// Connection agent flags
#define CA_CONNECTING_NOW       1
#define CA_OFFLINE_STATE_READ   2
#define CA_OFFLINE              4
#define CA_DIALED               8
#define CA_LOADED_RAS           16

// IOleCommandTarget commands
#define AGENT_CONNECT           0x00
#define AGENT_DISCONNECT        0x01
#define AGENT_NOTIFY            0x10

// CLIENTINFO struct - info about each client
typedef struct _ci {

    IOleCommandTarget *     poctClient;
    DWORD                   dwFlags;

} CLIENTINFO;

// flags for CLIENTINFO
#define CLIENT_DISCONNECT       1
#define CLIENT_NOTIFIED         2
