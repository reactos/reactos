
#ifndef __CNODE_H__
#define __CNODE_H__
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cnode.h

Abstract:

    header file for cnode.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/


class CResultItem;
class CResultComputer;
class CResultClass;
class CResultDevice;
class CDriverList;
class CDriver;
class CHwProfile;
class CMachine;
class CItemIdentifier;
class CDeviceIdentifier;
class CClassIdentifier;
class CComputerIdentifier;
class CResourceIdentifier;
class CResourceTypeIdentifier;

#define ALL_LOG_CONF        BOOT_LOG_CONF + ALLOC_LOG_CONF + FORCED_LOG_CONF

inline
COOKIE_TYPE CookieType(RESOURCEID ResType)
{
    if (ResType_Mem == ResType)
    {
        return COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY;
    }

    else if (ResType_IO == ResType)
    {
        return COOKIE_TYPE_RESULTITEM_RESOURCE_IO;
    }

    else if (ResType_DMA == ResType)
    {
        return COOKIE_TYPE_RESULTITEM_RESOURCE_DMA;
    }

    else if (ResType_IRQ == ResType)
    {
        return COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ;
    }

    else
    {
        ASSERT(FALSE);
        return COOKIE_TYPE_UNKNOWN;
    }
}

///
/// class to represent a result pane item
///

class CResultItem
{

public:
    CResultItem() : m_pMachine(NULL)
    {}

    virtual ~CResultItem() {};
    LPCTSTR GetDisplayName() const
    {
        return (LPCTSTR)m_strDisplayName;
    }

    int GetImageIndex()
    {
        return m_iImage;
    }
    virtual CItemIdentifier* CreateIdentifier() = 0;
    CMachine*   m_pMachine;

protected:
    int     m_iImage;
    String  m_strDisplayName;
};



class CClass : public CResultItem
{
public:
    CClass(CMachine* pMachine, LPGUID pGuid);

    virtual ~CClass();
    BOOL GetFirstDevice(CDevice** ppDevice, PVOID& pContext);
    BOOL GetNextDevice(CDevice** ppDevice, PVOID& Context);
    operator GUID&()
    {
        return m_Guid;
    }
    operator LPGUID()
    {
        return &m_Guid;
    }
    BOOL operator ==(const CClass& OtherClass)
    {
        return IsEqualGUID(m_Guid, (GUID&)OtherClass);
    }
    CDevInfoList* GetDevInfoList(HWND hwndParent = NULL);
    HICON LoadIcon();
    void AddDevice(CDevice* pDevice);
    int GetNumberOfDevices(BOOL Hidden = FALSE)
    {
            return Hidden ? m_TotalDevices :
                 m_TotalDevices - m_TotalHiddenDevices;
    }
    BOOL NoDisplay()
    {
        return m_NoDisplay;
    }
    virtual CItemIdentifier* CreateIdentifier();
    void PropertyChanged();

    CPropSheetData m_psd;

private:
    CClass(const CClass& ClassSrc);
    CClass& operator=(const CClass& ClassSrc);
    GUID    m_Guid;
    CList<CDevice*, CDevice*> m_listDevice;
    BOOL    m_NoDisplay;
    POSITION    m_pos;
    CDevInfoList* m_pDevInfoList;
    int     m_TotalDevices;
    int     m_TotalHiddenDevices;
};

class CDevice : public CResultItem
{
public:
    CDevice() : m_pClass(NULL), m_pSibling(NULL), m_pChild(NULL),
        m_pParent(NULL)
    {}
    CDevice(CMachine* pMachine, CClass* pClass, PSP_DEVINFO_DATA pDevData);

    operator SP_DEVINFO_DATA&()
    {
        return m_DevData;
    }
    operator PSP_DEVINFO_DATA()
    {
        return &m_DevData;
    }
    BOOL operator ==(LPCTSTR DeviceID)
    {
        return (0 == m_strDeviceID.CompareNoCase(DeviceID));
    }

    BOOL operator ==(CDevice& OtherDevice);

    CDevice* GetChild()
    {
        return m_pChild;
    }
    CDevice* GetParent()
    {
        return m_pParent;
    }
    CDevice* GetSibling()
    {
        return m_pSibling;
    }
    void SetChild(CDevice* pDevice)
    {
        m_pChild = pDevice;
    }
    void SetParent(CDevice* pDevice)
    {
        m_pParent = pDevice;
    }
    void SetSibling(CDevice* pDevice)
    {
        m_pSibling = pDevice;
    }
    CClass* GetClass()
    {
        return m_pClass;
    }
    virtual DEVNODE GetDevNode()
    {
        return m_DevData.DevInst;
    }
    LPCTSTR GetDeviceID() const
    {
        return (m_strDeviceID.IsEmpty()) ? NULL : (LPCTSTR)m_strDeviceID;
    }
    void SetClass(CClass* pClass)
    {
        m_pClass = pClass;
    }
    BOOL IsHidden();
    BOOL IsPhantom();
    BOOL NoShowInDM();
    BOOL IsPCMCIA();
    BOOL IsPCIDevice();
    BOOL IsPnpDevice();
    BOOL IsBiosDevice();

    BOOL GetMFG(TCHAR* pBuffer, DWORD Size, DWORD* pRequiredSize)
    {
        return m_pMachine->DiGetDeviceMFGString(&m_DevData, pBuffer, Size, pRequiredSize);
    }

    virtual CItemIdentifier* CreateIdentifier();
    CDevice* FindMFParent();
    HICON LoadClassIcon();
    BOOL GetStatus(DWORD* pStatus, DWORD* pProblem);
    BOOL IsRAW();
    BOOL IsUninstallable();
    BOOL IsDisableable();
    BOOL IsDisabled();
    BOOL IsStateDisabled();
    BOOL IsStarted();
    BOOL IsMFChild();
    BOOL IsSpecialMFChild();
    BOOL HasProblem();
    BOOL NeedsRestart();
    BOOL GetConfigFlags(DWORD* pFlags);
    BOOL GetKnownLogConf(LOG_CONF* plc, DWORD* plcType);
    BOOL HasResources();
    void GetMFGString(String& strMFG);
    void GetProviderString(String& strMFG);
    void GetDriverDateString(String& strMFG);
    void GetDriverVersionString(String& strMFG);
    LPCTSTR  GetClassDisplayName();
    void ClassGuid(GUID& ClassGuid)
    {
        ASSERT(m_pClass);
        ClassGuid = *m_pClass;
    }
    BOOL  NoChangeUsage();
    CDriver* CreateDriver();
    BOOL    HasDrivers();
    DWORD EnableDisableDevice(HWND hDlg, BOOL Enabling);
    void PropertyChanged();
    CPropSheetData m_psd;

private:
    CDevice(const CDevice& DeviceSrc);
    CDevice& operator=(const CDevice& DeviceSrc);

    CDevice*        m_pParent;
    CDevice*        m_pSibling;
    CDevice*        m_pChild;
    String      m_strDeviceID;
    SP_DEVINFO_DATA m_DevData;
    CClass*     m_pClass;
};

class CComputer : public CDevice
{
public:
    CComputer(CMachine* pMachine, DEVNODE dnRoot);
    virtual DEVNODE GetDevNode()
    {
        return m_dnRoot;
    }
    virtual CItemIdentifier* CreateIdentifier();

private:
    DEVNODE m_dnRoot;
    CComputer(const CComputer& ComputerSrc);
    CComputer& operator=(const CComputer& ComputerSrc);
};

class CResource : public CResultItem
{
public:
    CResource(CDevice* pDevice, RESOURCEID ResType, DWORDLONG dlBase, DWORDLONG dlLen,
          BOOL Forced, BOOL Free = FALSE);
    BOOL IsForced()
    {
        return m_Forced;
    }
    void GetValue(DWORDLONG* pdlBase, DWORDLONG* pdlLen) const
    {
        ASSERT(pdlBase && pdlLen);
        *pdlBase = m_dlBase;
        *pdlLen = m_dlLen;
    }
    virtual CItemIdentifier* CreateIdentifier();

    void GetRangeString(String& strRange);
    LPCTSTR GetViewName()
    {
            return (LPCTSTR)m_strViewName;
    }
#ifdef RESOURCE_STATUS
    void GetStatusString(String& strStatus);
#endif

    CDevice* GetDevice()
    {
        return m_pDevice;
    }
    CResource* GetChild()
    {
        return m_pChild;
    }
    CResource* GetSibling()
    {
        return m_pSibling;
    }
    CResource* GetParent()
    {
        return m_pParent;
    }
    void SetChild(CResource* pRes)
    {
        m_pChild = pRes;
    }
    void SetParent(CResource* pRes)
    {
        m_pParent = pRes;
    }
    void SetSibling(CResource* pRes)
    {
        m_pSibling = pRes;
    }
    BOOL operator <=(const CResource& resSrc);
    BOOL SameRange(const CResource& resSrc);
    BOOL EnclosedBy(const CResource& resSrc);
    RESOURCEID ResType()
    {
        return m_ResType;
    }
    CDevice*    m_pDevice;  // BUGBUG - this should not be public

private:
    CResource(const CResource& resSrc);
    CResource& operator=(const CResource& resSrc);
    RESOURCEID  m_ResType;
    DWORDLONG   m_dlBase;
    DWORDLONG   m_dlEnd;
    DWORDLONG   m_dlLen;
    BOOL    m_Forced;
    CResource*  m_pChild;
    CResource*  m_pSibling;
    CResource*  m_pParent;
//    COOKIE_TYPE m_CookieType;
    BOOL    m_Allocated;
    String      m_strViewName;
};

#define pIRQResData(pData) ((IRQ_RESOURCE*)pData)
#define pDMAResData(pData) ((DMA_RESOURCE*)pData)
#define pMemResData(pData) ((MEM_RESOURCE*)pData)
#define pIOResData(pData)  ((IO_RESOURCE*)pData)


class CResourceType : public CResultItem
{
public:
    CResourceType(CMachine* pMachine, RESOURCEID ResType);

    virtual CItemIdentifier* CreateIdentifier();

    CResource* GetChild()
    {
        return m_pChild;
    }
    CResourceType* GetSibling()
    {
        return m_pSibling;
    }
    CComputer* GetParent()
    {
        return m_pParent;
    }
    void SetChild(CResource* pRes)
    {
        m_pChild = pRes;
    }
    void SetParent(CComputer* pComp)
    {
        m_pParent = pComp;
    }
    void SetSibling(CResourceType* pResT)
    {
        m_pSibling = pResT;
    }
    RESOURCEID GetResType()
    {
        return m_ResType;
    }

private:
    CResourceType(const CResourceType& resSrc);
    CResourceType& operator=(const CResourceType& resSrc);
    RESOURCEID  m_ResType;
    CResource*  m_pChild;
    CResourceType*  m_pSibling;
    CComputer*  m_pParent;
};


class CResourceList
{
public:
    CResourceList(CDevice* pDevice, RESOURCEID ResType, 
                  ULONG LogConfType = ALLOC_LOG_CONF, ULONG AltLogConfType = BOOT_LOG_CONF);
    CResourceList(CMachine* pMachine, RESOURCEID ResType, 
                  ULONG LogConfType = ALLOC_LOG_CONF, ULONG AltLogConfType = BOOT_LOG_CONF);
    ~CResourceList();
    int GetCount()
    {
        return m_listRes.GetCount();
    }
    BOOL GetFirst(CResource** ppRes, PVOID& pContext);
    BOOL GetNext(CResource** ppRes, PVOID& Context);
    BOOL CreateResourceTree(CResource** ppRoot);
    static BOOL ExtractResourceValue(RESOURCEID ResType, PVOID pData,
                     DWORDLONG* pdlBase, DWORDLONG* pdlLen);
private:
    BOOL InsertResourceToTree(CResource* pRes, CResource* pResRoot, BOOL ForcedInsert);
    CResourceList(const CResourceList& Src);
    CResourceList& operator=(const CResourceList& Src);

    void CreateSubtreeResourceList(CDevice* pDeviceStart, RESOURCEID ResType, 
                                   ULONG LogConfType, ULONG AltLogConfType);
    CList<CResource*, CResource*> m_listRes;
    void InsertResourceToList(CResource* pRes);
#define pIRQResData(pData) ((IRQ_RESOURCE*)pData)
#define pDMAResData(pData) ((DMA_RESOURCE*)pData)
#define pMemResData(pData) ((MEM_RESOURCE*)pData)
#define pIOResData(pData)  ((IO_RESOURCE*)pData)
};


class CItemIdentifier
 {
public:
    CItemIdentifier()
    {}
    virtual ~CItemIdentifier()
    {}
    virtual BOOL operator==(CCookie& Cookie) = 0;
};


class CClassIdentifier : public CItemIdentifier
{
public:
    CClassIdentifier(CClass& Class)
    {
        m_Guid = (GUID&)Class;
    }
    virtual BOOL operator==(CCookie& Cookie)
    {
        return COOKIE_TYPE_RESULTITEM_CLASS == Cookie.GetType() &&
           IsEqualGUID(m_Guid, *((CClass*)Cookie.GetResultItem()));
    }

private:
    GUID    m_Guid;
};

class CDeviceIdentifier : public CItemIdentifier
{
public:
    CDeviceIdentifier(CDevice& Device)
    {
        ASSERT(Device.GetDeviceID());
        m_strDeviceId = Device.GetDeviceID();
    }
    virtual BOOL operator==(CCookie& Cookie)
    {
        return COOKIE_TYPE_RESULTITEM_DEVICE == Cookie.GetType() &&
           !lstrcmpi(m_strDeviceId, ((CDevice*)Cookie.GetResultItem())->GetDeviceID());
    }

private:
    String  m_strDeviceId;
};

class CResourceIdentifier : public CItemIdentifier
{
public:
    CResourceIdentifier(CResource& Res)
    {
        m_CookieType = CookieType(Res.ResType());
        m_strDisplayName = Res.GetDisplayName();
    }
    virtual BOOL operator==(CCookie& Cookie)
    {
        return m_CookieType == Cookie.GetType() &&
           !lstrcmpi((LPCTSTR)m_strDisplayName, Cookie.GetResultItem()->GetDisplayName());
    }

private:
    COOKIE_TYPE     m_CookieType;
    String      m_strDisplayName;
};

class CResourceTypeIdentifier : public CItemIdentifier
{
public:
    CResourceTypeIdentifier(CResourceType& ResType)
    {
        m_ResType = ResType.GetResType();
    }
    virtual BOOL operator==(CCookie& Cookie)
    {
        return COOKIE_TYPE_RESULTITEM_RESTYPE == Cookie.GetType() &&
           m_ResType == ((CResourceType*)Cookie.GetResultItem())->GetResType();
    }

private:
    RESOURCEID      m_ResType;
};

class CComputerIdentifier : public CItemIdentifier
{
public:
    CComputerIdentifier(CComputer& Computer)
    {
    ASSERT(Computer.GetDisplayName());
    m_strName = Computer.GetDisplayName();
    }
    virtual BOOL operator==(CCookie& Cookie)
    {
    return COOKIE_TYPE_RESULTITEM_COMPUTER == Cookie.GetType() &&
           !lstrcmpi((LPCTSTR)m_strName, Cookie.GetResultItem()->GetDisplayName());

    }

private:
    String  m_strName;
};


#endif // __CNODE_H__
