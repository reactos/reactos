/* $Id: shlobj.h,v 1.4 2004/06/29 13:40:40 gvg Exp $
 *
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include <winnetwk.h>

#include_next <shlobj.h>

#include <shlguid.h>
#include <shobjidl.h>

#ifndef __WINE_SHLOBJ_H
#define __WINE_SHLOBJ_H

#define SV_CLASS_NAME   ("SHELLDLL_DefView")

/* undocumented */
#define FCIDM_SHVIEW_ARRANGE    0x7001
#define FCIDM_SHVIEW_DELETE     0x7011
#define FCIDM_SHVIEW_PROPERTIES 0x7013
#define FCIDM_SHVIEW_CUT        0x7018
#define FCIDM_SHVIEW_COPY       0x7019
#define FCIDM_SHVIEW_INSERT     0x701A
#define FCIDM_SHVIEW_UNDO       0x701B
#define FCIDM_SHVIEW_INSERTLINK 0x701C
#define FCIDM_SHVIEW_SELECTALL  0x7021
#define FCIDM_SHVIEW_INVERTSELECTION 0x7022

#define FCIDM_SHVIEW_BIGICON    0x7029
#define FCIDM_SHVIEW_SMALLICON  0x702A
#define FCIDM_SHVIEW_LISTVIEW   0x702B
#define FCIDM_SHVIEW_REPORTVIEW 0x702C
/* 0x7030-0x703f are used by the shellbrowser */
#define FCIDM_SHVIEW_AUTOARRANGE 0x7031
#define FCIDM_SHVIEW_SNAPTOGRID 0x7032

#define FCIDM_SHVIEW_HELP       0x7041
#define FCIDM_SHVIEW_RENAME     0x7050
#define FCIDM_SHVIEW_CREATELINK 0x7051
#define FCIDM_SHVIEW_NEWLINK    0x7052
#define FCIDM_SHVIEW_NEWFOLDER  0x7053

#define FCIDM_SHVIEW_REFRESH    0x7100 /* FIXME */
#define FCIDM_SHVIEW_EXPLORE    0x7101 /* FIXME */
#define FCIDM_SHVIEW_OPEN       0x7102 /* FIXME */

/* undocumented toolbar items from stddlg's*/
#define FCIDM_TB_UPFOLDER       0xA001
#define FCIDM_TB_NEWFOLDER      0xA002
#define FCIDM_TB_SMALLICON      0xA003
#define FCIDM_TB_REPORTVIEW     0xA004
#define FCIDM_TB_DESKTOP        0xA005  /* FIXME */

#define CSIDL_FOLDER_MASK	0x00ff

LPVOID WINAPI SHAlloc(ULONG);
void WINAPI SHFree(LPVOID);
int WINAPI SHMapPIDLToSystemImageListIndex(IShellFolder*,LPCITEMIDLIST,int*);

/*****************************************************************************
 * IFileSystemBindData interface
 */
DEFINE_GUID(IID_IFileSystemBindData, 0x01e18d10, 0x4d8b, 0x11d2, 0x85,0x5d, 0x00,0x60,0x08,0x05,0x93,0x67);

/*** IUnknown methods ***/
#define IFileSystemBindData_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IFileSystemBindData_AddRef(p) (p)->lpVtbl->AddRef(p)
#define IFileSystemBindData_Release(p) (p)->lpVtbl->Release(p)
/*** IFileSystemBindData methods ***/
#define IFileSystemBindData_GetFindData(p,a) (p)->lpVtbl->GetFindData(p,a)
#define IFileSystemBindData_SetFindData(p,a) (p)->lpVtbl->SetFindData(p,a)

#endif /* __WINE_SHLOBJ_H */
