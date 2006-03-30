# Microsoft Developer Studio Generated NMAKE File, Based on unfrag.dsp
!IF "$(CFG)" == ""
CFG=unfrag - Win32 Debug
!MESSAGE No configuration specified. Defaulting to unfrag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "unfrag - Win32 Release" && "$(CFG)" != "unfrag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unfrag.mak" CFG="unfrag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unfrag - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unfrag - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "unfrag - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\unfrag.exe"


CLEAN :
	-@erase "$(INTDIR)\Defragment.obj"
	-@erase "$(INTDIR)\DriveVolume.obj"
	-@erase "$(INTDIR)\Unfrag.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\unfrag.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G6 /Gr /MD /W3 /GX /Ox /Ot /Og /Oi /Ob2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\unfrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\unfrag.bsc" 
BSC32_SBRS= \
	
LINK32=xilink6.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\unfrag.pdb" /machine:I386 /out:"$(OUTDIR)\unfrag.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Defragment.obj" \
	"$(INTDIR)\DriveVolume.obj" \
	"$(INTDIR)\Unfrag.obj"

"$(OUTDIR)\unfrag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=Copying to Program Files ...
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\unfrag.exe"
   copy Release\unfrag.exe "c:\Program Files\Fraginator\unfrag.exe"
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "unfrag - Win32 Debug"

OUTDIR=.\unfrag___Win32_Debug
INTDIR=.\unfrag___Win32_Debug
# Begin Custom Macros
OutDir=.\unfrag___Win32_Debug
# End Custom Macros

ALL : "$(OUTDIR)\unfrag.exe"


CLEAN :
	-@erase "$(INTDIR)\Defragment.obj"
	-@erase "$(INTDIR)\DriveVolume.obj"
	-@erase "$(INTDIR)\Unfrag.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\unfrag.exe"
	-@erase "$(OUTDIR)\unfrag.ilk"
	-@erase "$(OUTDIR)\unfrag.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\unfrag.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\unfrag.bsc" 
BSC32_SBRS= \
	
LINK32=xilink6.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\unfrag.pdb" /debug /machine:I386 /out:"$(OUTDIR)\unfrag.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Defragment.obj" \
	"$(INTDIR)\DriveVolume.obj" \
	"$(INTDIR)\Unfrag.obj"

"$(OUTDIR)\unfrag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("unfrag.dep")
!INCLUDE "unfrag.dep"
!ELSE 
!MESSAGE Warning: cannot find "unfrag.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "unfrag - Win32 Release" || "$(CFG)" == "unfrag - Win32 Debug"
SOURCE=.\Defragment.cpp

"$(INTDIR)\Defragment.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DriveVolume.cpp

"$(INTDIR)\DriveVolume.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Unfrag.cpp

"$(INTDIR)\Unfrag.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

