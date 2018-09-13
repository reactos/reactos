;;; this file is included by shell32\selfreg.inx
;;; it doesn't have #ifdef POSTSPLIT because it should not have been included otherwise

[RegShell32NT]
; NT specific Advanced Options settings
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","Text",,%DESC_ShowCompColor%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","ValueName",,"ShowCompColor"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowCompColor","HelpID",,"update.hlp#51130"



[RegShell32Part2]

; =========== Explorer View Advanced Options
; In: IE4 (B2)
HKLM,"%PATH_EXPLORER%\Advanced\Folder","Type",,"group"
HKLM,"%PATH_EXPLORER%\Advanced\Folder","Text",,%DESC_FileFolder%
HKLM,"%PATH_EXPLORER%\Advanced\Folder","Bitmap",,"%_MOD_PATH%,4"
HKLM,"%PATH_EXPLORER%\Advanced\Folder","HelpID",,"update.hlp#51140"
;
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden","Text",,"%DESC_HiddenFiles%"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden","Type",,"group"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden","Bitmap",,"%_MOD_PATH%,4"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden","HelpID",,"update.hlp#51131"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","Text",,"%DESC_ShowAll%"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","Type",,"radio"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","ValueName",,"Hidden"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\SHOWALL","HelpID",,"update.hlp#51105"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","Text",,"%DESC_NoHidden%"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","Type",,"radio"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","CheckedValue",%REGDW%,0x00000002
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","ValueName",,"Hidden"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDDEN","HelpID",,"update.hlp#51104"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","Text",,"%DESC_NoHidorSys%"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","Type",,"radio"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","CheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","ValueName",,"Hidden"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\Hidden\NOHIDORSYS","HelpID",,"update.hlp#51103"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","Text",,%DESC_DontPrettyPath%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","ValueName",,"DontPrettyPath"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DontPrettyPath","HelpID",,"update.hlp#51072"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","Text",,%DESC_ShowInfoTip%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","ValueName",,"ShowInfoTip"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","DefaultValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowInfoTip","HelpID",,"update.hlp#51102"

HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","Text",,%DESC_MapNetDrvBtn%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","ValueName",,"MapNetDrvBtn"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\MapNetDrvBtn","HelpID",,"update.hlp#51070"
;
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","Text",,%DESC_HideFileExt%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","ValueName",,"HideFileExt"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","DefaultValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\HideFileExt","HelpID",,"update.hlp#51101"
;
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","Text",,%DESC_ShowFullPath%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","RegPath",,"%PATH_EXPLORER%\CabinetState"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","ValueName",,"FullPath"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","UncheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ShowFullPath","HelpID",,"update.hlp#51100"
;
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","Text",,%DESC_ClassicViewState%
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","ValueName",,"ClassicViewState"
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","CheckedValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","UncheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","DefaultValue",%REGDW%,0x00000000
HKLM,"%PATH_EXPLORER%\Advanced\Folder\ClassicViewState","HelpID",,"update.hlp#51076"

HKLM,"%PATH_EXPLORER%\Advanced\Visual","Type",,"group"
HKLM,"%PATH_EXPLORER%\Advanced\Visual","Text",,%DESC_Visual%
HKLM,"%PATH_EXPLORER%\Advanced\Visual","Bitmap",,"%_MOD_PATH%,6"

HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","Text",,%DESC_DragFullWin%
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","RegPath",,"Control Panel\Desktop"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","ValueName",,"DragFullWindows"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","CheckedValue",,"1"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","UncheckedValue",,"0"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","DefaultValue",,"1"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","SPIAction",%REGDW%,0x00000025
HKLM,"%PATH_EXPLORER%\Advanced\Visual\DragFullWin","HelpID",,"update.hlp#51074"

HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","Text",,%DESC_FontSmooth%
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","RegPath",,"Control Panel\Desktop"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","ValueName",,"FontSmoothing"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","CheckedValueNT",,"2"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","CheckedValueW95",,"1"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","UncheckedValue",,"0"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","DefaultValue",,"0"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","SPIAction",%REGDW%,0x0000004b
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","SPIParamON",%REGDW%,0x00000002
HKLM,"%PATH_EXPLORER%\Advanced\Visual\FontSmooth","HelpID",,"update.hlp#51073"

HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","Type",,"checkbox"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","Text",,%DESC_HideIcons%
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","HKeyRoot",%REGDW%,0x80000001
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","RegPath",,"%PATH_EXPLORER%\Advanced"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","ValueName",,"HideIcons"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","CheckedValue",%REGDW%,0x00000001
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","UncheckedValue",%REGDW%,"0"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","DefaultValue",,"0"
HKLM,"%PATH_EXPLORER%\Advanced\Visual\HideIcons","HelpID",,"update.hlp#51106"


; default toolbars (taskbar, deskbars, and any other toolbars)
;
; as of 970212 (ie4, beta-1):
; - taskbar: has ISF band "ie 4.0" no title no text, and task band
; - toolbars: none
; - (machine name is ie_500, disk name is ie_500_c, profile is ie500.000)
; In: IE4 (B1, B2)
HKLM,"%PATH_EXPLORER%\Streams\Desktop","Default Taskbar",1,\
  0c,00,00,00,08,00,00,00,02,00,00,00,00,00,00,00,b0,e2,2b,\
  d8,64,57,d0,11,a9,6e,00,c0,4f,d7,05,a2,22,00,1c,00,0a,01,00,00,1a,00,00,00,\
  01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,4c,00,00,00,01,14,02,00,00,\
  00,00,00,c0,00,00,00,00,00,00,46,83,00,00,00,10,00,00,00,a0,b0,48,5e,89,8d,\
  bd,01,00,d8,9e,e4,2a,8d,bd,01,00,45,fe,5e,89,8d,bd,01,00,00,00,00,00,00,00,\
  00,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,13,01,14,00,1f,0f,e0,4f,\
  d0,20,ea,3a,69,10,a2,d8,08,00,2b,30,30,9d,19,00,23,44,3a,5c,00,00,00,00,00,\
  00,00,00,00,00,00,00,00,00,00,00,00,cd,97,15,00,31,00,00,00,00,00,af,24,5c,\
  b9,10,80,57,69,6e,6e,74,00,00,20,00,31,00,00,00,00,00,af,24,a3,b9,10,00,50,\
  72,6f,66,69,6c,65,73,00,50,52,4f,46,49,4c,45,53,00,19,00,31,00,00,00,00,00,\
  c1,24,08,92,10,00,69,65,35,30,30,2e,30,30,30,00,00,28,00,31,00,00,00,00,00,\
  c1,24,08,92,10,00,41,70,70,6c,69,63,61,74,69,6f,6e,20,44,61,74,61,00,41,50,\
  50,4c,49,43,7e,31,00,21,00,31,00,00,00,00,00,c5,22,9d,91,10,00,4d,69,63,72,\
  6f,73,6f,66,74,00,4d,49,43,52,4f,53,7e,31,00,29,00,31,00,00,00,00,00,d3,22,\
  32,a6,10,00,49,6e,74,65,72,6e,65,74,20,45,78,70,6c,6f,72,65,72,00,49,4e,54,\
  45,52,4e,7e,31,00,24,00,31,00,00,00,00,00,db,22,76,ba,10,00,51,75,69,63,6b,\
  20,4c,61,75,6e,63,68,00,51,55,49,43,4b,4c,7e,31,00,00,00,8d,00,00,00,1c,00,\
  00,00,01,00,00,00,1c,00,00,00,36,00,00,00,0d,f0,ad,ba,8c,00,00,00,1a,00,00,\
  00,03,00,00,00,e7,18,2d,23,10,00,00,00,49,45,35,30,30,5f,58,31,43,00,43,3a,\
  5c,57,49,4e,4e,54,5c,50,72,6f,66,69,6c,65,73,5c,49,45,35,30,30,2e,30,30,30,\
  5c,41,70,70,6c,69,63,61,74,69,6f,6e,20,44,61,74,61,5c,4d,69,63,72,6f,73,6f,\
  66,74,5c,49,6e,74,65,72,6e,65,74,20,45,78,70,6c,6f,72,65,72,5c,51,75,69,63,\
  6b,20,4c,61,75,6e,63,68,00,00,10,00,00,00,05,00,00,a0,1a,00,00,00,a3,00,00,\
  00,00,00,00,00,08,00,00,00,02,00,00,00,cc,00,00,00,01,00,00,00,03,00,00,00,\
  4a,00,00,00,01,00,00,00,40,00,32,00,e6,01,00,00,c4,24,15,9f,20,00,4c,61,75,\
  6e,63,68,20,49,6e,74,65,72,6e,65,74,20,45,78,70,6c,6f,72,65,72,20,42,72,6f,\
  77,73,65,72,2e,6c,6e,6b,00,4c,41,55,4e,43,48,7e,32,2e,4c,4e,4b,00,00,00,40,\
  00,00,00,02,00,00,00,36,00,32,00,48,02,00,00,c4,24,15,9f,20,00,4c,61,75,6e,\
  63,68,20,4f,75,74,6c,6f,6f,6b,20,45,78,70,72,65,73,73,2e,6c,6e,6b,00,4c,41,\
  55,4e,43,48,7e,31,2e,4c,4e,4b,00,00,00,36,00,00,00,00,00,00,00,2c,00,32,00,\
  51,00,00,00,ec,22,46,39,20,00,53,68,6f,77,20,44,65,73,6b,74,6f,70,2e,73,63,\
  66,00,53,48,4f,57,44,45,7e,31,2e,53,43,46,00,00,00,52,00,00,00,e0,00,00,00,\
  00,00,00,00,16,00,00,00,00,00,00,00,00,00,00,00,16,00,00,00,00,00,00,00,01,\
  00,00,00,01,00,00,00,aa,4f,28,68,48,6a,d0,11,8c,78,00,c0,4f,d9,18,b4,37,02,\
  00,00,e0,00,00,00,00,00,00,00,16,00,00,00,00,00,00,00,00,00,00,00,16,00,00,\
  00,00,00,00,00,01,00,00,00

HKLM,"%PATH_EXPLORER%\Streams\Desktop","Default Toolbars",1,\
  11,00,00,00,00,00,00,00



; CopyTo Menu
; In: IE4 (Memphis)
HKCR,"CLSID\%CLSID_CopyToMenu%",,,"%DESC_CopyToMenu%"
HKCR,"CLSID\%CLSID_CopyToMenu%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_CopyToMenu%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_CopyToMenu%","flags",65537,0x00000001
HKLM,"%SMWCVSEA%","%CLSID_CopyToMenu%",,"%DESC_CopyToMenu%"

; MoveTo Menu
; In: IE4 (Memphis)
HKCR,"CLSID\%CLSID_MoveToMenu%",,,"%DESC_MoveToMenu%"
HKCR,"CLSID\%CLSID_MoveToMenu%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_MoveToMenu%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_MoveToMenu%","flags",65537,0x00000001
HKLM,"%SMWCVSEA%","%CLSID_MoveToMenu%",,"%DESC_MoveToMenu%"

; In: IE4
; Shell Automation classes
HKCR,"CLSID\%CLSID_Shell%",,,"%DESC_ShellApp%"
HKCR,"CLSID\%CLSID_Shell%\%IPS%",,,"%_MOD_PATH%"
HKCR,"CLSID\%CLSID_Shell%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_Shell%\ProgID",,,"Shell.Application.1"
HKCR,"CLSID\%CLSID_Shell%\TypeLib",,,"%CLSID_Shell32TypeLib%"
HKCR,"CLSID\%CLSID_Shell%\Version",,,"1.1"
HKCR,"CLSID\%CLSID_Shell%\VersionIndependentProgID",,,"Shell.Application"
HKLM,"SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved","%CLSID_Shell%",,"%DESC_ShellApp%"

; In: IE4
HKCR,"Shell.Application",,,"%DESC_ShellApp%"
HKCR,"Shell.Application\CLSID",,,"%CLSID_Shell%"
HKCR,"Shell.Application\CurVer",,,"Shell.Application.1"
HKCR,"Shell.Application.1",,,"%DESC_ShellApp%"
HKCR,"Shell.Application.1\CLSID",,,"%CLSID_Shell%"

; In: IE4
HKCR,"CLSID\%CLSID_ShellFolderView%",,,"%DESC_ShellFolderView%"
HKCR,"CLSID\%CLSID_ShellFolderView%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_ShellFolderView%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved","%CLSID_ShellFolderView%",,"%DESC_ShellFolderView%"

; In: IE4
HKCR,"CLSID\%CLSID_ShellFolderViewOC%",,,"%DESC_ShellFolderViewOC%"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\Control",,,""
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\%IPS%",,,"%_MOD_PATH%"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\MiscStatus",,,"0"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\MiscStatus\1",,,"17408"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\ProgID",,,"Shell.FolderView.1"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\ToolboxBitmap32",,,"%_MOD_PATH%, 260"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\TypeLib",,,"%CLSID_Shell32TypeLib%"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\Version",,,"1.1"
HKCR,"CLSID\%CLSID_ShellFolderViewOC%\VersionIndependentProgID",,,"Shell.FolderView"
HKCR,"Shell.FolderView",,,"%DESC_ShellFolderViewOC%"
HKCR,"Shell.FolderView\CLSID",,,"%CLSID_ShellFolderViewOC%"
HKCR,"Shell.FolderView\CurVer",,,"Shell.FolderView.1"
HKCR,"Shell.FolderView.1",,,"%DESC_ShellFolderViewOC%"
HKCR,"Shell.FolderView.1\CLSID",,,"%CLSID_ShellFolderViewOC%"

; CLSID_CDefViewOC
; In: IE4
HKCR,"CLSID\%CLSID_CDefViewOC%",,,"%DESC_DefViewOC%"
HKCR,"CLSID\%CLSID_CDefViewOC%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_CDefViewOC%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_CDefViewOC%\TypeLib",,,"%CLSID_Shell32TypeLib%"
HKCR,"CLSID\%CLSID_CDefViewOC%\Implemented Categories\%CATID_SafeForScripting%",,,""
HKCR,"CLSID\%CLSID_CDefViewOC%\Implemented Categories\%CATID_SafeForInitializing%",,,""

; CLSID_StartMenu
; In: IE4
HKCR,"CLSID\%CLSID_StartMenu%",,,"%DESC_StartMenu%"
HKCR,"CLSID\%CLSID_StartMenu%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_StartMenu%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved","%CLSID_StartMenu%",,"%DESC_StartMenu%"

; Standard shell namespace settings
HKCR,"AllFilesystemEditObjects\shellex\ContextMenuHandlers\Copy To",,,"%CLSID_CopyToMenu%"
HKCR,"AllFilesystemEditObjects\shellex\ContextMenuHandlers\Move To",,,"%CLSID_MoveToMenu%"

; Install Web View
;
HKLM,"Software\Microsoft\Windows\CurrentVersion\ExtShellViews\%VID_WebView%","MenuName",,"%DESC_WebView_MenuName%"
HKLM,"Software\Microsoft\Windows\CurrentVersion\ExtShellViews\%VID_WebView%","HelpText",,"%DESC_WebView_HelpText%"
HKLM,"Software\Microsoft\Windows\CurrentVersion\ExtShellViews\%VID_WebView%","TooltipText",,"%DESC_WebView_TooltipText%"
HKCR,"CLSID\%CLSID_ShellDesktop%\%EXTVIEW%\%VID_WebView%","PersistMoniker",%REGEXSZ%,"file://%%userappdata%%\Microsoft\Internet Explorer\Desktop.htt"
HKCR,"CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\%EXTVIEW%\%VID_WebView%","PersistMoniker",%REGEXSZ%,"file://%25%\web\mycomp.htt"
HKCR,"CLSID\{21EC2020-3AEA-1069-A2DD-08002B30309D}\%EXTVIEW%\%VID_WebView%","PersistMoniker",%REGEXSZ%,"file://%25%\web\controlp.htt"
HKCR,"CLSID\{2227A280-3AEA-1069-A2DE-08002B30309D}\%EXTVIEW%\%VID_WebView%","PersistMoniker",%REGEXSZ%,"file://%25%\web\printers.htt"
HKCR,"Directory\%EXTVIEW%\%VID_WebView%","PersistMoniker",%REGEXSZ%,"file://%25%\web\folder.htt"

; Old view PersistFile format
HKCR,"CLSID\%CLSID_ShellDesktop%\%EXTVIEW%\%VID_WebView%","PersistFile"

; CLSID_CWebViewMimeFilter and associated template registration
; In: IE4
HKCR,"CLSID\%CLSID_CWebViewMimeFilter%",,,"%DESC_WebViewMimeFilter%"
HKCR,"CLSID\%CLSID_CWebViewMimeFilter%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_CWebViewMimeFilter%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"PROTOCOLS\Filter\text/webviewhtml",,,"%DESC_WebViewMimeFilter%"
HKCR,"PROTOCOLS\Filter\text/webviewhtml","CLSID",,"%CLSID_CWebViewMimeFilter%"
HKCR,"MIME\Database\Content Type\text/webviewhtml","CLSID",,"%CLSID_Mshtml%"
HKCR,"MIME\Database\Content Type\text/webviewhtml","Extension",,".htt"
HKCR,".htt",,2,"HTTfile"
HKCR,".htt","Content Type",,"text/webviewhtml"
HKCR,"HTTfile",,2,%HYPERTEXT_TEM%
HKCR,"HTTfile",EditFlags,3,00,00,01,00
HKCR,"HTTfile\DefaultIcon",,%REGEXSZ%,"%_MOD_PATH%,-6"

; DeskMovr OCX registration and unregistration 
HKCR,"CLSID\%CLSID_DeskMovr%",,,"%DESC_DeskMovr%"
HKCR,"CLSID\%CLSID_DeskMovr%\%IPS%",,,"%_MOD_PATH%"
HKCR,"CLSID\%CLSID_DeskMovr%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_DeskMovr%\Programmable",,,""
HKCR,"CLSID\%CLSID_DeskMovr%\Implemented Categories\%CATID_SafeForScripting%",,,""
HKCR,"CLSID\%CLSID_DeskMovr%\Implemented Categories\%CATID_SafeForInitializing%",,,""

; CLSID_StartMenuTask
; In: IE4
HKCR,"CLSID\%CLSID_StartMenuTask%",,,"%DESC_StartMenuTask%"
HKCR,"CLSID\%CLSID_StartMenuTask%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_StartMenuTask%\%IPS%","ThreadingModel",,"Both"

; CLSID_DesktopTask
; In: IE4
HKCR,"CLSID\%CLSID_DesktopTask%",,,"%DESC_DesktopTask%"
HKCR,"CLSID\%CLSID_DesktopTask%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_DesktopTask%\%IPS%","ThreadingModel",,"Both"


; SendTo Menu
; In: IE4
HKCR,"CLSID\%CLSID_SendToMenu%",,,"%DESC_SendToMenu%"
HKCR,"CLSID\%CLSID_SendToMenu%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_SendToMenu%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_SendToMenu%","flags",65537,0x00000001
HKLM,"%SMWCVSEA%","%CLSID_SendToMenu%",,"%DESC_SendToMenu%"
HKCR,"AllFilesystemObjects\shellex\ContextMenuHandlers\Send To",,,"%CLSID_SendToMenu%"

; New Menu
; In: IE4
HKCR,"CLSID\%CLSID_NewMenu%",,,"%DESC_NewMenu%"
HKCR,"CLSID\%CLSID_NewMenu%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_NewMenu%\%IPS%","ThreadingModel",,"Apartment"
HKCR,"CLSID\%CLSID_NewMenu%","flags",65537,0x00000000
HKLM,"%SMWCVSEA%","%CLSID_NewMenu%",,"%DESC_NewMenu%"
;   Fix .lnk to work with the New Newmenu
HKCR,".lnk\ShellNew","Command",,"RunDLL32 AppWiz.Cpl,NewLinkHere %2"
HKCR,"Directory\Background\shellex\ContextMenuHandlers\New",,,"%CLSID_NewMenu%"

; Keys required to configure the Active Desktop
; In: IE4 (B?)
HKCR,"CLSID\%CLSID_DeskHtmlProp%",,,"%DESC_DeskHtmlProp%"
HKCR,"CLSID\%CLSID_DeskHtmlProp%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_DeskHtmlProp%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"%SMWCV%\Controls Folder\Display\shellex\PropertySheetHandlers\DeskHtmlExt",,,"%CLSID_DeskHtmlProp%"
HKLM,"%SMWCVSEA%","%CLSID_DeskHtmlProp%",,"%DESC_DeskHtmlProp%"

HKCR,"CLSID\%CLSID_ActiveDesktop%",,,"%DESC_ActiveDesktop%"
HKCR,"CLSID\%CLSID_ActiveDesktop%\%IPS%",,%REGEXSZ%,"%_MOD_PATH%"
HKCR,"CLSID\%CLSID_ActiveDesktop%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"%SMWCVSEA%","%CLSID_ActiveDesktop%",,"%DESC_ActiveDesktop%"

; CLSID_ShellFldSetExt
; In: IE5
HKCR,"CLSID\%CLSID_ShellFldSetExt%",,,"%DESC_ShellFldSetExt%"
HKCR,"CLSID\%CLSID_ShellFldSetExt%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_ShellFldSetExt%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"%SMWCVSEA%","%CLSID_ShellFldSetExt%",,"%DESC_ShellFldSetExt%"

; CLSID_CmdFileIcon
; In: IE4
HKCR,"CLSID\%CLSID_CmdFileIcon%",,,"%DESC_CmdFileIcon%"
HKCR,"CLSID\%CLSID_CmdFileIcon%\%IPS%",,%REGEXSZ%,"%_SYS_MOD_PATH%"
HKCR,"CLSID\%CLSID_CmdFileIcon%\%IPS%","ThreadingModel",,"Apartment"
HKLM,"%SMWCVSEA%","%CLSID_CmdFileIcon%",,"%DESC_CmdFileIcon%"

; Standard shell namespace settings
; In: IE4
HKCR,"Folder\shell\open\ddeexec","NoActivateHandler",,""
HKCR,"Folder\shell\explore\ddeexec","NoActivateHandler",,""
HKCR,"Drive\shell\find\ddeexec","NoActivateHandler",,""
HKCR,"Directory\shell\find\ddeexec","NoActivateHandler",,""

HKCR,".scf",,,"SHCmdFile"
HKCR,"SHCmdFile","NeverShowExt",,""
HKCR,"SHCmdFile",,,"Windows Explorer Command"
HKCR,"SHCmdFile\CLSID",,,"{57651662-CE3E-11D0-8D77-00C04FC99D61}"
HKCR,"SHCmdFile\shellex\IconHandler",,,"{57651662-CE3E-11D0-8D77-00C04FC99D61}"
HKCR,"SHCmdFile\shell\open\command",,,"explorer.exe"
HKCR,"SHCmdFile\shell\open\ddeexec",,,"[ShellFile(""%1"",""%1"",%S)]"
HKCR,"SHCmdFile\shell\open\ddeexec","NoActivateHandler",,""
HKCR,"SHCmdFile\shell\open\ddeexec\Topic",,,"AppProperties"
HKCR,"SHCmdFile\shell\open\ddeexec\Application",,,"Folders"

[RegShell32Part2.Del]
; Desktop in Separate Process is not supported by IE4's Explorer
; IE5 Beta 1 added it by mistake, so we need to clean it out
HKLM,"%PATH_EXPLORER%\Advanced\Folder\DesktopProcess"

[Strings]
; These lines compress the file.
SMWCV="Software\Microsoft\Windows\CurrentVersion"
SMWCVSEA="Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
extview="shellex\ExtShellFolderViews"
PATH_EXPLORER="Software\Microsoft\Windows\CurrentVersion\Explorer"

CLSID_CopyToMenu        = "{C2FBB630-2971-11d1-A18C-00C04FD75D13}"
CLSID_MoveToMenu        = "{C2FBB631-2971-11d1-A18C-00C04FD75D13}"
CLSID_StartMenuTask     = "{DDB008FE-048D-11d1-B9CD-00C04FC2C1D2}"
CLSID_DesktopTask       = "{E7D3DB4E-199C-11d1-9828-00C04FD91972}"
CLSID_CWebViewMimeFilter = "{733AC4CB-F1A4-11d0-B951-00A0C90312E1}"
CLSID_DeskMovr          = "{72267F6A-A6F9-11D0-BC94-00C04FB67863}"
CLSID_DeskHtmlProp      = "{3FC0B520-68A9-11D0-8D77-00C04FD70822}"
CLSID_ActiveDesktop     = "{75048700-EF1F-11D0-9888-006097DEACF9}"
CLSID_Shell             = "{13709620-C279-11CE-A49E-444553540000}"
CLSID_CDefViewOC        = "{1820FED0-473E-11D0-A96C-00C04FD705A2}"
CLSID_ShellFolderView   = "{62112AA1-EBE4-11cf-A5FB-0020AFE7292D}"
CLSID_ShellFolderViewOC = "{9BA05971-F6A8-11CF-A442-00A0C90A8F39}"
CLSID_Mshtml            = "{25336920-03F9-11CF-8FD0-00AA00686F13}"
CLSID_ShellFldSetExt    = "{6D5313C0-8C62-11D1-B2CD-006097DF8C11}"
CLSID_StartMenu         = "{4622AD11-FF23-11d0-8D34-00A0C90F2719}"
CLSID_CmdFileIcon       = "{57651662-CE3E-11D0-8D77-00C04FC99D61}"
CLSID_ShellDesktop      = "{00021400-0000-0000-C000-000000000046}"
CLSID_SendToMenu        = "{7BA4C740-9E81-11CF-99D3-00AA004AE837}"
CLSID_NewMenu           = "{D969A300-E7FF-11d0-A93B-00A0C90F2719}"

VID_WebView                     = "{5984FFE0-28D4-11CF-AE66-08002B2E1262}"

DESC_SendToMenu         = "Microsoft SendTo Service"
DESC_NewMenu            = "Microsoft New Object Service"
DESC_CopyToMenu         = "Microsoft CopyTo Service"
DESC_MoveToMenu         = "Microsoft MoveTo Service"
DESC_StartMenuTask      = "Start Menu Task"
DESC_DesktopTask        = "Desktop Task"
DESC_WebViewMimeFilter  = "WebView MIME Filter"
DESC_DeskMovr           = "Active Desktop Mover"
DESC_DeskHtmlProp       = "Display Control Panel HTML Extensions"
DESC_ActiveDesktop      = "ActiveDesktop"
DESC_ShellFldSetExt     = "Folder Options Property Page Extension"
DESC_ShellApp           = "Shell Automation Service"
DESC_DefViewOC          = "Shell DefView"
DESC_ShellFolderView    = "Shell Automation Folder View"
DESC_ShellFolderViewOC  = "Microsoft Shell Folder View Router"
DESC_StartMenu          = "Start Menu"
DESC_CmdFileIcon        = "CmdFileIcon"

DESC_WebView_MenuName           = "as &Web Page"
DESC_WebView_HelpText           = "Displays items in Web View"
DESC_WebView_TooltipText        = "Web"

HYPERTEXT_TEM           = "HyperText Template"

; Advanced option descriptions:
DESC_FileFolder         = "Files and Folders"
DESC_DontPrettyPath     = "Allow all uppercase names"
DESC_ShowInfoTip        = "Show pop-up description for folder and desktop items."
DESC_MapNetDrvBtn       = "Show Map Network Drive button in toolbar"
DESC_Visual             = "Visual Settings"
DESC_DragFullWin        = "Show window contents while dragging"
DESC_FontSmooth         = "Smooth edges of screen fonts"
DESC_HideIcons          = "Hide icons when desktop is viewed as Web page"
DESC_HideFileExt        = "Hide file extensions for known file types"
DESC_HiddenFiles        = "Hidden files"
DESC_ShowAll            = "Show all files"
DESC_NoHidden           = "Do not show hidden files"
DESC_NoHidorSys         = "Do not show hidden or system files"
DESC_ShowFullPath       = "Display the full path in title bar"
DESC_ShowCompColor      = "Display compressed files and folders with alternate color"
DESC_ClassicViewState   = "Remember each folder's view settings"
