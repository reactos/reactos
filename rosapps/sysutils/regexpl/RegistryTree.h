/* $Id: RegistryTree.h,v 1.2 2001/01/10 01:25:29 narnaoud Exp $ */

// RegistryTree.h: interface for the CRegistryTree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_)
#define REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_

#include "RegistryKey.h"

// Max size of error description.
#define ERROR_MSG_BUFFER_SIZE	1024

class CRegistryTree  
{
public:
	// Constructor
  //
	// Parameters:
	//		nMaxPathSize - size in characters of longest path including terminating NULL char
	CRegistryTree();

  // Destructor
	virtual ~CRegistryTree();

  // Call this function after fail of this class method.
  //
  // Return value:
  //   Pointer to buffer containing description of last error.
  //   return value is valid until next method of this class is called.
	const TCHAR * GetLastErrorDescription();

  // Call this function to get string representation (path) of current key.
  //
  // Return value:
  //   Pointer to buffer containing current key path. The pointer is valid until next call to this objet method.
	const TCHAR * GetCurrentPath() const;

  // Call this function to check if current key is the root key.
  //
  // Return value:
  //   FALSE - current key is not the root key.
  //   TRUE - current key is the root key.
	BOOL IsCurrentRoot();

  // Call this function to change the current key.
  //
  // Parameters:
  //   pchRelativePath - relative path to target key.
  //
  // Return value:
  //   TRUE - current key changed successfully.
  //   FALSE - failed to change current key. Call GetLastErrorDescription() to get error description.
	BOOL ChangeCurrentKey(const TCHAR *pchRelativePath);

  // Call this function to obtain key at relative path and opened with desired access.
  //
  // Parametes:
  //   pchRelativePath - path to key to be opened.
  //   DesiredAccess - desired access to key.
  //   rKey - reference to variable that receives pointer to key. Caller must free object with delete operator, when object is not longer needed.
  //
  // Return value:
  //   TRUE - key opened successfully.
  //   FALSE - failed to open desired key path size. Call GetLastErrorDescription() to get error description.
  BOOL GetKey(const TCHAR *pchRelativePath, REGSAM DesiredAccess, CRegistryKey& rKey);

  // Call this function to delete key subkeys.
  //
  // Parameters:
  //   pszKeyPattern - pattern to specifying which subkeys to delete.
  //   pszPath - path to key which subkeys will be deleted.
  //   blnRecursive - if FALSE and particular subkey has subkeys, it will not be deleted.
  //
  // Return value:
  //   TRUE - key opened successfully.
  //   FALSE - error. Call GetLastErrorDescription() to get error description.
	BOOL DeleteSubkeys(const TCHAR *pszKeyPattern, const TCHAR *pszPath, BOOL blnRecursive = FALSE);
  
	BOOL NewKey(const TCHAR *pszKeyName, const TCHAR *pszPath, BOOL blnVolatile = FALSE);
  
	BOOL SetMachineName(LPCTSTR pszMachineName);

// Internal methods
private:
	CRegistryTree(const CRegistryTree& Tree);

  // returns description of error value returned by RegXXXX functions in advapi32.
  const TCHAR *GetErrorDescription(LONG nError);

  void SetError(LONG nError);
  void SetError(const TCHAR *pszFormat, ...);
  void SetErrorCommandNAOnRoot(const TCHAR *pszCommand);
  void SetInternalError();
  void AddErrorDescription(const TCHAR *pszFormat, ...);

  BOOL InternalChangeCurrentKey(const TCHAR *pszSubkeyName, REGSAM DesiredAccess);
  BOOL InternalGetSubkey(const TCHAR *pszSubkeyName, REGSAM DesiredAccess, CRegistryKey& rKey);
  void GotoRoot();
  BOOL DeleteSubkeys(CRegistryKey& rKey, const TCHAR *pszKeyPattern, BOOL blnRecursive);
  
private:
  class CNode
  {
  public:
    CNode *m_pUp;
    CRegistryKey m_Key;
  } m_Root;
  
	CNode *m_pCurrentKey;              // The current key.
	TCHAR m_ErrorMsg[ERROR_MSG_BUFFER_SIZE+1];   // Last error description buffer.
	LPTSTR m_pszMachineName;                     // Pointer to buffer containing machine name with leading backslashes. NULL if local.
};

#endif // !defined(REGISTRYTREE_H__239A6461_70F2_11D3_9085_204C4F4F5020__INCLUDED_)
