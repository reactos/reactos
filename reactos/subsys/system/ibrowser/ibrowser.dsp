# Microsoft Developer Studio Project File - Name="ibrowser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=IBROWSER - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ibrowser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ibrowser.mak" CFG="IBROWSER - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ibrowser - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ibrowser - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ibrowser - Win32 Debug Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ibrowser - Win32 Unicode Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ibrowser - Win32 Unicode Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ibrowser - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /D "NDEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 gdi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /machine:I386 /libpath:"Release"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ibrowser - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"Debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ibrowser - Win32 Debug Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 gdi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ibrowser - Win32 Unicode Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "NDEBUG" /D "UNICODE" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /Yu"precomp.h" /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 gdi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /machine:I386 /libpath:"Release"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ibrowser - Win32 Unicode Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "WIN32" /D _WIN32_IE=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib shell32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gdi32.lib comctl32.lib shell32.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"Debug"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ibrowser - Win32 Release"
# Name "ibrowser - Win32 Debug"
# Name "ibrowser - Win32 Debug Release"
# Name "ibrowser - Win32 Unicode Release"
# Name "ibrowser - Win32 Unicode Debug"
# Begin Group "utility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\utility\comutil.h
# End Source File
# Begin Source File

SOURCE=.\utility\expat.h
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

SOURCE=.\res\dot.ico
# End Source File
# Begin Source File

SOURCE=.\res\dot_red.ico
# End Source File
# Begin Source File

SOURCE=.\res\dot_trans.ico
# End Source File
# Begin Source File

SOURCE=.\res\favorites.ico
# End Source File
# Begin Source File

SOURCE=.\res\ibrowser.ico
# End Source File
# Begin Source File

SOURCE=.\ibrowser_intres.h
# End Source File
# Begin Source File

SOURCE=.\ibrowser_intres.rc
# End Source File
# Begin Source File

SOURCE=.\res\iexplore.ico
# End Source File
# Begin Source File

SOURCE=.\res\network.ico
# End Source File
# Begin Source File

SOURCE=.\res\reactos.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# End Group
# Begin Group "main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\favorites.cpp
# End Source File
# Begin Source File

SOURCE=.\favorites.h
# End Source File
# Begin Source File

SOURCE=.\ibrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\ibrowser.h
# End Source File
# Begin Source File

SOURCE=.\mainframe.cpp
# End Source File
# Begin Source File

SOURCE=.\mainframe.h
# End Source File
# Begin Source File

SOURCE=.\precomp.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\webchild.cpp
# End Source File
# Begin Source File

SOURCE=.\webchild.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
