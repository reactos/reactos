
#ifndef _AWTHUNK_H_
#define _AWTHUNK_H_

// The following functions were originally only TCHAR versions
// in Win95, but now have A/W versions.  Since we still need to
// run on Win95, we need to treat them as TCHAR versions and
// undo the A/W #define
#ifdef SHGetSpecialFolderPath
#undef SHGetSpecialFolderPath
#endif
#define SHGetSpecialFolderPath  _AorW_SHGetSpecialFolderPath

// Define the prototypes for each of these forwarders...

EXTERN_C BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR pszPath, int nFolder, BOOL fCreate);

#endif // _AWTHUNK_H_
