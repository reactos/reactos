/*****************************************************************************
 *	statusbr.h
 *****************************************************************************/

#ifndef _STATUSBAR_H
#define _STATUSBAR_H

#include <urlmon.h>

CStatusBar * CStatusBar_Create(HWND hwndStatus);

#define MAX_NUM_ZONES_ICONS         12

enum ICON_TODISPLAY
{
    ITD_WriteAllowed = 0,
    ITD_WriteNotAllowed,
    ITD_MAX
};

enum ICON_SLOT
{
    ISLOT_WritePermission = 0,
    ISLOT_MAX
};

#define STATUS_PANE_STATUS      0
#define STATUS_PANE_USERNAME    1
#define STATUS_PANE_ZONE        2
// #define STATUS_PANE_WRITEICON   4


/*****************************************************************************
 *
 *	CStatusBar
 *
 *****************************************************************************/

class CStatusBar
{
public:
    CStatusBar(HWND hwndStatus);
    ~CStatusBar(void);

    // Public Member Functions
    void SetStatusMessage(UINT nMessageID, LPCTSTR pszExtra);

    void SetUserName(LPCTSTR pszUserName, BOOL fAnnonymous);
    void SetFolderAttribute(BOOL fWriteAllowed);
    void UpdateZonesPane(LPCTSTR pszUrl);
    HRESULT Resize(LONG x, LONG y);

    friend CStatusBar * CStatusBar_Create(HWND hwndStatus) { return new CStatusBar(hwndStatus); };

protected:
    // Private Member Variables
    BOOL                    m_fInited : 1;
    BOOL                    m_fWriteAllowed : 1;

    HWND                    m_hwndStatus;                   // HWND for entire bar
    IInternetSecurityManager *  m_pism;
    IInternetZoneManager *  m_pizm;
    HICON                   m_arhiconZones[MAX_NUM_ZONES_ICONS];
    long                    m_lCurrentZone;
    HICON                   m_arhiconGeneral[ITD_MAX];

    LPTSTR                  m_pszUserName;
    LPTSTR                  m_pszUserNameTT;


    // Private Member Variables
    HRESULT _InitStatusBar(void);
    HRESULT _SetParts(void);

    HRESULT _SetIconAndTip(ICON_SLOT nIconSlot, ICON_TODISPLAY nIconToDisplay, LPCTSTR pszTip);
    HRESULT _LoadZoneInfo(LPCTSTR pszUrl);
    HRESULT _SetZone(void);
    void _SetUserParts(void);
    void _CacheZonesIcons(void);
};

#endif // _STATUSBAR_H
