# Microsoft Developer Studio Project File - Name="NarrHook" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NarrHook - Win32 UNICODE Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NarrHook.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NarrHook.mak" CFG="NarrHook - Win32 UNICODE Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NarrHook - Win32 ANSI Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NarrHook - Win32 ANSI Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NarrHook - Win32 UNICODE Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NarrHook - Win32 UNICODE Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NarrHook - Win32 ANSI Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ANSI Debug"
# PROP BASE Intermediate_Dir "ANSI Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\NarrHook\DebugA"
# PROP Intermediate_Dir "..\ObjFiles\NarrHook\DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\..\..\public\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
# ADD LINK32 comctl32.lib uuid.lib ..\..\..\..\..\..\public\sdk\lib\i386\oleacc.lib ..\..\..\..\..\..\public\sdk\lib\i386\user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /out:"../BinFiles/DebugA/NarrHook.dll"

!ELSEIF  "$(CFG)" == "NarrHook - Win32 ANSI Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ANSI Release"
# PROP BASE Intermediate_Dir "ANSI Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\NarrHook\ReleaseA"
# PROP Intermediate_Dir "..\ObjFiles\NarrHook\ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 comctl32.lib uuid.lib oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"../BinFiles/ReleaseA/NarrHook.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "NarrHook - Win32 UNICODE Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "UNICODE Debug"
# PROP BASE Intermediate_Dir "UNICODE Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\NarrHook\DebugW"
# PROP Intermediate_Dir "..\ObjFiles\NarrHook\DebugW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
# ADD LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /out:"../BinFiles/DebugW/NarrHook.dll"

!ELSEIF  "$(CFG)" == "NarrHook - Win32 UNICODE Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "UNICODE Release"
# PROP BASE Intermediate_Dir "UNICODE Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\NarrHook\ReleaseW"
# PROP Intermediate_Dir "..\ObjFiles\NarrHook\ReleaseW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib comctl32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"../BinFiles/ReleaseW/NarrHook.dll"
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "NarrHook - Win32 ANSI Debug"
# Name "NarrHook - Win32 ANSI Release"
# Name "NarrHook - Win32 UNICODE Debug"
# Name "NarrHook - Win32 UNICODE Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\getprop.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpThd.cpp
# End Source File
# Begin Source File

SOURCE=.\keys.cpp
# End Source File
# Begin Source File

SOURCE=.\list.cpp
# End Source File
# Begin Source File

SOURCE=.\NarrHook.def
# End Source File
# Begin Source File

SOURCE=.\NarrHook.rc

!IF  "$(CFG)" == "NarrHook - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "NarrHook - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "NarrHook - Win32 UNICODE Debug"

!ELSEIF  "$(CFG)" == "NarrHook - Win32 UNICODE Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\other.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\GETPROP.H
# End Source File
# Begin Source File

SOURCE=.\HelpThd.h
# End Source File
# Begin Source File

SOURCE=.\keys.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=..\Narrator\Narrator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
