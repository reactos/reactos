# Microsoft Developer Studio Generated NMAKE File, Based on liblwres.dsp
!IF "$(CFG)" == ""
CFG=liblwres - Win32 Debug
!MESSAGE No configuration specified. Defaulting to liblwres - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "liblwres - Win32 Release" && "$(CFG)" != "liblwres - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "liblwres.mak" CFG="liblwres - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "liblwres - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "liblwres - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "liblwres - Win32 Release"
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

!IF  "$(CFG)" == "liblwres - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\..\Build\Release\liblwres.dll"


CLEAN :
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\gai_strerror.obj"
	-@erase "$(INTDIR)\getaddrinfo.obj"
	-@erase "$(INTDIR)\gethost.obj"
	-@erase "$(INTDIR)\getipnode.obj"
	-@erase "$(INTDIR)\getnameinfo.obj"
	-@erase "$(INTDIR)\getrrset.obj"
	-@erase "$(INTDIR)\herror.obj"
	-@erase "$(INTDIR)\lwbuffer.obj"
	-@erase "$(INTDIR)\lwconfig.obj"
	-@erase "$(INTDIR)\lwinetaton.obj"
	-@erase "$(INTDIR)\lwinetntop.obj"
	-@erase "$(INTDIR)\lwinetpton.obj"
	-@erase "$(INTDIR)\lwpacket.obj"
	-@erase "$(INTDIR)\lwres_gabn.obj"
	-@erase "$(INTDIR)\lwres_gnba.obj"
	-@erase "$(INTDIR)\lwres_grbn.obj"
	-@erase "$(INTDIR)\lwres_noop.obj"
	-@erase "$(INTDIR)\lwresutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(OUTDIR)\liblwres.exp"
	-@erase "$(OUTDIR)\liblwres.lib"
	-@erase "..\..\..\Build\Release\liblwres.dll"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "./" /I "../../../lib/lwres/win32/include/lwres" /I "include" /I "../include" /I "../../../" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/dns/win32/include" /I "../../../lib/dns/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /I "../..../lib/dns/sec/openssl/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBLWRES_EXPORTS" /Fp"$(INTDIR)\liblwres.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\liblwres.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib iphlpapi.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\liblwres.pdb" /machine:I386 /def:".\liblwres.def" /out:"../../../Build/Release/liblwres.dll" /implib:"$(OUTDIR)\liblwres.lib" 
DEF_FILE= \
	".\liblwres.def"
LINK32_OBJS= \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\gai_strerror.obj" \
	"$(INTDIR)\getaddrinfo.obj" \
	"$(INTDIR)\gethost.obj" \
	"$(INTDIR)\getipnode.obj" \
	"$(INTDIR)\getnameinfo.obj" \
	"$(INTDIR)\getrrset.obj" \
	"$(INTDIR)\herror.obj" \
	"$(INTDIR)\lwbuffer.obj" \
	"$(INTDIR)\lwinetaton.obj" \
	"$(INTDIR)\lwinetntop.obj" \
	"$(INTDIR)\lwinetpton.obj" \
	"$(INTDIR)\lwpacket.obj" \
	"$(INTDIR)\lwres_gabn.obj" \
	"$(INTDIR)\lwres_gnba.obj" \
	"$(INTDIR)\lwres_grbn.obj" \
	"$(INTDIR)\lwres_noop.obj" \
	"$(INTDIR)\lwresutil.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\lwconfig.obj"

"..\..\..\Build\Release\liblwres.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "..\..\..\Build\Debug\liblwres.dll" "$(OUTDIR)\liblwres.bsc"


CLEAN :
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\context.sbr"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\DLLMain.sbr"
	-@erase "$(INTDIR)\gai_strerror.obj"
	-@erase "$(INTDIR)\gai_strerror.sbr"
	-@erase "$(INTDIR)\getaddrinfo.obj"
	-@erase "$(INTDIR)\getaddrinfo.sbr"
	-@erase "$(INTDIR)\gethost.obj"
	-@erase "$(INTDIR)\gethost.sbr"
	-@erase "$(INTDIR)\getipnode.obj"
	-@erase "$(INTDIR)\getipnode.sbr"
	-@erase "$(INTDIR)\getnameinfo.obj"
	-@erase "$(INTDIR)\getnameinfo.sbr"
	-@erase "$(INTDIR)\getrrset.obj"
	-@erase "$(INTDIR)\getrrset.sbr"
	-@erase "$(INTDIR)\herror.obj"
	-@erase "$(INTDIR)\herror.sbr"
	-@erase "$(INTDIR)\lwbuffer.obj"
	-@erase "$(INTDIR)\lwbuffer.sbr"
	-@erase "$(INTDIR)\lwconfig.obj"
	-@erase "$(INTDIR)\lwconfig.sbr"
	-@erase "$(INTDIR)\lwinetaton.obj"
	-@erase "$(INTDIR)\lwinetaton.sbr"
	-@erase "$(INTDIR)\lwinetntop.obj"
	-@erase "$(INTDIR)\lwinetntop.sbr"
	-@erase "$(INTDIR)\lwinetpton.obj"
	-@erase "$(INTDIR)\lwinetpton.sbr"
	-@erase "$(INTDIR)\lwpacket.obj"
	-@erase "$(INTDIR)\lwpacket.sbr"
	-@erase "$(INTDIR)\lwres_gabn.obj"
	-@erase "$(INTDIR)\lwres_gabn.sbr"
	-@erase "$(INTDIR)\lwres_gnba.obj"
	-@erase "$(INTDIR)\lwres_gnba.sbr"
	-@erase "$(INTDIR)\lwres_grbn.obj"
	-@erase "$(INTDIR)\lwres_grbn.sbr"
	-@erase "$(INTDIR)\lwres_noop.obj"
	-@erase "$(INTDIR)\lwres_noop.sbr"
	-@erase "$(INTDIR)\lwresutil.obj"
	-@erase "$(INTDIR)\lwresutil.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\socket.obj"
	-@erase "$(INTDIR)\socket.sbr"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(OUTDIR)\liblwres.bsc"
	-@erase "$(OUTDIR)\liblwres.exp"
	-@erase "$(OUTDIR)\liblwres.lib"
	-@erase "$(OUTDIR)\liblwres.pdb"
	-@erase "..\..\..\Build\Debug\liblwres.dll"
	-@erase "..\..\..\Build\Debug\liblwres.ilk"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../../../lib/lwres/win32/include/lwres" /I "include" /I "../include" /I "../../../" /I "../../../lib/isc/win32" /I "../../../lib/isc/win32/include" /I "../../../lib/dns/win32/include" /I "../../../lib/dns/include" /I "../../../lib/isc/include" /I "../../../lib/isc/noatomic/include" /I "../..../lib/dns/sec/openssl/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBLWRES_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\liblwres.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\liblwres.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\context.sbr" \
	"$(INTDIR)\DLLMain.sbr" \
	"$(INTDIR)\gai_strerror.sbr" \
	"$(INTDIR)\getaddrinfo.sbr" \
	"$(INTDIR)\gethost.sbr" \
	"$(INTDIR)\getipnode.sbr" \
	"$(INTDIR)\getnameinfo.sbr" \
	"$(INTDIR)\getrrset.sbr" \
	"$(INTDIR)\herror.sbr" \
	"$(INTDIR)\lwbuffer.sbr" \
	"$(INTDIR)\lwinetaton.sbr" \
	"$(INTDIR)\lwinetntop.sbr" \
	"$(INTDIR)\lwinetpton.sbr" \
	"$(INTDIR)\lwpacket.sbr" \
	"$(INTDIR)\lwres_gabn.sbr" \
	"$(INTDIR)\lwres_gnba.sbr" \
	"$(INTDIR)\lwres_grbn.sbr" \
	"$(INTDIR)\lwres_noop.sbr" \
	"$(INTDIR)\lwresutil.sbr" \
	"$(INTDIR)\socket.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\lwconfig.sbr"

"$(OUTDIR)\liblwres.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib iphlpapi.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\liblwres.pdb" /debug /machine:I386 /def:".\liblwres.def" /out:"../../../Build/Debug/liblwres.dll" /implib:"$(OUTDIR)\liblwres.lib" /pdbtype:sept 
DEF_FILE= \
	".\liblwres.def"
LINK32_OBJS= \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\gai_strerror.obj" \
	"$(INTDIR)\getaddrinfo.obj" \
	"$(INTDIR)\gethost.obj" \
	"$(INTDIR)\getipnode.obj" \
	"$(INTDIR)\getnameinfo.obj" \
	"$(INTDIR)\getrrset.obj" \
	"$(INTDIR)\herror.obj" \
	"$(INTDIR)\lwbuffer.obj" \
	"$(INTDIR)\lwinetaton.obj" \
	"$(INTDIR)\lwinetntop.obj" \
	"$(INTDIR)\lwinetpton.obj" \
	"$(INTDIR)\lwpacket.obj" \
	"$(INTDIR)\lwres_gabn.obj" \
	"$(INTDIR)\lwres_gnba.obj" \
	"$(INTDIR)\lwres_grbn.obj" \
	"$(INTDIR)\lwres_noop.obj" \
	"$(INTDIR)\lwresutil.obj" \
	"$(INTDIR)\socket.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\lwconfig.obj"

"..\..\..\Build\Debug\liblwres.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("liblwres.dep")
!INCLUDE "liblwres.dep"
!ELSE 
!MESSAGE Warning: cannot find "liblwres.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "liblwres - Win32 Release" || "$(CFG)" == "liblwres - Win32 Debug"
SOURCE=..\context.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\context.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\context.obj"	"$(INTDIR)\context.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\DLLMain.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\DLLMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\DLLMain.obj"	"$(INTDIR)\DLLMain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\gai_strerror.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\gai_strerror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\gai_strerror.obj"	"$(INTDIR)\gai_strerror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\getaddrinfo.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\getaddrinfo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\getaddrinfo.obj"	"$(INTDIR)\getaddrinfo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\gethost.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\gethost.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\gethost.obj"	"$(INTDIR)\gethost.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\getipnode.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\getipnode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\getipnode.obj"	"$(INTDIR)\getipnode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\getnameinfo.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\getnameinfo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\getnameinfo.obj"	"$(INTDIR)\getnameinfo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\getrrset.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\getrrset.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\getrrset.obj"	"$(INTDIR)\getrrset.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\herror.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\herror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\herror.obj"	"$(INTDIR)\herror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwbuffer.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwbuffer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwbuffer.obj"	"$(INTDIR)\lwbuffer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\lwconfig.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwconfig.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwconfig.obj"	"$(INTDIR)\lwconfig.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\lwinetaton.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwinetaton.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwinetaton.obj"	"$(INTDIR)\lwinetaton.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwinetntop.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwinetntop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwinetntop.obj"	"$(INTDIR)\lwinetntop.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwinetpton.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwinetpton.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwinetpton.obj"	"$(INTDIR)\lwinetpton.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwpacket.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwpacket.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwpacket.obj"	"$(INTDIR)\lwpacket.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwres_gabn.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwres_gabn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwres_gabn.obj"	"$(INTDIR)\lwres_gabn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwres_gnba.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwres_gnba.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwres_gnba.obj"	"$(INTDIR)\lwres_gnba.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwres_grbn.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwres_grbn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwres_grbn.obj"	"$(INTDIR)\lwres_grbn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwres_noop.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwres_noop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwres_noop.obj"	"$(INTDIR)\lwres_noop.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lwresutil.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\lwresutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\lwresutil.obj"	"$(INTDIR)\lwresutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\socket.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\socket.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\socket.obj"	"$(INTDIR)\socket.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\version.c

!IF  "$(CFG)" == "liblwres - Win32 Release"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "liblwres - Win32 Debug"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


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
