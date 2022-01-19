/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Copyright 1999 Ian Schmidt <ischmidt@cfl.rr.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *
 * Some of these policies can be tweaked via the System Policy
 * Editor which came with the Win95 Migration Guide, although
 * there doesn't appear to be an updated Win98 version that
 * would handle the many new policies introduced since then.
 * You could easily write one with the information in
 * this file...
 *
 * Up to date as of SHELL32 v5.00 (W2K)
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <wine/debug.h>

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHELL_NO_POLICY 0xffffffff

typedef struct tagPOLICYDAT
{
  DWORD policy;          /* policy value passed to SHRestricted */
  LPCSTR appstr;         /* application str such as "Explorer" */
  LPCSTR keystr;         /* name of the actual registry key / policy */
  DWORD cache;           /* cached value or 0xffffffff for invalid */
} POLICYDATA, *LPPOLICYDATA;

/* application strings */

static const char strExplorer[] = {"Explorer"};
static const char strActiveDesk[] = {"ActiveDesktop"};
static const char strWinOldApp[] = {"WinOldApp"};
static const char strAddRemoveProgs[] = {"AddRemoveProgs"};

/* key strings */

static const char strNoFileURL[] = {"NoFileUrl"};
static const char strNoFolderOptions[] = {"NoFolderOptions"};
static const char strNoChangeStartMenu[] = {"NoChangeStartMenu"};
static const char strNoWindowsUpdate[] = {"NoWindowsUpdate"};
static const char strNoSetActiveDesktop[] = {"NoSetActiveDesktop"};
static const char strNoForgetSoftwareUpdate[] = {"NoForgetSoftwareUpdate"};
//static const char strNoMSAppLogo[] = {"NoMSAppLogo5ChannelNotify"};
static const char strForceCopyACLW[] = {"ForceCopyACLWithFile"};
static const char strNoResolveTrk[] = {"NoResolveTrack"};
static const char strNoResolveSearch[] = {"NoResolveSearch"};
static const char strNoEditComponent[] = {"NoEditingComponents"};
static const char strNoMovingBand[] = {"NoMovingBands"};
static const char strNoCloseDragDrop[] = {"NoCloseDragDropBands"};
static const char strNoCloseComponent[] = {"NoClosingComponents"};
static const char strNoDelComponent[] = {"NoDeletingComponents"};
static const char strNoAddComponent[] = {"NoAddingComponents"};
static const char strNoComponent[] = {"NoComponents"};
static const char strNoChangeWallpaper[] = {"NoChangingWallpaper"};
static const char strNoHTMLWallpaper[] = {"NoHTMLWallpaper"};
static const char strNoCustomWebView[] = {"NoCustomizeWebView"};
static const char strClassicShell[] = {"ClassicShell"};
static const char strClearRecentDocs[] = {"ClearRecentDocsOnExit"};
static const char strNoFavoritesMenu[] = {"NoFavoritesMenu"};
static const char strNoActiveDesktopChanges[] = {"NoActiveDesktopChanges"};
static const char strNoActiveDesktop[] = {"NoActiveDesktop"};
static const char strNoRecentDocMenu[] = {"NoRecentDocsMenu"};
static const char strNoRecentDocHistory[] = {"NoRecentDocsHistory"};
static const char strNoInetIcon[] = {"NoInternetIcon"};
static const char strNoSettingsWizard[] = {"NoSettingsWizards"};
static const char strNoLogoff[] = {"NoLogoff"};
static const char strNoNetConDis[] = {"NoNetConnectDisconnect"};
static const char strNoViewContextMenu[] = {"NoViewContextMenu"};
static const char strNoTrayContextMenu[] = {"NoTrayContextMenu"};
static const char strNoWebMenu[] = {"NoWebMenu"};
static const char strLnkResolveIgnoreLnkInfo[] = {"LinkResolveIgnoreLinkInfo"};
static const char strNoCommonGroups[] = {"NoCommonGroups"};
static const char strEnforceShlExtSecurity[] = {"EnforceShellExtensionSecurity"};
static const char strNoRealMode[] = {"NoRealMode"};
static const char strMyDocsOnNet[] = {"MyDocsOnNet"};
static const char strNoStartMenuSubfolder[] = {"NoStartMenuSubFolders"};
static const char strNoAddPrinters[] = {"NoAddPrinter"};
static const char strNoDeletePrinters[] = {"NoDeletePrinter"};
static const char strNoPrintTab[] = {"NoPrinterTabs"};
static const char strRestrictRun[] = {"RestrictRun"};
static const char strNoStartBanner[] = {"NoStartBanner"};
static const char strNoNetworkNeighborhood[] = {"NoNetHood"};
static const char strNoDriveTypeAtRun[] = {"NoDriveTypeAutoRun"};
static const char strNoDrivesAutoRun[] = {"NoDriveAutoRun"};
static const char strSeparateProcess[] = {"SeparateProcess"};
static const char strNoDrives[] = {"NoDrives"};
static const char strNoFind[] = {"NoFind"};
static const char strNoDesktop[] = {"NoDesktop"};
static const char strNoSetTaskBar[] = {"NoSetTaskbar"};
static const char strNoSetFld[] = {"NoSetFolders"};
static const char strNoFileMenu[] = {"NoFileMenu"};
static const char strNoSaveSetting[] = {"NoSaveSettings"};
static const char strNoClose[] = {"NoClose"};
static const char strNoRun[] = {"NoRun"};
#ifdef __REACTOS__
static const char strNoSimpleStartMenu[] = {"NoSimpleStartMenu"};
#endif

/* policy data array */
static POLICYDATA sh32_policy_table[] =
{
  {
    REST_NORUN,
    strExplorer,
    strNoRun,
    SHELL_NO_POLICY
  },
  {
    REST_NOCLOSE,
    strExplorer,
    strNoClose,
    SHELL_NO_POLICY
  },
  {
    REST_NOSAVESET,
    strExplorer,
    strNoSaveSetting,
    SHELL_NO_POLICY
  },
  {
    REST_NOFILEMENU,
    strExplorer,
    strNoFileMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NOSETFOLDERS,
    strExplorer,
    strNoSetFld,
    SHELL_NO_POLICY
  },
  {
    REST_NOSETTASKBAR,
    strExplorer,
    strNoSetTaskBar,
    SHELL_NO_POLICY
  },
  {
    REST_NODESKTOP,
    strExplorer,
    strNoDesktop,
    SHELL_NO_POLICY
  },
  {
    REST_NOFIND,
    strExplorer,
    strNoFind,
    SHELL_NO_POLICY
  },
  {
    REST_NODRIVES,
    strExplorer,
    strNoDrives,
    SHELL_NO_POLICY
  },
  {
    REST_NODRIVEAUTORUN,
    strExplorer,
    strNoDrivesAutoRun,
    SHELL_NO_POLICY
  },
  {
    REST_NODRIVETYPEAUTORUN,
    strExplorer,
    strNoDriveTypeAtRun,
    SHELL_NO_POLICY
  },
  {
    REST_NONETHOOD,
    strExplorer,
    strNoNetworkNeighborhood,
    SHELL_NO_POLICY
  },
  {
    REST_STARTBANNER,
    strExplorer,
    strNoStartBanner,
    SHELL_NO_POLICY
  },
  {
    REST_RESTRICTRUN,
    strExplorer,
    strRestrictRun,
    SHELL_NO_POLICY
  },
  {
    REST_NOPRINTERTABS,
    strExplorer,
    strNoPrintTab,
    SHELL_NO_POLICY
  },
  {
    REST_NOPRINTERDELETE,
    strExplorer,
    strNoDeletePrinters,
    SHELL_NO_POLICY
  },
  {
    REST_NOPRINTERADD,
    strExplorer,
    strNoAddPrinters,
    SHELL_NO_POLICY
  },
  {
    REST_NOSTARTMENUSUBFOLDERS,
    strExplorer,
    strNoStartMenuSubfolder,
    SHELL_NO_POLICY
  },
  {
    REST_MYDOCSONNET,
    strExplorer,
    strMyDocsOnNet,
    SHELL_NO_POLICY
  },
  {
    REST_NOEXITTODOS,
    strWinOldApp,
    strNoRealMode,
    SHELL_NO_POLICY
  },
  {
    REST_ENFORCESHELLEXTSECURITY,
    strExplorer,
    strEnforceShlExtSecurity,
    SHELL_NO_POLICY
  },
  {
    REST_LINKRESOLVEIGNORELINKINFO,
    strExplorer,
    strLnkResolveIgnoreLnkInfo,
    SHELL_NO_POLICY
  },
  {
    REST_NOCOMMONGROUPS,
    strExplorer,
    strNoCommonGroups,
    SHELL_NO_POLICY
  },
  {
    REST_SEPARATEDESKTOPPROCESS,
    strExplorer,
    strSeparateProcess,
    SHELL_NO_POLICY
  },
  {
    REST_NOWEB,
    strExplorer,
    strNoWebMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NOTRAYCONTEXTMENU,
    strExplorer,
    strNoTrayContextMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NOVIEWCONTEXTMENU,
    strExplorer,
    strNoViewContextMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NONETCONNECTDISCONNECT,
    strExplorer,
    strNoNetConDis,
    SHELL_NO_POLICY
  },
  {
    REST_STARTMENULOGOFF,
    strExplorer,
    strNoLogoff,
    SHELL_NO_POLICY
  },
  {
    REST_NOSETTINGSASSIST,
    strExplorer,
    strNoSettingsWizard,
    SHELL_NO_POLICY
  },
  {
    REST_NOINTERNETICON,
    strExplorer,
    strNoInetIcon,
    SHELL_NO_POLICY
  },
  {
    REST_NORECENTDOCSHISTORY,
    strExplorer,
    strNoRecentDocHistory,
    SHELL_NO_POLICY
  },
  {
    REST_NORECENTDOCSMENU,
    strExplorer,
    strNoRecentDocMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NOACTIVEDESKTOP,
    strExplorer,
    strNoActiveDesktop,
    SHELL_NO_POLICY
  },
  {
    REST_NOACTIVEDESKTOPCHANGES,
    strExplorer,
    strNoActiveDesktopChanges,
    SHELL_NO_POLICY
  },
  {
    REST_NOFAVORITESMENU,
    strExplorer,
    strNoFavoritesMenu,
    SHELL_NO_POLICY
  },
  {
    REST_CLEARRECENTDOCSONEXIT,
    strExplorer,
    strClearRecentDocs,
    SHELL_NO_POLICY
  },
  {
    REST_CLASSICSHELL,
    strExplorer,
    strClassicShell,
    SHELL_NO_POLICY
  },
  {
    REST_NOCUSTOMIZEWEBVIEW,
    strExplorer,
    strNoCustomWebView,
    SHELL_NO_POLICY
  },
  {
    REST_NOHTMLWALLPAPER,
    strActiveDesk,
    strNoHTMLWallpaper,
    SHELL_NO_POLICY
  },
  {
    REST_NOCHANGINGWALLPAPER,
    strActiveDesk,
    strNoChangeWallpaper,
    SHELL_NO_POLICY
  },
  {
    REST_NODESKCOMP,
    strActiveDesk,
    strNoComponent,
    SHELL_NO_POLICY
  },
  {
    REST_NOADDDESKCOMP,
    strActiveDesk,
    strNoAddComponent,
    SHELL_NO_POLICY
  },
  {
    REST_NODELDESKCOMP,
    strActiveDesk,
    strNoDelComponent,
    SHELL_NO_POLICY
  },
  {
    REST_NOCLOSEDESKCOMP,
    strActiveDesk,
    strNoCloseComponent,
    SHELL_NO_POLICY
  },
  {
    REST_NOCLOSE_DRAGDROPBAND,
    strActiveDesk,
    strNoCloseDragDrop,
    SHELL_NO_POLICY
  },
  {
    REST_NOMOVINGBAND,
    strActiveDesk,
    strNoMovingBand,
    SHELL_NO_POLICY
  },
  {
    REST_NOEDITDESKCOMP,
    strActiveDesk,
    strNoEditComponent,
    SHELL_NO_POLICY
  },
  {
    REST_NORESOLVESEARCH,
    strExplorer,
    strNoResolveSearch,
    SHELL_NO_POLICY
  },
  {
    REST_NORESOLVETRACK,
    strExplorer,
    strNoResolveTrk,
    SHELL_NO_POLICY
  },
  {
    REST_FORCECOPYACLWITHFILE,
    strExplorer,
    strForceCopyACLW,
    SHELL_NO_POLICY
  },
#if (NTDDI_VERSION < NTDDI_LONGHORN)
  {
    REST_NOLOGO3CHANNELNOTIFY,
    strExplorer,
    strNoMSAppLogo,
    SHELL_NO_POLICY
  },
#endif
  {
    REST_NOFORGETSOFTWAREUPDATE,
    strExplorer,
    strNoForgetSoftwareUpdate,
    SHELL_NO_POLICY
  },
  {
    REST_NOSETACTIVEDESKTOP,
    strExplorer,
    strNoSetActiveDesktop,
    SHELL_NO_POLICY
  },
  {
    REST_NOUPDATEWINDOWS,
    strExplorer,
    strNoWindowsUpdate,
    SHELL_NO_POLICY
  },
  {
    REST_NOCHANGESTARMENU,
    strExplorer,
    strNoChangeStartMenu,
    SHELL_NO_POLICY
  },
  {
    REST_NOFOLDEROPTIONS,
    strExplorer,
    strNoFolderOptions,
    SHELL_NO_POLICY
  },
  {
    REST_HASFINDCOMPUTERS,
    strExplorer,
    "FindComputers",
    SHELL_NO_POLICY
  },
  {
    REST_INTELLIMENUS,
    strExplorer,
    "IntelliMenus",
    SHELL_NO_POLICY
  },
  {
    REST_RUNDLGMEMCHECKBOX,
    strExplorer,
    "MemCheckBoxInRunDlg",
    SHELL_NO_POLICY
  },
  {
    REST_ARP_ShowPostSetup,
    strAddRemoveProgs,
    "ShowPostSetup",
    SHELL_NO_POLICY
  },
  {
    REST_NOCSC,
    strExplorer,
    "NoSyncAll",
    SHELL_NO_POLICY
  },
  {
    REST_NOCONTROLPANEL,
    strExplorer,
    "NoControlPanel",
    SHELL_NO_POLICY
  },
  {
    REST_ENUMWORKGROUP,
    strExplorer,
    "EnumWorkgroup",
    SHELL_NO_POLICY
  },
  {
    REST_ARP_NOARP,
    strAddRemoveProgs,
    "NoAddRemovePrograms",
    SHELL_NO_POLICY
  },
  {
    REST_ARP_NOREMOVEPAGE,
    strAddRemoveProgs,
    "NoRemovePage",
    SHELL_NO_POLICY
  },
  {
    REST_ARP_NOADDPAGE,
    strAddRemoveProgs,
    "NoAddPage",
    SHELL_NO_POLICY
  },
  {
    REST_ARP_NOWINSETUPPAGE,
    strAddRemoveProgs,
    "NoWindowsSetupPage",
    SHELL_NO_POLICY
  },
  {
    REST_GREYMSIADS,
    strExplorer,
    "",
    SHELL_NO_POLICY
  },
  {
    REST_NOCHANGEMAPPEDDRIVELABEL,
    strExplorer,
    "NoChangeMappedDriveLabel",
    SHELL_NO_POLICY
  },
  {
    REST_NOCHANGEMAPPEDDRIVECOMMENT,
    strExplorer,
    "NoChangeMappedDriveComment",
    SHELL_NO_POLICY
  },
  {
    REST_MaxRecentDocs,
    strExplorer,
    "MaxRecentDocs",
    SHELL_NO_POLICY
  },
  {
    REST_NONETWORKCONNECTIONS,
    strExplorer,
    "NoNetworkConnections",
    SHELL_NO_POLICY
  },
  {
    REST_FORCESTARTMENULOGOFF,
    strExplorer,
    "ForceStartMenuLogoff",
    SHELL_NO_POLICY
  },
  {
    REST_NOWEBVIEW,
    strExplorer,
     "NoWebView",
    SHELL_NO_POLICY
  },
  {
    REST_NOCUSTOMIZETHISFOLDER,
    strExplorer,
    "NoCustomizeThisFolder",
    SHELL_NO_POLICY
  },
  {
    REST_NOENCRYPTION,
    strExplorer,
    "NoEncryption",
    SHELL_NO_POLICY
  },
  {
    REST_ALLOWFRENCHENCRYPTION,
    strExplorer,
    "AllowFrenchEncryption",
    SHELL_NO_POLICY
  },
  {
    REST_DONTSHOWSUPERHIDDEN,
    strExplorer,
    "DontShowSuperHidden",
    SHELL_NO_POLICY
  },
  {
    REST_NOSHELLSEARCHBUTTON,
    strExplorer,
    "NoShellSearchButton",
    SHELL_NO_POLICY
  },
  {
    REST_NOHARDWARETAB,
    strExplorer,
    "NoHardwareTab",
    SHELL_NO_POLICY
  },
  {
    REST_NORUNASINSTALLPROMPT,
    strExplorer,
    "NoRunasInstallPrompt",
    SHELL_NO_POLICY
  },
  {
    REST_PROMPTRUNASINSTALLNETPATH,
    strExplorer,
    "PromptRunasInstallNetPath",
    SHELL_NO_POLICY
  },
  {
    REST_NOMANAGEMYCOMPUTERVERB,
    strExplorer,
    "NoManageMyComputerVerb",
    SHELL_NO_POLICY
  },
  {
    REST_NORECENTDOCSNETHOOD,
    strExplorer,
    "NoRecentDocsNetHood",
    SHELL_NO_POLICY
  },
  {
    REST_DISALLOWRUN,
    strExplorer,
    "DisallowRun",
    SHELL_NO_POLICY
  },
  {
    REST_NOWELCOMESCREEN,
    strExplorer,
    "NoWelcomeScreen",
    SHELL_NO_POLICY
  },
  {
    REST_RESTRICTCPL,
    strExplorer,
    "RestrictCpl",
    SHELL_NO_POLICY
  },
  {
    REST_DISALLOWCPL,
    strExplorer,
    "DisallowCpl",
    SHELL_NO_POLICY
  },
  {
    REST_NOSMBALLOONTIP,
    strExplorer,
    "NoSMBalloonTip",
    SHELL_NO_POLICY
  },
  {
    REST_NOSMHELP,
    strExplorer,
    "NoSMHelp",
    SHELL_NO_POLICY
  },
  {
    REST_NOWINKEYS,
    strExplorer,
    "NoWinKeys",
    SHELL_NO_POLICY
  },
  {
    REST_NOENCRYPTONMOVE,
    strExplorer,
    "NoEncryptOnMove",
    SHELL_NO_POLICY
  },
  {
    REST_NOLOCALMACHINERUN,
    strExplorer,
    "DisableLocalMachineRun",
    SHELL_NO_POLICY
  },
  {
    REST_NOCURRENTUSERRUN,
    strExplorer,
    "DisableCurrentUserRun",
    SHELL_NO_POLICY
  },
  {
    REST_NOLOCALMACHINERUNONCE,
    strExplorer,
    "DisableLocalMachineRunOnce",
    SHELL_NO_POLICY
  },
  {
    REST_NOCURRENTUSERRUNONCE,
    strExplorer,
    "DisableCurrentUserRunOnce",
    SHELL_NO_POLICY
  },
  {
    REST_FORCEACTIVEDESKTOPON,
    strExplorer,
    "ForceActiveDesktopOn",
    SHELL_NO_POLICY
  },
  {
    REST_NOCOMPUTERSNEARME,
    strExplorer,
    "NoComputersNearMe",
    SHELL_NO_POLICY
  },
  {
    REST_NOVIEWONDRIVE,
    strExplorer,
    "NoViewOnDrive",
    SHELL_NO_POLICY
  },
  {
    REST_NONETCRAWL,
    strExplorer,
    "NoNetCrawl",
    SHELL_NO_POLICY
  },
  {
    REST_NOSHAREDDOCUMENTS,
    strExplorer,
    "NoSharedDocs",
    SHELL_NO_POLICY
  },
  {
    REST_NOSMMYDOCS,
    strExplorer,
    "NoSMMyDocs",
    SHELL_NO_POLICY
  },
/* 0x4000050 - 0x4000060 */
  {
    REST_NONLEGACYSHELLMODE,
    strExplorer,
    "NoneLegacyShellMode",
    SHELL_NO_POLICY
  },
#ifdef __REACTOS__
  {
    REST_NOSTARTPANEL,
    strExplorer,
    strNoSimpleStartMenu,
    SHELL_NO_POLICY
  },
#endif
  {
    REST_STARTRUNNOHOMEPATH,
    strExplorer,
    "StartRunNoHOMEPATH",
    SHELL_NO_POLICY
  },
/* 0x4000061 - 0x4000086 */
  {
    REST_NODISCONNECT,
    strExplorer,
    "NoDisconnect",
    SHELL_NO_POLICY
  },
  {
    REST_NOSECURITY,
    strExplorer,
    "NoNTSecurity",
    SHELL_NO_POLICY
  },
  {
    REST_NOFILEASSOCIATE,
    strExplorer,
    "NoFileAssociate",
    SHELL_NO_POLICY
  },
  {
    0x50000024,
    strExplorer,
    strNoFileURL,
    SHELL_NO_POLICY
  },
  {
    0,
    0,
    0,
    SHELL_NO_POLICY
  }
};

/*************************************************************************
 * SHRestricted				 [SHELL32.100]
 *
 * Get the value associated with a policy Id.
 *
 * PARAMS
 *     pol [I] Policy Id
 *
 * RETURNS
 *     The queried value for the policy.
 *
 * NOTES
 *     Exported by ordinal.
 *     This function caches the retrieved values to prevent unnecessary registry access,
 *     if SHSettingsChanged() was previously called.
 *
 * REFERENCES
 *     a: MS System Policy Editor.
 *     b: 98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     c: 'The Windows 95 Registry', by John Woram, 1996 MIS: Press
 */
DWORD WINAPI SHRestricted (RESTRICTIONS policy)
{
	char regstr[256];
	HKEY    xhkey;
	DWORD   retval, datsize = 4;
	LPPOLICYDATA p;

	TRACE("(%08x)\n", policy);

	/* scan to see if we know this policy ID */
	for (p = sh32_policy_table; p->policy; p++)
	{
	  if (policy == p->policy)
	  {
	    break;
	  }
	}

	if (p->policy == 0)
	{
	    /* we don't know this policy, return 0 */
	    TRACE("unknown policy: (%08x)\n", policy);
		return 0;
	}

	/* we have a known policy */

	/* first check if this policy has been cached, return it if so */
	if (p->cache != SHELL_NO_POLICY)
	{
	    return p->cache;
	}

	lstrcpyA(regstr, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\");
	lstrcatA(regstr, p->appstr);

	/* return 0 and don't set the cache if any registry errors occur */
#ifndef __REACTOS__
	retval = 0;
	if (RegOpenKeyA(HKEY_CURRENT_USER, regstr, &xhkey) == ERROR_SUCCESS)
#else // FIXME: Actually this *MUST* use shlwapi!SHRestrictionLookup()
      // See http://www.geoffchappell.com/studies/windows/shell/shell32/api/util/shrestricted.htm
    retval = RegOpenKeyA(HKEY_LOCAL_MACHINE, regstr, &xhkey);
    if (retval != ERROR_SUCCESS)
    {
        retval = RegOpenKeyA(HKEY_CURRENT_USER, regstr, &xhkey);
        if (retval != ERROR_SUCCESS)
            return 0;
    }
#endif
	{
	  if (RegQueryValueExA(xhkey, p->keystr, NULL, NULL, (LPBYTE)&retval, &datsize) == ERROR_SUCCESS)
	  {
	    p->cache = retval;
	  }
	  RegCloseKey(xhkey);
	}
	return retval;
}

/*************************************************************************
 * SHSettingsChanged          [SHELL32.244]
 *
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  unused    [I] Reserved.
 *  inpRegKey [I] Registry key to scan.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialised.
 *  Failure: 0, if inpRegKey is any value other than NULL, "Policy", or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 */
BOOL WINAPI SHSettingsChanged(LPCVOID unused, LPCVOID inpRegKey)
{
	TRACE("(%p, %p)\n", unused, inpRegKey);

	/* first check - if input is non-NULL and points to the secret
	   key string, then pass. Otherwise return 0.
	 */
	if (inpRegKey != NULL)
	{
	  if (SHELL_OsIsUnicode())
	  {
            if (lstrcmpiW(inpRegKey, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies") &&
                lstrcmpiW(inpRegKey, L"Policy"))
	      /* doesn't match, fail */
	      return FALSE;
	  }
	  else
	  {
            if (lstrcmpiA(inpRegKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies") &&
                lstrcmpiA(inpRegKey, "Policy"))
	      /* doesn't match, fail */
	      return FALSE;
	  }
	}

	return TRUE;
}
