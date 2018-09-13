//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//      DIALMON.H - central header file for dial monitor app
//

//      HISTORY:
//      
//      4/18/95         jeremys         Created.
//

#ifndef _DIALMON_H_
#define _DIALMON_H_

#include <regstr.h>

// We need winver 4.00 so ras doesn't puke on our new structure sizes in
// RasEnumConnections.
#undef WINVER
#define WINVER 0x400
#include <ras.h>
#include <raserror.h>

// how to tell dialmon that something is going on
void IndicateDialmonActivity(void);

// give user 30 seconds to respond to dialog 
#define DISCONNECT_DLG_COUNTDOWN        30      

// truncate and add "..." if connectoid name longer than 20 characters
#define MAX_CONNECTOID_DISPLAY_LEN      50      

// private message sent to disconnect dialog to dismiss it
#define WM_QUIT_DISCONNECT_DLG          WM_USER+50

// sizes of various unknown things
#define MAX_RES_LEN                     255
#define DEF_CONN_BUF_SIZE               4096

// class name used for dial monitoring
#define AUTODIAL_MONITOR_CLASS_NAME     "MS_AutodialMonitor"

// max ras connections we care about
#define MAX_CONNECTION                  8

// forward declaration
class BUFFER;

///////////////////////////////////////////////////////////////////////////
//
// CDialMon class definition
//
///////////////////////////////////////////////////////////////////////////

class CDialMon
{
private:
    BOOL        _fInDisconnectFunction; // prevent dialog reentrancy
    DWORD       _dwTimeoutMins;         // timeout value, in minutes
    DWORD       _dwElapsedTicks;        // elapsed ticks with no activity
    BOOL        _fNoTimeout;            // monitor idle or just exit?
    BOOL        _fDisconnectOnExit;
    BOOL        _fConnected;
    TCHAR       _pszConnectoidName[RAS_MaxEntryName+1];   
                                        // name of connectoid of interest
    UINT_PTR    _uIdleTimerID;          // timer id on parent window
    HWND        _hwndDialmon;

public:
    HWND        _hDisconnectDlg;

    CDialMon();
    ~CDialMon();

    void        OnSetConnectoid(BOOL fNoTimeout);
    void        OnActivity(void);
    void        OnTimer(UINT_PTR uTimerID);
    void        OnExplorerExit();
                
private:        
    BOOL        StartMonitoring(void);
    void        StopMonitoring(void);
    INT_PTR     StartIdleTimer(void);
    void        StopIdleTimer(void);
    void        CheckForDisconnect(BOOL fTimer);
    BOOL        PromptForDisconnect(BOOL fTimer, BOOL *pfDisconnectDisabled);
    BOOL        RefreshTimeoutSettings(void);
    BOOL        LoadRNADll(void);
    void        UnloadRNADll(void);
};

// structure for passing params to disconnect prompt dialog
typedef struct tagDISCONNECTDLGINFO {
        LPTSTR  pszConnectoidName;   // input: name of connectoid
        DWORD   dwTimeout;           // input: idle timeout in minutes
        BOOL    fTimer;              // input: timer or shutdown?
        DWORD   dwCountdownVal;      // internal: state of countdown in dialog
        BOOL    fDisconnectDisabled; // output: TRUE if disconnect disabled
        CDialMon *pDialMon;          // pointer back to dialmon class
} DISCONNECTDLGINFO;

///////////////////////////////////////////////////////////////////////////
//
// BUFFER class and helpers
//
///////////////////////////////////////////////////////////////////////////

class BUFFER_BASE
{
protected:
        UINT _cch;

        virtual BOOL Alloc( UINT cchBuffer ) = 0;
        virtual BOOL Realloc( UINT cchBuffer ) = 0;

public:
        BUFFER_BASE()  { _cch = 0; }
        ~BUFFER_BASE() { _cch = 0; }
        BOOL Resize( UINT cchNew );
        UINT QuerySize() const { return _cch; };
};

class BUFFER : public BUFFER_BASE
{
protected:
        TCHAR *_lpBuffer;

        virtual BOOL Alloc( UINT cchBuffer );
        virtual BOOL Realloc( UINT cchBuffer );

public:
        BUFFER( UINT cchInitial=0 );
        ~BUFFER();
        BOOL Resize( UINT cchNew );
        TCHAR * QueryPtr() const { return (TCHAR *)_lpBuffer; }
        operator TCHAR *() const { return (TCHAR *)_lpBuffer; }
};

#endif 
