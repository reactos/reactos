# Microsoft Developer Studio Project File - Name="math" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=math - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "math.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "math.mak" CFG="math - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "math - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "math - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "math - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "../../../include" /I "../" /I "../main" /I "../glapi" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "math - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../../include" /I "../" /I "../main" /I "../glapi" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
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

# Name "math - Win32 Release"
# Name "math - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\m_debug_clip.c
# End Source File
# Begin Source File

SOURCE=.\m_debug_norm.c
# End Source File
# Begin Source File

SOURCE=.\m_debug_xform.c
# End Source File
# Begin Source File

SOURCE=.\m_eval.c
# End Source File
# Begin Source File

SOURCE=.\m_matrix.c
# End Source File
# Begin Source File

SOURCE=.\m_translate.c
# End Source File
# Begin Source File

SOURCE=.\m_vector.c
# End Source File
# Begin Source File

SOURCE=.\m_xform.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\m_clip_tmp.h
# End Source File
# Begin Source File

SOURCE=.\m_copy_tmp.h
# End Source File
# Begin Source File

SOURCE=.\m_debug.h
# End Source File
# Begin Source File

SOURCE=.\m_debug_util.h
# End Source File
# Begin Source File

SOURCE=.\m_dotprod_tmp.h
# End Source File
# Begin Source File

SOURCE=.\m_eval.h
# End Source File
# Begin Source File

SOURCE=.\m_matrix.h
# End Source File
# Begin Source File

SOURCE=.\m_norm_tmp.h
# End Source File
# Begin Source File

SOURCE=.\m_trans_tmp.h
# End Source File
# Begin Source File

SOURCE=.\m_translate.h
# End Source File
# Begin Source File

SOURCE=.\m_vector.h
# End Source File
# Begin Source File

SOURCE=.\m_xform.h
# End Source File
# Begin Source File

SOURCE=.\m_xform_tmp.h
# End Source File
# Begin Source File

SOURCE=.\mathmod.h
# End Source File
# End Group
# End Target
# End Project
