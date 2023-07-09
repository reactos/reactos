# Microsoft Visual C++ Generated NMAKE File, Format Version 30002
# MSVCPRJ: version 3.00.4300
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "testundn.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
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

ALL : .\WinRel\testundn.exe .\WinRel\testundn.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /YX /O2 /D "NDEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /W3 /YX /O2 /D "NDEBUG" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /W3 /YX /O2 /D "NDEBUG" /D "_CONSOLE" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"testundn.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x0 /d "NDEBUG"
# ADD RSC /l 0x0 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"testundn.bsc" 
BSC32_SBRS= \
	.\WinRel\undname.sbr \
	.\WinRel\testundn.sbr

.\WinRel\testundn.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:"WinRel\undname.pdb" /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:"WinRel\undname.pdb" /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /incremental:no /pdb:"WinRel\undname.pdb" /machine:I386\
 /out:$(OUTDIR)/"testundn.exe" 
DEF_FILE=
LINK32_OBJS= \
	.\WinRel\undname.obj \
	.\WinRel\testundn.obj

.\WinRel\testundn.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

ALL : .\WinDebug\testundn.exe .\WinDebug\testundn.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /Zi /YX /Od /D "_DEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /W3 /Zi /YX /Od /D "_DEBUG" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /W3 /Zi /YX /Od /D "_DEBUG" /D "_CONSOLE" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"testundn.pch" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"testundn.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x0 /d "_DEBUG"
# ADD RSC /l 0x0 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"testundn.bsc" 
BSC32_SBRS= \
	.\WinDebug\undname.sbr \
	.\WinDebug\testundn.sbr

.\WinDebug\testundn.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:"WinDebug\undname.pdb" /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:"WinDebug\undname.pdb" /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /incremental:yes /pdb:"WinDebug\undname.pdb" /debug\
 /machine:I386 /out:$(OUTDIR)/"testundn.exe" 
DEF_FILE=
LINK32_OBJS= \
	.\WinDebug\undname.obj \
	.\WinDebug\testundn.obj

.\WinDebug\testundn.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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
# Begin Target "Win32 Release"

# Name "Win32 Release"
# Name "Win32 Debug"
################################################################################
# Begin Source File

SOURCE=.\undname.cxx
DEP_UNDNA=\
	.\undname.hxx\
	.\undname.inl\
	.\undname.h

!IF  "$(CFG)" == "Win32 Release"

.\WinRel\undname.obj :  $(SOURCE)  $(DEP_UNDNA) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug"

.\WinDebug\undname.obj :  $(SOURCE)  $(DEP_UNDNA) $(INTDIR)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\testundn.cxx
DEP_TESTU=\
	.\undname.h

!IF  "$(CFG)" == "Win32 Release"

.\WinRel\testundn.obj :  $(SOURCE)  $(DEP_TESTU) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug"

.\WinDebug\testundn.obj :  $(SOURCE)  $(DEP_TESTU) $(INTDIR)

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
