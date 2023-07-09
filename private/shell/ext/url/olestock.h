/*
 * olestock.h - Stock OLE header file.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


/* Types
 ********/

/* IDs */

DECLARE_STANDARD_TYPES(GUID);
DECLARE_STANDARD_TYPES(CLSID);
DECLARE_STANDARD_TYPES(IID);

typedef FARPROC *Interface;
DECLARE_STANDARD_TYPES(Interface);

/* interfaces */

DECLARE_STANDARD_TYPES(IAdviseSink);
DECLARE_STANDARD_TYPES(IBindCtx);
DECLARE_STANDARD_TYPES(IClassFactory);
DECLARE_STANDARD_TYPES(IDataObject);
DECLARE_STANDARD_TYPES(IDropSource);
DECLARE_STANDARD_TYPES(IDropTarget);
DECLARE_STANDARD_TYPES(IEnumFORMATETC);
DECLARE_STANDARD_TYPES(IEnumSTATDATA);
DECLARE_STANDARD_TYPES(IMalloc);
DECLARE_STANDARD_TYPES(IMoniker);
DECLARE_STANDARD_TYPES(IPersist);
DECLARE_STANDARD_TYPES(IPersistFile);
DECLARE_STANDARD_TYPES(IPersistStorage);
DECLARE_STANDARD_TYPES(IPersistStream);
DECLARE_STANDARD_TYPES(IStorage);
DECLARE_STANDARD_TYPES(IStream);
DECLARE_STANDARD_TYPES(IUnknown);

/* structures */

DECLARE_STANDARD_TYPES(DVTARGETDEVICE);
DECLARE_STANDARD_TYPES(FORMATETC);
DECLARE_STANDARD_TYPES(STGMEDIUM);

/* advise flags */

typedef enum advise_flags
{
   ALL_ADVISE_FLAGS   = (ADVF_NODATA |
                         ADVF_PRIMEFIRST |
                         ADVF_ONLYONCE |
                         ADVF_DATAONSTOP |
                         ADVFCACHE_NOHANDLER |
                         ADVFCACHE_FORCEBUILTIN |
                         ADVFCACHE_ONSAVE)
}
ADVISE_FLAGS;

/* data transfer direction flags */

typedef enum datadir_flags
{
   ALL_DATADIR_FLAGS   = (DATADIR_GET |
                          DATADIR_SET)
}
DATADIR_FLAGS;

/* drop effects */

typedef enum drop_effects
{
   ALL_DROPEFFECT_FLAGS   = (DROPEFFECT_NONE |
                             DROPEFFECT_COPY |
                             DROPEFFECT_MOVE |
                             DROPEFFECT_LINK |
                             DROPEFFECT_SCROLL)
}
DROP_EFFECTS;

/* mouse message key states */

typedef enum mk_flags
{
   ALL_KEYSTATE_FLAGS      = (MK_LBUTTON |
                              MK_RBUTTON |
                              MK_SHIFT |
                              MK_CONTROL |
                              MK_MBUTTON)
}
MK_FLAGS;

/* medium types */

typedef enum tymeds
{
   ALL_TYMED_FLAGS         = (TYMED_HGLOBAL |
                              TYMED_FILE |
                              TYMED_ISTREAM |
                              TYMED_ISTORAGE |
                              TYMED_GDI |
                              TYMED_MFPICT |
                              TYMED_ENHMF)
}
TYMEDS;


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

