# Microsoft Developer Studio Project File - Name="softx86_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=softx86_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "softx86_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "softx86_static.mak" CFG="softx86_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "softx86_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "softx86_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "softx86_static - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I "..\include\\" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\softx86_static.lib"

!ELSEIF  "$(CFG)" == "softx86_static - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\include\\" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\softx86_static.lib"

!ENDIF 

# Begin Target

# Name "softx86_static - Win32 Release"
# Name "softx86_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\aaa.c
# End Source File
# Begin Source File

SOURCE=.\add.c
# End Source File
# Begin Source File

SOURCE=.\binops.c
# End Source File
# Begin Source File

SOURCE=.\cbw.c
# End Source File
# Begin Source File

SOURCE=.\clc.c
# End Source File
# Begin Source File

SOURCE=.\drooling_duh.c
# End Source File
# Begin Source File

SOURCE=.\fpu.c
# End Source File
# Begin Source File

SOURCE=.\groupies.c
# End Source File
# Begin Source File

SOURCE=.\inc.c
# End Source File
# Begin Source File

SOURCE=.\interrupts.c
# End Source File
# Begin Source File

SOURCE=.\ioport.c
# End Source File
# Begin Source File

SOURCE=.\jumpy.c
# End Source File
# Begin Source File

SOURCE=.\mov.c
# End Source File
# Begin Source File

SOURCE=.\optable.c
# End Source File
# Begin Source File

SOURCE=.\prefixes.c
# End Source File
# Begin Source File

SOURCE=.\procframe.c
# End Source File
# Begin Source File

SOURCE=.\pushpop.c
# End Source File
# Begin Source File

SOURCE=.\shovel.c
# End Source File
# Begin Source File

SOURCE=.\softx86.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\aaa.h
# End Source File
# Begin Source File

SOURCE=.\add.h
# End Source File
# Begin Source File

SOURCE=.\binops.h
# End Source File
# Begin Source File

SOURCE=.\cbw.h
# End Source File
# Begin Source File

SOURCE=.\clc.h
# End Source File
# Begin Source File

SOURCE=.\drooling_duh.h
# End Source File
# Begin Source File

SOURCE=.\fpu.h
# End Source File
# Begin Source File

SOURCE=.\groupies.h
# End Source File
# Begin Source File

SOURCE=.\inc.h
# End Source File
# Begin Source File

SOURCE=.\interrupts.h
# End Source File
# Begin Source File

SOURCE=.\ioport.h
# End Source File
# Begin Source File

SOURCE=.\jumpy.h
# End Source File
# Begin Source File

SOURCE=.\mov.h
# End Source File
# Begin Source File

SOURCE=.\optable.h
# End Source File
# Begin Source File

SOURCE=.\prefixes.h
# End Source File
# Begin Source File

SOURCE=.\procframe.h
# End Source File
# Begin Source File

SOURCE=.\pushpop.h
# End Source File
# Begin Source File

SOURCE=.\shovel.h
# End Source File
# Begin Source File

SOURCE=..\include\softx86.h
# End Source File
# Begin Source File

SOURCE=..\include\softx86cfg.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\readme.txt
# End Source File
# End Target
# End Project
