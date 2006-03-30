# Microsoft Developer Studio Generated NMAKE File, Based on Fraginator.dsp
!IF "$(CFG)" == ""
CFG=Fraginator - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Fraginator - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Fraginator - Win32 Release" && "$(CFG)" != "Fraginator - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Fraginator.mak" CFG="Fraginator - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Fraginator - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Fraginator - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Fraginator - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Fraginator.exe"


CLEAN :
	-@erase "$(INTDIR)\Defragment.obj"
	-@erase "$(INTDIR)\DriveVolume.obj"
	-@erase "$(INTDIR)\Fraginator.obj"
	-@erase "$(INTDIR)\Fraginator.res"
	-@erase "$(INTDIR)\MainDialog.obj"
	-@erase "$(INTDIR)\ReportDialog.obj"
	-@erase "$(INTDIR)\Unfrag.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\Fraginator.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G6 /Gr /MT /W3 /GX /Ox /Ot /Og /Oi /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\Fraginator.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Fraginator.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Fraginator.bsc" 
BSC32_SBRS= \
	
LINK32=xilink6.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /machine:I386 /out:"$(OUTDIR)\Fraginator.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Defragment.obj" \
	"$(INTDIR)\DriveVolume.obj" \
	"$(INTDIR)\Fraginator.obj" \
	"$(INTDIR)\MainDialog.obj" \
	"$(INTDIR)\ReportDialog.obj" \
	"$(INTDIR)\Unfrag.obj" \
	"$(INTDIR)\Fraginator.res"

"$(OUTDIR)\Fraginator.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\Fraginator.exe"
   copy Release\Fraginator.exe "c:\Program Files\Fraginator\Fraginator.exe"
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "Fraginator - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Fraginator.exe"


CLEAN :
	-@erase "$(INTDIR)\Defragment.obj"
	-@erase "$(INTDIR)\DriveVolume.obj"
	-@erase "$(INTDIR)\Fraginator.obj"
	-@erase "$(INTDIR)\Fraginator.res"
	-@erase "$(INTDIR)\MainDialog.obj"
	-@erase "$(INTDIR)\ReportDialog.obj"
	-@erase "$(INTDIR)\Unfrag.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\Fraginator.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\Fraginator.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Fraginator.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Fraginator.bsc" 
BSC32_SBRS= \
	
LINK32=xilink6.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /debug /machine:I386 /out:"$(OUTDIR)\Fraginator.exe" 
LINK32_OBJS= \
	"$(INTDIR)\Defragment.obj" \
	"$(INTDIR)\DriveVolume.obj" \
	"$(INTDIR)\Fraginator.obj" \
	"$(INTDIR)\MainDialog.obj" \
	"$(INTDIR)\ReportDialog.obj" \
	"$(INTDIR)\Unfrag.obj" \
	"$(INTDIR)\Fraginator.res"

"$(OUTDIR)\Fraginator.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("Fraginator.dep")
!INCLUDE "Fraginator.dep"
!ELSE 
!MESSAGE Warning: cannot find "Fraginator.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Fraginator - Win32 Release" || "$(CFG)" == "Fraginator - Win32 Debug"
SOURCE=.\Defragment.cpp

"$(INTDIR)\Defragment.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DriveVolume.cpp

"$(INTDIR)\DriveVolume.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Fraginator.cpp

"$(INTDIR)\Fraginator.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MainDialog.cpp

"$(INTDIR)\MainDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ReportDialog.cpp

"$(INTDIR)\ReportDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Unfrag.cpp

"$(INTDIR)\Unfrag.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Fraginator.rc

"$(INTDIR)\Fraginator.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

