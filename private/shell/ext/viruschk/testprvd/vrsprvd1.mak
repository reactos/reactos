# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=vrsprvd1 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to vrsprvd1 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vrsprvd1 - Win32 Release" && "$(CFG)" !=\
 "vrsprvd1 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vrsprvd1.mak" CFG="vrsprvd1 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vrsprvd1 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vrsprvd1 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "vrsprvd1 - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

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

ALL : ".\Release\vrsprvd1.dll"

CLEAN : 
	-@erase ".\Release\provfct.obj"
	-@erase ".\Release\provmn.obj"
	-@erase ".\Release\provpch.obj"
	-@erase ".\Release\util.obj"
	-@erase ".\Release\vguids.obj"
	-@erase ".\Release\vrsprov.obj"
	-@erase ".\Release\vrsprov.res"
	-@erase ".\Release\vrsprvd1.dll"
	-@erase ".\Release\vrsprvd1.exp"
	-@erase ".\Release\vrsprvd1.lib"
	-@erase ".\Release\vrsprvd1.pch"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vrsprov.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vrsprvd1.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libc.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libc.lib /nologo\
 /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/vrsprvd1.pdb" /machine:I386 /nodefaultlib /def:".\vrsprvd1.def"\
 /out:"$(OUTDIR)/vrsprvd1.dll" /implib:"$(OUTDIR)/vrsprvd1.lib" 
DEF_FILE= \
	".\vrsprvd1.def"
LINK32_OBJS= \
	".\Release\provfct.obj" \
	".\Release\provmn.obj" \
	".\Release\provpch.obj" \
	".\Release\util.obj" \
	".\Release\vguids.obj" \
	".\Release\vrsprov.obj" \
	".\Release\vrsprov.res"

".\Release\vrsprvd1.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

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

ALL : ".\Debug\vrsprvd1.dll"

CLEAN : 
	-@erase ".\Debug\provfct.obj"
	-@erase ".\Debug\provmn.obj"
	-@erase ".\Debug\provpch.obj"
	-@erase ".\Debug\util.obj"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vguids.obj"
	-@erase ".\Debug\vrsprov.obj"
	-@erase ".\Debug\vrsprov.res"
	-@erase ".\Debug\vrsprvd1.dll"
	-@erase ".\Debug\vrsprvd1.exp"
	-@erase ".\Debug\vrsprvd1.ilk"
	-@erase ".\Debug\vrsprvd1.lib"
	-@erase ".\Debug\vrsprvd1.pch"
	-@erase ".\Debug\vrsprvd1.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vrsprov.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vrsprvd1.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libc.lib /nologo /entry:"DllMain" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libc.lib /nologo\
 /entry:"DllMain" /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/vrsprvd1.pdb" /debug /machine:I386 /nodefaultlib\
 /def:".\vrsprvd1.def" /out:"$(OUTDIR)/vrsprvd1.dll"\
 /implib:"$(OUTDIR)/vrsprvd1.lib" 
DEF_FILE= \
	".\vrsprvd1.def"
LINK32_OBJS= \
	".\Debug\provfct.obj" \
	".\Debug\provmn.obj" \
	".\Debug\provpch.obj" \
	".\Debug\util.obj" \
	".\Debug\vguids.obj" \
	".\Debug\vrsprov.obj" \
	".\Debug\vrsprov.res"

".\Debug\vrsprvd1.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "vrsprvd1 - Win32 Release"
# Name "vrsprvd1 - Win32 Debug"

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\vrsprov.rc

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"


".\Release\vrsprov.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"


".\Debug\vrsprov.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\provmn.cpp
DEP_CPP_PROVM=\
	".\provfct.h"\
	".\provmn.h"\
	".\provpch.h"\
	".\util.h"\
	".\vrsprov.h"\
	{$(INCLUDE)}"\vrsscan.h"\
	

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

# ADD CPP /Yu"provpch.h"

".\Release\provmn.obj" : $(SOURCE) $(DEP_CPP_PROVM) "$(INTDIR)"\
 ".\Release\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

# ADD CPP /Yu"provpch.h"

".\Debug\provmn.obj" : $(SOURCE) $(DEP_CPP_PROVM) "$(INTDIR)"\
 ".\Debug\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\provpch.cpp
DEP_CPP_PROVP=\
	".\provpch.h"\
	

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

# ADD CPP /Yc"provpch.h"

BuildCmds= \
	$(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yc"provpch.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

".\Release\provpch.obj" : $(SOURCE) $(DEP_CPP_PROVP) "$(INTDIR)"
   $(BuildCmds)

".\Release\vrsprvd1.pch" : $(SOURCE) $(DEP_CPP_PROVP) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

# ADD CPP /Yc"provpch.h"

BuildCmds= \
	$(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yc"provpch.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

".\Debug\provpch.obj" : $(SOURCE) $(DEP_CPP_PROVP) "$(INTDIR)"
   $(BuildCmds)

".\Debug\vrsprvd1.pch" : $(SOURCE) $(DEP_CPP_PROVP) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

DEP_CPP_UTIL_=\
	".\provpch.h"\
	".\util.h"\
	
# ADD CPP /Yu"provpch.h"

".\Release\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 ".\Release\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

DEP_CPP_UTIL_=\
	".\provpch.h"\
	".\util.h"\
	
NODEP_CPP_UTIL_=\
	".\void CopyWideStr(LPWSTR pwszTarget, LPWSTR pwszSource)"\
	
# ADD CPP /Yu"provpch.h"

".\Debug\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 ".\Debug\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vguids.cpp
DEP_CPP_VGUID=\
	".\vrsprov.h"\
	{$(INCLUDE)}"\vrsscan.h"\
	

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

# SUBTRACT CPP /YX

".\Release\vguids.obj" : $(SOURCE) $(DEP_CPP_VGUID) "$(INTDIR)"
   $(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

# SUBTRACT CPP /YX

".\Debug\vguids.obj" : $(SOURCE) $(DEP_CPP_VGUID) "$(INTDIR)"
   $(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vrsprov.cpp
DEP_CPP_VRSPR=\
	".\provmn.h"\
	".\provpch.h"\
	".\util.h"\
	".\vrsprov.h"\
	{$(INCLUDE)}"\vrsscan.h"\
	

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

# ADD CPP /Yu"provpch.h"

".\Release\vrsprov.obj" : $(SOURCE) $(DEP_CPP_VRSPR) "$(INTDIR)"\
 ".\Release\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

# ADD CPP /Yu"provpch.h"

".\Debug\vrsprov.obj" : $(SOURCE) $(DEP_CPP_VRSPR) "$(INTDIR)"\
 ".\Debug\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\provfct.cpp
DEP_CPP_PROVF=\
	".\provfct.h"\
	".\provmn.h"\
	".\provpch.h"\
	".\vrsprov.h"\
	{$(INCLUDE)}"\vrsscan.h"\
	

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

# ADD CPP /Yu"provpch.h"

".\Release\provfct.obj" : $(SOURCE) $(DEP_CPP_PROVF) "$(INTDIR)"\
 ".\Release\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

# ADD CPP /Yu"provpch.h"

".\Debug\provfct.obj" : $(SOURCE) $(DEP_CPP_PROVF) "$(INTDIR)"\
 ".\Debug\vrsprvd1.pch"
   $(CPP) /nologo /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/vrsprvd1.pch" /Yu"provpch.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vrsprvd1.def

!IF  "$(CFG)" == "vrsprvd1 - Win32 Release"

!ELSEIF  "$(CFG)" == "vrsprvd1 - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
