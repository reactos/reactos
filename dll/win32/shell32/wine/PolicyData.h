/*
 * PROJECT:     ReactOS Shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Defining shell policy data
 * COPYRIGHT:   Copyright 2022 Hermès Bélusca-Maïto
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/*
 * NOTE: Up to date as of SHELL32 v6.00 (Win2k3).
 * References:
 * https://www.geoffchappell.com/studies/windows/shell/shell32/api/util/restrictions.htm
 * https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/ne-shlobj_core-restrictions
 * https://abi-laboratory.pro/compatibility/Windows_5.0_to_Windows_6.0/x86_64/headers_diff/shell32.dll/diff.html
 */

DEFINE_POLICY( REST_NORUN,                          "Explorer",         "NoRun"                            ),
DEFINE_POLICY( REST_NOCLOSE,                        "Explorer",         "NoClose"                          ),
DEFINE_POLICY( REST_NOSAVESET,                      "Explorer",         "NoSaveSettings"                   ),
DEFINE_POLICY( REST_NOFILEMENU,                     "Explorer",         "NoFileMenu"                       ),
DEFINE_POLICY( REST_NOSETFOLDERS,                   "Explorer",         "NoSetFolders"                     ),
DEFINE_POLICY( REST_NOSETTASKBAR,                   "Explorer",         "NoSetTaskbar"                     ),
DEFINE_POLICY( REST_NODESKTOP,                      "Explorer",         "NoDesktop"                        ),
DEFINE_POLICY( REST_NOFIND,                         "Explorer",         "NoFind"                           ),
DEFINE_POLICY( REST_NODRIVES,                       "Explorer",         "NoDrives"                         ),
DEFINE_POLICY( REST_NODRIVEAUTORUN,                 "Explorer",         "NoDriveAutoRun"                   ),
DEFINE_POLICY( REST_NODRIVETYPEAUTORUN,             "Explorer",         "NoDriveTypeAutoRun"               ),
DEFINE_POLICY( REST_NONETHOOD,                      "Explorer",         "NoNetHood"                        ),
DEFINE_POLICY( REST_STARTBANNER,                    "Explorer",         "NoStartBanner"                    ),
DEFINE_POLICY( REST_RESTRICTRUN,                    "Explorer",         "RestrictRun"                      ),  // REGSTR_VAL_RESTRICTRUN
DEFINE_POLICY( REST_NOPRINTERTABS,                  "Explorer",         "NoPrinterTabs"                    ),  // REGSTR_VAL_PRINTERS_HIDETABS
DEFINE_POLICY( REST_NOPRINTERDELETE,                "Explorer",         "NoDeletePrinter"                  ),  // REGSTR_VAL_PRINTERS_NODELETE
DEFINE_POLICY( REST_NOPRINTERADD,                   "Explorer",         "NoAddPrinter"                     ),  // REGSTR_VAL_PRINTERS_NOADD
DEFINE_POLICY( REST_NOSTARTMENUSUBFOLDERS,          "Explorer",         "NoStartMenuSubFolders"            ),
DEFINE_POLICY( REST_MYDOCSONNET,                    "Explorer",         "MyDocsOnNet"                      ),
DEFINE_POLICY( REST_NOEXITTODOS,                    "WinOldApp",        "NoRealMode"                       ),  // REGSTR_VAL_WINOLDAPP_NOREALMODE
DEFINE_POLICY( REST_ENFORCESHELLEXTSECURITY,        "Explorer",         "EnforceShellExtensionSecurity"    ),
DEFINE_POLICY( REST_LINKRESOLVEIGNORELINKINFO,      "Explorer",         "LinkResolveIgnoreLinkInfo"        ),
DEFINE_POLICY( REST_NOCOMMONGROUPS,                 "Explorer",         "NoCommonGroups"                   ),
DEFINE_POLICY( REST_SEPARATEDESKTOPPROCESS,         "Explorer",         "SeparateProcess"                  ),
DEFINE_POLICY( REST_NOWEB,                          "Explorer",         "NoWebMenu"                        ),
DEFINE_POLICY( REST_NOTRAYCONTEXTMENU,              "Explorer",         "NoTrayContextMenu"                ),
DEFINE_POLICY( REST_NOVIEWCONTEXTMENU,              "Explorer",         "NoViewContextMenu"                ),
DEFINE_POLICY( REST_NONETCONNECTDISCONNECT,         "Explorer",         "NoNetConnectDisconnect"           ),

#if (WINE_FILEVERSION_MAJOR < 5)
// NOTE: This policy only controls the LogOff capability in the TaskMgr.
DEFINE_POLICY( REST_STARTMENULOGOFF,                "Explorer",         "NoLogoff"                         ),
#else
// NOTE: This policy now controls the presence/absence of the LogOff item in the Start Menu.
DEFINE_POLICY( REST_STARTMENULOGOFF,                "Explorer",         "StartMenuLogoff"                  ),
#endif

DEFINE_POLICY( REST_NOSETTINGSASSIST,               "Explorer",         "NoSettingsWizards"                ),
DEFINE_POLICY( REST_NOINTERNETICON,                 "Explorer",         "NoInternetIcon"                   ),
DEFINE_POLICY( REST_NORECENTDOCSHISTORY,            "Explorer",         "NoRecentDocsHistory"              ),
DEFINE_POLICY( REST_NORECENTDOCSMENU,               "Explorer",         "NoRecentDocsMenu"                 ),
DEFINE_POLICY( REST_NOACTIVEDESKTOP,                "Explorer",         "NoActiveDesktop"                  ),
DEFINE_POLICY( REST_NOACTIVEDESKTOPCHANGES,         "Explorer",         "NoActiveDesktopChanges"           ),
DEFINE_POLICY( REST_NOFAVORITESMENU,                "Explorer",         "NoFavoritesMenu"                  ),
DEFINE_POLICY( REST_CLEARRECENTDOCSONEXIT,          "Explorer",         "ClearRecentDocsOnExit"            ),
DEFINE_POLICY( REST_CLASSICSHELL,                   "Explorer",         "ClassicShell"                     ),
DEFINE_POLICY( REST_NOCUSTOMIZEWEBVIEW,             "Explorer",         "NoCustomizeWebView"               ),
DEFINE_POLICY( REST_NOHTMLWALLPAPER,                "ActiveDesktop",    "NoHTMLWallpaper"                  ),
DEFINE_POLICY( REST_NOCHANGINGWALLPAPER,            "ActiveDesktop",    "NoChangingWallpaper"              ),
DEFINE_POLICY( REST_NODESKCOMP,                     "ActiveDesktop",    "NoComponents"                     ),
DEFINE_POLICY( REST_NOADDDESKCOMP,                  "ActiveDesktop",    "NoAddingComponents"               ),
DEFINE_POLICY( REST_NODELDESKCOMP,                  "ActiveDesktop",    "NoDeletingComponents"             ),
DEFINE_POLICY( REST_NOCLOSEDESKCOMP,                "ActiveDesktop",    "NoClosingComponents"              ),

#if (WINE_FILEVERSION_MAJOR < 5)
DEFINE_POLICY( REST_NOCLOSE_DRAGDROPBAND,           "ActiveDesktop",    "NoCloseDragDropBands"             ),
DEFINE_POLICY( REST_NOMOVINGBAND,                   "ActiveDesktop",    "NoMovingBands"                    ),
#else
DEFINE_POLICY( REST_NOCLOSE_DRAGDROPBAND,           "Explorer",         "NoCloseDragDropBands"             ),
DEFINE_POLICY( REST_NOMOVINGBAND,                   "Explorer",         "NoMovingBands"                    ),
#endif

DEFINE_POLICY( REST_NOEDITDESKCOMP,                 "ActiveDesktop",    "NoEditingComponents"              ),
DEFINE_POLICY( REST_NORESOLVESEARCH,                "Explorer",         "NoResolveSearch"                  ),
DEFINE_POLICY( REST_NORESOLVETRACK,                 "Explorer",         "NoResolveTrack"                   ),
DEFINE_POLICY( REST_FORCECOPYACLWITHFILE,           "Explorer",         "ForceCopyACLWithFile"             ),

#if (WINE_FILEVERSION_MAJOR <= 6) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NOLOGO3CHANNELNOTIFY,           "Explorer",         "NoMSAppLogo5ChannelNotify"        ),
#endif

DEFINE_POLICY( REST_NOFORGETSOFTWAREUPDATE,         "Explorer",         "NoForgetSoftwareUpdate"           ),
DEFINE_POLICY( REST_NOSETACTIVEDESKTOP,             "Explorer",         "NoSetActiveDesktop"               ),
DEFINE_POLICY( REST_NOUPDATEWINDOWS,                "Explorer",         "NoWindowsUpdate"                  ),
DEFINE_POLICY( REST_NOCHANGESTARMENU,               "Explorer",         "NoChangeStartMenu"                ),
DEFINE_POLICY( REST_NOFOLDEROPTIONS,                "Explorer",         "NoFolderOptions"                  ),
DEFINE_POLICY( REST_HASFINDCOMPUTERS,               "Explorer",         "FindComputers"                    ),
DEFINE_POLICY( REST_INTELLIMENUS,                   "Explorer",         "IntelliMenus"                     ),
DEFINE_POLICY( REST_RUNDLGMEMCHECKBOX,              "Explorer",         "MemCheckBoxInRunDlg"              ),
DEFINE_POLICY( REST_ARP_ShowPostSetup,              "Uninstall",        "ShowPostSetup"                    ),
DEFINE_POLICY( REST_NOCSC,                          "Explorer",         "NoSyncAll"                        ),
DEFINE_POLICY( REST_NOCONTROLPANEL,                 "Explorer",         "NoControlPanel"                   ),
DEFINE_POLICY( REST_ENUMWORKGROUP,                  "Explorer",         "EnumWorkgroup"                    ),
DEFINE_POLICY( REST_ARP_NOARP,                      "Uninstall",        "NoAddRemovePrograms"              ),
DEFINE_POLICY( REST_ARP_NOREMOVEPAGE,               "Uninstall",        "NoRemovePage"                     ),
DEFINE_POLICY( REST_ARP_NOADDPAGE,                  "Uninstall",        "NoAddPage"                        ),
DEFINE_POLICY( REST_ARP_NOWINSETUPPAGE,             "Uninstall",        "NoWindowsSetupPage"               ),
DEFINE_POLICY( REST_GREYMSIADS,                     "Explorer",         "GreyMSIAds"                       ),
DEFINE_POLICY( REST_NOCHANGEMAPPEDDRIVELABEL,       "Explorer",         "NoChangeMappedDriveLabel"         ),
DEFINE_POLICY( REST_NOCHANGEMAPPEDDRIVECOMMENT,     "Explorer",         "NoChangeMappedDriveComment"       ),
DEFINE_POLICY( REST_MaxRecentDocs,                  "Explorer",         "MaxRecentDocs"                    ),
DEFINE_POLICY( REST_NONETWORKCONNECTIONS,           "Explorer",         "NoNetworkConnections"             ),
DEFINE_POLICY( REST_FORCESTARTMENULOGOFF,           "Explorer",         "ForceStartMenuLogoff"             ),
DEFINE_POLICY( REST_NOWEBVIEW,                      "Explorer",         "NoWebView"                        ),
DEFINE_POLICY( REST_NOCUSTOMIZETHISFOLDER,          "Explorer",         "NoCustomizeThisFolder"            ),
DEFINE_POLICY( REST_NOENCRYPTION,                   "Explorer",         "NoEncryption"                     ),

#if (WINE_FILEVERSION_MAJOR == 5) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_ALLOWFRENCHENCRYPTION,          "Explorer",         "AllowFrenchEncryption"            ),
#endif

DEFINE_POLICY( REST_DONTSHOWSUPERHIDDEN,            "Explorer",         "DontShowSuperHidden"              ),
DEFINE_POLICY( REST_NOSHELLSEARCHBUTTON,            "Explorer",         "NoShellSearchButton"              ),
DEFINE_POLICY( REST_NOHARDWARETAB,                  "Explorer",         "NoHardwareTab"                    ),

#if (WINE_FILEVERSION_MAJOR <= 6) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NORUNASINSTALLPROMPT,           "Explorer",         "NoRunasInstallPrompt"             ),
DEFINE_POLICY( REST_PROMPTRUNASINSTALLNETPATH,      "Explorer",         "PromptRunasInstallNetPath"        ),
#endif

DEFINE_POLICY( REST_NOMANAGEMYCOMPUTERVERB,         "Explorer",         "NoManageMyComputerVerb"           ),

#if (WINE_FILEVERSION_MAJOR <= 6) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NORECENTDOCSNETHOOD,            "Explorer",         "NoRecentDocsNetHood"              ),
#endif

DEFINE_POLICY( REST_DISALLOWRUN,                    "Explorer",         "DisallowRun"                      ),
DEFINE_POLICY( REST_NOWELCOMESCREEN,                "Explorer",         "NoWelcomeScreen"                  ),
DEFINE_POLICY( REST_RESTRICTCPL,                    "Explorer",         "RestrictCpl"                      ),
DEFINE_POLICY( REST_DISALLOWCPL,                    "Explorer",         "DisallowCpl"                      ),
DEFINE_POLICY( REST_NOSMBALLOONTIP,                 "Explorer",         "NoSMBalloonTip"                   ),
DEFINE_POLICY( REST_NOSMHELP,                       "Explorer",         "NoSMHelp"                         ),
DEFINE_POLICY( REST_NOWINKEYS,                      "Explorer",         "NoWinKeys"                        ),
DEFINE_POLICY( REST_NOENCRYPTONMOVE,                "Explorer",         "NoEncryptOnMove"                  ),
DEFINE_POLICY( REST_NOLOCALMACHINERUN,              "Explorer",         "DisableLocalMachineRun"           ),
DEFINE_POLICY( REST_NOCURRENTUSERRUN,               "Explorer",         "DisableCurrentUserRun"            ),
DEFINE_POLICY( REST_NOLOCALMACHINERUNONCE,          "Explorer",         "DisableLocalMachineRunOnce"       ),
DEFINE_POLICY( REST_NOCURRENTUSERRUNONCE,           "Explorer",         "DisableCurrentUserRunOnce"        ),
DEFINE_POLICY( REST_FORCEACTIVEDESKTOPON,           "Explorer",         "ForceActiveDesktopOn"             ),

#if (WINE_FILEVERSION_MAJOR <= 6) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NOCOMPUTERSNEARME,              "Explorer",         "NoComputersNearMe"                ),
#endif

DEFINE_POLICY( REST_NOVIEWONDRIVE,                  "Explorer",         "NoViewOnDrive"                    ),
DEFINE_POLICY( REST_NONETCRAWL,                     "Explorer",         "NoNetCrawling"                    ),
DEFINE_POLICY( REST_NOSHAREDDOCUMENTS,              "Explorer",         "NoSharedDocuments"                ),
DEFINE_POLICY( REST_NOSMMYDOCS,                     "Explorer",         "NoSMMyDocs"                       ),
DEFINE_POLICY( REST_NOSMMYPICS,                     "Explorer",         "NoSMMyPictures"                   ),
DEFINE_POLICY( REST_ALLOWBITBUCKDRIVES,             "Explorer",         "RecycleBinDrives"                 ),

/* 0x4000050 - 0x4000060 */
#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_NONLEGACYSHELLMODE,             "Explorer",         "NoneLegacyShellMode"              ),
#endif

DEFINE_POLICY( REST_NOCONTROLPANELBARRICADE,        "Explorer",         "NoControlPanelBarricade"          ),

// NOTE: REST_NOSTARTPAGE never really existed.

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_NOAUTOTRAYNOTIFY,               "Explorer",         "NoAutoTrayNotify"                 ),
DEFINE_POLICY( REST_NOTASKGROUPING,                 "Explorer",         "NoTaskGrouping"                   ),
DEFINE_POLICY( REST_NOCDBURNING,                    "Explorer",         "NoCDBurning"                      ),
#endif // WINE_FILEVERSION_MAJOR

DEFINE_POLICY( REST_MYCOMPNOPROP,                   "Explorer",         "NoPropertiesMyComputer"           ),
DEFINE_POLICY( REST_MYDOCSNOPROP,                   "Explorer",         "NoPropertiesMyDocuments"          ),

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_NOSTARTPANEL,                   "Explorer",         "NoSimpleStartMenu"                ),
DEFINE_POLICY( REST_NODISPLAYAPPEARANCEPAGE,        "System",           "NoDispAppearancePage"             ),  // REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE
DEFINE_POLICY( REST_NOTHEMESTAB,                    "Explorer",         "NoThemesTab"                      ),
DEFINE_POLICY( REST_NOVISUALSTYLECHOICE,            "System",           "NoVisualStyleChoice"              ),
DEFINE_POLICY( REST_NOSIZECHOICE,                   "System",           "NoSizeChoice"                     ),
DEFINE_POLICY( REST_NOCOLORCHOICE,                  "System",           "NoColorChoice"                    ),
DEFINE_POLICY( REST_SETVISUALSTYLE,                 "System",           "SetVisualStyle"                   ),
#endif // WINE_FILEVERSION_MAJOR

DEFINE_POLICY( REST_STARTRUNNOHOMEPATH,             "Explorer",         "StartRunNoHOMEPATH"               ),

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_NOUSERNAMEINSTARTPANEL,         "Explorer",         "NoUserNameInStartMenu"            ),
DEFINE_POLICY( REST_NOMYCOMPUTERICON,               "NonEnum",          "{20D04FE0-3AEA-1069-A2D8-08002B30309D}" ),
DEFINE_POLICY( REST_NOSMNETWORKPLACES,              "Explorer",         "NoStartMenuNetworkPlaces"         ),
DEFINE_POLICY( REST_NOSMPINNEDLIST,                 "Explorer",         "NoStartMenuPinnedList"            ),
DEFINE_POLICY( REST_NOSMMYMUSIC,                    "Explorer",         "NoStartMenuMyMusic"               ),
DEFINE_POLICY( REST_NOSMEJECTPC,                    "Explorer",         "NoStartMenuEjectPC"               ),
DEFINE_POLICY( REST_NOSMMOREPROGRAMS,               "Explorer",         "NoStartMenuMorePrograms"          ),
DEFINE_POLICY( REST_NOSMMFUPROGRAMS,                "Explorer",         "NoStartMenuMFUprogramsList"       ),
DEFINE_POLICY( REST_NOTRAYITEMSDISPLAY,             "Explorer",         "NoTrayItemsDisplay"               ),
DEFINE_POLICY( REST_NOTOOLBARSONTASKBAR,            "Explorer",         "NoToolbarsOnTaskbar"              ),
#endif // WINE_FILEVERSION_MAJOR

DEFINE_POLICY( REST_NOSMCONFIGUREPROGRAMS,          "Explorer",         "NoSMConfigurePrograms"            ),

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_HIDECLOCK,                      "Explorer",         "HideClock"                        ),
DEFINE_POLICY( REST_NOLOWDISKSPACECHECKS,           "Explorer",         "NoLowDiskSpaceChecks"             ),
#endif

#if (WINE_FILEVERSION_MAJOR <= 6) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NOENTIRENETWORK,                "Network",          "NoEntireNetwork"                  ),  // REGSTR_VAL_NOENTIRENETWORK
DEFINE_POLICY( REST_NODESKTOPCLEANUP,               "Explorer",         "NoDesktopCleanupWizard"           ),
#endif

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_BITBUCKNUKEONDELETE,            "Explorer",         "NoRecycleFiles"                   ),
DEFINE_POLICY( REST_BITBUCKCONFIRMDELETE,           "Explorer",         "ConfirmFileDelete"                ),
DEFINE_POLICY( REST_BITBUCKNOPROP,                  "Explorer",         "NoPropertiesRecycleBin"           ),
DEFINE_POLICY( REST_NODISPBACKGROUND,               "System",           "NoDispBackgroundPage"             ),  // REGSTR_VAL_DISPCPL_NOBACKGROUNDPAGE
DEFINE_POLICY( REST_NODISPSCREENSAVEPG,             "System",           "NoDispScrSavPage"                 ),  // REGSTR_VAL_DISPCPL_NOSCRSAVPAGE
DEFINE_POLICY( REST_NODISPSETTINGSPG,               "System",           "NoDispSettingsPage"               ),  // REGSTR_VAL_DISPCPL_NOSETTINGSPAGE
DEFINE_POLICY( REST_NODISPSCREENSAVEPREVIEW,        "System",           "NoScreenSavePreview"              ),
DEFINE_POLICY( REST_NODISPLAYCPL,                   "System",           "NoDispCPL"                        ),  // REGSTR_VAL_DISPCPL_NODISPCPL
DEFINE_POLICY( REST_HIDERUNASVERB,                  "Explorer",         "HideRunAsVerb"                    ),
DEFINE_POLICY( REST_NOTHUMBNAILCACHE,               "Explorer",         "NoThumbnailCache"                 ),
DEFINE_POLICY( REST_NOSTRCMPLOGICAL,                "Explorer",         "NoStrCmpLogical"                  ),
DEFINE_POLICY( REST_NOPUBLISHWIZARD,                "Explorer",         "NoPublishingWizard"               ),
DEFINE_POLICY( REST_NOONLINEPRINTSWIZARD,           "Explorer",         "NoOnlinePrintsWizard"             ),
DEFINE_POLICY( REST_NOWEBSERVICES,                  "Explorer",         "NoWebServices"                    ),
#endif // WINE_FILEVERSION_MAJOR

DEFINE_POLICY( REST_ALLOWUNHASHEDWEBVIEW,           "Explorer",         "AllowUnhashedWebView"             ),

#if (WINE_FILEVERSION_MAJOR >= 6)
DEFINE_POLICY( REST_ALLOWLEGACYWEBVIEW,             "Explorer",         "AllowLegacyWebView"               ),
#endif

DEFINE_POLICY( REST_REVERTWEBVIEWSECURITY,          "Explorer",         "RevertWebViewSecurity"            ),
DEFINE_POLICY( REST_INHERITCONSOLEHANDLES,          "Explorer",         "InheritConsoleHandles"            ),

#if (WINE_FILEVERSION_MAJOR >= 6)

#if (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_SORTMAXITEMCOUNT,               "Explorer",         "SortMaxItemCount"                 ),
#endif
DEFINE_POLICY( REST_NOREMOTERECURSIVEEVENTS,        "Explorer",         "NoRemoteRecursiveEvents"          ),

#endif // WINE_FILEVERSION_MAJOR

DEFINE_POLICY( REST_NOREMOTECHANGENOTIFY,           "Explorer",         "NoRemoteChangeNotify"             ),

#if (WINE_FILEVERSION_MAJOR >= 6)

#if (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NOSIMPLENETIDLIST,              "Explorer",         "NoSimpleNetIDList"                ),
#endif

// #if (NTDDI_VERSION < NTDDI_LONGHORN)
// NOTE: Geoff Chappell is inacurrate here.
  DEFINE_POLICY( REST_NOENUMENTIRENETWORK,          "Explorer",         "NoEnumEntireNetwork"              ),
// #endif

#if (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_NODETAILSTHUMBNAILONNETWORK,    "Explorer",         "NoDetailsThumbnailOnNetwork"      ),
#endif

DEFINE_POLICY( REST_NOINTERNETOPENWITH,             "Explorer",         "NoInternetOpenWith"               ),

#if (NTDDI_VERSION >= NTDDI_WINXPSP2) && (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_ALLOWLEGACYLMZBEHAVIOR,         "Explorer",         "AllowLegacyLMZBehavior"           ),
#endif

DEFINE_POLICY( REST_DONTRETRYBADNETNAME,            "Explorer",         "DontRetryBadNetName"              ),
DEFINE_POLICY( REST_ALLOWFILECLSIDJUNCTIONS,        "Explorer",         "AllowFileCLSIDJunctions"          ),
DEFINE_POLICY( REST_NOUPNPINSTALL,                  "Explorer",         "NoUPnPInstallTask"                ),

// "NormalizeLinkNetPidls" only in version 6.0 from Windows XP SP3.

#if (NTDDI_VERSION >= NTDDI_LONGHORN)
DEFINE_POLICY( REST_ARP_DONTGROUPPATCHES,           "Uninstall",        "DontGroupPatches"                 ),
DEFINE_POLICY( REST_ARP_NOCHOOSEPROGRAMSPAGE,       "Uninstall",        "NoChooseProgramsPage"             ),
#endif

// "AllowCLSIDPROGIDMapping" in Windows XP SP3 and Windows Server 2003 SP2 only.
// Maybe in Vista+ too?

#endif // WINE_FILEVERSION_MAJOR

/* 0x4000061 - 0x4000086 */
DEFINE_POLICY( REST_NODISCONNECT,                   "Explorer",         "NoDisconnect"                     ),
DEFINE_POLICY( REST_NOSECURITY,                     "Explorer",         "NoNTSecurity"                     ),
DEFINE_POLICY( REST_NOFILEASSOCIATE,                "Explorer",         "NoFileAssociate"                  ),

#if (WINE_FILEVERSION_MAJOR >= 6)

// #if (NTDDI_VERSION < NTDDI_LONGHORN)
// NOTE: Either Geoff Chappell or MSDN is inaccurate here.
DEFINE_POLICY( REST_ALLOWCOMMENTTOGGLE,             "Explorer",         "ToggleCommentPosition"            ),
// #endif
#if (NTDDI_VERSION < NTDDI_LONGHORN)
DEFINE_POLICY( REST_USEDESKTOPINICACHE,             "Explorer",         "UseDesktopIniCache"               ),
#endif

// "NoNetFolderInfoTip" only in version 6.0 from Windows XP SP3.

#endif // WINE_FILEVERSION_MAJOR

// NOTE: This is a SHDOCVW-only policy.
// DEFINE_POLICY( 0x50000024,   "Explorer",     "NoFileUrl" ),
