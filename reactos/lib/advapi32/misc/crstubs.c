#include <windows.h>
#include <wincrypt.h>
/*
 * @unimplemented
 */
BOOL STDCALL CryptAcquireContextA(HCRYPTPROV *phProv, LPCSTR pszContainer,
				   LPCSTR pszProvider, DWORD dwProvType,
				   DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDestroyHash (HCRYPTHASH hHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDestroyKey (HCRYPTKEY hKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptHashData (HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptImportKey (HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptVerifySignatureA (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
BOOL STDCALL CryptVerifySignatureW (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags)
{
  return(FALSE);
}
