# Microsoft Developer Studio Project File - Name="rbuild" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=rbuild - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rbuild.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rbuild.mak" CFG="rbuild - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rbuild - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rbuild - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rbuild - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "rbuild - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".." /I "../../lib/inflib" /I "../../include/reactos" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "INFLIB_HOST" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "rbuild - Win32 Release"
# Name "rbuild - Win32 Debug"
# Begin Group "backends"

# PROP Default_Filter ""
# Begin Group "devcpp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend\devcpp\devcpp.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\devcpp\devcpp.h
# End Source File
# End Group
# Begin Group "msvc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend\msvc\genguid.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\msvc\msvc.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\msvc\msvc.h
# End Source File
# Begin Source File

SOURCE=.\backend\msvc\msvcmaker.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\msvc\vcprojmaker.cpp
# End Source File
# End Group
# Begin Group "mingw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend\mingw\mingw.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\mingw\mingw.h
# End Source File
# Begin Source File

SOURCE=.\backend\mingw\modulehandler.cpp
# End Source File
# Begin Source File

SOURCE=.\backend\mingw\modulehandler.h
# End Source File
# Begin Source File

SOURCE=.\backend\mingw\pch_detection.h
# End Source File
# Begin Source File

SOURCE=.\backend\mingw\proxymakefile.cpp
# End Source File
# End Group
# Begin Group "backend_sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend\backend.cpp
# End Source File
# End Group
# Begin Group "backend_headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\backend\backend.h
# End Source File
# End Group
# End Group
# Begin Group "rbuild_sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\automaticdependency.cpp
# End Source File
# Begin Source File

SOURCE=.\bootstrap.cpp
# End Source File
# Begin Source File

SOURCE=.\cdfile.cpp
# End Source File
# Begin Source File

SOURCE=.\compilationunit.cpp
# End Source File
# Begin Source File

SOURCE=.\compilationunitsupportcode.cpp
# End Source File
# Begin Source File

SOURCE=.\compilerflag.cpp
# End Source File
# Begin Source File

SOURCE=.\configuration.cpp
# End Source File
# Begin Source File

SOURCE=.\define.cpp
# End Source File
# Begin Source File

SOURCE=.\directory.cpp
# End Source File
# Begin Source File

SOURCE=.\exception.cpp
# End Source File
# Begin Source File

SOURCE=.\filesupportcode.cpp
# End Source File
# Begin Source File

SOURCE=.\global.cpp
# End Source File
# Begin Source File

SOURCE=.\include.cpp
# End Source File
# Begin Source File

SOURCE=.\installfile.cpp
# End Source File
# Begin Source File

SOURCE=.\linkerflag.cpp
# End Source File
# Begin Source File

SOURCE=.\linkerscript.cpp
# End Source File
# Begin Source File

SOURCE=.\module.cpp
# End Source File
# Begin Source File

SOURCE=.\project.cpp
# End Source File
# Begin Source File

SOURCE=.\rbuild.cpp
# End Source File
# Begin Source File

SOURCE=.\stubbedcomponent.cpp
# End Source File
# Begin Source File

SOURCE=.\syssetupgenerator.cpp
# End Source File
# Begin Source File

SOURCE=.\testsupportcode.cpp
# End Source File
# Begin Source File

SOURCE=.\wineresource.cpp
# End Source File
# End Group
# Begin Group "rbuild_headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\exception.h
# End Source File
# Begin Source File

SOURCE=.\pch.h
# End Source File
# Begin Source File

SOURCE=.\rbuild.h
# End Source File
# Begin Source File

SOURCE=.\test.h
# End Source File
# End Group
# Begin Group "inflib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\lib\inflib\builddep.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infcommon.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infcore.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infget.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infhost.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infhostgen.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infhostget.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infhostglue.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infhostput.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\inflib.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infpriv.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\inflib\infput.c
# End Source File
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ssprintf.cpp
# End Source File
# Begin Source File

SOURCE=..\ssprintf.h
# End Source File
# Begin Source File

SOURCE=..\xml.cpp
# End Source File
# Begin Source File

SOURCE=..\xml.h
# End Source File
# End Group
# End Target
# End Project
