# Microsoft Developer Studio Project File - Name="main" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=main - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "main.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "main.mak" CFG="main - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "main - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "main - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "main - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "../../../include" /I "../" /I "../glapi" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /Zm1000 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "main - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../../include" /I "../" /I "../glapi" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /Zm1000 /c
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

# Name "main - Win32 Release"
# Name "main - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accum.c
# End Source File
# Begin Source File

SOURCE=.\api_arrayelt.c
# End Source File
# Begin Source File

SOURCE=.\api_loopback.c
# End Source File
# Begin Source File

SOURCE=.\api_noop.c
# End Source File
# Begin Source File

SOURCE=.\api_validate.c
# End Source File
# Begin Source File

SOURCE=.\arbfragparse.c
# End Source File
# Begin Source File

SOURCE=.\arbparse.c
# End Source File
# Begin Source File

SOURCE=.\arbprogram.c
# End Source File
# Begin Source File

SOURCE=.\arbvertparse.c
# End Source File
# Begin Source File

SOURCE=.\attrib.c
# End Source File
# Begin Source File

SOURCE=.\blend.c
# End Source File
# Begin Source File

SOURCE=.\bufferobj.c
# End Source File
# Begin Source File

SOURCE=.\buffers.c
# End Source File
# Begin Source File

SOURCE=.\clip.c
# End Source File
# Begin Source File

SOURCE=.\colortab.c
# End Source File
# Begin Source File

SOURCE=.\context.c
# End Source File
# Begin Source File

SOURCE=.\convolve.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\depth.c
# End Source File
# Begin Source File

SOURCE=.\dispatch.c
# End Source File
# Begin Source File

SOURCE=.\dlist.c
# End Source File
# Begin Source File

SOURCE=.\drawpix.c
# End Source File
# Begin Source File

SOURCE=.\enable.c
# End Source File
# Begin Source File

SOURCE=.\enums.c
# End Source File
# Begin Source File

SOURCE=.\eval.c
# End Source File
# Begin Source File

SOURCE=.\extensions.c
# End Source File
# Begin Source File

SOURCE=.\feedback.c
# End Source File
# Begin Source File

SOURCE=.\fog.c
# End Source File
# Begin Source File

SOURCE=.\get.c
# End Source File
# Begin Source File

SOURCE=.\hash.c
# End Source File
# Begin Source File

SOURCE=.\hint.c
# End Source File
# Begin Source File

SOURCE=.\histogram.c
# End Source File
# Begin Source File

SOURCE=.\image.c
# End Source File
# Begin Source File

SOURCE=.\imports.c
# End Source File
# Begin Source File

SOURCE=.\light.c
# End Source File
# Begin Source File

SOURCE=.\lines.c
# End Source File
# Begin Source File

SOURCE=.\matrix.c
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

SOURCE=.\occlude.c
# End Source File
# Begin Source File

SOURCE=.\pixel.c
# End Source File
# Begin Source File

SOURCE=.\points.c
# End Source File
# Begin Source File

SOURCE=.\polygon.c
# End Source File
# Begin Source File

SOURCE=.\program.c
# End Source File
# Begin Source File

SOURCE=.\rastpos.c
# End Source File
# Begin Source File

SOURCE=.\state.c
# End Source File
# Begin Source File

SOURCE=.\stencil.c
# End Source File
# Begin Source File

SOURCE=.\texcompress.c
# End Source File
# Begin Source File

SOURCE=.\texformat.c
# End Source File
# Begin Source File

SOURCE=.\teximage.c
# End Source File
# Begin Source File

SOURCE=.\texobj.c
# End Source File
# Begin Source File

SOURCE=.\texstate.c
# End Source File
# Begin Source File

SOURCE=.\texstore.c
# End Source File
# Begin Source File

SOURCE=.\texutil.c
# End Source File
# Begin Source File

SOURCE=.\varray.c
# End Source File
# Begin Source File

SOURCE=.\vtxfmt.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accum.h
# End Source File
# Begin Source File

SOURCE=.\api_arrayelt.h
# End Source File
# Begin Source File

SOURCE=.\api_eval.h
# End Source File
# Begin Source File

SOURCE=.\api_loopback.h
# End Source File
# Begin Source File

SOURCE=.\api_noop.h
# End Source File
# Begin Source File

SOURCE=.\api_validate.h
# End Source File
# Begin Source File

SOURCE=.\arbfragparse.h
# End Source File
# Begin Source File

SOURCE=.\arbparse.h
# End Source File
# Begin Source File

SOURCE=.\arbparse_syn.h
# End Source File
# Begin Source File

SOURCE=.\arbprogram.h
# End Source File
# Begin Source File

SOURCE=.\arbvertparse.h
# End Source File
# Begin Source File

SOURCE=.\attrib.h
# End Source File
# Begin Source File

SOURCE=.\blend.h
# End Source File
# Begin Source File

SOURCE=.\bufferobj.h
# End Source File
# Begin Source File

SOURCE=.\buffers.h
# End Source File
# Begin Source File

SOURCE=.\clip.h
# End Source File
# Begin Source File

SOURCE=.\colormac.h
# End Source File
# Begin Source File

SOURCE=.\colortab.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\context.h
# End Source File
# Begin Source File

SOURCE=.\convolve.h
# End Source File
# Begin Source File

SOURCE=.\dd.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\depth.h
# End Source File
# Begin Source File

SOURCE=.\dlist.h
# End Source File
# Begin Source File

SOURCE=.\drawpix.h
# End Source File
# Begin Source File

SOURCE=.\enable.h
# End Source File
# Begin Source File

SOURCE=.\enums.h
# End Source File
# Begin Source File

SOURCE=.\eval.h
# End Source File
# Begin Source File

SOURCE=.\extensions.h
# End Source File
# Begin Source File

SOURCE=.\feedback.h
# End Source File
# Begin Source File

SOURCE=.\fog.h
# End Source File
# Begin Source File

SOURCE=.\get.h
# End Source File
# Begin Source File

SOURCE=.\glheader.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\hint.h
# End Source File
# Begin Source File

SOURCE=.\histogram.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\imports.h
# End Source File
# Begin Source File

SOURCE=.\light.h
# End Source File
# Begin Source File

SOURCE=.\lines.h
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\matrix.h
# End Source File
# Begin Source File

SOURCE=.\mtypes.h
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

SOURCE=.\occlude.h
# End Source File
# Begin Source File

SOURCE=.\pixel.h
# End Source File
# Begin Source File

SOURCE=.\points.h
# End Source File
# Begin Source File

SOURCE=.\polygon.h
# End Source File
# Begin Source File

SOURCE=.\program.h
# End Source File
# Begin Source File

SOURCE=.\rastpos.h
# End Source File
# Begin Source File

SOURCE=.\simple_list.h
# End Source File
# Begin Source File

SOURCE=.\state.h
# End Source File
# Begin Source File

SOURCE=.\stencil.h
# End Source File
# Begin Source File

SOURCE=.\texcompress.h
# End Source File
# Begin Source File

SOURCE=.\texformat.h
# End Source File
# Begin Source File

SOURCE=.\texformat_tmp.h
# End Source File
# Begin Source File

SOURCE=.\teximage.h
# End Source File
# Begin Source File

SOURCE=.\texobj.h
# End Source File
# Begin Source File

SOURCE=.\texstate.h
# End Source File
# Begin Source File

SOURCE=.\texstore.h
# End Source File
# Begin Source File

SOURCE=.\texutil.h
# End Source File
# Begin Source File

SOURCE=.\texutil_tmp.h
# End Source File
# Begin Source File

SOURCE=.\varray.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\vtxfmt.h
# End Source File
# Begin Source File

SOURCE=.\vtxfmt_tmp.h
# End Source File
# End Group
# End Target
# End Project
