# Microsoft Developer Studio Project File - Name="wine_explore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=wine_explore - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wine_explore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wine_explore.mak" CFG="wine_explore - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wine_explore - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "wine_explore - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "wine_explore - Win32 Debug Release" (based on "Win32 (x86) Console Application")
!MESSAGE "wine_explore - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "wine_explore - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.cmd
RSC=rc.exe

!IF  "$(CFG)" == "wine_explore - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_ROS_" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i ".." /d "NDEBUG" /d "_WINEFILE_"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.cmd
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_ROS_" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i ".." /d "_DEBUG" /d "_WINEFILE_"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.cmd
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wineExplorer___Win32_Debug_Release"
# PROP BASE Intermediate_Dir "wineExplorer___Win32_Debug_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DRelease"
# PROP Intermediate_Dir "DRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_ROS_" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_ROS_" /FR /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i ".." /d "NDEBUG" /d "_WINEFILE_"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.cmd
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wineExplorer___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "wineExplorer___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "URelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i ".." /d "NDEBUG" /d "_WINEFILE_"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.cmd
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wineExplorer___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "wineExplorer___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UDebug"
# PROP Intermediate_Dir "UDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "UNICODE" /D "_ROS_" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i ".." /d "_DEBUG" /d "_WINEFILE_"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.cmd
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "wine_explore - Win32 Release"
# Name "wine_explore - Win32 Debug"
# Name "wine_explore - Win32 Debug Release"
# Name "wine_explore - Win32 Unicode Release"
# Name "wine_explore - Win32 Unicode Debug"
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\de.rc

!IF  "$(CFG)" == "wine_explore - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\res\drivebar.bmp
# End Source File
# Begin Source File

SOURCE=.\en.rc

!IF  "$(CFG)" == "wine_explore - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\res\images.bmp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc

!IF  "$(CFG)" == "wine_explore - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Debug Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "wine_explore - Win32 Unicode Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=..\res\winefile.ico
# End Source File
# Begin Source File

SOURCE=.\winefile.rc
# End Source File
# End Group
# Begin Group "plugins"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\taskbar\ex_bar.c
# End Source File
# Begin Source File

SOURCE=..\taskbar\ex_bar.h
# End Source File
# Begin Source File

SOURCE=..\taskbar\ex_clock.c
# End Source File
# Begin Source File

SOURCE=..\taskbar\ex_menu.c
# End Source File
# Begin Source File

SOURCE=..\taskbar\ex_shutdwn.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\desktop.c
# End Source File
# Begin Source File

SOURCE=..\explorer.h
# End Source File
# Begin Source File

SOURCE=.\externals.h
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=..\shell\startup.c
# End Source File
# Begin Source File

SOURCE=..\utility\utility.h
# End Source File
# Begin Source File

SOURCE=.\winefile.c
# End Source File
# Begin Source File

SOURCE=.\winefile.h
# End Source File
# End Target
# End Project
