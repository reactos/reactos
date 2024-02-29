/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Copyright 1999 Ian Schmidt <ischmidt@cfl.rr.com>
 * Copyright 2022 Hermes Belusca-Maito
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
 * Up to date as of SHELL32 v6.00 (Win2k3)
 * References:
 * https://www.geoffchappell.com/studies/windows/shell/shell32/api/util/restrictions.htm
 * https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/ne-shlobj_core-restrictions
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <initguid.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define REGKEY_POLICIES L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies"

DEFINE_GUID(GUID_Restrictions, 0xA48F1A32, 0xA340, 0x11D1, 0xBC, 0x6B, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

static const POLICYDATA s_PolicyTable[] =
{
    /* { policy, appstr, keystr }, */
    { REST_NORUN, L"Explorer", L"NoRun" },
    { REST_NOCLOSE, L"Explorer", L"NoClose" },
    { REST_NOSAVESET, L"Explorer", L"NoSaveSettings" },
    { REST_NOFILEMENU, L"Explorer", L"NoFileMenu" },
    { REST_NOSETFOLDERS, L"Explorer", L"NoSetFolders" },
    { REST_NOSETTASKBAR, L"Explorer", L"NoSetTaskbar" },
    { REST_NODESKTOP, L"Explorer", L"NoDesktop" },
    { REST_NOFIND, L"Explorer", L"NoFind" },
    { REST_NODRIVES, L"Explorer", L"NoDrives" },
    { REST_NODRIVEAUTORUN, L"Explorer", L"NoDriveAutoRun" },
    { REST_NODRIVETYPEAUTORUN, L"Explorer", L"NoDriveTypeAutoRun" },
    { REST_NONETHOOD, L"Explorer", L"NoNetHood" },
    { REST_STARTBANNER, L"Explorer", L"NoStartBanner" },
    { REST_RESTRICTRUN, L"Explorer", L"RestrictRun" },
    { REST_NOPRINTERTABS, L"Explorer", L"NoPrinterTabs" },
    { REST_NOPRINTERDELETE, L"Explorer", L"NoDeletePrinter" },
    { REST_NOPRINTERADD, L"Explorer", L"NoAddPrinter" },
    { REST_NOSTARTMENUSUBFOLDERS, L"Explorer", L"NoStartMenuSubFolders" },
    { REST_MYDOCSONNET, L"Explorer", L"MyDocsOnNet" },
    { REST_NOEXITTODOS, L"WinOldApp", L"NoRealMode" },
    { REST_ENFORCESHELLEXTSECURITY, L"Explorer", L"EnforceShellExtensionSecurity" },
    { REST_NOCOMMONGROUPS, L"Explorer", L"NoCommonGroups" },
    { REST_LINKRESOLVEIGNORELINKINFO, L"Explorer", L"LinkResolveIgnoreLinkInfo" },
    { REST_NOWEB, L"Explorer", L"NoWebMenu" },
    { REST_NOTRAYCONTEXTMENU, L"Explorer", L"NoTrayContextMenu" },
    { REST_NOVIEWCONTEXTMENU, L"Explorer", L"NoViewContextMenu" },
    { REST_NONETCONNECTDISCONNECT, L"Explorer", L"NoNetConnectDisconnect" },
    { REST_STARTMENULOGOFF, L"Explorer", L"StartMenuLogoff" },
    { REST_NOSETTINGSASSIST, L"Explorer", L"NoSettingsWizards" },
    { REST_NODISCONNECT, L"Explorer", L"NoDisconnect" },
    { REST_NOSECURITY, L"Explorer", L"NoNTSecurity" },
    { REST_NOFILEASSOCIATE, L"Explorer", L"NoFileAssociate" },
    { REST_NOINTERNETICON, L"Explorer", L"NoInternetIcon" },
    { REST_NORECENTDOCSHISTORY, L"Explorer", L"NoRecentDocsHistory" },
    { REST_NORECENTDOCSMENU, L"Explorer", L"NoRecentDocsMenu" },
    { REST_NOACTIVEDESKTOP, L"Explorer", L"NoActiveDesktop" },
    { REST_NOACTIVEDESKTOPCHANGES, L"Explorer", L"NoActiveDesktopChanges" },
    { REST_NOFAVORITESMENU, L"Explorer", L"NoFavoritesMenu" },
    { REST_CLEARRECENTDOCSONEXIT, L"Explorer", L"ClearRecentDocsOnExit" },
    { REST_CLASSICSHELL, L"Explorer", L"ClassicShell" },
    { REST_NOCUSTOMIZEWEBVIEW, L"Explorer", L"NoCustomizeWebView" },
    { REST_NOHTMLWALLPAPER, L"ActiveDesktop", L"NoHTMLWallPaper" },
    { REST_NOCHANGINGWALLPAPER, L"ActiveDesktop", L"NoChangingWallPaper" },
    { REST_NODESKCOMP, L"ActiveDesktop", L"NoComponents" },
    { REST_NOADDDESKCOMP, L"ActiveDesktop", L"NoAddingComponents" },
    { REST_NODELDESKCOMP, L"ActiveDesktop", L"NoDeletingComponents" },
    { REST_NOCLOSEDESKCOMP, L"ActiveDesktop", L"NoClosingComponents" },
    { REST_NOCLOSE_DRAGDROPBAND, L"Explorer", L"NoCloseDragDropBands" },
    { REST_NOMOVINGBAND, L"Explorer", L"NoMovingBands" },
    { REST_NOEDITDESKCOMP, L"ActiveDesktop", L"NoEditingComponents" },
    { REST_NORESOLVESEARCH, L"Explorer", L"NoResolveSearch" },
    { REST_NORESOLVETRACK, L"Explorer", L"NoResolveTrack" },
    { REST_FORCECOPYACLWITHFILE, L"Explorer", L"ForceCopyACLWithFile" },
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    { REST_NOLOGO3CHANNELNOTIFY, L"Explorer", L"NoMSAppLogo5ChannelNotify" },
#endif
    { REST_NOFORGETSOFTWAREUPDATE, L"Explorer", L"NoForgetSoftwareUpdate" },
    { REST_GREYMSIADS, L"Explorer", L"GreyMSIAds" },
    { REST_NOSETACTIVEDESKTOP, L"Explorer", L"NoSetActiveDesktop" },
    { REST_NOUPDATEWINDOWS, L"Explorer", L"NoWindowsUpdate" },
    { REST_NOCHANGESTARMENU, L"Explorer", L"NoChangeStartMenu" },
    { REST_NOFOLDEROPTIONS, L"Explorer", L"NoFolderOptions" },
    { REST_NOCSC, L"Explorer", L"NoSyncAll" },
    { REST_HASFINDCOMPUTERS, L"Explorer", L"FindComputers" },
    { REST_RUNDLGMEMCHECKBOX, L"Explorer", L"MemCheckBoxInRunDlg" },
    { REST_INTELLIMENUS, L"Explorer", L"IntelliMenus" },
    { REST_SEPARATEDESKTOPPROCESS, L"Explorer", L"SeparateProcess" },
    { REST_MaxRecentDocs, L"Explorer", L"MaxRecentDocs" },
    { REST_NOCONTROLPANEL, L"Explorer", L"NoControlPanel" },
    { REST_ENUMWORKGROUP, L"Explorer", L"EnumWorkgroup" },
    { REST_ARP_ShowPostSetup, L"Uninstall", L"ShowPostSetup" },
    { REST_ARP_NOARP, L"Uninstall", L"NoAddRemovePrograms" },
    { REST_ARP_NOREMOVEPAGE, L"Uninstall", L"NoRemovePage" },
    { REST_ARP_NOADDPAGE, L"Uninstall", L"NoAddPage" },
    { REST_GREYMSIADS, L"Uninstall", L"NoWindowsSetupPage" },
    { REST_NOCHANGEMAPPEDDRIVELABEL, L"Explorer", L"NoChangeMappedDriveLabel" },
    { REST_NOCHANGEMAPPEDDRIVECOMMENT, L"Explorer", L"NoChangeMappedDriveComment" },
    { REST_NONETWORKCONNECTIONS, L"Explorer", L"NoNetworkConnections" },
    { REST_FORCESTARTMENULOGOFF, L"Explorer", L"ForceStartMenuLogoff" },
    { REST_NOWEBVIEW, L"Explorer", L"NoWebView" },
    { REST_NOCUSTOMIZETHISFOLDER, L"Explorer", L"NoCustomizeThisFolder" },
    { REST_NOENCRYPTION, L"Explorer", L"NoEncryption" },
    { REST_DONTSHOWSUPERHIDDEN, L"Explorer", L"DontShowSuperHidden" },
    { REST_NOSHELLSEARCHBUTTON, L"Explorer", L"NoShellSearchButton" },
    { REST_NOHARDWARETAB, L"Explorer", L"NoHardwareTab" },
    { REST_NORUNASINSTALLPROMPT, L"Explorer", L"NoRunasInstallPrompt" },
    { REST_PROMPTRUNASINSTALLNETPATH, L"Explorer", L"PromptRunasInstallNetPath" },
    { REST_NOMANAGEMYCOMPUTERVERB, L"Explorer", L"NoManageMyComputerVerb" },
    { REST_NORECENTDOCSNETHOOD, L"Explorer", L"NoRecentDocsNetHood" },
    { REST_DISALLOWRUN, L"Explorer", L"DisallowRun" },
    { REST_NOWELCOMESCREEN, L"Explorer", L"NoWelcomeScreen" },
    { REST_RESTRICTCPL, L"Explorer", L"RestrictCpl" },
    { REST_DISALLOWCPL, L"Explorer", L"DisallowCpl" },
    { REST_NOSMBALLOONTIP, L"Explorer", L"NoSMBalloonTip" },
    { REST_NOSMHELP, L"Explorer", L"NoSMHelp" },
    { REST_NOWINKEYS, L"Explorer", L"NoWinKeys" },
    { REST_NOENCRYPTONMOVE, L"Explorer", L"NoEncryptOnMove" },
    { REST_NOLOCALMACHINERUN, L"Explorer", L"DisableLocalMachineRun" },
    { REST_NOCURRENTUSERRUN, L"Explorer", L"DisableCurrentUserRun" },
    { REST_NOLOCALMACHINERUNONCE, L"Explorer", L"DisableLocalMachineRunOnce" },
    { REST_NOCURRENTUSERRUNONCE, L"Explorer", L"DisableCurrentUserRunOnce" },
    { REST_FORCEACTIVEDESKTOPON, L"Explorer", L"ForceActiveDesktopOn" },
    { REST_NOCOMPUTERSNEARME, L"Explorer", L"NoComputersNearMe" },
    { REST_NOVIEWONDRIVE, L"Explorer", L"NoViewOnDrive" },
    { REST_NONETCRAWL, L"Explorer", L"NoNetCrawling" },
    { REST_NOSHAREDDOCUMENTS, L"Explorer", L"NoSharedDocuments" },
    { REST_NOSMMYDOCS, L"Explorer", L"NoSMMyDocs" },
    { REST_NOSMMYPICS, L"Explorer", L"NoSMMyPictures" },
    { REST_ALLOWBITBUCKDRIVES, L"Explorer", L"RecycleBinDrives" },
    { REST_NONLEGACYSHELLMODE, L"Explorer", L"NoneLegacyShellMode" },
    { REST_NOCONTROLPANELBARRICADE, L"Explorer", L"NoControlPanelBarricade" },
    { REST_NOAUTOTRAYNOTIFY, L"Explorer", L"NoAutoTrayNotify" },
    { REST_NOTASKGROUPING, L"Explorer", L"NoTaskGrouping" },
    { REST_NOCDBURNING, L"Explorer", L"NoCDBurning" },
    { REST_MYCOMPNOPROP, L"Explorer", L"NoPropertiesMyComputer" },
    { REST_MYDOCSNOPROP, L"Explorer", L"NoPropertiesMyDocuments" },
    { REST_NODISPLAYAPPEARANCEPAGE, L"System", L"NoDispAppearancePage" },
    { REST_NOTHEMESTAB, L"Explorer", L"NoThemesTab" },
    { REST_NOVISUALSTYLECHOICE, L"System", L"NoVisualStyleChoice" },
    { REST_NOSIZECHOICE, L"System", L"NoSizeChoice" },
    { REST_NOCOLORCHOICE, L"System", L"NoColorChoice" },
    { REST_SETVISUALSTYLE, L"System", L"SetVisualStyle" },
    { REST_STARTRUNNOHOMEPATH, L"Explorer", L"StartRunNoHOMEPATH" },
    { REST_NOSTARTPANEL, L"Explorer", L"NoSimpleStartMenu" },
    { REST_NOUSERNAMEINSTARTPANEL, L"Explorer", L"NoUserNameInStartMenu" },
    { REST_NOMYCOMPUTERICON, L"NonEnum", L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}" },
    { REST_NOSMNETWORKPLACES, L"Explorer", L"NoStartMenuNetworkPlaces" },
    { REST_NOSMPINNEDLIST, L"Explorer", L"NoStartMenuPinnedList" },
    { REST_NOSMMYMUSIC, L"Explorer", L"NoStartMenuMyMusic" },
    { REST_NOSMEJECTPC, L"Explorer", L"NoStartMenuEjectPC" },
    { REST_NOSMMOREPROGRAMS, L"Explorer", L"NoStartMenuMorePrograms" },
    { REST_NOSMMFUPROGRAMS, L"Explorer", L"NoStartMenuMFUprogramsList" },
    { REST_HIDECLOCK, L"Explorer", L"HideClock" },
    { REST_NOLOWDISKSPACECHECKS, L"Explorer", L"NoLowDiskSpaceChecks" },
    { REST_NODESKTOPCLEANUP, L"Explorer", L"NoDesktopCleanupWizard" },
    { REST_NOENTIRENETWORK, L"Network", L"NoEntireNetwork" },
    { REST_BITBUCKNUKEONDELETE, L"Explorer", L"NoRecycleFiles" },
    { REST_BITBUCKCONFIRMDELETE, L"Explorer", L"ConfirmFileDelete" },
    { REST_BITBUCKNOPROP, L"Explorer", L"NoPropertiesRecycleBin" },
    { REST_NOTRAYITEMSDISPLAY, L"Explorer", L"NoTrayItemsDisplay" },
    { REST_NOTOOLBARSONTASKBAR, L"Explorer", L"NoToolbarsOnTaskbar" },
    { 0x4000006C, L"Explorer", L"NoDriveTypeAutoRunCommandAndVerb" },   /* FIXME: name */
    { REST_NODISPBACKGROUND, L"System", L"NoDispBackgroundPage" },
    { REST_NODISPSCREENSAVEPG, L"System", L"NoDispScrSavPage" },
    { REST_NODISPSETTINGSPG, L"System", L"NoDispSettingsPage" },
    { REST_NODISPSCREENSAVEPREVIEW, L"System", L"NoScreenSavePreview" },
    { REST_NODISPLAYCPL, L"System", L"NoDispCPL" },
    { REST_HIDERUNASVERB, L"Explorer", L"HideRunAsVerb" },
    { REST_NOTHUMBNAILCACHE, L"Explorer", L"NoThumbnailCache" },
    { REST_NOSTRCMPLOGICAL, L"Explorer", L"NoStrCmpLogical" },
    { 0x40000095, L"Explorer", L"NoInternetOpenWith" },                 /* FIXME: name */
    { REST_NOSMCONFIGUREPROGRAMS, L"Explorer", L"NoSMConfigurePrograms" },
    { REST_NOPUBLISHWIZARD, L"Explorer", L"NoPublishingWizard" },
    { REST_NOONLINEPRINTSWIZARD, L"Explorer", L"NoOnlinePrintsWizard" },
    { REST_NOWEBSERVICES, L"Explorer", L"NoWebServices" },
    { REST_ALLOWUNHASHEDWEBVIEW, L"Explorer", L"AllowUnhashedWebView" },
    { REST_ALLOWLEGACYWEBVIEW, L"Explorer", L"AllowLegacyWebView" },
    { REST_REVERTWEBVIEWSECURITY, L"Explorer", L"RevertWebViewSecurity" },
    { REST_INHERITCONSOLEHANDLES, L"Explorer", L"InheritConsoleHandles" },
    { 0x40000089, L"Explorer", L"NoRemoteRecursiveEvents" },            /* FIXME: name */
    { 0x40000087, L"Explorer", L"SortMaxItemCount" },                   /* FIXME: name */
    { 0x40000091, L"Explorer", L"NoRemoteChangeNotify" },               /* FIXME: name */
    { 0x4000009C, L"Explorer", L"AllowFileCLSIDJunctions" },            /* FIXME: name */
    { 0x41000004, L"Explorer", L"ToggleCommentPosition" },              /* FIXME: name */
    { 0x40000093, L"Explorer", L"NoEnumEntireNetwork" },                /* FIXME: name */
    { 0x40000094, L"Explorer", L"NoDetailsThumbnailOnNetwork" },        /* FIXME: name */
    { 0x4000009B, L"Explorer", L"DontRetryBadNetName" },                /* FIXME: name */
    { 0x4000009C, L"Explorer", L"AllowFileCLSIDJunctions" },            /* FIXME: name */
    { 0x4000009D, L"Explorer", L"NoUPnPInstallTask" },                  /* FIXME: name */
    { 0x4000009A, L"Explorer", L"AllowLegacyLMZBehavior" }              /* FIXME: name */
    { 0x41000005, L"Explorer", L"UseDesktopIniCache" },                 /* FIXME: name */
    { 0x400000FF, L"Explorer", L"AllowCLSIDPROGIDMapping" },            /* FIXME: name */
    { 0x40000092, L"Explorer", L"NoSimpleNetIDList" },                  /* FIXME: name */
};

/*
 * The restriction-related variables
 */
HANDLE g_hRestGlobalCounter = NULL;
LONG g_nRestCountValue = -1;
DWORD g_RestValues[_countof(s_PolicyTable)] = { 0 };

static HANDLE
SHELL_GetCachedGlobalCounter(HANDLE *phGlobalCounter, REFGUID rguid)
{
    HANDLE hGlobalCounter;
    if (*phGlobalCounter)
        return *phGlobalCounter;
    hGlobalCounter = SHGlobalCounterCreate(rguid);
    if (SHInterlockedCompareExchange(phGlobalCounter, hGlobalCounter, NULL))
        CloseHandle(hGlobalCounter);
    return *phGlobalCounter;
}

static HANDLE SHELL_GetRestrictionsCounter(VOID)
{
    return SHELL_GetCachedGlobalCounter(&g_hRestGlobalCounter, &GUID_Restrictions);
}

static BOOL SHELL_QueryRestrictionsChanged(VOID)
{
    LONG Value = SHGlobalCounterGetValue(SHELL_GetRestrictionsCounter());
    if (g_nRestCountValue == Value)
        return FALSE;

    g_nRestCountValue = Value;
    return TRUE;
}

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
DWORD WINAPI SHRestricted (RESTRICTIONS rest)
{
    TRACE("(0x%08lX)\n", rest);

    if (SHELL_QueryRestrictionsChanged())
        FillMemory(&g_RestValues, sizeof(g_RestValues), 0xFF);

    return SHRestrictionLookup(rest, NULL, &s_PolicyTable, &g_RestValues);
}

/*************************************************************************
 * SHSettingsChanged          [SHELL32.244]
 *
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  unused    [I] Reserved.
 *  pszKey    [I] Registry key to scan.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialised.
 *  Failure: 0, if inpRegKey is any value other than NULL, "Policy", or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 */
BOOL WINAPI SHSettingsChanged(LPCVOID unused, LPCWSTR pszKey)
{
    TRACE("(%p, %s)\n", unused, debugstr_w(pszKey));

    if (pszKey &&
        lstrcmpiW(L"Policy", pszKey) != 0 &&
        lstrcmpiW(REGKEY_POLICIES, pszKey) != 0)
    {
        return FALSE;
    }

    return SHGlobalCounterIncrement(SHELL_GetRestrictionsCounter());
}
