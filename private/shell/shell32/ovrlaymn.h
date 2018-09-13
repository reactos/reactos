// Shell Icon Overlay Manager

#ifndef _OVERLAYMN_H_
#define _OVERLAYMN_H_

// HACK: This is defined in image.c, and it should be in one of the header files
#define MAX_OVERLAY_IMAGES  NUM_OVERLAY_IMAGES

#define REGSTR_ICONOVERLAYID     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers")
#define REGSTR_ICONOVERLAYCLSID  TEXT("CLSID\\%s")

STDAPI CFSIconOverlayManager_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);

STDAPI_(BOOL) IconOverlayManagerInit();
STDAPI_(void) IconOverlayManagerTerminate();

extern IShellIconOverlayManager * g_psiom;

#endif  
