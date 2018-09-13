/******************************************************************************

  Source File:  ICC Profile.CPP

  This implements the class we use to encapsulate everything we will ever care
  to know about a profile, including the classes we need to support
  associations and the like.

  Copyright (c) 1996, 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:

  10-31-96  A-RobKj (Pretty Penny Enterprises) began encapsulating it
  12-04-96  A-RobKj Added the CProfileArray and CAllDeviceList classes
  12-13-96  A-RobKj Modified for faster operation (more lazy evaluation,
                    and common DLL-wide database for installation checks)
                    Also moved CDeviceList derived classes to the header, so
                    I can use them other places, as well...
  01-07-97  KjelgaardR@acm.org  Fixed CProfileArray::Empty- wasn't setting Next
            object pointer to NULL after deleting said object (Fixed GP fault).
  01-08-97  KjelgaardR@acm.org  Modified printer enumeration routine to only
            enumerate color models (uses Global utility function).

******************************************************************************/

#include    "ICMUI.H"
#include    <shlobj.h>
#include    "shellext.h"
#include    "..\mscms\sti.h"

typedef HRESULT (__stdcall *PFNSTICREATEINSTANCE)(HINSTANCE, DWORD, PSTI*, LPDWORD);

TCHAR  gszStiDll[]             = __TEXT("sti.dll");
char   gszStiCreateInstance[]  = "StiCreateInstance";

//  Printer DeviceEnumeration method

void    CPrinterList::Enumerate() {

#if !defined(_WIN95_) // CPrinterList::Enumetate()

    //  Enumerate all local printers

    DWORD   dwcNeeded, dwcReturned;
    EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 4, NULL, 0, &dwcNeeded,
        &dwcReturned);

    union {
        PBYTE   pBuff;
        PPRINTER_INFO_4 ppi4;
    };

    pBuff = new BYTE[dwcNeeded];

    while   (pBuff && !EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 4, pBuff,
        dwcNeeded, &dwcNeeded, &dwcReturned) &&
        GetLastError() == ERROR_MORE_DATA) {
        delete pBuff;
        pBuff = new BYTE[dwcNeeded];
    }

    if  (pBuff) {

        for (unsigned u = 0; u < dwcReturned; u++)
            if  (CGlobals::ThisIsAColorPrinter(ppi4[u].pPrinterName)) {
                m_csaDeviceNames.Add(ppi4[u].pPrinterName);
                m_csaDisplayNames.Add(ppi4[u].pPrinterName);
            }

        delete  pBuff;
    }

    //  Now, enumerate all the connected printers

    EnumPrinters(PRINTER_ENUM_CONNECTIONS, NULL, 4, NULL, 0, &dwcNeeded,
        &dwcReturned);

    pBuff = new BYTE[dwcNeeded];

    while   (pBuff && !EnumPrinters(PRINTER_ENUM_CONNECTIONS, NULL, 4, pBuff,
        dwcNeeded, &dwcNeeded, &dwcReturned) &&
        GetLastError() == ERROR_MORE_DATA) {
        delete pBuff;
        pBuff = new BYTE[dwcNeeded];
    }

    if  (!pBuff)
        return;

    for (unsigned u = 0; u < dwcReturned; u++)
        if  (CGlobals::ThisIsAColorPrinter(ppi4[u].pPrinterName)) {
            m_csaDeviceNames.Add(ppi4[u].pPrinterName);
            m_csaDisplayNames.Add(ppi4[u].pPrinterName);
        }

    delete  pBuff;

#else 

    //  Enumerate all local printers

    DWORD   dwcNeeded, dwcReturned;
    EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &dwcNeeded,
        &dwcReturned);

    union {
        PBYTE   pBuff;
        PPRINTER_INFO_5 ppi5;
    };

    pBuff = new BYTE[dwcNeeded];

    while   (pBuff && !EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, pBuff,
        dwcNeeded, &dwcNeeded, &dwcReturned) &&
        GetLastError() == ERROR_MORE_DATA) {
        delete pBuff;
        pBuff = new BYTE[dwcNeeded];
    }

    if  (pBuff) {

        for (unsigned u = 0; u < dwcReturned; u++) {
            if  (CGlobals::ThisIsAColorPrinter(ppi5[u].pPrinterName)) {
                m_csaDeviceNames.Add(ppi5[u].pPrinterName);
                m_csaDisplayNames.Add(ppi5[u].pPrinterName);
            }
        }

        delete  pBuff;
    }
#endif
}

//  Printer Name Validity Check

BOOL    CPrinterList::IsValidDeviceName(LPCTSTR lpstrRef) {

    if  (!lpstrRef) return  FALSE;

    if  (!Count())
        Enumerate();

    for (unsigned u = 0; u < Count(); u++)
        if  (!lstrcmpi(m_csaDeviceNames[u], lpstrRef))
            break;

    return  u < Count();
}

//  Private monitor enumeration function- note this is ANSI only...

extern "C" BOOL WINAPI  EnumerateMonitors(LPBYTE pBuffer, PDWORD pdwcbNeeded,
                                          PDWORD pdwcReturned);

//  CMonitor class enumerator

void    CMonitorList::Enumerate() {

    ULONG          ulSerialNumber = 1;
    ULONG          ulDeviceIndex  = 0;
    DISPLAY_DEVICE ddPriv;

    ddPriv.cb = sizeof(ddPriv);

    // Enumurate display adaptor on the system.

    while (EnumDisplayDevices(NULL, ulDeviceIndex, &ddPriv, 0))
    {
        ULONG          ulMonitorIndex = 0;
        DISPLAY_DEVICE ddPrivMonitor;

        ddPrivMonitor.cb = sizeof(ddPrivMonitor);

        // then, enumurate monitor device, attached the display adaptor.

        while (EnumDisplayDevices(ddPriv.DeviceName, ulMonitorIndex, &ddPrivMonitor, 0))
        {
            TCHAR DisplayNameBuf[256]; // number: devicename - 256 is good enough.

            // Insert PnP id as device name.

            m_csaDeviceNames.Add(ddPrivMonitor.DeviceID);

            // If this is primary display device, remember it.

            if (ddPriv.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                m_csPrimaryDeviceName = ddPrivMonitor.DeviceID;
            }

            // Build display name.

            wsprintf(DisplayNameBuf,TEXT("%d. %s"),ulSerialNumber,ddPrivMonitor.DeviceString);
            m_csaDisplayNames.Add(DisplayNameBuf);

            ulMonitorIndex++;
            ulSerialNumber++;
            ddPrivMonitor.cb = sizeof(ddPrivMonitor);
        }

        ulDeviceIndex++;
        ddPriv.cb = sizeof(ddPriv);
    }
}

//  Monitor Name Validity Check

BOOL    CMonitorList::IsValidDeviceName(LPCTSTR lpstrRef) {

    if  (!lpstrRef) return  FALSE;

    if  (!Count())
        Enumerate();

    for (unsigned u = 0; u < Count(); u++)
        if  (!lstrcmpi(m_csaDeviceNames[u], lpstrRef))
            break;

    return  u < Count();
}

LPCSTR  CMonitorList::DeviceNameToDisplayName(LPCTSTR lpstrRef) {

    if  (!lpstrRef) return NULL;

    if  (!Count())
        Enumerate();

    for (unsigned u = 0; u < Count(); u++)
        if  (!lstrcmpi(m_csaDeviceNames[u], lpstrRef))
            return (LPCSTR)(m_csaDisplayNames[u]);

    return NULL;
}  

//  Scanner DeviceEnumeration method

void    CScannerList::Enumerate() {

    PFNSTICREATEINSTANCE    pStiCreateInstance;
    PSTI                    pSti = NULL;
    PSTI_DEVICE_INFORMATION pDevInfo;
    PVOID                   pBuffer = NULL;
    HINSTANCE               hModule;
    HRESULT                 hres;
    DWORD                   i, dwItemsReturned;
    #ifndef UNICODE
    char                    szName[256];
    #endif

    if (!(hModule = LoadLibrary(gszStiDll)))
    {
        _RPTF1(_CRT_WARN, "Error loading sti.dll: %d\n",
               GetLastError());
        return;
    }

    if (!(pStiCreateInstance = (PFNSTICREATEINSTANCE)GetProcAddress(hModule, gszStiCreateInstance)))
    {
        _RPTF0(_CRT_WARN, "Error getting proc StiCreateInstance\n");
        goto EndEnumerate;
    }

    hres = (*pStiCreateInstance)(GetModuleHandle(NULL), STI_VERSION, &pSti, NULL);

    if (FAILED(hres))
    {
        _RPTF1(_CRT_WARN, "Error creating sti instance: %d\n", hres);
        goto EndEnumerate;
    }

    hres = pSti->GetDeviceList(0, 0, &dwItemsReturned, &pBuffer);

    if (FAILED(hres) || !pBuffer)
    {
        _RPTF0(_CRT_WARN, "Error getting scanner devices\n");
        goto EndEnumerate;
    }

    pDevInfo = (PSTI_DEVICE_INFORMATION) pBuffer;

    for (i=0; i<dwItemsReturned; i++, pDevInfo++)
    {
        #ifndef UNICODE
        DWORD dwLen;                    // length of Ansi string
        BOOL  bUsedDefaultChar;

        dwLen = (lstrlenW(pDevInfo->pszLocalName) + 1) * sizeof(char);

        //
        // Convert Unicode name to Ansi
        //
        if (WideCharToMultiByte(CP_ACP, 0, pDevInfo->szDeviceInternalName, -1, szName,
              dwLen, NULL, &bUsedDefaultChar) && ! bUsedDefaultChar)
        {
            m_csaDeviceNames.Add(szName);
        }
        else
        {
            _RPTF0(_CRT_WARN, "Error converting internalName to Unicode name\n");
        }

        if (WideCharToMultiByte(CP_ACP, 0, pDevInfo->pszLocalName, -1, szName,
              dwLen, NULL, &bUsedDefaultChar) && ! bUsedDefaultChar)
        {
            m_csaDisplayNames.Add(szName);
        }
        else
        {
            _RPTF0(_CRT_WARN, "Error converting deviceName to Unicode name\n");
        }

        #else
        m_csaDeviceNames.Add(pDevInfo->szDeviceInternalName);
        m_csaDisplayNames.Add(pDevInfo->pszLocalName);
        #endif
    }

EndEnumerate:
    if (pBuffer)
    {
        LocalFree(pBuffer);
    }

    if (pSti)
    {
        pSti->Release();
    }

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return;
}

//  Scanner Name Validity Check

BOOL    CScannerList::IsValidDeviceName(LPCTSTR lpstrRef) {

    if  (!lpstrRef) return  FALSE;

    if  (!Count())
        Enumerate();

    for (unsigned u = 0; u < Count(); u++)
        if  (!lstrcmpi(m_csaDeviceNames[u], lpstrRef))
            break;

    return  u < Count();
}

//  CAllDeviceList class enumerator

void    CAllDeviceList::Enumerate() {

    CMonitorList    cml;
    CPrinterList    cpl;
    CScannerList    csl;

    cml.Enumerate();
    cpl.Enumerate();
    csl.Enumerate();

    for (unsigned u = 0; u < cpl.Count(); u++) {
        m_csaDeviceNames.Add(cpl.DeviceName(u));
        m_csaDisplayNames.Add(cpl.DisplayName(u));
    }

    for (u = 0; u < cml.Count(); u++) {
        m_csaDeviceNames.Add(cml.DeviceName(u));
        m_csaDisplayNames.Add(cml.DisplayName(u));
    }

    for (u = 0; u < csl.Count(); u++) {
        m_csaDeviceNames.Add(csl.DeviceName(u));
        m_csaDisplayNames.Add(csl.DisplayName(u));
    }
}

//  Device Name Validity Check

BOOL    CAllDeviceList::IsValidDeviceName(LPCTSTR lpstrRef) {

    if  (!lpstrRef) return  FALSE;

    if  (!Count())
        Enumerate();

    for (unsigned u = 0; u < Count(); u++)
        if  (!lstrcmpi(m_csaDeviceNames[u], lpstrRef))
            break;

    return  u < Count();
}

//  CProfile member functions

//  The following static functions fills the appropriate array using the
//  profiles that match the search criteria goven.

void    CProfile::Enumerate(ENUMTYPE& et, CStringArray& csaList) {

    //  Enumerate the existing profiles

    DWORD   dwBuffer =0, dwcProfiles;

    csaList.Empty();

    EnumColorProfiles(NULL, &et, NULL, &dwBuffer, &dwcProfiles);

    if  (!dwBuffer) {
        _RPTF2(_CRT_WARN,
            "CProfile::Enumerate(String)- empty list- dwBuffer %d Error %d\n",
            dwBuffer, GetLastError());
        return;
    }

    union {
        PBYTE   pbBuffer;
        PTSTR   pstrBuffer;
    };

    pbBuffer = new BYTE[dwBuffer];

    if (pbBuffer) {

        if  (EnumColorProfiles(NULL, &et, pbBuffer, &dwBuffer, &dwcProfiles)) {
            for (PTSTR pstrMe = pstrBuffer;
                 dwcProfiles--;
                 pstrMe += 1 + lstrlen(pstrMe)) {
                _RPTF1(_CRT_WARN, "CProfile::Enumerate(String) %s found\n",
                    pstrMe);
                csaList.Add(pstrMe);
            }
        }

        delete  pbBuffer;
    }
}

void    CProfile::Enumerate(ENUMTYPE& et, CStringArray& csaList, CStringArray& csaDesc) {

    //  Enumerate the existing profiles

    DWORD   dwBuffer =0, dwcProfiles;

    csaList.Empty();

    EnumColorProfiles(NULL, &et, NULL, &dwBuffer, &dwcProfiles);

    if  (!dwBuffer) {
        _RPTF2(_CRT_WARN,
            "CProfile::Enumerate(String)- empty list- dwBuffer %d Error %d\n",
            dwBuffer, GetLastError());
        return;
    }

    union {
        PBYTE   pbBuffer;
        PTSTR   pstrBuffer;
    };

    pbBuffer = new BYTE[dwBuffer];

    if (pbBuffer) {

        if  (EnumColorProfiles(NULL, &et, pbBuffer, &dwBuffer, &dwcProfiles)) {
            for (PTSTR pstrMe = pstrBuffer;
                 dwcProfiles--;
                 pstrMe += 1 + lstrlen(pstrMe)) {
                _RPTF1(_CRT_WARN, "CProfile::Enumerate(String) %s found\n",
                    pstrMe);

                CProfile cp(pstrMe);

                if (cp.IsValid()) {

                    CString csDescription = cp.TagContents('desc', 4);

                    if (csDescription.IsEmpty()) {
                        csaDesc.Add(pstrMe);
                    } else {
                        csaDesc.Add((LPTSTR)csDescription);
                    }

                    csaList.Add(pstrMe);
                }
            }
        }

        delete  pbBuffer;
    }
}

void    CProfile::Enumerate(ENUMTYPE& et, CProfileArray& cpaList) {

    //  Enumerate the existing profiles

    DWORD   dwBuffer = 0, dwcProfiles;

    cpaList.Empty();

    EnumColorProfiles(NULL, &et, NULL, &dwBuffer, &dwcProfiles);

    if  (!dwBuffer) {
        _RPTF2(_CRT_WARN,
            "CProfile::Enumerate(Profile)- empty list- dwBuffer %d Error %d\n",
            dwBuffer, GetLastError());
        return;
    }

    union {
        PBYTE   pbBuffer;
        PTSTR   pstrBuffer;
    };

    pbBuffer = new BYTE[dwBuffer];

    if (pbBuffer) {

        if  (EnumColorProfiles(NULL, &et, pbBuffer, &dwBuffer, &dwcProfiles)) {
            for (PTSTR pstrMe = pstrBuffer;
                 dwcProfiles--;
                 pstrMe += 1 + lstrlen(pstrMe)) {
                _RPTF1(_CRT_WARN, "CProfile::Enumerate(Profile) %s added\n",
                    pstrMe);
                cpaList.Add(pstrMe);
            }
        }

        delete  pbBuffer;
    }

}

//  This retrieves the color directory name.  Since it is a const, we whouldn't
//  be calling it too often...

const CString   CProfile::ColorDirectory() {
    TCHAR   acDirectory[MAX_PATH];
    DWORD   dwccDir = MAX_PATH;

    GetColorDirectory(NULL, acDirectory, &dwccDir);

    return  acDirectory;
}

//  This checks for profile installation

void    CProfile::InstallCheck() {

    //  Enumerate the existing profiles, so we can see if this one's been
    //  installed, already.

    ENUMTYPE    et = {sizeof (ENUMTYPE), ENUM_TYPE_VERSION, 0, NULL};

    CStringArray    csaWork;

    Enumerate(et, csaWork);

    for (unsigned u = 0; u < csaWork.Count(); u++)
        if  (!lstrcmpi(csaWork[u].NameOnly(), m_csName.NameOnly()))
            break;

    m_bIsInstalled = u < csaWork.Count();
    m_bInstallChecked = TRUE;
}

//  This Checks for Associated Devices

void    CProfile::AssociationCheck() {

    m_bAssociationsChecked = TRUE;

    //  If the profile isn't installed, associations are moot...

    if  (!IsInstalled())
        return;

    //  The final step is to build a list of associations

    ENUMTYPE        et = {sizeof (ENUMTYPE), ENUM_TYPE_VERSION, ET_DEVICENAME};
    CStringArray    csaWork;

    for (unsigned u = 0; u < DeviceCount(); u++) {

        et.pDeviceName = m_pcdlClass -> DeviceName(u);

        Enumerate(et, csaWork);

        //  We track associations by index into the total device list...

        for (unsigned uProfile = 0; uProfile < csaWork.Count(); uProfile++)
            if  (!lstrcmpi(csaWork[uProfile].NameOnly(), m_csName.NameOnly())){
                m_cuaAssociation.Add(u);    //  Found one!
                break;
            }
    }
}

//  This determines the device list of related class...

void    CProfile::DeviceCheck() {

    //  Enumerate the available devices of this type in the csaDevice Array

    m_pcdlClass -> Enumerate();
    m_bDevicesChecked = TRUE;
}

//  Class constructor

CProfile::CProfile(LPCTSTR lpstrTarget) {

    _ASSERTE(lpstrTarget && *lpstrTarget);

    m_pcdlClass = NULL;

    //  First, let's make sure it's the real McCoy

    PROFILE     prof = { PROFILE_FILENAME,
                         (LPVOID) lpstrTarget,
                         (1 + lstrlen(lpstrTarget)) * sizeof(TCHAR)};

    m_hprof = OpenColorProfile(&prof, PROFILE_READ,
                                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                                      OPEN_EXISTING);

    if  (!m_hprof)
        return;

    if  (!GetColorProfileHeader(m_hprof, &m_phThis)) {
        CloseColorProfile(m_hprof);
        m_hprof = NULL;
        return;
    }

    m_csName = lpstrTarget;
    m_bInstallChecked = m_bDevicesChecked = m_bAssociationsChecked = FALSE;

    //  Init the DeviceList pointer, because it doesn't cost much...

    switch  (m_phThis.phClass) {
        case    CLASS_PRINTER:

            //  Our device list is a printer list

            m_pcdlClass = new CPrinterList;
            break;

        case     CLASS_SCANNER:

            //  Our device list is a scanner list

            m_pcdlClass = new CScannerList;
            break;


        case    CLASS_MONITOR:

            //  Our device list is a monitor list

        #if 1 // ALLOW_MONITOR_PROFILE_TO_ANY_DEVICE
            m_pcdlClass = new CAllDeviceList;
        #else
            m_pcdlClass = new CMonitorList;
        #endif
            break;

        case    CLASS_COLORSPACE:

            //  List everything we can count

            m_pcdlClass = new CAllDeviceList;
            break;

        default:
            //  Use the base device class (i.e., no devices of this type).
            m_pcdlClass = new CDeviceList;
    }
}

//  Destructor

CProfile::~CProfile() {
    if  (m_hprof)
        CloseColorProfile(m_hprof);
    if  (m_pcdlClass)
        delete  m_pcdlClass;
}

//  Tag retrieval function

LPCSTR  CProfile::TagContents(TAGTYPE tt, unsigned uOffset) {

    DWORD   dwcNeeded = sizeof m_acTag;
    BOOL    bIgnore;

    if  (!GetColorProfileElement(m_hprof, tt, 8 + uOffset, &dwcNeeded, m_acTag,
         &bIgnore))
        return  NULL;   //  Nothing to copy!
    else
        return  m_acTag;
}

//  Profile Installation function

BOOL    CProfile::Install() {

    if  (!InstallColorProfile(NULL, m_csName)) {
        CGlobals::ReportEx(InstFailedWithName,NULL,FALSE,
                           MB_OK|MB_ICONEXCLAMATION,1,m_csName.NameAndExtension());
        return (FALSE);
    } else {
        m_bIsInstalled = TRUE;
        CGlobals::InvalidateList();
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, (LPCTSTR) m_csName, NULL);

        _RPTF1(_CRT_WARN, "CProfile::Install %s succeeded\n",
            (LPCSTR) m_csName);
        return (TRUE);
    }
}

//  Profile Uninstallation function

void    CProfile::Uninstall(BOOL bDelete) {

    while   (AssociationCount()) {    // Dissociate all uses
        Dissociate(DeviceName(m_cuaAssociation[0]));
        m_cuaAssociation.Remove(0);
    }

    if  (m_hprof)
    {
        CloseColorProfile(m_hprof);
        m_hprof = NULL;
    }

    if  (!UninstallColorProfile(NULL, m_csName.NameAndExtension(), bDelete)) {
        CGlobals::ReportEx(UninstFailedWithName,NULL,FALSE,
                           MB_OK|MB_ICONEXCLAMATION,1,m_csName.NameAndExtension());
    } else {
        m_bIsInstalled = FALSE;
        CGlobals::InvalidateList();
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, (LPCTSTR) m_csName, NULL);

        _RPTF1(_CRT_WARN, "CProfile::Uninstall %s succeeded\n",
            (LPCSTR) m_csName);
    }
}

//  Association

void    CProfile::Associate(LPCTSTR lpstrDevice) {

    // if the profile is not installed, install it first.

    BOOL bInstalled = FALSE;

    // Install profile, if not installed, yet.
    if  (!IsInstalled()) {
        bInstalled = Install();
    } else
        bInstalled = TRUE;

    if  (bInstalled) {
        if  (!AssociateColorProfileWithDevice(NULL, m_csName.NameAndExtension(),
            lpstrDevice)) {
            CGlobals::ReportEx(AssocFailedWithName,NULL,FALSE,1,
                               MB_OK|MB_ICONEXCLAMATION,m_csName.NameAndExtension());
        } else
            _RPTF2(_CRT_WARN, "CProfile::Associate %s with %s succeeded\n",
                lpstrDevice, (LPCTSTR) m_csName.NameAndExtension());
    }
}

//  Dissociation

void    CProfile::Dissociate(LPCTSTR lpstrDevice) {
    if  (!DisassociateColorProfileFromDevice(NULL, m_csName.NameAndExtension(),
        lpstrDevice)) {
        CGlobals::ReportEx(DisassocFailedWithName,NULL,FALSE,1,
                           MB_OK|MB_ICONEXCLAMATION,m_csName.NameAndExtension());
    } else
        _RPTF2(_CRT_WARN, "CProfile::Dissociate %s from %s succeeded\n",
            lpstrDevice, (LPCTSTR) m_csName.NameAndExtension());
}

//  CProfileArray class- Same basic implementation, different base type.

CProfile    *CProfileArray::Borrow() {
    CProfile    *pcpReturn = m_aStore[0];

    memcpy((LPSTR) m_aStore, (LPSTR) (m_aStore + 1),
        (ChunkSize() - 1) * sizeof m_aStore[0]);

    if  (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcpaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = (CProfile *) NULL;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcpaNext) {
        delete  m_pcpaNext;
        m_pcpaNext = NULL;
    }

    return  pcpReturn;
}

CProfileArray::CProfileArray() {
    m_ucUsed = 0;
    m_pcpaNext = NULL;
    memset(m_aStore, 0, sizeof m_aStore);
}

CProfileArray::~CProfileArray() {
    Empty();
}

void    CProfileArray::Empty() {
    if  (!m_ucUsed) return;

    if  (m_pcpaNext) {
        delete  m_pcpaNext;
        m_pcpaNext = NULL;
        m_ucUsed = ChunkSize();
    }

    while   (m_ucUsed--)
        delete  m_aStore[m_ucUsed];

    m_ucUsed = 0;
    memset(m_aStore, 0, sizeof m_aStore);
}

//  Add an item
void    CProfileArray::Add(LPCTSTR lpstrNew) {
    _ASSERTE(lpstrNew && *lpstrNew);

    if  (m_ucUsed < ChunkSize()) {
        m_aStore[m_ucUsed++] = new  CProfile(lpstrNew);
        return;
    }

    //  Not enough space!  Add another record, if there isn't one

    if  (!m_pcpaNext)
        m_pcpaNext = new CProfileArray;

    //  Add the profile to the next array (recursive call!)

    m_pcpaNext -> Add(lpstrNew);
    m_ucUsed++;
}

CProfile    *CProfileArray::operator [](unsigned u) const {
    return  u < ChunkSize() ?
        m_aStore[u] : m_pcpaNext -> operator[](u - ChunkSize());
}

void    CProfileArray::Remove(unsigned u) {

    if  (u > m_ucUsed)
        return;

    if  (u >= ChunkSize()) {
        m_pcpaNext -> Remove(u - ChunkSize());
        return;
    }

    delete  m_aStore[u];

    memmove((LPSTR) (m_aStore + u), (LPSTR) (m_aStore + u + 1),
        (ChunkSize() - (u + 1)) * sizeof m_aStore[0]);

    if (m_ucUsed > ChunkSize())
        m_aStore[ChunkSize() - 1] = m_pcpaNext -> Borrow();
    else
        m_aStore[ChunkSize() - 1] = (CProfile *) NULL;

    m_ucUsed--;

    if  (m_ucUsed <= ChunkSize() && m_pcpaNext) {
        delete  m_pcpaNext;
        m_pcpaNext = NULL;
    }
}
