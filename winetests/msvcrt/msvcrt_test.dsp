# Microsoft Developer Studio Project File - Name="msvcrt_test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=msvcrt_test - Win32 Wine Headers
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msvcrt_test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msvcrt_test.mak" CFG="msvcrt_test - Win32 Wine Headers"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msvcrt_test - Win32 MSVC Headers" (based on "Win32 (x86) Console Application")
!MESSAGE "msvcrt_test - Win32 Wine Headers" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
!IF  "$(CFG)" == "msvcrt_test - Win32 MSVC Headers"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Output\Win32_MSVC_Headers"
# PROP BASE Intermediate_Dir "Output\Win32_MSVC_Headers"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Output\Win32_MSVC_Headers"
# PROP Intermediate_Dir "Output\Win32_MSVC_Headers"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WINVER=0x0501" /D "_WIN32_WINNT=0x0501" /D "_WIN32_IE=0x0600" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ..\..\..\Output\\Win32_MSVC_Headers /D "WINVER=0x0501" /D "_WIN32_WINNT=0x0501" /D "_WIN32_IE=0x0600" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MSVCRT_TEST_" /D "__WINE_USE_NATIVE_HEADERS" /D __WINETEST_OUTPUT_DIR=\"Output\\Win32_MSVC_Headers\" /D "__i386__" /D "_X86_" /D inline=__inline /FR /FD /GZ /c
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /i "..\..\..\Output\\Win32_MSVC_Headers" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "msvcrt_test - Win32 Wine Headers"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Output\Win32_Wine_Headers"
# PROP BASE Intermediate_Dir "Output\Win32_Wine_Headers"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Output\Win32_Wine_Headers"
# PROP Intermediate_Dir "Output\Win32_Wine_Headers"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WINVER=0x0501" /D "_WIN32_WINNT=0x0501" /D "_WIN32_IE=0x0600" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ..\..\..\Output\\Win32_Wine_Headers /I ..\..\..\include /D "WINVER=0x0501" /D "_WIN32_WINNT=0x0501" /D "_WIN32_IE=0x0600" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_MSVCRT_TEST_" /D __WINETEST_OUTPUT_DIR=\"Output\\Win32_Wine_Headers\" /D "__i386__" /D "_X86_" /D inline=__inline /FR /FD /GZ /c
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /i "..\..\..\Output\\Win32_Wine_Headers" /i "..\..\..\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "msvcrt_test - Win32 MSVC Headers"
# Name "msvcrt_test - Win32 Wine Headers"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cpp.c
# End Source File
# Begin Source File

SOURCE=.\\
# End Source File
# Begin Source File

SOURCE=.\testlist.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
