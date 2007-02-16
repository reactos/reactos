# Microsoft Developer Studio Project File - Name="winefile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=winefile - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "winefile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "winefile.mak" CFG="winefile - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "winefile - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "winefile - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "winefile - Win32 UNICODE Release" (based on "Win32 (x86) Application")
!MESSAGE "winefile - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "winefile - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "winefileDebug"
# PROP Intermediate_Dir "winefileDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D _WIN32_WINNT=0x0501 /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i ".." /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib version.lib mpr.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "winefile - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Unicode Debug"
# PROP BASE Intermediate_Dir "Unicode Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "winefileUDebug"
# PROP Intermediate_Dir "winefileUDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D _WIN32_WINNT=0x0501 /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /i ".." /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib version.lib mpr.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "winefile - Win32 UNICODE Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "winefile___Win32_UNICODE_Release"
# PROP BASE Intermediate_Dir "winefile___Win32_UNICODE_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "winefileURelease"
# PROP Intermediate_Dir "winefileURelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D _WIN32_WINNT=0x0501 /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i ".." /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib version.lib mpr.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "winefile - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "winefile___Win32_Release"
# PROP BASE Intermediate_Dir "winefile___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "winefileRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D WINE_UNUSED= /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D _WIN32_WINNT=0x0501 /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /i ".." /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib version.lib mpr.lib /nologo /subsystem:windows /machine:I386 /out:"winefile-ansi.exe"

!ENDIF 

# Begin Target

# Name "winefile - Win32 Debug"
# Name "winefile - Win32 Unicode Debug"
# Name "winefile - Win32 UNICODE Release"
# Name "winefile - Win32 Release"
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cs.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\de.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\drivebar.bmp
# End Source File
# Begin Source File

SOURCE=.\en.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\es.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\fr.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\hu.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\images.bmp
# End Source File
# Begin Source File

SOURCE=.\it.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\nl.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\pl.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\pt.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\ru.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\si.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Sv.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\winefile.ico
# End Source File
# Begin Source File

SOURCE=.\winefile.rc
# End Source File
# Begin Source File

SOURCE=.\zh.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\splitpath.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\winefile.c
# End Source File
# Begin Source File

SOURCE=.\winefile.h
# End Source File
# End Target
# End Project
