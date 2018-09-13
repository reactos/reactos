//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       trayagnt.h
//
//  Contents:   tray notification agent
//
//  Classes:
//
//  Functions:
//
//  History:    01-14-1997  rayen (Raymond Endres)  Created
//
//----------------------------------------------------------------------------
#ifndef TRAYAGNT_H_
#define TRAYAGNT_H_

//----------------------------------------------------------------------------
// Tray Agent object
//----------------------------------------------------------------------------
class CTrayAgent : public INotificationSink
{
protected:
    ULONG           m_cRef;         // OLE ref count
    #ifdef DEBUG
    DWORD           m_AptThreadId;  // 
    #endif

private:
    ~CTrayAgent(void);

public:
    CTrayAgent(void);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // INotificationSink member(s)
    //
    STDMETHODIMP         OnNotification(
        LPNOTIFICATION         pNotification,
        LPNOTIFICATIONREPORT   pNotificationReport,
        DWORD                  dwReserved
    );
};

//----------------------------------------------------------------------------
// TrayUI object (not COM object)
//----------------------------------------------------------------------------
#define TRAYUI_CLOGS    128

typedef struct LogEntryType {
    FILETIME    ftLog;
    CLSID       clsidAgent;
    CLSID       startCookie;
    BSTR        bstrStatus;
} * PLogEntry;

class CTrayUI
{
private:
    HWND            m_hwnd;             // hidden window
    DWORD           m_fUpdatingTrayIcon;// Is updating?

#if WANT_REGISTRY_LOG    
    int             m_cLogs;            // count of valid logs;
    int             m_cLogPtr;          // pointer to the replacing candidate.
                                        // Round Robin Algorithm;
    LogEntryType    m_aLogEntry[TRAYUI_CLOGS];
#endif

    LONG    m_cUpdates;                 // count of ongoing updates
    #ifdef DEBUG
    DWORD   m_AptThreadId;              // 
    #endif

private:
    STDMETHODIMP        SetTrayIcon(DWORD fUpdating);
    STDMETHODIMP        SyncLogWithReg(int, BOOL);
    
public:
    CTrayUI(void);
    ~CTrayUI(void);

    STDMETHODIMP        InitTrayUI(void);
    STDMETHODIMP        DestroyTrayUI(void);
    STDMETHODIMP        OpenSubscriptionFolder(void);
    STDMETHODIMP        OpenContextMenu(POINT *);
    STDMETHODIMP        UpdateNow(INotification *);
    STDMETHODIMP        ConfigChanged(void);
    STDMETHODIMP        OnBeginReport(INotification *);
    STDMETHODIMP        OnEndReport(INotification *);
#if WANT_REGISTRY_LOG    
    STDMETHODIMP        AddToLog(BSTR bstrLog, CLSID clsidAgent, CLSID startCookie);
    STDMETHODIMP        LoadLogFromReg(void);
    STDMETHODIMP        SaveLogToReg(void);
#endif
};

#endif TRAYAGNT_H_

