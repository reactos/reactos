# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=nsc - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to nsc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "nsc - Win32 Release" && "$(CFG)" != "nsc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "nsc.mak" CFG="nsc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nsc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "nsc - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "nsc - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "nsc - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\nsc.exe"

CLEAN : 
	-@erase "$(INTDIR)\autoscrl.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\dropsrc.obj"
	-@erase "$(INTDIR)\Idlist.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\nsc.obj"
	-@erase "$(INTDIR)\nsc.res"
	-@erase "$(INTDIR)\nscdrop.obj"
	-@erase "$(OUTDIR)\nsc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /Gz /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/nsc.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/nsc.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nsc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib libcmt.lib /nologo /version:4.0 /subsystem:windows /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib libcmt.lib /nologo /version:4.0\
 /subsystem:windows /incremental:no /pdb:"$(OUTDIR)/nsc.pdb" /machine:I386\
 /nodefaultlib /out:"$(OUTDIR)/nsc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\autoscrl.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\dropsrc.obj" \
	"$(INTDIR)\Idlist.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\nsc.obj" \
	"$(INTDIR)\nsc.res" \
	"$(INTDIR)\nscdrop.obj"

"$(OUTDIR)\nsc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

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
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\nsc.exe"

CLEAN : 
	-@erase "$(INTDIR)\autoscrl.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\dropsrc.obj"
	-@erase "$(INTDIR)\Idlist.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\nsc.obj"
	-@erase "$(INTDIR)\nsc.res"
	-@erase "$(INTDIR)\nscdrop.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\nsc.exe"
	-@erase "$(OUTDIR)\nsc.ilk"
	-@erase "$(OUTDIR)\nsc.map"
	-@erase "$(OUTDIR)\nsc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /Gz /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/nsc.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/nsc.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/nsc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib libcmt.lib /nologo /version:4.0 /subsystem:windows /map /debug /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib libcmt.lib /nologo /version:4.0\
 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)/nsc.pdb"\
 /map:"$(INTDIR)/nsc.map" /debug /machine:I386 /nodefaultlib\
 /out:"$(OUTDIR)/nsc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\autoscrl.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\dropsrc.obj" \
	"$(INTDIR)\Idlist.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\nsc.obj" \
	"$(INTDIR)\nsc.res" \
	"$(INTDIR)\nscdrop.obj"

"$(OUTDIR)\nsc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "nsc - Win32 Release"
# Name "nsc - Win32 Debug"

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\autoscrl.c
DEP_CPP_AUTOS=\
	".\autoscrl.h"\
	".\common.h"\
	".\Debug.h"\
	

"$(INTDIR)\autoscrl.obj" : $(SOURCE) $(DEP_CPP_AUTOS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\autoscrl.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\common.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Debug.c
DEP_CPP_DEBUG=\
	".\Debug.h"\
	

"$(INTDIR)\Debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Debug.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dropsrc.c
DEP_CPP_DROPS=\
	".\common.h"\
	".\Debug.h"\
	".\dropsrc.h"\
	

"$(INTDIR)\dropsrc.obj" : $(SOURCE) $(DEP_CPP_DROPS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dropsrc.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Idlist.c
DEP_CPP_IDLIS=\
	".\common.h"\
	".\Debug.h"\
	".\idlist.h"\
	

"$(INTDIR)\Idlist.obj" : $(SOURCE) $(DEP_CPP_IDLIS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\idlist.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.c
DEP_CPP_MAIN_=\
	".\Debug.h"\
	".\nsc.H"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\nsc.C
DEP_CPP_NSC_C=\
	".\common.h"\
	".\Debug.h"\
	".\dropsrc.h"\
	".\idlist.h"\
	".\nsc.H"\
	

"$(INTDIR)\nsc.obj" : $(SOURCE) $(DEP_CPP_NSC_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\nsc.H

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nsc.RC

"$(INTDIR)\nsc.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\nscdrop.c
DEP_CPP_NSCDR=\
	".\autoscrl.h"\
	".\common.h"\
	".\Debug.h"\
	".\nsc.H"\
	

"$(INTDIR)\nscdrop.obj" : $(SOURCE) $(DEP_CPP_NSCDR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\resource.h

!IF  "$(CFG)" == "nsc - Win32 Release"

!ELSEIF  "$(CFG)" == "nsc - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
