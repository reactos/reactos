# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (MIPS) Application" 0x0501

!IF "$(CFG)" == ""
CFG=taskmgr - Win32 MIPSRetail
!MESSAGE No configuration specified.  Defaulting to taskmgr - Win32 MIPSRetail.
!ENDIF 

!IF "$(CFG)" != "taskmgr - Win32 Release" && "$(CFG)" !=\
 "taskmgr - Win32 Debug" && "$(CFG)" != "taskmgr - Win32 MIPSDebug" && "$(CFG)"\
 != "taskmgr - Win32 MIPSRetail"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "taskmgr.mak" CFG="taskmgr - Win32 MIPSRetail"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "taskmgr - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "taskmgr - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "taskmgr - Win32 MIPSDebug" (based on "Win32 (MIPS) Application")
!MESSAGE "taskmgr - Win32 MIPSRetail" (based on "Win32 (MIPS) Application")
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
# PROP Target_Last_Scanned "taskmgr - Win32 Debug"

!IF  "$(CFG)" == "taskmgr - Win32 Release"

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

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\taskmgr.exe" ".\Release\taskmgr.bsc"

CLEAN : 
	-@erase ".\Release\taskmgr.bsc"
	-@erase ".\Release\ptrarray.sbr"
	-@erase ".\Release\trayicon.sbr"
	-@erase ".\Release\procpage.sbr"
	-@erase ".\Release\taskpage.sbr"
	-@erase ".\Release\main.sbr"
	-@erase ".\Release\perfpage.sbr"
	-@erase ".\obj\i386\taskmgr.exe"
	-@erase ".\Release\perfpage.obj"
	-@erase ".\Release\ptrarray.obj"
	-@erase ".\Release\trayicon.obj"
	-@erase ".\Release\procpage.obj"
	-@erase ".\Release\taskpage.obj"
	-@erase ".\Release\main.obj"
	-@erase ".\Release\taskmgr.res"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /W4 /O2 /X /I "n:\nt\public\sdk\inc" /I "n:\nt\public\sdk\inc\crt" /I "n:\nt\private\windows\inc" /I "n:\nt\private\windows\inc16" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D WINNT=1 /D "_X86_" /D DEVL=1 /FR /YX"precomp.h" /c
CPP_PROJ=/nologo /Gz /ML /W4 /O2 /X /I "n:\nt\public\sdk\inc" /I\
 "n:\nt\public\sdk\inc\crt" /I "n:\nt\private\windows\inc" /I\
 "n:\nt\private\windows\inc16" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D WINNT=1\
 /D "_X86_" /D DEVL=1 /FR"$(INTDIR)/" /Fp"$(INTDIR)/taskmgr.pch" /YX"precomp.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

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
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/taskmgr.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ptrarray.sbr" \
	"$(INTDIR)/trayicon.sbr" \
	"$(INTDIR)/procpage.sbr" \
	"$(INTDIR)/taskpage.sbr" \
	"$(INTDIR)/main.sbr" \
	"$(INTDIR)/perfpage.sbr"

".\Release\taskmgr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib n:\nt\public\sdk\lib\i386\user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib ntdll.lib n:\nt\public\sdk\lib\i386\user32p.lib n:\nt\public\sdk\lib\i386\int64.lib /nologo /entry:"ModuleEntry" /subsystem:windows /machine:I386 /nodefaultlib /out:"obj\i386\taskmg.exe"
LINK32_FLAGS=kernel32.lib n:\nt\public\sdk\lib\i386\user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib\
 comctl32.lib ntdll.lib n:\nt\public\sdk\lib\i386\user32p.lib\
 n:\nt\public\sdk\lib\i386\int64.lib /nologo /entry:"ModuleEntry"\
 /subsystem:windows /incremental:no /pdb:"$(OUTDIR)/taskmgr.pdb" /machine:I386\
 /nodefaultlib /out:"obj\i386\taskmgr.exe" 
LINK32_OBJS= \
	"$(INTDIR)/perfpage.obj" \
	"$(INTDIR)/ptrarray.obj" \
	"$(INTDIR)/trayicon.obj" \
	"$(INTDIR)/procpage.obj" \
	"$(INTDIR)/taskpage.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/taskmgr.res"

"$(OUTDIR)\taskmgr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

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

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\taskmgr.exe" "$(OUTDIR)\taskmgr.bsc"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\taskmgr.bsc"
	-@erase ".\Debug\ptrarray.sbr"
	-@erase ".\Debug\taskpage.sbr"
	-@erase ".\Debug\main.sbr"
	-@erase ".\Debug\perfpage.sbr"
	-@erase ".\Debug\trayicon.sbr"
	-@erase ".\Debug\procpage.sbr"
	-@erase ".\Debug\taskmgr.exe"
	-@erase ".\Debug\perfpage.obj"
	-@erase ".\Debug\trayicon.obj"
	-@erase ".\Debug\procpage.obj"
	-@erase ".\Debug\ptrarray.obj"
	-@erase ".\Debug\taskpage.obj"
	-@erase ".\Debug\main.obj"
	-@erase ".\Debug\taskmgr.res"
	-@erase ".\Debug\taskmgr.ilk"
	-@erase ".\Debug\taskmgr.pdb"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /W4 /Gm /Zi /Od /X /I "n:\nt\public\sdk\inc" /I "n:\nt\public\sdk\inc\crt" /I "n:\nt\private\windows\inc" /I "n:\nt\private\windows\inc16" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D WINNT=1 /D "_X86_" /D DEVL=1 /FR /YX"precomp.h" /c
CPP_PROJ=/nologo /Gz /MLd /W4 /Gm /Zi /Od /X /I "n:\nt\public\sdk\inc" /I\
 "n:\nt\public\sdk\inc\crt" /I "n:\nt\private\windows\inc" /I\
 "n:\nt\private\windows\inc16" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D WINNT=1\
 /D "_X86_" /D DEVL=1 /FR"$(INTDIR)/" /Fp"$(INTDIR)/taskmgr.pch" /YX"precomp.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
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
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/taskmgr.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/ptrarray.sbr" \
	"$(INTDIR)/taskpage.sbr" \
	"$(INTDIR)/main.sbr" \
	"$(INTDIR)/perfpage.sbr" \
	"$(INTDIR)/trayicon.sbr" \
	"$(INTDIR)/procpage.sbr"

"$(OUTDIR)\taskmgr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib n:\nt\public\sdk\lib\i386\user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib comctl32.lib ntdll.lib n:\nt\public\sdk\lib\i386\user32p.lib n:\nt\public\sdk\lib\i386\int64.lib /nologo /entry:"ModuleEntry" /subsystem:windows /debug /machine:I386 /nodefaultlib
# SUBTRACT LINK32 /force
LINK32_FLAGS=kernel32.lib n:\nt\public\sdk\lib\i386\user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib\
 comctl32.lib ntdll.lib n:\nt\public\sdk\lib\i386\user32p.lib\
 n:\nt\public\sdk\lib\i386\int64.lib /nologo /entry:"ModuleEntry"\
 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)/taskmgr.pdb" /debug\
 /machine:I386 /nodefaultlib /out:"$(OUTDIR)/taskmgr.exe" 
LINK32_OBJS= \
	"$(INTDIR)/perfpage.obj" \
	"$(INTDIR)/trayicon.obj" \
	"$(INTDIR)/procpage.obj" \
	"$(INTDIR)/ptrarray.obj" \
	"$(INTDIR)/taskpage.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/taskmgr.res"

"$(OUTDIR)\taskmgr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "taskmgr__"
# PROP BASE Intermediate_Dir "taskmgr__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "mipsdbg"
# PROP Intermediate_Dir "mipsdbg"
# PROP Target_Dir ""
OUTDIR=.\mipsdbg
INTDIR=.\mipsdbg

ALL : "$(OUTDIR)\taskmgr.exe" "$(OUTDIR)\taskmgr.bsc"

CLEAN : 
	-@erase ".\mipsdbg\vc40.pdb"
	-@erase ".\mipsdbg\taskmgr.exe"
	-@erase ".\mipsdbg\taskpage.obj"
	-@erase ".\mipsdbg\perfpage.obj"
	-@erase ".\mipsdbg\trayicon.obj"
	-@erase ".\mipsdbg\procpage.obj"
	-@erase ".\mipsdbg\main.obj"
	-@erase ".\mipsdbg\ptrarray.obj"
	-@erase ".\mipsdbg\taskmgr.res"
	-@erase ".\mipsdbg\taskmgr.map"
	-@erase ".\mipsdbg\taskmgr.bsc"
	-@erase ".\mipsdbg\perfpage.sbr"
	-@erase ".\mipsdbg\trayicon.sbr"
	-@erase ".\mipsdbg\procpage.sbr"
	-@erase ".\mipsdbg\main.sbr"
	-@erase ".\mipsdbg\ptrarray.sbr"
	-@erase ".\mipsdbg\taskpage.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gt0 /QMOb2000 /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MIPS_" /D "WINNT" /D "DEBUG" /D "DBG" /D DEVL=1 /FR /YX"precomp.h" /c
CPP_PROJ=/nologo /MLd /Gt0 /QMOb2000 /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_MIPS_" /D "WINNT" /D "DEBUG" /D "DBG" /D DEVL=1 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/taskmgr.pch" /YX"precomp.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\mipsdbg/
CPP_SBRS=.\mipsdbg/

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
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "_DEBUG" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:MIPS
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib comctl32.lib ntdll.lib user32p.lib shell32.lib /nologo /entry:"ModuleEntry" /subsystem:windows /profile /map /debug /machine:MIPS /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib\
 comctl32.lib ntdll.lib user32p.lib shell32.lib /nologo /entry:"ModuleEntry"\
 /subsystem:windows /profile /map:"$(INTDIR)/taskmgr.map" /debug /machine:MIPS\
 /nodefaultlib /out:"$(OUTDIR)/taskmgr.exe" 
LINK32_OBJS= \
	"$(INTDIR)/taskpage.obj" \
	"$(INTDIR)/perfpage.obj" \
	"$(INTDIR)/trayicon.obj" \
	"$(INTDIR)/procpage.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/ptrarray.obj" \
	"$(INTDIR)/taskmgr.res"

"$(OUTDIR)\taskmgr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/taskmgr.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/perfpage.sbr" \
	"$(INTDIR)/trayicon.sbr" \
	"$(INTDIR)/procpage.sbr" \
	"$(INTDIR)/main.sbr" \
	"$(INTDIR)/ptrarray.sbr" \
	"$(INTDIR)/taskpage.sbr"

"$(OUTDIR)\taskmgr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "taskmgr__"
# PROP BASE Intermediate_Dir "taskmgr__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MIPSREL"
# PROP Intermediate_Dir "MIPSREL"
# PROP Target_Dir ""
OUTDIR=.\MIPSREL
INTDIR=.\MIPSREL

ALL : "$(OUTDIR)\taskmgr.exe" "$(OUTDIR)\taskmgr.bsc"

CLEAN : 
	-@erase ".\MIPSREL\taskmgr.exe"
	-@erase ".\MIPSREL\ptrarray.obj"
	-@erase ".\MIPSREL\main.obj"
	-@erase ".\MIPSREL\taskpage.obj"
	-@erase ".\MIPSREL\perfpage.obj"
	-@erase ".\MIPSREL\trayicon.obj"
	-@erase ".\MIPSREL\procpage.obj"
	-@erase ".\MIPSREL\taskmgr.res"
	-@erase ".\MIPSREL\taskmgr.ilk"
	-@erase ".\MIPSREL\taskmgr.bsc"
	-@erase ".\MIPSREL\taskpage.sbr"
	-@erase ".\MIPSREL\perfpage.sbr"
	-@erase ".\MIPSREL\trayicon.sbr"
	-@erase ".\MIPSREL\procpage.sbr"
	-@erase ".\MIPSREL\ptrarray.sbr"
	-@erase ".\MIPSREL\main.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /QMOb2000 /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MIPS_" /D "WINNT" /FR /YX"precomp.h" /c
# ADD CPP /nologo /Gt0 /QMOb2000 /W3 /O1 /Ob2 /D "WIN32" /D "_NDEBUG" /D "_WINDOWS" /D "_MIPS_" /D "WINNT" /D DEVL=1 /Fr /YX"precomp.h" /c
CPP_PROJ=/nologo /MLd /Gt0 /QMOb2000 /W3 /O1 /Ob2 /D "WIN32" /D "_NDEBUG" /D\
 "_WINDOWS" /D "_MIPS_" /D "WINNT" /D DEVL=1 /Fr"$(INTDIR)/"\
 /Fp"$(INTDIR)/taskmgr.pch" /YX"precomp.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\MIPSREL/
CPP_SBRS=.\MIPSREL/

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
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "_DEBUG" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib comctl32.lib ntdll.lib shell32.lib /nologo /entry:"ModuleEntry" /subsystem:windows /debug /machine:MIPS /nodefaultlib
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib comctl32.lib ntdll.lib user32p.lib shell32.lib /nologo /entry:"ModuleEntry" /subsystem:windows /machine:MIPS /nodefaultlib
# SUBTRACT LINK32 /incremental:no /debug
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib\
 comctl32.lib ntdll.lib user32p.lib shell32.lib /nologo /entry:"ModuleEntry"\
 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)/taskmgr.pdb" /machine:MIPS\
 /nodefaultlib /out:"$(OUTDIR)/taskmgr.exe" 
LINK32_OBJS= \
	"$(INTDIR)/ptrarray.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/taskpage.obj" \
	"$(INTDIR)/perfpage.obj" \
	"$(INTDIR)/trayicon.obj" \
	"$(INTDIR)/procpage.obj" \
	"$(INTDIR)/taskmgr.res"

"$(OUTDIR)\taskmgr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/taskmgr.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/taskpage.sbr" \
	"$(INTDIR)/perfpage.sbr" \
	"$(INTDIR)/trayicon.sbr" \
	"$(INTDIR)/procpage.sbr" \
	"$(INTDIR)/ptrarray.sbr" \
	"$(INTDIR)/main.sbr"

"$(OUTDIR)\taskmgr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "taskmgr - Win32 Release"
# Name "taskmgr - Win32 Debug"
# Name "taskmgr - Win32 MIPSDebug"
# Name "taskmgr - Win32 MIPSRetail"

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\taskmgr.rc

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_RSC_TASKM=\
	".\numbers.bmp"\
	".\ledlit.bmp"\
	".\ledunlit.bmp"\
	".\bmpback.bmp"\
	".\bmpforwa.bmp"\
	".\bitmap1.bmp"\
	".\bmp00001.bmp"\
	".\bitmap2.bmp"\
	".\main.ico"\
	".\default.ico"\
	".\tray0.ico"\
	".\tray1.ico"\
	".\tray2.ico"\
	".\tray3.ico"\
	".\tray4.ico"\
	".\tray5.ico"\
	".\tray6.ico"\
	".\tray7.ico"\
	".\tray8.ico"\
	".\tray9.ico"\
	".\tray10.ico"\
	".\tray11.ico"\
	

"$(INTDIR)\taskmgr.res" : $(SOURCE) $(DEP_RSC_SYSMO) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_RSC_TASKM=\
	".\numbers.bmp"\
	".\ledlit.bmp"\
	".\ledunlit.bmp"\
	".\bmpback.bmp"\
	".\bmpforwa.bmp"\
	".\bitmap1.bmp"\
	".\bmp00001.bmp"\
	".\bitmap2.bmp"\
	".\main.ico"\
	".\default.ico"\
	".\tray0.ico"\
	".\tray1.ico"\
	".\tray2.ico"\
	".\tray3.ico"\
	".\tray4.ico"\
	".\tray5.ico"\
	".\tray6.ico"\
	".\tray7.ico"\
	".\tray8.ico"\
	".\tray9.ico"\
	".\tray10.ico"\
	".\tray11.ico"\
	

"$(INTDIR)\taskmgr.res" : $(SOURCE) $(DEP_RSC_SYSMO) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_RSC_TASKM=\
	".\numbers.bmp"\
	".\ledlit.bmp"\
	".\ledunlit.bmp"\
	".\bmpback.bmp"\
	".\bmpforwa.bmp"\
	".\bitmap1.bmp"\
	".\bmp00001.bmp"\
	".\bitmap2.bmp"\
	".\main.ico"\
	".\default.ico"\
	".\tray0.ico"\
	".\tray1.ico"\
	".\tray2.ico"\
	".\tray3.ico"\
	".\tray4.ico"\
	".\tray5.ico"\
	".\tray6.ico"\
	".\tray7.ico"\
	".\tray8.ico"\
	".\tray9.ico"\
	".\tray10.ico"\
	".\tray11.ico"\
	

"$(INTDIR)\taskmgr.res" : $(SOURCE) $(DEP_RSC_TASKM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "_DEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_RSC_TASKM=\
	".\numbers.bmp"\
	".\ledlit.bmp"\
	".\ledunlit.bmp"\
	".\bmpback.bmp"\
	".\bmpforwa.bmp"\
	".\bitmap1.bmp"\
	".\bmp00001.bmp"\
	".\bitmap2.bmp"\
	".\main.ico"\
	".\default.ico"\
	".\tray0.ico"\
	".\tray1.ico"\
	".\tray2.ico"\
	".\tray3.ico"\
	".\tray4.ico"\
	".\tray5.ico"\
	".\tray6.ico"\
	".\tray7.ico"\
	".\tray8.ico"\
	".\tray9.ico"\
	".\tray10.ico"\
	".\tray11.ico"\
	

"$(INTDIR)\taskmgr.res" : $(SOURCE) $(DEP_RSC_TASKM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/taskmgr.res" /d "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_MAIN_=\
	".\precomp.h"\
	"n:\nt\public\sdk\inc\nt.h"\
	"n:\nt\public\sdk\inc\ntrtl.h"\
	"n:\nt\public\sdk\inc\nturtl.h"\
	"n:\nt\public\sdk\inc\ntexapi.h"\
	"n:\nt\private\windows\inc\winuserp.h"\
	"n:\nt\private\windows\inc16\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	"n:\nt\public\sdk\inc\ntdef.h"\
	"n:\nt\public\sdk\inc\ntstatus.h"\
	"n:\nt\public\sdk\inc\ntkeapi.h"\
	"n:\nt\public\sdk\inc\nti386.h"\
	"n:\nt\public\sdk\inc\ntmips.h"\
	"n:\nt\public\sdk\inc\ntalpha.h"\
	"n:\nt\public\sdk\inc\ntppc.h"\
	"n:\nt\public\sdk\inc\ntseapi.h"\
	"n:\nt\public\sdk\inc\ntobapi.h"\
	"n:\nt\public\sdk\inc\ntimage.h"\
	"n:\nt\public\sdk\inc\ntldr.h"\
	"n:\nt\public\sdk\inc\ntpsapi.h"\
	"n:\nt\public\sdk\inc\ntxcapi.h"\
	"n:\nt\public\sdk\inc\ntlpcapi.h"\
	"n:\nt\public\sdk\inc\ntioapi.h"\
	"n:\nt\public\sdk\inc\ntiolog.h"\
	"n:\nt\public\sdk\inc\ntpoapi.h"\
	"n:\nt\public\sdk\inc\ntkxapi.h"\
	"n:\nt\public\sdk\inc\ntmmapi.h"\
	"n:\nt\public\sdk\inc\ntregapi.h"\
	"n:\nt\public\sdk\inc\ntelfapi.h"\
	"n:\nt\public\sdk\inc\ntconfig.h"\
	"n:\nt\public\sdk\inc\ntnls.h"\
	"n:\nt\public\sdk\inc\ntpnpapi.h"\
	"n:\nt\public\sdk\inc\mipsinst.h"\
	"n:\nt\public\sdk\inc\ppcinst.h"\
	"n:\nt\public\sdk\inc\devioctl.h"\
	"n:\nt\public\sdk\inc\cfg.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_MAIN_=\
	".\precomp.h"\
	"n:\nt\public\sdk\inc\nt.h"\
	"n:\nt\public\sdk\inc\ntrtl.h"\
	"n:\nt\public\sdk\inc\nturtl.h"\
	"n:\nt\public\sdk\inc\ntexapi.h"\
	"n:\nt\private\windows\inc\winuserp.h"\
	"n:\nt\private\windows\inc16\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	"n:\nt\public\sdk\inc\ntdef.h"\
	"n:\nt\public\sdk\inc\ntstatus.h"\
	"n:\nt\public\sdk\inc\ntkeapi.h"\
	"n:\nt\public\sdk\inc\nti386.h"\
	"n:\nt\public\sdk\inc\ntmips.h"\
	"n:\nt\public\sdk\inc\ntalpha.h"\
	"n:\nt\public\sdk\inc\ntppc.h"\
	"n:\nt\public\sdk\inc\ntseapi.h"\
	"n:\nt\public\sdk\inc\ntobapi.h"\
	"n:\nt\public\sdk\inc\ntimage.h"\
	"n:\nt\public\sdk\inc\ntldr.h"\
	"n:\nt\public\sdk\inc\ntpsapi.h"\
	"n:\nt\public\sdk\inc\ntxcapi.h"\
	"n:\nt\public\sdk\inc\ntlpcapi.h"\
	"n:\nt\public\sdk\inc\ntioapi.h"\
	"n:\nt\public\sdk\inc\ntiolog.h"\
	"n:\nt\public\sdk\inc\ntpoapi.h"\
	"n:\nt\public\sdk\inc\ntkxapi.h"\
	"n:\nt\public\sdk\inc\ntmmapi.h"\
	"n:\nt\public\sdk\inc\ntregapi.h"\
	"n:\nt\public\sdk\inc\ntelfapi.h"\
	"n:\nt\public\sdk\inc\ntconfig.h"\
	"n:\nt\public\sdk\inc\ntnls.h"\
	"n:\nt\public\sdk\inc\ntpnpapi.h"\
	"n:\nt\public\sdk\inc\mipsinst.h"\
	"n:\nt\public\sdk\inc\ppcinst.h"\
	"n:\nt\public\sdk\inc\devioctl.h"\
	"n:\nt\public\sdk\inc\cfg.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_MAIN_=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_MAIN_=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\perfpage.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_PERFP=\
	".\precomp.h"\
	

"$(INTDIR)\perfpage.obj" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

"$(INTDIR)\perfpage.sbr" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_PERFP=\
	".\precomp.h"\
	".\nt.h"\
	

"$(INTDIR)\perfpage.obj" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

"$(INTDIR)\perfpage.sbr" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_PERFP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\perfpage.obj" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

"$(INTDIR)\perfpage.sbr" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_PERFP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	

"$(INTDIR)\perfpage.obj" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"

"$(INTDIR)\perfpage.sbr" : $(SOURCE) $(DEP_CPP_PERFP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\taskmgr.h

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\precomp.h

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\resource.h

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pages.h

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\procpage.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_PROCP=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	

"$(INTDIR)\procpage.obj" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

"$(INTDIR)\procpage.sbr" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_PROCP=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\procpage.obj" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

"$(INTDIR)\procpage.sbr" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_PROCP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\procpage.obj" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

"$(INTDIR)\procpage.sbr" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_PROCP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\procpage.obj" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"

"$(INTDIR)\procpage.sbr" : $(SOURCE) $(DEP_CPP_PROCP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptrarray.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_PTRAR=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	

"$(INTDIR)\ptrarray.obj" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

"$(INTDIR)\ptrarray.sbr" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_PTRAR=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\ptrarray.obj" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

"$(INTDIR)\ptrarray.sbr" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_PTRAR=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\ptrarray.obj" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

"$(INTDIR)\ptrarray.sbr" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_PTRAR=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\ptrarray.obj" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"

"$(INTDIR)\ptrarray.sbr" : $(SOURCE) $(DEP_CPP_PTRAR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptrarray.h

!IF  "$(CFG)" == "taskmgr - Win32 Release"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\taskpage.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_TASKP=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	

"$(INTDIR)\taskpage.obj" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

"$(INTDIR)\taskpage.sbr" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_TASKP=\
	".\precomp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\ntexapi.h"\
	".\winuserp.h"\
	".\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\taskpage.obj" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

"$(INTDIR)\taskpage.sbr" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_TASKP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\taskpage.obj" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

"$(INTDIR)\taskpage.sbr" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_TASKP=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\taskpage.obj" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"

"$(INTDIR)\taskpage.sbr" : $(SOURCE) $(DEP_CPP_TASKP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\trayicon.cpp

!IF  "$(CFG)" == "taskmgr - Win32 Release"

DEP_CPP_TRAYI=\
	".\precomp.h"\
	"n:\nt\public\sdk\inc\nt.h"\
	"n:\nt\public\sdk\inc\ntrtl.h"\
	"n:\nt\public\sdk\inc\nturtl.h"\
	"n:\nt\public\sdk\inc\ntexapi.h"\
	"n:\nt\private\windows\inc\winuserp.h"\
	"n:\nt\private\windows\inc16\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	"n:\nt\public\sdk\inc\ntdef.h"\
	"n:\nt\public\sdk\inc\ntstatus.h"\
	"n:\nt\public\sdk\inc\ntkeapi.h"\
	"n:\nt\public\sdk\inc\nti386.h"\
	"n:\nt\public\sdk\inc\ntmips.h"\
	"n:\nt\public\sdk\inc\ntalpha.h"\
	"n:\nt\public\sdk\inc\ntppc.h"\
	"n:\nt\public\sdk\inc\ntseapi.h"\
	"n:\nt\public\sdk\inc\ntobapi.h"\
	"n:\nt\public\sdk\inc\ntimage.h"\
	"n:\nt\public\sdk\inc\ntldr.h"\
	"n:\nt\public\sdk\inc\ntpsapi.h"\
	"n:\nt\public\sdk\inc\ntxcapi.h"\
	"n:\nt\public\sdk\inc\ntlpcapi.h"\
	"n:\nt\public\sdk\inc\ntioapi.h"\
	"n:\nt\public\sdk\inc\ntiolog.h"\
	"n:\nt\public\sdk\inc\ntpoapi.h"\
	"n:\nt\public\sdk\inc\ntkxapi.h"\
	"n:\nt\public\sdk\inc\ntmmapi.h"\
	"n:\nt\public\sdk\inc\ntregapi.h"\
	"n:\nt\public\sdk\inc\ntelfapi.h"\
	"n:\nt\public\sdk\inc\ntconfig.h"\
	"n:\nt\public\sdk\inc\ntnls.h"\
	"n:\nt\public\sdk\inc\ntpnpapi.h"\
	"n:\nt\public\sdk\inc\mipsinst.h"\
	"n:\nt\public\sdk\inc\ppcinst.h"\
	"n:\nt\public\sdk\inc\devioctl.h"\
	"n:\nt\public\sdk\inc\cfg.h"\
	

"$(INTDIR)\trayicon.obj" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

"$(INTDIR)\trayicon.sbr" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 Debug"

DEP_CPP_TRAYI=\
	".\precomp.h"\
	"n:\nt\public\sdk\inc\nt.h"\
	"n:\nt\public\sdk\inc\ntrtl.h"\
	"n:\nt\public\sdk\inc\nturtl.h"\
	"n:\nt\public\sdk\inc\ntexapi.h"\
	"n:\nt\private\windows\inc\winuserp.h"\
	"n:\nt\private\windows\inc16\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	"n:\nt\public\sdk\inc\ntdef.h"\
	"n:\nt\public\sdk\inc\ntstatus.h"\
	"n:\nt\public\sdk\inc\ntkeapi.h"\
	"n:\nt\public\sdk\inc\nti386.h"\
	"n:\nt\public\sdk\inc\ntmips.h"\
	"n:\nt\public\sdk\inc\ntalpha.h"\
	"n:\nt\public\sdk\inc\ntppc.h"\
	"n:\nt\public\sdk\inc\ntseapi.h"\
	"n:\nt\public\sdk\inc\ntobapi.h"\
	"n:\nt\public\sdk\inc\ntimage.h"\
	"n:\nt\public\sdk\inc\ntldr.h"\
	"n:\nt\public\sdk\inc\ntpsapi.h"\
	"n:\nt\public\sdk\inc\ntxcapi.h"\
	"n:\nt\public\sdk\inc\ntlpcapi.h"\
	"n:\nt\public\sdk\inc\ntioapi.h"\
	"n:\nt\public\sdk\inc\ntiolog.h"\
	"n:\nt\public\sdk\inc\ntpoapi.h"\
	"n:\nt\public\sdk\inc\ntkxapi.h"\
	"n:\nt\public\sdk\inc\ntmmapi.h"\
	"n:\nt\public\sdk\inc\ntregapi.h"\
	"n:\nt\public\sdk\inc\ntelfapi.h"\
	"n:\nt\public\sdk\inc\ntconfig.h"\
	"n:\nt\public\sdk\inc\ntnls.h"\
	"n:\nt\public\sdk\inc\ntpnpapi.h"\
	"n:\nt\public\sdk\inc\mipsinst.h"\
	"n:\nt\public\sdk\inc\ppcinst.h"\
	"n:\nt\public\sdk\inc\devioctl.h"\
	"n:\nt\public\sdk\inc\cfg.h"\
	

"$(INTDIR)\trayicon.obj" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

"$(INTDIR)\trayicon.sbr" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSDebug"

DEP_CPP_TRAYI=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	{$(INCLUDE)}"\ntdef.h"\
	

"$(INTDIR)\trayicon.obj" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

"$(INTDIR)\trayicon.sbr" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "taskmgr - Win32 MIPSRetail"

DEP_CPP_TRAYI=\
	".\precomp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\winuserp.h"\
	{$(INCLUDE)}"\shsemip.h"\
	".\taskmgr.h"\
	".\pages.h"\
	".\ptrarray.h"\
	

"$(INTDIR)\trayicon.obj" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"

"$(INTDIR)\trayicon.sbr" : $(SOURCE) $(DEP_CPP_TRAYI) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
