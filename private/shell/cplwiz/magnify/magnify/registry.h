/******************************************************************************
Module name: Registry.h
Written by:  Jeffrey Richter
Purpose:     C++ Class to make working with the registry easier.
******************************************************************************/


#ifndef __REGISTRY_H__
#define __REGISTRY_H__


class CRegSettings {
public:
   CRegSettings();
   ~CRegSettings();

   LONG OpenSubkey(BOOL fReadOnly, HKEY hkeyRoot, LPCTSTR pszSubkey);
   void CloseKey();

   LONG GetBOOL(LPCTSTR pszValueName, PBOOL pf);
   LONG PutBOOL(LPCTSTR pszValueName, BOOL f);

   LONG GetDWORD(LPCTSTR pszValueName, PDWORD pdw);
   LONG PutDWORD(LPCTSTR pszValueName, DWORD dw);

   LONG GetString(LPCTSTR pszValueName, LPTSTR psz, int nMaxSize);
   LONG PutString(LPCTSTR pszValueName, LPCTSTR psz);
   
   LONG GetBinary(LPCTSTR pszValueName, PBYTE pb, PDWORD pcbData);
   LONG PutBinary(LPCTSTR pszValueName, CONST BYTE* pb, int nSize);

private:
   HKEY  m_hkeySubkey;
};

inline CRegSettings::CRegSettings() { m_hkeySubkey = NULL; }
inline CRegSettings::~CRegSettings() { CloseKey(); }

inline void CRegSettings::CloseKey() {
   if (m_hkeySubkey != NULL) { RegCloseKey(m_hkeySubkey); m_hkeySubkey = NULL; }
}

inline LONG CRegSettings::OpenSubkey(BOOL fReadOnly, HKEY hkeyRoot, LPCTSTR pszSubkey) {
   CloseKey();
   LONG lError;
   if (fReadOnly) {
      lError = RegOpenKeyEx(hkeyRoot, pszSubkey, 0, KEY_QUERY_VALUE, &m_hkeySubkey); 
   } else {
      DWORD dwDisposition;
      lError = RegCreateKeyEx(hkeyRoot, pszSubkey, 0, NULL, 
         REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &m_hkeySubkey, &dwDisposition);
   }
   return(lError);
}


inline LONG CRegSettings::GetBOOL(LPCTSTR pszValueName, PBOOL pf) {
   return(GetDWORD(pszValueName, (PDWORD) pf));
}


inline LONG CRegSettings::PutBOOL(LPCTSTR pszValueName, BOOL f) {
   return(PutDWORD(pszValueName, (DWORD) f));
}


inline LONG CRegSettings::GetDWORD(LPCTSTR pszValueName, PDWORD pdw) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   DWORD cbData = sizeof(pdw);
   return(RegQueryValueEx(m_hkeySubkey, pszValueName, NULL, NULL, (LPBYTE) pdw, &cbData));
}


inline LONG CRegSettings::PutDWORD(LPCTSTR pszValueName, DWORD dw) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   return(RegSetValueEx(m_hkeySubkey, pszValueName, 0, REG_DWORD, (CONST BYTE*) &dw, sizeof(dw)));
}


inline LONG CRegSettings::GetString(LPCTSTR pszValueName, LPTSTR psz, int nMaxSize) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   DWORD cbData = nMaxSize;
   return(RegQueryValueEx(m_hkeySubkey, pszValueName, NULL, NULL, (LPBYTE) psz, &cbData));
}


inline LONG CRegSettings::PutString(LPCTSTR pszValueName, LPCTSTR psz) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   return(RegSetValueEx(m_hkeySubkey, pszValueName, 0, REG_SZ, (CONST BYTE*) psz, lstrlen(psz) + 1));
}


inline LONG CRegSettings::GetBinary(LPCTSTR pszValueName, PBYTE pb, PDWORD pcbData) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   return(RegQueryValueEx(m_hkeySubkey, pszValueName, NULL, NULL, pb, pcbData));
}


inline LONG CRegSettings::PutBinary(LPCTSTR pszValueName, CONST BYTE* pb, int nSize) {
   ASSERT(m_hkeySubkey != NULL); // No subkey is opened
   return(RegSetValueEx(m_hkeySubkey, pszValueName, 0, REG_BINARY, pb, nSize));
}


#endif   // __REGISTRY_H__


//////////////////////////////// End of File //////////////////////////////////
