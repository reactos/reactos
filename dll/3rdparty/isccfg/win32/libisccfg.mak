# Microsoft Developer Studio Generated NMAKE File, Based on libisccfg.dsp
!IF "$(CFG)" == ""
CFG=libisccfg - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libisccfg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libisccfg - Win32 Release" && "$(CFG)" != "libisccfg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libisccfg.mak" CFG="libisccfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libisccfg - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libisccfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "libisccfg - Win32 Release"
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

!IF  "$(CFG)" == "libisccfg - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Release\libisccfg.dll"

!ELSE 

ALL : "libdns - Win32 Release" "libisc - Win32 Release" "..\..\..\Build\Release\libisccfg.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libdns - Win32 ReleaseCLEAN" "libisc - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\aclconf.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\namedconf.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\libisccfg.exp"
	-@erase "$(OUTDIR)\libisccfg.lib"
	-@erase "..\..\..\Build\Release\libisccfg.dll"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "./" /I "../../../" /I "include" /I "../include" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /I "../../../lib/dns/win32/include" /I "../../../lib/dns/include" /I "../..../lib/dns/sec/openssl/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBISCCFG_EXPORTS" /Fp"$(INTDIR)\libisccfg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libisccfg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../dns/win32/Release/libdns.lib ../../isc/win32/Release/libisc.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\libisccfg.pdb" /machine:I386 /def:".\libisccfg.def" /out:"../../../Build/Release/libisccfg.dll" /implib:"$(OUTDIR)\libisccfg.lib" 
DEF_FILE= \
	".\libisccfg.def"
LINK32_OBJS= \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\aclconf.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\namedconf.obj" \
	"..\..\dns\win32\Release\libdns.lib" \
	"..\..\isc\win32\Release\libisc.lib"

"..\..\..\Build\Release\libisccfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Debug\libisccfg.dll" "$(OUTDIR)\libisccfg.bsc"

!ELSE 

ALL : "libisc - Win32 Debug" "..\..\..\Build\Debug\libisccfg.dll" "$(OUTDIR)\libisccfg.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libisc - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\DLLMain.sbr"
	-@erase "$(INTDIR)\aclconf.obj"
	-@erase "$(INTDIR)\aclconf.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\namedconf.obj"
	-@erase "$(INTDIR)\namedconf.sbr"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parser.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\libisccfg.bsc"
	-@erase "$(OUTDIR)\libisccfg.exp"
	-@erase "$(OUTDIR)\libisccfg.lib"
	-@erase "$(OUTDIR)\libisccfg.pdb"
	-@erase "..\..\..\Build\Debug\libisccfg.dll"
	-@erase "..\..\..\Build\Debug\libisccfg.ilk"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../../../" /I "include" /I "../include" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /I "../../../lib/dns/win32/include" /I "../../../lib/dns/include" /I "../..../lib/dns/sec/openssl/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBISCCFG_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libisccfg.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libisccfg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DLLMain.sbr" \
	"$(INTDIR)\aclconf.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\parser.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\namedconf.sbr"

"$(OUTDIR)\libisccfg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../dns/win32/debug/libdns.lib ../../isc/win32/debug/libisc.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\libisccfg.pdb" /debug /machine:I386 /def:".\libisccfg.def" /out:"../../../Build/Debug/libisccfg.dll" /implib:"$(OUTDIR)\libisccfg.lib" /pdbtype:sept 
DEF_FILE= \
	".\libisccfg.def"
LINK32_OBJS= \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\aclconf.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\namedconf.obj" \
	"..\..\dns\win32\Debug\libdns.lib" \
	"..\..\isc\win32\Debug\libisc.lib"

"..\..\..\Build\Debug\libisccfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("libisccfg.dep")
!INCLUDE "libisccfg.dep"
!ELSE 
!MESSAGE Warning: cannot find "libisccfg.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libisccfg - Win32 Release" || "$(CFG)" == "libisccfg - Win32 Debug"
SOURCE=.\DLLMain.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\DLLMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\DLLMain.obj"	"$(INTDIR)\DLLMain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\aclconf.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\aclconf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\aclconf.obj"	"$(INTDIR)\aclconf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\log.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\namedconf.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\namedconf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\namedconf.obj"	"$(INTDIR)\namedconf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\parser.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\parser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\parser.obj"	"$(INTDIR)\parser.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

!IF  "$(CFG)" == "libisccfg - Win32 Release"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

!IF  "$(CFG)" == "libisccfg - Win32 Release"

"libdns - Win32 Release" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Release" 
   cd "..\..\isccfg\win32"

"libdns - Win32 ReleaseCLEAN" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\isccfg\win32"

"libisc - Win32 Release" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" 
   cd "..\..\isccfg\win32"

"libisc - Win32 ReleaseCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\isccfg\win32"

!ELSEIF  "$(CFG)" == "libisccfg - Win32 Debug"

"libdns - Win32 Debug" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Debug" 
   cd "..\..\isccfg\win32"

"libdns - Win32 DebugCLEAN" : 
   cd "..\..\dns\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libdns.mak" CFG="libdns - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\isccfg\win32"

"libisc - Win32 Debug" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" 
   cd "..\..\isccfg\win32"

"libisc - Win32 DebugCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\isccfg\win32"

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
