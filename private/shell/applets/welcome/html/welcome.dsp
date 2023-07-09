# Microsoft Developer Studio Project File - Name="Welcome" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Welcome - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "welcome.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "welcome.mak" CFG="Welcome - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Welcome - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Welcome - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Welcome - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Welcome.exe"
# PROP BASE Bsc_Name "Welcome.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe"
# PROP Rebuild_Opt "-c"
# PROP Target_File "\ntrel\welcome.exe"
# PROP Bsc_Name "\ntrel\welcome.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Welcome - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Welcome.exe"
# PROP BASE Bsc_Name "Welcome.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe"
# PROP Rebuild_Opt "-c"
# PROP Target_File "welcome.exe"
# PROP Bsc_Name "welcome.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Welcome - Win32 Release"
# Name "Welcome - Win32 Debug"

!IF  "$(CFG)" == "Welcome - Win32 Release"

!ELSEIF  "$(CFG)" == "Welcome - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "*.c *.cpp *.rc"
# Begin Source File

SOURCE=.\container.cpp
# End Source File
# Begin Source File

SOURCE=.\eventsink.cpp
# End Source File
# Begin Source File

SOURCE=.\IniData.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\webapp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h; *.hxx; *.hpp; *.idl"
# Begin Source File

SOURCE=.\container.h
# End Source File
# Begin Source File

SOURCE=.\crtfree.h
# End Source File
# Begin Source File

SOURCE=.\IniData.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\webapp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.ico; *.bmp; *.html; *.htm; *.gif; *.jpg; *.jpeg; *.cur"
# Begin Source File

SOURCE=.\res\banner.gif
# End Source File
# Begin Source File

SOURCE=.\res\clipboard.bmp
# End Source File
# Begin Source File

SOURCE=.\res\computer.bmp
# End Source File
# Begin Source File

SOURCE=.\res\disk.bmp
# End Source File
# Begin Source File

SOURCE=.\res\globe.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logo.gif
# End Source File
# Begin Source File

SOURCE=.\res\noda.htm
# End Source File
# Begin Source File

SOURCE=.\res\styles.css
# End Source File
# Begin Source File

SOURCE=.\res\welcome.htm
# End Source File
# Begin Source File

SOURCE=.\res\Welcome.ico
# End Source File
# Begin Source File

SOURCE=.\res\welcome.js
# End Source File
# End Group
# End Target
# End Project
