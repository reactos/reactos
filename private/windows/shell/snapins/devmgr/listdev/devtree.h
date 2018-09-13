#ifndef _DEVTREE_H__
#define _DEVTREE_H__

extern "C" {
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
}

#include <afxtempl.h>
#include "resource.h"

class CAttribute
{
public:
    CAttribute(LPCTSTR Type, LPCTSTR Value)
    {
	m_strType = Type;
	m_strValue = Value;
    }
    LPCTSTR GetType()
    {
	return m_strType.IsEmpty() ? NULL : (LPCTSTR)m_strType;
    }
    LPCTSTR GetValue()
    {
	return m_strValue.IsEmpty() ? NULL : (LPCTSTR)m_strValue;
    }
private:
    CString m_strType;
    CString m_strValue;
};

class CDevice
{
public:

    CDevice() : m_pParent(NULL), m_pSibling(NULL), m_pChild(NULL),
		m_ImageIndex(0)
    {
    }

    BOOL Create(HMACHINE hMachine, DEVNODE dn);

    ~CDevice();

    CDevice* GetSibling(void)
    {
	return m_pSibling;
    }
    CDevice* GetChild(void)
    {
	return m_pChild;
    }
    CDevice* GetParent(void)
    {
	return m_pParent;
    }
    void SetChild(CDevice* pChild)
    {
	m_pChild = pChild;
    }
    void SetSibling(CDevice* pSibling)
    {
	m_pSibling = pSibling;
    }
    void SetParent(CDevice* pParent)
    {
	m_pParent = pParent;
    }
    LPCTSTR GetDisplayName()
    {
	return (LPCTSTR)m_strDisplayName;
    }
    int NumberOfAttributes()
    {
	return m_listAttributes.GetCount();
    }
    void SetClassGuid(LPGUID pGuid)
    {
	m_ClassGuid = *pGuid;
    }
    void SetImageIndex(int Index)
    {
	m_ImageIndex = Index;
    }
    int GetImageIndex()
    {
	return m_ImageIndex;
    }
    BOOL EnumerateAttribute(int Index, CAttribute** ppAttribute);

protected:
    CString	m_strDisplayName;
private:
    CDevice*	m_pParent;
    CDevice*	m_pSibling;
    CDevice*	m_pChild;
    int 	m_ImageIndex;
    void GetProperty(HMACHINE hMachine, DEVNODE DevNode, ULONG Property, int IdType);
    GUID    m_ClassGuid;
    CList<CAttribute*, CAttribute*>	m_listAttributes;
};

class CComputer : public CDevice
{
public:
    CComputer() : m_hMachine(NULL)
    {}
    ~CComputer();
    BOOL Create(LPCTSTR ComputerName);

    operator HMACHINE()
    {
	return m_hMachine;
    }
private:
    BOOL	m_IsLocal;
    HMACHINE	m_hMachine;
};

class CDeviceTree
{
public:
    CDeviceTree() : m_pComputer(NULL)
    {
	m_ImageListData.cbSize = 0;
    }
    ~CDeviceTree()
    {
	if (m_pComputer)
	    delete m_pComputer;
	if (m_ImageListData.cbSize)
	    SetupDiDestroyClassImageList(&m_ImageListData);
    }
    BOOL Create(LPCTSTR MachineName);
    BOOL NewComputer(LPCTSTR MachineName);
    HIMAGELIST GetClassImageList()
    {
	if (m_ImageListData.cbSize)
	    return m_ImageListData.ImageList;
	else
	    return NULL;
    }
    CComputer* GetComputer()
    {
	return m_pComputer;
    }
private:
    BOOL    CreateSubtree(CDevice* pParent, CDevice* pSibling, DEVNODE dn);
    CComputer*	m_pComputer;
    SP_CLASSIMAGELIST_DATA m_ImageListData;
};

#endif
