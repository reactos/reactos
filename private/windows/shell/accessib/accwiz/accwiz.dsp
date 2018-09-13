# Microsoft Developer Studio Project File - Name="AccWiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=AccWiz - Win32 UNICODE NT5 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "accwiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "accwiz.mak" CFG="AccWiz - Win32 UNICODE NT5 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AccWiz - Win32 ANSI Win97 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 ANSI Win97 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 UNICODE NT5 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 UNICODE NT5 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AccWiz__"
# PROP BASE Intermediate_Dir "AccWiz__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug97"
# PROP Intermediate_Dir "Debug97"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\windows\inc" /I "..\..\..\..\public\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /FR /Yu"pch.hxx" /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /GX /ZI /Od /I "\nt\public\sdk\inc" /I "\nt\private\windows\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /FR /Yu"pch.hxx" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32p.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib \nt\public\sdk\lib\i386\user32p.lib \nt\public\sdk\lib\i386\shlwapi.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AccWiz_0"
# PROP BASE Intermediate_Dir "AccWiz_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release97"
# PROP Intermediate_Dir "Release97"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /Yu"pch.hxx" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "\nt\public\sdk\inc" /I "Z:\nt\Private\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /Fr /Yu"pch.hxx" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib user32p.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib user32p.lib shlwapi.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none /incremental:yes

!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AccWiz_1"
# PROP BASE Intermediate_Dir "AccWiz_1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugNT"
# PROP Intermediate_Dir "DebugNT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\windows\inc" /I "..\..\..\..\public\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /FR /Yu"pch.hxx" /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /GX /ZI /Od /I "\nt\public\sdk\inc" /I "\nt\private\windows\inc" /I "\nt\private\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /D "UNICODE" /D "_UNICODE" /FR /YX"pch.hxx" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(_NTDRIVE)$(_NTROOT)\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32p.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib shlwapi.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /libpath:"\nt\public\sdk\lib\i386" /libpath:"\nt\private\iedev\lib\i386"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AccWiz_2"
# PROP BASE Intermediate_Dir "AccWiz_2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseNT"
# PROP Intermediate_Dir "ReleaseNT"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /Yu"pch.hxx" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "\nt\public\sdk\inc" /I "\nt\private\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "STRICT" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib user32p.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib user32p.lib shlwapi.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none /incremental:yes /debug

!ENDIF 

# Begin Target

# Name "AccWiz - Win32 ANSI Win97 Debug"
# Name "AccWiz - Win32 ANSI Win97 Release"
# Name "AccWiz - Win32 UNICODE NT5 Debug"
# Name "AccWiz - Win32 UNICODE NT5 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AccWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\AccWiz.rc
# End Source File
# Begin Source File

SOURCE=.\CurSchme.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgFonts.cpp
# End Source File
# Begin Source File

SOURCE=.\lookdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\lookprev.cpp
# End Source File
# Begin Source File

SOURCE=.\pgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\pgfinish.cpp
# End Source File
# Begin Source File

SOURCE=.\pgGenric.cpp
# End Source File
# Begin Source File

SOURCE=.\pgLokPrv.cpp
# End Source File
# Begin Source File

SOURCE=.\pgMseBut.cpp
# End Source File
# Begin Source File

SOURCE=.\pgMseCur.cpp
# End Source File
# Begin Source File

SOURCE=.\pgnwelcome.cpp
# End Source File
# Begin Source File

SOURCE=.\pgSveDef.cpp
# End Source File
# Begin Source File

SOURCE=.\pgSveFil.cpp
# End Source File
# Begin Source File

SOURCE=.\pgTmeOut.cpp
# End Source File
# Begin Source File

SOURCE=.\pgWelco2.cpp
# End Source File
# Begin Source File

SOURCE=.\pgWelcom.cpp
# End Source File
# Begin Source File

SOURCE=.\pgWizOpt.cpp
# End Source File
# Begin Source File

SOURCE=.\precomp.cpp
# End Source File
# Begin Source File

SOURCE=.\Schemes.cpp
# End Source File
# Begin Source File

SOURCE=.\Select.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\accwiz.h
# End Source File
# Begin Source File

SOURCE=.\CurSchme.h
# End Source File
# Begin Source File

SOURCE=.\DESK.H
# End Source File
# Begin Source File

SOURCE=.\DESKID.H
# End Source File
# Begin Source File

SOURCE=.\DlgFonts.h
# End Source File
# Begin Source File

SOURCE=.\LOOK.H
# End Source File
# Begin Source File

SOURCE=.\LookPrev.h
# End Source File
# Begin Source File

SOURCE=.\pch.hxx
# End Source File
# Begin Source File

SOURCE=.\pgbase.h
# End Source File
# Begin Source File

SOURCE=.\pgExtras.h
# End Source File
# Begin Source File

SOURCE=.\pgfinish.h
# End Source File
# Begin Source File

SOURCE=.\pgGenric.h
# End Source File
# Begin Source File

SOURCE=.\pgLokPrv.h
# End Source File
# Begin Source File

SOURCE=.\pgMseBut.h
# End Source File
# Begin Source File

SOURCE=.\pgMseCur.h
# End Source File
# Begin Source File

SOURCE=.\pgSveDef.h
# End Source File
# Begin Source File

SOURCE=.\pgSveFil.h
# End Source File
# Begin Source File

SOURCE=.\pgTmeOut.h
# End Source File
# Begin Source File

SOURCE=.\pgWelco2.h
# End Source File
# Begin Source File

SOURCE=.\pgWelcom.h
# End Source File
# Begin Source File

SOURCE=.\pgWizOpt.h
# End Source File
# Begin Source File

SOURCE=.\Schemes.h
# End Source File
# Begin Source File

SOURCE=.\Select.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Access.ico
# End Source File
# Begin Source File

SOURCE=.\cursorLB.ico
# End Source File
# Begin Source File

SOURCE=.\cursorLI.ico
# End Source File
# Begin Source File

SOURCE=.\cursorLW.ico
# End Source File
# Begin Source File

SOURCE=.\cursorMB.ico
# End Source File
# Begin Source File

SOURCE=.\cursorMI.ico
# End Source File
# Begin Source File

SOURCE=.\cursorMW.ico
# End Source File
# Begin Source File

SOURCE=.\cursorSB.ico
# End Source File
# Begin Source File

SOURCE=.\cursorSI.ico
# End Source File
# Begin Source File

SOURCE=.\cursorSW.ico
# End Source File
# Begin Source File

SOURCE=.\MOUSE.BMP
# End Source File
# Begin Source File

SOURCE=.\mouse2.bmp
# End Source File
# Begin Source File

SOURCE=.\SampleL.bmp
# End Source File
# Begin Source File

SOURCE=.\SampleN.bmp
# End Source File
# Begin Source File

SOURCE=.\SampleXL.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\gdi32.lib
# End Source File
# End Target
# End Project
