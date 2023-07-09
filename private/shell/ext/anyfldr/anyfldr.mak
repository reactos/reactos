# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=otherfld - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to otherfld - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "otherfld - Win32 Release" && "$(CFG)" !=\
 "otherfld - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "otherfld.mak" CFG="otherfld - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "otherfld - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "otherfld - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "otherfld - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "otherfld - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Retail"
# PROP Intermediate_Dir "Retail"
# PROP Target_Dir ""
OUTDIR=.\Retail
INTDIR=.\Retail

ALL : "$(OUTDIR)\otherfld.dll"

CLEAN : 
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\otherfld.obj"
	-@erase "$(INTDIR)\otherfld.res"
	-@erase "$(OUTDIR)\otherfld.dll"
	-@erase "$(OUTDIR)\otherfld.exp"
	-@erase "$(OUTDIR)\otherfld.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/otherfld.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Retail/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/otherfld.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/otherfld.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /entry:"DllMain"\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/otherfld.pdb"\
 /machine:I386 /nodefaultlib /def:".\otherfld.def" /out:"$(OUTDIR)/otherfld.dll"\
 /implib:"$(OUTDIR)/otherfld.lib" 
DEF_FILE= \
	".\otherfld.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\otherfld.obj" \
	"$(INTDIR)\otherfld.res"

"$(OUTDIR)\otherfld.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

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

ALL : "$(OUTDIR)\otherfld.dll"

CLEAN : 
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\otherfld.obj"
	-@erase "$(INTDIR)\otherfld.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\otherfld.dll"
	-@erase "$(OUTDIR)\otherfld.exp"
	-@erase "$(OUTDIR)\otherfld.ilk"
	-@erase "$(OUTDIR)\otherfld.lib"
	-@erase "$(OUTDIR)\otherfld.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/otherfld.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/otherfld.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/otherfld.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /entry:"DllMain" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /entry:"DllMain"\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/otherfld.pdb" /debug\
 /machine:I386 /nodefaultlib /def:".\otherfld.def" /out:"$(OUTDIR)/otherfld.dll"\
 /implib:"$(OUTDIR)/otherfld.lib" 
DEF_FILE= \
	".\otherfld.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\otherfld.obj" \
	"$(INTDIR)\otherfld.res"

"$(OUTDIR)\otherfld.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "otherfld - Win32 Release"
# Name "otherfld - Win32 Debug"

!IF  "$(CFG)" == "otherfld - Win32 Release"

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\resource.h

!IF  "$(CFG)" == "otherfld - Win32 Release"

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\debug.h

!IF  "$(CFG)" == "otherfld - Win32 Release"

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\otherfld.c
DEP_CPP_OTHER=\
	".\debug.h"\
	".\otherfld.h"\
	

"$(INTDIR)\otherfld.obj" : $(SOURCE) $(DEP_CPP_OTHER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\otherfld.h

!IF  "$(CFG)" == "otherfld - Win32 Release"

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\otherfld.rc
DEP_RSC_OTHERF=\
	".\otherfld.ico"\
	".\otherfld.rcv"\
	{$(INCLUDE)}"\..\sdk\inc16\common.ver"\
	{$(INCLUDE)}"\..\sdk\inc16\version.h"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\version.h"\
	

"$(INTDIR)\otherfld.res" : $(SOURCE) $(DEP_RSC_OTHERF) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\debug.c
DEP_CPP_DEBUG=\
	".\debug.h"\
	

"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\otherfld.def

!IF  "$(CFG)" == "otherfld - Win32 Release"

!ELSEIF  "$(CFG)" == "otherfld - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
