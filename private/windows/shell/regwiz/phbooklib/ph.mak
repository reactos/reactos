# Microsoft Developer Studio Generated NMAKE File, Based on PhbookLib.dsp
!IF "$(CFG)" == ""
CFG=PhbookLib - Win32 Debug
!MESSAGE No configuration specified. Defaulting to PhbookLib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "PhbookLib - Win32 Release" && "$(CFG)" !=\
 "PhbookLib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PhbookLib.mak" CFG="PhbookLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PhbookLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "PhbookLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "PhbookLib - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\PhbookLib.lib"

!ELSE 

ALL : "$(OUTDIR)\PhbookLib.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Ccsv.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\Main.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Phbk.obj"
	-@erase "$(INTDIR)\Rnaapi.obj"
	-@erase "$(INTDIR)\Suapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\PhbookLib.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\PhbookLib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\PhbookLib.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\PhbookLib.lib" 
LIB32_OBJS= \
	"$(INTDIR)\Ccsv.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\Main.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Phbk.obj" \
	"$(INTDIR)\Rnaapi.obj" \
	"$(INTDIR)\Suapi.obj"

"$(OUTDIR)\PhbookLib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "PhbookLib - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\PhbookLib.lib"

!ELSE 

ALL : "$(OUTDIR)\PhbookLib.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Ccsv.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\Main.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Phbk.obj"
	-@erase "$(INTDIR)\Rnaapi.obj"
	-@erase "$(INTDIR)\Suapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\PhbookLib.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\PhbookLib.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\PhbookLib.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\PhbookLib.lib" 
LIB32_OBJS= \
	"$(INTDIR)\Ccsv.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\Main.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Phbk.obj" \
	"$(INTDIR)\Rnaapi.obj" \
	"$(INTDIR)\Suapi.obj"

"$(OUTDIR)\PhbookLib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "PhbookLib - Win32 Release" || "$(CFG)" ==\
 "PhbookLib - Win32 Debug"
SOURCE=.\Ccsv.cpp
DEP_CPP_CCSV_=\
	".\ccsv.h"\
	".\debug.h"\
	".\pch.hpp"\
	

"$(INTDIR)\Ccsv.obj" : $(SOURCE) $(DEP_CPP_CCSV_) "$(INTDIR)"


SOURCE=.\Debug.cpp
DEP_CPP_DEBUG=\
	".\pch.hpp"\
	

"$(INTDIR)\Debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


SOURCE=.\Main.cpp
DEP_CPP_MAIN_=\
	".\pch.hpp"\
	

"$(INTDIR)\Main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


SOURCE=.\Misc.cpp
DEP_CPP_MISC_=\
	".\ccsv.h"\
	".\debug.h"\
	".\pch.hpp"\
	".\phbk.h"\
	

"$(INTDIR)\Misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


SOURCE=.\Phbk.cpp
DEP_CPP_PHBK_=\
	".\ccsv.h"\
	".\debug.h"\
	".\misc.h"\
	".\pch.hpp"\
	".\phbk.h"\
	".\suapi.h"\
	
NODEP_CPP_PHBK_=\
	".\bmp.h"\
	

"$(INTDIR)\Phbk.obj" : $(SOURCE) $(DEP_CPP_PHBK_) "$(INTDIR)"


SOURCE=.\Rnaapi.cpp
DEP_CPP_RNAAP=\
	".\debug.h"\
	".\pch.hpp"\
	".\rnaapi.h"\
	

"$(INTDIR)\Rnaapi.obj" : $(SOURCE) $(DEP_CPP_RNAAP) "$(INTDIR)"


SOURCE=.\Suapi.cpp
DEP_CPP_SUAPI=\
	".\ccsv.h"\
	".\debug.h"\
	".\misc.h"\
	".\pch.hpp"\
	".\phbk.h"\
	".\suapi.h"\
	

"$(INTDIR)\Suapi.obj" : $(SOURCE) $(DEP_CPP_SUAPI) "$(INTDIR)"



!ENDIF 

