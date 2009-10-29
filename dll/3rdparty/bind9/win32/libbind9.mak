# Microsoft Developer Studio Generated NMAKE File, Based on libbind9.dsp
!IF "$(CFG)" == ""
CFG=libbind9 - Win32 Release
!MESSAGE No configuration specified. Defaulting to libbind9 - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "libbind9 - Win32 Release" && "$(CFG)" != "libbind9 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libbind9.mak" CFG="libbind9 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libbind9 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libbind9 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "libbind9 - Win32 Release"
_VC_MANIFEST_INC=0
_VC_MANIFEST_BASENAME=__VC80
!ELSE
_VC_MANIFEST_INC=1
_VC_MANIFEST_BASENAME=__VC80.Debug
!ENDIF

####################################################
# Specifying name of temporary resource file used only in incremental builds:

!if "$(_VC_MANIFEST_INC)" == "1"
_VC_MANIFEST_AUTO_RES=$(_VC_MANIFEST_BASENAME).auto.res
!else
_VC_MANIFEST_AUTO_RES=
!endif

####################################################
# _VC_MANIFEST_EMBED_EXE - command to embed manifest in EXE:

!if "$(_VC_MANIFEST_INC)" == "1"

#MT_SPECIAL_RETURN=1090650113
#MT_SPECIAL_SWITCH=-notify_resource_update
MT_SPECIAL_RETURN=0
MT_SPECIAL_SWITCH=
_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -out:$(_VC_MANIFEST_BASENAME).auto.manifest $(MT_SPECIAL_SWITCH) & \
if "%ERRORLEVEL%" == "$(MT_SPECIAL_RETURN)" \
rc /r $(_VC_MANIFEST_BASENAME).auto.rc & \
link $** /out:$@ $(LFLAGS)

!else

_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -outputresource:$@;1

!endif

####################################################
# _VC_MANIFEST_EMBED_DLL - command to embed manifest in DLL:

!if "$(_VC_MANIFEST_INC)" == "1"

#MT_SPECIAL_RETURN=1090650113
#MT_SPECIAL_SWITCH=-notify_resource_update
MT_SPECIAL_RETURN=0
MT_SPECIAL_SWITCH=
_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -out:$(_VC_MANIFEST_BASENAME).auto.manifest $(MT_SPECIAL_SWITCH) & \
if "%ERRORLEVEL%" == "$(MT_SPECIAL_RETURN)" \
rc /r $(_VC_MANIFEST_BASENAME).auto.rc & \
link $** /out:$@ $(LFLAGS)

!else

_VC_MANIFEST_EMBED_EXE= \
if exist $@.manifest mt.exe -manifest $@.manifest -outputresource:$@;2

!endif
####################################################
# _VC_MANIFEST_CLEAN - command to clean resources files generated temporarily:

!if "$(_VC_MANIFEST_INC)" == "1"

_VC_MANIFEST_CLEAN=-del $(_VC_MANIFEST_BASENAME).auto.res \
    $(_VC_MANIFEST_BASENAME).auto.rc \
    $(_VC_MANIFEST_BASENAME).auto.manifest

!else

_VC_MANIFEST_CLEAN=

!endif

!IF  "$(CFG)" == "libbind9 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Release\libbind9.dll"

!ELSE 

ALL : "libisccfg - Win32 Release" "libisc - Win32 Release" "libdns - Win32 Release" "..\..\..\Build\Release\libbind9.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libdns - Win32 ReleaseCLEAN" "libisc - Win32 ReleaseCLEAN" "libisccfg - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\check.obj"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\getaddresses.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\libbind9.exp"
	-@erase "$(OUTDIR)\libbind9.lib"
	-@erase "..\..\..\Build\Release\libbind9.dll"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "../../../lib/dns/win32/include" /I "../..../lib/dns/sec/openssl/include" /I "./" /I "../../../" /I "include" /I "../include" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/isccfg/include" /I "../../../lib/dns/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /D "NDEBUG" /D "WIN32" /D "__STDC__" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBBIND9_EXPORTS" /Fp"$(INTDIR)\libbind9.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libbind9.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../isc/win32/Release/libisc.lib  ../../dns/win32/Release/libdns.lib ../../isccfg/win32/Release/libisccfg.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\libbind9.pdb" /machine:I386 /def:".\libbind9.def" /out:"../../../Build/Release/libbind9.dll" /implib:"$(OUTDIR)\libbind9.lib" 
DEF_FILE= \
	".\libbind9.def"
LINK32_OBJS= \
	"$(INTDIR)\check.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\getaddresses.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\dns\win32\Release\libdns.lib" \
	"..\..\isc\win32\Release\libisc.lib" \
	"..\..\isccfg\win32\Release\libisccfg.lib"

"..\..\..\Build\Release\libbind9.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Debug\libbind9.dll" "$(OUTDIR)\libbind9.bsc"

!ELSE 

ALL : "libisccfg - Win32 Debug" "libisc - Win32 Debug" "libdns - Win32 Debug" "..\..\..\Build\Debug\libbind9.dll" "$(OUTDIR)\libbind9.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libdns - Win32 DebugCLEAN" "libisc - Win32 DebugCLEAN" "libisccfg - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\check.obj"
	-@erase "$(INTDIR)\check.sbr"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\DLLMain.sbr"
	-@erase "$(INTDIR)\getaddresses.obj"
	-@erase "$(INTDIR)\getaddresses.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\libbind9.bsc"
	-@erase "$(OUTDIR)\libbind9.exp"
	-@erase "$(OUTDIR)\libbind9.lib"
	-@erase "$(OUTDIR)\libbind9.pdb"
	-@erase "..\..\..\Build\Debug\libbind9.dll"
	-@erase "..\..\..\Build\Debug\libbind9.ilk"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../lib/isccfg/include" /I "./" /I "../../../" /I "include" /I "../include" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/dns/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /D "_DEBUG" /D "WIN32" /D "__STDC__" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBBIND9_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libbind9.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libbind9.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\check.sbr" \
	"$(INTDIR)\DLLMain.sbr" \
	"$(INTDIR)\getaddresses.sbr" \
	"$(INTDIR)\version.sbr"

"$(OUTDIR)\libbind9.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../isc/win32/debug/libisc.lib ../../dns/win32/debug/libdns.lib ../../isccfg/win32/debug/libisccfg.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\libbind9.pdb" /debug /machine:I386 /def:".\libbind9.def" /out:"../../../Build/Debug/libbind9.dll" /implib:"$(OUTDIR)\libbind9.lib" /pdbtype:sept 
DEF_FILE= \
	".\libbind9.def"
LINK32_OBJS= \
	"$(INTDIR)\check.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\getaddresses.obj" \
	"$(INTDIR)\version.obj" \
	"..\..\dns\win32\Debug\libdns.lib" \
	"..\..\isc\win32\Debug\libisc.lib" \
	"..\..\isccfg\win32\Debug\libisccfg.lib"

"..\..\..\Build\Debug\libbind9.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

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
!IF EXISTS("libbind9.dep")
!INCLUDE "libbind9.dep"
!ELSE 
!MESSAGE Warning: cannot find "libbind9.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libbind9 - Win32 Release" || "$(CFG)" == "libbind9 - Win32 Debug"
SOURCE=..\check.c

!IF  "$(CFG)" == "libbind9 - Win32 Release"


"$(INTDIR)\check.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"


"$(INTDIR)\check.obj"	"$(INTDIR)\check.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\DLLMain.c

!IF  "$(CFG)" == "libbind9 - Win32 Release"


"$(INTDIR)\DLLMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"


"$(INTDIR)\DLLMain.obj"	"$(INTDIR)\DLLMain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\getaddresses.c

!IF  "$(CFG)" == "libbind9 - Win32 Release"


"$(INTDIR)\getaddresses.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"


"$(INTDIR)\getaddresses.obj"	"$(INTDIR)\getaddresses.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

!IF  "$(CFG)" == "libbind9 - Win32 Release"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

!IF  "$(CFG)" == "libbind9 - Win32 Release"

"libdns - Win32 Release" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Release" 
   cd "..\..\bind9\win32"

"libdns - Win32 ReleaseCLEAN" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"

"libdns - Win32 Debug" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Debug" 
   cd "..\..\bind9\win32"

"libdns - Win32 DebugCLEAN" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ENDIF 

!IF  "$(CFG)" == "libbind9 - Win32 Release"

"libisc - Win32 Release" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" 
   cd "..\..\bind9\win32"

"libisc - Win32 ReleaseCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"

"libisc - Win32 Debug" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" 
   cd "..\..\bind9\win32"

"libisc - Win32 DebugCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ENDIF 

!IF  "$(CFG)" == "libbind9 - Win32 Release"

"libisccfg - Win32 Release" : 
   cd "..\..\isccfg\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisccfg.mak" CFG="libisccfg - Win32 Release" 
   cd "..\..\bind9\win32"

"libisccfg - Win32 ReleaseCLEAN" : 
   cd "..\..\isccfg\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisccfg.mak" CFG="libisccfg - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ELSEIF  "$(CFG)" == "libbind9 - Win32 Debug"

"libisccfg - Win32 Debug" : 
   cd "..\..\isccfg\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisccfg.mak" CFG="libisccfg - Win32 Debug" 
   cd "..\..\bind9\win32"

"libisccfg - Win32 DebugCLEAN" : 
   cd "..\..\isccfg\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisccfg.mak" CFG="libisccfg - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\bind9\win32"

!ENDIF 


!ENDIF 

####################################################
# Commands to generate initial empty manifest file and the RC file
# that references it, and for generating the .res file:

$(_VC_MANIFEST_BASENAME).auto.res : $(_VC_MANIFEST_BASENAME).auto.rc

$(_VC_MANIFEST_BASENAME).auto.rc : $(_VC_MANIFEST_BASENAME).auto.manifest
    type <<$@
#include <winuser.h>
1RT_MANIFEST"$(_VC_MANIFEST_BASENAME).auto.manifest"
<< KEEP

$(_VC_MANIFEST_BASENAME).auto.manifest :
    type <<$@
<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>
</assembly>
<< KEEP
