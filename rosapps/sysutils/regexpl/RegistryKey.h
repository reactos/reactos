// RegistryKey.h: interface for the CRegistryKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
#define REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_

class CRegistryKey  
{
public:
	DWORD SetValue(LPCTSTR pszValueName, DWORD dwType, BYTE *lpData, DWORD dwDataSize);
	DWORD Delete(BOOL blnRecursive);
	DWORD DeleteSubkey(LPCTSTR pszSubKey, BOOL blnRecursive = FALSE);
	DWORD Create(REGSAM samDesired, DWORD *pdwDisposition = NULL, BOOL blnVolatile = FALSE);
	TCHAR * GetSubKeyNameByIndex(DWORD dwIndex);
	DWORD GetValuesCount();
	DWORD GetSubKeyCount();
	LONG GetSecurityDescriptor(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, LPDWORD lpcbSecurityDescriptor);
	DWORD GetSecurityDescriptorLength(DWORD *pdwSecurityDescriptor);
	DWORD GetValue(TCHAR *pchValueName, DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize);
	static const TCHAR * GetValueTypeName(DWORD dwType);
	void LinkParent();
	DWORD GetDefaultValue(DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize);
	BOOL IsPredefined();
	DWORD Open(REGSAM samDesired);
	DWORD GetMaxValueNameLength(DWORD& dwMaxValueNameBuferSize);
	DWORD GetMaxValueDataSize(DWORD& dwMaxValueDataBuferSize);
	TCHAR * GetLastWriteTime();
	void GetLastWriteTime(SYSTEMTIME& st);
	DWORD GetNextValue(TCHAR *pchValueNameBuffer,DWORD& dwValueNameSize, DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize);
	void InitValueEnumeration();
	void UpdateKeyNameCase();
	TCHAR * GetSubKeyName(DWORD& dwError);
	void InitSubKeyEnumeration();
	CRegistryKey * UpOneLevel();
	operator HKEY(){return m_hKey;};
	CRegistryKey * GetParent();
	class CRegistryKey * GetChild();
	TCHAR * GetKeyName();
	CRegistryKey(const TCHAR *pchKeyName, class CRegistryKey *pParent);
	CRegistryKey(HKEY hKey, LPCTSTR pszMachineName = NULL);
	virtual ~CRegistryKey();
private:
	DWORD m_dwCurrentSubKeyIndex;
	DWORD m_dwCurrentValueIndex;
	HKEY m_hKey;
	class CRegistryKey *m_pChild;
	class CRegistryKey *m_pParent;
	TCHAR m_pchKeyName[MAX_PATH+1];
//	TCHAR *m_pchValueName;
	TCHAR m_pszMachineName[MAX_PATH+1];
};

#endif // !defined(REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
