/******************************************************************************

  Source File:  Shell Extension Classes.CPP

  This file implements the Shell Extension classes.

  Copyright (c) 1996, 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:

  10-28-96  A-RobKj (Pretty Penny Enterprises) began coding
  12-04-96  A-RobKj Added the printer tab support
  12-13-96  A-RobKj Modified for faster Icon extraction
  01-07-97  KjelgaardR@ACM.Org  Removed IContextMenu functions in favor of
            an exported ManageColorProfile procedure.  This supplies a
            language-independent "Open" item that works with either mouse
            button.
  01-08-97  KjelgaardR@acm.org  Added utility routine to determine if a printer
            is a color model.  Modified printer UI to only add pages for color
            printers.

******************************************************************************/

#include    "ICMUI.H"

#include    <shlobj.h>
#include    <string.h>

#include    <initguid.h>
#include    "ShellExt.H"
#include    "Resource.H"

//  Declare some storage space for the global statics

int     CGlobals::m_icDLLReferences = 0;
HMODULE CGlobals::m_hmThisDll = NULL;
CStringArray    CGlobals::m_csaProfiles;
BOOL            CGlobals::m_bIsValid = FALSE;

//  Some global useful procs- an error reporter

void    CGlobals::Report(int idError, HWND m_hwndParent) {
    CString csMessage, csTitle;

    csMessage.Load(idError);
    csTitle.Load(MessageBoxTitle);

    MessageBox(m_hwndParent, csMessage, csTitle, MB_OK|MB_ICONEXCLAMATION);
}

int    CGlobals::ReportEx(int idError, HWND m_hwndParent,
                          BOOL bSystemMessage, UINT uType, DWORD dwNumMsg, ...) {
    CString csMessage, csTitle;
    va_list argList;

    va_start(argList,dwNumMsg);
    csMessage.LoadAndFormat(idError,NULL,bSystemMessage,dwNumMsg,&argList);
    csTitle.Load(MessageBoxTitle);
    va_end(argList);

    return (MessageBox(m_hwndParent, csMessage, csTitle, uType));
}

//  A profile status checker

BOOL    CGlobals::IsInstalled(CString& csProfile) {
//    if  (!m_bIsValid) {
        ENUMTYPE    et = {sizeof et, ENUM_TYPE_VERSION, 0, NULL};

        CProfile::Enumerate(et, m_csaProfiles);
        m_bIsValid = TRUE;
//    }

    for (unsigned u = 0; u < m_csaProfiles.Count(); u++)
        if  (!lstrcmpi(csProfile.NameOnly(), m_csaProfiles[u].NameOnly()))
            break;

    return  u < m_csaProfiles.Count();
}

//  Utility routine to report if a printer is color or monochrome
BOOL CGlobals::ThisIsAColorPrinter(LPCTSTR lpstrName) {
  HDC hdcThis = CGlobals::GetPrinterHDC(lpstrName);
  BOOL bReturn = FALSE;
  if  (hdcThis) {
    bReturn =  2 < (unsigned) GetDeviceCaps(hdcThis, NUMCOLORS);
    DeleteDC(hdcThis);
  }
  return bReturn;
}

// Utility to determine the hdc for a printer
// The caller is responsible for calling
// DeleteDC() on the result
HDC CGlobals::GetPrinterHDC(LPCTSTR lpstrName) {

    HANDLE  hPrinter;   //  Get a handle on it...
    LPTSTR  lpstrMe = const_cast <LPTSTR> (lpstrName);

    if  (!OpenPrinter(lpstrMe, &hPrinter, NULL)) {
        _RPTF2(_CRT_WARN, "Unable to open printer '%s'- error %d\n", lpstrName,
            GetLastError());
        return  FALSE;
    }

    //  First, use DocumentProperties to find the correct DEVMODE size- we
    //  must use the DEVMODE to force color on, in case the user's defaults
    //  have turned it off...

    unsigned short lcbNeeded = (unsigned short) DocumentProperties(NULL, hPrinter, lpstrMe, NULL,
        NULL, 0);

    if  (lcbNeeded <= 0) {
        _RPTF2(_CRT_WARN,
            "Document Properties (get size) for '%s' returned %d\n", lpstrName,
            lcbNeeded);
        ClosePrinter(hPrinter);
        return  FALSE;
    }

    HDC hdcThis = NULL;

    union {
        LPBYTE      lpb;
        LPDEVMODE   lpdm;
    };

    lpb = new BYTE[lcbNeeded];

    if  (lpb) {

        ZeroMemory(lpb,lcbNeeded);
        lpdm -> dmSize = lcbNeeded;
        lpdm -> dmFields = DM_COLOR;
        lpdm -> dmColor = DMCOLOR_COLOR;
        if  (IDOK == DocumentProperties(NULL, hPrinter, lpstrMe, lpdm, lpdm,
            DM_IN_BUFFER | DM_OUT_BUFFER)) {

            //  Turn off ICM, since not nessesary here.
            //
            lpdm -> dmICMMethod = DMICMMETHOD_NONE;

            //  Finally, we can create the DC!
            //  Note: we're not actually creating a DC, just an IC
            hdcThis = CreateIC(NULL, lpstrName, NULL, lpdm);
        } else {
            _RPTF2(_CRT_WARN,
                "DocumentProperties (retrieve) on '%s' failed- error %d\n",
                lpstrName, GetLastError());
        }
        delete lpb;
    }
    else
        _RPTF2(_CRT_WARN, "ThisIsAColorPrinter(%s) failed to get %d bytes\n",
            lpstrName, lcbNeeded);

    ClosePrinter(hPrinter);

    return hdcThis;
}

//  Required Shell Extension DLL interfaces

STDAPI  DllCanUnloadNow() {
    return  CGlobals::CanUnload();
}

STDAPI  DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvOut) {
    return  CIcmUiFactory::KeyToTheFactory(rclsid, riid, ppvOut);
}

extern "C" int APIENTRY DllMain(HMODULE hmThis, DWORD dwReason,
                                LPVOID lpvReserved) {
#if defined(DEBUG) || defined(_DEBUG)
    static  HANDLE  hfWarnings; //  Log file
#endif
    switch  (dwReason) {

        case    DLL_PROCESS_ATTACH:

            //  Save the handle
            CGlobals::SetHandle(hmThis);
#if defined(DEBUG) || defined(_DEBUG)
            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
            _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
            hfWarnings = CreateFileA("C:\\ICMUIWarn.Txt", GENERIC_WRITE, 0,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if  (hfWarnings!= INVALID_HANDLE_VALUE) {
                SetFilePointer(hfWarnings, 0, NULL, FILE_END);
                _CrtSetReportFile(_CRT_WARN, hfWarnings);
            }
            _RPTF1(_CRT_WARN, "ICMUI DLL being loaded- handle %X\n", hmThis);
            break;

        case    DLL_PROCESS_DETACH:
            _RPTF0(_CRT_WARN, "ICMUI DLL being unloaded\n");

            if  (hfWarnings != INVALID_HANDLE_VALUE)
                CloseHandle(hfWarnings);
#endif
    }

    return  1;
}

//  CIcmUiFactory member functions- these are used to provide external access
//  to the class factory.  The shell uses these to initialize extensions for
//  both context menus and property sheets, both of which we provide,
//  fortunately in the same object...

CIcmUiFactory::CIcmUiFactory(REFCLSID rclsid) {
    m_ulcReferences = 0;
    CGlobals::Attach();
    if  (IsEqualIID(rclsid, CLSID_ICM))
        m_utThis = IsProfile;
    else if (IsEqualIID(rclsid, CLSID_PRINTERUI))
        m_utThis = IsPrinter;
    else if (IsEqualIID(rclsid, CLSID_SCANNERUI))
        m_utThis = IsScanner;
    else
        m_utThis = IsMonitor;
}

STDMETHODIMP    CIcmUiFactory::QueryInterface(REFIID riid, void **ppvObject) {

    if  (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = this;
        AddRef();
        return  NOERROR;
    }
    //  Asked for an interface we ain't got!
    *ppvObject = NULL;
    return  E_NOINTERFACE;
}

//  IClassFactory interface functions

STDMETHODIMP    CIcmUiFactory::CreateInstance(LPUNKNOWN punk, REFIID riid,
                                              void **ppvInstance) {

    *ppvInstance = NULL;

    if  (punk)  //  We don't allow aggregation
        return  CLASS_E_NOAGGREGATION;

    //  We simply create a new ICM UI object, and return an interface to it.
    //  This will get queried by the shell for IExtShellInit, and the init job
    //  will be done.

    CICMUserInterface   *pcicmui = new CICMUserInterface(m_utThis);

    if  (!pcicmui)
        return  E_OUTOFMEMORY;

    //  Let's be paranoid- if the QueryInterface failes, kill the ICMUI object,
    //  so we can still be unloaded!

    HRESULT hrReturn = pcicmui -> QueryInterface(riid, ppvInstance);

    if  (!*ppvInstance)
        delete  pcicmui;

    return  hrReturn;
}


//  Key to the factory is a static function that allows outsiders to instance
//  the class factory.  So, the caller will first instance the factory, then
//  instance implementations of the interfaces it needs using the factory
//  instance it receives from here.

HRESULT CIcmUiFactory::KeyToTheFactory(REFCLSID rclsid, REFIID riid,
                                       void **ppvObject) {
  
    *ppvObject = NULL;

    if  (!IsEqualIID(rclsid, CLSID_ICM) &&
         !IsEqualIID(rclsid, CLSID_MONITORUI) &&
         !IsEqualIID(rclsid, CLSID_SCANNERUI) &&
         !IsEqualIID(rclsid, CLSID_PRINTERUI))
        return  CLASS_E_CLASSNOTAVAILABLE;

    CIcmUiFactory   *pciuf = new CIcmUiFactory(rclsid);

    if  (!pciuf)
        return  E_OUTOFMEMORY;

    HRESULT hrReturn = pciuf -> QueryInterface(riid, ppvObject);

    if  (!*ppvObject)
        delete  pciuf;

    return  hrReturn;
}

/******************************************************************************

  ICM UI class methods- these do the true interface work of the DLL.

******************************************************************************/


CICMUserInterface::CICMUserInterface(UITYPE utThis) {
    m_lpdoTarget = NULL;
    m_ulcReferences = 0;
    m_utThis = utThis;
    CGlobals::Attach();
    _RPTF2(_CRT_WARN, "CICMUserInterface(%d) constructed @ %lX\n", utThis, this);
}
//  QueryInterface gets a bit long, but not too badly.  The casts are needed
//  because we use multiple inheritance- casting the this pointer to a base
//  class actually returns a this pointer for that base class' part of the
//  instance.  Unlike single inheritance, the this pointer for the
//  CICMUserInterface class does not directly reference ANY of the base
//  classes.

STDMETHODIMP    CICMUserInterface::QueryInterface(REFIID riid,
                                                  void **ppvObject) {
    *ppvObject = NULL;  //  Assume the worst
    //  Since the device UI support a different set of functions, let's be
    //  particular about which interfaces we claim to support when...
    if  (m_utThis > IsProfile) {
        if  (IsEqualIID(riid, IID_IUnknown) ||
                IsEqualIID(riid, IID_IShellExtInit))
            *ppvObject = (IShellExtInit *) this;
        if  (IsEqualIID(riid, IID_IShellPropSheetExt))
			   *ppvObject = (IShellPropSheetExt *) this;
    }
    else {
        if  (IsEqualIID(riid, IID_IUnknown) ||
                IsEqualIID(riid, IID_IContextMenu))
            *ppvObject = (IContextMenu *) this;

        if  (IsEqualIID(riid, IID_IShellExtInit))
            *ppvObject = (IShellExtInit *) this;

        if  (IsEqualIID(riid, IID_IExtractIcon))
            *ppvObject = (IExtractIcon *) this;

        if  (IsEqualIID(riid, IID_IPersistFile) ||
                IsEqualIID(riid, IID_IPersist))
            *ppvObject = (IPersistFile *) this;

        if  (IsEqualIID(riid, IID_IShellPropSheetExt))
            *ppvObject = (IShellPropSheetExt *) this;
    }

    if  (*ppvObject)
        ((IUnknown *) *ppvObject) -> AddRef();

    _RPTF2(_CRT_WARN, "CICMUserInterace::QueryInterface(%lX) returns %lX\n",
        this, ppvObject);

    return  *ppvObject ? NOERROR : E_NOINTERFACE;
}

//  IShellExtInit member function- this interface needs only one

STDMETHODIMP    CICMUserInterface::Initialize(LPCITEMIDLIST pcidlFolder,
                                              LPDATAOBJECT pdoTarget,
                                              HKEY hKeyID) {

    _RPTF0(_CRT_WARN, "CICMUserInterface::Initialize\n");

    //  The target data object is an HDROP, or list of files from the shell.

    if  (m_lpdoTarget) {
        m_lpdoTarget -> Release();
        m_lpdoTarget = NULL;
    }

    if  (pdoTarget) {
        m_lpdoTarget = pdoTarget;
        m_lpdoTarget -> AddRef();
    }

    return  NOERROR;
}

//  IExtractIcon interface functions- for now, we'll default to providing a
//  default icon from our DLL.  We provide one icon for installed profiles,
//  and a second for uninstalled ones.

STDMETHODIMP    CICMUserInterface::GetIconLocation(UINT uFlags,
                                                   LPTSTR lpstrTarget,
                                                   UINT uccTarget,
                                                   int *piIndex,
                                                   UINT *puFlags) {

    *puFlags = (GIL_NOTFILENAME|GIL_DONTCACHE); // Make shell call our Extract function
                                                // And don't cache in the callee.

    return S_FALSE;
}

STDMETHODIMP    CICMUserInterface::Extract(LPCTSTR lpstrFile, UINT nIconIndex,
                            HICON *phiconLarge, HICON *phiconSmall,
                            UINT nIconSize) {

    *phiconSmall = *phiconLarge = LoadIcon(CGlobals::Instance(),
        MAKEINTRESOURCE(CGlobals::IsInstalled(m_csFile) ? DefaultIcon : UninstalledIcon));

    return NOERROR;
}

//  IPersistFile functions- there's only one worth implementing

STDMETHODIMP    CICMUserInterface::Load(LPCOLESTR lpwstrFileName,
                                        DWORD dwMode) {
    //  This interface is used to initialize the icon handler- it will
    //  receive the profile name, which we will save for later use.
    //  The CString assigment operator handles any encoding converions needed
    //  encoding conversions for us.

    m_csFile = lpwstrFileName;

    return  m_csFile.IsEmpty() ? E_OUTOFMEMORY : NO_ERROR;
}

//  IContextMenu functions-

STDMETHODIMP    CICMUserInterface::QueryContextMenu(HMENU hMenu, UINT indexMenu,
                                                    UINT idCmdFirst, UINT idCmdLast,
                                                    UINT uFlags) {

    //  Only CMF_NORMAL and CMF_EXPLORE case will be handled.
    //
    //  CMF_CANRENAME     -  This flag is set if the calling application supports
    //                      renaming of items. A context menu extension or drag-and-drop
    //                      handler should ignore this flag. A namespace extension should
    //                      add a rename item to the menu if applicable.
    //  CMF_DEFAULTONLY   -  This flag is set when the user is activating the default action,
    //                      typically by double-clicking. This flag provides a hint for the
    //                      context menu extension to add nothing if it does not modify the
    //                      default item in the menu. A context menu extension or drag-and-drop
    //                      handler should not add any menu items if this value is specified.
    //                      A namespace extension should add only the default item (if any).
    //  CMF_EXPLORE       -  This flag is set when Windows Explorer's tree window is present.
    //                      Context menu handlers should ignore this value.
    //  CMF_INCLUDESTATIC -  This flag is set when a static menu is being constructed.
    //                      Only the browser should use this flag. All other context menu
    //                      extensions should ignore this flag.
    //  CMF_NODEFAULT     -  This flag is set if no item in the menu should be the default item.
    //                      A context menu extension or drag-and-drop handler should ignore this
    //                      flag. A namespace extension should not set any of the menu items to
    //                      the default.
    //  CMF_NORMAL        -  Indicates normal operation. A context menu extension, namespace extension,
    //                      or drag-and-drop handler can add all menu items.
    //  CMF_NOVERBS       -  This flag is set for items displayed in the "Send To:" menu.
    //                      Context menu handlers should ignore this value.
    //  CMF_VERBSONLY     -  This flag is set if the context menu is for a shortcut object.
    //                      Context menu handlers should ignore this value.

    if (((uFlags & 0x000F) == CMF_NORMAL) || (uFlags & CMF_EXPLORE))
    {
        //
        //  Load the profile(s) in the list.
        //
        FORMATETC       fmte = {CF_HDROP, (DVTARGETDEVICE FAR *)NULL,
                                DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM       stgm;
        HRESULT         hres = m_lpdoTarget ?
                               m_lpdoTarget -> GetData(&fmte, &stgm) : 0;

        if  (!SUCCEEDED(hres))
            return  NOERROR;    //  Why bother reporting a failure, here?

        UINT    ucFiles = stgm.hGlobal ?
            DragQueryFile((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

        if  (!ucFiles)
            return  NOERROR;    //  Shouldn't happen, but it's not important
        else if (ucFiles == 1)
            m_bMultiSelection = FALSE;
        else
            m_bMultiSelection = TRUE;

        // Assume in installed context, but we will scan the selected item
        // is really installed everything.

        m_bInstalledContext = TRUE;

        TCHAR   acFile[_MAX_PATH];

        for (UINT u = 0; u < ucFiles; u++) {

            DragQueryFile((HDROP) stgm.hGlobal, u, acFile,
                sizeof acFile/ sizeof acFile[0]);

            CString csFile = acFile;

            m_bInstalledContext = (m_bInstalledContext && CGlobals::IsInstalled(csFile));
        }

        UINT idCmd = idCmdFirst;

        CString csInstallMenu, csAssociateMenu;

        //  If every profile(s) are already installed on this system,
        //  display "Uninstall Profile", otherwise display "Install Profile"

        csInstallMenu.Load(m_bInstalledContext ? UninstallProfileMenuString : InstallProfileMenuString);
        ::InsertMenu(hMenu,indexMenu,MF_STRING|MF_BYPOSITION,idCmd,csInstallMenu);

        //  Set "Install Profile" or "Uninstall Profile" as default.

        SetMenuDefaultItem(hMenu,indexMenu,TRUE);

        //  Increment Menu pos. and item id.

        indexMenu++; idCmd++;

        //  Add "Associate..." menu item

        csAssociateMenu.Load(AssociateMenuString);
        ::InsertMenu(hMenu,indexMenu++,MF_STRING|MF_BYPOSITION,idCmd++,csAssociateMenu);

        //  But if we have multi selection, disable "Associate..."

        if (m_bMultiSelection)
            ::EnableMenuItem(hMenu,(idCmd-1),MF_GRAYED);
        return (idCmd - idCmdFirst); // return number of menu inserted.
    }

    return NOERROR;
}

STDMETHODIMP    CICMUserInterface::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) {

    //  If HIWORD(lpcmi->lpVerb) then we have been called programmatically and
    //  lpVerb us a command that should be invoked. Otherwise, the shell has
    //  called us, abd LOWORD(lpcmi->lpVerb) is the menu ID the user has selected.
    //  Actually, it's (menu ID - icmdFirst) from QueryContextMenu().

    if (!HIWORD((ULONG)(ULONG_PTR)lpcmi->lpVerb))  {

        FORMATETC       fmte = {CF_HDROP, (DVTARGETDEVICE FAR *)NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM       stgm;
        HRESULT         hres = m_lpdoTarget ?
            m_lpdoTarget -> GetData(&fmte, &stgm) : 0;

        if  (!SUCCEEDED(hres))
            return  NOERROR;    //  Why bother reporting a failure, here?

        UINT    ucFiles = stgm.hGlobal ?
            DragQueryFile((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

        if  (!ucFiles)
            return  NOERROR;    //  Shouldn't happen, but it's not important

        UINT idCmd = LOWORD(lpcmi->lpVerb);

        // Walk through every selected item to install/uninstall.

        for (UINT u = 0; u < ucFiles; u++) {

            TCHAR   acFile[_MAX_PATH];

            DragQueryFile((HDROP) stgm.hGlobal, u, acFile,
                sizeof acFile/ sizeof acFile[0]);

            switch (idCmd) {

                case    0: {   // Install/Uninstall was selected.

                    // during the installation or un-installation,
                    // change the cursor icon to IDC_APPSTARTING.

                    HCURSOR hCursorOld = SetCursor(LoadCursor(NULL,IDC_APPSTARTING));

                    CProfile csProfile(acFile);

                    if (m_bInstalledContext) {

                        // All selected profile is already installed, then
                        // Uninstall every profile(s) are selected if installed.

                        if (csProfile.IsInstalled()) {
                            csProfile.Uninstall(FALSE); // never delete file from disk.
                        }
                    }
                    else {

                        // Some of selected profile is not installed, then
                        // Install every profile(s) are selected if not installed, yet.

                        if (!csProfile.IsInstalled()) {
                            csProfile.Install();
                        }
                    }

                    SetCursor(hCursorOld);

                    break;
                }

                case    1: {   // "Associate..." was selected.

                    CString csProfileName;

                    // Get profile "friendly" name.
                    {
                        CProfile csProfile(acFile);
                        csProfileName = csProfile.GetName();
                    } // de-constructer for csProfile should be here.

                    // Create PropertySheet with "Profile Information" and
                    // "Associate Device" pages

                    PROPSHEETHEADER psh;
                    HPROPSHEETPAGE  hpsp[2];

                    CProfileInformationPage *pcpip =                       
                        new CProfileInformationPage(CGlobals::Instance(), acFile);
                    CProfileAssociationPage *pcpap =
                        new CProfileAssociationPage(CGlobals::Instance(), acFile);
                    if( (pcpip!=NULL)&&(pcpap!=NULL) ) {    
                        hpsp[0] = pcpip->Handle();
                        hpsp[1] = pcpap->Handle();
    
                        ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    
                        // fill the property sheet structure.
    
                        psh.dwSize = sizeof(PROPSHEETHEADER);
                        psh.hInstance = CGlobals::Instance();
                        psh.hwndParent = NULL;
                        psh.nStartPage = 1; // Active "Associate Device" page.
                        psh.nPages = 2;
                        psh.phpage = hpsp;
                        psh.pszCaption = csProfileName;
    
                        PropertySheet(&psh);
    
                        delete pcpip; delete pcpap;
                        break;
                    } else {
                      if(pcpip) delete pcpip;
                      if(pcpap) delete pcpap;
                      return E_OUTOFMEMORY;
                    }
                }
            } // switch (idCmd)
        } // for (UINT u = 0; u < ucFiles; u++)
    } // if (!HIWORD(lpcmi->lpVerb))

    return NOERROR;
}

/* Supprisingly the code casts the unicode string to an
 * asciiz string and passes it. One assumes that nobody
 * actually interprets the string pointer as ascii on the
 * way to its destination where it is reinterpreted
 * as a pointer to a unicode string.
 */
STDMETHODIMP    CICMUserInterface::GetCommandString(UINT_PTR idCmd, UINT uFlags,
                                                    UINT FAR *reserved, LPSTR pszName,
                                                    UINT cchMax) {
    CString csReturnString;

    switch (idCmd) {
        case    0: {   // Install/Uninstall was selected.
          if(m_bMultiSelection) {
            csReturnString.Load(m_bInstalledContext ? UninstallMultiProfileContextMenuString : InstallMultiProfileContextMenuString);
          } else {
            csReturnString.Load(m_bInstalledContext ? UninstallProfileContextMenuString : InstallProfileContextMenuString);
          }
          lstrcpyn((LPTSTR)pszName, csReturnString, cchMax);
          break;
        }

        case    1: {   // Associate... was seleted.
          if (!m_bMultiSelection) {
            csReturnString.Load(AssociateContextMenuString);
            lstrcpyn((LPTSTR)pszName, csReturnString, cchMax);
          }
          break;
        }
    }

    return NOERROR;
}

//  IPropSheetExt functions- again, we only need to implement one of these
//  Since we now support two different interfaces, the actual implementation
//  is done in a private method germane to the desired interface.

STDMETHODIMP    CICMUserInterface::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                            LPARAM lParam) {
    _RPTF0(_CRT_WARN, "CICMUserInterface::AddPages\n");

    HRESULT hResult = NOERROR;
    
    switch  (m_utThis) {
        case    IsProfile: {
            hResult = AddProfileTab(lpfnAddPage, lParam);
            if (hResult == NOERROR) {
                hResult = AddAssociateTab(lpfnAddPage, lParam);
            }
            break;
        }

        case    IsMonitor: {
            hResult = AddMonitorTab(lpfnAddPage, lParam);
            break;
        }

        case    IsPrinter: {
            hResult = AddPrinterTab(lpfnAddPage, lParam);
            break;
        }

        case    IsScanner: {
            hResult = AddScannerTab(lpfnAddPage, lParam);
            break;
        }
    }

    return  hResult;
}

//  This member function handles the ICC profile information sheet.
//  In this case, the data object given via IShellExtInit::Initialize is
//  an HDROP (list of fully qualified file names).

HRESULT CICMUserInterface::AddProfileTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                        LPARAM lParam) {
    _RPTF0(_CRT_WARN, "CICMUserInterface::AddProfileTab\n");

    //  Load the profile(s) in the list.

    FORMATETC       fmte = {CF_HDROP, (DVTARGETDEVICE FAR *)NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM       stgm;
    HRESULT         hres = m_lpdoTarget ?
        m_lpdoTarget -> GetData(&fmte, &stgm) : 0;


    if  (!SUCCEEDED(hres))
        return  NOERROR;    //  Why bother reporting a failure, here?

    UINT    ucFiles = stgm.hGlobal ?
        DragQueryFile((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  (ucFiles != 1)
        return  NOERROR;

    TCHAR   acFile[_MAX_PATH];

    DragQueryFile((HDROP) stgm.hGlobal, 0, acFile,
        sizeof acFile/ sizeof acFile[0]);

    //  Create the property sheet- it will get deleted if it is not in
    //  use when the shell tries to unload the extension

    CProfileInformationPage *pcpip =
        new CProfileInformationPage(CGlobals::Instance(), acFile);

    if  (!(*lpfnAddPage)(pcpip -> Handle(), lParam))
        DestroyPropertySheetPage(pcpip -> Handle());

    return  NOERROR;
}

//  This member function handles the associate device tab

HRESULT CICMUserInterface::AddAssociateTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                           LPARAM lParam) {

    _RPTF0(_CRT_WARN, "CICMUserInterface::AddAssociateTab\n");

    //  Load the profile(s) in the list.

    FORMATETC       fmte = {CF_HDROP, (DVTARGETDEVICE FAR *)NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM       stgm;
    HRESULT         hres = m_lpdoTarget ?
        m_lpdoTarget -> GetData(&fmte, &stgm) : 0;

    if  (!SUCCEEDED(hres))
        return  NOERROR;    //  Why bother reporting a failure, here?

    UINT    ucFiles = stgm.hGlobal ?
        DragQueryFile((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  (ucFiles != 1)
        return  NOERROR;

    TCHAR   acFile[_MAX_PATH];

    DragQueryFile((HDROP) stgm.hGlobal, 0, acFile,
        sizeof acFile/ sizeof acFile[0]);

    //  Create the property sheet- it will get deleted if it is not in
    //  use when the shell tries to unload the extension

    CProfileAssociationPage *pcpap =
        new CProfileAssociationPage(CGlobals::Instance(), acFile);

    if  (!(*lpfnAddPage)(pcpap -> Handle(), lParam))
        DestroyPropertySheetPage(pcpap -> Handle());

    return  NOERROR;
}

//  This member function handles the monitor color management tab
//  In this case, no data object is given.

//  Private monitor enumeration function

HRESULT CICMUserInterface::AddMonitorTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                         LPARAM lParam) {

    //  Create the property sheet- it will get deleted if it is not in
    //  use when the shell tries to unload the extension

    CString csMonitorDevice;
    CString csMonitorFriendlyName;

    if (m_lpdoTarget) {

        FORMATETC fmte = { (CLIPFORMAT)RegisterClipboardFormat(_TEXT("Display Device")),
                           (DVTARGETDEVICE FAR *) NULL,
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;

        // Get device name from IDataObject.

        HRESULT   hres = m_lpdoTarget -> GetData(&fmte, &stgm);

        if  (!SUCCEEDED(hres) || !stgm.hGlobal) {
            return  NOERROR;    //  Why bother reporting a failure, here?
        }

        // The storage contains Display device path (\\.\DisplayX) in UNICODE.

        LPCWSTR lpDeviceName = (LPCWSTR) GlobalLock(stgm.hGlobal);
        CString csMonitorDevicePath = lpDeviceName;
        GlobalUnlock(stgm.hGlobal);

        // Query the device id, friendly name and other on the display device.

        DISPLAY_DEVICE ddPriv;

        ddPriv.cb = sizeof(ddPriv);

        if (!EnumDisplayDevices((LPCTSTR)csMonitorDevicePath, 0, &ddPriv, 0))
        {
            return  NOERROR;    //  Why bother reporting a failure, here?
        }

        #if HIDEYUKN_DBG
            MessageBox(NULL,csMonitorDevicePath,TEXT(""),MB_OK);
            MessageBox(NULL,(LPCTSTR)ddPriv.DeviceID,TEXT(""),MB_OK);
            MessageBox(NULL,(LPCTSTR)ddPriv.DeviceString,TEXT(""),MB_OK);
        #endif

        // Use deviceId (PnP Id) as device name, and set friendly name

        csMonitorDevice       = (LPTSTR)(ddPriv.DeviceID);
        csMonitorFriendlyName = (LPTSTR)(ddPriv.DeviceString);
    }
    else
    {
        // if we don't have IDataObject, enumerate monitor,
        // then use 1st entry.

        CMonitorList    cml;
        cml.Enumerate();
        _ASSERTE(cml.Count());  // At least, we should have one Monitor.
        csMonitorDevice = csMonitorFriendlyName = cml.DeviceName(0);
    }

    CMonitorProfileManagement *pcmpm =
        new CMonitorProfileManagement(csMonitorDevice,
                                      csMonitorFriendlyName,
                                      CGlobals::Instance());


    if  (!(*lpfnAddPage)(pcmpm -> Handle(), lParam))
        DestroyPropertySheetPage(pcmpm -> Handle());

    return  NOERROR;
}

//  Private scanner enumeration function

HRESULT CICMUserInterface::AddScannerTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                         LPARAM lParam) {

    //  Create the property sheet- it will get deleted if it is not in
    //  use when the shell tries to unload the extension

    CString csScannerDevice;

    if (m_lpdoTarget) {
        FORMATETC fmte = { (CLIPFORMAT)RegisterClipboardFormat(_TEXT("STIDeviceName")),
                           (DVTARGETDEVICE FAR *) NULL,
                           DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;

        // Get device name from IDataObject.

        HRESULT   hres = m_lpdoTarget -> GetData(&fmte, &stgm);

        if  (!SUCCEEDED(hres) || !stgm.hGlobal) {
            return  NOERROR;    //  Why bother reporting a failure, here?
        }

        // The storage contains Scanner in UNICODE string.

        LPCWSTR lpDeviceName = (LPCWSTR) GlobalLock(stgm.hGlobal);
        csScannerDevice = lpDeviceName;
        GlobalUnlock(stgm.hGlobal);

        #if HIDEYUKN_DBG
            MessageBox(NULL,csScannerDevice,TEXT(""),MB_OK);
        #endif

    } else {

        // if we don't have IDataObject, enumerate monitor,
        // then use 1st entry.

        CScannerList csl;
        csl.Enumerate();
        _ASSERTE(csl.Count());
        csScannerDevice = csl.DeviceName(0);
    }

    CScannerProfileManagement *pcspm =
        new CScannerProfileManagement(csScannerDevice, CGlobals::Instance());

    if  (!(*lpfnAddPage)(pcspm -> Handle(), lParam))
        DestroyPropertySheetPage(pcspm -> Handle());

    return  NOERROR;
}

//  The following is a helper function- it takes a Shell ID List array,
//  representing a printer in a printers folder, and a CString.  It
//  initializes the CString with the correct name of the printer.

static void RetrievePrinterName(LPIDA lpida, CString& csTarget) {

    //  Extract the container (Printers Folder) and target (Printer)
    //  IDs from the array.

    LPCITEMIDLIST pciilContainer =
        (LPCITEMIDLIST)((LPBYTE) lpida + lpida -> aoffset[0]);

    LPCITEMIDLIST pciilTarget =
        (LPCITEMIDLIST)((LPBYTE) lpida + lpida -> aoffset[1]);

    if  (!pciilContainer || !pciilTarget)
        return;

    //  Get a pointer to the printers folder.

    LPSHELLFOLDER   psfDesktop, psfPrinterFolder;

    if  (FAILED(SHGetDesktopFolder(&psfDesktop)))
        return;

    if  (FAILED(psfDesktop -> BindToObject(pciilContainer, NULL,
            IID_IShellFolder, (void **) &psfPrinterFolder))) {
        psfDesktop -> Release();
        return;
    }

    //  Retrieve the printer's display name

    STRRET  strret;

    if  (FAILED(psfPrinterFolder ->
            GetDisplayNameOf(pciilTarget, SHGDN_FORPARSING, &strret))) {
        psfPrinterFolder -> Release();
        psfDesktop -> Release();
        return;
    }

    //  Copy the display name- the CString class now handles any encoding
    //  issues

    switch  (strret.uType) {

        case    STRRET_WSTR:

            //  This is a Unicode string which was IMalloc'd

            csTarget = strret.pOleStr;

            IMalloc *pim;

            if  (SUCCEEDED(CoGetMalloc(1, &pim))) {
                pim -> Free(strret.pOleStr);
                pim -> Release();
            }

            break;

        case    STRRET_CSTR:

            //  This is an ANSI string in the buffer

            csTarget = strret.cStr;
            break;

        case    STRRET_OFFSET:

            //  This is an ANSI string at the given offset into the SHITEMID
            //  which pciilTarget points to.

            csTarget = (LPCSTR) pciilTarget + strret.uOffset;

    }
    psfPrinterFolder -> Release();
    psfDesktop -> Release();
}

//  Private member function for handling the printer profile manamgment tab.

HRESULT CICMUserInterface::AddPrinterTab(LPFNADDPROPSHEETPAGE lpfnAddPage,
                                         LPARAM lParam) {

    //  The list is formatted as a Shell IDList Array

    FORMATETC       fmte = { (CLIPFORMAT)RegisterClipboardFormat(_TEXT("Shell IDList Array")),
                             (DVTARGETDEVICE FAR *) NULL,
                             DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM       stgm;
    HRESULT         hres = m_lpdoTarget ?
        m_lpdoTarget -> GetData(&fmte, &stgm) : 0;

    if  (!SUCCEEDED(hres) || !stgm.hGlobal)
        return  NOERROR;    //  Why bother reporting a failure, here?

    CString csPrinter;

    RetrievePrinterName((LPIDA) stgm.hGlobal, csPrinter);

    #if HIDEYUKN_DBG
        MessageBox(NULL,csPrinter,TEXT(""),MB_OK);
    #endif

    //  If this is not a color printer, forget it...


    if  (!CGlobals::ThisIsAColorPrinter(csPrinter))
        return  NOERROR;

    //  Create the property sheet- it will get deleted if it is not in use when
    //  the shell tries to unload the extension


    CPrinterProfileManagement *pcppm =
            new CPrinterProfileManagement(csPrinter, CGlobals::Instance());

    if  (!pcppm)
        return  E_OUTOFMEMORY;

    if  (!(*lpfnAddPage)(pcppm -> Handle(), lParam))
        DestroyPropertySheetPage(pcppm -> Handle());

    return  NOERROR;
}

PSTR
GetFilenameFromPath(
    PSTR pPathName
    )
{
    DWORD dwLen;                    // length of pathname

    dwLen = lstrlenA(pPathName);

    //
    // Go to the end of the pathname, and start going backwards till
    // you reach the beginning or a backslash
    //

    pPathName += dwLen;

    while (dwLen-- && --pPathName)
    {
        if (*pPathName == '\\')
        {
            pPathName++;
            break;
        }
    }

    //
    // if *pPathName is zero, then we had a string that ends in a backslash
    //

    return *pPathName ? pPathName : NULL;
}
