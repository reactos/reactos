#include <windows.h>
#include <wincrypt.h>
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptAcquireContextA(HCRYPTPROV *phProv, LPCSTR pszContainer,
				   LPCSTR pszProvider, DWORD dwProvType,
				   DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptAcquireContextW (HCRYPTPROV *phProv, LPCWSTR pszContainer,
		LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGenRandom (HCRYPTPROV hProv, DWORD dwLen, BYTE *pbBuffer)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptContextAddRef (HCRYPTPROV hProv, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptCreateHash (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDecrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDeriveKey (HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData,
		DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDestroyHash (HCRYPTHASH hHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDestroyKey (HCRYPTKEY hKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDuplicateKey (HCRYPTKEY hKey, DWORD *pdwReserved, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptDuplicateHash (HCRYPTHASH hHash, DWORD *pdwReserved,
		DWORD dwFlags, HCRYPTHASH *phHash)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptEncrypt (HCRYPTKEY hKey, HCRYPTHASH hHash, WINBOOL Final,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptEnumProvidersA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptEnumProvidersW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptEnumProviderTypesA (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPSTR pszTypeName, DWORD *pcbTypeName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptEnumProviderTypesW (DWORD dwIndex, DWORD *pdwReserved,
		DWORD dwFlags, DWORD *pdwProvType, LPWSTR pszTypeName, DWORD *pcbTypeName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptExportKey (HCRYPTKEY hKey, HCRYPTKEY hExpKey, DWORD dwBlobType,
		DWORD dwFlags, BYTE *pbData, DWORD *pdwDataLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGenKey (HCRYPTPROV hProv, ALG_ID Algid, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData,
		DWORD *pdwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetDefaultProviderA (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetDefaultProviderW (DWORD dwProvType, DWORD *pdwReserved,
		DWORD dwFlags, LPWSTR pszProvName, DWORD *pcbProvName)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptGetUserKey (HCRYPTPROV hProv, DWORD dwKeySpec, HCRYPTKEY *phUserKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptHashData (HCRYPTHASH hHash, BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptHashSessionKey (HCRYPTHASH hHash, HCRYPTKEY hKey, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptImportKey (HCRYPTPROV hProv, BYTE *pbData, DWORD dwDataLen,
		HCRYPTKEY hPubKey, DWORD dwFlags, HCRYPTKEY *phKey)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptReleaseContext (HCRYPTPROV hProv, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSignHashA (HCRYPTHASH hHash, DWORD dwKeySpec, LPCSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSignHashW (HCRYPTHASH hHash, DWORD dwKeySpec, LPCWSTR sDescription,
		DWORD dwFlags, BYTE *pbSignature, DWORD *pdwSigLen)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetHashParam (HCRYPTHASH hHash, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetKeyParam (HCRYPTKEY hKey, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetProviderA (LPCSTR pszProvName, DWORD dwProvType)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetProviderW (LPCWSTR pszProvName, DWORD dwProvType)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetProviderExA (LPCSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetProviderExW (LPCWSTR pszProvName, DWORD dwProvType, DWORD *pdwReserved, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptSetProvParam (HCRYPTPROV hProv, DWORD dwParam, BYTE *pbData, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptVerifySignatureA (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCSTR sDescription, DWORD dwFlags)
{
  return(FALSE);
}
/*
 * @unimplemented
 */
WINBOOL STDCALL CryptVerifySignatureW (HCRYPTHASH hHash, BYTE *pbSignature, DWORD dwSigLen,
		HCRYPTKEY hPubKey, LPCWSTR sDescription, DWORD dwFlags)
{
  return(FALSE);
}
