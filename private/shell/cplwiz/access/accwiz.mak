# Microsoft Developer Studio Generated NMAKE File, Based on accwiz.dsp
!IF "$(CFG)" == ""
CFG=AccWiz - Win32 UNICODE NT5 Debug
!MESSAGE No configuration specified. Defaulting to AccWiz - Win32 UNICODE NT5\
 Debug.
!ENDIF 

!IF "$(CFG)" != "AccWiz - Win32 ANSI Win97 Debug" && "$(CFG)" !=\
 "AccWiz - Win32 ANSI Win97 Release" && "$(CFG)" !=\
 "AccWiz - Win32 UNICODE NT5 Debug" && "$(CFG)" !=\
 "AccWiz - Win32 UNICODE NT5 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "accwiz.mak" CFG="AccWiz - Win32 UNICODE NT5 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AccWiz - Win32 ANSI Win97 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 ANSI Win97 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 UNICODE NT5 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AccWiz - Win32 UNICODE NT5 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"

OUTDIR=.\Debug97
INTDIR=.\Debug97
# Begin Custom Macros
OutDir=.\Debug97
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\accwiz.exe" "$(OUTDIR)\accwiz.bsc"

!ELSE 

ALL : "$(OUTDIR)\accwiz.exe" "$(OUTDIR)\accwiz.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\AccWiz.obj"
	-@erase "$(INTDIR)\accwiz.pch"
	-@erase "$(INTDIR)\AccWiz.res"
	-@erase "$(INTDIR)\AccWiz.sbr"
	-@erase "$(INTDIR)\DlgFonts.obj"
	-@erase "$(INTDIR)\DlgFonts.sbr"
	-@erase "$(INTDIR)\lookdlg.obj"
	-@erase "$(INTDIR)\lookdlg.sbr"
	-@erase "$(INTDIR)\lookprev.obj"
	-@erase "$(INTDIR)\lookprev.sbr"
	-@erase "$(INTDIR)\PCH.obj"
	-@erase "$(INTDIR)\PCH.sbr"
	-@erase "$(INTDIR)\pgbase.obj"
	-@erase "$(INTDIR)\pgbase.sbr"
	-@erase "$(INTDIR)\pgfinish.obj"
	-@erase "$(INTDIR)\pgfinish.sbr"
	-@erase "$(INTDIR)\pgFltKey.obj"
	-@erase "$(INTDIR)\pgFltKey.sbr"
	-@erase "$(INTDIR)\pgGenric.obj"
	-@erase "$(INTDIR)\pgGenric.sbr"
	-@erase "$(INTDIR)\pgHghCon.obj"
	-@erase "$(INTDIR)\pgHghCon.sbr"
	-@erase "$(INTDIR)\pgHotKey.obj"
	-@erase "$(INTDIR)\pgHotKey.sbr"
	-@erase "$(INTDIR)\pgIconSz.obj"
	-@erase "$(INTDIR)\pgIconSz.sbr"
	-@erase "$(INTDIR)\pgLokPrv.obj"
	-@erase "$(INTDIR)\pgLokPrv.sbr"
	-@erase "$(INTDIR)\pgLookWz.obj"
	-@erase "$(INTDIR)\pgLookWz.sbr"
	-@erase "$(INTDIR)\pgMinTx2.obj"
	-@erase "$(INTDIR)\pgMinTx2.sbr"
	-@erase "$(INTDIR)\pgMinTxt.obj"
	-@erase "$(INTDIR)\pgMinTxt.sbr"
	-@erase "$(INTDIR)\pgMseBut.obj"
	-@erase "$(INTDIR)\pgMseBut.sbr"
	-@erase "$(INTDIR)\pgMseCur.obj"
	-@erase "$(INTDIR)\pgMseCur.sbr"
	-@erase "$(INTDIR)\pgMseKey.obj"
	-@erase "$(INTDIR)\pgMseKey.sbr"
	-@erase "$(INTDIR)\pgScrBar.obj"
	-@erase "$(INTDIR)\pgScrBar.sbr"
	-@erase "$(INTDIR)\pgSerKey.obj"
	-@erase "$(INTDIR)\pgSerKey.sbr"
	-@erase "$(INTDIR)\pgShwHlp.obj"
	-@erase "$(INTDIR)\pgShwHlp.sbr"
	-@erase "$(INTDIR)\pgSndSen.obj"
	-@erase "$(INTDIR)\pgSndSen.sbr"
	-@erase "$(INTDIR)\pgStkKey.obj"
	-@erase "$(INTDIR)\pgStkKey.sbr"
	-@erase "$(INTDIR)\pgSveDef.obj"
	-@erase "$(INTDIR)\pgSveDef.sbr"
	-@erase "$(INTDIR)\pgSveFil.obj"
	-@erase "$(INTDIR)\pgSveFil.sbr"
	-@erase "$(INTDIR)\pgTglKey.obj"
	-@erase "$(INTDIR)\pgTglKey.sbr"
	-@erase "$(INTDIR)\pgTmeOut.obj"
	-@erase "$(INTDIR)\pgTmeOut.sbr"
	-@erase "$(INTDIR)\pgWizOpt.obj"
	-@erase "$(INTDIR)\pgWizOpt.sbr"
	-@erase "$(INTDIR)\pgWizWiz.obj"
	-@erase "$(INTDIR)\pgWizWiz.sbr"
	-@erase "$(INTDIR)\Schemes.obj"
	-@erase "$(INTDIR)\Schemes.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\accwiz.bsc"
	-@erase "$(OUTDIR)\accwiz.exe"
	-@erase "$(OUTDIR)\accwiz.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\public\sdk\inc"\
 /I "..\..\..\windows\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\accwiz.pch" /Yu"pch.hxx"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug97/
CPP_SBRS=.\Debug97/

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\AccWiz.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\accwiz.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AccWiz.sbr" \
	"$(INTDIR)\DlgFonts.sbr" \
	"$(INTDIR)\lookdlg.sbr" \
	"$(INTDIR)\lookprev.sbr" \
	"$(INTDIR)\PCH.sbr" \
	"$(INTDIR)\pgbase.sbr" \
	"$(INTDIR)\pgfinish.sbr" \
	"$(INTDIR)\pgFltKey.sbr" \
	"$(INTDIR)\pgGenric.sbr" \
	"$(INTDIR)\pgHghCon.sbr" \
	"$(INTDIR)\pgHotKey.sbr" \
	"$(INTDIR)\pgIconSz.sbr" \
	"$(INTDIR)\pgLokPrv.sbr" \
	"$(INTDIR)\pgLookWz.sbr" \
	"$(INTDIR)\pgMinTx2.sbr" \
	"$(INTDIR)\pgMinTxt.sbr" \
	"$(INTDIR)\pgMseBut.sbr" \
	"$(INTDIR)\pgMseCur.sbr" \
	"$(INTDIR)\pgMseKey.sbr" \
	"$(INTDIR)\pgScrBar.sbr" \
	"$(INTDIR)\pgSerKey.sbr" \
	"$(INTDIR)\pgShwHlp.sbr" \
	"$(INTDIR)\pgSndSen.sbr" \
	"$(INTDIR)\pgStkKey.sbr" \
	"$(INTDIR)\pgSveDef.sbr" \
	"$(INTDIR)\pgSveFil.sbr" \
	"$(INTDIR)\pgTglKey.sbr" \
	"$(INTDIR)\pgTmeOut.sbr" \
	"$(INTDIR)\pgWizOpt.sbr" \
	"$(INTDIR)\pgWizWiz.sbr" \
	"$(INTDIR)\Schemes.sbr"

"$(OUTDIR)\accwiz.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32p.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\accwiz.pdb" /debug /machine:I386 /out:"$(OUTDIR)\accwiz.exe"\
 /libpath:"..\..\..\..\public\sdk\lib\i386" 
LINK32_OBJS= \
	"$(INTDIR)\AccWiz.obj" \
	"$(INTDIR)\AccWiz.res" \
	"$(INTDIR)\DlgFonts.obj" \
	"$(INTDIR)\lookdlg.obj" \
	"$(INTDIR)\lookprev.obj" \
	"$(INTDIR)\PCH.obj" \
	"$(INTDIR)\pgbase.obj" \
	"$(INTDIR)\pgfinish.obj" \
	"$(INTDIR)\pgFltKey.obj" \
	"$(INTDIR)\pgGenric.obj" \
	"$(INTDIR)\pgHghCon.obj" \
	"$(INTDIR)\pgHotKey.obj" \
	"$(INTDIR)\pgIconSz.obj" \
	"$(INTDIR)\pgLokPrv.obj" \
	"$(INTDIR)\pgLookWz.obj" \
	"$(INTDIR)\pgMinTx2.obj" \
	"$(INTDIR)\pgMinTxt.obj" \
	"$(INTDIR)\pgMseBut.obj" \
	"$(INTDIR)\pgMseCur.obj" \
	"$(INTDIR)\pgMseKey.obj" \
	"$(INTDIR)\pgScrBar.obj" \
	"$(INTDIR)\pgSerKey.obj" \
	"$(INTDIR)\pgShwHlp.obj" \
	"$(INTDIR)\pgSndSen.obj" \
	"$(INTDIR)\pgStkKey.obj" \
	"$(INTDIR)\pgSveDef.obj" \
	"$(INTDIR)\pgSveFil.obj" \
	"$(INTDIR)\pgTglKey.obj" \
	"$(INTDIR)\pgTmeOut.obj" \
	"$(INTDIR)\pgWizOpt.obj" \
	"$(INTDIR)\pgWizWiz.obj" \
	"$(INTDIR)\Schemes.obj"

"$(OUTDIR)\accwiz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"

OUTDIR=.\Release97
INTDIR=.\Release97
# Begin Custom Macros
OutDir=.\Release97
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\accwiz.exe"

!ELSE 

ALL : "$(OUTDIR)\accwiz.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\AccWiz.obj"
	-@erase "$(INTDIR)\accwiz.pch"
	-@erase "$(INTDIR)\AccWiz.res"
	-@erase "$(INTDIR)\DlgFonts.obj"
	-@erase "$(INTDIR)\lookdlg.obj"
	-@erase "$(INTDIR)\lookprev.obj"
	-@erase "$(INTDIR)\PCH.obj"
	-@erase "$(INTDIR)\pgbase.obj"
	-@erase "$(INTDIR)\pgfinish.obj"
	-@erase "$(INTDIR)\pgFltKey.obj"
	-@erase "$(INTDIR)\pgGenric.obj"
	-@erase "$(INTDIR)\pgHghCon.obj"
	-@erase "$(INTDIR)\pgHotKey.obj"
	-@erase "$(INTDIR)\pgIconSz.obj"
	-@erase "$(INTDIR)\pgLokPrv.obj"
	-@erase "$(INTDIR)\pgLookWz.obj"
	-@erase "$(INTDIR)\pgMinTx2.obj"
	-@erase "$(INTDIR)\pgMinTxt.obj"
	-@erase "$(INTDIR)\pgMseBut.obj"
	-@erase "$(INTDIR)\pgMseCur.obj"
	-@erase "$(INTDIR)\pgMseKey.obj"
	-@erase "$(INTDIR)\pgScrBar.obj"
	-@erase "$(INTDIR)\pgSerKey.obj"
	-@erase "$(INTDIR)\pgShwHlp.obj"
	-@erase "$(INTDIR)\pgSndSen.obj"
	-@erase "$(INTDIR)\pgStkKey.obj"
	-@erase "$(INTDIR)\pgSveDef.obj"
	-@erase "$(INTDIR)\pgSveFil.obj"
	-@erase "$(INTDIR)\pgTglKey.obj"
	-@erase "$(INTDIR)\pgTmeOut.obj"
	-@erase "$(INTDIR)\pgWizOpt.obj"
	-@erase "$(INTDIR)\pgWizWiz.obj"
	-@erase "$(INTDIR)\Schemes.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\accwiz.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\..\public\sdk\inc" /I\
 "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /Fp"$(INTDIR)\accwiz.pch" /Yu"pch.hxx" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release97/
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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\AccWiz.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\accwiz.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib comctl32.lib user32p.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)\accwiz.pdb" /machine:I386\
 /out:"$(OUTDIR)\accwiz.exe" /libpath:"..\..\..\..\public\sdk\lib\i386" 
LINK32_OBJS= \
	"$(INTDIR)\AccWiz.obj" \
	"$(INTDIR)\AccWiz.res" \
	"$(INTDIR)\DlgFonts.obj" \
	"$(INTDIR)\lookdlg.obj" \
	"$(INTDIR)\lookprev.obj" \
	"$(INTDIR)\PCH.obj" \
	"$(INTDIR)\pgbase.obj" \
	"$(INTDIR)\pgfinish.obj" \
	"$(INTDIR)\pgFltKey.obj" \
	"$(INTDIR)\pgGenric.obj" \
	"$(INTDIR)\pgHghCon.obj" \
	"$(INTDIR)\pgHotKey.obj" \
	"$(INTDIR)\pgIconSz.obj" \
	"$(INTDIR)\pgLokPrv.obj" \
	"$(INTDIR)\pgLookWz.obj" \
	"$(INTDIR)\pgMinTx2.obj" \
	"$(INTDIR)\pgMinTxt.obj" \
	"$(INTDIR)\pgMseBut.obj" \
	"$(INTDIR)\pgMseCur.obj" \
	"$(INTDIR)\pgMseKey.obj" \
	"$(INTDIR)\pgScrBar.obj" \
	"$(INTDIR)\pgSerKey.obj" \
	"$(INTDIR)\pgShwHlp.obj" \
	"$(INTDIR)\pgSndSen.obj" \
	"$(INTDIR)\pgStkKey.obj" \
	"$(INTDIR)\pgSveDef.obj" \
	"$(INTDIR)\pgSveFil.obj" \
	"$(INTDIR)\pgTglKey.obj" \
	"$(INTDIR)\pgTmeOut.obj" \
	"$(INTDIR)\pgWizOpt.obj" \
	"$(INTDIR)\pgWizWiz.obj" \
	"$(INTDIR)\Schemes.obj"

"$(OUTDIR)\accwiz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"

OUTDIR=.\DebugNT
INTDIR=.\DebugNT
# Begin Custom Macros
OutDir=.\DebugNT
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\accwiz.exe" "$(OUTDIR)\accwiz.bsc"

!ELSE 

ALL : "$(OUTDIR)\accwiz.exe" "$(OUTDIR)\accwiz.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\AccWiz.obj"
	-@erase "$(INTDIR)\accwiz.pch"
	-@erase "$(INTDIR)\AccWiz.res"
	-@erase "$(INTDIR)\AccWiz.sbr"
	-@erase "$(INTDIR)\DlgFonts.obj"
	-@erase "$(INTDIR)\DlgFonts.sbr"
	-@erase "$(INTDIR)\lookdlg.obj"
	-@erase "$(INTDIR)\lookdlg.sbr"
	-@erase "$(INTDIR)\lookprev.obj"
	-@erase "$(INTDIR)\lookprev.sbr"
	-@erase "$(INTDIR)\PCH.obj"
	-@erase "$(INTDIR)\PCH.sbr"
	-@erase "$(INTDIR)\pgbase.obj"
	-@erase "$(INTDIR)\pgbase.sbr"
	-@erase "$(INTDIR)\pgfinish.obj"
	-@erase "$(INTDIR)\pgfinish.sbr"
	-@erase "$(INTDIR)\pgFltKey.obj"
	-@erase "$(INTDIR)\pgFltKey.sbr"
	-@erase "$(INTDIR)\pgGenric.obj"
	-@erase "$(INTDIR)\pgGenric.sbr"
	-@erase "$(INTDIR)\pgHghCon.obj"
	-@erase "$(INTDIR)\pgHghCon.sbr"
	-@erase "$(INTDIR)\pgHotKey.obj"
	-@erase "$(INTDIR)\pgHotKey.sbr"
	-@erase "$(INTDIR)\pgIconSz.obj"
	-@erase "$(INTDIR)\pgIconSz.sbr"
	-@erase "$(INTDIR)\pgLokPrv.obj"
	-@erase "$(INTDIR)\pgLokPrv.sbr"
	-@erase "$(INTDIR)\pgLookWz.obj"
	-@erase "$(INTDIR)\pgLookWz.sbr"
	-@erase "$(INTDIR)\pgMinTx2.obj"
	-@erase "$(INTDIR)\pgMinTx2.sbr"
	-@erase "$(INTDIR)\pgMinTxt.obj"
	-@erase "$(INTDIR)\pgMinTxt.sbr"
	-@erase "$(INTDIR)\pgMseBut.obj"
	-@erase "$(INTDIR)\pgMseBut.sbr"
	-@erase "$(INTDIR)\pgMseCur.obj"
	-@erase "$(INTDIR)\pgMseCur.sbr"
	-@erase "$(INTDIR)\pgMseKey.obj"
	-@erase "$(INTDIR)\pgMseKey.sbr"
	-@erase "$(INTDIR)\pgScrBar.obj"
	-@erase "$(INTDIR)\pgScrBar.sbr"
	-@erase "$(INTDIR)\pgSerKey.obj"
	-@erase "$(INTDIR)\pgSerKey.sbr"
	-@erase "$(INTDIR)\pgShwHlp.obj"
	-@erase "$(INTDIR)\pgShwHlp.sbr"
	-@erase "$(INTDIR)\pgSndSen.obj"
	-@erase "$(INTDIR)\pgSndSen.sbr"
	-@erase "$(INTDIR)\pgStkKey.obj"
	-@erase "$(INTDIR)\pgStkKey.sbr"
	-@erase "$(INTDIR)\pgSveDef.obj"
	-@erase "$(INTDIR)\pgSveDef.sbr"
	-@erase "$(INTDIR)\pgSveFil.obj"
	-@erase "$(INTDIR)\pgSveFil.sbr"
	-@erase "$(INTDIR)\pgTglKey.obj"
	-@erase "$(INTDIR)\pgTglKey.sbr"
	-@erase "$(INTDIR)\pgTmeOut.obj"
	-@erase "$(INTDIR)\pgTmeOut.sbr"
	-@erase "$(INTDIR)\pgWizOpt.obj"
	-@erase "$(INTDIR)\pgWizOpt.sbr"
	-@erase "$(INTDIR)\pgWizWiz.obj"
	-@erase "$(INTDIR)\pgWizWiz.sbr"
	-@erase "$(INTDIR)\Schemes.obj"
	-@erase "$(INTDIR)\Schemes.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\accwiz.bsc"
	-@erase "$(OUTDIR)\accwiz.exe"
	-@erase "$(OUTDIR)\accwiz.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\public\sdk\inc"\
 /I "..\..\..\windows\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /D "UNICODE" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\accwiz.pch"\
 /Yu"pch.hxx" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\DebugNT/
CPP_SBRS=.\DebugNT/

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\AccWiz.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\accwiz.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AccWiz.sbr" \
	"$(INTDIR)\DlgFonts.sbr" \
	"$(INTDIR)\lookdlg.sbr" \
	"$(INTDIR)\lookprev.sbr" \
	"$(INTDIR)\PCH.sbr" \
	"$(INTDIR)\pgbase.sbr" \
	"$(INTDIR)\pgfinish.sbr" \
	"$(INTDIR)\pgFltKey.sbr" \
	"$(INTDIR)\pgGenric.sbr" \
	"$(INTDIR)\pgHghCon.sbr" \
	"$(INTDIR)\pgHotKey.sbr" \
	"$(INTDIR)\pgIconSz.sbr" \
	"$(INTDIR)\pgLokPrv.sbr" \
	"$(INTDIR)\pgLookWz.sbr" \
	"$(INTDIR)\pgMinTx2.sbr" \
	"$(INTDIR)\pgMinTxt.sbr" \
	"$(INTDIR)\pgMseBut.sbr" \
	"$(INTDIR)\pgMseCur.sbr" \
	"$(INTDIR)\pgMseKey.sbr" \
	"$(INTDIR)\pgScrBar.sbr" \
	"$(INTDIR)\pgSerKey.sbr" \
	"$(INTDIR)\pgShwHlp.sbr" \
	"$(INTDIR)\pgSndSen.sbr" \
	"$(INTDIR)\pgStkKey.sbr" \
	"$(INTDIR)\pgSveDef.sbr" \
	"$(INTDIR)\pgSveFil.sbr" \
	"$(INTDIR)\pgTglKey.sbr" \
	"$(INTDIR)\pgTmeOut.sbr" \
	"$(INTDIR)\pgWizOpt.sbr" \
	"$(INTDIR)\pgWizWiz.sbr" \
	"$(INTDIR)\Schemes.sbr"

"$(OUTDIR)\accwiz.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32p.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\accwiz.pdb" /debug /machine:I386 /out:"$(OUTDIR)\accwiz.exe"\
 /libpath:"..\..\..\..\public\sdk\lib\i386" 
LINK32_OBJS= \
	"$(INTDIR)\AccWiz.obj" \
	"$(INTDIR)\AccWiz.res" \
	"$(INTDIR)\DlgFonts.obj" \
	"$(INTDIR)\lookdlg.obj" \
	"$(INTDIR)\lookprev.obj" \
	"$(INTDIR)\PCH.obj" \
	"$(INTDIR)\pgbase.obj" \
	"$(INTDIR)\pgfinish.obj" \
	"$(INTDIR)\pgFltKey.obj" \
	"$(INTDIR)\pgGenric.obj" \
	"$(INTDIR)\pgHghCon.obj" \
	"$(INTDIR)\pgHotKey.obj" \
	"$(INTDIR)\pgIconSz.obj" \
	"$(INTDIR)\pgLokPrv.obj" \
	"$(INTDIR)\pgLookWz.obj" \
	"$(INTDIR)\pgMinTx2.obj" \
	"$(INTDIR)\pgMinTxt.obj" \
	"$(INTDIR)\pgMseBut.obj" \
	"$(INTDIR)\pgMseCur.obj" \
	"$(INTDIR)\pgMseKey.obj" \
	"$(INTDIR)\pgScrBar.obj" \
	"$(INTDIR)\pgSerKey.obj" \
	"$(INTDIR)\pgShwHlp.obj" \
	"$(INTDIR)\pgSndSen.obj" \
	"$(INTDIR)\pgStkKey.obj" \
	"$(INTDIR)\pgSveDef.obj" \
	"$(INTDIR)\pgSveFil.obj" \
	"$(INTDIR)\pgTglKey.obj" \
	"$(INTDIR)\pgTmeOut.obj" \
	"$(INTDIR)\pgWizOpt.obj" \
	"$(INTDIR)\pgWizWiz.obj" \
	"$(INTDIR)\Schemes.obj"

"$(OUTDIR)\accwiz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"

OUTDIR=.\ReleaseNT
INTDIR=.\ReleaseNT
# Begin Custom Macros
OutDir=.\ReleaseNT
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\accwiz.exe"

!ELSE 

ALL : "$(OUTDIR)\accwiz.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\AccWiz.obj"
	-@erase "$(INTDIR)\accwiz.pch"
	-@erase "$(INTDIR)\AccWiz.res"
	-@erase "$(INTDIR)\DlgFonts.obj"
	-@erase "$(INTDIR)\lookdlg.obj"
	-@erase "$(INTDIR)\lookprev.obj"
	-@erase "$(INTDIR)\PCH.obj"
	-@erase "$(INTDIR)\pgbase.obj"
	-@erase "$(INTDIR)\pgfinish.obj"
	-@erase "$(INTDIR)\pgFltKey.obj"
	-@erase "$(INTDIR)\pgGenric.obj"
	-@erase "$(INTDIR)\pgHghCon.obj"
	-@erase "$(INTDIR)\pgHotKey.obj"
	-@erase "$(INTDIR)\pgIconSz.obj"
	-@erase "$(INTDIR)\pgLokPrv.obj"
	-@erase "$(INTDIR)\pgLookWz.obj"
	-@erase "$(INTDIR)\pgMinTx2.obj"
	-@erase "$(INTDIR)\pgMinTxt.obj"
	-@erase "$(INTDIR)\pgMseBut.obj"
	-@erase "$(INTDIR)\pgMseCur.obj"
	-@erase "$(INTDIR)\pgMseKey.obj"
	-@erase "$(INTDIR)\pgScrBar.obj"
	-@erase "$(INTDIR)\pgSerKey.obj"
	-@erase "$(INTDIR)\pgShwHlp.obj"
	-@erase "$(INTDIR)\pgSndSen.obj"
	-@erase "$(INTDIR)\pgStkKey.obj"
	-@erase "$(INTDIR)\pgSveDef.obj"
	-@erase "$(INTDIR)\pgSveFil.obj"
	-@erase "$(INTDIR)\pgTglKey.obj"
	-@erase "$(INTDIR)\pgTmeOut.obj"
	-@erase "$(INTDIR)\pgWizOpt.obj"
	-@erase "$(INTDIR)\pgWizWiz.obj"
	-@erase "$(INTDIR)\Schemes.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\accwiz.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\..\public\sdk\inc" /I\
 "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /D "UNICODE" /D "_UNICODE" /Fp"$(INTDIR)\accwiz.pch" /Yu"pch.hxx"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseNT/
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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\AccWiz.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\accwiz.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib comctl32.lib user32p.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)\accwiz.pdb" /machine:I386\
 /out:"$(OUTDIR)\accwiz.exe" /libpath:"..\..\..\..\public\sdk\lib\i386" 
LINK32_OBJS= \
	"$(INTDIR)\AccWiz.obj" \
	"$(INTDIR)\AccWiz.res" \
	"$(INTDIR)\DlgFonts.obj" \
	"$(INTDIR)\lookdlg.obj" \
	"$(INTDIR)\lookprev.obj" \
	"$(INTDIR)\PCH.obj" \
	"$(INTDIR)\pgbase.obj" \
	"$(INTDIR)\pgfinish.obj" \
	"$(INTDIR)\pgFltKey.obj" \
	"$(INTDIR)\pgGenric.obj" \
	"$(INTDIR)\pgHghCon.obj" \
	"$(INTDIR)\pgHotKey.obj" \
	"$(INTDIR)\pgIconSz.obj" \
	"$(INTDIR)\pgLokPrv.obj" \
	"$(INTDIR)\pgLookWz.obj" \
	"$(INTDIR)\pgMinTx2.obj" \
	"$(INTDIR)\pgMinTxt.obj" \
	"$(INTDIR)\pgMseBut.obj" \
	"$(INTDIR)\pgMseCur.obj" \
	"$(INTDIR)\pgMseKey.obj" \
	"$(INTDIR)\pgScrBar.obj" \
	"$(INTDIR)\pgSerKey.obj" \
	"$(INTDIR)\pgShwHlp.obj" \
	"$(INTDIR)\pgSndSen.obj" \
	"$(INTDIR)\pgStkKey.obj" \
	"$(INTDIR)\pgSveDef.obj" \
	"$(INTDIR)\pgSveFil.obj" \
	"$(INTDIR)\pgTglKey.obj" \
	"$(INTDIR)\pgTmeOut.obj" \
	"$(INTDIR)\pgWizOpt.obj" \
	"$(INTDIR)\pgWizWiz.obj" \
	"$(INTDIR)\Schemes.obj"

"$(OUTDIR)\accwiz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug" || "$(CFG)" ==\
 "AccWiz - Win32 ANSI Win97 Release" || "$(CFG)" ==\
 "AccWiz - Win32 UNICODE NT5 Debug" || "$(CFG)" ==\
 "AccWiz - Win32 UNICODE NT5 Release"
SOURCE=.\AccWiz.cpp
DEP_CPP_ACCWI=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\LookPrev.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgfinish.h"\
	".\pgFltKey.h"\
	".\pgGenric.h"\
	".\pgHghCon.h"\
	".\pgHotKey.h"\
	".\pgIconSz.h"\
	".\pgLokPrv.h"\
	".\pgLookWz.h"\
	".\pgMinTx2.h"\
	".\pgMinTxt.h"\
	".\pgMseBut.h"\
	".\pgMseCur.h"\
	".\pgMseKey.h"\
	".\pgScrBar.h"\
	".\pgSerKey.h"\
	".\pgShwHlp.h"\
	".\pgSndSen.h"\
	".\pgStkKey.h"\
	".\pgSveDef.h"\
	".\pgSveFil.h"\
	".\pgTglKey.h"\
	".\pgTmeOut.h"\
	".\pgWizOpt.h"\
	".\pgWizWiz.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\AccWiz.obj"	"$(INTDIR)\AccWiz.sbr" : $(SOURCE) $(DEP_CPP_ACCWI)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\AccWiz.obj" : $(SOURCE) $(DEP_CPP_ACCWI) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\AccWiz.obj"	"$(INTDIR)\AccWiz.sbr" : $(SOURCE) $(DEP_CPP_ACCWI)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\AccWiz.obj" : $(SOURCE) $(DEP_CPP_ACCWI) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\AccWiz.rc
DEP_RSC_ACCWIZ=\
	".\ACCESS.ICO"\
	".\arrow_in.ico"\
	".\arrow_me.ico"\
	".\arrow_sm.ico"\
	".\arrow_wh.ico"\
	".\baner16.bmp"\
	".\baner256.bmp"\
	".\banner17.bmp"\
	".\banner25.bmp"\
	".\bitmap1.bmp"\
	".\bmp00001.bmp"\
	".\cursor_l.ico"\
	".\cursor_m.ico"\
	".\cursor_s.bmp"\
	".\cursor_s.ico"\
	".\Disk.Ico"\
	".\FILTER.ICO"\
	".\GRAPHAPP.ICO"\
	".\HIGHCON.ICO"\
	".\ibeam_bl.ico"\
	".\ibeam_in.ico"\
	".\ibeam_la.ico"\
	".\ibeam_me.ico"\
	".\ibeam_sm.ico"\
	".\ibeam_wh.ico"\
	".\ICO00001.ICO"\
	".\ico00002.ico"\
	".\ico00003.ico"\
	".\ico00004.ico"\
	".\ico00005.ico"\
	".\ico00006.ico"\
	".\ico00007.ico"\
	".\ico00008.ico"\
	".\icon1.ico"\
	".\icon_lar.bmp"\
	".\icon_nor.bmp"\
	".\iconExLg.bmp"\
	".\iconLg.bmp"\
	".\iconNorL.bmp"\
	".\iconNorS.bmp"\
	".\iconNrml.bmp"\
	".\KEYSHORT.ICO"\
	".\MOUSE.BMP"\
	".\mouse1.bmp"\
	".\mouse2.bmp"\
	".\MSEKEYS.ICO"\
	".\SERIALKY.ICO"\
	".\Setup.Ico"\
	".\SHOWSNDS.ICO"\
	".\SHOWSTRY.ICO"\
	".\STICKY.ICO"\
	".\testimag.bmp"\
	".\TEXTAPPS.ICO"\
	".\TOGGLE.ICO"\
	".\Water16.bmp"\
	".\Water256.BMP"\
	".\watermar.bmp"\
	".\WINAPPS.ICO"\
	

"$(INTDIR)\AccWiz.res" : $(SOURCE) $(DEP_RSC_ACCWIZ) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\DlgFonts.cpp
DEP_CPP_DLGFO=\
	"..\..\..\windows\inc\winuserp.h"\
	".\DlgFonts.h"\
	".\pch.hxx"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\DlgFonts.obj"	"$(INTDIR)\DlgFonts.sbr" : $(SOURCE) $(DEP_CPP_DLGFO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\DlgFonts.obj" : $(SOURCE) $(DEP_CPP_DLGFO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\DlgFonts.obj"	"$(INTDIR)\DlgFonts.sbr" : $(SOURCE) $(DEP_CPP_DLGFO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\DlgFonts.obj" : $(SOURCE) $(DEP_CPP_DLGFO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\lookdlg.cpp
DEP_CPP_LOOKD=\
	"..\..\..\windows\inc\help.h"\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\desk.h"\
	".\deskid.h"\
	".\look.h"\
	".\pch.hxx"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\lookdlg.obj"	"$(INTDIR)\lookdlg.sbr" : $(SOURCE) $(DEP_CPP_LOOKD)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\lookdlg.obj" : $(SOURCE) $(DEP_CPP_LOOKD) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\lookdlg.obj"	"$(INTDIR)\lookdlg.sbr" : $(SOURCE) $(DEP_CPP_LOOKD)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\lookdlg.obj" : $(SOURCE) $(DEP_CPP_LOOKD) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\lookprev.cpp
DEP_CPP_LOOKP=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\desk.h"\
	".\look.h"\
	".\LookPrev.h"\
	".\pch.hxx"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\lookprev.obj"	"$(INTDIR)\lookprev.sbr" : $(SOURCE) $(DEP_CPP_LOOKP)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\lookprev.obj" : $(SOURCE) $(DEP_CPP_LOOKP) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\lookprev.obj"	"$(INTDIR)\lookprev.sbr" : $(SOURCE) $(DEP_CPP_LOOKP)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\lookprev.obj" : $(SOURCE) $(DEP_CPP_LOOKP) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\PCH.cpp
DEP_CPP_PCH_C=\
	"..\..\..\windows\inc\winuserp.h"\
	".\pch.hxx"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"

CPP_SWITCHES=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I\
 "..\..\..\..\public\sdk\inc" /I "..\..\..\windows\inc" /D "_DEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "WINNT" /D "STRICT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\accwiz.pch"\
 /Yc"pch.hxx" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\PCH.obj"	"$(INTDIR)\PCH.sbr"	"$(INTDIR)\accwiz.pch" : $(SOURCE)\
 $(DEP_CPP_PCH_C) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"

CPP_SWITCHES=/nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\..\public\sdk\inc" /I\
 "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /Fp"$(INTDIR)\accwiz.pch" /Yc"pch.hxx" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\PCH.obj"	"$(INTDIR)\accwiz.pch" : $(SOURCE) $(DEP_CPP_PCH_C)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"

CPP_SWITCHES=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I\
 "..\..\..\..\public\sdk\inc" /I "..\..\..\windows\inc" /D "_DEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "WINNT" /D "STRICT" /D "UNICODE" /D "_UNICODE"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\accwiz.pch" /Yc"pch.hxx" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\PCH.obj"	"$(INTDIR)\PCH.sbr"	"$(INTDIR)\accwiz.pch" : $(SOURCE)\
 $(DEP_CPP_PCH_C) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"

CPP_SWITCHES=/nologo /Gz /MT /W3 /GX /O2 /I "..\..\..\..\public\sdk\inc" /I\
 "..\..\..\windows\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D\
 "STRICT" /D "UNICODE" /D "_UNICODE" /Fp"$(INTDIR)\accwiz.pch" /Yc"pch.hxx"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\PCH.obj"	"$(INTDIR)\accwiz.pch" : $(SOURCE) $(DEP_CPP_PCH_C)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\pgbase.cpp
DEP_CPP_PGBAS=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\DlgFonts.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgbase.obj"	"$(INTDIR)\pgbase.sbr" : $(SOURCE) $(DEP_CPP_PGBAS)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgbase.obj" : $(SOURCE) $(DEP_CPP_PGBAS) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgbase.obj"	"$(INTDIR)\pgbase.sbr" : $(SOURCE) $(DEP_CPP_PGBAS)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgbase.obj" : $(SOURCE) $(DEP_CPP_PGBAS) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgfinish.cpp
DEP_CPP_PGFIN=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgfinish.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgfinish.obj"	"$(INTDIR)\pgfinish.sbr" : $(SOURCE) $(DEP_CPP_PGFIN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgfinish.obj" : $(SOURCE) $(DEP_CPP_PGFIN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgfinish.obj"	"$(INTDIR)\pgfinish.sbr" : $(SOURCE) $(DEP_CPP_PGFIN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgfinish.obj" : $(SOURCE) $(DEP_CPP_PGFIN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgFltKey.cpp
DEP_CPP_PGFLT=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgFltKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgFltKey.obj"	"$(INTDIR)\pgFltKey.sbr" : $(SOURCE) $(DEP_CPP_PGFLT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgFltKey.obj" : $(SOURCE) $(DEP_CPP_PGFLT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgFltKey.obj"	"$(INTDIR)\pgFltKey.sbr" : $(SOURCE) $(DEP_CPP_PGFLT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgFltKey.obj" : $(SOURCE) $(DEP_CPP_PGFLT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgGenric.cpp
DEP_CPP_PGGEN=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgGenric.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgGenric.obj"	"$(INTDIR)\pgGenric.sbr" : $(SOURCE) $(DEP_CPP_PGGEN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgGenric.obj" : $(SOURCE) $(DEP_CPP_PGGEN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgGenric.obj"	"$(INTDIR)\pgGenric.sbr" : $(SOURCE) $(DEP_CPP_PGGEN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgGenric.obj" : $(SOURCE) $(DEP_CPP_PGGEN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgHghCon.cpp
DEP_CPP_PGHGH=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgHghCon.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgHghCon.obj"	"$(INTDIR)\pgHghCon.sbr" : $(SOURCE) $(DEP_CPP_PGHGH)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgHghCon.obj" : $(SOURCE) $(DEP_CPP_PGHGH) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgHghCon.obj"	"$(INTDIR)\pgHghCon.sbr" : $(SOURCE) $(DEP_CPP_PGHGH)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgHghCon.obj" : $(SOURCE) $(DEP_CPP_PGHGH) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgHotKey.cpp
DEP_CPP_PGHOT=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgHotKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgHotKey.obj"	"$(INTDIR)\pgHotKey.sbr" : $(SOURCE) $(DEP_CPP_PGHOT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgHotKey.obj" : $(SOURCE) $(DEP_CPP_PGHOT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgHotKey.obj"	"$(INTDIR)\pgHotKey.sbr" : $(SOURCE) $(DEP_CPP_PGHOT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgHotKey.obj" : $(SOURCE) $(DEP_CPP_PGHOT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgIconSz.cpp
DEP_CPP_PGICO=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgIconSz.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgIconSz.obj"	"$(INTDIR)\pgIconSz.sbr" : $(SOURCE) $(DEP_CPP_PGICO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgIconSz.obj" : $(SOURCE) $(DEP_CPP_PGICO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgIconSz.obj"	"$(INTDIR)\pgIconSz.sbr" : $(SOURCE) $(DEP_CPP_PGICO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgIconSz.obj" : $(SOURCE) $(DEP_CPP_PGICO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgLokPrv.cpp
DEP_CPP_PGLOK=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\LookPrev.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgLokPrv.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgLokPrv.obj"	"$(INTDIR)\pgLokPrv.sbr" : $(SOURCE) $(DEP_CPP_PGLOK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgLokPrv.obj" : $(SOURCE) $(DEP_CPP_PGLOK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgLokPrv.obj"	"$(INTDIR)\pgLokPrv.sbr" : $(SOURCE) $(DEP_CPP_PGLOK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgLokPrv.obj" : $(SOURCE) $(DEP_CPP_PGLOK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgLookWz.cpp
DEP_CPP_PGLOO=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgLookWz.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgLookWz.obj"	"$(INTDIR)\pgLookWz.sbr" : $(SOURCE) $(DEP_CPP_PGLOO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgLookWz.obj" : $(SOURCE) $(DEP_CPP_PGLOO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgLookWz.obj"	"$(INTDIR)\pgLookWz.sbr" : $(SOURCE) $(DEP_CPP_PGLOO)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgLookWz.obj" : $(SOURCE) $(DEP_CPP_PGLOO) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgMinTx2.cpp
DEP_CPP_PGMIN=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgMinTx2.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgMinTx2.obj"	"$(INTDIR)\pgMinTx2.sbr" : $(SOURCE) $(DEP_CPP_PGMIN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgMinTx2.obj" : $(SOURCE) $(DEP_CPP_PGMIN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgMinTx2.obj"	"$(INTDIR)\pgMinTx2.sbr" : $(SOURCE) $(DEP_CPP_PGMIN)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgMinTx2.obj" : $(SOURCE) $(DEP_CPP_PGMIN) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgMinTxt.cpp
DEP_CPP_PGMINT=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgMinTxt.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgMinTxt.obj"	"$(INTDIR)\pgMinTxt.sbr" : $(SOURCE) $(DEP_CPP_PGMINT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgMinTxt.obj" : $(SOURCE) $(DEP_CPP_PGMINT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgMinTxt.obj"	"$(INTDIR)\pgMinTxt.sbr" : $(SOURCE) $(DEP_CPP_PGMINT)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgMinTxt.obj" : $(SOURCE) $(DEP_CPP_PGMINT) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgMseBut.cpp
DEP_CPP_PGMSE=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgMseBut.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgMseBut.obj"	"$(INTDIR)\pgMseBut.sbr" : $(SOURCE) $(DEP_CPP_PGMSE)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgMseBut.obj" : $(SOURCE) $(DEP_CPP_PGMSE) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgMseBut.obj"	"$(INTDIR)\pgMseBut.sbr" : $(SOURCE) $(DEP_CPP_PGMSE)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgMseBut.obj" : $(SOURCE) $(DEP_CPP_PGMSE) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgMseCur.cpp
DEP_CPP_PGMSEC=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgMseCur.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgMseCur.obj"	"$(INTDIR)\pgMseCur.sbr" : $(SOURCE) $(DEP_CPP_PGMSEC)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgMseCur.obj" : $(SOURCE) $(DEP_CPP_PGMSEC) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgMseCur.obj"	"$(INTDIR)\pgMseCur.sbr" : $(SOURCE) $(DEP_CPP_PGMSEC)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgMseCur.obj" : $(SOURCE) $(DEP_CPP_PGMSEC) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgMseKey.cpp
DEP_CPP_PGMSEK=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgMseKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgMseKey.obj"	"$(INTDIR)\pgMseKey.sbr" : $(SOURCE) $(DEP_CPP_PGMSEK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgMseKey.obj" : $(SOURCE) $(DEP_CPP_PGMSEK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgMseKey.obj"	"$(INTDIR)\pgMseKey.sbr" : $(SOURCE) $(DEP_CPP_PGMSEK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgMseKey.obj" : $(SOURCE) $(DEP_CPP_PGMSEK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgScrBar.cpp
DEP_CPP_PGSCR=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgScrBar.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgScrBar.obj"	"$(INTDIR)\pgScrBar.sbr" : $(SOURCE) $(DEP_CPP_PGSCR)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgScrBar.obj" : $(SOURCE) $(DEP_CPP_PGSCR) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgScrBar.obj"	"$(INTDIR)\pgScrBar.sbr" : $(SOURCE) $(DEP_CPP_PGSCR)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgScrBar.obj" : $(SOURCE) $(DEP_CPP_PGSCR) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgSerKey.cpp
DEP_CPP_PGSER=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgSerKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgSerKey.obj"	"$(INTDIR)\pgSerKey.sbr" : $(SOURCE) $(DEP_CPP_PGSER)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgSerKey.obj" : $(SOURCE) $(DEP_CPP_PGSER) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgSerKey.obj"	"$(INTDIR)\pgSerKey.sbr" : $(SOURCE) $(DEP_CPP_PGSER)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgSerKey.obj" : $(SOURCE) $(DEP_CPP_PGSER) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgShwHlp.cpp
DEP_CPP_PGSHW=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgShwHlp.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgShwHlp.obj"	"$(INTDIR)\pgShwHlp.sbr" : $(SOURCE) $(DEP_CPP_PGSHW)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgShwHlp.obj" : $(SOURCE) $(DEP_CPP_PGSHW) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgShwHlp.obj"	"$(INTDIR)\pgShwHlp.sbr" : $(SOURCE) $(DEP_CPP_PGSHW)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgShwHlp.obj" : $(SOURCE) $(DEP_CPP_PGSHW) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgSndSen.cpp
DEP_CPP_PGSND=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgSndSen.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgSndSen.obj"	"$(INTDIR)\pgSndSen.sbr" : $(SOURCE) $(DEP_CPP_PGSND)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgSndSen.obj" : $(SOURCE) $(DEP_CPP_PGSND) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgSndSen.obj"	"$(INTDIR)\pgSndSen.sbr" : $(SOURCE) $(DEP_CPP_PGSND)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgSndSen.obj" : $(SOURCE) $(DEP_CPP_PGSND) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgStkKey.cpp
DEP_CPP_PGSTK=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgStkKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgStkKey.obj"	"$(INTDIR)\pgStkKey.sbr" : $(SOURCE) $(DEP_CPP_PGSTK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgStkKey.obj" : $(SOURCE) $(DEP_CPP_PGSTK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgStkKey.obj"	"$(INTDIR)\pgStkKey.sbr" : $(SOURCE) $(DEP_CPP_PGSTK)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgStkKey.obj" : $(SOURCE) $(DEP_CPP_PGSTK) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgSveDef.cpp
DEP_CPP_PGSVE=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgSveDef.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgSveDef.obj"	"$(INTDIR)\pgSveDef.sbr" : $(SOURCE) $(DEP_CPP_PGSVE)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgSveDef.obj" : $(SOURCE) $(DEP_CPP_PGSVE) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgSveDef.obj"	"$(INTDIR)\pgSveDef.sbr" : $(SOURCE) $(DEP_CPP_PGSVE)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgSveDef.obj" : $(SOURCE) $(DEP_CPP_PGSVE) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgSveFil.cpp
DEP_CPP_PGSVEF=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgSveFil.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgSveFil.obj"	"$(INTDIR)\pgSveFil.sbr" : $(SOURCE) $(DEP_CPP_PGSVEF)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgSveFil.obj" : $(SOURCE) $(DEP_CPP_PGSVEF) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgSveFil.obj"	"$(INTDIR)\pgSveFil.sbr" : $(SOURCE) $(DEP_CPP_PGSVEF)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgSveFil.obj" : $(SOURCE) $(DEP_CPP_PGSVEF) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgTglKey.cpp
DEP_CPP_PGTGL=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgTglKey.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgTglKey.obj"	"$(INTDIR)\pgTglKey.sbr" : $(SOURCE) $(DEP_CPP_PGTGL)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgTglKey.obj" : $(SOURCE) $(DEP_CPP_PGTGL) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgTglKey.obj"	"$(INTDIR)\pgTglKey.sbr" : $(SOURCE) $(DEP_CPP_PGTGL)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgTglKey.obj" : $(SOURCE) $(DEP_CPP_PGTGL) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgTmeOut.cpp
DEP_CPP_PGTME=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgTmeOut.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgTmeOut.obj"	"$(INTDIR)\pgTmeOut.sbr" : $(SOURCE) $(DEP_CPP_PGTME)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgTmeOut.obj" : $(SOURCE) $(DEP_CPP_PGTME) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgTmeOut.obj"	"$(INTDIR)\pgTmeOut.sbr" : $(SOURCE) $(DEP_CPP_PGTME)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgTmeOut.obj" : $(SOURCE) $(DEP_CPP_PGTME) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgWizOpt.cpp
DEP_CPP_PGWIZ=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgWizOpt.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgWizOpt.obj"	"$(INTDIR)\pgWizOpt.sbr" : $(SOURCE) $(DEP_CPP_PGWIZ)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgWizOpt.obj" : $(SOURCE) $(DEP_CPP_PGWIZ) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgWizOpt.obj"	"$(INTDIR)\pgWizOpt.sbr" : $(SOURCE) $(DEP_CPP_PGWIZ)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgWizOpt.obj" : $(SOURCE) $(DEP_CPP_PGWIZ) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\pgWizWiz.cpp
DEP_CPP_PGWIZW=\
	"..\..\..\windows\inc\winuserp.h"\
	".\accwiz.h"\
	".\pch.hxx"\
	".\pgbase.h"\
	".\pgWizWiz.h"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\pgWizWiz.obj"	"$(INTDIR)\pgWizWiz.sbr" : $(SOURCE) $(DEP_CPP_PGWIZW)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\pgWizWiz.obj" : $(SOURCE) $(DEP_CPP_PGWIZW) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\pgWizWiz.obj"	"$(INTDIR)\pgWizWiz.sbr" : $(SOURCE) $(DEP_CPP_PGWIZW)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\pgWizWiz.obj" : $(SOURCE) $(DEP_CPP_PGWIZW) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 

SOURCE=.\Schemes.cpp
DEP_CPP_SCHEM=\
	"..\..\..\windows\inc\winuserp.h"\
	".\pch.hxx"\
	".\Schemes.h"\
	

!IF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Debug"


"$(INTDIR)\Schemes.obj"	"$(INTDIR)\Schemes.sbr" : $(SOURCE) $(DEP_CPP_SCHEM)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 ANSI Win97 Release"


"$(INTDIR)\Schemes.obj" : $(SOURCE) $(DEP_CPP_SCHEM) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Debug"


"$(INTDIR)\Schemes.obj"	"$(INTDIR)\Schemes.sbr" : $(SOURCE) $(DEP_CPP_SCHEM)\
 "$(INTDIR)" "$(INTDIR)\accwiz.pch"


!ELSEIF  "$(CFG)" == "AccWiz - Win32 UNICODE NT5 Release"


"$(INTDIR)\Schemes.obj" : $(SOURCE) $(DEP_CPP_SCHEM) "$(INTDIR)"\
 "$(INTDIR)\accwiz.pch"


!ENDIF 


!ENDIF 

