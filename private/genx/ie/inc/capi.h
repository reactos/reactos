#include <wincrypt.h>
#include <sipbase.h>
#include <mscat.h>
#include <mssip.h>
#include <wintrust.h>

#ifndef _JTRUST_H
#define _JTRUST_H

#if !defined(JAVA_TRUST_PROVIDER)

#ifdef __cplusplus
extern "C" {
#endif


// New guids for Java Policy Provider
// {E6F795B1-F738-11d0-A72F-00A0C903B83D}
#define JAVA_POLICY_PROVIDER_DOWNLOAD \
{ 0xe6f795b1, 0xf738, 0x11d0, {0xa7, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0xb8, 0x3d } }

// {E6F795B2-F738-11d0-A72F-00A0C903B83D}
#define JAVA_POLICY_PROVIDER_CHECK \
{ 0xe6f795b2, 0xf738, 0x11d0, {0xa7, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0xb8, 0x3d } }

typedef struct _JAVA_TRUST {
    DWORD       cbSize;                   // Size of structure
    DWORD       flag;                     // Reserved
    BOOL        fAllActiveXPermissions;   // ActiveX explicitly asked for all (must have been signed)
    BOOL        fAllPermissions;          // Java permissions, explicit ask for all
    DWORD       dwEncodingType;           // Encoding type
    PBYTE       pbJavaPermissions;        // Encoded java permission blob
    DWORD       cbJavaPermissions;
    PBYTE       pbSigner;                 // Encoded signer.
    DWORD       cbSigner;
    LPCWSTR     pwszZone;                 // Zone index (copied from action data)
    GUID        guidZone;                 // Not used currently
    HRESULT     hVerify;                  // Authenticode policy return
} JAVA_TRUST, *PJAVA_TRUST;

typedef struct _JAVA_POLICY_PROVIDER {
    DWORD                 cbSize;                   // Size of policy provider
    LPVOID                pZoneManager;             // Zone interface manager
    LPCWSTR               pwszZone;                 // Zone index
    BOOL                  fNoBadUI;                 // Optional bad ui
    PJAVA_TRUST           pbJavaTrust;              // Returned java information (CoTaskMemAlloc)
    DWORD                 cbJavaTrust;              // Total allocated size of pJavaTrust
    DWORD                 dwActionID;               // Optional ActionID ID
    DWORD                 dwUnsignedActionID;       // Optional ActionID ID
    BOOL                  VMBased;                  // Called from VM (FALSE by DEFAULT)
} JAVA_POLICY_PROVIDER, *PJAVA_POLICY_PROVIDER;

#ifdef __cplusplus
}
#endif

#endif // !defined(JAVA_TRUST_PROVIDER)
#endif // _JTRUST_H
