#ifndef _FAVORITE_H
#define _FAVORITE_H

EXTERN_C const GUID CLSID_FavoritesFolder;
STDAPI CFavorites_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);

// Shell32.dll doesn't support DllCanUnLoadNow, and doesn't ref count its objects currently
#define DllRelease()    
#define DllAddRef()

//
// global variables
//
EXTERN_C HINSTANCE g_hinst;

//
// Trace flags
//

#define TF_FAV_FULL         0x00000010      // Verbose stuff 
#define TF_FAV_REGISTER     0x00000100      
#define TF_FAV_ISF          0x00000200      // IShellFolder related stuff
#define TF_FAV_CACHE        0x00000400      // FavExtra hdpa cache
#define TF_FAV_ENUM         0x00000800      // PIDL Enumerator

// Define a mask that we don't get as many squirty outs by default
#define DM_FULLTRACE    0x8000
#define DM_MSGTRACE     0x0800

//
// Function trace flags
//
#define FTF_FAV         0x00000001

#endif // _FAVORITE_H
