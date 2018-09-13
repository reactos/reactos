# Microsoft Developer Studio Project File - Name="Magnify" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Magnify - Win32 UNICODE Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Magnify.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Magnify.mak" CFG="Magnify - Win32 UNICODE Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Magnify - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE "Magnify - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "Magnify - Win32 UNICODE Debug" (based on "Win32 (x86) Application")
!MESSAGE "Magnify - Win32 UNICODE Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Magnify - Win32 ANSI Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Magnify_"
# PROP BASE Intermediate_Dir "Magnify_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Magnify\DebugA"
# PROP Intermediate_Dir "..\ObjFiles\Magnify\DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"../Debug/Magnify.exe" /pdbtype:sept
# ADD LINK32 ddraw.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"../BinFiles/DebugA/Magnify.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Magnify - Win32 ANSI Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Magnify0"
# PROP BASE Intermediate_Dir "Magnify0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Magnify\ReleaseA"
# PROP Intermediate_Dir "..\ObjFiles\Magnify\ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib /nologo /subsystem:windows /machine:I386 /out:"../Release/Magnify.exe"
# ADD LINK32 ddraw.lib /nologo /subsystem:windows /machine:I386 /out:"../BinFiles/ReleaseA/Magnify.exe"

!ELSEIF  "$(CFG)" == "Magnify - Win32 UNICODE Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Magnify1"
# PROP BASE Intermediate_Dir "Magnify1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Magnify\DebugW"
# PROP Intermediate_Dir "..\ObjFiles\Magnify\DebugW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"../Debug/Magnify.exe" /pdbtype:sept
# ADD LINK32 ddraw.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /debug /machine:I386 /out:"../BinFiles/DebugW/Magnify.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Magnify - Win32 UNICODE Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Magnify2"
# PROP BASE Intermediate_Dir "Magnify2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Magnify\ReleaseW"
# PROP Intermediate_Dir "..\ObjFiles\Magnify\ReleaseW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ddraw.lib /nologo /subsystem:windows /machine:I386 /out:"../Release/Magnify.exe"
# ADD LINK32 ddraw.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386 /out:"../BinFiles/ReleaseW/Magnify.exe"

!ENDIF 

# Begin Target

# Name "Magnify - Win32 ANSI Debug"
# Name "Magnify - Win32 ANSI Release"
# Name "Magnify - Win32 UNICODE Debug"
# Name "Magnify - Win32 UNICODE Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AppBar.cpp
# End Source File
# Begin Source File

SOURCE=.\FastDib.cpp
# End Source File
# Begin Source File

SOURCE=.\MagBar.cpp
# End Source File
# Begin Source File

SOURCE=.\MagDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Magnify.cpp
# End Source File
# Begin Source File

SOURCE=.\Magnify.rc

!IF  "$(CFG)" == "Magnify - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "Magnify - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "Magnify - Win32 UNICODE Debug"

!ELSEIF  "$(CFG)" == "Magnify - Win32 UNICODE Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ZoomRect.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AppBar.h
# End Source File
# Begin Source File

SOURCE=.\FastDib.h
# End Source File
# Begin Source File

SOURCE=.\MagBar.h
# End Source File
# Begin Source File

SOURCE=.\MagDlg.h
# End Source File
# Begin Source File

SOURCE=.\Magnify.h
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ZoomRect.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\arrow.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\hand.cur
# End Source File
# Begin Source File

SOURCE=.\res\Magnify.ico
# End Source File
# Begin Source File

SOURCE=.\res\Magnify.rc2
# End Source File
# End Group
# End Target
# End Project
