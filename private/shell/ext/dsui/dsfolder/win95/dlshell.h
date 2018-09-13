#ifndef __dlshell_h
#define __dlshell_h

//
// Redefine some useful macros
//

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef EVAL
#undef EVAL
#endif

#define EVAL(exp)       ((exp) != 0)

#include "assert.h"
#define ASSERT(x)       assert(x)

//
// More shell bits
//

#include "datautil.h"
#include "ccstock.h"
#include "validate.h"
#include "shlapip.h"

//
// SHReleaseStgMedium
//

#define SHReleaseStgMedium ReleaseStgMedium

//
// HIDA & HDXA
//

STDAPI_(void) IDLData_InitializeClipboardFormats(void);

typedef HGLOBAL HIDA;

STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl);
STDAPI_(void) HIDA_Free(HIDA hida);
STDAPI_(HIDA) HIDA_Clone(HIDA hida);
STDAPI_(UINT) HIDA_GetCount(HIDA hida);
STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax);

STDAPI_(LPITEMIDLIST) HIDA_ILClone(HIDA hida, UINT i);
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl);

//
// Struct:  ContextMenuInfo:
//
//  This data structure is used by FileView_DoContextMenu (and its private
// function, _AppendMenuItems) to handler multiple context menu handlers.
//
// History:
//  02-25-93 SatoNa     Created
//
typedef struct { // cmi
    IContextMenu  *pcm;
    UINT        idCmdFirst;
    UINT        idCmdMax;
    BOOL        fQIBug;
#ifdef DEBUG
    TCHAR       szKeyDebug[40]; // key name
#endif
} ContextMenuInfo;

typedef HDSA    HDXA;   // hdma

#define HDXA_Create()   ((HDXA)DSA_Create(SIZEOF(ContextMenuInfo), 4))

STDAPI_(UINT) HDXA_AppendMenuItems(
                        HDXA hdxa, IDataObject * pdtobj,
                        UINT nKeys, const HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        HMENU hmenu, UINT uInsert,
                        UINT idCmdFirst,  UINT idCmdLast,
                        UINT fFlags,
                        HDCA hdca);
STDAPI_(UINT) HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, const HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        QCMINFO* pqcm,
                        UINT fFlags,
                        HDCA hdca,
                        IUnknown* pSite);

STDAPI_(UINT) HDXA_LetHandlerProcessCommand(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici);
STDAPI HDXA_GetCommandString(HDXA hdxa, UINT idCmd, UINT wFlags, UINT * pwReserved, LPSTR pszName, UINT cchMax);
STDAPI_(void) HDXA_DeleteAll(HDXA hdxa);
STDAPI_(void) HDXA_Destroy(HDXA hdxa);

//
// Clipboard Format for IDLData object.
//

#define ICFHDROP                0
#define ICFFILENAME             1
#define ICFNETRESOURCE          2
#define ICFFILECONTENTS         3
#define ICFFILEGROUPDESCRIPTORA 4
#define ICFFILENAMEMAPW         5
#define ICFFILENAMEMAP          6
#define ICFHIDA                 7
#define ICFOFFSETS              8
#define ICFPRINTERFRIENDLYNAME  9
#define ICFPRIVATESHELLDATA     10
#define ICFHTML                 11
#define ICFFILENAMEW            12
#define ICFFILEGROUPDESCRIPTORW 13
#define ICFPREFERREDDROPEFFECT  14
#define ICFPERFORMEDDROPEFFECT  15
#define ICFPASTESUCCEEDED       16
#define ICFSHELLURL             17
#define ICFINDRAGLOOP           18
#define ICF_DRAGCONTEXT         19
#define ICF_TARGETCLSID         20
#define ICF_MAX                 21

EXTERN_C CLIPFORMAT g_acfIDLData[];

#define g_cfNetResource                 g_acfIDLData[ICFNETRESOURCE]
#define g_cfHIDA                        g_acfIDLData[ICFHIDA]
#define g_cfOFFSETS                     g_acfIDLData[ICFOFFSETS]
#define g_cfPrinterFriendlyName         g_acfIDLData[ICFPRINTERFRIENDLYNAME]
#define g_cfFileName                    g_acfIDLData[ICFFILENAME]
#define g_cfFileContents                g_acfIDLData[ICFFILECONTENTS]
#define g_cfFileGroupDescriptorA        g_acfIDLData[ICFFILEGROUPDESCRIPTORA]
#define g_cfFileGroupDescriptorW        g_acfIDLData[ICFFILEGROUPDESCRIPTORW]
#define g_cfFileNameMapW                g_acfIDLData[ICFFILENAMEMAPW]
#define g_cfFileNameMapA                g_acfIDLData[ICFFILENAMEMAP]
#define g_cfPrivateShellData            g_acfIDLData[ICFPRIVATESHELLDATA]
#define g_cfHTML                        g_acfIDLData[ICFHTML]
#define g_cfFileNameW                   g_acfIDLData[ICFFILENAMEW]
#define g_cfPreferredDropEffect         g_acfIDLData[ICFPREFERREDDROPEFFECT]
#define g_cfPerformedDropEffect         g_acfIDLData[ICFPERFORMEDDROPEFFECT]
#define g_cfPasteSucceeded              g_acfIDLData[ICFPASTESUCCEEDED]
#define g_cfShellURL                    g_acfIDLData[ICFSHELLURL]
#define g_cfInDragLoop                  g_acfIDLData[ICFINDRAGLOOP]
#define g_cfDragContext                 g_acfIDLData[ICF_DRAGCONTEXT]
#define g_cfTargetCLSID                 g_acfIDLData[ICF_TARGETCLSID]

// Most places will only generate one so minimize the number of changes in the code (bad idea!)
#ifdef UNICODE
#define g_cfFileNameMap         g_cfFileNameMapW
#else
#define g_cfFileNameMap         g_cfFileNameMapA
#endif


EXTERN_C void CopyInvokeInfo(CMINVOKECOMMANDINFOEX *pici, const CMINVOKECOMMANDINFO *piciIn);
#define SZ_PGP50_CONTEXTMENU  TEXT("{969223C0-26AA-11D0-90EE-444553540000}")


#endif