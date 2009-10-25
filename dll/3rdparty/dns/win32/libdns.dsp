# Microsoft Developer Studio Project File - Name="libdns" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libdns - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libdns.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libdns - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libdns_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../../../openssl-0.9.8d/inc32/openssl/include" /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 user32.lib advapi32.lib ws2_32.lib ../../isc/win32/Release/libisc.lib ../../../../openssl-0.9.8d/out32dll/libeay32.lib /nologo /dll /machine:I386 /out:"../../../Build/Release/libdns.dll"

!ELSEIF  "$(CFG)" == "libdns - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "libdns_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../../../" /I "include" /I "../include" /I "../../isc/win32" /I "../../isc/win32/include" /I "../../isc/include" /I "../../isc/noatomic/include" /I "../../../../openssl-0.9.8d/inc32" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__STDC__" /D "_MBCS" /D "_USRDLL" /D "USE_MD5" /D "OPENSSL" /D "DST_USE_PRIVATE_OPENSSL" /D "LIBDNS_EXPORTS" /FR /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib advapi32.lib ws2_32.lib ../../isc/win32/debug/libisc.lib ../../../../openssl-0.9.8d/out32dll/libeay32.lib /nologo /dll /map /debug /machine:I386 /out:"../../../Build/Debug/libdns.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libdns - Win32 Release"
# Name "libdns - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\dns\acache.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\acl.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\adb.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\bit.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\byaddr.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\cache.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\callbacks.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\cert.h
# End Source File
# Begin Source File

SOURCE=..\code.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\compress.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\db.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\dbiterator.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\dbtable.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\diff.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\dispatch.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\dlz.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\dnssec.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\ds.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\enumclass.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\enumtype.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\events.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\fixedname.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\forward.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\iptable.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\journal.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\keyflags.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\keytable.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\keyvalues.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\lib.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\log.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\lookup.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\master.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\masterdump.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\message.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\name.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\ncache.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\nsec.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\nsec3.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\order.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\peer.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\portlist.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rbt.h
# End Source File
# Begin Source File

SOURCE=..\rbtdb.h
# End Source File
# Begin Source File

SOURCE=..\rbtdb64.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rcode.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdata.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdataclass.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdatalist.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdataset.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdatasetiter.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdataslab.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdatastruct.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rdatatype.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\request.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\resolver.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\result.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\rootns.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\sdb.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\sdlz.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\secalg.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\secproto.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\soa.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\ssu.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\stats.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\tcpmsg.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\time.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\timer.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\tkey.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\tsig.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\ttl.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\types.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\validator.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\version.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\view.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\xfrin.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\zone.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\zonekey.h
# End Source File
# Begin Source File

SOURCE=..\include\dns\zt.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Main Dns Lib"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\acache.c
# End Source File
# Begin Source File

SOURCE=..\acl.c
# End Source File
# Begin Source File

SOURCE=..\adb.c
# End Source File
# Begin Source File

SOURCE=..\byaddr.c
# End Source File
# Begin Source File

SOURCE=..\cache.c
# End Source File
# Begin Source File

SOURCE=..\callbacks.c
# End Source File
# Begin Source File

SOURCE=..\compress.c
# End Source File
# Begin Source File

SOURCE=..\db.c
# End Source File
# Begin Source File

SOURCE=..\dbiterator.c
# End Source File
# Begin Source File

SOURCE=..\dbtable.c
# End Source File
# Begin Source File

SOURCE=..\diff.c
# End Source File
# Begin Source File

SOURCE=..\dispatch.c
# End Source File
# Begin Source File

SOURCE=..\dlz.c
# End Source File
# Begin Source File

SOURCE=.\DLLMain.c
# End Source File
# Begin Source File

SOURCE=..\dnssec.c
# End Source File
# Begin Source File

SOURCE=..\ds.c
# End Source File
# Begin Source File

SOURCE=..\forward.c
# End Source File
# Begin Source File

SOURCE=..\iptable.c
# End Source File
# Begin Source File

SOURCE=..\journal.c
# End Source File
# Begin Source File

SOURCE=..\keytable.c
# End Source File
# Begin Source File

SOURCE=..\lib.c
# End Source File
# Begin Source File

SOURCE=..\log.c
# End Source File
# Begin Source File

SOURCE=..\lookup.c
# End Source File
# Begin Source File

SOURCE=..\master.c
# End Source File
# Begin Source File

SOURCE=..\masterdump.c
# End Source File
# Begin Source File

SOURCE=..\message.c
# End Source File
# Begin Source File

SOURCE=..\name.c
# End Source File
# Begin Source File

SOURCE=..\ncache.c
# End Source File
# Begin Source File

SOURCE=..\nsec.c
# End Source File
# Begin Source File

SOURCE=..\nsec3.c
# End Source File
# Begin Source File

SOURCE=..\order.c
# End Source File
# Begin Source File

SOURCE=..\peer.c
# End Source File
# Begin Source File

SOURCE=..\portlist.c
# End Source File
# Begin Source File

SOURCE=..\rbt.c
# End Source File
# Begin Source File

SOURCE=..\rbtdb.c
# End Source File
# Begin Source File

SOURCE=..\rbtdb64.c
# End Source File
# Begin Source File

SOURCE=..\rcode.c
# End Source File
# Begin Source File

SOURCE=..\rdata.c
# End Source File
# Begin Source File

SOURCE=..\rdatalist.c
# End Source File
# Begin Source File

SOURCE=..\rdataset.c
# End Source File
# Begin Source File

SOURCE=..\rdatasetiter.c
# End Source File
# Begin Source File

SOURCE=..\rdataslab.c
# End Source File
# Begin Source File

SOURCE=..\request.c
# End Source File
# Begin Source File

SOURCE=..\resolver.c
# End Source File
# Begin Source File

SOURCE=..\result.c
# End Source File
# Begin Source File

SOURCE=..\rootns.c
# End Source File
# Begin Source File

SOURCE=..\sdb.c
# End Source File
# Begin Source File

SOURCE=..\soa.c
# End Source File
# Begin Source File

SOURCE=..\sdlz.c
# End Source File
# Begin Source File

SOURCE=..\ssu.c
# End Source File
# Begin Source File

SOURCE=..\stats.c
# End Source File
# Begin Source File

SOURCE=..\tcpmsg.c
# End Source File
# Begin Source File

SOURCE=..\time.c
# End Source File
# Begin Source File

SOURCE=..\timer.c
# End Source File
# Begin Source File

SOURCE=..\tkey.c
# End Source File
# Begin Source File

SOURCE=..\tsig.c
# End Source File
# Begin Source File

SOURCE=..\ttl.c
# End Source File
# Begin Source File

SOURCE=..\validator.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# Begin Source File

SOURCE=..\view.c
# End Source File
# Begin Source File

SOURCE=..\xfrin.c
# End Source File
# Begin Source File

SOURCE=..\zone.c
# End Source File
# Begin Source File

SOURCE=..\zonekey.c
# End Source File
# Begin Source File

SOURCE=..\zt.c
# End Source File
# End Group
# Begin Group "dst"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\dst_api.c
# End Source File
# Begin Source File

SOURCE=..\dst_lib.c
# End Source File
# Begin Source File

SOURCE=..\dst_parse.c
# End Source File
# Begin Source File

SOURCE=..\dst_result.c
# End Source File
# Begin Source File

SOURCE=..\gssapi_link.c
# End Source File
# Begin Source File

SOURCE=..\gssapictx.c
# End Source File
# Begin Source File

SOURCE=..\spnego.c
# End Source File
# Begin Source File

SOURCE=..\hmac_link.c
# End Source File
# Begin Source File

SOURCE=..\key.c
# End Source File
# Begin Source File

SOURCE=..\openssl_link.c
# End Source File
# Begin Source File

SOURCE=..\openssldh_link.c
# End Source File
# Begin Source File

SOURCE=..\openssldsa_link.c
# End Source File
# Begin Source File

SOURCE=..\opensslrsa_link.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\libdns.def
# End Source File
# End Target
# End Project
