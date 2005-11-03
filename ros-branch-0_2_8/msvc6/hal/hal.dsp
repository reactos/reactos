# Microsoft Developer Studio Project File - Name="hal" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hal - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hal.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hal.mak" CFG="hal - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hal - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hal - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hal - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Ot /Og /Os /Ob1 /I "..\..\reactos\include" /I "..\..\reactos\hal\halx86\include" /I "..\..\reactos\ntoskrnl\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "i386" /D "__NTHAL__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x417 /i "..\..\reactos\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:I386
# ADD LINK32 ..\ntoskrnl\Release\ntoskrnl.lib /nologo /dll /machine:I386 /fixed:no
# SUBTRACT LINK32 /pdb:none /force

!ELSEIF  "$(CFG)" == "hal - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Z7 /Od /I "..\..\reactos\include" /I "..\..\reactos\hal\halx86\include" /I "..\..\reactos\ntoskrnl\include" /D "_WINDOWS" /D "__NTHAL__" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "i386" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x417 /i "..\..\reactos\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\ntoskrnl\Debug\ntoskrnl.lib /nologo /dll /incremental:no /debug /machine:I386 /pdbtype:sept /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "hal - Win32 Release"
# Name "hal - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\adapter.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\beep.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\bus.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\display.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\dma.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\drive.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\enum.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\fmutex.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\fmutex_tmn.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\halinit.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\halx86mp.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\halx86up.rc
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\irql.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\isa.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\kdbg.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\mca.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\mp.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\mpsirql.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\pci.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\portio.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\pwroff.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\reboot.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\spinlock.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\spinlock_tmn.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\sysbus.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\sysinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\time.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\timer.c
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\include\bus.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\include\hal.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\hal\halx86\include\mps.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\hal.def
# End Source File
# End Target
# End Project
