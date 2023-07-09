# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (PPC) Application" 0x0701
# TARGTYPE "Win32 (MIPS) Application" 0x0501
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

!IF "$(CFG)" == ""
CFG=NetClip - Win32 (ALPHA) Debug Unicode
!MESSAGE No configuration specified.  Defaulting to NetClip - Win32 (ALPHA)\
 Debug Unicode.
!ENDIF 

!IF "$(CFG)" != "NetClip - Win32 Debug" && "$(CFG)" !=\
 "NetClip - Win32 MIPS Debug" && "$(CFG)" != "NetClip - Win32 Release" &&\
 "$(CFG)" != "NetClip - Win32 Release Unicode" && "$(CFG)" !=\
 "NetClip - Win32 Debug Unicode" && "$(CFG)" !=\
 "NetClip - Win32 (PPC) Release Unicode" && "$(CFG)" !=\
 "NetClip - Win32 (PPC) Debug Unicode" && "$(CFG)" !=\
 "NetClip - Win32 (ALPHA) Debug Unicode" && "$(CFG)" !=\
 "NetClip - Win32 (ALPHA) Release Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "netclip.mak" CFG="NetClip - Win32 (ALPHA) Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NetClip - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "NetClip - Win32 MIPS Debug" (based on "Win32 (MIPS) Application")
!MESSAGE "NetClip - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "NetClip - Win32 Release Unicode" (based on "Win32 (x86) Application")
!MESSAGE "NetClip - Win32 Debug Unicode" (based on "Win32 (x86) Application")
!MESSAGE "NetClip - Win32 (PPC) Release Unicode" (based on\
 "Win32 (PPC) Application")
!MESSAGE "NetClip - Win32 (PPC) Debug Unicode" (based on\
 "Win32 (PPC) Application")
!MESSAGE "NetClip - Win32 (ALPHA) Debug Unicode" (based on\
 "Win32 (ALPHA) Application")
!MESSAGE "NetClip - Win32 (ALPHA) Release Unicode" (based on\
 "Win32 (ALPHA) Application")
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
# PROP Target_Last_Scanned "NetClip - Win32 Debug"

!IF  "$(CFG)" == "NetClip - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\netclip.exe" "$(OUTDIR)\netclip.bsc"

CLEAN : 
	-@erase "$(INTDIR)\cntritem.obj"
	-@erase "$(INTDIR)\cntritem.sbr"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\dataobj.sbr"
	-@erase "$(INTDIR)\doc.obj"
	-@erase "$(INTDIR)\doc.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\NetClip.obj"
	-@erase "$(INTDIR)\netclip.pch"
	-@erase "$(INTDIR)\NetClip.res"
	-@erase "$(INTDIR)\NetClip.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\svrdlg.obj"
	-@erase "$(INTDIR)\svrdlg.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view.sbr"
	-@erase "$(OUTDIR)\netclip.bsc"
	-@erase "$(OUTDIR)\netclip.exe"
	-@erase "$(OUTDIR)\netclip.ilk"
	-@erase "$(OUTDIR)\netclip.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/netclip.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cntritem.sbr" \
	"$(INTDIR)\dataobj.sbr" \
	"$(INTDIR)\doc.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\NetClip.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\svrdlg.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\view.sbr"

"$(OUTDIR)\netclip.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:IX86
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 version.lib /nologo /subsystem:windows /debug /machine:IX86
# SUBTRACT LINK32 /incremental:no
LINK32_FLAGS=version.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/netclip.pdb" /debug /machine:IX86 /out:"$(OUTDIR)/netclip.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cntritem.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\doc.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NetClip.obj" \
	"$(INTDIR)\NetClip.res" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrdlg.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\netclip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "d:\source\NetClip\MIPSDbg"
# PROP Intermediate_Dir "d:\source\NetClip\MIPSDbg"
# PROP Target_Dir ""
OUTDIR=d:\source\NetClip\MIPSDbg
INTDIR=d:\source\NetClip\MIPSDbg

ALL :    "$(OUTDIR)\NetClip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CLEAN : 
	-@erase "d:\source\NetClip\MIPSDbg\vc40.pdb"
	-@erase "d:\source\NetClip\MIPSDbg\NetClip.exe"
	-@erase "d:\source\NetClip\MIPSDbg\NetClip.obj"
	-@erase "d:\source\NetClip\MIPSDbg\StdAfx.obj"
	-@erase "d:\source\NetClip\MIPSDbg\MainFrm.obj"
	-@erase "d:\source\NetClip\MIPSDbg\NetClipDoc.obj"
	-@erase "d:\source\NetClip\MIPSDbg\util.obj"
	-@erase "d:\source\NetClip\MIPSDbg\cntritem.obj"
	-@erase "d:\source\NetClip\MIPSDbg\NetClipView.obj"
	-@erase "d:\source\NetClip\MIPSDbg\ServerInfoDlg.obj"
	-@erase "d:\source\NetClip\MIPSDbg\NetClipServer.obj"
	-@erase "d:\source\NetClip\MIPSDbg\NetClip.res"
	-@erase "d:\source\NetClip\MIPSDbg\NetClip.ilk"
	-@erase "d:\source\NetClip\MIPSDbg\NetClip.pdb"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MDd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MDd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/NetClip.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=d:\source\NetClip\MIPSDbg/
CPP_SBRS=

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
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL" 
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:MIPS
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 /nologo /subsystem:windows /debug /machine:MIPS
# SUBTRACT LINK32 /incremental:no
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/NetClip.pdb" /debug /machine:MIPS /out:"$(OUTDIR)/NetClip.exe" 
LINK32_OBJS= \
	"$(INTDIR)/NetClip.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/NetClipDoc.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/cntritem.obj" \
	"$(INTDIR)/NetClipView.obj" \
	"$(INTDIR)/ServerInfoDlg.obj" \
	"$(INTDIR)/NetClipServer.obj" \
	"$(INTDIR)/NetClip.res"

"$(OUTDIR)\NetClip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NetClip.bsc" 
BSC32_SBRS=

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\netclip.exe"

CLEAN : 
	-@erase "$(INTDIR)\cntritem.obj"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\doc.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\NetClip.obj"
	-@erase "$(INTDIR)\netclip.pch"
	-@erase "$(INTDIR)\NetClip.res"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrdlg.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(OUTDIR)\netclip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /c 
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
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/netclip.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:IX86
# ADD LINK32 version.lib /nologo /subsystem:windows /machine:IX86
LINK32_FLAGS=version.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/netclip.pdb" /machine:IX86 /out:"$(OUTDIR)/netclip.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cntritem.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\doc.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NetClip.obj" \
	"$(INTDIR)\NetClip.res" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrdlg.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\netclip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "URel"
# PROP Intermediate_Dir "URel"
# PROP Target_Dir ""
OUTDIR=.\URel
INTDIR=.\URel

ALL : "$(OUTDIR)\netclip.exe" "$(OUTDIR)\netclip.bsc"

CLEAN : 
	-@erase "$(INTDIR)\cntritem.obj"
	-@erase "$(INTDIR)\cntritem.sbr"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\dataobj.sbr"
	-@erase "$(INTDIR)\doc.obj"
	-@erase "$(INTDIR)\doc.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\NetClip.obj"
	-@erase "$(INTDIR)\netclip.pch"
	-@erase "$(INTDIR)\NetClip.res"
	-@erase "$(INTDIR)\NetClip.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\svrdlg.obj"
	-@erase "$(INTDIR)\svrdlg.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view.sbr"
	-@erase "$(OUTDIR)\netclip.bsc"
	-@erase "$(OUTDIR)\netclip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W4 /WX /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W4 /WX /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/netclip.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\URel/
CPP_SBRS=.\URel/

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
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/netclip.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cntritem.sbr" \
	"$(INTDIR)\dataobj.sbr" \
	"$(INTDIR)\doc.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\NetClip.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\svrdlg.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\view.sbr"

"$(OUTDIR)\netclip.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:IX86
# ADD LINK32 version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:IX86
LINK32_FLAGS=version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/netclip.pdb" /machine:IX86\
 /out:"$(OUTDIR)/netclip.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cntritem.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\doc.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NetClip.obj" \
	"$(INTDIR)\NetClip.res" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrdlg.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\netclip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NetClip0"
# PROP BASE Intermediate_Dir "NetClip0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UDebug"
# PROP Intermediate_Dir "UDebug"
# PROP Target_Dir ""
OUTDIR=.\UDebug
INTDIR=.\UDebug

ALL : "$(OUTDIR)\netclip.exe" "$(OUTDIR)\netclip.bsc"

CLEAN : 
	-@erase "$(INTDIR)\cntritem.obj"
	-@erase "$(INTDIR)\cntritem.sbr"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\dataobj.sbr"
	-@erase "$(INTDIR)\doc.obj"
	-@erase "$(INTDIR)\doc.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\NetClip.obj"
	-@erase "$(INTDIR)\netclip.pch"
	-@erase "$(INTDIR)\NetClip.res"
	-@erase "$(INTDIR)\NetClip.sbr"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\svrdlg.obj"
	-@erase "$(INTDIR)\svrdlg.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view.sbr"
	-@erase "$(OUTDIR)\netclip.bsc"
	-@erase "$(OUTDIR)\netclip.exe"
	-@erase "$(OUTDIR)\netclip.ilk"
	-@erase "$(OUTDIR)\netclip.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\UDebug/
CPP_SBRS=.\UDebug/

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
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/netclip.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cntritem.sbr" \
	"$(INTDIR)\dataobj.sbr" \
	"$(INTDIR)\doc.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\NetClip.sbr" \
	"$(INTDIR)\server.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\svrdlg.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\view.sbr"

"$(OUTDIR)\netclip.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:IX86
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:IX86
# SUBTRACT LINK32 /incremental:no
LINK32_FLAGS=version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/netclip.pdb" /debug /machine:IX86\
 /out:"$(OUTDIR)/netclip.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cntritem.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\doc.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NetClip.obj" \
	"$(INTDIR)\NetClip.res" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrdlg.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\view.obj"

"$(OUTDIR)\netclip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "d:\source\netclip\UPPCRel"
# PROP Intermediate_Dir "d:\source\netclip\UPPCRel"
# PROP Target_Dir ""
OUTDIR=d:\source\netclip\UPPCRel
INTDIR=d:\source\netclip\UPPCRel

ALL :   "$(OUTDIR)\NetClip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CLEAN : 
	-@erase "d:\source\netclip\UPPCRel\NetClip.exe"
	-@erase "d:\source\netclip\UPPCRel\NetClip.obj"
	-@erase "d:\source\netclip\UPPCRel\NetClip.pch"
	-@erase "d:\source\netclip\UPPCRel\util.obj"
	-@erase "d:\source\netclip\UPPCRel\NetClipView.obj"
	-@erase "d:\source\netclip\UPPCRel\NetClipDoc.obj"
	-@erase "d:\source\netclip\UPPCRel\StdAfx.obj"
	-@erase "d:\source\netclip\UPPCRel\GenericDataObject.obj"
	-@erase "d:\source\netclip\UPPCRel\MainFrm.obj"
	-@erase "d:\source\netclip\UPPCRel\NetClipServer.obj"
	-@erase "d:\source\netclip\UPPCRel\ServerInfoDlg.obj"
	-@erase "d:\source\netclip\UPPCRel\cntritem.obj"
	-@erase "d:\source\netclip\UPPCRel\NetClip.res"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /PPC32
# ADD MTL /nologo /D "NDEBUG" /PPC32
MTL_PROJ=/nologo /D "NDEBUG" /PPC32 
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)/NetClip.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=d:\source\netclip\UPPCRel/
CPP_SBRS=

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
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NetClip.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:PPC
# ADD LINK32 version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:PPC
LINK32_FLAGS=version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /pdb:"$(OUTDIR)/NetClip.pdb" /machine:PPC /out:"$(OUTDIR)/NetClip.exe" 
LINK32_OBJS= \
	"$(INTDIR)/NetClip.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/NetClipView.obj" \
	"$(INTDIR)/NetClipDoc.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/GenericDataObject.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/NetClipServer.obj" \
	"$(INTDIR)/ServerInfoDlg.obj" \
	"$(INTDIR)/cntritem.obj" \
	"$(INTDIR)/NetClip.res"

"$(OUTDIR)\NetClip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NetClip0"
# PROP BASE Intermediate_Dir "NetClip0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "d:\source\netclip\UPPCDebug"
# PROP Intermediate_Dir "d:\source\netclip\UPPCDebug"
# PROP Target_Dir ""
OUTDIR=d:\source\netclip\UPPCDebug
INTDIR=d:\source\netclip\UPPCDebug

ALL :   "$(OUTDIR)\NetClip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CLEAN : 
	-@erase "d:\source\netclip\UPPCDebug\vc40.pdb"
	-@erase "d:\source\netclip\UPPCDebug\NetClip.pch"
	-@erase "d:\source\netclip\UPPCDebug\NetClip.exe"
	-@erase "d:\source\netclip\UPPCDebug\NetClip.obj"
	-@erase "d:\source\netclip\UPPCDebug\NetClipServer.obj"
	-@erase "d:\source\netclip\UPPCDebug\NetClipView.obj"
	-@erase "d:\source\netclip\UPPCDebug\MainFrm.obj"
	-@erase "d:\source\netclip\UPPCDebug\StdAfx.obj"
	-@erase "d:\source\netclip\UPPCDebug\NetClipDoc.obj"
	-@erase "d:\source\netclip\UPPCDebug\util.obj"
	-@erase "d:\source\netclip\UPPCDebug\ServerInfoDlg.obj"
	-@erase "d:\source\netclip\UPPCDebug\GenericDataObject.obj"
	-@erase "d:\source\netclip\UPPCDebug\cntritem.obj"
	-@erase "d:\source\netclip\UPPCDebug\NetClip.res"
	-@erase "d:\source\netclip\UPPCDebug\NetClip.pdb"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /PPC32
# ADD MTL /nologo /D "_DEBUG" /PPC32
MTL_PROJ=/nologo /D "_DEBUG" /PPC32 
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MDd /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)/NetClip.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=d:\source\netclip\UPPCDebug/
CPP_SBRS=

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
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NetClip.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:PPC
# ADD LINK32 version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:PPC
LINK32_FLAGS=version.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /pdb:"$(OUTDIR)/NetClip.pdb" /debug /machine:PPC /out:"$(OUTDIR)/NetClip.exe" 
LINK32_OBJS= \
	"$(INTDIR)/NetClip.obj" \
	"$(INTDIR)/NetClipServer.obj" \
	"$(INTDIR)/NetClipView.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/NetClipDoc.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/ServerInfoDlg.obj" \
	"$(INTDIR)/GenericDataObject.obj" \
	"$(INTDIR)/cntritem.obj" \
	"$(INTDIR)/NetClip.res"

"$(OUTDIR)\NetClip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "NetClip_"
# PROP BASE Intermediate_Dir "NetClip_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AlphaDbg"
# PROP Intermediate_Dir "c:\temp"
# PROP Target_Dir ""
OUTDIR=.\AlphaDbg
INTDIR=c:\temp

ALL :   "$(OUTDIR)\NetClip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CLEAN : 
	-@erase "c:\temp\vc40.pdb"
	-@erase "c:\temp\NetClip.pch"
	-@erase ".\AlphaDbg\NetClip.exe"
	-@erase "c:\temp\NetClip.obj"
	-@erase "c:\temp\StdAfx.obj"
	-@erase "c:\temp\MainFrm.obj"
	-@erase "c:\temp\NetClipDoc.obj"
	-@erase "c:\temp\util.obj"
	-@erase "c:\temp\cntritem.obj"
	-@erase "c:\temp\NetClipView.obj"
	-@erase "c:\temp\ServerInfoDlg.obj"
	-@erase "c:\temp\NetClipServer.obj"
	-@erase "c:\temp\GenericDataObject.obj"
	-@erase "c:\temp\NetClip.res"
	-@erase ".\AlphaDbg\NetClip.ilk"
	-@erase ".\AlphaDbg\NetClip.pdb"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MDd /Gt0 /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /Gt0 /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/NetClip.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=c:\temp/
CPP_SBRS=

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
# ADD BASE MTL /nologo /D "_DEBUG" /alpha
# ADD MTL /nologo /D "_DEBUG" /alpha
MTL_PROJ=/nologo /D "_DEBUG" /alpha 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NetClip.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:ALPHA
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 version.lib /nologo /subsystem:windows /debug /machine:ALPHA
# SUBTRACT LINK32 /incremental:no
LINK32_FLAGS=version.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/NetClip.pdb" /debug /machine:ALPHA /out:"$(OUTDIR)/NetClip.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)/NetClip.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/NetClipDoc.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/cntritem.obj" \
	"$(INTDIR)/NetClipView.obj" \
	"$(INTDIR)/ServerInfoDlg.obj" \
	"$(INTDIR)/NetClipServer.obj" \
	"$(INTDIR)/GenericDataObject.obj" \
	"$(INTDIR)/NetClip.res"

"$(OUTDIR)\NetClip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NetClip0"
# PROP BASE Intermediate_Dir "NetClip0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AlphaRel"
# PROP Intermediate_Dir "c:\temp"
# PROP Target_Dir ""
OUTDIR=.\AlphaRel
INTDIR=c:\temp

ALL :   "$(OUTDIR)\NetClip.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CLEAN : 
	-@erase ".\AlphaRel\NetClip.exe"
	-@erase "c:\temp\NetClip.obj"
	-@erase "c:\temp\NetClip.pch"
	-@erase "c:\temp\StdAfx.obj"
	-@erase "c:\temp\MainFrm.obj"
	-@erase "c:\temp\NetClipDoc.obj"
	-@erase "c:\temp\util.obj"
	-@erase "c:\temp\cntritem.obj"
	-@erase "c:\temp\NetClipView.obj"
	-@erase "c:\temp\ServerInfoDlg.obj"
	-@erase "c:\temp\NetClipServer.obj"
	-@erase "c:\temp\GenericDataObject.obj"
	-@erase "c:\temp\NetClip.res"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX /c
# ADD CPP /nologo /MD /Gt0 /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /Gt0 /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/NetClip.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=c:\temp/
CPP_SBRS=

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
# ADD BASE MTL /nologo /D "NDEBUG" /alpha
# ADD MTL /nologo /D "NDEBUG" /alpha
MTL_PROJ=/nologo /D "NDEBUG" /alpha 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/NetClip.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:ALPHA
# ADD LINK32 version.lib /nologo /subsystem:windows /machine:ALPHA
LINK32_FLAGS=version.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/NetClip.pdb" /machine:ALPHA /out:"$(OUTDIR)/NetClip.exe" 
LINK32_OBJS= \
	"$(INTDIR)/NetClip.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/MainFrm.obj" \
	"$(INTDIR)/NetClipDoc.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/cntritem.obj" \
	"$(INTDIR)/NetClipView.obj" \
	"$(INTDIR)/ServerInfoDlg.obj" \
	"$(INTDIR)/NetClipServer.obj" \
	"$(INTDIR)/GenericDataObject.obj" \
	"$(INTDIR)/NetClip.res"

"$(OUTDIR)\NetClip.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "NetClip - Win32 Debug"
# Name "NetClip - Win32 MIPS Debug"
# Name "NetClip - Win32 Release"
# Name "NetClip - Win32 Release Unicode"
# Name "NetClip - Win32 Debug Unicode"
# Name "NetClip - Win32 (PPC) Release Unicode"
# Name "NetClip - Win32 (PPC) Debug Unicode"
# Name "NetClip - Win32 (ALPHA) Debug Unicode"
# Name "NetClip - Win32 (ALPHA) Release Unicode"

!IF  "$(CFG)" == "NetClip - Win32 Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "NetClip - Win32 Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NetClip.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_NETCL=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\NetClip.sbr" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_CPP_NETCL=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_NETCL=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_NETCL=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\NetClip.sbr" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_NETCL=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\NetClip.sbr" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_CPP_NETCL=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_CPP_NETCL=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_CPP_NETCL=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_CPP_NETCL=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\NetClip.obj" : $(SOURCE) $(DEP_CPP_NETCL) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_STDAF=\
	"..\\proxy\\NetClip.h"\
	".\guids.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\netclip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_CPP_STDAF=\
	".\guids.h"\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_STDAF=\
	"..\\proxy\\NetClip.h"\
	".\guids.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/netclip.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\netclip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_STDAF=\
	"..\\proxy\\NetClip.h"\
	".\guids.h"\
	".\stdafx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W4 /WX /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/netclip.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\netclip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_STDAF=\
	"..\\proxy\\NetClip.h"\
	".\guids.h"\
	".\stdafx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\netclip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_CPP_STDAF=\
	".\guids.h"\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"
BuildCmds= \
	$(CPP) /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)/NetClip.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\NetClip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_CPP_STDAF=\
	".\guids.h"\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"
BuildCmds= \
	$(CPP) /nologo /MDd /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)/NetClip.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\NetClip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_CPP_STDAF=\
	".\guids.h"\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	
# ADD BASE CPP /Gt0
# ADD CPP /Gt0 /Yc"stdafx.h"
BuildCmds= \
	$(CPP) /nologo /MDd /Gt0 /W4 /WX /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/NetClip.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\NetClip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_CPP_STDAF=\
	".\guids.h"\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	
# ADD BASE CPP /Gt0
# ADD CPP /Gt0 /Yc"stdafx.h"
BuildCmds= \
	$(CPP) /nologo /MD /Gt0 /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/NetClip.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\NetClip.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_MAINF=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_CPP_MAINF=\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\ServerInfoDlg.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_MAINF=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_MAINF=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_MAINF=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_CPP_MAINF=\
	".\..\\ole2view\\iviewers\\iview.h"\
	".\GenericDataObject.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\ServerInfoDlg.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_CPP_MAINF=\
	".\..\\ole2view\\iviewers\\iview.h"\
	".\GenericDataObject.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\ServerInfoDlg.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_CPP_MAINF=\
	".\..\\ole2view\\iviewers\\iview.h"\
	".\GenericDataObject.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\ServerInfoDlg.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_CPP_MAINF=\
	".\..\\ole2view\\iviewers\\iview.h"\
	".\GenericDataObject.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipServer.h"\
	".\NetClipView.h"\
	".\ServerInfoDlg.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NetClip.rc

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_RSC_NETCLI=\
	".\doc.ico"\
	".\idr_main.ico"\
	".\mainfram.bmp"\
	".\netclip.ico"\
	".\netclip.rc2"\
	".\toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_RSC_NETCLI=\
	".\res\bitmap1.bmp"\
	".\res\NetClip.ico"\
	".\res\NetClip.rc2"\
	".\res\NetClipDoc.ico"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLIP) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_RSC_NETCLI=\
	".\doc.ico"\
	".\idr_main.ico"\
	".\mainfram.bmp"\
	".\netclip.ico"\
	".\netclip.rc2"\
	".\toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_RSC_NETCLI=\
	".\doc.ico"\
	".\idr_main.ico"\
	".\mainfram.bmp"\
	".\netclip.ico"\
	".\netclip.rc2"\
	".\toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_RSC_NETCLI=\
	".\doc.ico"\
	".\idr_main.ico"\
	".\mainfram.bmp"\
	".\netclip.ico"\
	".\netclip.rc2"\
	".\toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_RSC_NETCLI=\
	".\res\bitmap1.bmp"\
	".\res\idr_main.ico"\
	".\res\mainfram.bmp"\
	".\res\NetClip.ico"\
	".\res\NetClip.rc2"\
	".\res\NetClipDoc.ico"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLIP) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_RSC_NETCLI=\
	".\res\bitmap1.bmp"\
	".\res\idr_main.ico"\
	".\res\mainfram.bmp"\
	".\res\NetClip.ico"\
	".\res\NetClip.rc2"\
	".\res\NetClipDoc.ico"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLIP) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_RSC_NETCLI=\
	".\res\bitmap1.bmp"\
	".\res\idr_main.ico"\
	".\res\mainfram.bmp"\
	".\res\NetClip.ico"\
	".\res\NetClip.rc2"\
	".\res\NetClipDoc.ico"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLIP) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/NetClip.res" /d "_DEBUG" /d "_AFXDLL"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_RSC_NETCLI=\
	".\res\bitmap1.bmp"\
	".\res\idr_main.ico"\
	".\res\mainfram.bmp"\
	".\res\NetClip.ico"\
	".\res\NetClip.rc2"\
	".\res\NetClipDoc.ico"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\NetClip.res" : $(SOURCE) $(DEP_RSC_NETCLIP) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/NetClip.res" /d "NDEBUG" /d "_AFXDLL"\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_UTIL_=\
	"..\\proxy\\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	
# ADD CPP /Yu

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x400 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_CPP_UTIL_=\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_UTIL_=\
	"..\\proxy\\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(CPP) /nologo /MD /W4 /WX /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /c $(SOURCE)


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_UTIL_=\
	"..\\proxy\\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

BuildCmds= \
	$(CPP) /nologo /MD /W4 /WX /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/netclip.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_UTIL_=\
	"..\\proxy\\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	
# ADD BASE CPP /Yu
# ADD CPP /Yu

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /WX /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/netclip.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_CPP_UTIL_=\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_CPP_UTIL_=\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_CPP_UTIL_=\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_CPP_UTIL_=\
	".\NetClipPS\NetClip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cntritem.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_CNTRI=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\doc.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\cntritem.sbr" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

DEP_CPP_CNTRI=\
	".\cntritem.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_CNTRI=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\doc.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_CNTRI=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\doc.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\cntritem.sbr" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_CNTRI=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\doc.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\cntritem.sbr" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

DEP_CPP_CNTRI=\
	".\cntritem.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

DEP_CPP_CNTRI=\
	".\cntritem.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

DEP_CPP_CNTRI=\
	".\cntritem.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

DEP_CPP_CNTRI=\
	".\cntritem.h"\
	".\netclip.h"\
	".\NetClipDoc.h"\
	".\NetClipPS\NetClip.h"\
	".\NetClipView.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\cntritem.obj" : $(SOURCE) $(DEP_CPP_CNTRI) "$(INTDIR)"\
 "$(INTDIR)\NetClip.pch"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dataobj.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_DATAO=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\guids.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\dataobj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\dataobj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_DATAO=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\guids.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\dataobj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_DATAO=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\guids.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\dataobj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\dataobj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_DATAO=\
	"..\\proxy\\NetClip.h"\
	".\dataobj.h"\
	".\guids.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\dataobj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\dataobj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\view.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_VIEW_=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\view.sbr" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_VIEW_=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_VIEW_=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\view.sbr" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_VIEW_=\
	"..\\proxy\\NetClip.h"\
	".\doc.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\view.obj" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\view.sbr" : $(SOURCE) $(DEP_CPP_VIEW_) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\server.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_SERVE=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\server.obj" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\server.sbr" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_SERVE=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\server.obj" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_SERVE=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\server.obj" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\server.sbr" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_SERVE=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\server.h"\
	".\stdafx.h"\
	".\util.h"\
	

"$(INTDIR)\server.obj" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\server.sbr" : $(SOURCE) $(DEP_CPP_SERVE) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\doc.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_DOC_C=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\doc.sbr" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_DOC_C=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_DOC_C=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\doc.sbr" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_DOC_C=\
	"..\\proxy\\NetClip.h"\
	".\cntritem.h"\
	".\dataobj.h"\
	".\doc.h"\
	".\guids.h"\
	".\mainfrm.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\util.h"\
	".\view.h"\
	

"$(INTDIR)\doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\doc.sbr" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\svrdlg.cpp

!IF  "$(CFG)" == "NetClip - Win32 Debug"

DEP_CPP_SVRDL=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	

"$(INTDIR)\svrdlg.obj" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\svrdlg.sbr" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "NetClip - Win32 Release"

DEP_CPP_SVRDL=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	

"$(INTDIR)\svrdlg.obj" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Release Unicode"

DEP_CPP_SVRDL=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	

"$(INTDIR)\svrdlg.obj" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\svrdlg.sbr" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 Debug Unicode"

DEP_CPP_SVRDL=\
	"..\\proxy\\NetClip.h"\
	".\netclip.h"\
	".\stdafx.h"\
	".\svrdlg.h"\
	".\util.h"\
	

"$(INTDIR)\svrdlg.obj" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"

"$(INTDIR)\svrdlg.sbr" : $(SOURCE) $(DEP_CPP_SVRDL) "$(INTDIR)"\
 "$(INTDIR)\netclip.pch"


!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Release Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (PPC) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Debug Unicode"

!ELSEIF  "$(CFG)" == "NetClip - Win32 (ALPHA) Release Unicode"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
