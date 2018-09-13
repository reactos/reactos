#ifndef SHELL32_VIEWS_INC
#define SHELL32_VIEWS_INC

#include "idlcomm.h"
#include "pidl.h"

// from mulprsht.c
STDAPI FileSystem_AddPages(void *pv, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);


STDAPI_(LPTSTR) SHGetCaption(HIDA hida);
STDAPI SHPropertiesForPidl(HWND hwnd, LPCITEMIDLIST pidlFull, LPCTSTR lpParameters);
typedef HRESULT (CALLBACK *PFNGAOCALLBACK)(IShellFolder2 *psf, LPCITEMIDLIST pidl, ULONG* prgfInOut);
STDAPI Multi_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST* apidl, ULONG *prgfInOut, PFNGAOCALLBACK pfnGAOCallback);


STDAPI CNETIDLData_GetNetResourceForFS(IDataObject *pdtobj, LPSTGMEDIUM pmedium);
STDAPI CDesktopIDLData_GetNetResourceForFS(IDataObject *pdtobj, LPSTGMEDIUM pmedium);

// IDLIST.C
STDAPI ILCompareRelIDs(IShellFolder *psf, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
STDAPI ILGetRelDisplayName(IShellFolder *psf, LPSTRRET pStrRet,
                                   LPCITEMIDLIST pidlRel, LPCTSTR pszName,
                                   LPCTSTR pszTemplate);

// RUNDLL32.C
STDAPI_(BOOL) SHRunDLLProcess(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow, UINT idStr, BOOL fRunAsNewUser);
STDAPI_(BOOL) SHRunDLLThread(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow);

// CPLOBJ.C
STDAPI_(BOOL) SHRunControlPanelEx(LPCTSTR pszOrigCmdLine, HWND hwnd, BOOL fRunAsNewUser);


// REGITMS.C

typedef struct
{
    const CLSID * pclsid;
    UINT    uNameID;
    LPCTSTR pszIconFile;
    int     iDefIcon;
    BYTE    bOrder;
    DWORD   dwAttributes;
    LPCTSTR pszCPL;
} REQREGITEM;

#define RIISA_ORIGINAL              0x00000001  // regitems first then rest (desktop)
#define RIISA_FOLDERFIRST           0x00000002  // folders first then the rest (regitems or not)
#define RIISA_ALPHABETICAL          0x00000004  // alphabetical (doesn't care about folder, regitems, ...)

typedef struct
{
    LPCTSTR pszAllow;
    RESTRICTIONS restAllow;
    LPCTSTR pszDisallow;
    RESTRICTIONS restDisallow;
} REGITEMSPOLICY;

typedef struct
{
    LPCTSTR             pszRegKey;      // registry location for this name space
    REGITEMSPOLICY*     pPolicy;        // registry location to look for Restrict and Disallow info
    TCHAR               cRegItem;       // parsing prefix, must be TEXT(':')
    BYTE                bFlags;         // flags field for PIDL construction
    int                 iCmp;           // compare multiplier used to revers the sort order
    DWORD               rgfRegItems;    // default attributes for items
    int                 iReqItems;      // # of required items
    REQREGITEM const *  pReqItems;      // require items array
    DWORD               dwSortAttrib;   // sorting attributes
    LPCTSTR             pszMachine;     // optional remote machine to init items from (\\server)
    DWORD               cbPadding;      // Number of bytes of padding to put between IDREGITEMEX.bOrder and IDREGITEMEX.clsid
    BYTE                bFlagsLegacy;   // legacy "bFlags", so that we can handle previous bFlags (equiv of cbPadding = 0)
                                        // CANNOT be 0, 0 means no bFlagsLegacy
} REGITEMSINFO;

// class factory like entry to create the regitems folder. it only supports the agregatied case

STDAPI CRegFolder_CreateInstance(REGITEMSINFO *pri, IUnknown *punkOutter, REFIID riid, void **ppv);

// this should be private
#pragma pack(1)
typedef struct
{
    IDREGITEM       idri;
    USHORT          cbNext;
} IDLREGITEM;           // "RegItem" IDList
typedef UNALIGNED IDLREGITEM *LPIDLREGITEM;
typedef const UNALIGNED IDLREGITEM *LPCIDLREGITEM;
#pragma pack()

EXTERN_C const IDLREGITEM c_idlNet;
EXTERN_C const IDLREGITEM c_idlDrives;
EXTERN_C const IDLREGITEM c_idlInetRoot;

#define MAX_REGITEMCCH  128     // for rename in place operations

//--------------------------------------------------------------------------
// Menu offset-IDs for object context menu (CFSMenu)
//  They must be 0-based and not too big (<0x3ff)
//  We are lumping all of the DefView clients in here so that we are
//  sure the ID ranges are separate (making MenuHelp easier)
//---------------------------------------------------------------------------
#define FSIDM_OBJPROPS_FIRST    0x0000
#define FSIDM_PROPERTIESBG      (FSIDM_OBJPROPS_FIRST + 0x0000)

// find extension commands
#define FSIDM_FINDFILES         0x0004
#define FSIDM_FINDCOMPUTER      0x0005
#define FSIDM_SAVESEARCH        0x0006
#define FSIDM_OPENCONTAININGFOLDER 0x0007

#define FSIDM_DRIVES_FIRST      0x0008
#define FSIDM_FORMAT            (FSIDM_DRIVES_FIRST + 0x0000)
#define FSIDM_DISCONNECT        (FSIDM_DRIVES_FIRST + 0x0001)
#define FSIDM_EJECT             (FSIDM_DRIVES_FIRST + 0x0002)
#define FSIDM_DISKCOPY          (FSIDM_DRIVES_FIRST + 0x0003)

#define FSIDM_NETWORK_FIRST     0x0010
#define FSIDM_CONNECT           (FSIDM_NETWORK_FIRST + 0x0000)
#define FSIDM_NETPRN_INSTALL    (FSIDM_NETWORK_FIRST + 0x0001)
#define FSIDM_CONNECT_PRN       (FSIDM_NETWORK_FIRST + 0x0002)
#define FSIDM_DISCONNECT_PRN    (FSIDM_NETWORK_FIRST + 0x0003)

// Command offsets for context menu (verb ids must be mutually exclusive
// from non-verb ids.  Non-verb ids are first for easier menu merging.)
// non-verb ids:
#define FSIDM_CPLPRN_FIRST      0x0020
#define FSIDM_SETDEFAULTPRN     (FSIDM_CPLPRN_FIRST + 0x0000)
#define FSIDM_SHARING           (FSIDM_CPLPRN_FIRST + 0x0001)
#define FSIDM_DOCUMENTDEFAULTS  (FSIDM_CPLPRN_FIRST + 0x0002)
#define FSIDM_SERVERPROPERTIES  (FSIDM_CPLPRN_FIRST + 0x0003)

// verb ids:
#define FSIDM_OPENPRN           (FSIDM_CPLPRN_FIRST + 0x0008)
#define FSIDM_RESUMEPRN         (FSIDM_CPLPRN_FIRST + 0x0009)
#define FSIDM_PAUSEPRN          (FSIDM_CPLPRN_FIRST + 0x000a)
#define FSIDM_WORKONLINE        (FSIDM_CPLPRN_FIRST + 0x000b)
#define FSIDM_WORKOFFLINE       (FSIDM_CPLPRN_FIRST + 0x000c)
#define FSIDM_PURGEPRN          (FSIDM_CPLPRN_FIRST + 0x000d)

#define FSIDM_RUNAS             (FSIDM_CPLPRN_FIRST + 0x000e)

// these need to be in the same order as the ICOL in fstreex.c (chee)
#define FSIDM_SORT_FIRST        0x0030
#define FSIDM_SORTBYNAME        (FSIDM_SORT_FIRST + 0x0000)
#define FSIDM_SORTBYSIZE        (FSIDM_SORT_FIRST + 0x0001)
#define FSIDM_SORTBYTYPE        (FSIDM_SORT_FIRST + 0x0002)
#define FSIDM_SORTBYDATE        (FSIDM_SORT_FIRST + 0x0003)
#define FSIDM_SORTBYATTRIBUTES  (FSIDM_SORT_FIRST + 0x0004)
#define FSIDM_SORTBYCOMMENT     (FSIDM_SORT_FIRST + 0x0005)
#define FSIDM_SORTBYLOCATION    (FSIDM_SORT_FIRST + 0x0006)
#define FSIDM_SORTBYORIGIN      (FSIDM_SORT_FIRST + 0x0007)
#define FSIDM_SORTBYDELETEDDATE (FSIDM_SORT_FIRST + 0x0008)

#define FSIDM_MENU_SENDTO       0x0040
#define FSIDM_SENDTOFIRST       (FSIDM_MENU_SENDTO + 0x0001)
#define FSIDM_SENDTOLAST        (FSIDM_MENU_SENDTO + 0x001f)

#define FSIDM_MENU_NEW          0x0060
#define FSIDM_NEWFOLDER         (FSIDM_MENU_NEW + 0x0001)
#define FSIDM_NEWLINK           (FSIDM_MENU_NEW + 0x0002)
#define FSIDM_NEWOTHER          (FSIDM_MENU_NEW + 0x0003)
#define FSIDM_NEWLAST           (FSIDM_MENU_NEW + 0x003f)

// BITBUCKET ids.
#define FSIDM_RESTORE           0x0070
#define FSIDM_PURGE             0x0071
#define FSIDM_PURGEALL          0x0072

//---------------------------------------------------------------------------
// Desktop view specific command IDs
//
#define FSIDM_DESKTOPEX_FIRST   (FSIDM_NEWLAST+1)
#define FSIDM_DESKTOPEX_LAST    (FSIDM_DESKTOPEX_FIRST+0x1f)

//---------------------------------------------------------------------------
// Briefcase view specific command IDs
//
#define FSIDM_MENU_BRIEFCASE    0x00b0
#define FSIDM_UPDATEALL         (FSIDM_MENU_BRIEFCASE + 0x0001)
#define FSIDM_UPDATESELECTION   (FSIDM_MENU_BRIEFCASE + 0x0002)
#define FSIDM_SPLIT             (FSIDM_MENU_BRIEFCASE + 0x0003)

//---------------------------------------------------------------------------
// Context Menu negotiation command IDs for seperators
//

#define FSIDM_FOLDER_SEP    0x00c0
#define FSIDM_VIEW_SEP         (FSIDM_FOLDER_SEP + 0x0001)



//---------------------------------------------------------------------------
// Items added by DefCM
//
// HACK: Put these at the same offsets from each other as the SFVIDM
// commands so that we can easily reuse the help strings and the menu
// initialization code
//
#define DCMIDM_LINK             SHARED_FILE_LINK
#define DCMIDM_DELETE           SHARED_FILE_DELETE
#define DCMIDM_RENAME           SHARED_FILE_RENAME
#define DCMIDM_PROPERTIES       SHARED_FILE_PROPERTIES

#define DCMIDM_CUT              SHARED_EDIT_CUT
#define DCMIDM_COPY             SHARED_EDIT_COPY
#define DCMIDM_PASTE            SHARED_EDIT_PASTE

//
// Now for the MenuHelp ID's for the defview client menu commands
//
#define IDS_MH_PROPERTIESBG     (IDS_MH_FSIDM_FIRST + FSIDM_PROPERTIESBG)

#define IDS_MH_FORMAT           (IDS_MH_FSIDM_FIRST + FSIDM_FORMAT)
#define IDS_MH_DISCONNECT       (IDS_MH_FSIDM_FIRST + FSIDM_DISCONNECT)
#define IDS_MH_EJECT            (IDS_MH_FSIDM_FIRST + FSIDM_EJECT)
#define IDS_MH_DISKCOPY         (IDS_MH_FSIDM_FIRST + FSIDM_DISKCOPY)

#define IDS_MH_CONNECT          (IDS_MH_FSIDM_FIRST + FSIDM_CONNECT)

#define IDS_MH_NETPRN_INSTALL   (IDS_MH_FSIDM_FIRST + FSIDM_NETPRN_INSTALL)
#define IDS_MH_CONNECT_PRN      (IDS_MH_FSIDM_FIRST + FSIDM_CONNECT_PRN)
#define IDS_MH_DISCONNECT_PRN   (IDS_MH_FSIDM_FIRST + FSIDM_DISCONNECT_PRN)

#define IDS_MH_SETDEFAULTPRN    (IDS_MH_FSIDM_FIRST + FSIDM_SETDEFAULTPRN)

#define IDS_MH_SERVERPROPERTIES (IDS_MH_FSIDM_FIRST + FSIDM_SERVERPROPERTIES)
#define IDS_MH_SHARING          (IDS_MH_FSIDM_FIRST + FSIDM_SHARING)
#define IDS_MH_DOCUMENTDEFAULTS (IDS_MH_FSIDM_FIRST + FSIDM_DOCUMENTDEFAULTS )

#define IDS_MH_OPENPRN          (IDS_MH_FSIDM_FIRST + FSIDM_OPENPRN)
#define IDS_MH_PAUSEPRN         (IDS_MH_FSIDM_FIRST + FSIDM_PAUSEPRN)
#define IDS_MH_WORKOFFLINE      (IDS_MH_FSIDM_FIRST + FSIDM_WORKOFFLINE)
#define IDS_MH_PURGEPRN         (IDS_MH_FSIDM_FIRST + FSIDM_PURGEPRN)

#define IDS_MH_SORTBYNAME       (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYNAME)
#define IDS_MH_SORTBYSIZE       (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYSIZE)
#define IDS_MH_SORTBYTYPE       (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYTYPE)
#define IDS_MH_SORTBYDATE       (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYDATE)
#define IDS_MH_SORTBYATTRIBUTES (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYATTRIBUTES)
#define IDS_MH_SORTBYCOMMENT    (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYCOMMENT)
#define IDS_MH_SORTBYLOCATION   (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYLOCATION)
#define IDS_MH_SORTBYORIGIN     (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYORIGIN)
#define IDS_MH_SORTBYDELETEDDATE (IDS_MH_FSIDM_FIRST + FSIDM_SORTBYDELETEDDATE)

#define IDS_MH_MENU_SENDTO      (IDS_MH_FSIDM_FIRST + FSIDM_MENU_SENDTO)
#define IDS_MH_SENDTOFIRST      (IDS_MH_FSIDM_FIRST + FSIDM_SENDTOFIRST)
#define IDS_MH_SENDTOLAST       (IDS_MH_FSIDM_FIRST + FSIDM_SENDTOLAST)

#define IDS_MH_MENU_NEW         (IDS_MH_FSIDM_FIRST + FSIDM_MENU_NEW)
#define IDS_MH_NEWFOLDER        (IDS_MH_FSIDM_FIRST + FSIDM_NEWFOLDER)
#define IDS_MH_NEWLINK          (IDS_MH_FSIDM_FIRST + FSIDM_NEWLINK)
#define IDS_MH_NEWOTHER         (IDS_MH_FSIDM_FIRST + FSIDM_NEWOTHER)

#define IDS_MH_MENU_BRIEFCASE   (IDS_MH_FSIDM_FIRST + FSIDM_MENU_BRIEFCASE)
#define IDS_MH_UPDATEALL        (IDS_MH_FSIDM_FIRST + FSIDM_UPDATEALL)
#define IDS_MH_UPDATESELECTION  (IDS_MH_FSIDM_FIRST + FSIDM_UPDATESELECTION)
#define IDS_MH_SPLIT            (IDS_MH_FSIDM_FIRST + FSIDM_SPLIT)

// bitbucket menu help strings
#define IDS_MH_RESTORE          (IDS_MH_FSIDM_FIRST + FSIDM_RESTORE)
#define IDS_MH_PURGE            (IDS_MH_FSIDM_FIRST + FSIDM_PURGE)
#define IDS_MH_PURGEALL         (IDS_MH_FSIDM_FIRST + FSIDM_PURGEALL)

// find extensions
#define IDS_MH_FINDFILES        (IDS_MH_FSIDM_FIRST + FSIDM_FINDFILES)
#define IDS_MH_FINDCOMPUTER     (IDS_MH_FSIDM_FIRST + FSIDM_FINDCOMPUTER)

//
// Now for the MenuHelp ID's for the defview client menu commands
//
#define IDS_TT_SORTBYNAME       (IDS_TT_FSIDM_FIRST + FSIDM_SORTBYNAME)
#define IDS_TT_SORTBYTYPE       (IDS_TT_FSIDM_FIRST + FSIDM_SORTBYTYPE)
#define IDS_TT_SORTBYSIZE       (IDS_TT_FSIDM_FIRST + FSIDM_SORTBYSIZE)
#define IDS_TT_SORTBYDATE       (IDS_TT_FSIDM_FIRST + FSIDM_SORTBYDATE)

#define IDS_TT_UPDATEALL        (IDS_TT_FSIDM_FIRST + FSIDM_UPDATEALL)
#define IDS_TT_UPDATESELECTION  (IDS_TT_FSIDM_FIRST + FSIDM_UPDATESELECTION)

#endif

