# Microsoft Developer Studio Project File - Name="shader" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=shader - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shader.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shader.mak" CFG="shader - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shader - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "shader - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shader - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "../../../include" /I "../" /I "../main" /I "../glapi" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /Zm500 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "shader - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../../include" /I "../" /I "../main" /I "../glapi" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /Zm500 /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "shader - Win32 Release"
# Name "shader - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\arbfragparse.c
# End Source File
# Begin Source File

SOURCE=.\arbprogparse.c
# End Source File
# Begin Source File

SOURCE=.\arbprogram.c
# End Source File
# Begin Source File

SOURCE=.\arbvertparse.c
# End Source File
# Begin Source File

SOURCE=.\grammar.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\grammar_mesa.c
# End Source File
# Begin Source File

SOURCE=.\nvfragparse.c
# End Source File
# Begin Source File

SOURCE=.\nvprogram.c
# End Source File
# Begin Source File

SOURCE=.\nvvertexec.c
# End Source File
# Begin Source File

SOURCE=.\nvvertparse.c
# End Source File
# Begin Source File

SOURCE=.\program.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\arbfragparse.h
# End Source File
# Begin Source File

SOURCE=.\arbprogparse.h
# End Source File
# Begin Source File

SOURCE=.\arbprogram.h
# End Source File
# Begin Source File

SOURCE=.\arbprogram_syn.h
# End Source File
# Begin Source File

SOURCE=.\arbvertparse.h
# End Source File
# Begin Source File

SOURCE=.\grammar.h
# End Source File
# Begin Source File

SOURCE=.\grammar_mesa.h
# End Source File
# Begin Source File

SOURCE=.\grammar_syn.h
# End Source File
# Begin Source File

SOURCE=.\nvfragparse.h
# End Source File
# Begin Source File

SOURCE=.\nvfragprog.h
# End Source File
# Begin Source File

SOURCE=.\nvprogram.h
# End Source File
# Begin Source File

SOURCE=.\nvvertexec.h
# End Source File
# Begin Source File

SOURCE=.\nvvertparse.h
# End Source File
# Begin Source File

SOURCE=.\nvvertprog.h
# End Source File
# Begin Source File

SOURCE=.\program.h
# End Source File
# End Group
# End Target
# End Project
