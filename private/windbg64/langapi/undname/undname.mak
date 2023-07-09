# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=undname - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to undname - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "undname - Win32 Release" && "$(CFG)" !=\
 "undname - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "undname.mak" CFG="undname - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "undname - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "undname - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "undname - Win32 Release"

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

ALL : "$(OUTDIR)\undname.exe"

CLEAN : 
	-@erase "$(OUTDIR)\undname.exe"
	-@erase "$(INTDIR)\UNDNAME.OBJ"
	-@erase "$(INTDIR)\STUBMAIN.OBJ"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /O2 /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /O2 /D "NDEBUG" /D "_CONSOLE" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /ML /W3 /O2 /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/undname.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=
# ADD BASE RSC /l 0x0 /d "NDEBUG"
# ADD RSC /l 0x0 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/undname.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/undname.pdb" /machine:$(PROCESSOR_ARCHITECTURE) /out:"$(OUTDIR)/undname.exe" 
LINK32_OBJS= \
	"$(INTDIR)/UNDNAME.OBJ" \
	"$(INTDIR)/STUBMAIN.OBJ"

"$(OUTDIR)\undname.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "undname - Win32 Debug"

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

ALL : "$(OUTDIR)\undname.exe"

CLEAN : 
	-@erase "$(OUTDIR)\undname.exe"
	-@erase "$(INTDIR)\UNDNAME.OBJ"
	-@erase "$(INTDIR)\STUBMAIN.OBJ"
	-@erase "$(INTDIR)\undname.ilk"
	-@erase "$(INTDIR)\undname.pdb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\vc40.idb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /ML /W3 /Zi /Od /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /Zi /Od /D "_DEBUG" /D "_CONSOLE" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MLd /W3 /Gm /Zi /Od /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/undname.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=
# ADD BASE RSC /l 0x0 /d "_DEBUG"
# ADD RSC /l 0x0 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/undname.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib odbc32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/undname.pdb" /debug /machine:I386 /out:"$(OUTDIR)/undname.exe" 
LINK32_OBJS= \
	"$(INTDIR)/UNDNAME.OBJ" \
	"$(INTDIR)/STUBMAIN.OBJ"

"$(OUTDIR)\undname.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "undname - Win32 Release"
# Name "undname - Win32 Debug"

!IF  "$(CFG)" == "undname - Win32 Release"

!ELSEIF  "$(CFG)" == "undname - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\UNDNAME.CXX
DEP_CPP_UNDNA=\
	".\undname.hxx"\
	".\undname.inl"\
	".\undname.h"\
	

"$(INTDIR)\UNDNAME.OBJ" : $(SOURCE) $(DEP_CPP_UNDNA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\STUBMAIN.CXX
DEP_CPP_STUBM=\
	".\undname.h"\
	

"$(INTDIR)\STUBMAIN.OBJ" : $(SOURCE) $(DEP_CPP_STUBM) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
