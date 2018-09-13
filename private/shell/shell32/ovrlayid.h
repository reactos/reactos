// Shell Icon Overlay Identifiers 

#ifndef _OVERLAYID_H_
#define _OVERLAYID_H_

#define REGSTR_ICONOVERLAYID     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers")
#define REGSTR_ICONOVERLAYCLSID  TEXT("CLSID\\%s")

STDAPI CFSIconOverlayIdentifier_SlowFile_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);

#endif  
