# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=sendmail - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to sendmail - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "sendmail - Win32 Release" && "$(CFG)" !=\
 "sendmail - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "sendmail.mak" CFG="sendmail - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sendmail - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sendmail - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "sendmail - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "sendmail - Win32 Release"

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
# Begin Custom Macros
TargetDir=.\Retail
TargetName=sendmail
# End Custom Macros

ALL : "$(OUTDIR)\sendmail.dll" "$(OUTDIR)\sendmail.sym"

CLEAN : 
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\dll.obj"
	-@erase "$(INTDIR)\sendmail.obj"
	-@erase "$(INTDIR)\sendmail.res"
	-@erase "$(OUTDIR)\sendmail.dll"
	-@erase "$(OUTDIR)\sendmail.exp"
	-@erase "$(OUTDIR)\sendmail.lib"
	-@erase "$(OUTDIR)\sendmail.map"
	-@erase "$(OUTDIR)\sendmail.sym"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/sendmail.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Retail/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sendmail.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/sendmail.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /version:4.0 /entry:"DllMain" /subsystem:windows /dll /map /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /version:4.0\
 /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/sendmail.pdb" /map:"$(INTDIR)/sendmail.map" /machine:I386\
 /nodefaultlib /def:".\sendmail.def" /out:"$(OUTDIR)/sendmail.dll"\
 /implib:"$(OUTDIR)/sendmail.lib" 
DEF_FILE= \
	".\sendmail.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\dll.obj" \
	"$(INTDIR)\sendmail.obj" \
	"$(INTDIR)\sendmail.res"

"$(OUTDIR)\sendmail.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
TargetDir=.\Retail
TargetName=sendmail
InputPath=.\Retail\sendmail.dll
SOURCE=$(InputPath)

"$(TargetDir)\$(TargetName).sym" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   mapsym -nologo -o $(TargetDir)\$(TargetName).sym\
        $(TargetDir)\$(TargetName).map

# End Custom Build

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

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
# Begin Custom Macros
TargetDir=.\Debug
TargetName=sendmail
# End Custom Macros

ALL : "$(OUTDIR)\sendmail.dll" "$(OUTDIR)\sendmail.sym"

CLEAN : 
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\dll.obj"
	-@erase "$(INTDIR)\sendmail.obj"
	-@erase "$(INTDIR)\sendmail.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\sendmail.dll"
	-@erase "$(OUTDIR)\sendmail.exp"
	-@erase "$(OUTDIR)\sendmail.ilk"
	-@erase "$(OUTDIR)\sendmail.lib"
	-@erase "$(OUTDIR)\sendmail.map"
	-@erase "$(OUTDIR)\sendmail.pdb"
	-@erase "$(OUTDIR)\sendmail.sym"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/sendmail.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/sendmail.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/sendmail.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /version:4.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib uuid.lib comctl32.lib mpr.lib /nologo /version:4.0\
 /entry:"DllMain" /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/sendmail.pdb" /map:"$(INTDIR)/sendmail.map" /debug\
 /machine:I386 /nodefaultlib /def:".\sendmail.def" /out:"$(OUTDIR)/sendmail.dll"\
 /implib:"$(OUTDIR)/sendmail.lib" 
DEF_FILE= \
	".\sendmail.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\dll.obj" \
	"$(INTDIR)\sendmail.obj" \
	"$(INTDIR)\sendmail.res"

"$(OUTDIR)\sendmail.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
TargetDir=.\Debug
TargetName=sendmail
InputPath=.\Debug\sendmail.dll
SOURCE=$(InputPath)

"$(TargetDir)\$(TargetName).sym" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   mapsym -nologo -o $(TargetDir)\$(TargetName).sym\
        $(TargetDir)\$(TargetName).map

# End Custom Build

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

# Name "sendmail - Win32 Release"
# Name "sendmail - Win32 Debug"

!IF  "$(CFG)" == "sendmail - Win32 Release"

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\resource.h

!IF  "$(CFG)" == "sendmail - Win32 Release"

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\debug.c
DEP_CPP_DEBUG=\
	".\sendmail.h"\
	

"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dll.c
DEP_CPP_DLL_C=\
	".\sendmail.h"\
	

"$(INTDIR)\dll.obj" : $(SOURCE) $(DEP_CPP_DLL_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sendmail.rcv

!IF  "$(CFG)" == "sendmail - Win32 Release"

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sendmail.def

!IF  "$(CFG)" == "sendmail - Win32 Release"

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sendmail.h

!IF  "$(CFG)" == "sendmail - Win32 Release"

!ELSEIF  "$(CFG)" == "sendmail - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sendmail.rc
DEP_RSC_SENDM=\
	".\MAIL.ICO"\
	".\sendmail.rcv"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\ntverp.h"\
	

"$(INTDIR)\sendmail.res" : $(SOURCE) $(DEP_RSC_SENDM) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sendmail.c
DEP_CPP_SENDMA=\
	".\sendmail.h"\
	

"$(INTDIR)\sendmail.obj" : $(SOURCE) $(DEP_CPP_SENDMA) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
