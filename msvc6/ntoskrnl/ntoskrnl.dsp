# Microsoft Developer Studio Project File - Name="ntoskrnl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ntoskrnl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ntoskrnl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ntoskrnl.mak" CFG="ntoskrnl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ntoskrnl - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ntoskrnl - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Z7 /Ot /Og /Os /Ob1 /I "..\..\reactos\include" /I "..\..\reactos\ntoskrnl\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__NTOSKRNL__" /D "i386" /D "__i386__" /FD /c
# SUBTRACT CPP /Ox /X /YX
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 ..\hal\Release\hal.lib /nologo /base:"0xC0000000" /version:1 /stack:0x200000 /entry:"NtProcessStartup" /map /debug /machine:I386 /subsystem:native /order:@linkorder.txt
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Z7 /Od /Gy /I "..\..\reactos\include" /I "..\..\reactos\ntoskrnl\include" /D "_CONSOLE" /D "__NTOSKRNL__" /D "__i386__" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "i386" /FR /FD /c
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x417 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\hal\Debug\hal.lib /nologo /base:"0xC0000000" /version:1 /stack:0x200000 /entry:"NtProcessStartup" /incremental:no /map /debug /machine:I386 /pdbtype:sept /subsystem:native /order:@linkorder.txt
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ntoskrnl - Win32 Release"
# Name "ntoskrnl - Win32 Debug"
# Begin Group "cc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cc\cacheman.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cc\copy.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cc\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cc\pin.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cc\view.c
# End Source File
# End Group
# Begin Group "cm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\cm.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\import.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\ntfunc.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\regfile.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\regobj.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\cm\rtlfunc.c
# End Source File
# End Group
# Begin Group "dbg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\dbgctrl.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\errinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\kdb.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\kdb.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\kdb_keyboard.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\kdb_serial.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\kdb_stabs.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\print.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\profile.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\rdebug.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\dbg\user.c
# End Source File
# End Group
# Begin Group "ex"

# PROP Default_Filter ""
# Begin Group "i386_ex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\i386\interlck.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ex_i386"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ex_i386"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\btree.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\callback.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\fmutex.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\hashtab.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\init.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\interlck.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\irp.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\list.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\lookas.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\napi.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\parttab.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\power.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\resource.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\stree.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\sysinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\time.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\win32k.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\work.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ex\zone.c
# End Source File
# End Group
# Begin Group "fs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\dbcsname.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\filelock.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\mcb.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\mdl.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\name.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\notify.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\oplock.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\pool.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\tunnel.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\unc.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\fs\util.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\fs"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\fs"

!ENDIF 

# End Source File
# End Group
# Begin Group "inbv"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\inbv\inbv.c
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "internal"

# PROP Default_Filter ""
# Begin Group "i386_include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\i386\fpu.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\i386\ke.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\i386\mm.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\i386\ps.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\i386\segment.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\cc.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\config.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ctype.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ex.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\handle.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\id.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ifs.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\io.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\kbd.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\kd.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ke.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ldr.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\mm.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\module.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\nls.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\nt.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ntoskrnl.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ob.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\po.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\pool.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\port.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\ps.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\safe.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\se.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\trap.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\v86m.h
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\include\internal\xhal.h
# End Source File
# End Group
# End Group
# Begin Group "io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\adapter.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\arcname.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\buildirp.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\cancel.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\cleanup.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\cntrller.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\create.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\device.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\deviface.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\dir.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\driver.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\errlog.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\error.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\event.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\file.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\flush.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\fs.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\iocomp.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\ioctrl.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\iomgr.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\iowork.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\irp.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\lock.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\mailslot.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\mdl.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\npipe.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\page.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\parttab.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\pnpdma.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\pnpmgr.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\pnpnotify.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\pnpreport.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\pnproot.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\process.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\queue.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\rawfs.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\remlock.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\resource.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\rw.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\share.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\shutdown.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\symlink.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\timer.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\vpb.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\wdm.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\xhaldisp.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\io\xhaldrv.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\io"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\io"

!ENDIF 

# End Source File
# End Group
# Begin Group "kd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\kd\dlog.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\kd\gdbstub.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\kd\kdebug.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\kd\mda.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\kd\service.c
# End Source File
# End Group
# Begin Group "ke"

# PROP Default_Filter ""
# Begin Group "i386"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\bios.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\brkpoint.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\bthread.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\exp.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\fpu.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\gdt.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\reactos\ntoskrnl\ke\i386\i386-mcount.S"

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\idt.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\irq.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\irqhand.s

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\kernel.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\ldt.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\multiboot.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\stkswitch.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\syscall.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\thread.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\trap.s

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\tskswitch.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\tss.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\usercall.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\usertrap.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\v86m.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\i386\v86m_sup.S

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\apc.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\bug.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\catch.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\critical.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\dpc.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\error.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\event.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\kqueue.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\kthread.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\main.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\mutex.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\process.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\queue.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\sem.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\spinlock.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\timer.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ke\wait.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ke"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ke"

!ENDIF 

# End Source File
# End Group
# Begin Group "ldr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\init.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\loader.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\resource.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\rtl.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\sysdll.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ldr\userldr.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ldr"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ldr"

!ENDIF 

# End Source File
# End Group
# Begin Group "lpc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\close.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\complete.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\connect.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\create.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\listen.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\port.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\query.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\queue.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\receive.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\reply.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\lpc\send.c
# End Source File
# End Group
# Begin Group "mm"

# PROP Default_Filter ""
# Begin Group "i386_mm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\i386\memsafe.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\i386\page.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# SUBTRACT CPP /Gy

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\i386\pfault.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\anonmem.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\aspace.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\balance.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\cont.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\drvlck.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\freelist.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# SUBTRACT CPP /Gy

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\iospace.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\kmap.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\marea.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\mdl.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\mm.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\mminit.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\mpw.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\ncache.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\npool.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# SUBTRACT CPP /Gy

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\pagefile.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\pageop.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\pager.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\pagfault.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\paging.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\pool.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\ppool.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\region.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\rmap.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\section.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\slab.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\virtual.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\mm\wset.c
# End Source File
# End Group
# Begin Group "nt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\channel.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\evtpair.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\misc.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\mutant.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\nt.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\ntevent.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\ntsem.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\nttimer.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\plugplay.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\profile.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\vdm.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\nt\zw.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\nt"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\nt"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "ob"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\dirobj.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\handle.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\namespc.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\ntobj.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\object.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\security.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ob\symlink.c
# End Source File
# End Group
# Begin Group "po"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\po\power.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\po"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\po"

!ENDIF 

# End Source File
# End Group
# Begin Group "ps"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\create.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\debug.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\idle.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\kill.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\locale.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\process.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\psmgr.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\suspend.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\thread.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\tinfo.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\w32call.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ps\win32.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\ps"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\ps"

!ENDIF 

# End Source File
# End Group
# Begin Group "rtl"

# PROP Default_Filter ""
# Begin Group "rtl_i386"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\alldiv.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\allmul.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\allrem.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\allshl.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\allshr.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\aulldiv.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\aullrem.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\aullshr.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\except.s
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\exception.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\i386\seh.s
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\atom.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\bitmap.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\capture.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\compress.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\ctype.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\dos8dot3.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\error.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\handle.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\largeint.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\mem.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\message.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\nls.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\purecall.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\random.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\regio.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\sprintf.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\stdlib.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\string.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\strtok.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\swprintf.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\time.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\timezone.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\unicode.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\rtl\wstring.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# PROP Intermediate_Dir "Release\rtl"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# PROP Intermediate_Dir "Debug\rtl"

!ENDIF 

# End Source File
# End Group
# Begin Group "se"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\access.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\acl.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\audit.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\lsa.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\luid.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\priv.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\sd.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\semgr.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\sid.c
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\se\token.c
# End Source File
# End Group
# Begin Group "MSVC_extras"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ke_i386_bthread.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# ADD CPP /Od /Oy-

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ke_i386_irqhand.c
# End Source File
# Begin Source File

SOURCE=.\ke_i386_multiboot.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

# ADD CPP /Gy

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ke_i386_stkswitch.c
# End Source File
# Begin Source File

SOURCE=.\ke_i386_syscall.c
# End Source File
# Begin Source File

SOURCE=.\ke_i386_trap.c
# End Source File
# Begin Source File

SOURCE=.\ke_i386_tskswitch.c
# End Source File
# Begin Source File

SOURCE=.\ke_i386_v86m_sup.c
# End Source File
# Begin Source File

SOURCE=.\mm_i386_memsafe.c
# End Source File
# Begin Source File

SOURCE=.\nt_zw_msvc.c
# End Source File
# Begin Source File

SOURCE=.\rtl_i386_except.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\bugcodes.rc
# End Source File
# Begin Source File

SOURCE=.\linkorder.txt
# End Source File
# Begin Source File

SOURCE=.\mm_mminit_msvc.c

!IF  "$(CFG)" == "ntoskrnl - Win32 Release"

!ELSEIF  "$(CFG)" == "ntoskrnl - Win32 Debug"

# SUBTRACT CPP /Gy

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ntoskrnl.def
# End Source File
# Begin Source File

SOURCE=..\..\reactos\ntoskrnl\ntoskrnl.mc
# End Source File
# End Target
# End Project
