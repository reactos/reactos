# Microsoft Developer Studio Generated NMAKE File, Based on libdns.dsp
!IF "$(CFG)" == ""
CFG=libdns - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libdns - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libdns - Win32 Release" && "$(CFG)" != "libdns - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libdns.mak" CFG="libdns - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libdns - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libdns - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libdns - Win32 Release"
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

!IF  "$(CFG)" == "libdns - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Release\libdns.dll"

!ELSE 

ALL : "libisc - Win32 Release" "..\..\..\Build\Release\libdns.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libisc - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\acache.obj"
	-@erase "$(INTDIR)\acl.obj"
	-@erase "$(INTDIR)\adb.obj"
	-@erase "$(INTDIR)\byaddr.obj"
	-@erase "$(INTDIR)\cache.obj"
	-@erase "$(INTDIR)\callbacks.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\dbiterator.obj"
	-@erase "$(INTDIR)\dbtable.obj"
	-@erase "$(INTDIR)\diff.obj"
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\dlz.obj"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\dnssec.obj"
	-@erase "$(INTDIR)\ds.obj"
	-@erase "$(INTDIR)\dst_api.obj"
	-@erase "$(INTDIR)\dst_lib.obj"
	-@erase "$(INTDIR)\dst_parse.obj"
	-@erase "$(INTDIR)\dst_result.obj"
	-@erase "$(INTDIR)\forward.obj"
	-@erase "$(INTDIR)\gssapi_link.obj"
	-@erase "$(INTDIR)\gssapictx.obj"
	-@erase "$(INTDIR)\spnego.obj"
	-@erase "$(INTDIR)\hmac_link.obj"
	-@erase "$(INTDIR)\iptable.obj"
	-@erase "$(INTDIR)\journal.obj"
	-@erase "$(INTDIR)\key.obj"
	-@erase "$(INTDIR)\keytable.obj"
	-@erase "$(INTDIR)\lib.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\lookup.obj"
	-@erase "$(INTDIR)\master.obj"
	-@erase "$(INTDIR)\masterdump.obj"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\ncache.obj"
	-@erase "$(INTDIR)\nsec.obj"
	-@erase "$(INTDIR)\nsec3.obj"
	-@erase "$(INTDIR)\openssl_link.obj"
	-@erase "$(INTDIR)\openssldh_link.obj"
	-@erase "$(INTDIR)\openssldsa_link.obj"
	-@erase "$(INTDIR)\opensslrsa_link.obj"
	-@erase "$(INTDIR)\order.obj"
	-@erase "$(INTDIR)\peer.obj"
	-@erase "$(INTDIR)\portlist.obj"
	-@erase "$(INTDIR)\rbt.obj"
	-@erase "$(INTDIR)\rbtdb.obj"
	-@erase "$(INTDIR)\rbtdb64.obj"
	-@erase "$(INTDIR)\rcode.obj"
	-@erase "$(INTDIR)\rdata.obj"
	-@erase "$(INTDIR)\rdatalist.obj"
	-@erase "$(INTDIR)\rdataset.obj"
	-@erase "$(INTDIR)\rdatasetiter.obj"
	-@erase "$(INTDIR)\rdataslab.obj"
	-@erase "$(INTDIR)\request.obj"
	-@erase "$(INTDIR)\resolver.obj"
	-@erase "$(INTDIR)\result.obj"
	-@erase "$(INTDIR)\rootns.obj"
	-@erase "$(INTDIR)\sdb.obj"
	-@erase "$(INTDIR)\sdlz.obj"
	-@erase "$(INTDIR)\soa.obj"
	-@erase "$(INTDIR)\ssu.obj"
	-@erase "$(INTDIR)\stats.obj"
	-@erase "$(INTDIR)\tcpmsg.obj"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\tkey.obj"
	-@erase "$(INTDIR)\tsig.obj"
	-@erase "$(INTDIR)\ttl.obj"
	-@erase "$(INTDIR)\validator.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\xfrin.obj"
	-@erase "$(INTDIR)\zone.obj"
	-@erase "$(INTDIR)\zonekey.obj"
	-@erase "$(INTDIR)\zt.obj"
	-@erase "$(OUTDIR)\libdns.exp"
	-@erase "$(OUTDIR)\libdns.lib"
	-@erase "..\..\..\Build\Release\libdns.dll"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "../../../../../openssl-0.9.8d/inc32/openssl/include" /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../../lib/isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /Fp"$(INTDIR)\libdns.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libdns.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../isc/win32/Release/libisc.lib ../../../../openssl-0.9.8d/out32dll/libeay32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\libdns.pdb" /machine:I386 /def:".\libdns.def" /out:"../../../Build/Release/libdns.dll" /implib:"$(OUTDIR)\libdns.lib" 
DEF_FILE= \
	".\libdns.def"
LINK32_OBJS= \
	"$(INTDIR)\acache.obj" \
	"$(INTDIR)\acl.obj" \
	"$(INTDIR)\adb.obj" \
	"$(INTDIR)\byaddr.obj" \
	"$(INTDIR)\cache.obj" \
	"$(INTDIR)\callbacks.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dbiterator.obj" \
	"$(INTDIR)\dbtable.obj" \
	"$(INTDIR)\diff.obj" \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\dlz.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\dnssec.obj" \
	"$(INTDIR)\ds.obj" \
	"$(INTDIR)\forward.obj" \
	"$(INTDIR)\iptable.obj" \
	"$(INTDIR)\journal.obj" \
	"$(INTDIR)\keytable.obj" \
	"$(INTDIR)\lib.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\lookup.obj" \
	"$(INTDIR)\master.obj" \
	"$(INTDIR)\masterdump.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\ncache.obj" \
	"$(INTDIR)\nsec.obj" \
	"$(INTDIR)\nsec3.obj" \
	"$(INTDIR)\order.obj" \
	"$(INTDIR)\peer.obj" \
	"$(INTDIR)\portlist.obj" \
	"$(INTDIR)\rbt.obj" \
	"$(INTDIR)\rbtdb.obj" \
	"$(INTDIR)\rbtdb64.obj" \
	"$(INTDIR)\rcode.obj" \
	"$(INTDIR)\rdata.obj" \
	"$(INTDIR)\rdatalist.obj" \
	"$(INTDIR)\rdataset.obj" \
	"$(INTDIR)\rdatasetiter.obj" \
	"$(INTDIR)\rdataslab.obj" \
	"$(INTDIR)\request.obj" \
	"$(INTDIR)\resolver.obj" \
	"$(INTDIR)\result.obj" \
	"$(INTDIR)\rootns.obj" \
	"$(INTDIR)\sdb.obj" \
	"$(INTDIR)\sdlz.obj" \
	"$(INTDIR)\soa.obj" \
	"$(INTDIR)\ssu.obj" \
	"$(INTDIR)\stats.obj" \
	"$(INTDIR)\tcpmsg.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkey.obj" \
	"$(INTDIR)\tsig.obj" \
	"$(INTDIR)\ttl.obj" \
	"$(INTDIR)\validator.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\view.obj" \
	"$(INTDIR)\xfrin.obj" \
	"$(INTDIR)\zone.obj" \
	"$(INTDIR)\zonekey.obj" \
	"$(INTDIR)\zt.obj" \
	"$(INTDIR)\dst_api.obj" \
	"$(INTDIR)\dst_lib.obj" \
	"$(INTDIR)\dst_parse.obj" \
	"$(INTDIR)\dst_result.obj" \
	"$(INTDIR)\gssapi_link.obj" \
	"$(INTDIR)\gssapictx.obj" \
	"$(INTDIR)\spnego.obj" \
	"$(INTDIR)\hmac_link.obj" \
	"$(INTDIR)\key.obj" \
	"$(INTDIR)\openssl_link.obj" \
	"$(INTDIR)\openssldh_link.obj" \
	"$(INTDIR)\openssldsa_link.obj" \
	"$(INTDIR)\opensslrsa_link.obj" \
	"..\..\isc\win32\Release\libisc.lib"

"..\..\..\Build\Release\libdns.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\..\Build\Debug\libdns.dll" "$(OUTDIR)\libdns.bsc"

!ELSE 

ALL : "libisc - Win32 Debug" "..\..\..\Build\Debug\libdns.dll" "$(OUTDIR)\libdns.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"libisc - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\acache.obj"
	-@erase "$(INTDIR)\acache.sbr"
	-@erase "$(INTDIR)\acl.obj"
	-@erase "$(INTDIR)\acl.sbr"
	-@erase "$(INTDIR)\adb.obj"
	-@erase "$(INTDIR)\adb.sbr"
	-@erase "$(INTDIR)\byaddr.obj"
	-@erase "$(INTDIR)\byaddr.sbr"
	-@erase "$(INTDIR)\cache.obj"
	-@erase "$(INTDIR)\cache.sbr"
	-@erase "$(INTDIR)\callbacks.obj"
	-@erase "$(INTDIR)\callbacks.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\db.obj"
	-@erase "$(INTDIR)\db.sbr"
	-@erase "$(INTDIR)\dbiterator.obj"
	-@erase "$(INTDIR)\dbiterator.sbr"
	-@erase "$(INTDIR)\dbtable.obj"
	-@erase "$(INTDIR)\dbtable.sbr"
	-@erase "$(INTDIR)\diff.obj"
	-@erase "$(INTDIR)\diff.sbr"
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\dispatch.sbr"
	-@erase "$(INTDIR)\dlz.obj"
	-@erase "$(INTDIR)\dlz.sbr"
	-@erase "$(INTDIR)\DLLMain.obj"
	-@erase "$(INTDIR)\DLLMain.sbr"
	-@erase "$(INTDIR)\dnssec.obj"
	-@erase "$(INTDIR)\dnssec.sbr"
	-@erase "$(INTDIR)\ds.obj"
	-@erase "$(INTDIR)\ds.sbr"
	-@erase "$(INTDIR)\dst_api.obj"
	-@erase "$(INTDIR)\dst_api.sbr"
	-@erase "$(INTDIR)\dst_lib.obj"
	-@erase "$(INTDIR)\dst_lib.sbr"
	-@erase "$(INTDIR)\dst_parse.obj"
	-@erase "$(INTDIR)\dst_parse.sbr"
	-@erase "$(INTDIR)\dst_result.obj"
	-@erase "$(INTDIR)\dst_result.sbr"
	-@erase "$(INTDIR)\forward.obj"
	-@erase "$(INTDIR)\forward.sbr"
	-@erase "$(INTDIR)\gssapi_link.obj"
	-@erase "$(INTDIR)\gssapi_link.sbr"
	-@erase "$(INTDIR)\gssapictx.obj"
	-@erase "$(INTDIR)\gssapictx.sbr"
	-@erase "$(INTDIR)\spnego.obj"
	-@erase "$(INTDIR)\spnego.sbr"
	-@erase "$(INTDIR)\hmac_link.obj"
	-@erase "$(INTDIR)\hmac_link.sbr"
	-@erase "$(INTDIR)\iptable.obj"
	-@erase "$(INTDIR)\iptable.sbr"
	-@erase "$(INTDIR)\journal.obj"
	-@erase "$(INTDIR)\journal.sbr"
	-@erase "$(INTDIR)\key.obj"
	-@erase "$(INTDIR)\key.sbr"
	-@erase "$(INTDIR)\keytable.obj"
	-@erase "$(INTDIR)\keytable.sbr"
	-@erase "$(INTDIR)\lib.obj"
	-@erase "$(INTDIR)\lib.sbr"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\log.sbr"
	-@erase "$(INTDIR)\lookup.obj"
	-@erase "$(INTDIR)\lookup.sbr"
	-@erase "$(INTDIR)\master.obj"
	-@erase "$(INTDIR)\master.sbr"
	-@erase "$(INTDIR)\masterdump.obj"
	-@erase "$(INTDIR)\masterdump.sbr"
	-@erase "$(INTDIR)\message.obj"
	-@erase "$(INTDIR)\message.sbr"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\name.sbr"
	-@erase "$(INTDIR)\ncache.obj"
	-@erase "$(INTDIR)\ncache.sbr"
	-@erase "$(INTDIR)\nsec.obj"
	-@erase "$(INTDIR)\nsec.sbr"
	-@erase "$(INTDIR)\nsec3.obj"
	-@erase "$(INTDIR)\nsec3.sbr"
	-@erase "$(INTDIR)\openssl_link.obj"
	-@erase "$(INTDIR)\openssl_link.sbr"
	-@erase "$(INTDIR)\openssldh_link.obj"
	-@erase "$(INTDIR)\openssldh_link.sbr"
	-@erase "$(INTDIR)\openssldsa_link.obj"
	-@erase "$(INTDIR)\openssldsa_link.sbr"
	-@erase "$(INTDIR)\opensslrsa_link.obj"
	-@erase "$(INTDIR)\opensslrsa_link.sbr"
	-@erase "$(INTDIR)\order.obj"
	-@erase "$(INTDIR)\order.sbr"
	-@erase "$(INTDIR)\peer.obj"
	-@erase "$(INTDIR)\peer.sbr"
	-@erase "$(INTDIR)\portlist.obj"
	-@erase "$(INTDIR)\portlist.sbr"
	-@erase "$(INTDIR)\rbt.obj"
	-@erase "$(INTDIR)\rbt.sbr"
	-@erase "$(INTDIR)\rbtdb.obj"
	-@erase "$(INTDIR)\rbtdb.sbr"
	-@erase "$(INTDIR)\rbtdb64.obj"
	-@erase "$(INTDIR)\rbtdb64.sbr"
	-@erase "$(INTDIR)\rcode.obj"
	-@erase "$(INTDIR)\rcode.sbr"
	-@erase "$(INTDIR)\rdata.obj"
	-@erase "$(INTDIR)\rdata.sbr"
	-@erase "$(INTDIR)\rdatalist.obj"
	-@erase "$(INTDIR)\rdatalist.sbr"
	-@erase "$(INTDIR)\rdataset.obj"
	-@erase "$(INTDIR)\rdataset.sbr"
	-@erase "$(INTDIR)\rdatasetiter.obj"
	-@erase "$(INTDIR)\rdatasetiter.sbr"
	-@erase "$(INTDIR)\rdataslab.obj"
	-@erase "$(INTDIR)\rdataslab.sbr"
	-@erase "$(INTDIR)\request.obj"
	-@erase "$(INTDIR)\request.sbr"
	-@erase "$(INTDIR)\resolver.obj"
	-@erase "$(INTDIR)\resolver.sbr"
	-@erase "$(INTDIR)\result.obj"
	-@erase "$(INTDIR)\result.sbr"
	-@erase "$(INTDIR)\rootns.obj"
	-@erase "$(INTDIR)\rootns.sbr"
	-@erase "$(INTDIR)\sdb.obj"
	-@erase "$(INTDIR)\sdb.sbr"
	-@erase "$(INTDIR)\sdlz.obj"
	-@erase "$(INTDIR)\sdlz.sbr"
	-@erase "$(INTDIR)\soa.obj"
	-@erase "$(INTDIR)\soa.sbr"
	-@erase "$(INTDIR)\ssu.obj"
	-@erase "$(INTDIR)\ssu.sbr"
	-@erase "$(INTDIR)\stats.obj"
	-@erase "$(INTDIR)\stats.sbr"
	-@erase "$(INTDIR)\tcpmsg.obj"
	-@erase "$(INTDIR)\tcpmsg.sbr"
	-@erase "$(INTDIR)\time.obj"
	-@erase "$(INTDIR)\time.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\tkey.obj"
	-@erase "$(INTDIR)\tkey.sbr"
	-@erase "$(INTDIR)\tsig.obj"
	-@erase "$(INTDIR)\tsig.sbr"
	-@erase "$(INTDIR)\ttl.obj"
	-@erase "$(INTDIR)\ttl.sbr"
	-@erase "$(INTDIR)\validator.obj"
	-@erase "$(INTDIR)\validator.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view.sbr"
	-@erase "$(INTDIR)\xfrin.obj"
	-@erase "$(INTDIR)\xfrin.sbr"
	-@erase "$(INTDIR)\zone.obj"
	-@erase "$(INTDIR)\zone.sbr"
	-@erase "$(INTDIR)\zonekey.obj"
	-@erase "$(INTDIR)\zonekey.sbr"
	-@erase "$(INTDIR)\zt.obj"
	-@erase "$(INTDIR)\zt.sbr"
	-@erase "$(OUTDIR)\libdns.bsc"
	-@erase "$(OUTDIR)\libdns.exp"
	-@erase "$(OUTDIR)\libdns.lib"
	-@erase "$(OUTDIR)\libdns.map"
	-@erase "$(OUTDIR)\libdns.pdb"
	-@erase "..\..\..\Build\Debug\libdns.dll"
	-@erase "..\..\..\Build\Debug\libdns.ilk"
	-@$(_VC_MANIFEST_CLEAN)

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../../lib/isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libdns.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libdns.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\acache.sbr" \
	"$(INTDIR)\acl.sbr" \
	"$(INTDIR)\adb.sbr" \
	"$(INTDIR)\byaddr.sbr" \
	"$(INTDIR)\cache.sbr" \
	"$(INTDIR)\callbacks.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\db.sbr" \
	"$(INTDIR)\dbiterator.sbr" \
	"$(INTDIR)\dbtable.sbr" \
	"$(INTDIR)\diff.sbr" \
	"$(INTDIR)\dispatch.sbr" \
	"$(INTDIR)\dlz.sbr" \
	"$(INTDIR)\DLLMain.sbr" \
	"$(INTDIR)\dnssec.sbr" \
	"$(INTDIR)\ds.sbr" \
	"$(INTDIR)\forward.sbr" \
	"$(INTDIR)\iptable.sbr" \
	"$(INTDIR)\journal.sbr" \
	"$(INTDIR)\keytable.sbr" \
	"$(INTDIR)\lib.sbr" \
	"$(INTDIR)\log.sbr" \
	"$(INTDIR)\lookup.sbr" \
	"$(INTDIR)\master.sbr" \
	"$(INTDIR)\masterdump.sbr" \
	"$(INTDIR)\message.sbr" \
	"$(INTDIR)\name.sbr" \
	"$(INTDIR)\ncache.sbr" \
	"$(INTDIR)\nsec.sbr" \
	"$(INTDIR)\nsec3.sbr" \
	"$(INTDIR)\order.sbr" \
	"$(INTDIR)\peer.sbr" \
	"$(INTDIR)\portlist.sbr" \
	"$(INTDIR)\rbt.sbr" \
	"$(INTDIR)\rbtdb.sbr" \
	"$(INTDIR)\rbtdb64.sbr" \
	"$(INTDIR)\rcode.sbr" \
	"$(INTDIR)\rdata.sbr" \
	"$(INTDIR)\rdatalist.sbr" \
	"$(INTDIR)\rdataset.sbr" \
	"$(INTDIR)\rdatasetiter.sbr" \
	"$(INTDIR)\rdataslab.sbr" \
	"$(INTDIR)\request.sbr" \
	"$(INTDIR)\resolver.sbr" \
	"$(INTDIR)\result.sbr" \
	"$(INTDIR)\rootns.sbr" \
	"$(INTDIR)\sdb.sbr" \
	"$(INTDIR)\sdlz.sbr" \
	"$(INTDIR)\soa.sbr" \
	"$(INTDIR)\ssu.sbr" \
	"$(INTDIR)\stats.sbr" \
	"$(INTDIR)\tcpmsg.sbr" \
	"$(INTDIR)\time.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\tkey.sbr" \
	"$(INTDIR)\tsig.sbr" \
	"$(INTDIR)\ttl.sbr" \
	"$(INTDIR)\validator.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\view.sbr" \
	"$(INTDIR)\xfrin.sbr" \
	"$(INTDIR)\zone.sbr" \
	"$(INTDIR)\zonekey.sbr" \
	"$(INTDIR)\zt.sbr" \
	"$(INTDIR)\dst_api.sbr" \
	"$(INTDIR)\dst_lib.sbr" \
	"$(INTDIR)\dst_parse.sbr" \
	"$(INTDIR)\dst_result.sbr" \
	"$(INTDIR)\gssapi_link.sbr" \
	"$(INTDIR)\gssapictx.sbr" \
	"$(INTDIR)\spnego.sbr" \
	"$(INTDIR)\hmac_link.sbr" \
	"$(INTDIR)\key.sbr" \
	"$(INTDIR)\openssl_link.sbr" \
	"$(INTDIR)\openssldh_link.sbr" \
	"$(INTDIR)\openssldsa_link.sbr" \
	"$(INTDIR)\opensslrsa_link.sbr"

"$(OUTDIR)\libdns.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=user32.lib advapi32.lib ws2_32.lib ../../isc/win32/debug/libisc.lib ../../../../openssl-0.9.8d/out32dll/libeay32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\libdns.pdb" /map:"$(INTDIR)\libdns.map" /debug /machine:I386 /def:".\libdns.def" /out:"../../../Build/Debug/libdns.dll" /implib:"$(OUTDIR)\libdns.lib" /pdbtype:sept 
DEF_FILE= \
	".\libdns.def"
LINK32_OBJS= \
	"$(INTDIR)\acache.obj" \
	"$(INTDIR)\acl.obj" \
	"$(INTDIR)\adb.obj" \
	"$(INTDIR)\byaddr.obj" \
	"$(INTDIR)\cache.obj" \
	"$(INTDIR)\callbacks.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\db.obj" \
	"$(INTDIR)\dbiterator.obj" \
	"$(INTDIR)\dbtable.obj" \
	"$(INTDIR)\diff.obj" \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\dlz.obj" \
	"$(INTDIR)\DLLMain.obj" \
	"$(INTDIR)\dnssec.obj" \
	"$(INTDIR)\ds.obj" \
	"$(INTDIR)\forward.obj" \
	"$(INTDIR)\iptable.obj" \
	"$(INTDIR)\journal.obj" \
	"$(INTDIR)\keytable.obj" \
	"$(INTDIR)\lib.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\lookup.obj" \
	"$(INTDIR)\master.obj" \
	"$(INTDIR)\masterdump.obj" \
	"$(INTDIR)\message.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\ncache.obj" \
	"$(INTDIR)\nsec.obj" \
	"$(INTDIR)\nsec3.obj" \
	"$(INTDIR)\order.obj" \
	"$(INTDIR)\peer.obj" \
	"$(INTDIR)\portlist.obj" \
	"$(INTDIR)\rbt.obj" \
	"$(INTDIR)\rbtdb.obj" \
	"$(INTDIR)\rbtdb64.obj" \
	"$(INTDIR)\rcode.obj" \
	"$(INTDIR)\rdata.obj" \
	"$(INTDIR)\rdatalist.obj" \
	"$(INTDIR)\rdataset.obj" \
	"$(INTDIR)\rdatasetiter.obj" \
	"$(INTDIR)\rdataslab.obj" \
	"$(INTDIR)\request.obj" \
	"$(INTDIR)\resolver.obj" \
	"$(INTDIR)\result.obj" \
	"$(INTDIR)\rootns.obj" \
	"$(INTDIR)\sdb.obj" \
	"$(INTDIR)\sdlz.obj" \
	"$(INTDIR)\soa.obj" \
	"$(INTDIR)\ssu.obj" \
	"$(INTDIR)\stats.obj" \
	"$(INTDIR)\tcpmsg.obj" \
	"$(INTDIR)\time.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkey.obj" \
	"$(INTDIR)\tsig.obj" \
	"$(INTDIR)\ttl.obj" \
	"$(INTDIR)\validator.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\view.obj" \
	"$(INTDIR)\xfrin.obj" \
	"$(INTDIR)\zone.obj" \
	"$(INTDIR)\zonekey.obj" \
	"$(INTDIR)\zt.obj" \
	"$(INTDIR)\dst_api.obj" \
	"$(INTDIR)\dst_lib.obj" \
	"$(INTDIR)\dst_parse.obj" \
	"$(INTDIR)\dst_result.obj" \
	"$(INTDIR)\gssapi_link.obj" \
	"$(INTDIR)\gssapictx.obj" \
	"$(INTDIR)\spnego.obj" \
	"$(INTDIR)\hmac_link.obj" \
	"$(INTDIR)\key.obj" \
	"$(INTDIR)\openssl_link.obj" \
	"$(INTDIR)\openssldh_link.obj" \
	"$(INTDIR)\openssldsa_link.obj" \
	"$(INTDIR)\opensslrsa_link.obj" \
	"..\..\isc\win32\Debug\libisc.lib"

"..\..\..\Build\Debug\libdns.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
  $(_VC_MANIFEST_EMBED_DLL)

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libdns.dep")
!INCLUDE "libdns.dep"
!ELSE 
!MESSAGE Warning: cannot find "libdns.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libdns - Win32 Release" || "$(CFG)" == "libdns - Win32 Debug"
SOURCE=..\acache.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\acache.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\acache.obj"	"$(INTDIR)\acache.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\acl.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\acl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\acl.obj"	"$(INTDIR)\acl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\adb.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\adb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\adb.obj"	"$(INTDIR)\adb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\byaddr.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\byaddr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\byaddr.obj"	"$(INTDIR)\byaddr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\cache.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\cache.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\cache.obj"	"$(INTDIR)\cache.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\callbacks.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\callbacks.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\callbacks.obj"	"$(INTDIR)\callbacks.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\compress.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\compress.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\compress.obj"	"$(INTDIR)\compress.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\db.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\db.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\db.obj"	"$(INTDIR)\db.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dbiterator.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dbiterator.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dbiterator.obj"	"$(INTDIR)\dbiterator.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dbtable.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dbtable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dbtable.obj"	"$(INTDIR)\dbtable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\diff.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\diff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\diff.obj"	"$(INTDIR)\diff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dispatch.c

!IF  "$(CFG)" == "libdns - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "../../../../../openssl-0.9.8d/inc32/openssl/include" /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /Fp"$(INTDIR)\libdns.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dispatch.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\libdns.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\dispatch.obj"	"$(INTDIR)\dispatch.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\dlz.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dlz.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"

"$(INTDIR)\dlz.obj"	"$(INTDIR)\dlz.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\DLLMain.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\DLLMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\DLLMain.obj"	"$(INTDIR)\DLLMain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\dnssec.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dnssec.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dnssec.obj"	"$(INTDIR)\dnssec.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\ds.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\ds.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\ds.obj"	"$(INTDIR)\ds.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\forward.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\forward.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\forward.obj"	"$(INTDIR)\forward.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\iptable.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\iptable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\iptable.obj"	"$(INTDIR)\iptable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\journal.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\journal.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\journal.obj"	"$(INTDIR)\journal.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\keytable.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\keytable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\keytable.obj"	"$(INTDIR)\keytable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lib.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\lib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\lib.obj"	"$(INTDIR)\lib.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\log.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\log.obj"	"$(INTDIR)\log.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\lookup.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\lookup.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\lookup.obj"	"$(INTDIR)\lookup.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\master.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\master.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\master.obj"	"$(INTDIR)\master.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\masterdump.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\masterdump.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\masterdump.obj"	"$(INTDIR)\masterdump.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\message.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\message.obj"	"$(INTDIR)\message.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\name.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\name.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\name.obj"	"$(INTDIR)\name.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\ncache.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\ncache.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\ncache.obj"	"$(INTDIR)\ncache.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\nsec.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\nsec.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\nsec.obj"	"$(INTDIR)\nsec.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\nsec3.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\nsec3.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\nsec3.obj"	"$(INTDIR)\nsec3.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\order.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\order.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\order.obj"	"$(INTDIR)\order.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\peer.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\peer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\peer.obj"	"$(INTDIR)\peer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


SOURCE=..\portlist.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\portlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\portlist.obj"	"$(INTDIR)\portlist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rbt.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rbt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rbt.obj"	"$(INTDIR)\rbt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rbtdb.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rbtdb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rbtdb.obj"	"$(INTDIR)\rbtdb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rbtdb64.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rbtdb64.obj" : $(SOURCE) "$(INTDIR)" "..\rbtdb.c"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rbtdb64.obj"	"$(INTDIR)\rbtdb64.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rcode.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rcode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rcode.obj"	"$(INTDIR)\rcode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rdata.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rdata.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rdata.obj"	"$(INTDIR)\rdata.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rdatalist.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rdatalist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rdatalist.obj"	"$(INTDIR)\rdatalist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rdataset.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rdataset.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rdataset.obj"	"$(INTDIR)\rdataset.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rdatasetiter.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rdatasetiter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rdatasetiter.obj"	"$(INTDIR)\rdatasetiter.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rdataslab.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rdataslab.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rdataslab.obj"	"$(INTDIR)\rdataslab.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\request.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\request.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\request.obj"	"$(INTDIR)\request.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\resolver.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\resolver.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\resolver.obj"	"$(INTDIR)\resolver.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\result.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\result.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\result.obj"	"$(INTDIR)\result.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\rootns.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\rootns.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\rootns.obj"	"$(INTDIR)\rootns.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\sdb.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\sdb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\sdb.obj"	"$(INTDIR)\sdb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\sdlz.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\sdlz.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\sdlz.obj"	"$(INTDIR)\sdlz.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\soa.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\soa.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\soa.obj"	"$(INTDIR)\soa.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\ssu.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\ssu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\ssu.obj"	"$(INTDIR)\ssu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\stats.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\stats.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\stats.obj"	"$(INTDIR)\stats.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\tcpmsg.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\tcpmsg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\tcpmsg.obj"	"$(INTDIR)\tcpmsg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\time.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\time.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\time.obj"	"$(INTDIR)\time.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\timer.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\timer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\timer.obj"	"$(INTDIR)\timer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\tkey.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\tkey.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\tkey.obj"	"$(INTDIR)\tkey.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\tsig.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\tsig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\tsig.obj"	"$(INTDIR)\tsig.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\ttl.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\ttl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\ttl.obj"	"$(INTDIR)\ttl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\validator.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\validator.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\validator.obj"	"$(INTDIR)\validator.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\version.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\view.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\view.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\view.obj"	"$(INTDIR)\view.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\xfrin.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\xfrin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\xfrin.obj"	"$(INTDIR)\xfrin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zone.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\zone.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\zone.obj"	"$(INTDIR)\zone.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zonekey.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\zonekey.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\zonekey.obj"	"$(INTDIR)\zonekey.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\zt.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\zt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\zt.obj"	"$(INTDIR)\zt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dst_api.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dst_api.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dst_api.obj"	"$(INTDIR)\dst_api.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dst_lib.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dst_lib.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dst_lib.obj"	"$(INTDIR)\dst_lib.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dst_parse.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dst_parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dst_parse.obj"	"$(INTDIR)\dst_parse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\dst_result.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\dst_result.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\dst_result.obj"	"$(INTDIR)\dst_result.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\gssapi_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\gssapi_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\gssapi_link.obj"	"$(INTDIR)\gssapi_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\gssapictx.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\gssapictx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\gssapictx.obj"	"$(INTDIR)\gssapictx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\spnego.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\spnego.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\spnego.obj"	"$(INTDIR)\spnego.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\hmac_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\hmac_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\hmac_link.obj"	"$(INTDIR)\hmac_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\key.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\key.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\key.obj"	"$(INTDIR)\key.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\openssl_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\openssl_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\openssl_link.obj"	"$(INTDIR)\openssl_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\openssldh_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\openssldh_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\openssldh_link.obj"	"$(INTDIR)\openssldh_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\openssldsa_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\openssldsa_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\openssldsa_link.obj"	"$(INTDIR)\openssldsa_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\opensslrsa_link.c

!IF  "$(CFG)" == "libdns - Win32 Release"


"$(INTDIR)\opensslrsa_link.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"


"$(INTDIR)\opensslrsa_link.obj"	"$(INTDIR)\opensslrsa_link.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

!IF  "$(CFG)" == "libdns - Win32 Release"

"libisc - Win32 Release" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" 
   cd "..\..\dns\win32"

"libisc - Win32 ReleaseCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\dns\win32"

!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"

"libisc - Win32 Debug" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" 
   cd "..\..\dns\win32"

"libisc - Win32 DebugCLEAN" : 
   cd "..\..\isc\win32"
   $(MAKE) /$(MAKEFLAGS) /F ".\libisc.mak" CFG="libisc - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\dns\win32"

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
