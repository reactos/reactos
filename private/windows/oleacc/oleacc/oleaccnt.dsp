# Microsoft Developer Studio Project File - Name="oleaccnt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=oleaccnt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "oleaccnt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oleaccnt.mak" CFG="oleaccnt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "oleaccnt - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "oleaccnt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "oleaccnt - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /Oi /Gf /Gy /X /I "f:\nt\private\sdk\inc" /I "i386\\" /I "." /I "f:\nt\private\windows\inc" /I "f:\nt\public\oak\inc" /I "f:\nt\public\sdk\inc" /I "f:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32_LEAN_AND_MEAN" /D "IS_32" /D "M4" /D "_X86_" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D "_WIN32" /D "_M_IX86" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /fo"Release/oleacc.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /pdb:none /map /machine:I386 /out:"Release/oleacc.dll"

!ELSEIF  "$(CFG)" == "oleaccnt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "oleaccnt"
# PROP BASE Intermediate_Dir "oleaccnt"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /Gz /w /W0 /Z7 /Od /Gf /Gy /X /I "i386\\" /I "." /I "f:\nt\private\windows\inc" /I "f:\nt\public\oak\inc" /I "f:\nt\public\sdk\inc" /I "f:\nt\public\sdk\inc\crt" /D "_M_IX86" /D "_WIN32" /D _WIN32_WINNT=0x0500 /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /D "_STDCALL_SUPPORTED" /FR /FD /c
# SUBTRACT CPP /nologo /u
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo"Debug/oleacc.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /pdb:none /map /debug /debugtype:both /machine:I386 /out:"Debug/oleacc.dll"

!ENDIF 

# Begin Target

# Name "oleaccnt - Win32 Release"
# Name "oleaccnt - Win32 Debug"
# Begin Source File

SOURCE=.\alttab.cpp
# End Source File
# Begin Source File

SOURCE=.\alttab.h
# End Source File
# Begin Source File

SOURCE=.\animated.cpp
# End Source File
# Begin Source File

SOURCE=.\animated.h
# End Source File
# Begin Source File

SOURCE=.\api.cpp
# End Source File
# Begin Source File

SOURCE=.\button.cpp
# End Source File
# Begin Source File

SOURCE=.\button.h
# End Source File
# Begin Source File

SOURCE=.\calendar.cpp

!IF  "$(CFG)" == "oleaccnt - Win32 Release"

!ELSEIF  "$(CFG)" == "oleaccnt - Win32 Debug"

# ADD CPP /nologo

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\calendar.h
# End Source File
# Begin Source File

SOURCE=.\caret.cpp
# End Source File
# Begin Source File

SOURCE=.\caret.h
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\combo.cpp
# End Source File
# Begin Source File

SOURCE=.\combo.h
# End Source File
# Begin Source File

SOURCE=.\cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\cursor.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\default.cpp
# End Source File
# Begin Source File

SOURCE=.\default.h
# End Source File
# Begin Source File

SOURCE=.\desktop.cpp
# End Source File
# Begin Source File

SOURCE=.\desktop.h
# End Source File
# Begin Source File

SOURCE=.\dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog.h
# End Source File
# Begin Source File

SOURCE=.\edit.cpp
# End Source File
# Begin Source File

SOURCE=.\edit.h
# End Source File
# Begin Source File

SOURCE=.\guids.c
# End Source File
# Begin Source File

SOURCE=.\header.cpp
# End Source File
# Begin Source File

SOURCE=.\header.h
# End Source File
# Begin Source File

SOURCE=.\hotkey.cpp
# End Source File
# Begin Source File

SOURCE=.\hotkey.h
# End Source File
# Begin Source File

SOURCE=.\html.cpp
# End Source File
# Begin Source File

SOURCE=.\html.h
# End Source File
# Begin Source File

SOURCE=.\listbox.cpp
# End Source File
# Begin Source File

SOURCE=.\listbox.h
# End Source File
# Begin Source File

SOURCE=.\listview.cpp
# End Source File
# Begin Source File

SOURCE=.\listview.h
# End Source File
# Begin Source File

SOURCE=.\mdicli.cpp
# End Source File
# Begin Source File

SOURCE=.\mdicli.h
# End Source File
# Begin Source File

SOURCE=.\memchk.h
# End Source File
# Begin Source File

SOURCE=.\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\oleacc.cpp
# End Source File
# Begin Source File

SOURCE=.\oleacc_p.h
# End Source File
# Begin Source File

SOURCE=.\outline.cpp
# End Source File
# Begin Source File

SOURCE=.\outline.h
# End Source File
# Begin Source File

SOURCE=.\progress.cpp
# End Source File
# Begin Source File

SOURCE=.\progress.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\scroll.cpp
# End Source File
# Begin Source File

SOURCE=.\scroll.h
# End Source File
# Begin Source File

SOURCE=.\sdm.cpp
# End Source File
# Begin Source File

SOURCE=.\sdm.h
# End Source File
# Begin Source File

SOURCE=.\sdm95.h
# End Source File
# Begin Source File

SOURCE=.\slider.cpp
# End Source File
# Begin Source File

SOURCE=.\slider.h
# End Source File
# Begin Source File

SOURCE=.\statbar.cpp
# End Source File
# Begin Source File

SOURCE=.\statbar.h
# End Source File
# Begin Source File

SOURCE=.\static.cpp
# End Source File
# Begin Source File

SOURCE=.\static.h
# End Source File
# Begin Source File

SOURCE=.\strtable.h
# End Source File
# Begin Source File

SOURCE=.\tabctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\tabctrl.h
# End Source File
# Begin Source File

SOURCE=.\titlebar.cpp
# End Source File
# Begin Source File

SOURCE=.\titlebar.h
# End Source File
# Begin Source File

SOURCE=.\toolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\toolbar.h
# End Source File
# Begin Source File

SOURCE=.\tooltips.cpp
# End Source File
# Begin Source File

SOURCE=.\tooltips.h
# End Source File
# Begin Source File

SOURCE=.\updown.cpp
# End Source File
# Begin Source File

SOURCE=.\updown.h
# End Source File
# Begin Source File

SOURCE=.\verdefs.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\w95trace.h
# End Source File
# Begin Source File

SOURCE=.\window.cpp
# End Source File
# Begin Source File

SOURCE=.\window.h
# End Source File
# End Target
# End Project
