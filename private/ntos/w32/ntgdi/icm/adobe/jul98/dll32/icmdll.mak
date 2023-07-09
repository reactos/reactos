# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "icmdll.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/icmdll.dll $(OUTDIR)/icmdll.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ICMDLL" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "ICMDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"icmdll.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"icmdll.bsc" 
BSC32_SBRS= \
	$(INTDIR)/DLLMAIN.SBR \
	$(INTDIR)/csprof.sbr \
	$(INTDIR)/getcsa.sbr \
	$(INTDIR)/getcrd.sbr \
	$(INTDIR)/profcrd.sbr

$(OUTDIR)/icmdll.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"icmdll.pdb" /MACHINE:I386 /DEF:".\icmdll.def"\
 /OUT:$(OUTDIR)/"icmdll.dll" /IMPLIB:$(OUTDIR)/"icmdll.lib" 
DEF_FILE=.\icmdll.def
LINK32_OBJS= \
	$(INTDIR)/DLLMAIN.OBJ \
	$(INTDIR)/csprof.obj \
	$(INTDIR)/getcsa.obj \
	$(INTDIR)/getcrd.obj \
	$(INTDIR)/profcrd.obj

$(OUTDIR)/icmdll.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/icmdll.dll $(OUTDIR)/icmdll.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ICMDLL" /FR /c
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "ICMDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"icmdll.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"icmdll.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"icmdll.bsc" 
BSC32_SBRS= \
	$(INTDIR)/DLLMAIN.SBR \
	$(INTDIR)/csprof.sbr \
	$(INTDIR)/getcsa.sbr \
	$(INTDIR)/getcrd.sbr \
	$(INTDIR)/profcrd.sbr

$(OUTDIR)/icmdll.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"icmdll.pdb" /DEBUG /MACHINE:I386 /DEF:".\icmdll.def"\
 /OUT:$(OUTDIR)/"icmdll.dll" /IMPLIB:$(OUTDIR)/"icmdll.lib" 
DEF_FILE=.\icmdll.def
LINK32_OBJS= \
	$(INTDIR)/DLLMAIN.OBJ \
	$(INTDIR)/csprof.obj \
	$(INTDIR)/getcsa.obj \
	$(INTDIR)/getcrd.obj \
	$(INTDIR)/profcrd.obj

$(OUTDIR)/icmdll.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\DLLMAIN.C

$(INTDIR)/DLLMAIN.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\icmdll.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\csprof.c
DEP_CSPRO=\
	.\GENERIC.H\
	.\icmdll.h\
	.\icc.h\
	.\icm.h\
	.\icc_i386.h\
	.\csprof.h\
	.\getcrd.h\
	.\getcsa.h\
	.\icmstr.h

$(INTDIR)/csprof.obj :  $(SOURCE)  $(DEP_CSPRO) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\getcsa.c
DEP_GETCS=\
	.\GENERIC.H\
	.\icmstr.h\
	.\icmdll.h\
	.\icc.h\
	.\icm.h\
	.\icc_i386.h\
	.\csprof.h\
	.\getcrd.h\
	.\getcsa.h

$(INTDIR)/getcsa.obj :  $(SOURCE)  $(DEP_GETCS) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\getcrd.c
DEP_GETCR=\
	.\GENERIC.H\
	.\icmdll.h\
	.\icc.h\
	.\icm.h\
	.\icc_i386.h\
	.\csprof.h\
	.\getcrd.h\
	.\getcsa.h\
	.\icmstr.h

$(INTDIR)/getcrd.obj :  $(SOURCE)  $(DEP_GETCR) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\profcrd.c
DEP_PROFC=\
	.\GENERIC.H\
	.\icmdll.h\
	.\icc.h\
	.\icm.h\
	.\icc_i386.h\
	.\csprof.h\
	.\getcrd.h\
	.\getcsa.h\
	.\icmstr.h\
	.\profcrd.h

$(INTDIR)/profcrd.obj :  $(SOURCE)  $(DEP_PROFC) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
