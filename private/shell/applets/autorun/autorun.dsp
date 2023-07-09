# Microsoft Developer Studio Project File - Name="Autorun" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Autorun - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "autorun.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "autorun.mak" CFG="Autorun - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Autorun - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Autorun - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Autorun - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Autorun.exe"
# PROP BASE Bsc_Name "Autorun.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe"
# PROP Rebuild_Opt "-c"
# PROP Target_File "\ntrel\autorun.exe"
# PROP Bsc_Name "\ntrel\autorun.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Autorun - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Autorun.exe"
# PROP BASE Bsc_Name "Autorun.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe"
# PROP Rebuild_Opt "-c"
# PROP Target_File "autorun.exe"
# PROP Bsc_Name "autorun.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Autorun - Win32 Release"
# Name "Autorun - Win32 Debug"

!IF  "$(CFG)" == "Autorun - Win32 Release"

!ELSEIF  "$(CFG)" == "Autorun - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "*.c; *.cpp; *.rc"
# Begin Source File

SOURCE=.\autorun.cpp
# End Source File
# Begin Source File

SOURCE=.\dataitem.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgapp.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h; *.hxx; *.hpp; *.idl"
# Begin Source File

SOURCE=.\autorun.h
# End Source File
# Begin Source File

SOURCE=.\crtfree.h
# End Source File
# Begin Source File

SOURCE=.\DataItem.h
# End Source File
# Begin Source File

SOURCE=.\dlgapp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.ico; *.bmp; *.html; *.htm; *.gif; *.jpg; *.jpeg; *.cur"
# Begin Source File

SOURCE=.\res\autorun.ico
# End Source File
# Begin Source File

SOURCE=.\res\brhand.cur
# End Source File
# End Group
# End Target
# End Project
