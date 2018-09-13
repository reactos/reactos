/******************************************************************************

  Source File:  Shell Extension Classes.H

  This file defines the Shell extension classes.  Since the ICM UI is a shell
  extension, these are essential.  Rather than slavishly including sample code,
  this has been written as much as possible from scratch.

  If you're not familiar with OLE, then this is going to be a bit difficult
  going.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  10-28-96 A-RobKj (Pretty Penny Enterprises) began coding
  12-03-96 A-RobKj moved the CGlobals class to the pre-comp header file.
  01-07-97 KjelgaardR@acm.org   Stubbed IContextMenu interface for profile
            management in favor of shell association which uses a RunDLL
            Entry point- this allows invocation via Enter, and double-click

******************************************************************************/

// The class ID of this Shell extension is taken from the one used on Win95.

//  This was a deliberate decision, to ease the upgrade process.
//
// class id:  dbce2480-c732-101b-be72-ba78e9ad5b27
//
                                  
DEFINE_GUID(CLSID_ICM, 0xDBCE2480L, 0xC732, 0x101B, 0xBE, 0x72, 0xBA, 0x78, 
            0xE9, 0xAD, 0x5B, 0x27);

//  This class ID is for the printer profile management UI.  It is implemented
//  within the same module, for now, but having a separate GUID makes it
//  easier to implement separately later, if so desired.  It also simplifies
//  implementation.

//  Class ID:  675f097e-4c4d-11d0-b6c1-0800091aa605

DEFINE_GUID(CLSID_PRINTERUI, 0x675F097EL, 0x4C4D, 0x11D0, 0xB6, 0xC1, 0x08,
             0x00, 0x09, 0x1A, 0xA6, 0x05);

//  This class ID is used (at least temporarily) for the display profile
//  management UI.  If I wind up not needing it, I will convert it to a
//  different class (such as scanners or cameras)

//  Class ID: 5db2625a-54df-11d0-b6c4-0800091aa605

DEFINE_GUID(CLSID_MONITORUI, 0x5db2625a, 0x54df, 0x11d0, 0xb6, 0xc4, 0x08, 
            0x00, 0x09, 0x1a, 0xa6, 0x05);

//  This class ID is used (at least temporarily) for the scanner/camera profile
//  management UI.

//  Class ID: 176d6597-26d3-11d1-b350-080036a75b03

DEFINE_GUID(CLSID_SCANNERUI, 0x176d6597, 0x26d3, 0x11d1, 0xb3, 0x50, 0x08,
            0x00, 0x36, 0xa7, 0x5b, 0x03);

typedef enum    {IsProfile, IsPrinter, IsScanner, IsMonitor} UITYPE;

//  First of all, we're going to need a class factory.  The shell uses this
//  factory to create instances of the objects which implement the interfaces
//  it needs.

class CIcmUiFactory : public IClassFactory
{
    ULONG   m_ulcReferences;
    UITYPE  m_utThis;

public:
    CIcmUiFactory(REFCLSID rclsid);
    ~CIcmUiFactory() { CGlobals::Detach(); }

    //IUnknown interface
    STDMETHODIMP            QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG)    AddRef() { return ++m_ulcReferences; }
    STDMETHODIMP_(ULONG)    Release() {
        if  (--m_ulcReferences) 
            return  m_ulcReferences;

        delete  this;
        return  0L;
    }

    //IClassFactory interface
    STDMETHODIMP    CreateInstance(LPUNKNOWN punk, REFIID riid, 
                                   void **ppvObject);
    STDMETHODIMP    LockServer(BOOL) { return NOERROR; }

    static SCODE    KeyToTheFactory(REFCLSID rclsid, REFIID riid, 
                                    void **ppvObject);
};

//  This class implements the entire extension- it includes a context menu
//  handler, and Icon handler, and a property sheet extension.

class CICMUserInterface : public IContextMenu, IShellExtInit, IExtractIcon, 
                                    IPersistFile, IShellPropSheetExt
{
    ULONG           m_ulcReferences;
    LPDATAOBJECT    m_lpdoTarget;
    CString         m_csFile;         //  Profile for icon extraction
    //check this - m_acWork doesn't appear to be referenced anywhere.
    TCHAR           m_acWork[80];     //  A little work buffer
    UITYPE          m_utThis;
    BOOL            m_bInstalledContext, // 'True' when every selected file(s) are installed
                    m_bMultiSelection;

    HRESULT         AddPrinterTab(LPFNADDPROPSHEETPAGE lpfnAddPage, 
                                  LPARAM lParam);

    HRESULT         AddAssociateTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                    LPARAM lParam);

    HRESULT         AddProfileTab(LPFNADDPROPSHEETPAGE lpfnAddPage, 
                                  LPARAM lParam);

    HRESULT         AddScannerTab(LPFNADDPROPSHEETPAGE lpfnAddPage, 
                                  LPARAM lParam);

    HRESULT         AddMonitorTab(LPFNADDPROPSHEETPAGE lpfnAddPage, 
                                  LPARAM lParam);

public:
    CICMUserInterface(UITYPE utThis);

    ~CICMUserInterface() { 

        if  (m_lpdoTarget)
            m_lpdoTarget -> Release();
        CGlobals::Detach();
    }

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef() { return ++m_ulcReferences; }
    STDMETHODIMP_(ULONG)    Release() {
        if  (--m_ulcReferences) 
            return  m_ulcReferences;
        
        delete  this;
        return  0L;
    }

    //  IContextMenu methods
    STDMETHODIMP    QueryContextMenu(HMENU hMenu, UINT indexMenu, 
                                     UINT idCmdFirst, UINT idCmdLast, 
                                     UINT uFlags);
   
    STDMETHODIMP    InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);

    STDMETHODIMP    GetCommandString(UINT_PTR idCmd, UINT uFlags, 
                                     UINT FAR *reserved, LPSTR pszName, 
                                     UINT cchMax);

    //  IShellExtInit methods
    STDMETHODIMP    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj,
                               HKEY hKeyID);

    //  IExtractIcon methods
    STDMETHODIMP    GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax,
                                    int *piIndex, UINT *pwFlags);

    STDMETHODIMP    Extract(LPCTSTR pszFile, UINT nIconIndex, 
                            HICON *phiconLarge, HICON *phiconSmall, 
                            UINT nIconSize);

    //  IPersistFile methods- note that (as the OLE documentation says) only 
    //  Load ever gets used.  GetClassID is from IPersist, from which
    //  IPersistFile is derived.  We fail everything we don't expect to see
    //  called.

    STDMETHODIMP    GetClassID(LPCLSID lpClassID) { return E_FAIL; }

    STDMETHODIMP    IsDirty() { return S_FALSE; }

    STDMETHODIMP    Load(LPCOLESTR lpszFileName, DWORD grfMode);

    STDMETHODIMP    Save(LPCOLESTR lpszFileName, BOOL fRemember) {
        return  E_FAIL;
    }

    STDMETHODIMP    SaveCompleted(LPCOLESTR lpszFileName) { return E_FAIL; }

    STDMETHODIMP    GetCurFile(LPOLESTR FAR* lplpszFileName) {
        return  E_FAIL;
    }

    //  IShellPropSheetExt methods
    STDMETHODIMP    AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    
    STDMETHODIMP    ReplacePage(UINT uPageID, 
                                LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                                LPARAM lParam) { return E_FAIL; }

};

