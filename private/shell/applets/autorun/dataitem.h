//
//
//

#pragma once

#include <debug.h>

enum {
    WF_PERUSER          = 0x0001,   // item is per user as opposed to per machine
    WF_ADMINONLY        = 0x0002,   // only show item if user is an admin
    WF_ALTERNATECOLOR   = 0x1000,   // show menu item text in the "visited" color
    WF_DISABLED         = 0x2000,   // Treated normally except cannot be launched
};

class CDataItem
{
public:
    CDataItem();
    ~CDataItem();

    TCHAR * GetTitle()      { return m_pszTitle; }
    TCHAR * GetMenuName()   { return m_pszMenuName?m_pszMenuName:m_pszTitle; }
    TCHAR * GetDescription(){ return m_pszDescription; }
    TCHAR   GetAccel()      { return m_chAccel; }
    int     GetImgIndex()   { return m_iImage; }

    BOOL SetData( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex );
    BOOL Invoke( HWND hwnd );

    // flags
    //
    // This var is a bit mask of the following values
    //  PERUSER     True if item must be completed on a per user basis
    //              False if it's per machine
    //  ADMINONLY   True if this item can only be run by an admin
    //              False if all users should do this
    DWORD   m_dwFlags;

protected:
    TCHAR * m_pszTitle;
    TCHAR * m_pszMenuName;
    TCHAR * m_pszDescription;
    TCHAR   m_chAccel;
    int     m_iImage;

    TCHAR * m_pszCmdLine;
    TCHAR * m_pszArgs;
};
