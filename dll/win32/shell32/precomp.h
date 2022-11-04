#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <commdlg.h>
#include <ddeml.h>
#include <userenv.h>

#include <shlwapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <atlcoll.h>
#include <powrprof.h>
#include <winnetwk.h>
#include <objsafe.h>

#include <comctl32_undoc.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>

#include <shellapi.h>
#undef ShellExecute
#include <undocshell.h>

/*
 * For versions < Vista+, redefine ShellMessageBoxW to ShellMessageBoxWrapW
 * (this is needed to avoid a linker error). On Vista+ onwards, shell32.ShellMessageBoxW
 * redirects to shlwapi.ShellMessageBoxW so the #define should not be needed.
 *
 * However our shell32 is built with _WIN32_WINNT set to 0x600 (Vista+),
 * yet its exports (especially regarding ShellMessageBoxA/W) are Win2003
 * compatible. So the #define is still needed, and the check be disabled.
 */
// #if (_WIN32_WINNT < 0x0600)
#define ShellMessageBoxW ShellMessageBoxWrapW
// #endif

#include <browseui_undoc.h>

#include <shellutils.h>

#include "shellrecyclebin/recyclebin.h"
#include "wine/pidl.h"
#include "debughlp.h"
#include "wine/shell32_main.h"
#include "shresdef.h"
#include "wine/cpanel.h"
#include "CActiveDesktop.h"
#include "CEnumIDListBase.h"
#include "shfldr.h"
#include "CShellItem.h"
#include "CShellLink.h"
#include "CFolderItemVerbs.h"
#include "CFolderItems.h"
#include "CFolder.h"
#include "CShellDispatch.h"
#include "CDropTargetHelper.h"
#include "CFolderOptions.h"
#include "folders/CFSFolder.h"
#include "folders/CDrivesFolder.h"
#include "folders/CDesktopFolder.h"
#include "folders/CControlPanelFolder.h"
#include "folders/CMyDocsFolder.h"
#include "folders/CNetFolder.h"
#include "folders/CFontsFolder.h"
#include "folders/CPrinterFolder.h"
#include "folders/CAdminToolsFolder.h"
#include "folders/CRecycleBin.h"
#include "droptargets/CexeDropHandler.h"
#include "droptargets/CFSDropTarget.h"
#include "COpenWithMenu.h"
#include "CNewMenu.h"
#include "CSendToMenu.h"
#include "CCopyToMoveToMenu.h"
#include "dialogs/filedefext.h"
#include "dialogs/drvdefext.h"
#include "CQueryAssociations.h"
#include "shellmenu/CMenuBand.h"
#include "shellmenu/CMenuDeskBar.h"
#include "shellmenu/CMenuSite.h"
#include "shellmenu/CMergedFolder.h"
#include "shellmenu/shellmenu.h"
#include "CUserNotification.h"
#include "dialogs/folder_options.h"
#include "shelldesktop/CChangeNotifyServer.h"
#include "shell32_private.h"

#include <wine/debug.h>
#include <wine/unicode.h>

#endif /* _PRECOMP_H__ */
