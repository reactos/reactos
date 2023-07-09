# Microsoft Developer Studio Project File - Name="MLang" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MLang - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mlang.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mlang.mak" CFG="MLang - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MLang - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MLang - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MLang - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f MLang.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MLang.exe"
# PROP BASE Bsc_Name "MLang.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "command /E:2048 /C VCBuild retail"
# PROP Rebuild_Opt "all"
# PROP Target_File "Obj\i386\MLang.dll"
# PROP Bsc_Name "MLang.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MLang - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f MLang.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MLang.exe"
# PROP BASE Bsc_Name "MLang.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "command /E:2048 /C VCBuild debug"
# PROP Rebuild_Opt "all"
# PROP Target_File "Objd\i386\MLang.dll"
# PROP Bsc_Name "MLang.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MLang - Win32 Release"
# Name "MLang - Win32 Debug"

!IF  "$(CFG)" == "MLang - Win32 Release"

!ELSEIF  "$(CFG)" == "MLang - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\AttrLoc.cpp
# End Source File
# Begin Source File

SOURCE=.\AttrLoc.h
# End Source File
# Begin Source File

SOURCE=.\AttrStr.cpp
# End Source File
# Begin Source File

SOURCE=.\AttrStr.h
# End Source File
# Begin Source File

SOURCE=.\AttrStrA.cpp
# End Source File
# Begin Source File

SOURCE=.\AttrStrA.h
# End Source File
# Begin Source File

SOURCE=.\AttrStrW.cpp
# End Source File
# Begin Source File

SOURCE=.\attrstrw.h
# End Source File
# Begin Source File

SOURCE=.\codepage.h
# End Source File
# Begin Source File

SOURCE=.\convbase.cpp
# End Source File
# Begin Source File

SOURCE=.\convbase.h
# End Source File
# Begin Source File

SOURCE=.\convinet.cpp
# End Source File
# Begin Source File

SOURCE=.\convobj.cpp
# End Source File
# Begin Source File

SOURCE=.\convobj.h
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\detcbase.cpp
# End Source File
# Begin Source File

SOURCE=.\detcbase.h
# End Source File
# Begin Source File

SOURCE=.\detcjpn.cpp
# End Source File
# Begin Source File

SOURCE=.\detcjpn.h
# End Source File
# Begin Source File

SOURCE=.\detckrn.cpp
# End Source File
# Begin Source File

SOURCE=.\detckrn.h
# End Source File
# Begin Source File

SOURCE=.\dllload.cpp
# End Source File
# Begin Source File

SOURCE=.\enumcp.cpp
# End Source File
# Begin Source File

SOURCE=.\enumcp.h
# End Source File
# Begin Source File

SOURCE=.\eucjobj.cpp
# End Source File
# Begin Source File

SOURCE=.\eucjobj.h
# End Source File
# Begin Source File

SOURCE=.\fechrcnv.h
# End Source File
# Begin Source File

SOURCE=.\hzgbobj.cpp
# End Source File
# Begin Source File

SOURCE=.\hzgbobj.h
# End Source File
# Begin Source File

SOURCE=.\ichrcnv.cpp
# End Source File
# Begin Source File

SOURCE=.\ichrcnv.h
# End Source File
# Begin Source File

SOURCE=.\init.cpp
# End Source File
# Begin Source File

SOURCE=.\jisobj.cpp
# End Source File
# Begin Source File

SOURCE=.\jisobj.h
# End Source File
# Begin Source File

SOURCE=.\kscobj.cpp
# End Source File
# Begin Source File

SOURCE=.\kscobj.h
# End Source File
# Begin Source File

SOURCE=.\mimedb.cpp
# End Source File
# Begin Source File

SOURCE=.\mimedb.h
# End Source File
# Begin Source File

SOURCE=.\mlang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\iedev\inc\mlang.idl
# End Source File
# Begin Source File

SOURCE=.\mlang.rc
# End Source File
# Begin Source File

SOURCE=.\mlatl.h
# End Source File
# Begin Source File

SOURCE=.\mlflink.cpp
# End Source File
# Begin Source File

SOURCE=.\mlflink.h
# End Source File
# Begin Source File

SOURCE=.\mllbcons.cpp
# End Source File
# Begin Source File

SOURCE=.\mllbcons.h
# End Source File
# Begin Source File

SOURCE=.\mlmain.h
# End Source File
# Begin Source File

SOURCE=.\mlsbwalk.h
# End Source File
# Begin Source File

SOURCE=.\mlstr.cpp
# End Source File
# Begin Source File

SOURCE=.\mlstr.h
# End Source File
# Begin Source File

SOURCE=.\mlstra.cpp
# End Source File
# Begin Source File

SOURCE=.\mlstra.h
# End Source File
# Begin Source File

SOURCE=.\mlstrbuf.h
# End Source File
# Begin Source File

SOURCE=.\mlstrw.cpp
# End Source File
# Begin Source File

SOURCE=.\mlstrw.h
# End Source File
# Begin Source File

SOURCE=.\mlswalk.cpp
# End Source File
# Begin Source File

SOURCE=.\mlswalk.h
# End Source File
# Begin Source File

SOURCE=.\private.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\rfc1766.cpp
# End Source File
# Begin Source File

SOURCE=.\utf7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\utf7obj.h
# End Source File
# Begin Source File

SOURCE=.\utf8obj.cpp
# End Source File
# Begin Source File

SOURCE=.\utf8obj.h
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.h
# End Source File
# End Target
# End Project
