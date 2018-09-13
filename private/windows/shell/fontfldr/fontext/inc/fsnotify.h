//------------------------------------------------------------------------
// FSNotify.h
//
// Include file for FSNotify.cpp
//------------------------------------------------------------------------

#if !defined(__FSNOTIFY_H__)
#define __FSNOTIFY_H__

#if defined(__FCN__)

// Forward Declarations --------------------------------------------------
//
class CFontManager;

//------------------------------------------------------------------------

typedef struct {
   HANDLE          m_hWatch;  // Returned from FindFirstChangeNotify.
} NOTIFYWATCH, FAR * LPNOTIFYWATCH;

DWORD dwNotifyWatchProc( LPVOID pvParams );

#endif // __FCN__ 

#endif   // __FSNOTIFY_H__
