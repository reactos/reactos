# Microsoft Developer Studio Generated NMAKE File, Based on dm.bld
!IF "$(CFG)" == ""
CFG=dm - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to dm - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "dm - Win32 Release" && "$(CFG)" != "dm - Win32 Debug" &&\
 "$(CFG)" != "dm - Power Macintosh Release" && "$(CFG)" !=\
 "dm - Power Macintosh Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "dm.bld" CFG="dm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dm - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dm - Power Macintosh Release" (based on\
 "Power Macintosh Dynamic-Link Library")
!MESSAGE "dm - Power Macintosh Debug" (based on\
 "Power Macintosh Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "dm - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\dm.dll"

CLEAN : 
	-@erase "$(INTDIR)\bp.obj"
	-@erase "$(INTDIR)\dm.res"
	-@erase "$(INTDIR)\dmdisasm.obj"
	-@erase "$(INTDIR)\dmole.obj"
	-@erase "$(INTDIR)\dmx32.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\funccall.obj"
	-@erase "$(INTDIR)\i386mach.obj"
	-@erase "$(INTDIR)\i386thnk.obj"
	-@erase "$(INTDIR)\procem.obj"
	-@erase "$(INTDIR)\process.obj"
	-@erase "$(INTDIR)\step.obj"
	-@erase "$(INTDIR)\userapi.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\walk.obj"
	-@erase "$(INTDIR)\wow.obj"
	-@erase "$(OUTDIR)\dm.dll"
	-@erase "$(OUTDIR)\dm.exp"
	-@erase "$(OUTDIR)\dm.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

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

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dm.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/dm.pdb" /machine:I386 /def:".\user\dm.def"\
 /out:"$(OUTDIR)/dm.dll" /implib:"$(OUTDIR)/dm.lib" 
LINK32_OBJS= \
	"$(INTDIR)\bp.obj" \
	"$(INTDIR)\dm.res" \
	"$(INTDIR)\dmdisasm.obj" \
	"$(INTDIR)\dmole.obj" \
	"$(INTDIR)\dmx32.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\funccall.obj" \
	"$(INTDIR)\i386mach.obj" \
	"$(INTDIR)\i386thnk.obj" \
	"$(INTDIR)\procem.obj" \
	"$(INTDIR)\process.obj" \
	"$(INTDIR)\step.obj" \
	"$(INTDIR)\userapi.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\walk.obj" \
	"$(INTDIR)\wow.obj"

"$(OUTDIR)\dm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "dm - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bind\dmd.dll"

CLEAN : 
	-@erase "$(INTDIR)\bp.obj"
	-@erase "$(INTDIR)\dm.res"
	-@erase "$(INTDIR)\dmdisasm.obj"
	-@erase "$(INTDIR)\dmole.obj"
	-@erase "$(INTDIR)\dmx32.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\funccall.obj"
	-@erase "$(INTDIR)\i386mach.obj"
	-@erase "$(INTDIR)\i386thnk.obj"
	-@erase "$(INTDIR)\procem.obj"
	-@erase "$(INTDIR)\process.obj"
	-@erase "$(INTDIR)\step.obj"
	-@erase "$(INTDIR)\userapi.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\walk.obj"
	-@erase "$(INTDIR)\wow.obj"
	-@erase "$(OUTDIR)\dmd.exp"
	-@erase "$(OUTDIR)\dmd.lib"
	-@erase "..\..\bind\dmd.dll"
	-@erase "..\..\bind\dmd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /GR /GX /Od /Gf /Gy /I ".\" /I "..\include" /I\
 "..\od\include" /I "..\..\..\langapi\debugger\osdebug4" /I\
 "..\..\..\langapi\debugger" /I "..\..\..\langapi\include" /I\
 "..\..\..\public\nonship\inc" /D _DBCS=1 /D "_DEBUG" /D "_DLL" /D "_MBCS" /D\
 _MT=1 /D _NT1X_=100 /D "_NTWIN" /D "_SUSHI" /D "_TEST" /D _WIN32_WINNT=0x0400\
 /D "_WINDOWS" /D _X86_=1 /D "ADDR_MIXED" /D "CODEVIEW" /D CONDITION_HANDLING=1\
 /D "CROSS_PLATFORM" /D DBG=1 /D DEBUG=1 /D DEVL=1 /D "DOLPHIN" /D FPO=0 /D\
 "HOST32" /D "HOST_i386" /D i386=1 /D "NEW_PROJ_VIEW" /D "NT_BUILD" /D NT_INST=0\
 /D NT_UP=1 /D "OSDEBUG4" /D "PPC_PLATFORM" /D "SHIP" /D "STD_CALL" /D "STRICT"\
 /D "TARGET32" /D WIN32=100 /D WINNT=1 /D "x86" /D "BORDER_BUTTONS" /D\
 DWORDLONG=ULONGLONG /D "TARGET_i386" /D "WOW" /Fp"$(INTDIR)/dm.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /Bd /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

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

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dm.res" /i "..\include" /i "..\od\include" /i\
 "..\..\..\langapi\debugger\osdebug4" /i "..\..\..\langapi\debugger" /i\
 "..\..\..\langapi\include" /i "..\..\..\public\nonship\inc" /d _DBCS=1 /d\
 _DLL=1 /d "_MBCS" /d _MT=1 /d _NT1X_=100 /d "_NTWIN" /d "_SUSHI" /d "_TEST" /d\
 _WIN32_WINNT=0x0400 /d "_WINDOWS" /d _X86_=1 /d "ADDR_MIXED" /d "CODEVIEW" /d\
 CONDITION_HANDLING=1 /d "CROSS_PLATFORM" /d DBG=1 /d DEBUG=1 /d DEVL=1 /d\
 "DOLPHIN" /d FPO=0 /d "HOST32" /d i386=1 /d "NEW_PROJ_VIEW" /d "NT_BUILD" /d\
 NT_INST=0 /d NT_UP=1 /d "OSDEBUG4" /d "PPC_PLATFORM" /d "SHIP" /d "STD_CALL" /d\
 "STRICT" /d "TARGET32" /d WIN32=100 /d WINNT=1 /d "BORDER_BUTTONS" /d\
 DWORDLONG=ULONGLONG 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib ..\..\..\public\nonship\lib\i386\crashlib.lib\
 ..\..\..\public\nonship\lib\i386\disasm.lib kernel32.lib user32.lib vdmdbg.lib\
 advapi32.lib /nologo /version:5.0 /entry:"DllMain@12" /subsystem:windows /dll\
 /incremental:no /pdb:"..\..\bind/dmd.pdb" /debug /machine:I386 /nodefaultlib\
 /def:".\user\dm.def" /force /out:"..\..\bind/dmd.dll"\
 /implib:"$(OUTDIR)/dmd.lib" /merge:.rdata=.text /merge:_PAGE=PAGE\
 /merge:_TEXT=.text /opt:REF /section:INIT,d 
LINK32_OBJS= \
	"$(INTDIR)\bp.obj" \
	"$(INTDIR)\dm.res" \
	"$(INTDIR)\dmdisasm.obj" \
	"$(INTDIR)\dmole.obj" \
	"$(INTDIR)\dmx32.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\funccall.obj" \
	"$(INTDIR)\i386mach.obj" \
	"$(INTDIR)\i386thnk.obj" \
	"$(INTDIR)\procem.obj" \
	"$(INTDIR)\process.obj" \
	"$(INTDIR)\step.obj" \
	"$(INTDIR)\userapi.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\walk.obj" \
	"$(INTDIR)\wow.obj"

"..\..\bind\dmd.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

OUTDIR=.\PMacRel
INTDIR=.\PMacRel

ALL : "$(OUTDIR)\dm.dll" "$(OUTDIR)\dm.trg"

CLEAN : 
	-@erase "$(INTDIR)\bp.obj"
	-@erase "$(INTDIR)\dm.rsc"
	-@erase "$(INTDIR)\dmdisasm.obj"
	-@erase "$(INTDIR)\dmole.obj"
	-@erase "$(INTDIR)\dmx32.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\funccall.obj"
	-@erase "$(INTDIR)\i386mach.obj"
	-@erase "$(INTDIR)\i386thnk.obj"
	-@erase "$(INTDIR)\procem.obj"
	-@erase "$(INTDIR)\process.obj"
	-@erase "$(INTDIR)\step.obj"
	-@erase "$(INTDIR)\userapi.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\walk.obj"
	-@erase "$(INTDIR)\wow.obj"
	-@erase "$(OUTDIR)\dm.dll"
	-@erase "$(OUTDIR)\dm.exp"
	-@erase "$(OUTDIR)\dm.lib"
	-@erase "$(OUTDIR)\dm.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "NDEBUG" /ppc 
CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D "WIN32"\
 /D "NDEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\PMacRel/
CPP_SBRS=.\.

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /r /m /fo"$(INTDIR)/dm.rsc" /d "_MAC" /d "_MPPC_" /d "NDEBUG"\
 
MRC=mrc.exe
MRC_PROJ=/D "_MPPC_" /D "_MAC" /D "NDEBUG" /l 0x409 /NOLOGO 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /mac:nobundle /mac:type="shlb" /mac:creator="cfmg"\
 /mac:init="WlmConnectionInit" /mac:term="WlmConnectionTerm" /dll\
 /incremental:no /pdb:"$(OUTDIR)/dm.pdb" /machine:MPPC /def:".\user\dm.def"\
 /out:"$(OUTDIR)/dm.dll" /implib:"$(OUTDIR)/dm.lib" 
LINK32_OBJS= \
	"$(INTDIR)\bp.obj" \
	"$(INTDIR)\dm.rsc" \
	"$(INTDIR)\dmdisasm.obj" \
	"$(INTDIR)\dmole.obj" \
	"$(INTDIR)\dmx32.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\funccall.obj" \
	"$(INTDIR)\i386mach.obj" \
	"$(INTDIR)\i386thnk.obj" \
	"$(INTDIR)\procem.obj" \
	"$(INTDIR)\process.obj" \
	"$(INTDIR)\step.obj" \
	"$(INTDIR)\userapi.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\walk.obj" \
	"$(INTDIR)\wow.obj"

"$(OUTDIR)\dm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

MFILE32=mfile.exe
MFILE32_FLAGS=COPY /NOLOGO 
MFILE32_FILES= \
	"$(OUTDIR)\dm.dll"

"$(OUTDIR)\dm.trg" : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacRel\dm.dll\
 "$(MFILE32_DEST):dm.dll">"$(OUTDIR)\dm.trg"

DOWNLOAD : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacRel\dm.dll\
 "$(MFILE32_DEST):dm.dll">"$(OUTDIR)\dm.trg"

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

OUTDIR=.\PMacDbg
INTDIR=.\PMacDbg

ALL : "$(OUTDIR)\dm.dll" "$(OUTDIR)\dm.trg"

CLEAN : 
	-@erase "$(INTDIR)\bp.obj"
	-@erase "$(INTDIR)\dm.rsc"
	-@erase "$(INTDIR)\dmdisasm.obj"
	-@erase "$(INTDIR)\dmole.obj"
	-@erase "$(INTDIR)\dmx32.obj"
	-@erase "$(INTDIR)\event.obj"
	-@erase "$(INTDIR)\funccall.obj"
	-@erase "$(INTDIR)\i386mach.obj"
	-@erase "$(INTDIR)\i386thnk.obj"
	-@erase "$(INTDIR)\procem.obj"
	-@erase "$(INTDIR)\process.obj"
	-@erase "$(INTDIR)\step.obj"
	-@erase "$(INTDIR)\userapi.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\walk.obj"
	-@erase "$(INTDIR)\wow.obj"
	-@erase "$(OUTDIR)\dm.dll"
	-@erase "$(OUTDIR)\dm.exp"
	-@erase "$(OUTDIR)\dm.ilk"
	-@erase "$(OUTDIR)\dm.lib"
	-@erase "$(OUTDIR)\dm.pdb"
	-@erase "$(OUTDIR)\dm.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "_DEBUG" /ppc 
CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D\
 "WIN32" /D "_DEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /QPm /c 
CPP_OBJS=.\PMacDbg/
CPP_SBRS=.\.

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /r /m /fo"$(INTDIR)/dm.rsc" /d "_MAC" /d "_MPPC_" /d "_DEBUG"\
 
MRC=mrc.exe
MRC_PROJ=/D "_MPPC_" /D "_MAC" /D "_DEBUG" /l 0x409 /NOLOGO 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /mac:nobundle /mac:type="shlb" /mac:creator="cfmg"\
 /mac:init="WlmConnectionInit" /mac:term="WlmConnectionTerm" /dll\
 /incremental:yes /pdb:"$(OUTDIR)/dm.pdb" /debug /machine:MPPC\
 /def:".\user\dm.def" /out:"$(OUTDIR)/dm.dll" /implib:"$(OUTDIR)/dm.lib" 
LINK32_OBJS= \
	"$(INTDIR)\bp.obj" \
	"$(INTDIR)\dm.rsc" \
	"$(INTDIR)\dmdisasm.obj" \
	"$(INTDIR)\dmole.obj" \
	"$(INTDIR)\dmx32.obj" \
	"$(INTDIR)\event.obj" \
	"$(INTDIR)\funccall.obj" \
	"$(INTDIR)\i386mach.obj" \
	"$(INTDIR)\i386thnk.obj" \
	"$(INTDIR)\procem.obj" \
	"$(INTDIR)\process.obj" \
	"$(INTDIR)\step.obj" \
	"$(INTDIR)\userapi.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\walk.obj" \
	"$(INTDIR)\wow.obj"

"$(OUTDIR)\dm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

MFILE32=mfile.exe
MFILE32_FLAGS=COPY /NOLOGO 
MFILE32_FILES= \
	"$(OUTDIR)\dm.dll"

"$(OUTDIR)\dm.trg" : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacDbg\dm.dll\
 "$(MFILE32_DEST):dm.dll">"$(OUTDIR)\dm.trg"

DOWNLOAD : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacDbg\dm.dll\
 "$(MFILE32_DEST):dm.dll">"$(OUTDIR)\dm.trg"

!ENDIF 


!IF "$(CFG)" == "dm - Win32 Release" || "$(CFG)" == "dm - Win32 Debug" ||\
 "$(CFG)" == "dm - Power Macintosh Release" || "$(CFG)" ==\
 "dm - Power Macintosh Debug"

!IF  "$(CFG)" == "dm - Win32 Release"

!ELSEIF  "$(CFG)" == "dm - Win32 Debug"

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

!ENDIF 

SOURCE=.\user\wow.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\wow.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\wow.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

DEP_CPP_WOW_C=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_WOW_C=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\wow.obj" : $(SOURCE) $(DEP_CPP_WOW_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

DEP_CPP_WOW_C=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_WOW_C=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\wow.obj" : $(SOURCE) $(DEP_CPP_WOW_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\user\process.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\process.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\process.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

DEP_CPP_PROCE=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_PROCE=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\process.obj" : $(SOURCE) $(DEP_CPP_PROCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

DEP_CPP_PROCE=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_PROCE=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\process.obj" : $(SOURCE) $(DEP_CPP_PROCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\user\userapi.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\userapi.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\userapi.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

DEP_CPP_USERA=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_USERA=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\userapi.obj" : $(SOURCE) $(DEP_CPP_USERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

DEP_CPP_USERA=\
	".\user\dmole.h"\
	".\user\precomp.h"\
	
NODEP_CPP_USERA=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	

"$(INTDIR)\userapi.obj" : $(SOURCE) $(DEP_CPP_USERA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\user\dmole.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\dmole.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\dmole.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

DEP_CPP_DMOLE=\
	".\user\dmole.h"\
	".\user\glue.h"\
	".\user\precomp.h"\
	
NODEP_CPP_DMOLE=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	".\user\orpc_dbg.h"\
	

"$(INTDIR)\dmole.obj" : $(SOURCE) $(DEP_CPP_DMOLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

DEP_CPP_DMOLE=\
	".\user\dmole.h"\
	".\user\glue.h"\
	".\user\precomp.h"\
	
NODEP_CPP_DMOLE=\
	".\user\bp.h"\
	".\user\ctxptrs.h"\
	".\user\dbgver.h"\
	".\user\debug.h"\
	".\user\dm.h"\
	".\user\funccall.h"\
	".\user\list.h"\
	".\user\orpc_dbg.h"\
	

"$(INTDIR)\dmole.obj" : $(SOURCE) $(DEP_CPP_DMOLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\walk.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\walk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\walk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_WALK_=\
	".\precomp.h"\
	

"$(INTDIR)\walk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_WALK_=\
	".\precomp.h"\
	

"$(INTDIR)\walk.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\dmdisasm.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\dmdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\dmdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_DMDIS=\
	".\precomp.h"\
	

"$(INTDIR)\dmdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_DMDIS=\
	".\precomp.h"\
	

"$(INTDIR)\dmdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\dmx32.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\dmx32.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\dmx32.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

DEP_CPP_DMX32=\
	".\resrc1.h"\
	
NODEP_CPP_DMX32=\
	".\precomp.h"\
	

"$(INTDIR)\dmx32.obj" : $(SOURCE) $(DEP_CPP_DMX32) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

DEP_CPP_DMX32=\
	".\resrc1.h"\
	
NODEP_CPP_DMX32=\
	".\precomp.h"\
	

"$(INTDIR)\dmx32.obj" : $(SOURCE) $(DEP_CPP_DMX32) "$(INTDIR)"


!ENDIF 

SOURCE=.\event.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\event.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\event.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_EVENT=\
	".\precomp.h"\
	

"$(INTDIR)\event.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_EVENT=\
	".\precomp.h"\
	

"$(INTDIR)\event.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\funccall.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\funccall.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\funccall.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_FUNCC=\
	".\precomp.h"\
	

"$(INTDIR)\funccall.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_FUNCC=\
	".\precomp.h"\
	

"$(INTDIR)\funccall.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\i386mach.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\i386mach.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\i386mach.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_I386M=\
	".\precomp.h"\
	

"$(INTDIR)\i386mach.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_I386M=\
	".\precomp.h"\
	

"$(INTDIR)\i386mach.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\i386thnk.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\i386thnk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\i386thnk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_I386T=\
	".\precomp.h"\
	

"$(INTDIR)\i386thnk.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_I386T=\
	".\precomp.h"\
	

"$(INTDIR)\i386thnk.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\procem.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\procem.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\procem.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_PROCEM=\
	".\precomp.h"\
	

"$(INTDIR)\procem.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_PROCEM=\
	".\precomp.h"\
	

"$(INTDIR)\procem.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\step.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\step.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\step.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_STEP_=\
	".\precomp.h"\
	

"$(INTDIR)\step.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_STEP_=\
	".\precomp.h"\
	

"$(INTDIR)\step.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\util.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_UTIL_=\
	".\precomp.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_UTIL_=\
	".\precomp.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\bp.c

!IF  "$(CFG)" == "dm - Win32 Release"


"$(INTDIR)\bp.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"


"$(INTDIR)\bp.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) /nologo /Gz /MTd /W3 /GR /GX /Od /Gf /Gy /I ".\" /I "..\include" /I\
 "..\od\include" /I "..\..\..\langapi\debugger\osdebug4" /I\
 "..\..\..\langapi\debugger" /I "..\..\..\langapi\include" /I\
 "..\..\..\public\nonship\inc" /D _DBCS=1 /D "_DEBUG" /D "_DLL" /D "_MBCS" /D\
 _MT=1 /D _NT1X_=100 /D "_NTWIN" /D "_SUSHI" /D "_TEST" /D _WIN32_WINNT=0x0400\
 /D "_WINDOWS" /D _X86_=1 /D "ADDR_MIXED" /D "CODEVIEW" /D CONDITION_HANDLING=1\
 /D "CROSS_PLATFORM" /D DBG=1 /D DEBUG=1 /D DEVL=1 /D "DOLPHIN" /D FPO=0 /D\
 "HOST32" /D "HOST_i386" /D i386=1 /D "NEW_PROJ_VIEW" /D "NT_BUILD" /D NT_INST=0\
 /D NT_UP=1 /D "OSDEBUG4" /D "PPC_PLATFORM" /D "SHIP" /D "STD_CALL" /D "STRICT"\
 /D "TARGET32" /D WIN32=100 /D WINNT=1 /D "x86" /D "BORDER_BUTTONS" /D\
 DWORDLONG=ULONGLONG /D "TARGET_i386" /D "WOW" /Fp"$(INTDIR)/dm.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /Bd /c $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_CPP_BP_C1c=\
	".\precomp.h"\
	

"$(INTDIR)\bp.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) /nologo /MD /W3 /GX /O2 /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D\
 "WIN32" /D "NDEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/" /c\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_CPP_BP_C1c=\
	".\precomp.h"\
	

"$(INTDIR)\bp.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) /nologo /MDd /W3 /GX /Zi /Od /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D\
 "WIN32" /D "_DEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/dm.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /QPm /c $(SOURCE)


!ENDIF 

SOURCE=.\user\dm.rc

!IF  "$(CFG)" == "dm - Win32 Release"

NODEP_RSC_DM_RC=\
	".\user\resrc1.h"\
	".\user\verstamp.h"\
	

"$(INTDIR)\dm.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/dm.res" /i "user" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Win32 Debug"

DEP_RSC_DM_RC=\
	"..\..\..\langapi\include\common.ver"\
	"..\..\..\langapi\include\version.h"\
	"..\..\..\langapi\include\verstamp.h"\
	
NODEP_RSC_DM_RC=\
	".\user\resrc1.h"\
	

"$(INTDIR)\dm.res" : $(SOURCE) $(DEP_RSC_DM_RC) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/dm.res" /i "..\include" /i "..\od\include" /i\
 "..\..\..\langapi\debugger\osdebug4" /i "..\..\..\langapi\debugger" /i\
 "..\..\..\langapi\include" /i "..\..\..\public\nonship\inc" /i "user" /d\
 _DBCS=1 /d _DLL=1 /d "_MBCS" /d _MT=1 /d _NT1X_=100 /d "_NTWIN" /d "_SUSHI" /d\
 "_TEST" /d _WIN32_WINNT=0x0400 /d "_WINDOWS" /d _X86_=1 /d "ADDR_MIXED" /d\
 "CODEVIEW" /d CONDITION_HANDLING=1 /d "CROSS_PLATFORM" /d DBG=1 /d DEBUG=1 /d\
 DEVL=1 /d "DOLPHIN" /d FPO=0 /d "HOST32" /d i386=1 /d "NEW_PROJ_VIEW" /d\
 "NT_BUILD" /d NT_INST=0 /d NT_UP=1 /d "OSDEBUG4" /d "PPC_PLATFORM" /d "SHIP" /d\
 "STD_CALL" /d "STRICT" /d "TARGET32" /d WIN32=100 /d WINNT=1 /d\
 "BORDER_BUTTONS" /d DWORDLONG=ULONGLONG $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

NODEP_RSC_DM_RC=\
	".\user\resrc1.h"\
	".\user\verstamp.h"\
	

"$(INTDIR)\dm.rsc" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /r /m /fo"$(INTDIR)/dm.rsc" /i "user" /d "_MAC" /d "_MPPC_"\
 /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

NODEP_RSC_DM_RC=\
	".\user\resrc1.h"\
	".\user\verstamp.h"\
	

"$(INTDIR)\dm.rsc" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /r /m /fo"$(INTDIR)/dm.rsc" /i "user" /d "_MAC" /d "_MPPC_"\
 /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\user\dm.def

!IF  "$(CFG)" == "dm - Win32 Release"

!ELSEIF  "$(CFG)" == "dm - Win32 Debug"

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Release"

!ELSEIF  "$(CFG)" == "dm - Power Macintosh Debug"

!ENDIF 


!ENDIF 

