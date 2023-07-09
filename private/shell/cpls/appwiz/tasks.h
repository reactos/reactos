#ifndef __TASKS_H_
#define __TASKS_H_

#include <runtask.h>

// Search Depth
#define MAX_EXE_SEARCH_DEPTH 2
HRESULT FindAppInfo(LPCTSTR pszFolder, LPCTSTR pszFullName, LPCTSTR pszShortName, PSLOWAPPINFO psai, BOOL bDarwin);

#endif // _TASKS_H_
