// RegistryTree.h: interface for the CRegistryTree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_)
#define REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_

#include "RegistryKey.h"

#define ERROR_MSG_BUFFER_SIZE	1024

class CRegistryTree  
{
public:
	BOOL DeleteKey(const TCHAR *pchKeyName, BOOL blnRecursive = FALSE);
	BOOL NewKey(const TCHAR *pchKeyName, BOOL blnVolatile = FALSE);
	void SetMachineName(LPCTSTR pszMachineName);
	int ConnectRegistry(HKEY hKey);
	REGSAM GetDesiredOpenKeyAccess() const;
	void SetDesiredOpenKeyAccess(REGSAM samDesired);
	CRegistryKey * GetCurrentKey();
	TCHAR * GetLastErrorDescription();
	BOOL ChangeCurrentKey(const TCHAR *pchRelativePath);
	BOOL IsCurrentRoot();
	const TCHAR * GetCurrentPath() const;

	// Constructor
	// Parameters:
	//		nMaxPathSize - size in characters of longest path including terminating NULL char
	CRegistryTree(unsigned int nMaxPathSize);
	CRegistryTree(const CRegistryTree& Tree);

	virtual ~CRegistryTree();
private:
	unsigned int m_nMaxPathSize;
	TCHAR *m_ChangeKeyBuffer;
	CRegistryKey *m_pRoot, *m_pCurrentKey;
	TCHAR m_ErrorMsg[ERROR_MSG_BUFFER_SIZE+1];
	REGSAM m_samDesiredOpenKeyAccess;
	LPTSTR m_pszMachineName;
};

#endif // !defined(REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_)
