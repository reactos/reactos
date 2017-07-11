/*
    Copyright 1991-2017 Amebis

    This file is part of atlex.

    Setup is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Setup is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Setup. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "atlex.h"
#include <atlcoll.h>
#include <atlstr.h>
#include <WinCrypt.h>

///
/// \defgroup ATLCryptoAPI Cryptography API
/// Integrates ATL classes with Microsoft Cryptography API
///
/// @{

///
/// Obtains the subject or issuer name from a certificate [CERT_CONTEXT](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377189.aspx) structure and stores it in a ATL::CAtlStringA string.
///
/// \sa [CertGetNameString function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376086.aspx)
///
inline DWORD CertGetNameStringA(_In_ PCCERT_CONTEXT pCertContext, _In_ DWORD dwType, _In_ DWORD dwFlags, _In_ void *pvTypePara, _Out_ ATL::CAtlStringA &sNameString)
{
    // Query the final string length first.
    DWORD dwSize = ::CertGetNameStringA(pCertContext, dwType, dwFlags, pvTypePara, NULL, 0);

    // Allocate buffer on heap to format the string data into and read it.
    LPSTR szBuffer = sNameString.GetBuffer(dwSize);
    if (!szBuffer) return ERROR_OUTOFMEMORY;
    dwSize = ::CertGetNameStringA(pCertContext, dwType, dwFlags, pvTypePara, szBuffer, dwSize);
    sNameString.ReleaseBuffer(dwSize);
    return dwSize;
}


///
/// Obtains the subject or issuer name from a certificate [CERT_CONTEXT](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377189.aspx) structure and stores it in a ATL::CAtlStringW string.
///
/// \sa [CertGetNameString function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376086.aspx)
///
inline DWORD CertGetNameStringW(_In_ PCCERT_CONTEXT pCertContext, _In_ DWORD dwType, _In_ DWORD dwFlags, _In_ void *pvTypePara, _Out_ ATL::CAtlStringW &sNameString)
{
    // Query the final string length first.
    DWORD dwSize = ::CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara, NULL, 0);

    // Allocate buffer on heap to format the string data into and read it.
    LPWSTR szBuffer = sNameString.GetBuffer(dwSize);
    if (!szBuffer) return ERROR_OUTOFMEMORY;
    dwSize = ::CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara, szBuffer, dwSize);
    sNameString.ReleaseBuffer(dwSize);
    return dwSize;
}


///
/// Retrieves data that governs the operations of a hash object. The actual hash value can be retrieved by using this function.
///
/// \sa [CryptGetHashParam function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379947.aspx)
///
inline BOOL CryptGetHashParam(_In_ HCRYPTHASH  hHash, _In_ DWORD dwParam, _Out_ ATL::CAtlArray<BYTE> &aData, _In_ DWORD dwFlags)
{
    DWORD dwHashSize;

    if (CryptGetHashParam(hHash, dwParam, NULL, &dwHashSize, dwFlags)) {
        if (aData.SetCount(dwHashSize)) {
            if (CryptGetHashParam(hHash, dwParam, aData.GetData(), &dwHashSize, dwFlags)) {
                return TRUE;
            } else {
                aData.SetCount(0);
                return FALSE;
            }
        } else {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    } else
        return FALSE;
}


///
/// Exports a cryptographic key or a key pair from a cryptographic service provider (CSP) in a secure manner.
///
/// \sa [CryptExportKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379931.aspx)
///
inline BOOL CryptExportKey(_In_ HCRYPTKEY hKey, _In_ HCRYPTKEY hExpKey, _In_ DWORD dwBlobType, _In_ DWORD dwFlags, _Out_ ATL::CAtlArray<BYTE> &aData)
{
    DWORD dwKeyBLOBSize;

    if (CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, NULL, &dwKeyBLOBSize)) {
        if (aData.SetCount(dwKeyBLOBSize)) {
            if (CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, aData.GetData(), &dwKeyBLOBSize)) {
                return TRUE;
            } else {
                aData.SetCount(0);
                return FALSE;
            }
        } else {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    } else
        return FALSE;
}

/// @}


namespace ATL
{
    namespace Crypt
    {
        /// \addtogroup ATLCryptoAPI
        /// @{

        ///
        /// PCCERT_CONTEXT wrapper class
        ///
        class CCertContext : public ATL::CObjectWithHandleDuplT<PCCERT_CONTEXT>
        {
        public:
            ///
            /// Destroys the certificate context.
            ///
            /// \sa [CertFreeCertificateContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376075.aspx)
            ///
            virtual ~CCertContext()
            {
                if (m_h)
                    CertFreeCertificateContext(m_h);
            }

            ///
            /// Creates the certificate context.
            ///
            /// \return
            /// - TRUE when creation succeeds;
            /// - FALSE when creation fails. For extended error information, call `GetLastError()`.
            /// \sa [CertCreateCertificateContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376033.aspx)
            ///
            inline BOOL Create(_In_  DWORD dwCertEncodingType, _In_  const BYTE *pbCertEncoded, _In_  DWORD cbCertEncoded)
            {
                HANDLE h = CertCreateCertificateContext(dwCertEncodingType, pbCertEncoded, cbCertEncoded);
                if (h) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Destroys the certificate context.
            ///
            /// \sa [CertFreeCertificateContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376075.aspx)
            ///
            virtual void InternalFree()
            {
                CertFreeCertificateContext(m_h);
            }

            ///
            /// Duplicates the certificate context.
            ///
            /// \param[in] h Object handle of existing certificate context
            /// \return Duplicated certificate context handle
            /// \sa [CertDuplicateCertificateContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376045.aspx)
            ///
            virtual HANDLE InternalDuplicate(_In_ HANDLE h) const
            {
                return CertDuplicateCertificateContext(h);
            }
        };


        ///
        /// PCCERT_CHAIN_CONTEXT wrapper class
        ///
        class CCertChainContext : public ATL::CObjectWithHandleDuplT<PCCERT_CHAIN_CONTEXT>
        {
        public:
            ///
            /// Destroys the certificate chain context.
            ///
            /// \sa [CertFreeCertificateChain function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376073.aspx)
            ///
            virtual ~CCertChainContext()
            {
                if (m_h)
                    CertFreeCertificateChain(m_h);
            }

            ///
            /// Creates the certificate chain context.
            ///
            /// \return
            /// - TRUE when creation succeeds;
            /// - FALSE when creation fails. For extended error information, call `GetLastError()`.
            /// \sa [CertGetCertificateChain function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376078.aspx)
            ///
            inline BOOL Create(_In_opt_ HCERTCHAINENGINE hChainEngine, _In_ PCCERT_CONTEXT pCertContext, _In_opt_ LPFILETIME pTime, _In_opt_ HCERTSTORE hAdditionalStore, _In_ PCERT_CHAIN_PARA pChainPara, _In_ DWORD dwFlags, __reserved LPVOID pvReserved)
            {
                HANDLE h;
                if (CertGetCertificateChain(hChainEngine, pCertContext, pTime, hAdditionalStore, pChainPara, dwFlags, pvReserved, &h)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Destroys the certificate chain context.
            ///
            /// \sa [CertFreeCertificateChain function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376073.aspx)
            ///
            virtual void InternalFree()
            {
                CertFreeCertificateChain(m_h);
            }

            ///
            /// Duplicates the certificate chain context.
            ///
            /// \param[in] h Object handle of existing certificate chain context
            /// \return Duplicated certificate chain context handle
            /// \sa [CertDuplicateCertificateContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376045.aspx)
            ///
            virtual HANDLE InternalDuplicate(_In_ HANDLE h) const
            {
                return CertDuplicateCertificateChain(h);
            }
        };


        ///
        /// HCERTSTORE wrapper class
        ///
        class CCertStore : public ATL::CObjectWithHandleT<HCERTSTORE>
        {
        public:
            ///
            /// Closes the certificate store.
            ///
            /// \sa [CertCloseStore function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376026.aspx)
            ///
            virtual ~CCertStore()
            {
                if (m_h)
                    CertCloseStore(m_h, 0);
            }

            ///
            /// Opens the certificate store.
            ///
            /// \return
            /// - TRUE when creation succeeds;
            /// - FALSE when creation fails. For extended error information, call `GetLastError()`.
            /// \sa [CertOpenStore function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376559.aspx)
            ///
            inline BOOL Create(_In_ LPCSTR lpszStoreProvider, _In_ DWORD dwEncodingType, _In_opt_ HCRYPTPROV_LEGACY hCryptProv, _In_ DWORD dwFlags, _In_opt_ const void *pvPara)
            {
                HANDLE h = CertOpenStore(lpszStoreProvider, dwEncodingType, hCryptProv, dwFlags, pvPara);
                if (h) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Closes the certificate store.
            ///
            /// \sa [CertCloseStore function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa376026.aspx)
            ///
            virtual void InternalFree()
            {
                CertCloseStore(m_h, 0);
            }
        };


        ///
        /// HCRYPTPROV wrapper class
        ///
        class CContext : public ATL::CObjectWithHandleT<HCRYPTPROV>
        {
        public:
            ///
            /// Releases the cryptographi context.
            ///
            /// \sa [CryptReleaseContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa380268.aspx)
            ///
            virtual ~CContext()
            {
                if (m_h)
                    CryptReleaseContext(m_h, 0);
            }

            ///
            /// Acquires the cryptographic context.
            ///
            /// \return
            /// - TRUE when creation succeeds;
            /// - FALSE when creation fails. For extended error information, call `GetLastError()`.
            /// \sa [CryptAcquireContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379886.aspx)
            ///
            inline BOOL Create(_In_opt_ LPCTSTR szContainer, _In_opt_ LPCTSTR szProvider, _In_ DWORD dwProvType, _In_ DWORD dwFlags)
            {
                HANDLE h;
                if (CryptAcquireContext(&h, szContainer, szProvider, dwProvType, dwFlags)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Releases the cryptographic context.
            ///
            /// \sa [CryptReleaseContext function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa380268.aspx)
            ///
            virtual void InternalFree()
            {
                CryptReleaseContext(m_h, 0);
            }
        };


        ///
        /// HCRYPTHASH wrapper class
        ///
        class CHash : public ATL::CObjectWithHandleDuplT<HCRYPTHASH>
        {
        public:
            ///
            /// Destroys the hash context.
            ///
            /// \sa [CryptDestroyHash function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379917.aspx)
            ///
            virtual ~CHash()
            {
                if (m_h)
                    CryptDestroyHash(m_h);
            }

            ///
            /// Creates the hash context.
            ///
            /// \return
            /// - TRUE when creation succeeds;
            /// - FALSE when creation fails. For extended error information, call `GetLastError()`.
            /// \sa [CryptCreateHash function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379908.aspx)
            ///
            inline BOOL Create(_In_ HCRYPTPROV  hProv, _In_ ALG_ID Algid, _In_ HCRYPTKEY hKey, _In_ DWORD dwFlags)
            {
                HANDLE h;
                if (CryptCreateHash(hProv, Algid, hKey, dwFlags, &h)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Destroys the hash context.
            ///
            /// \sa [CryptDestroyHash function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379917.aspx)
            ///
            virtual void InternalFree()
            {
                CryptDestroyHash(m_h);
            }

            ///
            /// Duplicates the hash context.
            ///
            /// \param[in] h Object handle of existing hash context
            /// \return Duplicated hash context handle
            /// \sa [CryptDuplicateHash function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379919.aspx)
            ///
            virtual HANDLE InternalDuplicate(_In_ HANDLE h) const
            {
                HANDLE hNew = NULL;
                return CryptDuplicateHash(h, NULL, 0, &hNew) ? hNew : NULL;
            }
        };


        ///
        /// HCRYPTKEY wrapper class
        ///
        class CKey : public ATL::CObjectWithHandleDuplT<HCRYPTKEY>
        {
        public:
            ///
            /// Destroys the key.
            ///
            /// \sa [CryptDestroyKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379918.aspx)
            ///
            virtual ~CKey()
            {
                if (m_h)
                    CryptDestroyKey(m_h);
            }

            ///
            /// Generates the key.
            ///
            /// \sa [CryptGenKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379941.aspx)
            ///
            inline BOOL Generate(_In_ HCRYPTPROV hProv, _In_ ALG_ID Algid, _In_ DWORD dwFlags)
            {
                HANDLE h;
                if (CryptGenKey(hProv, Algid, dwFlags, &h)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

            ///
            /// Imports the key.
            ///
            /// \sa [CryptImportKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa380207.aspx)
            ///
            inline BOOL Import(_In_ HCRYPTPROV hProv, __in_bcount(dwDataLen) CONST BYTE *pbData, _In_ DWORD dwDataLen, _In_ HCRYPTKEY hPubKey, _In_ DWORD dwFlags)
            {
                HANDLE h;
                if (CryptImportKey(hProv, pbData, dwDataLen, hPubKey, dwFlags, &h)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

            ///
            /// Imports the public key.
            ///
            /// \sa [CryptImportPublicKeyInfo function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa380209.aspx)
            ///
            inline BOOL ImportPublic(_In_ HCRYPTPROV hCryptProv, _In_ DWORD dwCertEncodingType, _In_ PCERT_PUBLIC_KEY_INFO pInfo)
            {
                HANDLE h;
                if (CryptImportPublicKeyInfo(hCryptProv, dwCertEncodingType, pInfo, &h)) {
                    Attach(h);
                    return TRUE;
                } else
                    return FALSE;
            }

        protected:
            ///
            /// Destroys the key.
            ///
            /// \sa [CryptDestroyKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379918.aspx)
            ///
            virtual void InternalFree()
            {
                CryptDestroyKey(m_h);
            }

            ///
            /// Duplicates the key.
            ///
            /// \param[in] h Object handle of existing key
            /// \return Duplicated key handle
            /// \sa [CryptDuplicateKey function](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379920.aspx)
            ///
            virtual HANDLE InternalDuplicate(_In_ HANDLE h) const
            {
                HANDLE hNew = NULL;
                return CryptDuplicateKey(h, NULL, 0, &hNew) ? hNew : NULL;
            }
        };

        /// @}
    }
}
