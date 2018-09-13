//
//  ITBDROP_H
//  Header file for the internet toolbar's drop target.
//
//  History:
//      8/22/96 -   t-mkim: created

#ifndef _ITBDROP_H
#define _ITBDROP_H

#define TBIDM_BACK              0x120
#define TBIDM_FORWARD           0x121
#define TBIDM_HOME              0x122
#define TBIDM_SEARCH            0x123  // copy of this in shdocvw\basesb.cpp
#define TBIDM_STOPDOWNLOAD      0x124
#define TBIDM_REFRESH           0x125
#define TBIDM_FAVORITES         0x126
#define TBIDM_THEATER           0x128
#define TBIDM_HISTORY           0x12E
#ifdef ENABLE_CHANNELPANE
#define TBIDM_CHANNELS          0x12F
#endif
#define TBIDM_PREVIOUSFOLDER    0x130
#define TBIDM_CONNECT           0x131
#define TBIDM_DISCONNECT        0x132
#define TBIDM_ALLFOLDERS        0x133

#define REGSTR_SET_HOMEPAGE_RESTRICTION               TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define REGVAL_HOMEPAGE_RESTRICTION                   TEXT("HomePage")

// Class for implementing a single drop target for all the various and sundry things
// that can be dropped onto on the internet toolbar.
class CITBarDropTarget : public IDropTarget
{
private:
    ULONG _cRef;
    HWND _hwndParent;
    IDropTarget *_pdrop;    // hand on to the the favorites target
    int _iDropType;         // Which format data is in.
    int _iTarget;           // what item are we running for

public:
    CITBarDropTarget(HWND hwnd, int iTarget);

    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef (void);
    STDMETHODIMP_(ULONG) Release (void);

    STDMETHODIMP DragEnter(IDataObject *dtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
};

#endif //_ITBDROP_H

