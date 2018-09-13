
#ifndef _RECDOCS_H_
#define _RECDOCS_H_

//  SRMLF_* flags to pass into CreateSharedRecentMRUList()
#define SRMLF_COMPNAME  0x00000000   // default:  compare using the name of the recent file
#define SRMLF_COMPPIDL  0x00000001   // use the pidl in the recent folder

STDAPI_(HANDLE) CreateSharedRecentMRUList(LPCTSTR pszExt, DWORD *pcMax, DWORD dwFlags);
STDAPI_(int) EnumSharedRecentMRUList(HANDLE hmru, int iItem, LPTSTR *ppszName, LPITEMIDLIST *ppidl);
STDAPI_(void) AddToRecentDocs( LPCITEMIDLIST pidl, LPCTSTR lpszPath );
STDAPI CTaskAddDoc_Create(HANDLE hMem, DWORD dwProcId, IRunnableTask **pptask);
STDAPI RecentDocs_GetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName, DWORD cchName);

#define MAXRECENTDOCS 15

#endif  //  _RECDOCS_H_

