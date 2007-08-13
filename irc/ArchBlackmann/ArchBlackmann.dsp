# Microsoft Developer Studio Project File - Name="ArchBlackmann" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ArchBlackmann - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ArchBlackmann.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ArchBlackmann.mak" CFG="ArchBlackmann - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ArchBlackmann - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ArchBlackmann - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ArchBlackmann - Win32 Release"

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

!ELSEIF  "$(CFG)" == "ArchBlackmann - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
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

# Name "ArchBlackmann - Win32 Release"
# Name "ArchBlackmann - Win32 Debug"
# Begin Source File

SOURCE=.\ArchBlackmann.cpp
# End Source File
# Begin Source File

SOURCE=.\auto_ptr.h
# End Source File
# Begin Source File

SOURCE=.\auto_vector.h
# End Source File
# Begin Source File

SOURCE=.\base64.cpp
# End Source File
# Begin Source File

SOURCE=.\base64.h
# End Source File
# Begin Source File

SOURCE=.\chomp.cpp
# End Source File
# Begin Source File

SOURCE=.\chomp.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\cram_md5.cpp
# End Source File
# Begin Source File

SOURCE=.\cram_md5.h
# End Source File
# Begin Source File

SOURCE=.\File.cpp
# End Source File
# Begin Source File

SOURCE=.\File.h
# End Source File
# Begin Source File

SOURCE=.\IRCClient.cpp
# End Source File
# Begin Source File

SOURCE=.\IRCClient.h
# End Source File
# Begin Source File

SOURCE=.\MD5.cpp
# End Source File
# Begin Source File

SOURCE=.\MD5.h
# End Source File
# Begin Source File

SOURCE=.\panic.cpp
# End Source File
# Begin Source File

SOURCE=.\panic.h
# End Source File
# Begin Source File

SOURCE=.\QueueT.h
# End Source File
# Begin Source File

SOURCE=.\Reli.h
# End Source File
# Begin Source File

SOURCE=.\ReliMT.cpp
# End Source File
# Begin Source File

SOURCE=.\ReliMT.h
# End Source File
# Begin Source File

SOURCE=.\SockUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\SockUtils.h
# End Source File
# Begin Source File

SOURCE=.\SplitJoin.cpp
# End Source File
# Begin Source File

SOURCE=.\SplitJoin.h
# End Source File
# Begin Source File

SOURCE=.\ssprintf.cpp
# End Source File
# Begin Source File

SOURCE=.\ssprintf.h
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.h
# End Source File
# Begin Source File

SOURCE=.\trim.cpp
# End Source File
# Begin Source File

SOURCE=.\trim.h
# End Source File
# Begin Source File

SOURCE=.\verify.h
# End Source File
# End Target
# End Project
