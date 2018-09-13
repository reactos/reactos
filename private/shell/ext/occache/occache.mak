# Microsoft Developer Studio Generated NMAKE File, Based on occache.dsp
!IF "$(CFG)" == ""
CFG=occache - Win32 Debug
!MESSAGE No configuration specified. Defaulting to occache - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "occache - Win32 Release" && "$(CFG)" !=\
 "occache - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "occache.mak" CFG="occache - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "occache - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "occache - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "occache - Win32 Release"

OUTDIR=.\obj\i386
INTDIR=.\obj\i386
# Begin Custom Macros
OutDir=.\obj\i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\occache.dll"

!ELSE 

ALL : "$(OUTDIR)\occache.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cdlbsc.obj"
	-@erase "$(INTDIR)\cleanoc.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\emptyvol.obj"
	-@erase "$(INTDIR)\enum.obj"
	-@erase "$(INTDIR)\filenode.obj"
	-@erase "$(INTDIR)\folder.obj"
	-@erase "$(INTDIR)\general.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\item.obj"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\occache.res"
	-@erase "$(INTDIR)\parseinf.obj"
	-@erase "$(INTDIR)\persist.obj"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(OUTDIR)\occache.dll"
	-@erase "$(OUTDIR)\occache.exp"
	-@erase "$(OUTDIR)\occache.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\occache.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\obj\i386/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\occache.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\occache.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=int64.lib libcmt.lib gdi32.lib user32.lib kernel32.lib\
 advapi32.lib shlwapi.lib comctl32.lib shell32p.lib ole32.lib oleaut32.lib\
 version.lib uuid.lib urlmon.lib shguid.lib shguidp.lib stocklib.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\occache.pdb"\
 /machine:I386 /nodefaultlib /out:"$(OUTDIR)\occache.dll"\
 /implib:"$(OUTDIR)\occache.lib" 
LINK32_OBJS= \
	"$(INTDIR)\cdlbsc.obj" \
	"$(INTDIR)\cleanoc.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\emptyvol.obj" \
	"$(INTDIR)\enum.obj" \
	"$(INTDIR)\filenode.obj" \
	"$(INTDIR)\folder.obj" \
	"$(INTDIR)\general.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\item.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\occache.res" \
	"$(INTDIR)\parseinf.obj" \
	"$(INTDIR)\persist.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\occache.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

OUTDIR=.\objd/i386
INTDIR=.\objd/i386
# Begin Custom Macros
OutDir=.\objd/i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\occache.dll"

!ELSE 

ALL : "$(OUTDIR)\occache.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cdlbsc.obj"
	-@erase "$(INTDIR)\cleanoc.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\emptyvol.obj"
	-@erase "$(INTDIR)\enum.obj"
	-@erase "$(INTDIR)\filenode.obj"
	-@erase "$(INTDIR)\folder.obj"
	-@erase "$(INTDIR)\general.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\item.obj"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\occache.res"
	-@erase "$(INTDIR)\parseinf.obj"
	-@erase "$(INTDIR)\persist.obj"
	-@erase "$(INTDIR)\property.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(OUTDIR)\occache.dll"
	-@erase "$(OUTDIR)\occache.exp"
	-@erase "$(OUTDIR)\occache.ilk"
	-@erase "$(OUTDIR)\occache.lib"
	-@erase "$(OUTDIR)\occache.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\occache.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\objd/i386/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\occache.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\occache.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=int64.lib libcmt.lib gdi32.lib user32.lib kernel32.lib\
 advapi32.lib shlwapi.lib comctl32.lib shell32p.lib ole32.lib oleaut32.lib\
 version.lib uuid.lib urlmon.lib shguid.lib shguidp.lib stocklib.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\occache.pdb" /debug\
 /machine:I386 /nodefaultlib /out:"$(OUTDIR)\occache.dll"\
 /implib:"$(OUTDIR)\occache.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cdlbsc.obj" \
	"$(INTDIR)\cleanoc.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\emptyvol.obj" \
	"$(INTDIR)\enum.obj" \
	"$(INTDIR)\filenode.obj" \
	"$(INTDIR)\folder.obj" \
	"$(INTDIR)\general.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\item.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\occache.res" \
	"$(INTDIR)\parseinf.obj" \
	"$(INTDIR)\persist.obj" \
	"$(INTDIR)\property.obj" \
	"$(INTDIR)\utils.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\occache.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "occache - Win32 Release" || "$(CFG)" ==\
 "occache - Win32 Debug"
SOURCE=.\cdlbsc.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_CDLBS=\
	".\cdlbsc.hpp"\
	

"$(INTDIR)\cdlbsc.obj" : $(SOURCE) $(DEP_CPP_CDLBS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_CDLBS=\
	"..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\cdlbsc.hpp"\
	

"$(INTDIR)\cdlbsc.obj" : $(SOURCE) $(DEP_CPP_CDLBS) "$(INTDIR)"


!ENDIF 

SOURCE=.\cleanoc.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_CLEAN=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\filenode.h"\
	".\general.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\cleanoc.obj" : $(SOURCE) $(DEP_CPP_CLEAN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_CLEAN=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\filenode.h"\
	".\general.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\cleanoc.obj" : $(SOURCE) $(DEP_CPP_CLEAN) "$(INTDIR)"


!ENDIF 

SOURCE=.\debug.c

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_DEBUG=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_DEBUG=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


!ENDIF 

SOURCE=.\emptyvol.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_EMPTY=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\emptyvol.h"\
	".\init.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	{$(INCLUDE)}"volcache.h"\
	

"$(INTDIR)\emptyvol.obj" : $(SOURCE) $(DEP_CPP_EMPTY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_EMPTY=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\emptyvol.h"\
	".\init.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	{$(INCLUDE)}"volcache.h"\
	

"$(INTDIR)\emptyvol.obj" : $(SOURCE) $(DEP_CPP_EMPTY) "$(INTDIR)"


!ENDIF 

SOURCE=.\enum.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_ENUM_=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\enum.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\enum.obj" : $(SOURCE) $(DEP_CPP_ENUM_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_ENUM_=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\enum.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\enum.obj" : $(SOURCE) $(DEP_CPP_ENUM_) "$(INTDIR)"


!ENDIF 

SOURCE=.\filenode.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_FILEN=\
	".\filenode.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\filenode.obj" : $(SOURCE) $(DEP_CPP_FILEN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_FILEN=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\filenode.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\filenode.obj" : $(SOURCE) $(DEP_CPP_FILEN) "$(INTDIR)"


!ENDIF 

SOURCE=.\folder.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_FOLDE=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\folder.obj" : $(SOURCE) $(DEP_CPP_FOLDE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_FOLDE=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\folder.obj" : $(SOURCE) $(DEP_CPP_FOLDE) "$(INTDIR)"


!ENDIF 

SOURCE=.\general.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_GENER=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\general.obj" : $(SOURCE) $(DEP_CPP_GENER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_GENER=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\general.obj" : $(SOURCE) $(DEP_CPP_GENER) "$(INTDIR)"


!ENDIF 

SOURCE=.\init.c

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_INIT_=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_INIT_=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ENDIF 

SOURCE=.\item.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_ITEM_=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\cdlbsc.hpp"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"mstask.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	{$(INCLUDE)}"webcheck.h"\
	

"$(INTDIR)\item.obj" : $(SOURCE) $(DEP_CPP_ITEM_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_ITEM_=\
	"..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\cdlbsc.hpp"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"mstask.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	{$(INCLUDE)}"webcheck.h"\
	

"$(INTDIR)\item.obj" : $(SOURCE) $(DEP_CPP_ITEM_) "$(INTDIR)"


!ENDIF 

SOURCE=.\menu.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_MENU_=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\menu.obj" : $(SOURCE) $(DEP_CPP_MENU_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_MENU_=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\menu.obj" : $(SOURCE) $(DEP_CPP_MENU_) "$(INTDIR)"


!ENDIF 

SOURCE=.\occache.rc
DEP_RSC_OCCAC=\
	".\default.ico"\
	".\folder.ico"\
	".\occache.rcv"\
	".\selfreg_occache.inf"\
	{$(INCLUDE)}"common.ver"\
	{$(INCLUDE)}"ntverp.h"\
	

"$(INTDIR)\occache.res" : $(SOURCE) $(DEP_RSC_OCCAC) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\parseinf.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_PARSE=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\filenode.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgguid.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\parseinf.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_PARSE=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\filenode.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgguid.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\parseinf.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"


!ENDIF 

SOURCE=.\persist.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_PERSI=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\persist.obj" : $(SOURCE) $(DEP_CPP_PERSI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_PERSI=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\persist.obj" : $(SOURCE) $(DEP_CPP_PERSI) "$(INTDIR)"


!ENDIF 

SOURCE=.\property.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_PROPE=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\filenode.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"mstask.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\property.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_PROPE=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\filenode.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"mstask.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\property.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"


!ENDIF 

SOURCE=.\utils.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_UTILS=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\filenode.h"\
	".\general.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\utils.obj" : $(SOURCE) $(DEP_CPP_UTILS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_UTILS=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\filenode.h"\
	".\general.h"\
	".\init.h"\
	".\parseinf.h"\
	".\utils.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"pkgmgr.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\utils.obj" : $(SOURCE) $(DEP_CPP_UTILS) "$(INTDIR)"


!ENDIF 

SOURCE=.\view.cpp

!IF  "$(CFG)" == "occache - Win32 Release"

DEP_CPP_VIEW_=\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "occache - Win32 Debug"

DEP_CPP_VIEW_=\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\public\sdk\inc\shlwapi.h"\
	"..\..\..\windows\inc\comctrlp.h"\
	"..\..\..\windows\inc\commdlgp.h"\
	"..\..\..\windows\inc\prshtp.h"\
	"..\..\..\windows\inc\shlapip.h"\
	"..\..\..\windows\inc\shlguidp.h"\
	"..\..\..\windows\inc\winbasep.h"\
	"..\..\..\windows\inc\wingdip.h"\
	"..\..\..\windows\inc\winsprlp.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\folder.h"\
	".\general.h"\
	".\init.h"\
	".\item.h"\
	{$(INCLUDE)}"advpub.h"\
	{$(INCLUDE)}"ccstock.h"\
	{$(INCLUDE)}"cleanoc.h"\
	{$(INCLUDE)}"crtfree.h"\
	{$(INCLUDE)}"debug.h"\
	{$(INCLUDE)}"dvocx.h"\
	{$(INCLUDE)}"sfview.h"\
	{$(INCLUDE)}"shellp.h"\
	{$(INCLUDE)}"sherror.h"\
	{$(INCLUDE)}"shguidp.h"\
	{$(INCLUDE)}"shlobjp.h"\
	{$(INCLUDE)}"shlwapi.h"\
	{$(INCLUDE)}"shlwapip.h"\
	{$(INCLUDE)}"shsemip.h"\
	{$(INCLUDE)}"uastrfnc.h"\
	{$(INCLUDE)}"validate.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"


!ENDIF 


!ENDIF 

