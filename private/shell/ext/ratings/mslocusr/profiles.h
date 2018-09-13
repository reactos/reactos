void AddBackslash(LPSTR lpPath);
void AddBackslash(NLS_STR& nlsPath);
BOOL FileExists(LPCSTR pszPath);
BOOL DirExists(LPCSTR pszPath);
UINT SafeCopy(LPCSTR pszSrc, LPCSTR pszDest, DWORD dwAttrs);
HRESULT GiveUserDefaultProfile(LPCSTR pszPath);
HRESULT CopyProfile(LPCSTR pszSrcPath, LPCSTR pszDestPath);
LONG MyRegLoadKey(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszFile);
HRESULT ReconcileFiles(HKEY hkeyProfile, NLS_STR& nlsProfilePath, NLS_STR& nlsOtherProfilePath);
HRESULT DefaultReconcileKey(HKEY hkeyProfile, NLS_STR& nlsProfilePath,
                            LPCSTR pszKeyName, BOOL fSecondary);
void ComputeLocalProfileName(LPCSTR pszUsername, NLS_STR *pnlsLocalProfile);
HRESULT DeleteProfile(LPCSTR pszName);
BOOL UseUserProfiles(void);
void EnableProfiles(void);
LONG OpenLogonKey(HKEY *phkey);
BOOL CreateDirectoryPath(LPCSTR pszPath);

#ifdef REGENTRY_INC
void GetSetRegistryPath(HKEY hkeyProfile, RegEntry& re, NLS_STR *pnlsPath, BOOL fSet);
#endif