/*
 * inetps.hpp - Property sheet implementation for Internet class.
 */


/* GUIDs
 ********/

#ifndef UNIX
// IEUNIX : conflicts with ..\..\inc\shguidp.h
DEFINE_GUID(CLSID_Internet, 0xFBF23B42L, 0xE3F0, 0x101B, 0x84, 0x88, 0x00, 0xAA, 0x00, 0x3E, 0x56, 0xF8);
#endif

#ifdef __cplusplus

/* Types
 ********/

// Internet property sheet

class Internet : public RefCount,
                 public IShellExtInit,
                 public IShellPropSheetExt
{
public:
   Internet(void);
   ~Internet(void);

   // IShellExtInit methods

   HRESULT STDMETHODCALLTYPE Initialize(PCITEMIDLIST pcidlFolder, PIDataObject pidobj, HKEY hkeyProgID);

   // IShellPropSheetExt methods

   HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
   HRESULT STDMETHODCALLTYPE ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

   // IUnknown methods

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCInternet(const Internet *pcmimehk);

#endif

};
DECLARE_STANDARD_TYPES(Internet);

#endif  /* __cplusplus */
