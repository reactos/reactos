# Microsoft Developer Studio Project File - Name="swrast" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=swrast - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "swrast.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "swrast.mak" CFG="swrast - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "swrast - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "swrast - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "swrast - Win32 Release"

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

!ELSEIF  "$(CFG)" == "swrast - Win32 Debug"

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

# Name "swrast - Win32 Release"
# Name "swrast - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\s_aaline.c
# End Source File
# Begin Source File

SOURCE=.\s_aatriangle.c
# End Source File
# Begin Source File

SOURCE=.\s_accum.c
# End Source File
# Begin Source File

SOURCE=.\s_alpha.c
# End Source File
# Begin Source File

SOURCE=.\s_alphabuf.c
# End Source File
# Begin Source File

SOURCE=.\s_bitmap.c
# End Source File
# Begin Source File

SOURCE=.\s_blend.c
# End Source File
# Begin Source File

SOURCE=.\s_buffers.c
# End Source File
# Begin Source File

SOURCE=.\s_context.c
# End Source File
# Begin Source File

SOURCE=.\s_copypix.c
# End Source File
# Begin Source File

SOURCE=.\s_depth.c
# End Source File
# Begin Source File

SOURCE=.\s_drawpix.c
# End Source File
# Begin Source File

SOURCE=.\s_feedback.c
# End Source File
# Begin Source File

SOURCE=.\s_fog.c
# End Source File
# Begin Source File

SOURCE=.\s_imaging.c
# End Source File
# Begin Source File

SOURCE=.\s_lines.c
# End Source File
# Begin Source File

SOURCE=.\s_logic.c
# End Source File
# Begin Source File

SOURCE=.\s_masking.c
# End Source File
# Begin Source File

SOURCE=.\s_nvfragprog.c
# End Source File
# Begin Source File

SOURCE=.\s_pixeltex.c
# End Source File
# Begin Source File

SOURCE=.\s_points.c
# End Source File
# Begin Source File

SOURCE=.\s_readpix.c
# End Source File
# Begin Source File

SOURCE=.\s_span.c
# End Source File
# Begin Source File

SOURCE=.\s_stencil.c
# End Source File
# Begin Source File

SOURCE=.\s_texstore.c
# End Source File
# Begin Source File

SOURCE=.\s_texture.c
# End Source File
# Begin Source File

SOURCE=.\s_triangle.c
# End Source File
# Begin Source File

SOURCE=.\s_zoom.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\s_aaline.h
# End Source File
# Begin Source File

SOURCE=.\s_aalinetemp.h
# End Source File
# Begin Source File

SOURCE=.\s_aatriangle.h
# End Source File
# Begin Source File

SOURCE=.\s_aatritemp.h
# End Source File
# Begin Source File

SOURCE=.\s_accum.h
# End Source File
# Begin Source File

SOURCE=.\s_alpha.h
# End Source File
# Begin Source File

SOURCE=.\s_alphabuf.h
# End Source File
# Begin Source File

SOURCE=.\s_blend.h
# End Source File
# Begin Source File

SOURCE=.\s_context.h
# End Source File
# Begin Source File

SOURCE=.\s_depth.h
# End Source File
# Begin Source File

SOURCE=.\s_drawpix.h
# End Source File
# Begin Source File

SOURCE=.\s_feedback.h
# End Source File
# Begin Source File

SOURCE=.\s_fog.h
# End Source File
# Begin Source File

SOURCE=.\s_lines.h
# End Source File
# Begin Source File

SOURCE=.\s_linetemp.h
# End Source File
# Begin Source File

SOURCE=.\s_logic.h
# End Source File
# Begin Source File

SOURCE=.\s_masking.h
# End Source File
# Begin Source File

SOURCE=.\s_nvfragprog.h
# End Source File
# Begin Source File

SOURCE=.\s_pixeltex.h
# End Source File
# Begin Source File

SOURCE=.\s_points.h
# End Source File
# Begin Source File

SOURCE=.\s_pointtemp.h
# End Source File
# Begin Source File

SOURCE=.\s_span.h
# End Source File
# Begin Source File

SOURCE=.\s_spantemp.h
# End Source File
# Begin Source File

SOURCE=.\s_stencil.h
# End Source File
# Begin Source File

SOURCE=.\s_texture.h
# End Source File
# Begin Source File

SOURCE=.\s_triangle.h
# End Source File
# Begin Source File

SOURCE=.\s_trispan.h
# End Source File
# Begin Source File

SOURCE=.\s_tritemp.h
# End Source File
# Begin Source File

SOURCE=.\s_zoom.h
# End Source File
# Begin Source File

SOURCE=.\swrast.h
# End Source File
# End Group
# End Target
# End Project
