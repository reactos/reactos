// PermPage.h : Declaration of the standard permissions page class

#ifndef __PERMPAGE_H_INCLUDED__
#define __PERMPAGE_H_INCLUDED__

#include "aclui.h"

class CSecurityInformation : public ISecurityInformation, public CComObjectRoot
{
    DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
    BEGIN_COM_MAP(CSecurityInformation)
        COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo ) = 0;
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault ) = 0;
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor ) = 0;
    STDMETHOD(GetAccessRights) (const GUID* pguidObjectType,
                                DWORD dwFlags,
                                PSI_ACCESS *ppAccess,
                                ULONG *pcAccesses,
                                ULONG *piDefaultAccess );
    STDMETHOD(MapGeneric) (const GUID *pguidObjectType,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pMask);
    STDMETHOD(GetInheritTypes) (PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes );
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage );

protected:
	HRESULT NewDefaultDescriptor(
		PSECURITY_DESCRIPTOR* ppsd,
		SECURITY_INFORMATION RequestedInformation
		);

	// this will throw a memory exception where appropriate
	HRESULT MakeSelfRelativeCopy(
		PSECURITY_DESCRIPTOR  psdOriginal,
		PSECURITY_DESCRIPTOR* ppsdNew );
};

class CShareSecurityInformation : public CSecurityInformation
{
private:
	LPWSTR m_strMachineName;
	LPWSTR m_strShareName;
public:
	void SetMachineName( LPWSTR pszMachineName )
	{
		m_strMachineName = pszMachineName;
	}
	void SetShareName( LPWSTR pszShareName )
	{
		m_strShareName = pszShareName;
	}
	// note: these should be LPCTSTR but are left this way for convenience
	LPWSTR QueryMachineName()
	{
		return m_strMachineName;
	}
	LPWSTR QueryShareName()
	{
		return m_strShareName;
	}

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo );
};

class CSMBSecurityInformation : public CShareSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );
public:
	// SHARE_INFO_502* m_pvolumeinfo;
	PSECURITY_DESCRIPTOR m_pInitialDescriptor;
	PSECURITY_DESCRIPTOR* m_ppCurrentDescriptor;
	CSMBSecurityInformation();
	~CSMBSecurityInformation();
};


#endif // ~__PERMPAGE_H_INCLUDED__
