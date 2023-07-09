# Microsoft Developer Studio Generated NMAKE File, Based on ee.bld
!IF "$(CFG)" == ""
CFG=ee - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ee - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ee - Win32 Release" && "$(CFG)" != "ee - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ee.bld" CFG="ee - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ee - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ee - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "ee - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\ee.dll"

CLEAN : 
	-@erase "$(INTDIR)\debapi.obj"
	-@erase "$(INTDIR)\debbind.obj"
	-@erase "$(INTDIR)\deberr.obj"
	-@erase "$(INTDIR)\debeval.obj"
	-@erase "$(INTDIR)\debfmt.obj"
	-@erase "$(INTDIR)\deblex.obj"
	-@erase "$(INTDIR)\deblexr.obj"
	-@erase "$(INTDIR)\debparse.obj"
	-@erase "$(INTDIR)\debsrch.obj"
	-@erase "$(INTDIR)\debsup.obj"
	-@erase "$(INTDIR)\debsym.obj"
	-@erase "$(INTDIR)\debtree.obj"
	-@erase "$(INTDIR)\debtyper.obj"
	-@erase "$(INTDIR)\debutil.obj"
	-@erase "$(INTDIR)\debwalk.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\eecxx.res"
	-@erase "$(INTDIR)\ldouble.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(OUTDIR)\ee.dll"
	-@erase "$(OUTDIR)\ee.exp"
	-@erase "$(OUTDIR)\ee.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/ee.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/eecxx.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ee.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/ee.pdb" /machine:I386 /def:".\eecxx.def"\
 /out:"$(OUTDIR)/ee.dll" /implib:"$(OUTDIR)/ee.lib" 
LINK32_OBJS= \
	"$(INTDIR)\debapi.obj" \
	"$(INTDIR)\debbind.obj" \
	"$(INTDIR)\deberr.obj" \
	"$(INTDIR)\debeval.obj" \
	"$(INTDIR)\debfmt.obj" \
	"$(INTDIR)\deblex.obj" \
	"$(INTDIR)\deblexr.obj" \
	"$(INTDIR)\debparse.obj" \
	"$(INTDIR)\debsrch.obj" \
	"$(INTDIR)\debsup.obj" \
	"$(INTDIR)\debsym.obj" \
	"$(INTDIR)\debtree.obj" \
	"$(INTDIR)\debtyper.obj" \
	"$(INTDIR)\debutil.obj" \
	"$(INTDIR)\debwalk.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\eecxx.res" \
	"$(INTDIR)\ldouble.obj"

"$(OUTDIR)\ee.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ee - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\ee.dll"

CLEAN : 
	-@erase "$(INTDIR)\debapi.obj"
	-@erase "$(INTDIR)\debbind.obj"
	-@erase "$(INTDIR)\deberr.obj"
	-@erase "$(INTDIR)\debeval.obj"
	-@erase "$(INTDIR)\debfmt.obj"
	-@erase "$(INTDIR)\deblex.obj"
	-@erase "$(INTDIR)\deblexr.obj"
	-@erase "$(INTDIR)\debparse.obj"
	-@erase "$(INTDIR)\debsrch.obj"
	-@erase "$(INTDIR)\debsup.obj"
	-@erase "$(INTDIR)\debsym.obj"
	-@erase "$(INTDIR)\debtree.obj"
	-@erase "$(INTDIR)\debtyper.obj"
	-@erase "$(INTDIR)\debutil.obj"
	-@erase "$(INTDIR)\debwalk.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\eecxx.res"
	-@erase "$(INTDIR)\ldouble.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\ee.dll"
	-@erase "$(OUTDIR)\ee.exp"
	-@erase "$(OUTDIR)\ee.ilk"
	-@erase "$(OUTDIR)\ee.lib"
	-@erase "$(OUTDIR)\ee.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W3 /WX /Gm- /GR /GX /Zi /Od /Gf /Gy /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/ee.pch" /Yu"debexpr.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/eecxx.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ee.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/ee.pdb" /debug /machine:I386 /def:".\eecxx.def"\
 /out:"$(OUTDIR)/ee.dll" /implib:"$(OUTDIR)/ee.lib" 
LINK32_OBJS= \
	"$(INTDIR)\debapi.obj" \
	"$(INTDIR)\debbind.obj" \
	"$(INTDIR)\deberr.obj" \
	"$(INTDIR)\debeval.obj" \
	"$(INTDIR)\debfmt.obj" \
	"$(INTDIR)\deblex.obj" \
	"$(INTDIR)\deblexr.obj" \
	"$(INTDIR)\debparse.obj" \
	"$(INTDIR)\debsrch.obj" \
	"$(INTDIR)\debsup.obj" \
	"$(INTDIR)\debsym.obj" \
	"$(INTDIR)\debtree.obj" \
	"$(INTDIR)\debtyper.obj" \
	"$(INTDIR)\debutil.obj" \
	"$(INTDIR)\debwalk.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\eecxx.res" \
	"$(INTDIR)\ldouble.obj"

"$(OUTDIR)\ee.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "ee - Win32 Release" || "$(CFG)" == "ee - Win32 Debug"

!IF  "$(CFG)" == "ee - Win32 Release"

!ELSEIF  "$(CFG)" == "ee - Win32 Debug"

!ENDIF 

SOURCE=.\ldouble.c

"$(INTDIR)\ldouble.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debbind.c

"$(INTDIR)\debbind.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\deberr.c

"$(INTDIR)\deberr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debeval.c

"$(INTDIR)\debeval.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debfmt.c

"$(INTDIR)\debfmt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\deblex.c

"$(INTDIR)\deblex.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\deblexr.c

"$(INTDIR)\deblexr.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debparse.c

"$(INTDIR)\debparse.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debsrch.c

"$(INTDIR)\debsrch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debsup.c

"$(INTDIR)\debsup.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debsym.c

"$(INTDIR)\debsym.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debtree.c

"$(INTDIR)\debtree.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debtyper.c

"$(INTDIR)\debtyper.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debutil.c

"$(INTDIR)\debutil.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debwalk.c

"$(INTDIR)\debwalk.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dllmain.c

"$(INTDIR)\dllmain.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\debapi.c

!IF  "$(CFG)" == "ee - Win32 Release"


"$(INTDIR)\debapi.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/ee.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c $(SOURCE)


!ELSEIF  "$(CFG)" == "ee - Win32 Debug"


"$(INTDIR)\debapi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ee.pch"
   $(CPP) /nologo /Gz /MTd /W3 /WX /Gm- /GR /GX /Zi /Od /Gf /Gy /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/ee.pch" /Yu"debexpr.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /FD /c $(SOURCE)


!ENDIF 

SOURCE=.\eecxx.def

!IF  "$(CFG)" == "ee - Win32 Release"

!ELSEIF  "$(CFG)" == "ee - Win32 Debug"

!ENDIF 

SOURCE=.\eecxx.rc

"$(INTDIR)\eecxx.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

