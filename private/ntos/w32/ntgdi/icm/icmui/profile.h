/******************************************************************************

  Header File:  ICC Profile.H

  This defines the C++ class we encapsulate the profile in.  It also defines
  the base class for device enumeration.  All activity related to the profile,
  including installations, associations, and so forth, is encapsulated in the
  CProfile class.  The UI classes themselves never call an ICM API, but are
  instead concerned with simply handling the interface.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  10-31-96  A-RobKj (Pretty Penny Enterprises) began encapsulating
  11-22-96  A-RobKj changed associations from string array to uint array to
                    facilitate the spec'd device naming conventions, and
                    filtering of lists for previously installed devices.

  12-04-96  A-RobKj Added the CProfileArray class to make the Device
                    management UI more efficient.

  12-13-96  A-RobKj Moved the CDeviceList derived classes here, so I can use
                    them elsewhere.

******************************************************************************/

#if !defined(ICC_PROFILE_CLASS)

#define ICC_PROFILE_CLASS

#include    "StringAr.H"
#include    "Dialog.H"

//  CDeviceList class

/******************************************************************************

  Highlights:

  This is a base class for the enumeration and reporting of Device
  class-specific information.  The spec is such that a device can be associated
  with a friendly name, but displayed in the UI with an enhanced name.  Also,
  the method for enumerating a device can differ from class to class.

  This base class is usable for cases where there is no means of enumerating
  devices currently.  It reports that there are no devices.
  To make overrides easier, the default display name returns the friendly name,
  so derived classes where this is the case do not need to override this
  method.

******************************************************************************/

class CDeviceList { //  Device-specific information- base class
    CString m_csDummy;

public:
    CDeviceList() {}
    ~CDeviceList() {}

    virtual unsigned    Count() { return    0; }
    virtual CString&    DeviceName(unsigned u) { return m_csDummy; }
    virtual CString&    DisplayName(unsigned u) { return DeviceName(u); }
    virtual void        Enumerate() {}
    virtual BOOL        IsValidDeviceName(LPCTSTR lpstr) { return FALSE; }
};

//  Device Enumeration classes- these must all derive from CDeviceList

//  The CPrinterList class handles printers.  Enumeration is via the Win32
//  spooler API.

class CPrinterList : public CDeviceList {
    CStringArray    m_csaDeviceNames;
    CStringArray    m_csaDisplayNames;

public:
    CPrinterList() {}
    ~CPrinterList() {}

    virtual unsigned    Count() { return m_csaDeviceNames.Count(); }
    virtual CString&    DeviceName(unsigned u) { return m_csaDeviceNames[u]; }
    virtual CString&    DisplayName(unsigned u) { return m_csaDisplayNames[u]; }

    virtual void        Enumerate();
    virtual BOOL        IsValidDeviceName(LPCTSTR lpstr);
};

//  The CMonitorList class handles monitors.  Enumeration is via a private ICM
//  API.

class CMonitorList : public CDeviceList {
    CStringArray    m_csaDeviceNames;
    CStringArray    m_csaDisplayNames;

    CString         m_csPrimaryDeviceName;

public:
    CMonitorList() {}
    ~CMonitorList() {}

    virtual unsigned    Count() { return m_csaDeviceNames.Count(); }
    virtual CString&    DeviceName(unsigned u) { return m_csaDeviceNames[u]; }
    virtual CString&    DisplayName(unsigned u) { return m_csaDisplayNames[u]; }

    virtual CString&    PrimaryDeviceName() { return m_csPrimaryDeviceName; }

    virtual void        Enumerate();
    virtual BOOL        IsValidDeviceName(LPCTSTR lpstr);

    virtual LPCSTR      DeviceNameToDisplayName(LPCTSTR lpstr);
};

//  The CScannerList class handles scanners.  Enumeration is via the STI
//  interface.

class CScannerList : public CDeviceList {
    CStringArray    m_csaDeviceNames;
    CStringArray    m_csaDisplayNames;

public:
    CScannerList() {}
    ~CScannerList() {}

    virtual unsigned    Count() { return m_csaDeviceNames.Count(); }
    virtual CString&    DeviceName(unsigned u) { return m_csaDeviceNames[u]; }
    virtual CString&    DisplayName(unsigned u) { return m_csaDisplayNames[u]; }

    virtual void        Enumerate();
    virtual BOOL        IsValidDeviceName(LPCTSTR lpstr);
};

//  The CAllDeviceList class shows everything.  We enumerate by combining the
//  results of enumerating all of the other classes.

class CAllDeviceList : public CDeviceList {
    CStringArray    m_csaDeviceNames;
    CStringArray    m_csaDisplayNames;

public:
    CAllDeviceList() {}
    ~CAllDeviceList() {}

    virtual unsigned    Count() { return m_csaDeviceNames.Count(); }
    virtual CString&    DeviceName(unsigned u) { return m_csaDeviceNames[u]; }
    virtual CString&    DisplayName(unsigned u) { return m_csaDisplayNames[u]; }

    virtual void        Enumerate();
    virtual BOOL        IsValidDeviceName(LPCTSTR lpstr);
};

//  CProfile class

class CProfile {

    HPROFILE        m_hprof;                //  Profile handle
    PROFILEHEADER   m_phThis;               //  Profile header
    CString         m_csName;
    BOOL            m_bIsInstalled, m_bInstallChecked, m_bAssociationsChecked,
                    m_bDevicesChecked;
    CDeviceList     *m_pcdlClass;           //  Devices of this class
    CUintArray      m_cuaAssociation;       //  Associated devices (indices)
    char            m_acTag[MAX_PATH * 2];
    void    InstallCheck();
    void    AssociationCheck();
    void    DeviceCheck();

public:

    static void Enumerate(ENUMTYPE& et, CStringArray& csaList);
    static void Enumerate(ENUMTYPE& et, CStringArray& csaList, CStringArray& csaDesc);
    static void Enumerate(ENUMTYPE& et, class CProfileArray& cpaList);
    static const CString  ColorDirectory();

    CProfile(LPCTSTR lpstr);
    ~CProfile();

    //  Queries

    CString GetName() { return m_csName.NameOnly(); }
    DWORD   GetType() { return m_hprof ? m_phThis.phClass : 0; }
    DWORD   GetCMM()  { return m_hprof ? m_phThis.phCMMType : 0; }

    // Inquire the color space information from the header
    DWORD   GetColorSpace() {return m_hprof ? m_phThis.phDataColorSpace : 0;}

    BOOL    IsInstalled() {
        if  (!m_bInstallChecked)
            InstallCheck();
        return m_bIsInstalled;
    }
    BOOL    IsValid() {
        BOOL bValid = FALSE;

        if (m_hprof)
            IsColorProfileValid(m_hprof, &bValid);

        return  bValid;
    }

    unsigned    DeviceCount() {
        if (m_pcdlClass) {
          if (!m_bDevicesChecked)
              DeviceCheck();
          return m_pcdlClass -> Count();
        } else {
          return 0;                           // low memory - m_pcdlClass allocation failed
        }
    }

    unsigned    AssociationCount() {
        if  (!m_bAssociationsChecked)
            AssociationCheck();
        return m_cuaAssociation.Count();
    }

    LPCTSTR     DeviceName(unsigned u) {
        if (m_pcdlClass) {
          if  (!m_bDevicesChecked)
              DeviceCheck();
          return m_pcdlClass -> DeviceName(u);
        } else {
          return TEXT("");                    // low memory - m_pcdlClass allocation failed
        }
    }

    LPCTSTR     DisplayName(unsigned u) {
        if (m_pcdlClass) {
          if  (!m_bDevicesChecked)
              DeviceCheck();
          return m_pcdlClass -> DisplayName(u);
        } else {
          return TEXT("");                    // low memory - m_pcdlClass allocation failed
        }
    }

    unsigned    Association(unsigned u) {
        if  (!m_bAssociationsChecked)
            AssociationCheck();
        return m_cuaAssociation[u];
    }

    LPCSTR      TagContents(TAGTYPE tt, unsigned uOffset = 0);

    //  Operations

    BOOL    Install();
    void    Uninstall(BOOL bDelete);
    void    Associate(LPCTSTR lpstrNew);
    void    Dissociate(LPCTSTR lpstrNew);

};

//  CProfileArray class- this is a list of profiles- it is used by the Device
//  Management UI, so we only construct a CProfile object once per profile.

class   CProfileArray {
    CProfile        *m_aStore[20];
    CProfileArray   *m_pcpaNext;
    unsigned        m_ucUsed;

    const unsigned ChunkSize() const {
        return sizeof m_aStore / sizeof m_aStore[0];
    }

    CProfile    *Borrow();

public:

    CProfileArray();
    ~CProfileArray();

    unsigned    Count() const { return m_ucUsed; }

    //  Add an item
    void        Add(LPCTSTR lpstrNew);

    CProfile    *operator [](unsigned u) const;

    void        Remove(unsigned u);
    void        Empty();
};

#endif
