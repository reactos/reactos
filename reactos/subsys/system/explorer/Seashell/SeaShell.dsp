# Microsoft Developer Studio Project File - Name="SeaShell" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=SeaShell - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SeaShell.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SeaShell.mak" CFG="SeaShell - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SeaShell - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "SeaShell - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SeaShell - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "SeaShellExt\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "SeaShell - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "SeaShellExt\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SeaShell - Win32 Release"
# Name "SeaShell - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FilterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MFCExplorerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShell.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShell.rc
# End Source File
# Begin Source File

SOURCE=.\SeaShellDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellView.cpp
# End Source File
# Begin Source File

SOURCE=.\ShellTreeDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\SeaShellExt\Include\cbformats.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\Dirwalk.h
# End Source File
# Begin Source File

SOURCE=.\FilterDlg.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\HtmlCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEFolderTreeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellComboBox.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellDragDrop.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellListView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellTreeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\IEShellTreeView.h
# End Source File
# Begin Source File

SOURCE=.\LeftView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\PIDL.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\Refresh.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SeaShell.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellDoc.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\ShellContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\ShellDetails.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\ShellPidl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\ShellSettings.h
# End Source File
# Begin Source File

SOURCE=.\ShellTreeDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\TextParse.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIApp.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UICONT.H
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UICoolBar.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UICoolMenu.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\Uictrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIDATA.H
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\uidragdropctrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIDragDropTree.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\uidragimage.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\uidroptarget.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIExplorerFrameWnd.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIFixTB.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIFlatBar.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIFolderRefresh.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIFrameWnd.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIHtmlView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIImageDropTarget.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIListView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIMenuBar.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIMessages.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UIStatusBar.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UISubclass.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UITabSplitterWnd.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UITabView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UITreeCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\UITreeView.h
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Include\WindowPlacement.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\coldtool.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hottoolb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\SeaShell.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeaShell.rc2
# End Source File
# Begin Source File

SOURCE=.\res\SeaShellDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "SeaShellExt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SeaShellExt\cbformats.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\HtmlCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEFolderTreeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellDragDrop.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellListView.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellTreeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\IEShellTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\InPlaceEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\LocaleInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\PIDL.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\ShellContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\ShellDetails.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\ShellPidl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\ShellSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\TextParse.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\TextProgressCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIApp.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UICONT.CPP
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UICoolBar.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UICoolMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\Uictrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIDATA.CPP
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\uidragdropctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIDragDropTree.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\uidragimage.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\uidroptarget.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIExplorerFrameWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIFixTB.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIFlatBar.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIFrameWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIHtmlView.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIImageDropTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIListView.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIMenuBar.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIModulVer.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UIStatusBar.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UISubclass.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UITabSplitterWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UITreeCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\UITreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\SeaShellExt\WindowPlacement.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
