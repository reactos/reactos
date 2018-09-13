# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=sample - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to sample - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "sample - Win32 Release" && "$(CFG)" != "sample - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "sample.mak" CFG="sample - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sample - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sample - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "sample - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "sample - Win32 Release"

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

ALL : "$(OUTDIR)\sample.dll"

CLEAN : 
	-@erase "$(INTDIR)\comobj.obj"
	-@erase "$(INTDIR)\defguid.obj"
	-@erase "$(INTDIR)\getlabel.obj"
	-@erase "$(INTDIR)\sample.res"
	-@erase "$(OUTDIR)\sample.dll"
	-@erase "$(OUTDIR)\sample.exp"
	-@erase "$(OUTDIR)\sample.lib"
	-@erase "$(OUTDIR)\sample.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX"project.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/sample.pch" /YX"project.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sample.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/sample.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /machine:I386
# SUBTRACT LINK32 /incremental:yes /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/sample.pdb" /map:"$(INTDIR)/sample.map" /machine:I386\
 /def:".\sample.def" /out:"$(OUTDIR)/sample.dll" /implib:"$(OUTDIR)/sample.lib" 
DEF_FILE= \
	".\sample.def"
LINK32_OBJS= \
	"$(INTDIR)\comobj.obj" \
	"$(INTDIR)\defguid.obj" \
	"$(INTDIR)\getlabel.obj" \
	"$(INTDIR)\sample.res"

"$(OUTDIR)\sample.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

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

ALL : "$(OUTDIR)\sample.dll" "$(OUTDIR)\sample.bsc"

CLEAN : 
	-@erase "$(INTDIR)\comobj.obj"
	-@erase "$(INTDIR)\comobj.sbr"
	-@erase "$(INTDIR)\defguid.obj"
	-@erase "$(INTDIR)\defguid.sbr"
	-@erase "$(INTDIR)\getlabel.obj"
	-@erase "$(INTDIR)\getlabel.sbr"
	-@erase "$(INTDIR)\sample.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\sample.bsc"
	-@erase "$(OUTDIR)\sample.dll"
	-@erase "$(OUTDIR)\sample.exp"
	-@erase "$(OUTDIR)\sample.lib"
	-@erase "$(OUTDIR)\sample.map"
	-@erase "$(OUTDIR)\sample.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"project.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/sample.pch" /YX"project.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sample.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/sample.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\comobj.sbr" \
	"$(INTDIR)\defguid.sbr" \
	"$(INTDIR)\getlabel.sbr"

"$(OUTDIR)\sample.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/sample.pdb" /map:"$(INTDIR)/sample.map" /debug /machine:I386\
 /def:".\sample.def" /out:"$(OUTDIR)/sample.dll" /implib:"$(OUTDIR)/sample.lib" 
DEF_FILE= \
	".\sample.def"
LINK32_OBJS= \
	"$(INTDIR)\comobj.obj" \
	"$(INTDIR)\defguid.obj" \
	"$(INTDIR)\getlabel.obj" \
	"$(INTDIR)\sample.res"

"$(OUTDIR)\sample.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "sample - Win32 Release"
# Name "sample - Win32 Debug"

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\comobj.cpp
DEP_CPP_COMOB=\
	".\classes.h"\
	".\defguid.h"\
	".\project.h"\
	".\ratings.h"\
	

!IF  "$(CFG)" == "sample - Win32 Release"


"$(INTDIR)\comobj.obj" : $(SOURCE) $(DEP_CPP_COMOB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"


"$(INTDIR)\comobj.obj" : $(SOURCE) $(DEP_CPP_COMOB) "$(INTDIR)"

"$(INTDIR)\comobj.sbr" : $(SOURCE) $(DEP_CPP_COMOB) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\defguid.cpp
DEP_CPP_DEFGU=\
	".\classes.h"\
	".\defguid.h"\
	".\project.h"\
	".\ratings.h"\
	

!IF  "$(CFG)" == "sample - Win32 Release"


"$(INTDIR)\defguid.obj" : $(SOURCE) $(DEP_CPP_DEFGU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"


"$(INTDIR)\defguid.obj" : $(SOURCE) $(DEP_CPP_DEFGU) "$(INTDIR)"

"$(INTDIR)\defguid.sbr" : $(SOURCE) $(DEP_CPP_DEFGU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\defguid.h

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\getlabel.cpp
DEP_CPP_GETLA=\
	".\classes.h"\
	".\defguid.h"\
	".\project.h"\
	".\ratings.h"\
	

!IF  "$(CFG)" == "sample - Win32 Release"


"$(INTDIR)\getlabel.obj" : $(SOURCE) $(DEP_CPP_GETLA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"


"$(INTDIR)\getlabel.obj" : $(SOURCE) $(DEP_CPP_GETLA) "$(INTDIR)"

"$(INTDIR)\getlabel.sbr" : $(SOURCE) $(DEP_CPP_GETLA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ratings.h

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\classes.h

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\project.h

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sample.def

!IF  "$(CFG)" == "sample - Win32 Release"

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sample.rc
DEP_RSC_SAMPL=\
	{$(INCLUDE)}"\..\sdk\inc16\version.h"\
	{$(INCLUDE)}"\version.h"\
	

"$(INTDIR)\sample.res" : $(SOURCE) $(DEP_RSC_SAMPL) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
