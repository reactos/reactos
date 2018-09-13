#include "stdafx.h"
#include "devtree.h"
#include <objbase.h>
#include <initguid.h>
#include <devguid.h>
#include <rpc.h>
void
CDevice::GetProperty(
    HMACHINE hMachine,
    DEVNODE dn,
    ULONG Property,
    int IdType
    )
{
    CString strType;
    TCHAR Buffer[REGSTR_VAL_MAX_HCID_LEN];
    ULONG Type;
    ULONG Size = sizeof(Buffer);
    strType.LoadString(IdType);
    // preload default string
    strType.LoadString(IDS_UNKNOWN);
    lstrcpy(Buffer, (LPCTSTR)strType);
    strType.LoadString(IdType);
    if (CR_SUCCESS == CM_Get_DevNode_Registry_Property_Ex(dn, Property,
							  &Type,
							  Buffer,
							  &Size,
							  0, hMachine))
    {
	if (REG_DWORD == Type || REG_MULTI_SZ == Type || REG_SZ == Type)
	{
	    if (REG_DWORD == Type)
	    {
		DWORD Data = *((DWORD*)Buffer);
		wsprintf(Buffer, _T("%08x"), *((DWORD*)Buffer) );
	    }
	    else if (REG_MULTI_SZ == Type)
	    {
		LPTSTR p = Buffer;
		while (_T('\0') != *p)
		{
		    p += lstrlen(p);
		    if (_T('\0') != *p)
			*p++ = _T(',');
		}
	    }
	}
    }
    CAttribute* pAttr = new CAttribute(strType, Buffer);
    m_listAttributes.AddTail(pAttr);
}


// prepare device's attributes
BOOL
CDevice::Create(
    HMACHINE hMachine,
    DEVNODE  DevNode
    )
{
    CString	strType;
    CString	strValue;
    LPTSTR	Buffer;
    int  BufferSize = MAX_PATH + MAX_DEVICE_ID_LEN;
    ULONG  BufferLen = BufferSize * sizeof(TCHAR);
    m_pSibling = NULL;
    m_pParent = NULL;
    m_pChild = NULL;
    m_ImageIndex = 0;
    Buffer  = strValue.GetBuffer(BufferSize);
    if (CR_SUCCESS == CM_Get_DevNode_Registry_Property_Ex(DevNode,
					CM_DRP_FRIENDLYNAME, NULL,
					Buffer, &BufferLen, 0, hMachine))
    {
	m_strDisplayName = Buffer;
    }
    else
    {
	BufferLen = BufferSize * sizeof(TCHAR);
	if (CR_SUCCESS == CM_Get_DevNode_Registry_Property_Ex(DevNode,
					CM_DRP_DEVICEDESC, NULL,
					Buffer, &BufferLen, 0, hMachine))
	{
	    m_strDisplayName = Buffer;
	}
	else
	{
	    m_strDisplayName.LoadString(IDS_UNKNOWN);
	}
    }
    CAttribute* pAttr;
    // pepare device' attributes
    // do standard ones first;
    if (CR_SUCCESS == CM_Get_Device_ID_Ex(DevNode, Buffer, BufferSize, 0, hMachine))
    {
	strType.LoadString(IDS_DEVICEID);
	pAttr = new CAttribute(strType, Buffer);
	m_listAttributes.AddTail(pAttr);
    }
    ULONG Status, Problem;

    if (CR_SUCCESS == CM_Get_DevNode_Status_Ex(&Status, &Problem, DevNode, 0, hMachine))
    {
	strValue.Format(_T("%08x"), Status);
	strType.LoadString(IDS_STATUS);
	pAttr = new CAttribute(strType, strValue);
	m_listAttributes.AddTail(pAttr);
	strType.LoadString(IDS_PROBLEM);
	strValue.Format(_T("%08x"), Problem);
	pAttr = new CAttribute(strType, strValue);
	m_listAttributes.AddTail(pAttr);
    }
    GetProperty(hMachine, DevNode, CM_DRP_SERVICE, IDS_SERVICE);
    GetProperty(hMachine, DevNode, CM_DRP_CAPABILITIES, IDS_CAPABILITIES);
    GetProperty(hMachine, DevNode, CM_DRP_CONFIGFLAGS, IDS_CONFIGFLAGS);
    GetProperty(hMachine, DevNode, CM_DRP_MFG, IDS_MFG);
    GetProperty(hMachine, DevNode, CM_DRP_CLASS, IDS_CLASS);
    GetProperty(hMachine, DevNode, CM_DRP_HARDWAREID, IDS_HARDWAREID);
    GetProperty(hMachine, DevNode, CM_DRP_COMPATIBLEIDS, IDS_COMPATIBLEID);
    GetProperty(hMachine, DevNode, CM_DRP_CLASSGUID, IDS_CLASSGUID);
    GetProperty(hMachine, DevNode, CM_DRP_LOCATION_INFORMATION, IDS_LOCATION);
    GetProperty(hMachine, DevNode, CM_DRP_BUSNUMBER, IDS_BUSNUMBER);
    GetProperty(hMachine, DevNode, CM_DRP_ENUMERATOR_NAME, IDS_ENUMERATOR_NAME);
    GetProperty(hMachine, DevNode, CM_DRP_DEVICEDESC, IDS_DEVICEDESC);
    GetProperty(hMachine, DevNode, CM_DRP_FRIENDLYNAME, IDS_FRIENDLYNAME);
    GetProperty(hMachine, DevNode, CM_DRP_DRIVER, IDS_DRIVER);
    GetProperty(hMachine, DevNode, CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME, IDS_PHYSICAL_DEVPATH);
    GetProperty(hMachine, DevNode, CM_DRP_UI_NUMBER, IDS_UI_NUMBER);
    GetProperty(hMachine, DevNode, CM_DRP_UPPERFILTERS, IDS_UPPERFILTERS);
    GetProperty(hMachine, DevNode, CM_DRP_LOWERFILTERS, IDS_LOWERFILTERS);
    GetProperty(hMachine, DevNode, CM_DRP_BUSTYPEGUID, IDS_BUSTYPEGUID);
    GetProperty(hMachine, DevNode, CM_DRP_LEGACYBUSTYPE, IDS_LEGACYBUSTYPE);

    return TRUE;
}

BOOL
CDevice::EnumerateAttribute(
    int Index,
    CAttribute** ppAttr
    )
{
    if (m_listAttributes.IsEmpty() || Index >= m_listAttributes.GetCount())
	return FALSE;
    POSITION pos = m_listAttributes.FindIndex(Index);
    *ppAttr = m_listAttributes.GetAt(pos);
    return TRUE;
}

CDevice::~CDevice()
{
    if (!m_listAttributes.IsEmpty())
    {
	POSITION pos = m_listAttributes.GetHeadPosition();
	while (NULL != pos)
	{
	    delete (CAttribute*)m_listAttributes.GetNext(pos);
	}
	m_listAttributes.RemoveAll();
    }
    if (m_pChild)
	delete m_pChild;
    if (m_pSibling)
	delete m_pSibling;
}

BOOL
CComputer::Create(
    LPCTSTR ComputerName
    )
{
    TCHAR LocalComputer[MAX_PATH];
    DWORD Size = MAX_PATH - 2;
    GetComputerName(LocalComputer + 2, &Size);
    LocalComputer[0] = _T('\\');
    LocalComputer[1] = _T('\\');
    if (ComputerName && _T('\0') != *ComputerName &&
	_T('\\') == ComputerName[0] && _T('\\') == ComputerName[1])
    {
	m_strDisplayName = ComputerName + 2;
	m_IsLocal = (0 == lstrcmpi(ComputerName, LocalComputer));
	CONFIGRET cr;
	if (CR_SUCCESS != (cr = CM_Connect_Machine(ComputerName, &m_hMachine)))
	{
	    m_hMachine = NULL;
	    SetLastError(ERROR_INVALID_COMPUTERNAME);
	    OutputDebugString(_T("Connect computer failed\n"));
	    TCHAR Text[MAX_PATH];
	    wsprintf(Text, _T("Machine Connection failed, cr= %lx(hex)\n"), cr);
	    MessageBox(NULL, Text, _T("List Device"), MB_ICONSTOP | MB_OK);
	    return FALSE;
	}
    }
    else
    {
	m_strDisplayName = LocalComputer + 2;
	m_IsLocal = TRUE;
	m_hMachine = NULL;
    }
    return TRUE;
}

CComputer::~CComputer()
{
    CM_Disconnect_Machine(m_hMachine);
}

BOOL
CDeviceTree::Create(
    LPCTSTR ComputerName
    )
{
    if (m_pComputer)
    {
	delete m_pComputer;
	SetupDiDestroyClassImageList(&m_ImageListData);
	m_ImageListData.cbSize = 0;
	m_pComputer = NULL;
    }
    ASSERT(NULL == m_pComputer);

    m_pComputer = new CComputer;
    if (m_pComputer->Create(ComputerName))
    {
	m_ImageListData.cbSize = sizeof(m_ImageListData);
	SetupDiGetClassImageList(&m_ImageListData);
	int ImageIndex;
	SetupDiGetClassImageIndex(&m_ImageListData,  (LPGUID)&GUID_DEVCLASS_COMPUTER, &ImageIndex);
	m_pComputer->SetImageIndex(ImageIndex);
	DEVNODE dnRoot;
	CM_Locate_DevNode_Ex(&dnRoot, NULL, 0, *m_pComputer);
	DEVNODE dnFirst;
	CM_Get_Child_Ex(&dnFirst, dnRoot, 0, *m_pComputer);
	return CreateSubtree(m_pComputer, NULL, dnFirst);
    }
    else
    {
	delete m_pComputer;
	m_pComputer = NULL;
	return FALSE;
    }
}



BOOL
CDeviceTree::CreateSubtree(
    CDevice* pParent,
    CDevice* pSibling,
    DEVNODE  dn
    )
{
    CDevice* pDevice;
    DEVNODE dnSibling, dnChild;
    do
    {
	if (CR_SUCCESS != CM_Get_Sibling_Ex(&dnSibling, dn, 0, *m_pComputer))
	    dnSibling = NULL;
	pDevice = new CDevice;
	pDevice->Create(*m_pComputer, dn);
	pDevice->SetParent(pParent);
	if (pSibling)
	    pSibling->SetSibling(pDevice);
	else if (pParent)
	    pParent->SetChild(pDevice);

	TCHAR GuidString[MAX_GUID_STRING_LEN];
	ULONG Size = sizeof(GuidString);
	if (CR_SUCCESS == CM_Get_DevNode_Registry_Property_Ex(dn,
					CM_DRP_CLASSGUID, NULL,
					GuidString, &Size, 0,  *m_pComputer) &&
	    _T('{') == GuidString[0] && _T('}') == GuidString[MAX_GUID_STRING_LEN - 2])
	{
	    GUID Guid;
	    GuidString[MAX_GUID_STRING_LEN - 2] = _T('\0');
	    UuidFromString(&GuidString[1], &Guid);
	    pDevice->SetClassGuid(&Guid);
	    int Index;
	    if (SetupDiGetClassImageIndex(&m_ImageListData, &Guid, &Index))
		pDevice->SetImageIndex(Index);
	}
	if (CR_SUCCESS == CM_Get_Child_Ex(&dnChild, dn, 0, *m_pComputer))
	{
	    CreateSubtree(pDevice, NULL, dnChild);
	}
	dn = dnSibling;
	pSibling = pDevice;
    } while (NULL != dn);
    return TRUE;
}
