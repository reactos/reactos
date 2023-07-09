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
!MESSAGE NMAKE /f "PLUSTAB.MAK" CFG="Win32 Debug"
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
# PROP Target_Last_Scanned "Win32 Release"
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

ALL : $(OUTDIR)/plustab.dll $(OUTDIR)/PLUSTAB.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp$(OUTDIR)/"PLUSTAB.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"PLUSTAB.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"PLUSTAB.bsc" 

$(OUTDIR)/PLUSTAB.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /NOLOGO /DLL /MACHINE:I386 /OUT:"$(OUTDIR)\plustab.dll" /SUBSYSTEM:windows,4.0
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /NOLOGO\
 /DLL /INCREMENTAL:no /PDB:$(OUTDIR)/"PLUSTAB.pdb" /MACHINE:I386\
 /DEF:".\PLUSTAB.DEF" /OUT:$(OUTDIR)\"plustab.dll"\
 /IMPLIB:$(OUTDIR)/"PLUSTAB.lib" /SUBSYSTEM:windows,4.0  
DEF_FILE=.\PLUSTAB.DEF
LINK32_OBJS= \
	$(INTDIR)/PLUSTAB.OBJ \
	$(INTDIR)/CLSSFACT.OBJ \
	$(INTDIR)/PLUSTAB.res \
	$(INTDIR)/PROPSEXT.OBJ \
	$(INTDIR)/REGUTILS.OBJ \
	$(INTDIR)/PICKICON.OBJ

$(OUTDIR)/plustab.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

ALL : $(OUTDIR)/plustab.dll $(OUTDIR)/PLUSTAB.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp$(OUTDIR)/"PLUSTAB.pch" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"PLUSTAB.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"PLUSTAB.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"PLUSTAB.bsc" 

$(OUTDIR)/PLUSTAB.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /NOLOGO /DLL /DEBUG /MACHINE:I386 /OUT:"$(OUTDIR)\plustab.dll" /SUBSYSTEM:windows,4.0
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /NOLOGO\
 /DLL /INCREMENTAL:yes /PDB:$(OUTDIR)/"PLUSTAB.pdb" /DEBUG /MACHINE:I386\
 /DEF:".\PLUSTAB.DEF" /OUT:$(OUTDIR)\"plustab.dll"\
 /IMPLIB:$(OUTDIR)/"PLUSTAB.lib" /SUBSYSTEM:windows,4.0  
DEF_FILE=.\PLUSTAB.DEF
LINK32_OBJS= \
	$(INTDIR)/PLUSTAB.OBJ \
	$(INTDIR)/CLSSFACT.OBJ \
	$(INTDIR)/PLUSTAB.res \
	$(INTDIR)/PROPSEXT.OBJ \
	$(INTDIR)/REGUTILS.OBJ \
	$(INTDIR)/PICKICON.OBJ

$(OUTDIR)/plustab.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

!if "$(DBCS)"=="ON"
CPP_PROJ=$(CPP_PROJ) /D "DBCS"
!endif

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

SOURCE=.\PLUSTAB.CPP
DEP_PLUST=\
	.\PLUSTAB.H\
	.\CLSSFACT.H\
	.\PROPSEXT.H

$(INTDIR)/PLUSTAB.OBJ :  $(SOURCE)  $(DEP_PLUST) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PLUSTAB.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=.\CLSSFACT.CPP
DEP_CLSSF=\
	.\CLSSFACT.H\
	.\PROPSEXT.H

$(INTDIR)/CLSSFACT.OBJ :  $(SOURCE)  $(DEP_CLSSF) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PLUSTAB.RC
DEP_PLUSTA=\
	.\PPP0.ICO\
	.\PPP1.ICO\
	.\PPP2.ICO\
	.\PPP3.ICO\
	.\PLUSTAB.RCV

$(INTDIR)/PLUSTAB.res :  $(SOURCE)  $(DEP_PLUSTA) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PROPSEXT.CPP
DEP_PROPS=\
	.\PROPSEXT.H\
	.\REGUTILS.H\
	.\PICKICON.H

$(INTDIR)/PROPSEXT.OBJ :  $(SOURCE)  $(DEP_PROPS) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\REGUTILS.CPP
DEP_REGUT=\
	.\REGUTILS.H

$(INTDIR)/REGUTILS.OBJ :  $(SOURCE)  $(DEP_REGUT) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PICKICON.CPP
DEP_PICKI=\
	.\PICKICON.H

$(INTDIR)/PICKICON.OBJ :  $(SOURCE)  $(DEP_PICKI) $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
