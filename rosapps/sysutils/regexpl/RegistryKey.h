/* $Id: RegistryKey.h,v 1.4 2001/01/13 23:54:41 narnaoud Exp $ */

// RegistryKey.h: interface for the CRegistryKey class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
#define REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_

class CRegistryKey  
{
public:
  // Constructor. Call InitXXX methods to make real construct.
  CRegistryKey();
  
  // Call this key to init root key.
  //
  // Parameters:
  //   pszMachineName - pointer to buffer containing machine name. NULL means local machine.
  //
  // Return value:
  //   S_OK - All ok.
  //   E_XXX - Error.
	HRESULT InitRoot(const TCHAR *pszMachineName = NULL);

  // Call this method to init normal key.
  //
  // Parameters:
  //   hKey - handle to opened key.
  //   pszPath - optional path string. NULL if pszKeyName is the needed name.
  //   pszKeyName - pointer to buffer conatining name of key.
  //   CurrentAccess - Access of hKey.
  //
  // Remarks:
  //   Constructs key object from handle.
  //   The constructed object hold the handle and closes it on destruction. Do not close handle outside.
  //   If pszPath is not NULL, it is concatenated with pszKeyName.
  //
  // Return value:
  //   S_OK - All ok.
  //   E_XXX - Error.
  HRESULT Init(HKEY hKey, const TCHAR *pszPath, const TCHAR *pszKeyName, REGSAM CurrentAccess);

  // Call this method to uninitialize the object.
  //
  // Return value:
  //   S_OK - All ok.
  //   E_XXX - Error.
  HRESULT Uninit();
  
  // Destructor
	virtual ~CRegistryKey();

  // Call ths function to check if handle to key is handle to hive root.
  //
  // Parameters:
  //   hKey - handle to check.
  //
  // Return value:
  //   TRUE - hKey is handle to hive root.
  //   FALSE - hKey is not handle to hive root.
  static BOOL IsHive(HKEY hKey);

  // Call this method to get name of key represented by  this object.
  //
  // Return value:
  //   Pointer to buffer containing key name. Return value is valid until next call to this object method.
	const TCHAR * GetKeyName();

  BOOL IsRoot();

  // Call this method to open existing subkey of this key.
  //
  // Parameters:
  //   samDesired - deisred access.
  //   pszSubkeyName - pointer to bufer containing name of key to open.
  //   rhKey - reference to variable that receives handle of opened key. If method fails, variable value is unchanged.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG OpenSubkey(REGSAM samDesired, const TCHAR *pszSubkeyName, HKEY &rhKey);
  
  // Call this method to open existing subkey of this key.
  //
  // Parameters:
  //   samDesired - deisred access.
  //   pszSubkeyName - pointer to bufer containing name of key to open.
  //   rKey - reference to CRegistryKey object. If method succeeds, rKey is initialized with newly opened key.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG OpenSubkey(REGSAM samDesired, const TCHAR *pszSubkeyName, CRegistryKey &rKey);

  // Call this method to get the length in TCHARs of longest subkey name, including terminating null.
  //
  // Parameters:
  //   rdwMaxSubkeyNameLength, reference to variable that receives size in TCHARs of longest subkey name.
  //
  // Return value.
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetSubkeyNameMaxLength(DWORD &rdwMaxSubkeyNameLength);

  // Call this method to init subkey enumeration. I.e. before first call to GetSubkeyName()
  //
  // Parameters:
  //   pchSubkeyNameBuffer - pointer to buffer receiving subkey name.
  //   dwBufferSize - size, in TCHARs of buffer pointed by pchSubkeyNameBuffer.
  //
	void InitSubkeyEnumeration(TCHAR *pchSubkeyNameBuffer, DWORD dwBufferSize);

  // Call this method to get next subkey name. Name is stored in buffer specified in call to InitSubKeyEnumeration.
  //
  // Parameters:
  //   pdwActualSize - optional pointer to variable receiving actual size, in TCHARs, of key name. The count returned does not include the terminating null.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
  //   If no more items available, return error is ERROR_NO_MORE_ITEMS.
	LONG GetNextSubkeyName(DWORD *pdwActualSize = NULL);

  // Call this method to get count of subkeys.
  //
  // Parameters:
  //   rdwSubkeyCount - reference to variable that receives subkey count.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetSubkeyCount(DWORD &rdwSubkeyCount);

  // Call this method to get the length in TCHARs of longest value name, including terminating null.
  //
  // Parameters:
  //   rdwMaxValueNameBufferSize receives the length, in TCHARs, of the key's longest value name.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetMaxValueNameLength(DWORD& rdwMaxValueNameBufferSize);

  // Call this method to get the size of larges value data.
  //
  // Parameters:
  //   rdwMaxValueDataBufferSize receives the length, in bytes, of the longest data component among the key's values.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetMaxValueDataSize(DWORD& rdwMaxValueDataBufferSize);

  // Call this method to init subkey enumeration. I.e. before first call to GetSubkeyName()
  //
  // Parameters:
  //   pszValueNameBuffer - pointer to buffer receiving value name. If NULL, value name in not received upon iteration.
  //   dwValueNameBufferSize - size, in TCHARs of buffer pointed by pszValueNameBuffer. If pszValueNameBuffer is NULL, parameter is ignored.
  //   pbValueDataBuffer - pointer to buffer receiving value name. If NULL, value data is not received upon iteration.
  //   dwValueDataBufferSize - size, in bytes of buffer pointed by pbValueDataBuffer. If pbValueDataBuffer is NULL, parameter is ignored.
  //   pdwType - pointer to variable receiving value type. If NULL, value type is not received upon iteration.
	void InitValueEnumeration(TCHAR *pszValueNameBuffer,
                            DWORD dwValueNameBufferSize,
                            BYTE *pbValueDataBuffer,
                            DWORD dwValueDataBufferSize,
                            DWORD *pdwType);
  
  // Call this method to get next value  name/data/type. Name/data/type is/are stored in buffer(s) specified in call to InitValueEnumeration.
  //
  // Parameters:
  //   pdwNameActualSize - optional pointer to variable receiving actual size, in TCHARs, of value name. The count returned includes the terminating null.
  //   pdwActualSize - optional pointer to variable receiving actual size, in bytes, of key name. The count returned does not include the terminating null.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
  //   If no more items available, return error is ERROR_NO_MORE_ITEMS.
	LONG GetNextValue(DWORD *pdwNameActualSize = NULL, DWORD *pdwDataActualSize = NULL);
  
  // Call this method to get count of values.
  //
  // Parameters:
  //   rdwValueCount - reference to variable that receives value count.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetValueCount(DWORD& rdwValueCount);

  // Call this method to get data and/or type of default value.
  //
  // Parameters:
  //   pdwType - optional pointer to variable receiving default value type. NULL if not requred.
  //   pbValueDataBuffer - optional pointer to buffer receiving default value data. NULL if not requred.
  //   dwValueDataBufferSize - size of buffer pointer by pbValueDataBuffer. Ignored if pbValueDataBuffer is NULL.
  //   pdwValueDataActualSize - optional pointer to variable receiving size, in bytes, of data stored into buffer. If pbValueDataBuffer is NULL, returned value is size of default value data, in bytes.
  // 
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG GetDefaultValue(DWORD *pdwType, BYTE *pbValueDataBuffer, DWORD dwValueDataBufferSize, DWORD *pdwValueDataActualSize);

  // Call this function to get text representation of value type.
  //
  // Parameters:
  //   dwType - type to get text representation from.
  //
  // Return value:
  //   text representation od value type.
	static const TCHAR * GetValueTypeName(DWORD dwType);
  
	DWORD GetValue(TCHAR *pchValueName, DWORD *pdwType, LPBYTE lpValueDataBuffer, DWORD *pdwValueDataSize);

  // Call this method to create subkey of this key.
  //
  // Parameters:
  //   samDesired - deisred access.
  //   pszKeyName - pointer to bufer containing name of key to create.
  //   rhKey - reference to variable that receives handle of opened key. If method fails, variable value is unchanged.
  //   pblnOpened - optional pointer to variable that receives create/open status. If subkey is opened value is TRUE. If key is created value is FALSE.
  //   blnVolatile - opitional parameter specifining if created key is volatile.
  //
  // Return value:
  //   If the method succeeds, the return value is ERROR_SUCCESS.
  //   If the method fails, the return value is a nonzero error code defined in winerror.h.
	LONG CreateSubkey(REGSAM samDesired, const TCHAR *pszKeyName, HKEY &rhKey, BOOL *pblnOpened = NULL, BOOL blnVolatile = FALSE);
  
	LONG GetLastWriteTime(SYSTEMTIME& st);
	const TCHAR * GetLastWriteTime();
  
	LONG DeleteValue(const TCHAR *pszValueName);
	LONG DeleteSubkey(const TCHAR *pszPatternSubkeyName);
  
	LONG SetValue(LPCTSTR pszValueName, DWORD dwType, BYTE *lpData, DWORD dwDataSize);
	TCHAR * GetSubKeyNameByIndex(DWORD dwIndex);
	LONG GetSecurityDescriptor(SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, LPDWORD lpcbSecurityDescriptor);
	LONG GetSecurityDescriptorLength(DWORD *pdwSecurityDescriptor);
	BOOL IsPredefined();
	operator HKEY(){return m_hKey;};
private:
	DWORD m_dwCurrentSubKeyIndex;
  TCHAR *m_pchSubkeyNameBuffer;
  DWORD m_dwSubkeyNameBufferSize;
  
	DWORD m_dwCurrentValueIndex;
  TCHAR *m_pszValueNameBuffer;
  DWORD m_dwValueNameBufferSize;
  BYTE *m_pbValueDataBuffer;
  DWORD m_dwValueDataBufferSize;
  DWORD *m_pdwType;
  
	HKEY m_hKey;
	TCHAR *m_pszKeyName;
	TCHAR *m_pszMachineName;
	REGSAM m_CurrentAccess;
};

#endif // !defined(REGISTRYKEY_H__FEF419ED_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
