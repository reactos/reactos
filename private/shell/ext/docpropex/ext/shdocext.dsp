# Microsoft Developer Studio Project File - Name="ShDocExt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ShDocExt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ShDocExt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ShDocExt.mak" CFG="ShDocExt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ShDocExt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShDocExt - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShDocExt - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShDocExt - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\..\..\..\windows\inc" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libcmtd.lib winmm.lib ..\srv\imageflt\image.lib vfw32.lib msacm32.lib shdocvw.lib shell32p.lib shlwapi.lib shlwapip.lib comctl32.lib comctlp.lib oledlg.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"..\binaries\Debug\docprop2.dll" /pdbtype:sept
# Begin Custom Build - Registering COM Server...
OutDir=.\Debug
TargetPath=\nt\private\shell\ext\docpropex\binaries\Debug\docprop2.dll
InputPath=\nt\private\shell\ext\docpropex\binaries\Debug\docprop2.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\inc" /I "..\..\..\..\windows\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /FR /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libcmtd.lib mmsystem.lib, ..\srv\imageflt\image.lib winmm.lib vfw32.lib msacm32.lib shdocvw.lib shell32p.lib shlwapi.lib shlwapip.lib comctl32.lib comctlp.lib oledlg.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"..\binaries\DebugU\docprop2.dll" /pdbtype:sept
# Begin Custom Build - Registering COM Server...
OutDir=.\DebugU
TargetPath=\nt\private\shell\ext\docpropex\binaries\DebugU\docprop2.dll
InputPath=\nt\private\shell\ext\docpropex\binaries\DebugU\docprop2.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ShDocPro"
# PROP BASE Intermediate_Dir "ShDocPro"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"pch.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\inc" /I "..\..\..\..\windows\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libcmt.lib mmsystem.lib, ..\srv\imageflt\image.lib winmm.lib vfw32.lib msacm32.lib shdocvw.lib shell32p.lib shlwapi.lib shlwapip.lib comctl32.lib comctlp.lib oledlg.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"..\binaries\Release\docprop2.dll"
# Begin Custom Build - Registering COM Server...
OutDir=.\Release
TargetPath=\nt\private\shell\ext\docpropex\binaries\Release\docprop2.dll
InputPath=\nt\private\shell\ext\docpropex\binaries\Release\docprop2.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ShDocPr0"
# PROP BASE Intermediate_Dir "ShDocPr0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"pch.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\inc" /I "..\..\..\..\windows\inc" /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"pch.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libcmt.lib mmsystem.lib, ..\srv\imageflt\image.lib winmm.lib vfw32.lib msacm32.lib shdocvw.lib shell32p.lib shlwapi.lib shlwapip.lib comctl32.lib comctlp.lib oledlg.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib version.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"..\binaries\ReleaseU\docprop2.dll"
# Begin Custom Build - Registering COM Server...
OutDir=.\ReleaseU
TargetPath=\nt\private\shell\ext\docpropex\binaries\ReleaseU\docprop2.dll
InputPath=\nt\private\shell\ext\docpropex\binaries\ReleaseU\docprop2.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ShDocExt - Win32 Debug"
# Name "ShDocExt - Win32 Unicode Debug"
# Name "ShDocExt - Win32 Release"
# Name "ShDocExt - Win32 Unicode Release"
# Begin Group "ActiveX Ctl - Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\ctl\ctl.cpp
# End Source File
# Begin Source File

SOURCE=..\ctl\ctlwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\ctl\dictionary.cpp
# End Source File
# Begin Source File

SOURCE=..\ctl\editctl.cpp
# End Source File
# Begin Source File

SOURCE=..\ctl\proptree.idl

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

# Begin Custom Build - MIDL - Proptree.IDL
InputPath=..\ctl\proptree.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\proptree.h" /iid "..\cmn\proptree_i.c"\
                /proxy "..\cmn\proptree_p.c" /tlb "..\ctl\proptree.tlb" "..\ctl\proptree.idl"

"..\ctl\proptree.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\inc\proptree.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\proptree_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# Begin Custom Build - MIDL - Proptree.IDL
InputPath=..\ctl\proptree.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\proptree.h" /iid "..\cmn\proptree_i.c"\
                /proxy "..\cmn\proptree_p.c" /tlb "..\ctl\proptree.tlb" "..\ctl\proptree.idl"

"..\ctl\proptree.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\inc\proptree.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\proptree_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# Begin Custom Build - MIDL - Proptree.IDL
InputPath=..\ctl\proptree.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\proptree.h" /iid "..\cmn\proptree_i.c"\
                /proxy "..\cmn\proptree_p.c" /tlb "..\ctl\proptree.tlb" "..\ctl\proptree.idl"

"..\ctl\proptree.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\inc\proptree.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\proptree_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# Begin Custom Build - MIDL - Proptree.IDL
InputPath=..\ctl\proptree.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\proptree.h" /iid "..\cmn\proptree_i.c"\
                /proxy "..\cmn\proptree_p.c" /tlb "..\ctl\proptree.tlb" "..\ctl\proptree.idl"

"..\ctl\proptree.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\inc\proptree.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\proptree_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\ctl\treeitems.cpp
# End Source File
# End Group
# Begin Group "ActiveX Ctl - Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\ctl\ctl.h
# End Source File
# Begin Source File

SOURCE=..\ctl\ctlconn.h
# End Source File
# Begin Source File

SOURCE=..\ctl\dictionary.h
# End Source File
# Begin Source File

SOURCE=..\ctl\metrics.h
# End Source File
# Begin Source File

SOURCE=..\ctl\treeitems.h
# End Source File
# End Group
# Begin Group "Default Property Server - Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\srv\colprov.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\DefProp.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\DefSrv32.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\Enum.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\imageprop.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\MruProp.cpp
# End Source File
# Begin Source File

SOURCE=..\srv\ptsrv32.idl
USERDEP__PTSRV="..\cmn\ptserver.idl"	

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

# Begin Custom Build - MIDL - PTSrv32
InputPath=..\srv\ptsrv32.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\ptsrv32.h" /iid "..\cmn\ptsrv32_i.c"\
               /proxy "..\cmn\ptsrv32_p.c" /tlb "..\srv\ptsrv32.tlb"  "..\srv\ptsrv32.idl"

"..\inc\ptsrv32.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptsrv32_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\srv\ptsrv32.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# Begin Custom Build - MIDL - PTSrv32
InputPath=..\srv\ptsrv32.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\ptsrv32.h" /iid "..\cmn\ptsrv32_i.c"\
               /proxy "..\cmn\ptsrv32_p.c" /tlb "..\srv\ptsrv32.tlb"  "..\srv\ptsrv32.idl"

"..\inc\ptsrv32.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptsrv32_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\srv\ptsrv32.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# Begin Custom Build - MIDL - PTSrv32
InputPath=..\srv\ptsrv32.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\ptsrv32.h" /iid "..\cmn\ptsrv32_i.c"\
               /proxy "..\cmn\ptsrv32_p.c" /tlb "..\srv\ptsrv32.tlb"  "..\srv\ptsrv32.idl"

"..\inc\ptsrv32.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptsrv32_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\srv\ptsrv32.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# Begin Custom Build - MIDL - PTSrv32
InputPath=..\srv\ptsrv32.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\ptsrv32.h" /iid "..\cmn\ptsrv32_i.c"\
               /proxy "..\cmn\ptsrv32_p.c" /tlb "..\srv\ptsrv32.tlb"  "..\srv\ptsrv32.idl"

"..\inc\ptsrv32.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptsrv32_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\srv\ptsrv32.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Default Property Server - Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\srv\DefProp.h
# End Source File
# Begin Source File

SOURCE=..\srv\DefSrv32.h
# End Source File
# Begin Source File

SOURCE=..\srv\Enum.h
# End Source File
# Begin Source File

SOURCE=..\srv\imageprop.h
# End Source File
# Begin Source File

SOURCE=..\srv\MruProp.h
# End Source File
# End Group
# Begin Group "Shell Extension - Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ext.cpp
# End Source File
# Begin Source File

SOURCE=.\page.cpp
# End Source File
# Begin Source File

SOURCE=.\shdocext.idl

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

# Begin Custom Build - Performing MIDL step
InputPath=.\shdocext.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\shdocext.h" /iid "..\cmn\shdocext_i.c"\
                /proxy "..\cmn\shdocext_p.c"  /tlb "..\ext\shdocext.tlb"  ".\shdocext.idl"

"..\inc\shdocext.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\ext\shdocext.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\shdocext_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# Begin Custom Build - Performing MIDL step
InputPath=.\shdocext.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\shdocext.h" /iid "..\cmn\shdocext_i.c"\
                /proxy "..\cmn\shdocext_p.c"  /tlb "..\ext\shdocext.tlb"  ".\shdocext.idl"

"..\inc\shdocext.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\ext\shdocext.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\shdocext_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# Begin Custom Build - Performing MIDL step
InputPath=.\shdocext.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\shdocext.h" /iid "..\cmn\shdocext_i.c"\
                /proxy "..\cmn\shdocext_p.c"  /tlb "..\ext\shdocext.tlb"  ".\shdocext.idl"

"..\inc\shdocext.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\ext\shdocext.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\shdocext_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# Begin Custom Build - Performing MIDL step
InputPath=.\shdocext.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\shdocext.h" /iid "..\cmn\shdocext_i.c"\
                /proxy "..\cmn\shdocext_p.c"  /tlb "..\ext\shdocext.tlb"  ".\shdocext.idl"

"..\inc\shdocext.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\ext\shdocext.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\shdocext_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Shell Extension - Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ext.h
# End Source File
# Begin Source File

SOURCE=.\page.h
# End Source File
# End Group
# Begin Group "Common Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\cmn\cpool.cpp
# End Source File
# Begin Source File

SOURCE=..\cmn\pchsink.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=..\cmn\propvar.cpp
# End Source File
# Begin Source File

SOURCE=..\cmn\ptdebug.cpp
# End Source File
# Begin Source File

SOURCE=..\cmn\ptserver.idl

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

# Begin Custom Build - MIDL - PTServer.IDL
InputPath=..\cmn\ptserver.idl

BuildCmds= \
	midl /Oicf /h "..\inc\ptserver.h" /iid "..\cmn\ptserver_i.c"\
                   "..\cmn\ptserver.idl"

"..\inc\ptserver.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptserver_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# Begin Custom Build - MIDL - PTServer.IDL
InputPath=..\cmn\ptserver.idl

BuildCmds= \
	midl /Oicf /h "..\inc\ptserver.h" /iid "..\cmn\ptserver_i.c"\
                   "..\cmn\ptserver.idl"

"..\inc\ptserver.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptserver_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# Begin Custom Build - MIDL - PTServer.IDL
InputPath=..\cmn\ptserver.idl

BuildCmds= \
	midl /Oicf /h "..\inc\ptserver.h" /iid "..\cmn\ptserver_i.c"\
                   "..\cmn\ptserver.idl"

"..\inc\ptserver.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptserver_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# Begin Custom Build - MIDL - PTServer.IDL
InputPath=..\cmn\ptserver.idl

BuildCmds= \
	midl /Oicf /h "..\inc\ptserver.h" /iid "..\cmn\ptserver_i.c"\
                   "..\cmn\ptserver.idl"

"..\inc\ptserver.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\ptserver_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\cmn\PTsniff.cpp
# End Source File
# Begin Source File

SOURCE=..\cmn\ptutil.cpp
# End Source File
# Begin Source File

SOURCE=.\ShDocExt.cpp
# End Source File
# Begin Source File

SOURCE=.\ShDocExt.def
# End Source File
# Begin Source File

SOURCE=.\ShDocExt.rc

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Common Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\cpool.h
# End Source File
# Begin Source File

SOURCE=..\inc\dictbase.h
# End Source File
# Begin Source File

SOURCE=..\inc\fmtids.h
# End Source File
# Begin Source File

SOURCE=.\pch.h
# End Source File
# Begin Source File

SOURCE=..\inc\prioritylst.h
# End Source File
# Begin Source File

SOURCE=..\inc\propvar.h
# End Source File
# Begin Source File

SOURCE=..\inc\ptdebug.h
# End Source File
# Begin Source File

SOURCE=..\inc\PTsniff.h
# End Source File
# Begin Source File

SOURCE=..\inc\ptutil.h
# End Source File
# Begin Source File

SOURCE=..\inc\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\ctl\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\ctl\res\TILsmall.bmp
# End Source File
# End Group
# Begin Group "AVProp - Source"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\avprop\avprop.idl

!IF  "$(CFG)" == "ShDocExt - Win32 Debug"

# Begin Custom Build
InputPath=..\avprop\avprop.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\avprop.h" /iid "..\cmn\avprop_i.c"  /proxy\
  "..\cmn\avprop_p.c" /tlb "..\avprop\avprop.tlb"  "..\avprop\avprop.idl"

"..\inc\avprop.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\avprop_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\avprop\avprop.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Debug"

# Begin Custom Build
InputPath=..\avprop\avprop.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\avprop.h" /iid "..\cmn\avprop_i.c"  /proxy\
  "..\cmn\avprop_p.c" /tlb "..\avprop\avprop.tlb"  "..\avprop\avprop.idl"

"..\inc\avprop.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\avprop_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\avprop\avprop.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Release"

# Begin Custom Build
InputPath=..\avprop\avprop.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\avprop.h" /iid "..\cmn\avprop_i.c"  /proxy\
  "..\cmn\avprop_p.c" /tlb "..\avprop\avprop.tlb"  "..\avprop\avprop.idl"

"..\inc\avprop.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\avprop_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\avprop\avprop.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ShDocExt - Win32 Unicode Release"

# Begin Custom Build
InputPath=..\avprop\avprop.idl

BuildCmds= \
	midl /Oicf /out "..\cmn" /h "..\inc\avprop.h" /iid "..\cmn\avprop_i.c"  /proxy\
  "..\cmn\avprop_p.c" /tlb "..\avprop\avprop.tlb"  "..\avprop\avprop.idl"

"..\inc\avprop.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\cmn\avprop_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\avprop\avprop.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\avprop\media.cpp
# End Source File
# Begin Source File

SOURCE=..\avprop\srv.cpp
# End Source File
# End Group
# Begin Group "AVProp - Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\avprop\media.h
# End Source File
# Begin Source File

SOURCE=..\avprop\srv.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\avprop\avprop.rgs
# End Source File
# Begin Source File

SOURCE=..\srv\colprov.rgs
# End Source File
# Begin Source File

SOURCE=..\ctl\ctl.rgs
# End Source File
# Begin Source File

SOURCE=..\srv\PTsrv32.rgs
# End Source File
# Begin Source File

SOURCE=.\shdocext.rgs
# End Source File
# Begin Source File

SOURCE=.\ShellExt.rgs
# End Source File
# End Target
# End Project
