# Microsoft Developer Studio Project File - Name="rosshell" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=rosshell - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rosshell.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rosshell.mak" CFG="rosshell - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rosshell - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rosshell - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "rosshell - Win32 Debug Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rosshell - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rosshell - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rosshell - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /D "NDEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /D "ROSSHELL" /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 shell32.lib comctl32.lib gdi32.lib user32.lib advapi32.lib ole32.lib delayimp.lib /nologo /subsystem:windows /machine:I386 /libpath:"Release" /delayload:oleaut32.dll /delayload:wsock32.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rosshell - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /D "ROSSHELL" /FR /Yu"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib shell32.lib comctl32.lib gdi32.lib user32.lib advapi32.lib ole32.lib delayimp.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"Debug" /delayload:oleaut32.dll /delayload:wsock32.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rosshell - Win32 Debug Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DRelease"
# PROP BASE Intermediate_Dir "DRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DRelease"
# PROP Intermediate_Dir "DRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_ROS_" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /D "ROSSHELL" /FR /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 shell32.lib comctl32.lib gdi32.lib user32.lib advapi32.lib ole32.lib delayimp.lib /nologo /subsystem:windows /debug /machine:I386 /delayload:oleaut32.dll /delayload:wsock32.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rosshell - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "URelease"
# PROP BASE Intermediate_Dir "URelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "URelease"
# PROP Intermediate_Dir "URelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "NDEBUG" /D "UNICODE" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /D "ROSSHELL" /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 shell32.lib comctl32.lib gdi32.lib user32.lib advapi32.lib ole32.lib delayimp.lib /nologo /subsystem:windows /machine:I386 /libpath:"Release" /delayload:oleaut32.dll /delayload:wsock32.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rosshell - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "UDebug"
# PROP BASE Intermediate_Dir "UDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UDebug"
# PROP Intermediate_Dir "UDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /D "ROSSHELL" /FR /Yu"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shell32.lib comctl32.lib gdi32.lib user32.lib advapi32.lib ole32.lib delayimp.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"Debug" /delayload:oleaut32.dll /delayload:wsock32.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "rosshell - Win32 Release"
# Name "rosshell - Win32 Debug"
# Name "rosshell - Win32 Debug Release"
# Name "rosshell - Win32 Unicode Release"
# Name "rosshell - Win32 Unicode Debug"
# Begin Group "utility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\utility\dragdropimpl.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\dragdropimpl.h
# End Source File
# Begin Source File

SOURCE=.\utility\shellbrowserimpl.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\shellbrowserimpl.h
# End Source File
# Begin Source File

SOURCE=.\utility\shellclasses.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\shellclasses.h
# End Source File
# Begin Source File

SOURCE=.\utility\treedroptarget.h
# End Source File
# Begin Source File

SOURCE=.\utility\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\utility.h
# End Source File
# Begin Source File

SOURCE=.\utility\window.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\window.h
# End Source File
# Begin Source File

SOURCE=.\utility\xmlstorage.cpp
# End Source File
# Begin Source File

SOURCE=.\utility\xmlstorage.h
# End Source File
# End Group
# Begin Group "resources"

# PROP Default_Filter "bmp,ico"
# Begin Source File

SOURCE=.\res\action.ico
# End Source File
# Begin Source File

SOURCE=.\res\appicon.ico
# End Source File
# Begin Source File

SOURCE=.\res\apps.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrow.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrow_dwn.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrow_up.ico
# End Source File
# Begin Source File

SOURCE=.\res\arrowsel.ico
# End Source File
# Begin Source File

SOURCE=.\res\computer.ico
# End Source File
# Begin Source File

SOURCE=.\res\config.ico
# End Source File
# Begin Source File

SOURCE=.\res\documents.ico
# End Source File
# Begin Source File

SOURCE=.\res\dot.ico
# End Source File
# Begin Source File

SOURCE=.\res\dot_red.ico
# End Source File
# Begin Source File

SOURCE=.\res\dot_trans.ico
# End Source File
# Begin Source File

SOURCE=.\res\drivebar.bmp
# End Source File
# Begin Source File

SOURCE=".\explorer-jp.rc"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\res\explorer.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\explorer_intres.rc
# End Source File
# Begin Source File

SOURCE=.\res\favorites.ico
# End Source File
# Begin Source File

SOURCE=.\res\floating.ico
# End Source File
# Begin Source File

SOURCE=.\res\folder.ico
# End Source File
# Begin Source File

SOURCE=.\res\icoali10.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig5.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig6.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig7.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig8.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icoalig9.bmp
# End Source File
# Begin Source File

SOURCE=.\res\images.bmp
# End Source File
# Begin Source File

SOURCE=.\res\info.ico
# End Source File
# Begin Source File

SOURCE=.\res\logoff.ico
# End Source File
# Begin Source File

SOURCE=.\res\logov.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logov16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logov256.bmp
# End Source File
# Begin Source File

SOURCE=.\res\network.ico
# End Source File
# Begin Source File

SOURCE=.\res\notify_l.ico
# End Source File
# Begin Source File

SOURCE=.\res\notify_r.ico
# End Source File
# Begin Source File

SOURCE=.\res\printer.ico
# End Source File
# Begin Source File

SOURCE=.\res\reactos.ico
# End Source File
# Begin Source File

SOURCE=".\res\ros-big.ico"
# End Source File
# Begin Source File

SOURCE=".\res\search-doc.ico"
# End Source File
# Begin Source File

SOURCE=.\res\search.ico
# End Source File
# Begin Source File

SOURCE=.\res\speaker.ico
# End Source File
# Begin Source File

SOURCE=.\res\startmenu.ico
# End Source File
# End Group
# Begin Group "taskbar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\taskbar\desktopbar.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\desktopbar.h
# End Source File
# Begin Source File

SOURCE=.\taskbar\favorites.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\favorites.h
# End Source File
# Begin Source File

SOURCE=.\notifyhook\notifyhook.h
# End Source File
# Begin Source File

SOURCE=.\taskbar\quicklaunch.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\quicklaunch.h
# End Source File
# Begin Source File

SOURCE=.\taskbar\startmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\startmenu.h
# End Source File
# Begin Source File

SOURCE=.\taskbar\taskbar.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\taskbar.h
# End Source File
# Begin Source File

SOURCE=.\taskbar\traynotify.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar\traynotify.h
# End Source File
# End Group
# Begin Group "desktop"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\desktop\desktop.cpp
# End Source File
# Begin Source File

SOURCE=.\desktop\desktop.h
# End Source File
# End Group
# Begin Group "shell"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\shell\entries.cpp
# End Source File
# Begin Source File

SOURCE=.\shell\entries.h
# End Source File
# Begin Source File

SOURCE=.\shell\filechild.h
# End Source File
# Begin Source File

SOURCE=.\shell\pane.cpp
# End Source File
# Begin Source File

SOURCE=.\shell\pane.h
# End Source File
# Begin Source File

SOURCE=.\shell\shellbrowser.h
# End Source File
# Begin Source File

SOURCE=.\shell\shellfs.cpp
# End Source File
# Begin Source File

SOURCE=.\shell\shellfs.h
# End Source File
# Begin Source File

SOURCE=.\shell\winfs.cpp
# End Source File
# Begin Source File

SOURCE=.\shell\winfs.h
# End Source File
# End Group
# Begin Group "dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dialogs\searchprogram.cpp
# End Source File
# Begin Source File

SOURCE=.\dialogs\searchprogram.h
# End Source File
# Begin Source File

SOURCE=.\dialogs\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\dialogs\settings.h
# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\explorer.cpp
# End Source File
# Begin Source File

SOURCE=.\explorer.h
# End Source File
# Begin Source File

SOURCE=.\externals.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=".\i386-stub-win32.c"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# End Group
# Begin Group "services"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\services\shellservices.cpp
# End Source File
# Begin Source File

SOURCE=.\services\shellservices.h
# End Source File
# Begin Source File

SOURCE=.\services\startup.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Target
# End Project
