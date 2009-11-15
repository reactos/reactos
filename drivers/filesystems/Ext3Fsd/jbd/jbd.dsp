# Microsoft Developer Studio Project File - Name="jbd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=jbd - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jbd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jbd.mak" CFG="jbd - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jbd - Win32 Free" (based on "Win32 (x86) Static Library")
!MESSAGE "jbd - Win32 Checked" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "jbd"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jbd - Win32 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
LINK32=link.exe
# ADD BASE LINK32 /nologo /machine:IX86
# ADD LINK32 ntoskrnl.lib hal.lib wmilib.lib /nologo /base:"" /version:4.0 /entry:"" /pdb:none /debug /debugtype:coff /machine:IX86 /nodefaultlib /out:".\Free\i386\jbd.lib" /libpath:"$(DDKROOT)\lib\w2k\i386" /driver /debug:notmapped,MINIMAL /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# ADD BASE CPP /nologo /Gz /W3 /WX /Oy /Gy /D "WIN32" /D "_WINDOWS" /Oxs /c
# ADD CPP /nologo /Gz /W3 /Oy /Gy /I "..\..\winnt\include" /I "." /I "..\include" /I "..\..\include" /I "$(DDKROOT)\inc\ifs\w2k" /I "$(DDKROOT)\inc\ddk\w2k" /I "$(DDKROOT)\inc\w2k" /D "__WINNT__" /D "__KERNEL__" /D WIN32=100 /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=1 /D "_IDWBUILD" /D "NDEBUG" /D _DLL=1 /D _X86_=1 /FR /Oxs /Zel -cbstring /QIfdiv- /QIf /GF /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(DDKROOT)\inc\ifs\w2k" /i "$(DDKROOT)\inc\ddk\w2k" /i "$(DDKROOT)\inc\w2k" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo /out:"Free\i386\jbd.lib"

!ELSEIF  "$(CFG)" == "jbd - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
LINK32=link.exe
# ADD BASE LINK32 /nologo /machine:IX86
# ADD LINK32 ntoskrnl.lib hal.lib wmilib.lib /base:"" /version:4.0 /entry:"" /incremental:no /pdb:".\Checked\i386\jbd.pdb" /debug /machine:IX86 /nodefaultlib /out:".\Checked\i386\jbd.lib" /libpath:"$(DDKROOT)\lib\w2k\i386" /driver /debug:notmapped,FULL /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# SUBTRACT LINK32 /nologo /pdb:none
# ADD BASE CPP /nologo /Gz /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Zi /Gy /I "." /I "..\include" /I "$(DDKROOT)\inc\ifs\w2k" /I "$(DDKROOT)\inc\ddk\w2k" /I "$(DDKROOT)\inc\w2k" /D "__WINNT__" /D "__KERNEL__" /D WIN32=100 /D "_DEBUG" /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _X86_=1 /D DBG=1 /FR /FD /Zel -cbstring /QIfdiv- /QIf /GF /c
# SUBTRACT CPP /Oi /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(DDKROOT)\inc\ifs\w2k" /i "$(DDKROOT)\inc\ddk\w2k" /i "$(DDKROOT)\inc\w2k" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo /out:"Checked\i386\jbd.lib"

!ENDIF 

# Begin Target

# Name "jbd - Win32 Free"
# Name "jbd - Win32 Checked"
# Begin Group "jbd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\recovery.c
DEP_CPP_RECOV=\
	"..\include\asm\semaphore.h"\
	"..\include\linux\bit_spinlock.h"\
	"..\include\linux\buffer_head.h"\
	"..\include\linux\errno.h"\
	"..\include\linux\fs.h"\
	"..\include\linux\jbd.h"\
	"..\include\linux\journal-head.h"\
	"..\include\linux\list.h"\
	"..\include\linux\lockdep.h"\
	"..\include\linux\module.h"\
	"..\include\linux\mutex.h"\
	"..\include\linux\sched.h"\
	"..\include\linux\slab.h"\
	"..\include\linux\stddef.h"\
	"..\include\linux\time.h"\
	"..\include\linux\timer.h"\
	"..\include\linux\types.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ia64reg.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntdddisk.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntddstor.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntdef.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntnls.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntstatus.h"\
	"c:\winddk\3790\inc\ifs\w2k\ntifs.h"\
	"c:\winddk\3790\inc\w2k\basetsd.h"\
	"c:\winddk\3790\inc\w2k\bugcodes.h"\
	"c:\winddk\3790\inc\w2k\guiddef.h"\
	"c:\winddk\3790\inc\w2k\ntiologc.h"\
	
NODEP_CPP_RECOV=\
	"..\include\linux\jfs_compat.h"\
	".\jfs_user.h"\
	"c:\winddk\3790\inc\ifs\w2k\alpharef.h"\
	

!IF  "$(CFG)" == "jbd - Win32 Free"

!ELSEIF  "$(CFG)" == "jbd - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\replay.c
DEP_CPP_REPLA=\
	"..\include\asm\semaphore.h"\
	"..\include\linux\bit_spinlock.h"\
	"..\include\linux\buffer_head.h"\
	"..\include\linux\debugfs.h"\
	"..\include\linux\errno.h"\
	"..\include\linux\freezer.h"\
	"..\include\linux\fs.h"\
	"..\include\linux\init.h"\
	"..\include\linux\jbd.h"\
	"..\include\linux\journal-head.h"\
	"..\include\linux\kthread.h"\
	"..\include\linux\list.h"\
	"..\include\linux\lockdep.h"\
	"..\include\linux\mm.h"\
	"..\include\linux\module.h"\
	"..\include\linux\mutex.h"\
	"..\include\linux\pagemap.h"\
	"..\include\linux\poison.h"\
	"..\include\linux\proc_fs.h"\
	"..\include\linux\sched.h"\
	"..\include\linux\slab.h"\
	"..\include\linux\stddef.h"\
	"..\include\linux\time.h"\
	"..\include\linux\timer.h"\
	"..\include\linux\types.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ia64reg.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntdddisk.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntddstor.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntdef.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntnls.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntstatus.h"\
	"c:\winddk\3790\inc\ifs\w2k\ntifs.h"\
	"c:\winddk\3790\inc\w2k\basetsd.h"\
	"c:\winddk\3790\inc\w2k\bugcodes.h"\
	"c:\winddk\3790\inc\w2k\guiddef.h"\
	"c:\winddk\3790\inc\w2k\ntiologc.h"\
	
NODEP_CPP_REPLA=\
	"..\include\linux\jfs_compat.h"\
	"c:\winddk\3790\inc\ifs\w2k\alpharef.h"\
	

!IF  "$(CFG)" == "jbd - Win32 Free"

!ELSEIF  "$(CFG)" == "jbd - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\revoke.c
DEP_CPP_REVOK=\
	"..\include\asm\semaphore.h"\
	"..\include\linux\bit_spinlock.h"\
	"..\include\linux\bitops.h"\
	"..\include\linux\buffer_head.h"\
	"..\include\linux\errno.h"\
	"..\include\linux\fs.h"\
	"..\include\linux\init.h"\
	"..\include\linux\jbd.h"\
	"..\include\linux\journal-head.h"\
	"..\include\linux\list.h"\
	"..\include\linux\lockdep.h"\
	"..\include\linux\log2.h"\
	"..\include\linux\module.h"\
	"..\include\linux\mutex.h"\
	"..\include\linux\sched.h"\
	"..\include\linux\slab.h"\
	"..\include\linux\stddef.h"\
	"..\include\linux\time.h"\
	"..\include\linux\timer.h"\
	"..\include\linux\types.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ia64reg.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntdddisk.h"\
	"C:\WINDDK\3790\inc\ddk\w2k\ntddstor.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntdef.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntnls.h"\
	"c:\winddk\3790\inc\ddk\w2k\ntstatus.h"\
	"c:\winddk\3790\inc\ifs\w2k\ntifs.h"\
	"c:\winddk\3790\inc\w2k\basetsd.h"\
	"c:\winddk\3790\inc\w2k\bugcodes.h"\
	"c:\winddk\3790\inc\w2k\guiddef.h"\
	"c:\winddk\3790\inc\w2k\ntiologc.h"\
	
NODEP_CPP_REVOK=\
	"..\include\linux\jfs_compat.h"\
	".\jfs_user.h"\
	"c:\winddk\3790\inc\ifs\w2k\alpharef.h"\
	

!IF  "$(CFG)" == "jbd - Win32 Free"

!ELSEIF  "$(CFG)" == "jbd - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "linux"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\include\linux\bit_spinlock.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\bitops.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\buffer_head.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\config.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\debugfs.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\errno.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\Ext2_fs.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\ext3_fs.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\ext3_fs_i.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\ext3_fs_sb.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\ext3_jbd.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\freezer.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\fs.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\highmem.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\init.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\jbd.h
# End Source File
# Begin Source File

SOURCE="..\include\linux\journal-head.h"
# End Source File
# Begin Source File

SOURCE=..\include\linux\kernel.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\kthread.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\list.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\lockdep.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\log2.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\mm.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\module.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\mutex.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\nls.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\pagemap.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\poison.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\proc_fs.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\sched.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\slab.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\spinlock.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\stddef.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\string.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\time.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\timer.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\types.h
# End Source File
# Begin Source File

SOURCE=..\include\linux\version.h
# End Source File
# End Group
# Begin Group "asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\include\asm\page.h
# End Source File
# Begin Source File

SOURCE=..\include\asm\semaphore.h
# End Source File
# Begin Source File

SOURCE=..\include\asm\uaccess.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\include\common.h
# End Source File
# Begin Source File

SOURCE=..\include\ext2fs.h
# End Source File
# Begin Source File

SOURCE=..\include\ntifs.gnu.h
# End Source File
# Begin Source File

SOURCE=..\include\resource.h
# End Source File
# End Group
# End Target
# End Project
