/*
 * TREEWALK.h -- Shell Icon Overlay Manager
 */

#ifndef _TREEWALK_H_
#define _TREEWALK_H_

//
// Prototypes for all modules
//
#ifdef __cplusplus
extern "C" {
#endif
    
STDAPI CShellTreeWalker_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID *  ppvOut);
#ifdef __cplusplus
};
#endif

BOOL   BeenThereDoneThat(LPCTSTR pszOriginal, LPCTSTR pszPath);

#endif  
