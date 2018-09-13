# Microsoft Developer Studio Project File - Name="shdocvw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=shdocvw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shdocvw_.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shdocvw_.mak" CFG="shdocvw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shdocvw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shdocvw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shdocvw - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHDOCVW_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHDOCVW_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "shdocvw - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHDOCVW_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /W3 /WX /ZI /X /I ".\..\..\..\public\sdk\lib\i386" /I "." /I ".\..\..\..\private\shell\inc" /I ".\..\..\..\private\shell\inc\stubs" /I ".\..\..\..\private\windows\inc" /I ".\..\..\..\public\sdk\inc" /I ".\..\..\..\private\inc" /I "objd\i386" /I ".\..\..\..\public\oak\inc" /I ".\..\..\..\public\sdk\inc\crt" /FI"\nt\public\sdk\inc\warning.h" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0500 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _MT=1 /D "NASHVILLE" /D "IN_SHDOCVW" /D "FEATURE_FRAMES" /D MSOCT2=1 /D "BETA1_DIALMON_HACK" /D "FEATURE_URLHIST" /D "FEATURE_PICS" /D "PAGER" /D "_HSFOLDER" /D "_NTSDK" /D "_USRDLL" /D "_WINNT" /D "VSTF" /D "WIN32" /D "_WIN32" /D "POSTSPLIT" /D "USE_MIRRORING" /D "NT" /D "DEBUG" /D "NO_NOVTABLE" /D "WINNT_ENV" /D "FEATURE_IE40" /D "NASH" /D "NOWINRES" /YX"priv.h" /FD /Zel /QIfdiv- /QIf /QI0f /GF /Odi /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /i ".\dll\objd\i386" /i "\nt\public\sdk\lib\i386" /i ".." /i "\nt\private\shell\inc" /i "\nt\private\shell\inc\stubs" /i "\nt\private\windows\inc" /i "\nt\public\sdk\inc" /i "\nt\private\inc" /i "objd\i386" /i "\nt\public\oak\inc" /i "\nt\public\sdk\inc\crt" /d "_DEBUG" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0400 /d WINVER=0x0400 /d _WIN32_IE=0x0500 /d DBG=1 /d DEVL=1 /d FPO=0 /d "NDEBUG" /d _MT=1 /d "NASHVILLE" /d "IN_SHDOCVW" /d "FEATURE_FRAMES" /d MSOCT2=1 /d "BETA1_DIALMON_HACK" /d "FEATURE_URLHIST" /d "FEATURE_PICS" /d "PAGER" /d "_HSFOLDER" /d "_NTSDK" /d "_USRDLL" /d "_WINNT" /d "VSTF" /d "_WIN32" /d "POSTSPLIT" /d "USE_MIRRORING" /d "NT" /d "DEBUG" /d "NO_NOVTABLE" /d "WINNT_ENV" /d "FEATURE_IE40" /d "NASH" /d "NOWINRES" /d WIN32=100
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 srcc\objd\i386\srcc.lib util\objd\i386\util.lib .\hist\objd\i386\hist.lib .\cdfview\objd\i386\cdfview.lib .\..\..\..\public\sdk\lib\i386\int64.lib .\..\..\..\public\sdk\lib\i386\iert.lib .\..\..\..\public\sdk\lib\i386\kernel32.lib .\..\..\..\public\sdk\lib\i386\shlwapip.lib .\..\..\..\public\sdk\lib\i386\gdi32.lib .\..\..\..\public\sdk\lib\i386\user32.lib .\..\..\..\public\sdk\lib\i386\advapi32.lib .\..\..\..\public\sdk\lib\i386\uuid.lib .\..\..\..\public\sdk\lib\i386\ole32.lib .\..\..\..\public\sdk\lib\i386\htmlhelp.lib .\..\..\..\private\shell\lib\objd\i386\shguidp.lib .\..\..\..\public\sdk\lib\i386\comctl32.lib .\..\..\..\public\sdk\lib\i386\comctlp.lib .\..\..\..\private\iedev\lib\chicago\i386\shell32.w95 .\..\..\..\private\shell\lib\objd\i386\stock5.lib .\..\..\..\private\shell\lib\objd\i386\stocklib.lib .\..\..\..\public\sdk\lib\i386\delayload.lib /nologo /base:"0x71500000" /version:5.0 /stack:0x40000,0x1000 /entry:"LibMain@12" /dll /machine:I386 /nodefaultlib /def:"dll\objd\i386\shdocvw.def" /pdbtype:sept -MERGE:.CRT=.data -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -FULLBUILD -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4198 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text /delayload:OLE32.DLL -subsystem:windows,4.00
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "shdocvw - Win32 Release"
# Name "shdocvw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\about.cpp
# End Source File
# Begin Source File

SOURCE=.\basesb.cpp
# End Source File
# Begin Source File

SOURCE=.\bcwx.cpp
# End Source File
# Begin Source File

SOURCE=.\bindcb.cpp
# End Source File
# Begin Source File

SOURCE=.\cachecln.cpp
# End Source File
# Begin Source File

SOURCE=.\caggunk.cpp
# End Source File
# Begin Source File

SOURCE=.\chanoc.cpp
# End Source File
# Begin Source File

SOURCE=.\clslock.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdfile.cpp
# End Source File
# Begin Source File

SOURCE=.\cnctnpt.cpp
# End Source File
# Begin Source File

SOURCE=.\cobjsafe.cpp
# End Source File
# Begin Source File

SOURCE=.\cowsite.cpp
# End Source File
# Begin Source File

SOURCE=.\cwndproc.cpp
# End Source File
# Begin Source File

SOURCE=.\DEBDUMP.CPP
# End Source File
# Begin Source File

SOURCE=.\dhuihand.cpp
# End Source File
# Begin Source File

SOURCE=.\dllreg.cpp
# End Source File
# Begin Source File

SOURCE=.\dochost.cpp
# End Source File
# Begin Source File

SOURCE=.\dochostbsc.cpp
# End Source File
# Begin Source File

SOURCE=.\download.cpp
# End Source File
# Begin Source File

SOURCE=.\dspsprt.cpp
# End Source File
# Begin Source File

SOURCE=.\expdsprt.cpp
# End Source File
# Begin Source File

SOURCE=.\favorite.cpp
# End Source File
# Begin Source File

SOURCE=.\filter.cpp
# End Source File
# Begin Source File

SOURCE=.\fldset.cpp
# End Source File
# Begin Source File

SOURCE=.\history.cpp
# End Source File
# Begin Source File

SOURCE=.\hlframe.cpp
# End Source File
# Begin Source File

SOURCE=.\iedde.cpp
# End Source File
# Begin Source File

SOURCE=.\iedisp.cpp
# End Source File
# Begin Source File

SOURCE=.\iethread.cpp
# End Source File
# Begin Source File

SOURCE=.\impexp.cpp
# End Source File
# Begin Source File

SOURCE=.\infotip.cpp
# End Source File
# Begin Source File

SOURCE=.\inpobj.cpp
# End Source File
# Begin Source File

SOURCE=.\inst.cpp
# End Source File
# Begin Source File

SOURCE=.\ipstg.cpp
# End Source File
# Begin Source File

SOURCE=.\isbase.cpp
# End Source File
# Begin Source File

SOURCE=.\isdtobj.cpp
# End Source File
# Begin Source File

SOURCE=.\isexicon.cpp
# End Source File
# Begin Source File

SOURCE=.\isnewshk.cpp
# End Source File
# Begin Source File

SOURCE=.\ispersis.cpp
# End Source File
# Begin Source File

SOURCE=.\isprsht.cpp
# End Source File
# Begin Source File

SOURCE=.\isshlink.cpp
# End Source File
# Begin Source File

SOURCE=.\isurl.cpp
# End Source File
# Begin Source File

SOURCE=.\libx.cpp
# End Source File
# Begin Source File

SOURCE=.\mainloop.cpp
# End Source File
# Begin Source File

SOURCE=.\mime64.cpp
# End Source File
# Begin Source File

SOURCE=.\mruex.cpp
# End Source File
# Begin Source File

SOURCE=.\multimon.cpp
# End Source File
# Begin Source File

SOURCE=.\olestuff.cpp
# End Source File
# Begin Source File

SOURCE=.\opsprof.cpp
# End Source File
# Begin Source File

SOURCE=.\packager.cpp
# End Source File
# Begin Source File

SOURCE=.\ratings.cpp
# End Source File
# Begin Source File

SOURCE=.\reload.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\sftupmb.cpp
# End Source File
# Begin Source File

SOURCE=.\shdocfl.cpp
# End Source File
# Begin Source File

SOURCE=.\shdocvw.cpp
# End Source File
# Begin Source File

SOURCE=.\shdocvw.rc
# End Source File
# Begin Source File

SOURCE=.\shell32.cpp
# End Source File
# Begin Source File

SOURCE=.\shembed.cpp
# End Source File
# Begin Source File

SOURCE=.\shocx.cpp
# End Source File
# Begin Source File

SOURCE=.\shuioc.cpp
# End Source File
# Begin Source File

SOURCE=.\shvocx.cpp
# End Source File
# Begin Source File

SOURCE=.\site.cpp
# End Source File
# Begin Source File

SOURCE=.\sitelist.cpp
# End Source File
# Begin Source File

SOURCE=.\splash.cpp
# End Source File
# Begin Source File

SOURCE=.\stdenum.cpp
# End Source File
# Begin Source File

SOURCE=.\strbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\stream.cpp
# End Source File
# Begin Source File

SOURCE=.\swindows.cpp
# End Source File
# Begin Source File

SOURCE=.\tasklist.cpp
# End Source File
# Begin Source File

SOURCE=.\tframe.cpp
# End Source File
# Begin Source File

SOURCE=.\thicket.cpp
# End Source File
# Begin Source File

SOURCE=.\tlog.cpp
# End Source File
# Begin Source File

SOURCE=.\trsite.cpp
# End Source File
# Begin Source File

SOURCE=.\url.cpp
# End Source File
# Begin Source File

SOURCE=.\urlassoc.cpp
# End Source File
# Begin Source File

SOURCE=.\urlhist.cpp
# End Source File
# Begin Source File

SOURCE=.\urlhook.cpp
# End Source File
# Begin Source File

SOURCE=.\urlprop.cpp
# End Source File
# Begin Source File

SOURCE=.\urltrack.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\winlist.cpp
# End Source File
# Begin Source File

SOURCE=.\wvt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\admovr2.h
# End Source File
# Begin Source File

SOURCE=.\advanced.h
# End Source File
# Begin Source File

SOURCE=.\assocurl.h
# End Source File
# Begin Source File

SOURCE=.\asyncrat.h
# End Source File
# Begin Source File

SOURCE=.\autoscrl.h
# End Source File
# Begin Source File

SOURCE=.\basesb.h
# End Source File
# Begin Source File

SOURCE=.\bcw.h
# End Source File
# Begin Source File

SOURCE=.\bindcb.h
# End Source File
# Begin Source File

SOURCE=.\cabdde.h
# End Source File
# Begin Source File

SOURCE=.\cabsh.h
# End Source File
# Begin Source File

SOURCE=.\caggunk.h
# End Source File
# Begin Source File

SOURCE=.\chanbar.h
# End Source File
# Begin Source File

SOURCE=.\channel.h
# End Source File
# Begin Source File

SOURCE=.\cobjsafe.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\convert.h
# End Source File
# Begin Source File

SOURCE=.\cowsite.h
# End Source File
# Begin Source File

SOURCE=.\cwndproc.h
# End Source File
# Begin Source File

SOURCE=.\dhuihand.h
# End Source File
# Begin Source File

SOURCE=.\dllload.h
# End Source File
# Begin Source File

SOURCE=.\dochost.h
# End Source File
# Begin Source File

SOURCE=.\dpastuff.h
# End Source File
# Begin Source File

SOURCE=.\droptgt.h
# End Source File
# Begin Source File

SOURCE=.\dspsprt.h
# End Source File
# Begin Source File

SOURCE=.\dynastr.h
# End Source File
# Begin Source File

SOURCE=.\events.h
# End Source File
# Begin Source File

SOURCE=.\expdsprt.h
# End Source File
# Begin Source File

SOURCE=.\favorite.h
# End Source File
# Begin Source File

SOURCE=.\filter.h
# End Source File
# Begin Source File

SOURCE=.\hist.h
# End Source File
# Begin Source File

SOURCE=.\hlframe.h
# End Source File
# Begin Source File

SOURCE=.\hlinkp.h
# End Source File
# Begin Source File

SOURCE=.\hnfblock.h
# End Source File
# Begin Source File

SOURCE=.\htmlstr.h
# End Source File
# Begin Source File

SOURCE=.\htregmng.h
# End Source File
# Begin Source File

SOURCE=.\idlist.h
# End Source File
# Begin Source File

SOURCE=.\iedde.h
# End Source File
# Begin Source File

SOURCE=.\iface.h
# End Source File
# Begin Source File

SOURCE=.\impexp.h
# End Source File
# Begin Source File

SOURCE=.\infotip.h
# End Source File
# Begin Source File

SOURCE=.\inpobj.h
# End Source File
# Begin Source File

SOURCE=.\ipstg.h
# End Source File
# Begin Source File

SOURCE=.\ishcut.h
# End Source File
# Begin Source File

SOURCE=.\logo.h
# End Source File
# Begin Source File

SOURCE=.\mime64.h
# End Source File
# Begin Source File

SOURCE=.\mruex.h
# End Source File
# Begin Source File

SOURCE=.\msstkppg.h
# End Source File
# Begin Source File

SOURCE=.\mttf.h
# End Source File
# Begin Source File

SOURCE=.\multinfo.h
# End Source File
# Begin Source File

SOURCE=.\notsink.h
# End Source File
# Begin Source File

SOURCE=.\objwsite.h
# End Source File
# Begin Source File

SOURCE=.\packager.h
# End Source File
# Begin Source File

SOURCE=.\ppages.h
# End Source File
# Begin Source File

SOURCE=.\priv.h
# End Source File
# Begin Source File

SOURCE=.\propstg.h
# End Source File
# Begin Source File

SOURCE=.\proxyisf.h
# End Source File
# Begin Source File

SOURCE=.\reload.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\runonnt.h
# End Source File
# Begin Source File

SOURCE=.\sccls.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\shembed.h
# End Source File
# Begin Source File

SOURCE=.\shocx.h
# End Source File
# Begin Source File

SOURCE=.\shvocx.h
# End Source File
# Begin Source File

SOURCE=.\site.h
# End Source File
# Begin Source File

SOURCE=.\sitelist.h
# End Source File
# Begin Source File

SOURCE=.\stdenum.h
# End Source File
# Begin Source File

SOURCE=.\strbuf.h
# End Source File
# Begin Source File

SOURCE=.\stream.h
# End Source File
# Begin Source File

SOURCE=.\theater.h
# End Source File
# Begin Source File

SOURCE=.\thicket.h
# End Source File
# Begin Source File

SOURCE=.\track.h
# End Source File
# Begin Source File

SOURCE=.\trsite.h
# End Source File
# Begin Source File

SOURCE=.\unixstuff.h
# End Source File
# Begin Source File

SOURCE=.\urlprop.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\utilmenu.h
# End Source File
# Begin Source File

SOURCE=.\winlist.h
# End Source File
# Begin Source File

SOURCE=.\wvtp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
