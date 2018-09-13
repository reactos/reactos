# Microsoft Developer Studio Project File - Name="Mag_Hook" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Mag_Hook - Win32 UNICODE Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mag_Hook.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mag_Hook.mak" CFG="Mag_Hook - Win32 UNICODE Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mag_Hook - Win32 ANSI Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mag_Hook - Win32 ANSI Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mag_Hook - Win32 UNICODE Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Mag_Hook - Win32 UNICODE Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Mag_Hook - Win32 ANSI Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Mag_Hook"
# PROP BASE Intermediate_Dir "Mag_Hook"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Mag_Hook\DebugA"
# PROP Intermediate_Dir "..\ObjFiles\Mag_Hook\DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /X /I "c:\nt\public\sdk\inc" /I "c:\nt\public\sdk\inc\mfc42" /I "c:\nt\public\sdk\inc\crt" /I "c:\progra~1\devstudio\vc\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"../Debug/Mag_Hook.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"../BinFiles/DebugA/Mag_Hook.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Mag_Hook - Win32 ANSI Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Mag_Hoo0"
# PROP BASE Intermediate_Dir "Mag_Hoo0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Mag_Hook\ReleaseA"
# PROP Intermediate_Dir "..\ObjFiles\Mag_Hook\ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "c:\nt\public\sdk\inc" /I "c:\nt\public\sdk\inc\mfc42" /I "c:\nt\public\sdk\inc\crt" /I "c:\progra~1\devstudio\vc\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../Release/Mag_Hook.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../BinFiles/ReleaseA/Mag_Hook.dll"

!ELSEIF  "$(CFG)" == "Mag_Hook - Win32 UNICODE Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Mag_Hoo1"
# PROP BASE Intermediate_Dir "Mag_Hoo1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Mag_Hook\DebugW"
# PROP Intermediate_Dir "..\ObjFiles\Mag_Hook\DebugW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /X /I "c:\nt\public\sdk\inc" /I "c:\nt\public\sdk\inc\mfc42" /I "c:\nt\public\sdk\inc\crt" /I "c:\progra~1\devstudio\vc\mfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"../Debug/Mag_Hook.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"../BinFiles/DebugW/Mag_Hook.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Mag_Hook - Win32 UNICODE Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Mag_Hoo2"
# PROP BASE Intermediate_Dir "Mag_Hoo2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Mag_Hook\ReleaseW"
# PROP Intermediate_Dir "..\ObjFiles\Mag_Hook\ReleaseW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "c:\nt\public\sdk\inc" /I "c:\nt\public\sdk\inc\mfc42" /I "c:\nt\public\sdk\inc\crt" /I "c:\progra~1\devstudio\vc\mfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../Release/Mag_Hook.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../BinFiles/ReleaseW/Mag_Hook.dll"

!ENDIF 

# Begin Target

# Name "Mag_Hook - Win32 ANSI Debug"
# Name "Mag_Hook - Win32 ANSI Release"
# Name "Mag_Hook - Win32 UNICODE Debug"
# Name "Mag_Hook - Win32 UNICODE Release"
# Begin Group "Souce Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Mag_Hook.cpp
# End Source File
# Begin Source File

SOURCE=.\Mag_Hook.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Mag_Hook.h
# End Source File
# End Group
# End Target
# End Project
