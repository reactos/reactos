//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// utils.h 
//
//   Misc routines.
//
//   History:
//
//       6/25/97  tnoonan   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _UTILS_H_

#define _UTILS_H_

#define MAX_DEFCHANNELNAME  64

HRESULT GetURLFromIni(LPCTSTR pszPath, BSTR* pbstrURL);
HRESULT GetNameAndURLAndSubscriptionInfo(LPCTSTR pszPath, BSTR* pbstrName,
                                         BSTR* pbstrURL, SUBSCRIPTIONINFO* psi);
int CDFMessageBox(HWND hwnd, UINT idTextFmt, UINT idCaption, UINT uType, ...);
BOOL DownloadCdfUI(HWND hwnd, LPCWSTR szURL, IXMLDocument* pIMLDocument);

BOOL IsGlobalOffline(void);
void SetGlobalOffline(BOOL fOffline);

BOOL CanSubscribe(LPCWSTR pwszURL);

#endif

