# Microsoft Developer Studio Project File - Name="Narrator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Narrator - Win32 UNICODE Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Narrator.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Narrator.mak" CFG="Narrator - Win32 UNICODE Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Narrator - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE "Narrator - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "Narrator - Win32 UNICODE Debug" (based on "Win32 (x86) Application")
!MESSAGE "Narrator - Win32 UNICODE Release" (based on\
 "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Narrator - Win32 ANSI Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Narrator___Wi"
# PROP BASE Intermediate_Dir "Narrator___Wi"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Narrator\DebugA"
# PROP Intermediate_Dir "..\ObjFiles\Narrator\DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /Gy /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /Gy /I "..\..\..\..\..\..\public\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 comdlg32.lib advapi32.lib shell32.lib winmm.lib ..\..\..\..\..\..\public\sdk\lib\i386\oleacc.lib ..\..\..\..\..\..\public\sdk\lib\i386\user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"../BinFiles/DebugA/Narrator.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Narrator - Win32 ANSI Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Narrator___W0"
# PROP BASE Intermediate_Dir "Narrator___W0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Narrator\ReleaseA"
# PROP Intermediate_Dir "..\ObjFiles\Narrator\ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 comdlg32.lib advapi32.lib shell32.lib winmm.lib oleacc.lib user32.lib kernel32.lib gdi32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /map /machine:I386 /out:"../BinFiles/ReleaseA/Narrator.exe"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Narrator - Win32 UNICODE Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Narrator___W1"
# PROP BASE Intermediate_Dir "Narrator___W1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\ObjFiles\Narrator\DebugW"
# PROP Intermediate_Dir "..\ObjFiles\Narrator\DebugW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /Gy /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /Gy /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib winmm.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"../BinFiles/DebugW/Narrator.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Narrator - Win32 UNICODE Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Narrator___W2"
# PROP BASE Intermediate_Dir "Narrator___W2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ObjFiles\Narrator\ReleaseW"
# PROP Intermediate_Dir "..\ObjFiles\Narrator\ReleaseW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 oleacc.lib user32.lib kernel32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib winmm.lib /nologo /subsystem:windows /map /machine:I386 /out:"../BinFiles/ReleaseW/Narrator.exe"
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "Narrator - Win32 ANSI Debug"
# Name "Narrator - Win32 ANSI Release"
# Name "Narrator - Win32 UNICODE Debug"
# Name "Narrator - Win32 UNICODE Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Narrator.cpp
# End Source File
# Begin Source File

SOURCE=.\Narrator.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Narrator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\msn.ico
# End Source File
# Begin Source File

SOURCE=.\Narrator.ico
# End Source File
# End Group
# End Target
# End Project
