# Microsoft Developer Studio Project File - Name="tcptest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=tcptest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tcptest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tcptest.mak" CFG="tcptest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tcptest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "tcptest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tcptest - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "tcptest - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../../drivers/lib/oskittcp/include" /I "../../../drivers\lib\oskittcp\include\freebsd\src\sys" /I "../../../drivers\lib\oskittcp\include\freebsd\dev\include" /I "../../../drivers\lib\oskittcp\include\freebsd\net\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "KERNEL" /D __REACTOS__=1 /D "FREEZAP" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "tcptest - Win32 Release"
# Name "tcptest - Win32 Debug"
# Begin Group "tcptest"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tcptest.cpp

!IF  "$(CFG)" == "tcptest - Win32 Release"

!ELSEIF  "$(CFG)" == "tcptest - Win32 Debug"

# SUBTRACT CPP /D "KERNEL"

!ENDIF 

# End Source File
# End Group
# Begin Group "oskittcp"

# PROP Default_Filter ""
# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\defaults.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\in.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\in_cksum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\in_pcb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\in_proto.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\in_rmx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\inet_ntoa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\ip_input.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\ip_output.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\kern_clock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\kern_subr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\param.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\radix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\random.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\raw_cb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\raw_ip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\raw_usrreq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\route.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\rtsock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\scanc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\sleep.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_input.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_output.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_subr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_timer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\tcp_usrreq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\uipc_domain.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\uipc_mbuf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\uipc_socket.c
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\oskittcp\uipc_socket2.c
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Group "freebsd"

# PROP Default_Filter ""
# Begin Group "src No. 1"

# PROP Default_Filter ""
# Begin Group "sys"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\buf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\callout.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\cdefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\domain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\errno.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\fcntl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\file.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\filedesc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\filio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\ioccom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\ioctl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\kernel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\libkern.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\malloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\mbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\param.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\proc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\protosw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\queue.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\resourcevar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\rtprio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\select.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\signal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\signalvar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\socketvar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\sockio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\stat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\sysctl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\syslimits.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\syslog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\systm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\time.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\ttycom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\ucred.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\uio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\freebsd\src\sys\sys\unistd.h
# End Source File
# End Group
# End Group
# End Group
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\memtrack.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\oskitdebug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\oskiterrno.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\oskitfreebsd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\oskittcp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\drivers\lib\oskittcp\include\oskittypes.h
# End Source File
# End Group
# End Group
# End Target
# End Project
