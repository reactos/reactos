# Microsoft Developer Studio Generated NMAKE File, Based on em.bld
!IF "$(CFG)" == ""
CFG=em - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to em - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "em - Win32 Release" && "$(CFG)" != "em - Win32 Debug" &&\
 "$(CFG)" != "em - Power Macintosh Release" && "$(CFG)" !=\
 "em - Power Macintosh Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "em.bld" CFG="em - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "em - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "em - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "em - Power Macintosh Release" (based on\
 "Power Macintosh Dynamic-Link Library")
!MESSAGE "em - Power Macintosh Debug" (based on\
 "Power Macintosh Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "em - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\em.dll"

CLEAN : 
	-@erase "$(INTDIR)\em.res"
	-@erase "$(INTDIR)\emdisasm.obj"
	-@erase "$(INTDIR)\emdp.obj"
	-@erase "$(INTDIR)\emdp2.obj"
	-@erase "$(INTDIR)\emdp3.obj"
	-@erase "$(INTDIR)\emdp_axp.obj"
	-@erase "$(INTDIR)\emdp_mip.obj"
	-@erase "$(INTDIR)\emdp_x86.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(OUTDIR)\em.dll"
	-@erase "$(OUTDIR)\em.exp"
	-@erase "$(OUTDIR)\em.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/em.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c 
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/em.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/em.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/em.pdb" /machine:I386 /def:".\em.def" /out:"$(OUTDIR)/em.dll"\
 /implib:"$(OUTDIR)/em.lib" 
LINK32_OBJS= \
	"$(INTDIR)\em.res" \
	"$(INTDIR)\emdisasm.obj" \
	"$(INTDIR)\emdp.obj" \
	"$(INTDIR)\emdp2.obj" \
	"$(INTDIR)\emdp3.obj" \
	"$(INTDIR)\emdp_axp.obj" \
	"$(INTDIR)\emdp_mip.obj" \
	"$(INTDIR)\emdp_x86.obj"

"$(OUTDIR)\em.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "em - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\em.dll"

CLEAN : 
	-@erase "$(INTDIR)\em.res"
	-@erase "$(INTDIR)\emdisasm.obj"
	-@erase "$(INTDIR)\emdp.obj"
	-@erase "$(INTDIR)\emdp2.obj"
	-@erase "$(INTDIR)\emdp3.obj"
	-@erase "$(INTDIR)\emdp_axp.obj"
	-@erase "$(INTDIR)\emdp_mip.obj"
	-@erase "$(INTDIR)\emdp_x86.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\em.dll"
	-@erase "$(OUTDIR)\em.exp"
	-@erase "$(OUTDIR)\em.lib"
	-@erase "$(OUTDIR)\em.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm- /GR /GX /Zi /Od /Gf /Gy /I ".\include" /I\
 ".\od\include" /I "..\..\langapi\debugger\osdebug4" /I "..\..\langapi\debugger"\
 /I "..\..\langapi\include" /I "..\..\public\nonship\inc" /D _DBCS=1 /D "_DEBUG"\
 /D "_DLL" /D "_MBCS" /D _MT=1 /D _NT1X_=100 /D "_NTWIN" /D "_SUSHI" /D "_TEST"\
 /D _WIN32_WINNT=0x0400 /D "_WINDOWS" /D _X86_=1 /D "ADDR_MIXED" /D "CODEVIEW"\
 /D CONDITION_HANDLING=1 /D "CROSS_PLATFORM" /D DBG=1 /D DEBUG=1 /D DEVL=1 /D\
 "DOLPHIN" /D FPO=0 /D "HOST32" /D "HOST_i386" /D i386=1 /D "NEW_PROJ_VIEW" /D\
 "NT_BUILD" /D NT_INST=0 /D NT_UP=1 /D "OSDEBUG4" /D "PPC_PLATFORM" /D "SHIP" /D\
 "STD_CALL" /D "STRICT" /D "TARGET32" /D WIN32=100 /D WINNT=1 /D "x86"\
 /Fp"$(INTDIR)/em.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /FD /c 
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
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)/em.res" /i ".\include" /i ".\od\include" /i\
 "..\..\langapi\debugger" /i "..\..\langapi\debugger\osdebug4" /i\
 "..\..\langapi\include" /i "..\..\public\nonship\inc" /d _DBCS=1 /d _DLL=1 /d\
 "_MBCS" /d _MT=1 /d _NT1X_=100 /d "_NTWIN" /d "_SUSHI" /d "_TEST" /d\
 _WIN32_WINNT=0x0400 /d "_WINDOWS" /d _X86_=1 /d "ADDR_MIXED" /d "CODEVIEW" /d\
 CONDITION_HANDLING=1 /d "CROSS_PLATFORM" /d DBG=1 /d DEBUG=1 /d DEVL=1 /d\
 "DOLPHIN" /d FPO=0 /d "HOST32" /d i386=1 /d "NEW_PROJ_VIEW" /d "NT_BUILD" /d\
 NT_INST=0 /d NT_UP=1 /d "OSDEBUG4" /d "PPC_PLATFORM" /d "SHIP" /d "STD_CALL" /d\
 "STRICT" /d "TARGET32" /d WIN32=100 /d WINNT=1 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/em.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib ..\..\public\nonship\lib\i386\disasm.lib\
 ..\..\public\nonship\lib\i386\imagehlp.lib kernel32.lib /nologo /version:5.0\
 /entry:"_DllMainCRTStartup@12" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/em.pdb" /debug /machine:I386 /nodefaultlib /def:".\em.def"\
 /force /out:"$(OUTDIR)/em.dll" /implib:"$(OUTDIR)/em.lib" /merge:.rdata=.text\
 /merge:_PAGE=PAGE /merge:_TEXT=.text /opt:REF /section:INIT,d 
LINK32_OBJS= \
	"$(INTDIR)\em.res" \
	"$(INTDIR)\emdisasm.obj" \
	"$(INTDIR)\emdp.obj" \
	"$(INTDIR)\emdp2.obj" \
	"$(INTDIR)\emdp3.obj" \
	"$(INTDIR)\emdp_axp.obj" \
	"$(INTDIR)\emdp_mip.obj" \
	"$(INTDIR)\emdp_x86.obj"

"$(OUTDIR)\em.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

OUTDIR=.\PMacRel
INTDIR=.\PMacRel

ALL : "$(OUTDIR)\em.dll" "$(OUTDIR)\em.trg"

CLEAN : 
	-@erase "$(INTDIR)\em.rsc"
	-@erase "$(INTDIR)\emdisasm.obj"
	-@erase "$(INTDIR)\emdp.obj"
	-@erase "$(INTDIR)\emdp2.obj"
	-@erase "$(INTDIR)\emdp3.obj"
	-@erase "$(INTDIR)\emdp_axp.obj"
	-@erase "$(INTDIR)\emdp_mip.obj"
	-@erase "$(INTDIR)\emdp_x86.obj"
	-@erase "$(OUTDIR)\em.dll"
	-@erase "$(OUTDIR)\em.exp"
	-@erase "$(OUTDIR)\em.lib"
	-@erase "$(OUTDIR)\em.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "NDEBUG" /ppc 
CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D "WIN32"\
 /D "NDEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/em.pch" /YX /Fo"$(INTDIR)/" /c 
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
RSC_PROJ=/l 0x409 /r /m /fo"$(INTDIR)/em.rsc" /d "_MAC" /d "_MPPC_" /d "NDEBUG"\
 
MRC=mrc.exe
MRC_PROJ=/D "_MPPC_" /D "_MAC" /D "NDEBUG" /l 0x409 /NOLOGO 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/em.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /mac:nobundle /mac:type="shlb" /mac:creator="cfmg"\
 /mac:init="WlmConnectionInit" /mac:term="WlmConnectionTerm" /dll\
 /incremental:no /pdb:"$(OUTDIR)/em.pdb" /machine:MPPC /def:".\em.def"\
 /out:"$(OUTDIR)/em.dll" /implib:"$(OUTDIR)/em.lib" 
LINK32_OBJS= \
	"$(INTDIR)\em.rsc" \
	"$(INTDIR)\emdisasm.obj" \
	"$(INTDIR)\emdp.obj" \
	"$(INTDIR)\emdp2.obj" \
	"$(INTDIR)\emdp3.obj" \
	"$(INTDIR)\emdp_axp.obj" \
	"$(INTDIR)\emdp_mip.obj" \
	"$(INTDIR)\emdp_x86.obj"

"$(OUTDIR)\em.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

MFILE32=mfile.exe
MFILE32_FLAGS=COPY /NOLOGO 
MFILE32_FILES= \
	"$(OUTDIR)\em.dll"

"$(OUTDIR)\em.trg" : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacRel\em.dll\
 "$(MFILE32_DEST):em.dll">"$(OUTDIR)\em.trg"

DOWNLOAD : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacRel\em.dll\
 "$(MFILE32_DEST):em.dll">"$(OUTDIR)\em.trg"

!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

OUTDIR=.\PMacDbg
INTDIR=.\PMacDbg

ALL : "$(OUTDIR)\em.dll" "$(OUTDIR)\em.trg"

CLEAN : 
	-@erase "$(INTDIR)\em.rsc"
	-@erase "$(INTDIR)\emdisasm.obj"
	-@erase "$(INTDIR)\emdp.obj"
	-@erase "$(INTDIR)\emdp2.obj"
	-@erase "$(INTDIR)\emdp3.obj"
	-@erase "$(INTDIR)\emdp_axp.obj"
	-@erase "$(INTDIR)\emdp_mip.obj"
	-@erase "$(INTDIR)\emdp_x86.obj"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\em.dll"
	-@erase "$(OUTDIR)\em.exp"
	-@erase "$(OUTDIR)\em.ilk"
	-@erase "$(OUTDIR)\em.lib"
	-@erase "$(OUTDIR)\em.pdb"
	-@erase "$(OUTDIR)\em.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
MTL_PROJ=/nologo /D "_DEBUG" /ppc 
CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /D "_MAC" /D "_MPPC_" /D "_WINDOWS" /D\
 "WIN32" /D "_DEBUG" /D "_WLMDLL" /Fp"$(INTDIR)/em.pch" /YX /Fo"$(INTDIR)/"\
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
RSC_PROJ=/l 0x409 /r /m /fo"$(INTDIR)/em.rsc" /d "_MAC" /d "_MPPC_" /d "_DEBUG"\
 
MRC=mrc.exe
MRC_PROJ=/D "_MPPC_" /D "_MAC" /D "_DEBUG" /l 0x409 /NOLOGO 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)/em.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /mac:nobundle /mac:type="shlb" /mac:creator="cfmg"\
 /mac:init="WlmConnectionInit" /mac:term="WlmConnectionTerm" /dll\
 /incremental:yes /pdb:"$(OUTDIR)/em.pdb" /debug /machine:MPPC /def:".\em.def"\
 /out:"$(OUTDIR)/em.dll" /implib:"$(OUTDIR)/em.lib" 
LINK32_OBJS= \
	"$(INTDIR)\em.rsc" \
	"$(INTDIR)\emdisasm.obj" \
	"$(INTDIR)\emdp.obj" \
	"$(INTDIR)\emdp2.obj" \
	"$(INTDIR)\emdp3.obj" \
	"$(INTDIR)\emdp_axp.obj" \
	"$(INTDIR)\emdp_mip.obj" \
	"$(INTDIR)\emdp_x86.obj"

"$(OUTDIR)\em.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

MFILE32=mfile.exe
MFILE32_FLAGS=COPY /NOLOGO 
MFILE32_FILES= \
	"$(OUTDIR)\em.dll"

"$(OUTDIR)\em.trg" : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacDbg\em.dll\
 "$(MFILE32_DEST):em.dll">"$(OUTDIR)\em.trg"

DOWNLOAD : "$(OUTDIR)" $(MFILE32_FILES)
    $(MFILE32) $(MFILE32_FLAGS) .\PMacDbg\em.dll\
 "$(MFILE32_DEST):em.dll">"$(OUTDIR)\em.trg"

!ENDIF 


!IF "$(CFG)" == "em - Win32 Release" || "$(CFG)" == "em - Win32 Debug" ||\
 "$(CFG)" == "em - Power Macintosh Release" || "$(CFG)" ==\
 "em - Power Macintosh Debug"

!IF  "$(CFG)" == "em - Win32 Release"

!ELSEIF  "$(CFG)" == "em - Win32 Debug"

!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

!ENDIF 

SOURCE=.\emdp3.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp3.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp3.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP3=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	".\win32msg.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP3=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp3.obj" : $(SOURCE) $(DEP_CPP_EMDP3) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP3=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	".\win32msg.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP3=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp3.obj" : $(SOURCE) $(DEP_CPP_EMDP3) "$(INTDIR)"


!ENDIF 

SOURCE=.\em.rc

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\em.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\em.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"


"$(INTDIR)\em.rsc" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"


"$(INTDIR)\em.rsc" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\emdisasm.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdisasm.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDIS=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDIS=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdisasm.obj" : $(SOURCE) $(DEP_CPP_EMDIS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDIS=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDIS=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdisasm.obj" : $(SOURCE) $(DEP_CPP_EMDIS) "$(INTDIR)"


!ENDIF 

SOURCE=.\emdp.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP_=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_=\
	".\dbgver.h"\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp.obj" : $(SOURCE) $(DEP_CPP_EMDP_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP_=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_=\
	".\dbgver.h"\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp.obj" : $(SOURCE) $(DEP_CPP_EMDP_) "$(INTDIR)"


!ENDIF 

SOURCE=.\emdp_axp.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp_axp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp_axp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP_A=\
	".\biavst.h"\
	".\ehdata.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_axp.h"\
	".\regs_axp.h"\
	".\str_axp.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_A=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp_axp.obj" : $(SOURCE) $(DEP_CPP_EMDP_A) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP_A=\
	".\biavst.h"\
	".\ehdata.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_axp.h"\
	".\regs_axp.h"\
	".\str_axp.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_A=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp_axp.obj" : $(SOURCE) $(DEP_CPP_EMDP_A) "$(INTDIR)"


!ENDIF 

SOURCE=.\emdp_mip.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp_mip.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp_mip.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP_M=\
	".\biavst.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_mip.h"\
	".\regs_mip.h"\
	".\str_mip.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_M=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp_mip.obj" : $(SOURCE) $(DEP_CPP_EMDP_M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP_M=\
	".\biavst.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_mip.h"\
	".\regs_mip.h"\
	".\str_mip.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_M=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	

"$(INTDIR)\emdp_mip.obj" : $(SOURCE) $(DEP_CPP_EMDP_M) "$(INTDIR)"


!ENDIF 

SOURCE=.\emdp_x86.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp_x86.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp_x86.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP_X=\
	".\biavst.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_x86.h"\
	".\regs_x86.h"\
	".\str_x86.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_X=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	".\x86regs.h"\
	

"$(INTDIR)\emdp_x86.obj" : $(SOURCE) $(DEP_CPP_EMDP_X) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP_X=\
	".\biavst.h"\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emdp_plt.h"\
	".\emproto.h"\
	".\flag_x86.h"\
	".\regs_x86.h"\
	".\str_x86.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP_X=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	".\x86regs.h"\
	

"$(INTDIR)\emdp_x86.obj" : $(SOURCE) $(DEP_CPP_EMDP_X) "$(INTDIR)"


!ENDIF 

SOURCE=.\emdp2.cpp

!IF  "$(CFG)" == "em - Win32 Release"


"$(INTDIR)\emdp2.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Win32 Debug"


"$(INTDIR)\emdp2.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

DEP_CPP_EMDP2=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP2=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	".\xosd.h"\
	

"$(INTDIR)\emdp2.obj" : $(SOURCE) $(DEP_CPP_EMDP2) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

DEP_CPP_EMDP2=\
	".\emcpu.h"\
	".\emdata.h"\
	".\emdp.h"\
	".\emproto.h"\
	{$(INCLUDE)}"\imagehlp.h"\
	
NODEP_CPP_EMDP2=\
	".\emdm.h"\
	".\od.h"\
	".\odassert.h"\
	".\odp.h"\
	".\shapi.hxx"\
	".\win32dm.h"\
	".\xosd.h"\
	

"$(INTDIR)\emdp2.obj" : $(SOURCE) $(DEP_CPP_EMDP2) "$(INTDIR)"


!ENDIF 

SOURCE=.\em.def

!IF  "$(CFG)" == "em - Win32 Release"

!ELSEIF  "$(CFG)" == "em - Win32 Debug"

!ELSEIF  "$(CFG)" == "em - Power Macintosh Release"

!ELSEIF  "$(CFG)" == "em - Power Macintosh Debug"

!ENDIF 


!ENDIF 

