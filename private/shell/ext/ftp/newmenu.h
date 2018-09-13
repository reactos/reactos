/*****************************************************************************\
    FILE: newmenu.h
    
    DESCRIPTION:
        The file supports the "New" menu to create new items on the FTP server.
    This currently only supports Folders but hopefully it will support other
    items later.
\*****************************************************************************/

#ifndef _NEWMENU_H
#define _NEWMENU_H

// For CreateNewFolderCB:
// The following struct is used when recursively downloading
// files/dirs from the FTP server after a "Download" verb.
typedef struct tagFTPCREATEFOLDERSTRUCT
{
    LPCWSTR             pszNewFolderName;
    CFtpFolder *        pff;
} FTPCREATEFOLDERSTRUCT;


// Public APIs (DLL wide)
HRESULT CreateNewFolder(HWND hwnd, CFtpFolder * pff, CFtpDir * pfd, IUnknown * punkSite, BOOL fPosition, POINT point);
HRESULT CreateNewFolderCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pvFCFS, BOOL * pfReleaseHint);


#endif // _NEWMENU_H





