# Microsoft Developer Studio Project File - Name="LibJPEG" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=LibJPEG - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LibJPEG.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LibJPEG.mak" CFG="LibJPEG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LibJPEG - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "LibJPEG - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/FreeImage/LibJPEG", IHAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LibJPEG - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\zlib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "LibJPEG - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

# Name "LibJPEG - Win32 Release"
# Name "LibJPEG - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\jcapimin.c
# End Source File
# Begin Source File

SOURCE=.\jcapistd.c
# End Source File
# Begin Source File

SOURCE=.\jccoefct.c
# End Source File
# Begin Source File

SOURCE=.\jccolor.c
# End Source File
# Begin Source File

SOURCE=.\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jchuff.c
# End Source File
# Begin Source File

SOURCE=.\jcinit.c
# End Source File
# Begin Source File

SOURCE=.\jcmainct.c
# End Source File
# Begin Source File

SOURCE=.\jcmarker.c
# End Source File
# Begin Source File

SOURCE=.\jcmaster.c
# End Source File
# Begin Source File

SOURCE=.\jcomapi.c
# End Source File
# Begin Source File

SOURCE=.\jcparam.c
# End Source File
# Begin Source File

SOURCE=.\jcphuff.c
# End Source File
# Begin Source File

SOURCE=.\jcprepct.c
# End Source File
# Begin Source File

SOURCE=.\jcsample.c
# End Source File
# Begin Source File

SOURCE=.\jctrans.c
# End Source File
# Begin Source File

SOURCE=.\jdapimin.c
# End Source File
# Begin Source File

SOURCE=.\jdapistd.c
# End Source File
# Begin Source File

SOURCE=.\jdatadst.c
# End Source File
# Begin Source File

SOURCE=.\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=.\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=.\jdcolor.c
# End Source File
# Begin Source File

SOURCE=.\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=.\jdhuff.c
# End Source File
# Begin Source File

SOURCE=.\jdinput.c
# End Source File
# Begin Source File

SOURCE=.\jdmainct.c
# End Source File
# Begin Source File

SOURCE=.\jdmarker.c
# End Source File
# Begin Source File

SOURCE=.\jdmaster.c
# End Source File
# Begin Source File

SOURCE=.\jdmerge.c
# End Source File
# Begin Source File

SOURCE=.\jdphuff.c
# End Source File
# Begin Source File

SOURCE=.\jdpostct.c
# End Source File
# Begin Source File

SOURCE=.\jdsample.c
# End Source File
# Begin Source File

SOURCE=.\jdtrans.c
# End Source File
# Begin Source File

SOURCE=.\jerror.c
# End Source File
# Begin Source File

SOURCE=.\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=.\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=.\jfdctint.c
# End Source File
# Begin Source File

SOURCE=.\jidctflt.c
# End Source File
# Begin Source File

SOURCE=.\jidctfst.c
# End Source File
# Begin Source File

SOURCE=.\jidctint.c
# End Source File
# Begin Source File

SOURCE=.\jidctred.c
# End Source File
# Begin Source File

SOURCE=.\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=.\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=.\jquant1.c
# End Source File
# Begin Source File

SOURCE=.\jquant2.c
# End Source File
# Begin Source File

SOURCE=.\jutils.c
# End Source File
# Begin Source File

SOURCE=.\transupp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jversion.h
# End Source File
# Begin Source File

SOURCE=.\transupp.h
# End Source File
# End Group
# End Target
# End Project
