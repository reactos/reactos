// SecurityDescriptor.h: interface for the CSecurityDescriptor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SECURITYDESCRIPTOR_H__71D0A7E6_8A00_11D3_9103_204C4F4F5020__INCLUDED_)
#define SECURITYDESCRIPTOR_H__71D0A7E6_8A00_11D3_9103_204C4F4F5020__INCLUDED_

BOOL GetTextualSid(
    PSID pSid,            // binary Sid
    LPTSTR TextualSid,    // buffer for Textual representation of Sid
    LPDWORD lpdwBufferLen // required/provided TextualSid buffersize
    );

const TCHAR * GetSidTypeName(SID_NAME_USE Use);

class CSecurityDescriptor  
{
public:
	void GetCurrentACE_AccessMask(DWORD& dwMask);
	PSID GetCurrentACE_SID();
	enum ACEntryType
	{
		Unknown,
		AccessAlowed,
		AccessDenied,
		SystemAudit
	};
	ACEntryType GetDACLEntry(DWORD nIndex);
	ACEntryType GetSACLEntry(DWORD nIndex, BOOL& blnFailedAccess, BOOL& blnSeccessfulAccess);
	DWORD GetDACLEntriesCount();
	DWORD GetSACLEntriesCount();
	BOOL HasValidDACL();
	BOOL HasNULLDACL();
	BOOL HasValidSACL();
	BOOL HasNULLSACL();
	BOOL DescriptorContainsDACL();
	BOOL DescriptorContainsSACL();
	DWORD BeginDACLInteration();
	DWORD BeginSACLInteration();
	void AssociateDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor);
	CSecurityDescriptor();
	virtual ~CSecurityDescriptor();
private:
	PSECURITY_DESCRIPTOR m_pSecurityDescriptor;
	BOOL m_blnDACLPresent;
	BOOL m_blnDACLDefaulted;
	PACL m_pDACL;
	BOOL m_blnSACLPresent;
	BOOL m_blnSACLDefaulted;
	PACL m_pSACL;
	ACE_HEADER *m_pCurrentACEHeader;
};

#endif // !defined(SECURITYDESCRIPTOR_H__71D0A7E6_8A00_11D3_9103_204C4F4F5020__INCLUDED_)
