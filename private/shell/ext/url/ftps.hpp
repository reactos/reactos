/*
 * ftps.hpp - File Types property sheet implementation for MIME types
 *            description.
 */


/* GUIDs
 ********/

DEFINE_GUID(CLSID_MIMEFileTypesPropSheetHook,   0xFBF23B41L, 0xE3F0, 0x101B, 0x84, 0x88, 0x00, 0xAA, 0x00, 0x3E, 0x56, 0xF8);

#ifdef __cplusplus

/* Types
 ********/

// MIME File Types property sheet hook

class MIMEHook : public RefCount,
                 public IShellExtInit,
                 public IShellPropSheetExt
{
public:
   MIMEHook(void);
   ~MIMEHook(void);

   // IShellExtInit methods

   HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

   // IShellPropSheetExt methods

   HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
   HRESULT STDMETHODCALLTYPE ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

   // IUnknown methods

   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCMIMEHook(const MIMEHook *pcmimehk);

#endif

};
DECLARE_STANDARD_TYPES(MIMEHook);

#endif  /* __cplusplus */
